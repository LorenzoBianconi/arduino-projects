#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

/*
 * gardenino - automatic irrigation system
 * me@lorenzobianconi.net
 *
 */

/*
 * Haljia RTC logger
 */
RTC_DS1307 rtc;

/*
 * makerstudio relay shiled v1.1 (www.makerstudio.cc)
 * D4: digital pin 4
 * D5: digital pin 5
 * D6: digital pin 6
 * D7: digital pin 7
 */
const static byte relayMap[] = { 4, 5, 6, 7 };
static byte relayValue[] = { LOW, LOW, LOW, LOW };
#define RELAY_DEPTH     4
#define LOOP_DELAY      1000

struct timeSlot {
        byte enabled;
        byte hStart;
        byte mStart;
        byte hStop;
        byte mStop;
};

#define TIMESLOT_MAP_MAXDEPTH      4
struct timeSlot gardenTimeTable[RELAY_DEPTH][TIMESLOT_MAP_MAXDEPTH] = {};

/* Bluetooth stuff */
#define BT_LINK_PIN     A0
byte btConnected;

/* SD stuff */
const byte chipSelect = 10;
#define CONFIG_FILE     "garden.xml"

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

        for (byte i = 0; i < RELAY_DEPTH; i++) {
             pinMode(relayMap[i], OUTPUT);
             digitalWrite(relayMap[i], LOW);   
        }

        Wire.begin();
        rtc.begin();
        if (!rtc.isrunning()) {
                rtc.adjust(DateTime(__DATE__, __TIME__));
        }

        if (SD.begin(chipSelect))
                loadConfiguration();

        /* init bt connection */
        btConnected = false;
        delay(1000);
        resetBtLink();
}


void saveConfiguration() {
        SD.remove(CONFIG_FILE);
        File configFile = SD.open(CONFIG_FILE, FILE_WRITE);
        if (configFile) {
                String xml;
                for (byte i = 0; i < RELAY_DEPTH; i++) {
                        for (byte j = 0; j < TIMESLOT_MAP_MAXDEPTH; j++) {
                                xml = getXmlTimeSlotConf(i, j);
                                configFile.println(xml);
                        }
                }
                configFile.close();
        }
}

void loadConfiguration() {
        File configFile = SD.open(CONFIG_FILE);
        if (configFile) {
                String timeslot = "";
                while (configFile.available()) {
                        char data = configFile.read();
                        if (data == '\n') {
                                parseTimeSlotXml(timeslot);
                                timeslot = "";
                        } else {
                                timeslot += String(data);
                        }
                }
                configFile.close();
        }
}

String getXmlTimeSlotConf(byte channel, byte timeSlot) {
        String xml = "<ts chan=\"";
        xml += channel;
        xml += "\" idx=\"";
        xml += timeSlot;
        xml += "\" en=\"";
        xml += gardenTimeTable[channel][timeSlot].enabled;
        xml += "\" bt=\"";
        xml += gardenTimeTable[channel][timeSlot].hStart;
        xml += ":";
        xml += gardenTimeTable[channel][timeSlot].mStart;
        xml += "\" et=\"";
        xml += gardenTimeTable[channel][timeSlot].hStop;
        xml += ":";
        xml += gardenTimeTable[channel][timeSlot].mStop;
        xml += "\"/>";

        return xml;
}

void sendXmlConf() {
        String xml = "";
        for (byte i = 0; i < RELAY_DEPTH; i++) {
                for (byte j = 0; j < TIMESLOT_MAP_MAXDEPTH; j += 2) {
                        xml = getXmlTimeSlotConf(i, j);
                        xml += getXmlTimeSlotConf(i, j + 1);
                        Serial.println(xml);
                }
        }
}

