/*
 * DS3232RTC.h - library for DS3232 RTC module from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time

 (See DS3232RTC.h for notes & license)
 */

#include <Wire.h>
#include "DS3232RTC.h"
/*  Only need this for debug Serial.print...'s
#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif /* */

// Based on page 11 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_I2C_ADDRESS 0x68

DS3232RTC::DS3232RTC() {
  Wire.begin();
}
  
time_t DS3232RTC::get() {
  tmElements_t tm;
  read(tm);
  return makeTime(tm);
}

void DS3232RTC::set(time_t t) {
  tmElements_t tm;
  breakTime(t, tm);
  write(tm); 
}

void DS3232RTC::read( tmElements_t &tm ) { 
  uint8_t b;

  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0);  // sends 00h - seconds register
  Wire.endTransmission();
  Wire.requestFrom(DS3232_I2C_ADDRESS, 7);

  if (Wire.available()) {
    tm.Second = bcd2dec(Wire.read() & 0x7f);  // 00h
    tm.Minute = bcd2dec(Wire.read() & 0x7f);  // 01h
    b = Wire.read() & 0x7f;                   // 02h
    if ((b & 0x40) != 0) {  // 12 hour format with bit 5 set as PM
      tm.Hour = bcd2dec(b & 0x1f);
      if ((b & 0x20) != 0) tm.Hour += 12;
    } else {  // 24 hour format
      tm.Hour = bcd2dec(b & 0x3f);
    }
    tm.Wday =  bcd2dec(Wire.read() & 0x07);   // 03h
    tm.Day =   bcd2dec(Wire.read() & 0x3f);   // 04h
    b = Wire.read() & 0x9f;                   // 05h
    tm.Month = bcd2dec(b & 0x1f);
    tm.Year =  bcd2dec(Wire.read());   // 06h
    if ((b & 0x80) != 0) tm.Year += 100;
    tm.Year = y2kYearToTm(tm.Year);
  }
}

void DS3232RTC::writeTime(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0);  // sends 00h - seconds register
  _wTime(tm);
  Wire.endTransmission();
}

void DS3232RTC::writeDate(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(3);  // sends 03h - day (of week) register
  _wDate(tm);
  Wire.endTransmission();
}

void DS3232RTC::write(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0);  // sends 00h - seconds register
  _wTime(tm);
  _wDate(tm);
  Wire.endTransmission();
}

float DS3232RTC::readTemperature() {
  float temp;

  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x11);  // sends 11h - MSB of Temp register
  Wire.endTransmission();

  Wire.requestFrom(DS3232_I2C_ADDRESS, 2);

  if (Wire.available()) {
    temp = Wire.read();
    temp += 0.25 * (Wire.read() >> 6);
    return temp;
  } else {
    return NO_TEMPERATURE;
  }
}

void DS3232RTC::_wTime(tmElements_t &tm) {
  Wire.write(dec2bcd(tm.Second)); // set seconds
  Wire.write(dec2bcd(tm.Minute)); // set minutes
  Wire.write(dec2bcd(tm.Hour));   // set hours [NB! sets 24 hour format]
}

void DS3232RTC::_wDate(tmElements_t &tm) {
  uint8_t m, y;
  if (tm.Wday == 0 || tm.Wday > 7) {
    tmElements_t tm2;
    breakTime( makeTime(tm), tm2 );  // make and break to get Wday from Unix time
    tm.Wday = tm2.Wday;
  }
  Wire.write(tm.Wday);            // set day (of week) (1~7, 1 = Sunday)
  Wire.write(dec2bcd(tm.Day));    // set date (1~31)
  y = tmYearToY2k(tm.Year);
  m = dec2bcd(tm.Month);
  if (y > 99) {
    m |= 0x80;  // MSB is Century
    y -= 100;
  }
  Wire.write(m);                 // set month, and MSB is year >= 100
  Wire.write(dec2bcd(y));        // set year (0~99), 100~199 flag in month
}

// Convert Decimal to Binary Coded Decimal (BCD)
uint8_t DS3232RTC::dec2bcd(uint8_t num) {
  return (num/10 * 16) + (num % 10);
}

// Convert Binary Coded Decimal (BCD) to Decimal
uint8_t DS3232RTC::bcd2dec(uint8_t num) {
  return (num/16 * 10) + (num % 16);
}

DS3232RTC RTC = DS3232RTC();  // instantiate for use
