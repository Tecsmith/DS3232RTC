/*
 * DS3232RTC.h - library for DS3232 RTC from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time
 * Based on code by Jonathan Oxer at http://www.freetronics.com/pages/rtc-real-time-clock-module-quickstart-guide
 * Based on code by Michael Margolis at https://code.google.com/p/arduino-time
 * Based on code by Ryhs Weather at https://github.com/rweather/arduinolibs/tree/master/libraries/RTC
  
  Copyright (c) 2013 by the Arduino community (Author anonymous)
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
#include <Stream.h>

// Based on page 11 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_I2C_ADDRESS 0x68

// Helpers
#define temperatureCToF(C) (C * 9 / 5 + 32)
#define temperatureFToC(F) ((F - 32) * 5 / 9)

static const int16_t NO_TEMPERATURE = 0x7FFF;

/**
 * DS3232SRAM Class
 */
class DS3232SRAM : public Stream
{
  public:
    DS3232SRAM();
    // from Print class
    #if ARDUINO >= 100
    virtual size_t write(uint8_t data);
    virtual size_t write(const char *str);
    virtual size_t write(const uint8_t *buf, size_t size);
    #else
    virtual void write(uint8_t);
    virtual void write(const char *str);
    virtual void write(const uint8_t *buf, size_t size);
    #endif
    // from Stream class
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();  // Sets the next character position to 0.
    uint8_t seek(uint8_t pos);  // Sets the position of the next character to be extracted from the stream.
    uint8_t tell();  // Returns the position of the current character in the stream.
  private:
    uint8_t _cursor;
};

extern DS3232SRAM SRAM;

/**
 * DS3232RTC Class
 */
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
    // Control Register
    // static void setBBOscillator(bool enable);
    // static void setBBSqareWave(bool enable);
    // Control/Status Register
    static bool isOscillatorStopFlag();
    static void setOscillatorStopFlag(bool enable);
    // static void setBB33kHzOutput(bool enable);
    static void set33kHzOutput(bool enable);
    static bool isBusy();
  private:
    static void _wTime(tmElements_t &tm);
    static void _wDate(tmElements_t &tm);
    static uint8_t dec2bcd(uint8_t num);
    static uint8_t bcd2dec(uint8_t num);
    static uint8_t readRegister(uint8_t o);
    static void writeRegister(uint8_t o, uint8_t value);
};

extern DS3232RTC RTC;

#endif
