/*
 * gardenino - automatic irrigation system
 * me@lorenzobianconi.net
 *
 */

/*
 * makerstudio relay shiled v1.1 (www.makerstudio.cc)
 * D4: digital pin 4
 * D5: digital pin 5
 * D6: digital pin 6
 * D7: digital pin 7
 */
const static int relayMap[] = { 4, 5, 6, 7 };
static int relayValue[] = { LOW, LOW, LOW, LOW };
#define RELAY_DEPTH     4
#define RELAY_MASK      0xf

#define LOOP_DELAY      500

struct timeSlot {
        bool enabled;
        int hStart;
        int mStart;
        int hStop;
        int mStop;
};

#define TIMESLOT_MAP_MAXDEPTH      4
struct timeTableElem {
          struct timeSlot tsList[TIMESLOT_MAP_MAXDEPTH];
          bool enabled;
};

const struct timeTableElem gadenTimeTable[] = {
        /* relay D4 */
        {
                .tsList = {
                        [0] = { true, 10, 20, 30, 40 },
                        [1] = { true, 10, 20, 30, 40 },
                        [2] = { true, 10, 20, 30, 40 },
                        [3] = { true, 10, 20, 30, 40 },
                },
                .enabled = true,
        },
        /* relay D5 */
        {
                 .tsList = {
                        [0] = { false, 10, 20, 30, 40 },
                        [1] = { false, 10, 20, 30, 40 },
                        [2] = { false, 10, 20, 50, 40 },
                        [3] = { false, 10, 20, 30, 40 },
                },
                .enabled = false,
        },
        /* relay D6 */
        {
                 .tsList = {
                        [0] = { false, 10, 20, 30, 40 },
                        [1] = { true, 10, 20, 30, 40 },
                        [2] = { false, 10, 20, 30, 40 },
                        [3] = { true, 10, 20, 30, 40 },
                },
                .enabled = false,
        },
        /* relay D7 */
        {
                 .tsList = {
                        [0] = { false, 10, 20, 30, 40 },
                        [1] = { false, 10, 20, 50, 40 },
                        [2] = { true, 20, 20, 30, 40 },
                        [3] = { false, 10, 20, 30, 40 },
                },
                .enabled = false,
        },

};

/* Bluetooth stuff */
#define BT_LINK_PIN     A5
bool btConnected;

void sendBtCmd(char *cmd)
{
        Serial.print(cmd);
        delay(3000);
}

void resetBtLink() {
        /* set device as slave */
        sendBtCmd("\r\n+STWMOD=0\r\n");
        /* set nick */
        sendBtCmd("\r\n+STNA=gardenino\r\n");
        sendBtCmd("\r\n+STAUTO=0\r\n");
        /* permit autoconnect to paired device */
        sendBtCmd("\r\n+STPIN=0000\r\n");
        sendBtCmd("\r\n+INQ=0\r\n");
        delay(2000);
        sendBtCmd("\r\n+INQ=1\r\n");
        delay(2000);
}

void setup() {
        Serial.begin(38400);
        Serial.setTimeout(500);

        for (int i = 0; i < RELAY_DEPTH; i++) {
             pinMode(relayMap[i], OUTPUT);
             digitalWrite(relayMap[i], LOW);   
        }

        /* init bt connection */
        btConnected = false;
        delay(1000);
        resetBtLink();
}

String getXmlChannelConf(int index) {
        String xml = "";

        for (int i = 0; i < TIMESLOT_MAP_MAXDEPTH; i++) {
                xml += "<time_slot channel=\"";
                xml += index;
                xml += "\" channel_enabled=\"";
                xml += gadenTimeTable[index].enabled;
                xml += "\" index=\"";
                xml += i;
                xml += "\" enabled=\"";
                xml += gadenTimeTable[index].tsList[i].enabled;
                xml += "\" start_time=\"";
                xml += gadenTimeTable[index].tsList[i].hStart;
                xml += "\" stop_time=\"";
                xml += gadenTimeTable[index].tsList[i].hStop;
                xml += "\"/>";
        }

        return xml;
}

void sendXmlConf() {        
        for (int i = 0; i < RELAY_DEPTH; i++) {
                String xml = getXmlChannelConf(i);
                Serial.println(xml);
                delay(200);
        }
}

void setRelay(int index, int hTime, int sTime) {
        int val = LOW;

        for (int i = 0; i < TIMESLOT_MAP_MAXDEPTH; i++) {
                if (hTime >= gadenTimeTable[index].tsList[i].hStart &&
                    sTime >= gadenTimeTable[index].tsList[i].mStart &&
                    hTime <= gadenTimeTable[index].tsList[i].hStop &&
                    sTime >= gadenTimeTable[index].tsList[i].mStop)
                        val = HIGH;
        }
        if (val != relayValue[index]) {
                digitalWrite(relayMap[index], val);
                relayValue[index] = val;
        }
}

void loop() {
        bool btLink = digitalRead(BT_LINK_PIN);

        if (!btLink && btConnected) {
                btConnected = false;
                resetBtLink();
        } else {
                btConnected = btLink;
        }

        if (btConnected) {
                String data = Serial.readString();
                if (data == "GET")
                        sendXmlConf();
        }

        for (int i = 0; i < RELAY_DEPTH; i++) {
                if (!gadenTimeTable[i].enabled)
                        continue;
                setRelay(i, millis() / 1000, millis() % 1000);
        }
        delay(LOOP_DELAY);
}
