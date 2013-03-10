/*
 * DS3232RTC.h - library for DS3232 RTC module from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time

 (See DS3232RTC.h for notes & license)
 */

#include <Wire.h>
#include <Stream.h>
#include "DS3232RTC.h"

// Bits in the Control register
// Based on page 13 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_EOSC         0x80
#define DS3232_BBSQW        0x40
#define DS3232_CONV         0x20
#define DS3232_RS2          0x10
#define DS3232_RS1          0x08
#define DS3232_INTCN        0x04
#define DS3232_A2IE         0x02
#define DS3232_A1IE         0x01

#define DS3232_RS_1HZ       0x00
#define DS3232_RS_1024HZ    0x08
#define DS3232_RS_4096HZ    0x10
#define DS3232_RS_8192HZ    0x18

// Bits in the Status register
// Based on page 14 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_OSF          0x80
#define DS3232_BB33KHZ      0x40
#define DS3232_CRATE1       0x20
#define DS3232_CRATE0       0x10
#define DS3232_EN33KHZ      0x08
#define DS3232_BSY          0x04
#define DS3232_A2F          0x02
#define DS3232_A1F          0x01

#define DS3232_CRATE_64     0x00
#define DS3232_CRATE_128    0x10
#define DS3232_CRATE_256    0x20
#define DS3232_CRATE_512    0x30


/* +----------------------------------------------------------------------+ */
/* | DS3232SRAM Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 * \brief Attaches to the DS3232 RTC module on the I2C Wire
 */
