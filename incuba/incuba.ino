#include "DHT.h"
#include "Timer.h"
#include <ezButton.h>
#include <EEPROM.h>

//A0=14, A1=15, A2=16,A3=17,A4=18,A5=19,A6=20,A7=21

#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display



#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DHTPIN 8

#define LED_A 5
#define LED_B 4


int temp1 = 6; //Q1
int temp2 = 7 ;//Q2
int vantillator_air_quality = 9; //Q3
int lamp_water = 11; //Q4
int ventillator_humidity = 10; //Q0

int mask[5] = {temp1, temp2, vantillator_air_quality, ventillator_humidity, lamp_water};

ezButton button_ok(14);
ezButton button_set(15);
ezButton button_haut(16);
ezButton button_bas(17);


DHT dht(DHTPIN, DHTTYPE, 4);
Timer task;

String current_time = "12:00";

int addr_minute = 100;
int addr_hour = 200;

int addr_start_incubation = 300;
int addr_jour_incubation = 350;

int minute = 0;
int hour = 12;

String incubation_days = "waiting...";
String time;

float ambient_temperature = 0.00;

int humidity = 0;

void setup() {
  Serial.begin(115200);

  dht.begin();


  Wire.begin();
  Wire.beginTransmission(0x27);
  lcd.begin(20, 4);
  lcd.setBacklight(255);

  for (int i = 0; i <= 4; i++) {
    pinMode(mask[i], OUTPUT);
  }

  for (int i = 0; i <= 4; i++) {
    digitalWrite(mask[i], LOW);
  }

  for (int i = 0; i <= 4; i++) {
    digitalWrite(mask[i], HIGH);
    delay(2000);
  }

  for (int i = 0; i <= 4; i++) {
    digitalWrite(mask[i], LOW);
    delay(500);
  }

  button_ok.setDebounceTime(200); //300ms
  button_set.setDebounceTime(200); //300ms
  button_haut.setDebounceTime(200); //300ms
  button_bas.setDebounceTime(200); //300ms

  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);

  task.oscillate(LED_A, 1000, HIGH);


  if (EEPROM.read(addr_minute) >= 254) {
    EEPROM.write(addr_minute, 0);
    minute = 0;
  }

  else {
    minute = EEPROM.read(addr_minute);
  }


  if (EEPROM.read(addr_hour) >= 254) {
    EEPROM.write(addr_hour, 12);
    hour = 12;
  }

  else {
    hour = EEPROM.read(addr_hour);
  }

  minute %= 60;
  hour %= 24;


  if (EEPROM.read(addr_jour_incubation) >= 254) {
    EEPROM.write(addr_jour_incubation, 0);
  }


  if (EEPROM.read(addr_start_incubation) == 20) { //put 20 when incubation start
    incubation_days = String(EEPROM.read(addr_jour_incubation)) + " days";
  }

  task.every(5000, display_essentiel);
  task.every(500, time_update_callback);

  checkStatus();

}

void loop() {
  // put your main code here, to run repeatedly:
  task.update();
  button_set.loop();
  button_haut.loop();
  button_bas.loop();

  unsigned long last = millis();

  if (button_set.isPressed()) {

    if (EEPROM.read(addr_start_incubation) == 20) //incubation has started
      return;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Okay");
    lcd.setCursor(0, 1);
    lcd.print("L'incubation demarre");


    lcd.setCursor(0, 2);
    lcd.print("Appuyez SET encore");

    while (1) {

      if ((last + 5000) < millis()) {
        lcd.clear();
        delay(1000);
        return;
      }
      button_set.loop();
      if (button_set.isPressed()) {
        EEPROM.write(addr_start_incubation, 20);
        incubation_days = String(EEPROM.read(addr_jour_incubation)) + " days";
        delay(3000);
        lcd.clear();
        return;
      }


      delay(100);
    }
    delay(2000);
  }


  if (button_haut.isPressed()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pour Reinitialiser");
    lcd.setCursor(0, 2);
    lcd.print("Pressez BAS");

    while (1) {

      if ((last + 5000) < millis()) {
        lcd.clear();
        delay(1000);
        return;
      }

      button_set.loop();
      button_haut.loop();
      button_bas.loop();
      if (button_bas.isPressed()) {
        EEPROM.write(addr_start_incubation, 229);
        EEPROM.write(addr_jour_incubation, 0);
        Serial.println("Reinitialiser avec succes");

        if (EEPROM.read(addr_start_incubation) == 20) { //put 20 when incubation start
          incubation_days = String(EEPROM.read(addr_jour_incubation)) + " days";
        }
        else {
          incubation_days = "waiting..";
        }
        delay(3000);
        lcd.clear();
        return;
      }
      delay(100);
    }
    delay(3000);
  }

  if (button_bas.isPressed()) {
    time_update(false);

    int cnt = 0;

    lcd.clear();
    while (1) {

      lcd.setCursor(0, 0);
      button_set.loop();
      button_haut.loop();
      button_bas.loop();

      if (button_bas.isPressed())
        minute--;
      if (button_haut.isPressed())
        minute++;

      cnt = cnt < 0 ? 59 : cnt;

      if (minute > 59) { //next minutes, save time to EEPROM
        minute = 0;
        hour++;
        if (hour > 23) { //next days
          hour = 0;
        }
      }


      String h = hour < 10 ? "0" + String(hour) : hour;
      String m = minute < 10 ? "0" + String(minute) : minute;

      lcd.print(h  + ":" + m);


      if (button_set.isPressed()) {
        EEPROM.write(addr_minute, minute);
        EEPROM.write(addr_hour, hour);
        lcd.noCursor();
        lcd.clear();
        return;
      }
      delay(20);
    }

    delay(3000);
  }

  delay(2);

}


