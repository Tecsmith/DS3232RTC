/*
 * DS3232RTC.h - library for DS3232 RTC module from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time

 (See DS3232RTC.h for notes & license)
 */

#include <Stdint.h>
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
/* | DS3232RTC Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 *
 */
DS3232RTC::DS3232RTC() {
  Wire.begin();
}

bool DS3232RTC::available() {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x05);  // sends 05h - month register
  Wire.endTransmission();
  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t dummy = Wire.read();
    return true;
  }
  return false;
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
    tm.Second = bcd2dec(Wire.read() & 0x7F);  // 00h
    tm.Minute = bcd2dec(Wire.read() & 0x7F);  // 01h
    b = Wire.read() & 0x7F;                   // 02h
    if ((b & 0x40) != 0) {  // 12 hour format with bit 5 set as PM
      tm.Hour = bcd2dec(b & 0x1F);
      if ((b & 0x20) != 0) tm.Hour += 12;
    } else {  // 24 hour format
      tm.Hour = bcd2dec(b & 0x3F);
    }
    tm.Wday =  bcd2dec(Wire.read() & 0x07);   // 03h
    tm.Day =   bcd2dec(Wire.read() & 0x3F);   // 04h
    b = Wire.read() & 0x9F;                   // 05h
    tm.Month = bcd2dec(b & 0x1F);
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
void DS3232RTC::readAlarm(uint8_t alarm, alarmMode_t &mode, tmElements_t &tm) {
  uint8_t data[4];
  uint8_t flags;
  int i;

  memset(&tm, 0, sizeof(tmElements_t));
  mode = alarmModeUnknown;
  if ((alarm > 2) || (alarm < 1)) return;

  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write( ((alarm == 1) ? 0x07 : 0x0B) );
  Wire.endTransmission();
  Wire.requestFrom( DS3232_I2C_ADDRESS, ((alarm == 1) ? 4 : 3) );
  if (Wire.available()) {
    if (alarm == 1) {
      for (i = 0; i < 4; i++) data[i] = Wire.read();
    } else {
      data[0] = 0;  // alarm 2 doesn't use seconds
      for (i = 1; i < 4; i++) data[i] = Wire.read();
    }

    flags = ((data[0] & 0x80) >> 7) | ((data[1] & 0x80) >> 6) |
      ((data[2] & 0x80) >> 5) | ((data[3] & 0x80) >> 4);
    if (flags == 0) flags = ((data[3] & 0x40) >> 2);
    switch (flags) {
      case 0x04: mode = alarmModePerSecond; break;  // X1111
      case 0x0E: mode = (alarm == 1) ? alarmModeSecondsMatch : alarmModePerMinute; break;  // X1110
      case 0x0A: mode = alarmModeMinutesMatch; break;  // X1100
      case 0x08: mode = alarmModeHoursMatch; break;  // X1000
      case 0x00: mode = alarmModeDateMatch; break;  // 00000
      case 0x10: mode = alarmModeDayMatch; break;  // 10000
    }

    if (alarm == 1) tm.Second = bcd2dec(data[0] & 0x7F);
    tm.Minute = bcd2dec(data[1] & 0x7F);
    if ((data[2] & 0x40) != 0) {
      // 12 hour format with bit 5 set as PM
      tm.Hour = bcd2dec(data[2] & 0x1F);
      if ((data[2] & 0x20) != 0) tm.Hour += 12;
    } else {
      // 24 hour format
      tm.Hour = bcd2dec(data[2] & 0x3F);
    }
    if ((data[3] & 0x40) == 0) {
      // Alarm holds Date (of Month)
      tm.Day = bcd2dec(data[3] & 0x3F);
    } else {
      // Alarm holds Day (of Week)
      tm.Wday = bcd2dec(data[3] & 0x07);
    }

    // TODO : Not too sure about this.
    /*
      If the alarm is set to trigger every Nth of the month
      (or every 1-7 week day), but the date/day are 0 then
      what?  The spec is not clear about alarm off conditions.
      My assumption is that it would not trigger is date/day
      set to 0, so I've created a Alarm-Off mode.
    */
    if ((mode == alarmModeDateMatch) && (tm.Day == 0)) {
      mode = alarmModeOff;
    } else if ((mode == alarmModeDayMatch) && (tm.Wday == 0)) {
      mode = alarmModeOff;
    }
  }
}

