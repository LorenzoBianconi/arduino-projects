/*
 * tankino 
 *
 * Copyright 2012-2013	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/* ArduMoto pin Maps */
const int pwmA = 3;
const int pwmB = 11;
const int dirA = 12;
const int dirB = 13;
/* light pin maps */
const int sL = A3;
const int dL = 8;
const int tlfL = 6;
const int trfL = A0;
const int tlbL = A5;
const int trbL = 10;

/* motor direction and speed */
int direction = 'F';
int speed = 0;

/* lights */
long previousTLMillis = 0;
int tLState = LOW;
int dLState = LOW;

/* ArduMoto methods */

void turningLight()
{
  if ((direction == 'L' || direction == 'R') &&
      (millis() - previousTLMillis > 300)) {
    previousTLMillis = millis();
    if (tLState == LOW)
      tLState = HIGH;
    else
      tLState = LOW;
    if (direction == 'L') {  
      digitalWrite(tlfL, tLState);
      digitalWrite(tlbL, tLState);
    } else {
      digitalWrite(trfL, tLState);
      digitalWrite(trbL, tLState);
    }
  }
}

void setdirection(int d)
{
  switch (d) {
  case 'F':
    digitalWrite(dirA, LOW);
    digitalWrite(dirB, LOW);
    break;
  case 'B':
    digitalWrite(dirA, HIGH);
    digitalWrite(dirB, HIGH);
    break;
  case 'L':
    digitalWrite(dirA, HIGH);
    digitalWrite(dirB, LOW);
    break;
  case 'R':
    digitalWrite(dirA, LOW);
    digitalWrite(dirB, HIGH);
    break;
  }
}

void setspeed(int s)
{
  if (s == 0) {
    digitalWrite(sL, HIGH);
    digitalWrite(tlfL, LOW);
    digitalWrite(trfL, LOW);
    digitalWrite(tlbL, LOW);
    digitalWrite(trbL, LOW);
  } else
    digitalWrite(sL, LOW);
  analogWrite(pwmA, s);
  analogWrite(pwmB, s);
}

/* BT methods */
 
void setupBlueToothConnection()
{
  Serial.begin(38400);
  delay(1000);  
  sendBlueToothCommand("\r\n+STWMOD=0\r\n");
  sendBlueToothCommand("\r\n+STNA=tankino\r\n");
  sendBlueToothCommand("\r\n+STAUTO=0\r\n");
  sendBlueToothCommand("\r\n+STOAUT=1\r\n");
  sendBlueToothCommand("\r\n+STPIN=0000\r\n");
  delay(2000);
  Serial.print("\r\n+INQ=1\r\n");
  delay(2000);
}
 
void sendBlueToothCommand(char command[])
{
  Serial.print(command);  
  delay(3000);
}

void setup()
{
  /* BT init */
  setupBlueToothConnection();
  
  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(dirA, OUTPUT);
  pinMode(dirB, OUTPUT);
  pinMode(sL, OUTPUT);
  pinMode(dL, OUTPUT);
  pinMode(tlfL, OUTPUT);
  pinMode(trfL, OUTPUT);
  pinMode(tlbL, OUTPUT);
  pinMode(trbL, OUTPUT);
  
  /* initialize direction and speed */
  digitalWrite(dirA, LOW);
  digitalWrite(dirB, LOW);
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  /* initialize lights */
  digitalWrite(sL, HIGH);
  digitalWrite(dL, LOW);
  digitalWrite(tlfL, LOW);
  digitalWrite(trfL, LOW);
  digitalWrite(tlbL, LOW);
  digitalWrite(trbL, LOW);

}

void loop()
{
  if (Serial.available() > 0) {
    char r = Serial.read();
    if (r == '\n') {
      setspeed(speed);
      setdirection(direction);
    } else if (r == 'D') {
      if (dLState == LOW)
        dLState = HIGH;
      else
        dLState = LOW;
     digitalWrite(dL, dLState); 
    } else if (r >= 48 && r <= 57)
      speed = map(r, 48, 57, 0, 255);
    else
      direction = r;
  }
  turningLight();
  delay(10);
}

