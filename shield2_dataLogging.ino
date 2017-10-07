#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
RTC_DS1307 rtc;
DHT dht;
const int SD_CS = 10;
int ledPin = 6;
#define DHTPin 7
File dataFile;
void setup () {
  Serial.begin(9600);
  rtc.begin();
  SD.begin(SD_CS);
  dht.setup(DHTPin);
  File dataFile = SD.open ("log.csv",FILE_WRITE);
  dataFile.println("vreme,temperatura,vlaznost");
  dataFile.close();
}
void loop () {
    File dataFile = SD.open ("log.csv",FILE_WRITE);
    float temp = dht.getTemperature();
    float vlaga = dht.getHumidity();  
    DateTime now = rtc.now(); 
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    Serial.print("Temperatura: ");
    Serial.println(temp);
    Serial.println();
    
    if(dataFile){

    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(",");   
    dataFile.print(temp);
    dataFile.print(",");
    dataFile.print(vlaga);
    dataFile.println();
    dataFile.close();
    digitalWrite(ledPin,HIGH);
    delay(100);
    digitalWrite(ledPin,LOW); 
      }else{
        Serial.println("nije kreirana datoteka");
        }

    
    delay(60000);
}