void display_essentiel() {

  ambient_temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  ambient_temperature = (float) (ambient_temperature);

  checkStatus();

  lcd.setCursor(0, 1);
  lcd.print("Temperature: ");
  lcd.print(ambient_temperature);
  lcd.print("^C");


  Serial.print("Temperature :");
  Serial.print(ambient_temperature);
  Serial.println("°C");

  lcd.setCursor(0, 2);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");


  Serial.print("Humidity :");
  Serial.print(humidity);
  Serial.println("%");

  lcd.setCursor(0, 3);
  lcd.print("Incubation: ");
  lcd.print(incubation_days);


  Serial.print("Incubation days ");
  Serial.println(incubation_days);

}

bool flicker = false;
int seconds = 0;

void time_update_callback() {
  time_update(true);
}

void time_update(bool inc) {

  String StringHour;
  String StringMinutes;

  if (!flicker) {

    if (hour < 10) StringHour = '0' + String(hour); else StringHour = String(hour);
    if (minute < 10) StringMinutes = '0' + String(minute); else StringMinutes = String(minute);
    time = StringHour + " " + StringMinutes;
  }
  else { //1second has passed
    if (inc == true) seconds++;
    if (seconds > 59) {
      seconds = 0;
      minute++;
      if (minute > 59) { //next minutes, save time to EEPROM
        minute = 0;
        hour++;
        if (hour > 23) { //next days
          hour = 0;
          if (EEPROM.read(addr_start_incubation) == 20) { //incubation has started yet
            EEPROM.write(addr_jour_incubation, (int)EEPROM.read(addr_jour_incubation) + 1);
            incubation_days = String(EEPROM.read(addr_jour_incubation)) + " days";
          }

          else {
            incubation_days =  "--- days";
          }

        }
      }
    }

    if (inc == true) {
      if (minute % 5 == 0) { //next 5minutes we save the time
        EEPROM.write(addr_minute, minute);
        EEPROM.write(addr_hour, hour);
      }
    }
    if (hour < 10) StringHour = '0' + String(hour); else StringHour = String(hour);
    if (minute < 10) StringMinutes = '0' + String(minute); else StringMinutes = String(minute);
    time = StringHour + ":" + StringMinutes;

  }


  Serial.println(time);

  lcd.setCursor(0, 0);
  lcd.print(time);
  flicker = !flicker;
}


//int humidity_relay=0; //Q0 with lamp to pull water
//int temp1 =1 //Q1
//int temp2 = 2 //Q2
//int vantillator_air = 3; //Q3
//int vantillator_cooling = 4 //Q4
unsigned long counter_5sec = 0;
unsigned long counter_min = 0;

