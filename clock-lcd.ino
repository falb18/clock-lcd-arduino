#include <stdio.h>

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <AbleButtons.h>

/* Macros for DS3231 functions */
#define SET_12_HR_FRMT false
#define SET_PM_FRMT false

/* Macros for AbleButtons functions */
#define BUTTON_PIN 2

LiquidCrystal_I2C lcd(0x27,16,2);
DS3231 rtc;
AblePullupDirectButton btn(BUTTON_PIN);

bool h12_flag = SET_12_HR_FRMT;
bool pm_flag = SET_PM_FRMT;
bool century = false;

unsigned long prev_time_ms = 0;
unsigned long current_time_ms = 0;

struct lcd_position {
    uint8_t col;
    uint8_t row;
};

void setup(void)
{
    lcd.init();
    lcd.backlight();

    btn.begin();
    pinMode(LED_BUILTIN, OUTPUT);

    prev_time_ms = millis();
    current_time_ms = prev_time_ms;
}

void loop(void)
{
    char str_time[8] = {0};
    char str_date[10] = {0};

    struct lcd_position pos_time = {4, 1};
    struct lcd_position pos_date = {3, 0};

    byte hours = rtc.getHour(h12_flag, pm_flag);
    byte minutes = rtc.getMinute();
    byte seconds = rtc.getSecond();

    byte date = rtc.getDate();
    byte month = rtc.getMonth(century);
    byte year = rtc.getYear();

    sprintf(str_time, "%02d:%02d:%02d", hours, minutes, seconds);
    sprintf(str_date, "%02d/%02d/20%02d", date, month, year);

    lcd.setCursor(pos_time.col, pos_time.row);
    lcd.print(str_time);

    lcd.setCursor(pos_date.col, pos_date.row);
    lcd.print(str_date);
    
    while(1) {

        current_time_ms = millis();
        
        if ( (current_time_ms - prev_time_ms) >= 1000UL) {
            hours = rtc.getHour(h12_flag, pm_flag);
            minutes = rtc.getMinute();
            seconds = rtc.getSecond();
            sprintf(str_time, "%02d:%02d:%02d", hours, minutes, seconds);

            lcd.setCursor(pos_time.col, pos_time.row);
            lcd.print(str_time);

            current_time_ms = prev_time_ms;
        }

        btn.handle();
        
        digitalWrite(LED_BUILTIN, btn.isPressed());
    }
}
