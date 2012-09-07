#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

#define RH_SENSOR  0
/*
 * humidity measurement
 */
float RH;
/*
 * MPL115A1 register address map
 * and definitions
 */
#define MPL115A1_ENABLE_PIN 9
#define MPL115A1_SELECT_PIN 8

#define MPL115A1_READ_MASK  0x80
#define MPL115A1_WRITE_MASK 0x7F 

#define PRESH   0x00
#define PRESL   0x02
#define TEMPH   0x04
#define TEMPL   0x06

#define A0MSB   0x08
#define A0LSB   0x0A
#define B1MSB   0x0C
#define B1LSB   0x0E
#define B2MSB   0x10
#define B2LSB   0x12
#define C12MSB  0x14
#define C12LSB  0x16
#define C11MSB  0x18
#define C11LSB  0x1A
#define C22MSB  0x1C
#define C22LSB  0x1E
/*
 * Pressure measurement
 */
float P;
#define MAX_PRESSURE  115
#define MIN_PRESSURE  50
/*
 * temperature measurement
 */
float T;

const unsigned long measurementInterval = 23000;
long measurementTime = 0; 

/*
 * pachube definitions
 */
#define FEEDID  <your feed-id>
#define APIKEY "<your api-key>"
/*
 * network stuffs
 */
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xE7, 0x7C };
byte ip[] = { 192, 168, 20, 70 };
byte gw[] = { 192, 168, 20, 254 };
byte dns[] = { 8, 8, 8, 8 };
byte subnet[] = { 255, 255, 255, 0 };

bool reset;

EthernetClient pclient;
EthernetServer wserver(80);

long lastConnectionTime = 0; 
boolean lastConnected = false;
const unsigned long postingInterval = 60000;

/*
 * get humidity measurement from an analog HIH4030 sensor
 * http://sensing.honeywell.com/product%20page?pr_id=53969
 */
void getHIH4030(float temp)
{
  int RHAnalogValue = analogRead(RH_SENSOR);
  float Vout = ((float)RHAnalogValue * 5) / 1023;
  float rH = (Vout - 0.8) / 0.031;
  /* temperature compensation ( T in C ) */
  RH = rH / (1.0546 - 0.00216 * temp);
}

/*
 * get pressure and temperature measurements from an analog MPL115A1 sensor
 * http://www.freescale.com/webapp/sps/site/prod_summary.jsp?code=MPL115A
 */
void get_MPL115A1()
{
  signed char sia0MSB, sia0LSB;
  signed char sib1MSB, sib1LSB;
  signed char sib2MSB, sib2LSB;
  signed char sic12MSB, sic12LSB;
  signed char sic11MSB, sic11LSB;
  signed char sic22MSB, sic22LSB;
  
  signed int sia0, sib1, sib2, sic12, sic11, sic22, siPcomp;
  
  signed long lt1, lt2, lt3, si_c11x1, si_a11, si_c12x2;
  signed long si_a1, si_c22x2, si_a2, si_a1x1, si_y1, si_a2x2;
  
  unsigned char uiPH, uiPL, uiTH, uiTL;
  unsigned int uiPadc, uiTadc;
  
  float curr_p;
  
  /* wake up  MPL115A1 */
  digitalWrite(MPL115A1_ENABLE_PIN, HIGH);
  delay(20);
  /* start temperature and pressure  measurements */
  writeRegister(0x24, 0x00);
  delay(2);
  
  uiPH = readRegister(PRESH);
  uiPL = readRegister(PRESL);
  uiPadc = (unsigned int) uiPH << 8;
  uiPadc += (unsigned int) uiPL & 0x00FF;
  uiPadc = uiPadc >> 6;
    
  uiTH = readRegister(TEMPH);
  uiTL = readRegister(TEMPL);
  uiTadc = (unsigned int) uiTH << 8;
  uiTadc += (unsigned int) uiTL & 0x00FF;
  uiTadc = uiTadc >> 6;
  
  /* a0 */
  sia0MSB = readRegister(A0MSB);
  sia0LSB = readRegister(A0LSB);
  sia0 = (signed int) sia0MSB << 8;
  sia0 += (signed int) sia0LSB & 0x00FF;
  /* b1 */
  sib1MSB = readRegister(B1MSB);
  sib1LSB = readRegister(B1LSB);
  sib1 = (signed int) sib1MSB << 8;
  sib1 += (signed int) sib1LSB & 0x00FF;
  /* b2 */
  sib2MSB = readRegister(B2MSB);
  sib2LSB = readRegister(B2LSB);
  sib2 = (signed int) sib2MSB << 8;
  sib2 += (signed int) sib2LSB & 0x00FF;
  /* c12 */
  sic12MSB = readRegister(C12MSB);
  sic12LSB = readRegister(C12LSB);
  sic12 = (signed int) sic12MSB << 8;
  sic12 += (signed int) sic12LSB & 0x00FF;
  /* c11 */
  sic11MSB = readRegister(C11MSB);
  sic11LSB = readRegister(C11LSB);
  sic11 = (signed int) sic11MSB << 8;
  sic11 += (signed int) sic11LSB & 0x00FF;
  /* c22 */
  sic22MSB = readRegister(C22MSB);
  sic22LSB = readRegister(C22LSB);
  sic22 = (signed int) sic22MSB << 8;
  sic22 += (signed int) sic22LSB & 0x00FF;

  /* put MPL115A1 to sleep */
  digitalWrite(MPL115A1_ENABLE_PIN, LOW);
    
  /* Step 1 c11x1 = c11 * Padc */
  lt1 = (signed long) sic11;
  lt2 = (signed long) uiPadc;
  lt3 = lt1 * lt2;
  si_c11x1 = (signed long) lt3;
  
  /* Step 2 a11 = b1 + c11x1 */
  lt1 = ((signed long) sib1) << 14;
  lt2 = (signed long) si_c11x1;
  lt3 = lt1 + lt2;
  si_a11 = (signed long)(lt3 >> 14);
    
  /* Step 3 c12x2 = c12 * Tadc */
  lt1 = (signed long) sic12;
  lt2 = (signed long) uiTadc;
  lt3 = lt1 * lt2;
  si_c12x2 = (signed long) lt3;
    
  /* Step 4 a1 = a11 + c12x2 */
  lt1 = ((signed long) si_a11 << 11);
  lt2 = (signed long) si_c12x2;
  lt3 = lt1 + lt2;
  si_a1 = (signed long) lt3 >> 11;
  
  /* Step 5 c22x2 = c22 * Tadc */
  lt1 = (signed long) sic22;
  lt2 = (signed long) uiTadc;
  lt3 = lt1 * lt2;
  si_c22x2 = (signed long)(lt3);
    
  /* Step 6 a2 = b2 + c22x2 */
  lt1 = ((signed long) sib2 << 15);
  lt2 = ((signed long) si_c22x2 >> 1);
  lt3 = lt1 + lt2;
  si_a2 = ((signed long)lt3 >> 16);
    
  /* Step 7 a1x1 = a1 * Padc */
  lt1 = (signed long) si_a1;
  lt2 = (signed long) uiPadc;
  lt3 = lt1 * lt2;
  si_a1x1 = (signed long)(lt3);
    
  /* Step 8 y1 = a0 + a1x1 */
  lt1 = ((signed long) sia0 << 10);
  lt2 = (signed long) si_a1x1;
  lt3 = lt1 + lt2;
  si_y1 = ((signed long) lt3 >> 10);
    
  /* Step 9 a2x2 = a2 * Tadc */
  lt1 = (signed long) si_a2;
  lt2 = (signed long) uiTadc;
  lt3 = lt1 * lt2;
  si_a2x2 = (signed long)(lt3);
    
  /* Step 10 pComp = y1 + a2x2 */
  lt1 = ((signed long) si_y1 << 10);
  lt2 = (signed long) si_a2x2;
  lt3 = lt1 + lt2;
    
  /* Fixed point result with rounding */
  siPcomp = lt3 >> 13;
    
  curr_p = ((65.0 / 1023.0) * siPcomp) + 50;
  if (curr_p <= MAX_PRESSURE && curr_p >= MIN_PRESSURE)
    P = curr_p;

  T = 25 + (uiTadc - 472) / -5.35;
}