/**
 *
 */
void DS3232RTC::writeAlarm(uint8_t alarm, alarmMode_t mode, tmElements_t tm) {
  uint8_t data[4];

  switch (mode) {
    case alarmModePerSecond:
      data[0] = 0x80;
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModePerMinute:
      data[0] = 0x00;
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeSecondsMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeMinutesMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeHoursMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x80;
      break;
    case alarmModeDateMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x00 | dec2bcd(tm.Day);
      break;
    case alarmModeDayMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x40 | dec2bcd(tm.Wday);
      break;
    case alarmModeOff:
      data[0] = 0x00;
      data[1] = 0x00;
      data[2] = 0x00;
      data[3] = 0x00;
      break;
    default: return;
  }

  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write( ((alarm == 1) ? 0x07 : 0x0B) );
  if (alarm == 1) Wire.write(data[0]);
  Wire.write(data[1]);
  Wire.write(data[2]);
  Wire.write(data[3]);
  Wire.endTransmission();
}

/**
 * Enable or disable the Oscillator in battery-backup mode, always on when powered by Vcc
 */
void DS3232RTC::setBBOscillator(bool enable) {
  // Bit7 is NOT EOSC, i.e. 0=started, 1=stopped when on battery power
  uint8_t value = read1(0x0E);  // sends 0Eh - Control register
  if (enable) {
    value &= ~(DS3232_EOSC);
  } else {
    value |= DS3232_EOSC;
  }
  write1(0x0E, value);    // sends 0Eh - Control register
}

/**
 * Enable or disable the Sqare Wave in battery-backup mode
 */
void DS3232RTC::setBBSqareWave(bool enable) {
  uint8_t value = read1(0x0E);  // sends 0Eh - Control register
  if (enable) {
    value |= DS3232_BBSQW;
  } else {
    value &= ~(DS3232_BBSQW);
  }
  write1(0x0E, value);  // sends 0Eh - Control register
}

/**
 *  Set the SQI pin to either a square wave generator or an alarm interupt
 */
void DS3232RTC::setSQIMode(sqiMode_t mode) {
  uint8_t value = read1(0x0E) & 0xE0;  // sends 0Eh - Control register
  switch (mode) {
    case sqiModeNone: value |= DS3232_INTCN; break;
    case sqiMode1Hz: value |= DS3232_RS_1HZ;  break;
    case sqiMode1024Hz: value |= DS3232_RS_1024HZ; break;
    case sqiMode4096Hz: value |= DS3232_RS_4096HZ; break;
    case sqiMode8192Hz: value |= DS3232_RS_8192HZ; break;
    case sqiModeAlarm1: value |= (DS3232_INTCN | DS3232_A1IE); break;
    case sqiModeAlarm2: value |= (DS3232_INTCN | DS3232_A2IE); break;
    case sqiModeAlarmBoth: value |= (DS3232_INTCN | DS3232_A1IE | DS3232_A2IE); break;
  }
  write1(0x0E, value);  // sends 0Eh - Control register
}

bool DS3232RTC::isAlarmInterupt(uint8_t alarm) {
  if ((alarm > 2) || (alarm < 1)) return false;
  uint8_t value = read1(0x0E) & 0x07;  // sends 0Eh - Control register
  if (alarm == 1) {
    return ((value & 0x05) == 0x05);
  } else {
    return ((value & 0x06) == 0x06);
  }
}

/**
 *
 */
bool DS3232RTC::isOscillatorStopFlag() {
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  return ((value & DS3232_OSF) != 0);
}

/**
 *
 */
void DS3232RTC::setOscillatorStopFlag(bool enable) {
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  if (enable) {
    value |= DS3232_OSF;
  } else {
    value &= ~(DS3232_OSF);
  }
  write1(0x0F, value);  // sends 0Fh - Ctrl/Status register
}

/**
 *
 */
void DS3232RTC::setBB33kHzOutput(bool enable) {
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  if (enable) {
    value |= DS3232_BB33KHZ;
  } else {
    value &= ~(DS3232_BB33KHZ);
  }
  write1(0x0F, value);  // sends 0Fh - Ctrl/Status register
}

/**
 *
 */
