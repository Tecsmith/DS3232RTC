Arduino library for DS3232 RTC
==============================

For use with the Freetronics RTC module; http://www.freetronics.com/rtc


Credits
-------

* This library is intended to be used with **Arduino *Time.h* library** functions
  - http://playground.arduino.cc/Code/Time

* Based on code by **Jonathan Oxer**
  - http://www.freetronics.com/pages/rtc-real-time-clock-module-quickstart-guide
  - Controlling The RTC Module Via Direct I2C Commands examples
	
* Based on code by **Michael Margolis**
  - https://code.google.com/p/arduino-time
  - Class structure and examples on integrating to *Time.h* Sync

* Based on code by **Ryhs Weather**
  - http://github.com/rweather/arduinolibs/tree/master/libraries/RTC
  - TestRTC project file

	
About
-----

Freetronics sells a RTC module based on the RC3232 "Extremely Accurate I2C-Integrated" RTC module.
See http://www.maxim-ic.com/datasheet/index.mvp/id/4984

This IC attempts to compensate for crystal inaccuracies due to temperature change and monitors ambient to control accurate time keeping.
As such, it also features a Thermometer with a 0'C to 70'C range (though the temprature read is the internal IC temprature, but a good indicator of ambient).

The RC3232 is very similar to the RC3231 (used by Adafruits' "ChronoDot" RTC module; http://www.adafruit.com/products/255), but adds 2 programable Alarms and 263 bytes of read/write SRAM.
*(Note the SRAM is not FLASH or EEPROM, as data will be lost when the battery-backup is removed. The plus side is that it has a higher write count.)*

Freetronics officially supports the excellent RTC library from Rhys Weather, but I found that Audrino's official *Time.h* library has a wider adoption, more integrated libraries (including the *Timezone.h* from Jack Christensen) and is based on C++ *ctime* library, so is more portable.
This library aims to replicate the effort, but make it *Time.h* friendly.

,','d(-_-)b',',