unsigned int readRegister(byte thisRegister)
{
  byte result = 0;
  
  digitalWrite(MPL115A1_SELECT_PIN, LOW);
  SPI.transfer(thisRegister | MPL115A1_READ_MASK);
  result = SPI.transfer(0x00);
  digitalWrite(MPL115A1_SELECT_PIN, HIGH);
    
  return result;  
}

void writeRegister(byte thisRegister, byte thisValue)
{
  digitalWrite(MPL115A1_SELECT_PIN, LOW);
  SPI.transfer(thisRegister & MPL115A1_WRITE_MASK);
  SPI.transfer(thisValue);
  digitalWrite(MPL115A1_SELECT_PIN, HIGH);
}

void get_measurements()
{
  if (millis() - measurementTime > measurementInterval) {
    get_MPL115A1();
    getHIH4030(T);
    Serial.print("T=");
    Serial.print(T);
    Serial.print(" C - P=");
    Serial.print(P);
    Serial.print(" kPa - RH=");
    Serial.print(RH);
    Serial.println(" %");
    measurementTime = millis();
  }
}

/*
 * send measurements to pachube data-logging web service
 */
void sendData(String data) {
  if (pclient.connect("api.pachube.com", 80)) {
    Serial.println("connecting...");
    Serial.println(data);
    pclient.print("PUT /v2/feeds/");
    pclient.print(FEEDID);
    pclient.println(".csv HTTP/1.1");
    pclient.println("Host: api.pachube.com");

    pclient.print("X-PachubeApiKey: ");
    pclient.println(APIKEY);
    pclient.print("Content-Length: ");
    pclient.println(data.length(), DEC);

    pclient.println("Content-Type: text/csv");
    pclient.println("Connection: close\n");
    pclient.println(data);
  } else
    Serial.println("connection failed");
}

void pachubeClient()
{
  if (pclient.available()) {
    char c = pclient.read();
    Serial.print(c);
  }
  if (!pclient.connected() && lastConnected) {
    reset = false;
    Serial.println();
    Serial.println("disconnecting.");
    pclient.stop();
  }

  if (!pclient.connected() && (millis() - lastConnectionTime > postingInterval)) {
    reset = true;
    String datastring = "temperature, ";
    datastring += (int)T;
    datastring += "\nrlHumidity, ";
    datastring += (int)RH;
    datastring += "\npressure, ";
    datastring += (int)P;
    sendData(datastring);
    lastConnectionTime = millis();
  }
  
  lastConnected = pclient.connected();
}

void setup() {
  Serial.begin(115200);
  Serial.println("***** Setup *****");
  
  wdt_enable(WDTO_8S);
  reset = false;  
  
  Ethernet.begin(mac, ip, dns, gw, subnet);
  
  SPI.begin();
  pinMode(MPL115A1_SELECT_PIN, OUTPUT);
  pinMode(MPL115A1_ENABLE_PIN, OUTPUT);    
  digitalWrite(MPL115A1_ENABLE_PIN, LOW);
  digitalWrite(MPL115A1_SELECT_PIN, HIGH);
}

void loop()
{
 if (reset == false)
    wdt_reset();
  get_measurements();
  pachubeClient();
}