void DS3232RTC::setTCXORate(tempScanRate_t rate) {
  uint8_t value = read1(0x0F) & 0xCF;  // sends 0Fh - Ctrl/Status register
  switch (rate) {
    case tempScanRate64sec: value |= DS3232_CRATE_64; break;
    case tempScanRate128sec: value |= DS3232_CRATE_128; break;
    case tempScanRate256sec: value |= DS3232_CRATE_256; break;
    case tempScanRate512sec: value |= DS3232_CRATE_512; break;
  }
  write1(0x0F, value);  // sends 0Fh - Ctrl/Status register
}

/**
 *
 */
void DS3232RTC::set33kHzOutput(bool enable) {
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  if (enable) {
    value |= DS3232_EN33KHZ;
  } else {
    value &= ~(DS3232_EN33KHZ);
  }
  write1(0x0F, value);  // sends 0Fh - Ctrl/Status register
}

/**
 *
 */
bool DS3232RTC::isTCXOBusy() {
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  return ((value & DS3232_BSY) != 0);
}

/**
 *
 */
bool DS3232RTC::isAlarmFlag(uint8_t alarm) {
  uint8_t value = isAlarmFlag();
  return ((value & alarm) != 0);
}

/**
 *
 */
uint8_t DS3232RTC::isAlarmFlag(){
  uint8_t value = read1(0x0F);  // sends 0Fh - Ctrl/Status register
  return (value & (DS3232_A1F | DS3232_A2F));
}

/**
 *
 */
void DS3232RTC::clearAlarmFlag(uint8_t alarm) {
  alarm &= (DS3232_A1F | DS3232_A2F);
  if (alarm == 0) return;
  alarm = ~alarm;  // invert
  alarm &= (DS3232_A1F | DS3232_A2F);
  uint8_t value = read1(0x0F) & (~(DS3232_A1F | DS3232_A2F));  // sends 0Fh - Ctrl/Status register
  value |= alarm;
  write1(0x0F, value);  // sends 0Fh - Ctrl/Status register
}

/**
 *
 */
void DS3232RTC::readTemperature(tpElements_t &tmp) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x11);  // sends 11h - MSB of Temp register
  Wire.endTransmission();

  Wire.requestFrom(DS3232_I2C_ADDRESS, 2);

  if (Wire.available()) {
    tmp.Temp = Wire.read();
    tmp.Decimal = (Wire.read() >> 6) * 25;
  } else {
    tmp.Temp = NO_TEMPERATURE;
    tmp.Decimal = NO_TEMPERATURE;
  }
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
 *
 */
uint8_t DS3232RTC::read1(uint8_t addr) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(addr);
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
void DS3232RTC::write1(uint8_t addr, uint8_t data){
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

DS3232RTC RTC = DS3232RTC();  // instantiate for use


/* +----------------------------------------------------------------------+ */
/* | DS3232SRAM Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 * \brief Attaches to the DS3232 RTC module on the I2C Wire
 */
DS3232SRAM::DS3232SRAM()
  : _cursor(0)
  , _avail(false)
  , _init(false)
{
  Wire.begin();
}

/**
 *
 */
uint8_t DS3232SRAM::read(int addr) {
  if (addr > 0xEC) return 0x00;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x14 + addr); 
  Wire.endTransmission();  
  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  } else {
    return 0x00;
  }
}

void DS3232SRAM::write(int addr, uint8_t data) {
  if (addr > 0xEC) return;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x14 + addr); 
  Wire.write(data);
  Wire.endTransmission();  
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(uint8_t data) {
#else
void DS3232SRAM::write(uint8_t) {
#endif
  if (available() > 0) {
    write(_cursor, data);
    _cursor++;
    #if ARDUINO >= 100
    return 1;
  } else {
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
  if (available() > 0) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (*str && ((available() + i) > 0))
      i += Wire.write(*str++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
  } else {
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
  if (available() > 0) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (size-- && ((available() + i) > 0))
      i += Wire.write(*buf++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
  } else {
    return 0;
    #endif
  }
}

/**
 *
 */
int DS3232SRAM::available() {
  if (!_init) {
    _init = true;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x05);  // sends 05h - month register
    Wire.endTransmission();
    Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
    if (Wire.available()) {
      uint8_t dummy = Wire.read();
      _avail = true;
    } else {
      _avail = false;
    }
  }

  if (_avail) {
    return 0xEC - _cursor;  // How many bytes left
  } else {
    return -1;
  }
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
  if (available() > 0) {
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