void checkStatus() { //int mask[5]={temp1,temp2,vantillator_air_quality,ventillator_humidity,lamp_water};
  byte mask_relay = 0;
  counter_5sec++;
  counter_min++;

  if (counter_5sec >= 360) { //30minutes (1min = 12calls, 30min = 12x30 calls), // vantillo air turn on each 30minutes in 3minutes

    if (counter_5sec >= (360 + 72)) { //6minutes
      counter_5sec = 0;
      //bitClear(mask_relay, vantillator_air);
      digitalWrite(mask[2], LOW); //vantillator_air_quality
    }

    else {
      //bitSet(mask_relay, vantillator_air);
      digitalWrite(mask[2], HIGH); //vantillator_air_quality
    }
  }

  if (counter_min >= 2160) { //3hours
    if (counter_min >= (2160 + 72)) { //6min
      counter_min = 0;
      digitalWrite(mask[4], LOW); //lamp_water
    }
    else {
      digitalWrite(mask[4], HIGH); //lamp_water
    }
  }

  if (EEPROM.read(addr_jour_incubation) > 18) {
    if (humidity >= 70  & humidity <= 80 & digitalRead(mask[3])) {
      digitalWrite(mask[3], LOW); //ventillator_humidity
    }
    else if (humidity > 80) {
      //bitClear(mask_relay, humidity_relay);
      digitalWrite(mask[3], LOW); //ventillator_humidity
      //digitalWrite(mask[4], LOW); //lamp_water
    }

    else if (humidity <= 68 & !digitalRead(mask[3])) { //very low humidity
      digitalWrite(mask[3], HIGH); //ventillator_humidity
      //digitalWrite(mask[4], HIGH); //lamp_water
    }
  }

  else {
    if (humidity >= 60  & humidity <= 70 & digitalRead(mask[3])) {
      digitalWrite(mask[3], LOW); //ventillator_humidity
    }
    else if (humidity > 70) {
      digitalWrite(mask[3], LOW); //ventillator_humidity
      //digitalWrite(mask[4], LOW); //lamp_water
    }

    else if (humidity < 58 & !digitalRead(mask[3])) { //very low humidity
      digitalWrite(mask[3], HIGH); //ventillator_humidity
      //digitalWrite(mask[4], HIGH); //lamp_water
    }
  }


  if (EEPROM.read(addr_jour_incubation) < 18) {
    if (ambient_temperature > 38.20) {
      //bitSet(mask_relay, vantillator_cooling);
      //bitClear(mask_relay, temp2);
      //bitClear(mask_relay, temp1);
      digitalWrite(mask[2], HIGH); //vantillator_air_quality
      digitalWrite(mask[0], LOW); //temp1
      digitalWrite(mask[1], LOW); //temp2
      digitalWrite(mask[4], LOW); //lamp_water
      digitalWrite(LED_B, LOW);
    }

    else if ( ambient_temperature >= 37.6 & ambient_temperature <= 38.2 & digitalRead(mask[1])) {
      // bitClear(mask_relay, vantillator_cooling);
      // bitClear(mask_relay, temp2);
      // bitClear(mask_relay, temp1);
      digitalWrite(mask[0], LOW); //temp1
      digitalWrite(mask[1], LOW); //temp2
      digitalWrite(LED_B, HIGH);
    }

    else if (ambient_temperature <= 37.6 & ambient_temperature > 36.3) {
      if (ambient_temperature < 37.2) {
        //bitClear(mask_relay, vantillator_cooling);
        //bitSet(mask_relay, temp2);
        //bitClear(mask_relay, temp1);

        digitalWrite(mask[2], LOW); //vantillator_air_quality
        digitalWrite(mask[0], LOW); //temp1
        digitalWrite(mask[1], HIGH); //temp2
        digitalWrite(LED_B, LOW);
      }
    }

    else if (ambient_temperature <= 36.3) {
      //bitClear(mask_relay, vantillator_cooling);
      //bitSet(mask_relay, temp2);
      //bitSet(mask_relay, temp1);
      digitalWrite(mask[2], LOW); //vantillator_air_quality
      digitalWrite(mask[0], HIGH); //temp1
      digitalWrite(mask[1], HIGH); //temp2
      digitalWrite(LED_B, LOW);
    }
  }

  else {
    if (ambient_temperature > 37.80) {
      //bitSet(mask_relay, vantillator_cooling);
      //bitClear(mask_relay, temp2);
      //bitClear(mask_relay, temp1);
      digitalWrite(mask[2], HIGH); //vantillator_air_quality
      digitalWrite(mask[0], LOW); //temp1
      digitalWrite(mask[1], LOW); //temp2
      digitalWrite(mask[4], LOW); //lamp_water
      digitalWrite(LED_B, LOW);
    }

    else if ( ambient_temperature >= 37.2 & ambient_temperature <= 37.8 & digitalRead(mask[1])) {
      // bitClear(mask_relay, vantillator_cooling);
      // bitClear(mask_relay, temp2);
      // bitClear(mask_relay, temp1);
      digitalWrite(mask[0], LOW); //temp1
      digitalWrite(mask[1], LOW); //temp2
      digitalWrite(LED_B, HIGH);
    }

    else if (ambient_temperature <= 37.2 & ambient_temperature > 35.9) {
      if (ambient_temperature < 36.8) {
        //bitClear(mask_relay, vantillator_cooling);
        //bitSet(mask_relay, temp2);
        //bitClear(mask_relay, temp1);

        digitalWrite(mask[2], LOW); //vantillator_air_quality
        digitalWrite(mask[0], LOW); //temp1
        digitalWrite(mask[1], HIGH); //temp2
        digitalWrite(LED_B, LOW);
      }
    }

    else if (ambient_temperature <= 35.9) {
      //bitClear(mask_relay, vantillator_cooling);
      //bitSet(mask_relay, temp2);
      //bitSet(mask_relay, temp1);
      digitalWrite(mask[2], LOW); //vantillator_air_quality
      digitalWrite(mask[0], HIGH); //temp1
      digitalWrite(mask[1], HIGH); //temp2
      digitalWrite(LED_B, LOW);
    }
  }

}
