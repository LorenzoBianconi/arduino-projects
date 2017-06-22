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

#define LOOP_DELAY      1000

struct timeSlot {
        int hStart;
        int mStart;
        int hStop;
        int mStop;
};

#define TIMESLOT_MAP_MAXDEPTH      10
struct timeTableElem {
          struct timeSlot tsList[TIMESLOT_MAP_MAXDEPTH];
          int len;
};

const struct timeTableElem gadenTimeTable[] = {
        /* relay D4 */
        {
                .tsList = {
                        [0] = { 1, 0, 10, 0},
                        [1] = { 60, 0, 70, 0},
                        [2] = { 110, 0, 120, 0},
                },
                .len = 3,
        },
        /* relay D5 */
        {
                .tsList = {
                        [0] = { 15, 0, 25, 0},
                        [1] = { 75, 0, 85, 0},
                },
                .len = 2,
        },
        /* relay D6 */
        {
                .tsList = {
                        [0] = { 30, 0, 40, 0},
                        [1] = { 80, 0, 100, 0},
                },
                .len = 2,
        },
        /* relay D7 */
        {
                .tsList = {
                        [0] = { 45, 0, 55, 0},
                        [1] = { 105, 0, 115, 0},
                },
                .len = 2,
        },
};

void setup() {
        Serial.begin(115200);
        Serial.println("setup");

        for (int i = 0; i < RELAY_DEPTH; i++) {
             pinMode(relayMap[i], OUTPUT);
             digitalWrite(relayMap[i], LOW);   
        }
}

void setRelay(int index, int hTime, int sTime) {
        int val = LOW;

        for (int i = 0; i < gadenTimeTable[index].len; i++) {
                if (hTime >= gadenTimeTable[index].tsList[i].hStart &&
                    sTime >= gadenTimeTable[index].tsList[i].mStart &&
                    hTime <= gadenTimeTable[index].tsList[i].hStop &&
                    sTime >= gadenTimeTable[index].tsList[i].mStop)
                        val = HIGH;
        }
        if (val != relayValue[index]) {
                Serial.print("setting relay");
                Serial.print(" D" + String(index + 4));
                Serial.println(" to " + String(val));

                digitalWrite(relayMap[index], val);
                relayValue[index] = val;
        }
}

void loop() {
        for (int i = 0; i < RELAY_DEPTH; i++) {
                if (!bitRead(RELAY_MASK, i))
                        continue;
                setRelay(i, millis() / 1000, millis() % 1000);
        }
        delay(LOOP_DELAY);
}
