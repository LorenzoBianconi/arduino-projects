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
const static byte relayMap[] = { 5, 4, 7, 6 };
enum chanState {
        CHAN_CLOSED = 0,
        CHAN_OPEN,
};
static enum chanState channelState[] = { CHAN_CLOSED, CHAN_CLOSED };
#define CHAN_DEPTH      2
#define LOOP_DELAY      1000
#define PULSE_DEPTH     1500

String logInfo[CHAN_DEPTH];

struct timeSlot {
        byte enabled;
        byte hStart;
        byte mStart;
        byte hStop;
        byte mStop;
};

#define TIMESLOT_MAP_MAXDEPTH      4
struct timeSlot gardenTimeTable[CHAN_DEPTH][TIMESLOT_MAP_MAXDEPTH] = {};

/* Bluetooth stuff */
#define BT_LINK_PIN     A0
byte btConnected;

/* SD stuff */
const byte chipSelect = 10;
#define CONFIG_FILE     "GARDEN.XML"

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

        /* setup intial values for relay channels */
        for (byte i = 0; i < 2 * CHAN_DEPTH; i++) {
             pinMode(relayMap[i], OUTPUT);
             digitalWrite(relayMap[i], LOW);   
        }
        /* close valves at bootstrap */
        for (byte i = 0; i < CHAN_DEPTH; i++)
                relayPulse(relayMap[2 * i + 1]);

        Wire.begin();
        rtc.begin();
        if (!rtc.isrunning())
                rtc.adjust(DateTime(__DATE__, __TIME__));

        if (SD.begin(chipSelect)) {
                /* load channel configuration */
                loadConfiguration();
                /* load channel log */
                for (byte i = 0; i < CHAN_DEPTH; i++)
                        logInfo[i] = loadChanLog(i);
        }

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
                for (byte i = 0; i < CHAN_DEPTH; i++) {
                        for (byte j = 0; j < TIMESLOT_MAP_MAXDEPTH; j++) {
                                xml = getXmlTimeSlotConf(i, j);
                                configFile.println(xml);
                        }
                }
                configFile.close();
        }
}

void saveChannelLog(byte index, bool start) {
        if (start) {
                logInfo[index] = (getCurrTime() + " - ");
        } else {
                logInfo[index] += getCurrTime();
                /* save channel log to SD */
                String fileName = "CHAN" + String(index) + ".LOG";
                SD.remove(fileName);
                File logFile = SD.open(fileName, FILE_WRITE);
                if (logFile) {
                        logFile.println(logInfo[index]);
                        logFile.close();
                }
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

String loadChanLog(int index) {
        String ret = "", fileName = "CHAN" + String(index) + ".LOG";
        File logFile = SD.open(fileName);
        if (logFile) {
                while (logFile.available()) {
                        char data = logFile.read();
                        if (data == '\r' || data == '\n')
                                continue;
                        ret += String(data);
                }
                logFile.close(); 
        }
        return ret;
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

String getXmlChanLog(int index) {
        String xml = "<log chan=\"";
        xml += String(index);
        xml += "\" log=\"";
        xml += logInfo[index];
        xml += "\"/>";

        return xml;
}

void sendXml() {
        String xml = "";
        for (byte i = 0; i < CHAN_DEPTH; i++) {
                for (byte j = 0; j < TIMESLOT_MAP_MAXDEPTH; j += 2) {
                        xml = getXmlTimeSlotConf(i, j);
                        xml += getXmlTimeSlotConf(i, j + 1);
                        Serial.println(xml);
                        Serial.flush();
                }
                xml = getXmlChanLog(i);
                Serial.println(xml);
                Serial.flush();
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
        byte chInfo = (xml.substring(sd + 1, sd + 2).toInt() % CHAN_DEPTH);
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

        /*currTime += String(now.year());
        currTime += "/";
        currTime += String(now.month());
        currTime += "/";
        currTime += String(now.day());
        currTime += " ";*/
        currTime += String(now.hour());
        currTime += ":";
        currTime += String(now.minute());

        return currTime;
}

int parseRxBuffer(String data) {
        if (data.startsWith("<SET")) {
                int i = 0, closeTag;

                while (i < data.length()) {
                    closeTag = data.indexOf("/>", i) + 2;
                    if (closeTag < 0)
                        return closeTag;

                    int err = parseTimeSlotXml(data.substring(i, closeTag));
                    if (err < 0)
                        return err;
                    i = closeTag;
                }
                saveConfiguration();
        } else if (data.startsWith("<GET>")) {
                sendXml();
        } else if (data.startsWith("<TIME")) {
                updateRtcTime(data);
                delay(500);
                sendXml();
        }
        return 0;
}

void relayPulse(byte index) {
        digitalWrite(index, HIGH);
        delay(PULSE_DEPTH);
        digitalWrite(index, LOW);
}

void setRelay(int index, int hTime, int sTime) {
        enum chanState state = CHAN_CLOSED;
        int currTime = hTime * 60 + sTime;

        for (byte i = 0; i < TIMESLOT_MAP_MAXDEPTH; i++) {
                if (!gardenTimeTable[index][i].enabled)
                        continue;

                int startTime = 60 * gardenTimeTable[index][i].hStart +
                                gardenTimeTable[index][i].mStart;
                int stopTime = 60 * gardenTimeTable[index][i].hStop +
                               gardenTimeTable[index][i].mStop;
                if (currTime >= startTime && currTime <= stopTime)
                        state = CHAN_OPEN;
        }
        if (state != channelState[index]) {
                int relay_idx = (state == CHAN_OPEN) ? index : index + 1;
                relayPulse(relay_idx);
                channelState[index] = state;

                saveChannelLog(index, (state == CHAN_OPEN));

                /* send event over bluetooth */
                if (btConnected) {
                        String xml = getXmlChanLog(index);
                        Serial.println(xml);
                        Serial.flush();
                }
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
                for (byte i = 0; i < CHAN_DEPTH; i++)
                        setRelay(i, now.hour(), now.minute());
        }
        delay(LOOP_DELAY);
}
