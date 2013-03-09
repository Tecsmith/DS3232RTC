/*
 * DS3232RTC.h - library for DS3232 RTC from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time
 * Based on code by Jonathan Oxer at http://www.freetronics.com/pages/rtc-real-time-clock-module-quickstart-guide
 * Based on code by Michael Margolis at https://code.google.com/p/arduino-time
 * Based on code by Ryhs Weather at https://github.com/rweather/arduinolibs/tree/master/libraries/RTC
  
  Copyright (c) 2013 by the Arduino community
  This library is intended to be uses with Arduino Time.h library functions

  The library is free software; you can redistribute it and/or
  modify it under the terms of the Creative Commons ShareAlike 1.0 Generic (CC SA 1.0)
  License as published at; http://creativecommons.org/licenses/sa/1.0

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DS3232RTC_h
#define DS3232RTC_h

#include <Time.h>

#define temperatureCToF(C) (C * 9 / 5 + 32)
#define temperatureFToC(F) ((F - 32) * 5 / 9)

static const int16_t NO_TEMPERATURE = 0x7FFF;

// library interface description
class DS3232RTC
{
  public:
    DS3232RTC();
    static time_t get();
    static void set(time_t t);
    static void read(tmElements_t &tm);
    static void write(tmElements_t &tm);
    static void writeTime(tmElements_t &tm);
    static void writeDate(tmElements_t &tm);
    static float readTemperature();
  private:
    static void _wTime(tmElements_t &tm);
    static void _wDate(tmElements_t &tm);
    static uint8_t dec2bcd(uint8_t num);
    static uint8_t bcd2dec(uint8_t num);
};

extern DS3232RTC RTC;

#endif
