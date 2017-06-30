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
#define LOOP_DELAY      500

struct timeSlot {
        bool enabled;
        int hStart;
        int mStart;
        int hStop;
        int mStop;
};

#define TIMESLOT_MAP_MAXDEPTH      4
struct timeSlot gadenTimeTable[][TIMESLOT_MAP_MAXDEPTH] = {
        /* relay D4 */
        {
                [0] = { true, 10, 20, 30, 40 },
                [1] = { true, 10, 20, 30, 40 },
                [2] = { true, 10, 20, 3, 40 },
                [3] = { true, 10, 20, 30, 40 },
        },
        /* relay D5 */
        {
                [0] = { false, 10, 20, 30, 40 },
                [1] = { true, 10, 20, 30, 40 },
                [2] = { true, 10, 20, 50, 40 },
                [3] = { false, 10, 20, 30, 40 },
        },
        /* relay D6 */
        {
                [0] = { false, 10, 20, 30, 40 },
                [1] = { true, 10, 20, 30, 40 },
                [2] = { false, 10, 20, 30, 40 },
                [3] = { true, 10, 20, 30, 40 },
        },
        /* relay D7 */
        {
                [0] = { false, 10, 20, 30, 40 },
                [1] = { false, 10, 20, 50, 40 },
                [2] = { true, 20, 20, 30, 40 },
                [3] = { false, 10, 20, 30, 40 },
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
        /* permit autoconnect to paired device */
        sendBtCmd("\r\n+STAUTO=0\r\n");
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
                xml += "<ts chan=\"";
                xml += index;
                xml += "\" idx=\"";
                xml += i;
                xml += "\" en=\"";
                xml += gadenTimeTable[index][i].enabled;
                xml += "\" bt=\"";
                xml += gadenTimeTable[index][i].hStart;
                xml += ":";
                xml += gadenTimeTable[index][i].mStart;
                xml += "\" et=\"";
                xml += gadenTimeTable[index][i].hStop;
                xml += ":";
                xml += gadenTimeTable[index][i].mStop;
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

int parseTimeSlotXml(String xml) {
        int sd, td;

        /* channel info */
        Serial.println(xml);
        sd = xml.indexOf("\"");
        if (sd < 0)
                return -1;
        int chInfo = (xml.substring(sd + 1, sd + 2).toInt() % RELAY_DEPTH);
        /* index info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        int indexInfo = (xml.substring(sd + 1, sd + 2).toInt() % TIMESLOT_MAP_MAXDEPTH);
        /* enable info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        int enableInfo = xml.substring(sd + 1, sd + 2).toInt();
        /* start_time info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        String startInfo = xml.substring(sd + 1, sd + 6);
        td = startInfo.indexOf(":");
        if (td < 0)
                return -1;
        int hStart = startInfo.substring(0, td).toInt();
        int mStart = startInfo.substring(td + 1).toInt();
        /* stop_time info */
        sd = xml.indexOf("\"", sd + 7);
        if (sd < 0)
                return -1;
        String stopInfo = xml.substring(sd + 1, sd + 6);
        td = startInfo.indexOf(":");
        if (td < 0)
                return -1;
        int hStop = stopInfo.substring(0, td).toInt();
        int mStop = stopInfo.substring(td + 1).toInt();

        /* update timetable */
        gadenTimeTable[chInfo][indexInfo].enabled = enableInfo;
        gadenTimeTable[chInfo][indexInfo].hStart = hStart;
        gadenTimeTable[chInfo][indexInfo].mStart = mStart;
        gadenTimeTable[chInfo][indexInfo].hStop = hStop;
        gadenTimeTable[chInfo][indexInfo].mStop = mStop;
}

void parseRxBuffer(String data) {
        if (data.startsWith("<SET")) {
                int i = 0, closeTag;

                while (i < data.length()) {
                    closeTag = data.indexOf("/>", i) + 2;
                    if (closeTag < 0)
                        break;

                    int err = parseTimeSlotXml(data.substring(i + 5, closeTag));
                    if (err < 0)
                        break;
                    i = closeTag;
                }
        } else if (data.startsWith("<GET>")) {
                sendXmlConf();
        }
}

void setRelay(int index, int hTime, int sTime) {
        int val = LOW;

        for (int i = 0; i < TIMESLOT_MAP_MAXDEPTH; i++) {
                if (!gadenTimeTable[index][i].enabled)
                        continue;

                if (hTime >= gadenTimeTable[index][i].hStart &&
                    sTime >= gadenTimeTable[index][i].mStart &&
                    hTime <= gadenTimeTable[index][i].hStop &&
                    sTime >= gadenTimeTable[index][i].mStop)
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
                delay(1000);
                resetBtLink();
        }
        btConnected = btLink;

       if (btConnected) {
                String data = Serial.readString();
                parseRxBuffer(data);
                Serial.println(data);
        }

        for (int i = 0; i < RELAY_DEPTH; i++)
                setRelay(i, millis() / 1000, millis() % 1000);

        delay(LOOP_DELAY);
}
