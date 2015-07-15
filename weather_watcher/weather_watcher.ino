/**
 * LM335A Temeprature sensor
 * https://www.sparkfun.com/products/retired/9438
 */
#define TEMP_PIN  A1
float cur_temp;

float get_temp_lm335a() {
  int val = analogRead(TEMP_PIN);
  float temp_v = (val / 1023.0) * 5000;
  float temp_k = temp_v / 10;
  float temp_c = temp_k - 273.15;

  return temp_c;
}

/*
 * HIH4030 humidity sensor
 * http://sensing.honeywell.com/product%20page?pr_id=53969
 */
#define RH_PIN  A0

float get_rh_HIH4030(float temp)
{
  int val = analogRead(RH_PIN);
  float rh_v = (5 * val) / 1023.0;
  float rh_25 = (rh_v - 0.8) / 0.031;
  float rh = rh_25 / (1.0546 - 0.00216 * temp);

  return rh;  
}

#define BT_LINK_PIN A5
bool conncted;

void send_bt_cmd(char command[])
{
  Serial.print(command);
  delay(3000);
}

/* configure bluetooth connection */
void reset_bt_link()
{
  /* set device as slave */
  send_bt_cmd("\r\n+STWMOD=0\r\n");
  /* set nick */
  send_bt_cmd("\r\n+STNA=weater_station\r\n");
  send_bt_cmd("\r\n+STAUTO=0\r\n");
  /* permit autoconnect to paired device */
  send_bt_cmd("\r\n+STPIN=0000\r\n");
  send_bt_cmd("\r\n+INQ=0\r\n");
  delay(2000);
  send_bt_cmd("\r\n+INQ=1\r\n");
  delay(2000);
}

void setup()
{
  Serial.begin(38400);
  cur_temp = 25;

  pinMode(BT_LINK_PIN, INPUT);
  conncted = false;

  delay(1000);
  
  reset_bt_link();
}

void loop()
{
  bool bt_link = digitalRead(BT_LINK_PIN);

  if (!bt_link && conncted) {
    conncted = false;
    reset_bt_link();
  } else {
    char cmd;

    conncted = bt_link;
    cmd = Serial.read();  
    if (cmd == 't') {
      char temp;
      
      cur_temp = get_temp_lm335a();
      temp = cur_temp;
      Serial.print(temp);
      Serial.flush();
    } else if (cmd == 'h') {
      char rh;

      rh = get_rh_HIH4030(cur_temp);
      Serial.print(rh);
      Serial.flush();
    }
  }
  delay(100);
}