char parseTimeSlotXml(String xml) {
        int sd, td;
        String data;

        /* sanity check */
        sd = xml.indexOf("/>");
        if (sd < 0)
                return -1;
        /* channel info */
        sd = xml.indexOf("\"");
        if (sd < 0)
                return -1;
        byte chInfo = (xml.substring(sd + 1, sd + 2).toInt() % RELAY_DEPTH);
        /* index info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        byte indexInfo = (xml.substring(sd + 1, sd + 2).toInt() % TIMESLOT_MAP_MAXDEPTH);
        /* enable info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        byte enableInfo = xml.substring(sd + 1, sd + 2).toInt();
        /* start_time info */
        sd = xml.indexOf("\"", sd + 3);
        if (sd < 0)
                return -1;
        data = xml.substring(sd + 1, sd + 6);
        td = data.indexOf(":");
        if (td < 0)
                return -1;
        byte hStart = data.substring(0, td).toInt();
        byte mStart = data.substring(td + 1).toInt();
        /* stop_time info */
        sd = xml.indexOf("\"", sd + 7);
        if (sd < 0)
                return -1;
        data = xml.substring(sd + 1, sd + 6);
        td = data.indexOf(":");
        if (td < 0)
                return -1;
        byte hStop = data.substring(0, td).toInt();
        byte mStop = data.substring(td + 1).toInt();

        /* update timetable */
        gardenTimeTable[chInfo][indexInfo].enabled = enableInfo;
        gardenTimeTable[chInfo][indexInfo].hStart = hStart;
        gardenTimeTable[chInfo][indexInfo].mStart = mStart;
        gardenTimeTable[chInfo][indexInfo].hStop = hStop;
        gardenTimeTable[chInfo][indexInfo].mStop = mStop;

        return 0;
}

int updateRtcTime(String xml) {
        String data;
        int sd;

        sd = xml.indexOf("/>");
        if (sd < 0)
                return -1;

        sd = xml.indexOf("\"");
        if (sd < 0)
                return -1;

        data = xml.substring(sd + 1, sd + 11);
        String currDay = data.substring(0, 2);
        String currMonth = data.substring(3, 5);
        String currYear = data.substring(6, 10);

        sd = xml.indexOf(" ", sd + 1);
        if (sd < 0)
                return -1;
        data = xml.substring(sd + 1, sd + 6);
        String currHour = data.substring(0, 2);
        String currMin = data.substring(3, 5);

        rtc.adjust(DateTime(currYear.toInt(), currMonth.toInt(), currDay.toInt(),
                            currHour.toInt(), currMin.toInt(), 0));
}

String getCurrTime() {
        String currTime = "";
        DateTime now = rtc.now();

        currTime += String(now.year());
        currTime += "/";
        currTime += String(now.month());
        currTime += "/";
        currTime += String(now.day());
        currTime += " ";
        currTime += String(now.hour());
        currTime += ":";
        currTime += String(now.minute());
        currTime += ":";
        currTime += String(now.second());

        return currTime;
}

void parseRxBuffer(String data) {
        if (data.startsWith("<SET")) {
                int i = 0, closeTag;

                while (i < data.length()) {
                    closeTag = data.indexOf("/>", i) + 2;
                    if (closeTag < 0)
                        return;

                    int err = parseTimeSlotXml(data.substring(i, closeTag));
                    if (err < 0)
                        return;
                    i = closeTag;
                }
                saveConfiguration();
        } else if (data.startsWith("<GET>")) {
                sendXmlConf();
        } else if (data.startsWith("<TIME")) {
                updateRtcTime(data);
                delay(500);
                sendXmlConf();
        }
}

void setRelay(int index, int hTime, int sTime) {
        int currTime = hTime * 60 + sTime;
        byte val = LOW;

        for (byte i = 0; i < TIMESLOT_MAP_MAXDEPTH; i++) {
                if (!gardenTimeTable[index][i].enabled)
                        continue;

                int startTime = 60 * gardenTimeTable[index][i].hStart +
                                gardenTimeTable[index][i].mStart;
                int stopTime = 60 * gardenTimeTable[index][i].hStop +
                               gardenTimeTable[index][i].mStop;
                if (currTime >= startTime && currTime <= stopTime)
                        val = HIGH;
        }
        if (val != relayValue[index]) {
                digitalWrite(relayMap[index], val);
                relayValue[index] = val;
        }
}

void loop() {
        byte btLink = digitalRead(BT_LINK_PIN);

        if (!btLink && btConnected) {
                delay(1000);
                resetBtLink();
        }
        btConnected = btLink;

       if (btConnected) {
                String data = Serial.readString();
                parseRxBuffer(data);
        }

        if (rtc.isrunning()) {
                DateTime now = rtc.now();
                for (byte i = 0; i < RELAY_DEPTH; i++)
                        setRelay(i, now.hour(), now.minute());
        }
        delay(LOOP_DELAY);
}
