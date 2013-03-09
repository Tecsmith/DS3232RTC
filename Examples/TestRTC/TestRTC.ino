/*
 * Copyright (C) 2012 Southern Storm Software, Pty Ltd.
 *
 * MODIFIED (2013) to use the Arduino Time.h library functions; http://playground.arduino.cc/Code/Time
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
This example demonstrates how to read and write time, date, and other
information from the DS3232 realtime clock chip.  The application is
controlled from the Serial Monitor in the Arduino IDE using a simple
command and response system.  Configure the Serial Monitor to use
"Newline" line endings.  Type "HELP" to get a list of commands.
*/

#include <Wire.h>  
#include <Time.h>  
#include <avr/pgmspace.h>
#include <string.h>
#include "DS3232RTC.h"  // DS3232 library that returns time as a time_t

char buffer[64];
size_t buflen;
int led = 13;
bool led_on = false;

const char *days[] = {
    "Sun, ", "Mon, ", "Tue, ", "Wed, ", "Thu, ", "Fri, ", "Sat, "
};

const char *months[] = {
    " Jan ", " Feb ", " Mar ", " Apr ", " May ", " Jun ",
    " Jul ", " Aug ", " Sep ", " Oct ", " Nov ", " Dec "
};

static uint8_t monthLengths[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

inline bool isLeapYear(unsigned int year)
{
    if ((year % 100) == 0)
        return (year % 400) == 0;
    else
        return (year % 4) == 0;
}

inline uint8_t monthLength(const tmElements_t *date)
{
    if (date->Month != 2 || !isLeapYear(date->Year))
        return monthLengths[date->Month - 1];
    else
        return 29;
}

void setup() {
    Serial.begin(9600);
    buflen = 0;
    pinMode(led, OUTPUT);
    cmdHelp(0);
}

void loop() {
    // blink the pin 13 led
    int i = millis() / 1000;  bool j = (i % 2) == 0;
    if (led_on != j) { led_on = j; if (j) { digitalWrite(led, HIGH); } else { digitalWrite(led, LOW); } }

    if (Serial.available()) {
        // Process serial input for commands from the host.
        int ch = Serial.read();
        if (ch == 0x0A || ch == 0x0D) {
            // End of the current command.  Blank lines are ignored.
            if (buflen > 0) {
                buffer[buflen] = '\0';
                buflen = 0;
                processCommand(buffer);
            }
        } else if (ch == 0x08) {
            // Backspace over the last character.
            if (buflen > 0)
                --buflen;
        } else if (buflen < (sizeof(buffer) - 1)) {
            // Add the character to the buffer after forcing to upper case.
            if (ch >= 'a' && ch <= 'z')
                buffer[buflen++] = ch - 'a' + 'A';
            else
                buffer[buflen++] = ch;
        }
    }
}

void printDec2(int value)
{
    Serial.print((char)('0' + (value / 10)));
    Serial.print((char)('0' + (value % 10)));
}

void printProgString(const prog_char *str)
{
    for (;;) {
        char ch = (char)(pgm_read_byte(str));
        if (ch == '\0')
            break;
        Serial.print(ch);
        ++str;
    }
}

byte readField(const char *args, int &posn, int maxValue)
{
    int value = -1;
    if (args[posn] == ':' && posn != 0)
        ++posn;
    while (args[posn] >= '0' && args[posn] <= '9') {
        if (value == -1)
            value = 0;
        value = (value * 10) + (args[posn++] - '0');
        if (value > 99)
            return 99;
    }
    if (value == -1 || value > maxValue)
        return 99;
    else
        return value;
}

// "TIME" command.
void cmdTime(const char *args)
{
    tmElements_t tm;

    if (*args != '\0') {
        // Set the current time.
        int posn = 0;
        tm.Hour = readField(args, posn, 23);
        tm.Minute = readField(args, posn, 59);
        if (args[posn] != '\0')
            tm.Second = readField(args, posn, 59);
        else
            tm.Second = 0;
        if (tm.Hour == 99 || tm.Minute == 99 || tm.Second == 99) {
            Serial.println("Invalid time format; use HH:MM:SS");
            return;
        }
        RTC.writeTime(tm);
        Serial.print("Time has been set to: ");
    }

    // Read the current time.
    RTC.read(tm);
    printDec2(tm.Hour);
    Serial.print(':');
    printDec2(tm.Minute);
    Serial.print(':');
    printDec2(tm.Second);
    Serial.println();
}

// "DATE" command.
void cmdDate(const char *args)
{
    tmElements_t tm;

    if (*args != '\0') {
        // Set the current date.
        unsigned long value = 0;
        while (*args >= '0' && *args <= '9')
            value = value * 10 + (*args++ - '0');
        if (value < 20000000 || value >= 22000000) {
            Serial.println("Year must be between 2000 and 2199");
            return;
        }
        tm.Day = (byte)(value % 100);
        tm.Month = (byte)((value / 100) % 100);
        tm.Year = CalendarYrToTm( (unsigned int)(value / 10000) );
        if (tm.Month < 1 || tm.Month > 12) {
            Serial.println("Month must be between 1 and 12");
            return;
        }
        uint8_t len = monthLength(&tm);
        if (tm.Day < 1 || tm.Day > len) {
            Serial.print("Day must be between 1 and ");
            Serial.println(len, DEC);
            return;
        }
        RTC.writeDate(tm);
        Serial.print("Date has been set to: ");
    } /* */

    // Read the current date.
    RTC.read(tm);
    if (tm.Wday > 0) Serial.print(days[tm.Wday - 1]);
    Serial.print(tm.Day, DEC);
    Serial.print(months[tm.Month - 1]);
    Serial.println(tmYearToCalendar(tm.Year), DEC);  // NB! Remember tmYearToCalendar()
}

// "TEMP" command.
void cmdTemp(const char *args)
{
    float temp = RTC.readTemperature();
    if (temp != NO_TEMPERATURE) {  // 32767 / 4
        Serial.print(temp);
        Serial.print("�C (");
        Serial.print(temperatureCToF(temp));
        Serial.println("�F)");
    } else {
        Serial.println("Temperature is not available");
    }
}

/* void printAlarm(byte alarmNum, const RTCAlarm *alarm)
{
    Serial.print("Alarm ");
    Serial.print(alarmNum + 1, DEC);
    Serial.print(": ");
    if (alarm->flags & 0x01) {
        printDec2(alarm->hour);
        Serial.print(':');
        printDec2(alarm->minute);
        Serial.println();
    } else {
        Serial.println("Off");
    }
}  /* */

// "ALARMS" command.
/* void cmdAlarms(const char *args)
{
    RTCAlarm alarm;
    for (byte alarmNum = 0; alarmNum < RTC::ALARM_COUNT; ++alarmNum) {
        rtc.readAlarm(alarmNum, &alarm);
        printAlarm(alarmNum, &alarm);
    }
}  /* */

const char s_ON[] PROGMEM = "ON";
const char s_OFF[] PROGMEM = "OFF";

// "ALARM" command.
/* void cmdAlarm(const char *args)
{
    RTCAlarm alarm;
    int posn = 0;
    byte alarmNum = readField(args, posn, RTC::ALARM_COUNT);
    if (!alarmNum || alarmNum == 99) {
        Serial.print("Alarm number must be between 1 and ");
        Serial.println(RTC::ALARM_COUNT, DEC);
        return;
    }
    --alarmNum;
    while (args[posn] == ' ' || args[posn] == '\t')
        ++posn;
    if (args[posn] != '\0') {
        // Set the alarm to a new value.
        if (matchString(s_ON, args + posn, strlen(args + posn))) {
            rtc.readAlarm(alarmNum, &alarm);
            alarm.flags = 1;
        } else if (matchString(s_OFF, args + posn, strlen(args + posn))) {
            rtc.readAlarm(alarmNum, &alarm);
            alarm.flags = 0;
        } else {
            alarm.hour = readField(args, posn, 23);
            alarm.minute = readField(args, posn, 59);
            if (alarm.hour == 99 || alarm.minute == 99) {
                Serial.println("Invalid alarm time format; use HH:MM");
                return;
            }
            alarm.flags = 1;
        }
        rtc.writeAlarm(alarmNum, &alarm);
    }

    // Print the current state of the alarm.
    rtc.readAlarm(alarmNum, &alarm);
    printAlarm(alarmNum, &alarm);
}  /* */

// "NVRAM" command.
/* void cmdNvram(const char *args)
{
    static const char hexchars[] = "0123456789ABCDEF";
    int count = rtc.byteCount();
    for (int offset = 0; offset < count; ++offset) {
        if ((offset % 16) == 0) {
            if (offset)
                Serial.println();
            Serial.print(hexchars[(offset >> 12) & 0x0F]);
            Serial.print(hexchars[(offset >> 8) & 0x0F]);
            Serial.print(hexchars[(offset >> 4) & 0x0F]);
            Serial.print(hexchars[offset & 0x0F]);
            Serial.print(':');
            Serial.print(' ');
        }
        byte value = rtc.readByte(offset);
        Serial.print(hexchars[(value >> 4) & 0x0F]);
        Serial.print(hexchars[value & 0x0F]);
        Serial.print(' ');
    }
    Serial.println();
}  /* */

// List of all commands that are understood by the sketch.
typedef void (*commandFunc)(const char *args);
typedef struct
{
    const prog_char *name;
    commandFunc func;
    const prog_char *desc;
    const prog_char *args;
} command_t;
const char s_cmdTime[] PROGMEM = "TIME";
const char s_cmdTimeDesc[] PROGMEM =
    "Read or write the current time";
const char s_cmdTimeArgs[] PROGMEM = "[HH:MM:SS]";
const char s_cmdDate[] PROGMEM = "DATE";
const char s_cmdDateDesc[] PROGMEM =
    "Read or write the current date";
const char s_cmdDateArgs[] PROGMEM = "[YYYYMMDD]";
const char s_cmdTemp[] PROGMEM = "TEMP";
const char s_cmdTempDesc[] PROGMEM =
    "Read the current temperature";
const char s_cmdAlarms[] PROGMEM = "ALARMS";
const char s_cmdAlarmsDesc[] PROGMEM =
    "Print the status of all alarms";
const char s_cmdAlarm[] PROGMEM = "ALARM";
const char s_cmdAlarmDesc[] PROGMEM =
    "Read or write a specific alarm";
const char s_cmdAlarmArgs[] PROGMEM = "NUM [HH:MM|ON|OFF]";
const char s_cmdNvram[] PROGMEM = "NVRAM";
const char s_cmdNvramDesc[] PROGMEM =
    "Print the contents of NVRAM, excluding alarms";
const char s_cmdHelp[] PROGMEM = "HELP";
const char s_cmdHelpDesc[] PROGMEM =
    "Prints this help message";
const command_t commands[] PROGMEM = {
    {s_cmdTime, cmdTime, s_cmdTimeDesc, s_cmdTimeArgs},
    {s_cmdDate, cmdDate, s_cmdDateDesc, s_cmdDateArgs},
    {s_cmdTemp, cmdTemp, s_cmdTempDesc, 0},
    // {s_cmdAlarms, cmdAlarms, s_cmdAlarmsDesc, 0},
    // {s_cmdAlarm, cmdAlarm, s_cmdAlarmDesc, s_cmdAlarmArgs},
    // {s_cmdNvram, cmdNvram, s_cmdNvramDesc, 0},
    {s_cmdHelp, cmdHelp, s_cmdHelpDesc, 0},
    {0, 0}
};

// "HELP" command.
void cmdHelp(const char *)
{
    int index = 0;
    for (;;) {
        const prog_char *name = (const prog_char *)
            (pgm_read_word(&(commands[index].name)));
        if (!name)
            break;
        const prog_char *desc = (const prog_char *)
            (pgm_read_word(&(commands[index].desc)));
        const prog_char *args = (const prog_char *)
            (pgm_read_word(&(commands[index].args)));
        printProgString(name);
        if (args) {
            Serial.print(' ');
            printProgString(args);
        }
        Serial.println();
        Serial.print("    ");
        printProgString(desc);
        Serial.println();
        ++index;
    }
}

// Match a data-space string where the name comes from PROGMEM.
bool matchString(const prog_char *name, const char *str, int len)
{
    for (;;) {
        char ch1 = (char)(pgm_read_byte(name));
        if (ch1 == '\0')
            return len == 0;
        else if (len == 0)
            break;
        if (ch1 >= 'a' && ch1 <= 'z')
            ch1 = ch1 - 'a' + 'A';
        char ch2 = *str;
        if (ch2 >= 'a' && ch2 <= 'z')
            ch2 = ch2 - 'a' + 'A';
        if (ch1 != ch2)
            break;
        ++name;
        ++str;
        --len;
    }
    return false;
}

// Process commands from the host.
void processCommand(const char *buf)
{
    // Skip white space at the start of the command.
    while (*buf == ' ' || *buf == '\t')
        ++buf;
    if (*buf == '\0')
        return;     // Ignore blank lines.

    // Extract the command portion of the line.
    const char *cmd = buf;
    int len = 0;
    for (;;) {
        char ch = *buf;
        if (ch == '\0' || ch == ' ' || ch == '\t')
            break;
        ++buf;
        ++len;
    }

    // Skip white space after the command name and before the arguments.
    while (*buf == ' ' || *buf == '\t')
        ++buf;

    // Find the command and execute it.
    int index = 0;
    for (;;) {
        const prog_char *name = (const prog_char *)
            (pgm_read_word(&(commands[index].name)));
        if (!name)
            break;
        if (matchString(name, cmd, len)) {
            commandFunc func =
                (commandFunc)(pgm_read_word(&(commands[index].func)));
            (*func)(buf);
            return;
        }
        ++index;
    }

    // Unknown command.
    Serial.println("Unknown command, valid commands are:");
    cmdHelp(0);
}