DS3232SRAM::DS3232SRAM() {
  _cursor = 0;
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(uint8_t data) {
#else
void DS3232SRAM::write(uint8_t) {
#endif
  if (available()) {
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    Wire.write(data);
    Wire.endTransmission();  
    _cursor++;
    #if ARDUINO >= 100
    return 1;
    #endif
  } else {
    #if ARDUINO >= 100
    return 0;
    #endif
  }
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(const char *str) {
#else
void DS3232SRAM::write(const char *str) {
#endif
  if (available()) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (*str && ((available() + i) > 0))
      i += Wire.write(*str++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
    #endif
  } else {
    #if ARDUINO >= 100
    return 0;
    #endif
  }
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(const uint8_t *buf, size_t size) {
#else
void DS3232SRAM::write(const uint8_t *buf, size_t size) {
#endif
  if (available()) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (size-- && ((available() + i) > 0))
      i += Wire.write(*buf++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
    #endif
  } else {
    #if ARDUINO >= 100
    return 0;
    #endif
  }
}

/**
 *
 */
int DS3232SRAM::available() {
  return 0xEC - _cursor;  // How many bytes left
}

/**
 * \brief Read a single byte from SRAM
 */
int DS3232SRAM::read() {
  int res = peek();
  if (res != -1) _cursor++;
  return res;
}

/**
 *
 */
int DS3232SRAM::peek() {
  if (available()) {
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    Wire.endTransmission();  
    Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
    if (Wire.available()) {
      return Wire.read();
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

/**
 * \brief Reset the cursor to 0
 */
void DS3232SRAM::flush() {
  _cursor = 0;
}

/**
 *
 */
uint8_t DS3232SRAM::seek(uint8_t pos) {
  if (pos <= 0xEB)
    _cursor = pos;
  return _cursor;
}

/**
 *
 */
uint8_t DS3232SRAM::tell() {
  return _cursor;
}

DS3232SRAM SRAM = DS3232SRAM();  // instantiate for use


/* +----------------------------------------------------------------------+ */
/* | DS3232RTC Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 *
 */
DS3232RTC::DS3232RTC() {
  Wire.begin();
}
  
/**
 *
 */
time_t DS3232RTC::get() {
  tmElements_t tm;
  read(tm);
  return makeTime(tm);
}

/**
 *
 */
void DS3232RTC::set(time_t t) {
  tmElements_t tm;
  breakTime(t, tm);
  write(tm); 
}

/**
 *
 */
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

/**
 *
 */
void DS3232RTC::writeTime(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0);  // sends 00h - seconds register
  _wTime(tm);
  Wire.endTransmission();
  setOscillatorStopFlag(false);
}

/**
 *
 */
void DS3232RTC::writeDate(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(3);  // sends 03h - day (of week) register
  _wDate(tm);
  Wire.endTransmission();
}

/**
 *
 */
void DS3232RTC::write(tmElements_t &tm) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0);  // sends 00h - seconds register
  _wTime(tm);
  _wDate(tm);
  Wire.endTransmission();
  setOscillatorStopFlag(false);
}

/**
 *
 */
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

/**
 * Enable or disable the Oscillator in battery-backup mode, always on when powered by Vcc
 */
/* void DS3232RTC::setBBOscillator(bool enable) {
  // Bit7 is NOT EOSC, i.e. 0=started, 1=stopped when on battery power
  uint8_t value = readRegister(0);
  if (enable) {
    value &= ~(DS3232_EOSC);
  } else {
    value |= DS3232_EOSC;
  }
  writeRegister(0, value);
} */

/**
 * Enable or disable the Sqare Wave in battery-backup mode
 */
/* void DS3232RTC::setBBSqareWave(bool enable) {
  uint8_t value = readRegister(0);
  if (enable) {
    value |= DS3232_BBSQW;
  } else {
    value &= ~(DS3232_BBSQW);
  }
  writeRegister(0, value);
} */

/**
 *
 */
bool DS3232RTC::isOscillatorStopFlag() {
  uint8_t value = readRegister(1);
  return ((value & DS3232_OSF) != 0);
}

/**
 *
 */
void DS3232RTC::setOscillatorStopFlag(bool enable) {
  uint8_t value = readRegister(1);
  if (enable) {
    value |= DS3232_OSF;
  } else {
    value &= ~(DS3232_OSF);
  }
  writeRegister(1, value);
}

/**
 *
 */
/* void DS3232RTC::setBB33kHzOutput(bool enable) {
  uint8_t value = readRegister(1);
  if (enable) {
    value |= DS3232_BB33KHZ;
  } else {
    value &= ~(DS3232_BB33KHZ);
  }
  writeRegister(1, value);
} */

/**
 *
 */
void DS3232RTC::set33kHzOutput(bool enable) {
  uint8_t value = readRegister(1);
  if (enable) {
    value |= DS3232_EN33KHZ;
  } else {
    value &= ~(DS3232_EN33KHZ);
  }
  writeRegister(1, value);
}

/**
 *
 */
bool DS3232RTC::isBusy() {
  uint8_t value = readRegister(1);
  return ((value & DS3232_BSY) != 0);
}

/**
 *
 */
void DS3232RTC::_wTime(tmElements_t &tm) {
  Wire.write(dec2bcd(tm.Second)); // set seconds
  Wire.write(dec2bcd(tm.Minute)); // set minutes
  Wire.write(dec2bcd(tm.Hour));   // set hours [NB! sets 24 hour format]
}

/**
 *
 */
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

/**
 * Convert Decimal to Binary Coded Decimal (BCD)
 */
uint8_t DS3232RTC::dec2bcd(uint8_t num) {
  return (num/10 * 16) + (num % 10);
}

/**
 * Convert Binary Coded Decimal (BCD) to Decimal
 */
uint8_t DS3232RTC::bcd2dec(uint8_t num) {
  return (num/16 * 10) + (num % 16);
}

/**
 *
 */
uint8_t DS3232RTC::readRegister(uint8_t o) {
  if (o > 1) return 0xFF;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x0E + o);  // sends 0Eh ~o=0 - Control register, or 0Fh ~o=1 - Ctrl/Status register
  Wire.endTransmission();

  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);

  if (Wire.available()) {
    return Wire.read();
  } else {
    return 0xFF;
  }
}

/**
 *
 */
void DS3232RTC::writeRegister(uint8_t o, uint8_t value){
  if (o > 1) return;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x0E + o);  // sends 0Eh ~o=0 - Control register, or 0Fh ~o=1 - Ctrl/Status register
  Wire.write(value);
  Wire.endTransmission();
}


DS3232RTC RTC = DS3232RTC();  // instantiate for use
