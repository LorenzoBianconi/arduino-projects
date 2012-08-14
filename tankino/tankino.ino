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

/* motor direction and speed */
int direction = 'F';
int speed = 0;

/* ArduMoto methods */

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
  //sendBlueToothCommand("\r\n+STPIN=0000\r\n");
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
  
  /* initialize direction and speed */
  digitalWrite(dirA, LOW);
  digitalWrite(dirB, LOW);
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
}


void loop()
{
  if (Serial.available() > 0) {
    char r = Serial.read();
    if (r == '\n') {
      setspeed(speed);
      setdirection(direction);
    } else if (r >= 48 && r <= 57)
      speed = map(r, 48, 57, 0, 255);
    else
      direction = r;
  }
  delay(10);
}

