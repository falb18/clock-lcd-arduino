#include <stdio.h>

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <AbleButtons.h>

/* Macros for DS3231 functions */
#define SET_12_HR_FRMT false
#define SET_PM_FRMT false

/* Macros for AbleButtons functions: */

/* Button to edit the date and time */
#define BUTTON_EDIT_PIN 2

/* Button to confirm the value for date and time */
#define BUTTON_SET_PIN 3

/* Application macros: */

/* The total number of editable paramaters in date and time strings */
#define NUM_DATE_TIME_PARAMS 5

#define BLINKING_PARAM_MS 300UL
#define BLINK_CLEAR_PARAM 0x00
#define BLINK_SET_PARAM 0x01

LiquidCrystal_I2C lcd(0x27,16,2);

DS3231 rtc;

using Button = AblePullupClickerButton;
using ButtonList = AblePullupClickerButtonList;

Button btn_edit(BUTTON_EDIT_PIN);
Button btn_set(BUTTON_SET_PIN);

/* Array with the menu's buttons */
Button *btns_menu[] = {
    &btn_edit,
    &btn_set
};

ButtonList btnList(btns_menu);

bool h12_flag = SET_12_HR_FRMT;
bool pm_flag = SET_PM_FRMT;
bool century = false;

unsigned long prev_time_ms = 0;
unsigned long current_time_ms = 0;

struct lcd_position {
    uint8_t col;
    uint8_t row;
};

struct rtc_date_t {
    char str_date[11]; /* 10 characters for the date + null character */

    uint8_t date;
    uint8_t month;
    uint8_t year;

    uint8_t str_lcd_col;
    uint8_t str_lcd_row;

    lcd_position pos_date_date;
    lcd_position pos_date_month;
    lcd_position pos_date_year;
}rtc_date;

struct rtc_time_t {
    char str_time[9]; /* 8 characters for the date + null character */

    uint8_t hours;
    uint8_t mins;
    uint8_t secs;

    uint8_t str_lcd_col;
    uint8_t str_lcd_row;

    lcd_position pos_time_hr;
    lcd_position pos_time_min;
    lcd_position pos_time_sec;
}rtc_time;

struct lcd_position *date_time_positions[6] = {
    &rtc_date.pos_date_date,
    &rtc_date.pos_date_month,
    &rtc_date.pos_date_year,
    &rtc_time.pos_time_hr,
    &rtc_time.pos_time_min,
    &rtc_time.pos_time_sec
};

void setup(void)
{
    lcd.init();
    lcd.backlight();

    btnList.begin();
    btn_edit.setDebounceTime(20);
    btn_set.setDebounceTime(20);

    prev_time_ms = millis();
    current_time_ms = prev_time_ms;

    /* Initialize variables for current date and time */
    rtc_date.str_date[11] = {0};
    rtc_date.date = 0;
    rtc_date.month = 0;
    rtc_date.year = 0;
    rtc_date.str_lcd_col = 3;
    rtc_date.str_lcd_row = 0;
    rtc_date.pos_date_date = {3, 0};
    rtc_date.pos_date_month = {6, 0};
    rtc_date.pos_date_year = {11, 0};

    rtc_time.str_time[9] = {0};
    rtc_time.hours = 0;
    rtc_time.mins = 0;
    rtc_time.secs = 0;
    rtc_time.str_lcd_col = 4;
    rtc_time.str_lcd_row = 1;
    rtc_time.pos_time_hr = {4, 1};
    rtc_time.pos_time_min = {7, 1};
    rtc_time.pos_time_sec = {10, 1};

    rtc_time.hours = rtc.getHour(h12_flag, pm_flag);
    rtc_time.mins = rtc.getMinute();
    rtc_time.secs = rtc.getSecond();

    rtc_date.date = rtc.getDate();
    rtc_date.month = rtc.getMonth(century);
    rtc_date.year = rtc.getYear();

    display_date_time();
}

void loop(void)
{
    btnList.handle();
    current_time_ms = millis();
    
    if ( (current_time_ms - prev_time_ms) >= 1000UL) {
        display_date_time();
        prev_time_ms = current_time_ms;
    }

    if (btn_edit.isClicked() == true) {
        btn_edit.resetClicked();
        edit_date_time();
    }
}

void display_date_time(void)
{
    rtc_time.hours = rtc.getHour(h12_flag, pm_flag);
    rtc_time.mins = rtc.getMinute();
    rtc_time.secs = rtc.getSecond();

    sprintf(rtc_time.str_time, "%02u:%02u:%02u", rtc_time.hours, rtc_time.mins, rtc_time.secs);
    lcd.setCursor(rtc_time.str_lcd_col, rtc_time.str_lcd_row);
    lcd.print(rtc_time.str_time);

    sprintf(rtc_date.str_date, "%02u/%02u/20%02u", rtc_date.date, rtc_date.month, rtc_date.year);
    lcd.setCursor(rtc_date.str_lcd_col,rtc_date.str_lcd_row);
    lcd.print(rtc_date.str_date);
}

void edit_date_time(void)
{
    byte rtc_current[6] = {
        rtc_date.date,
        rtc_date.month,
        rtc_date.year,
        rtc_time.hours,
        rtc_time.mins,
        rtc_time.secs
    };

    char str_param[2] = {0};

    uint8_t param_idx = 0;
    uint8_t tmp_lcd_col = 0;
    uint8_t tmp_lcd_row = 0;

    sprintf(str_param, "%02u", rtc_current[param_idx]);

    /* Loop through each parameter in the date and time */
    while(param_idx <= NUM_DATE_TIME_PARAMS) {
        btnList.handle();

        tmp_lcd_col = date_time_positions[param_idx]->col;
        tmp_lcd_row = date_time_positions[param_idx]->row;
        blink_parameter(tmp_lcd_col, tmp_lcd_row, str_param);

        /* Edit the next parameter on the LCD after the button edit is clicked */
        if(btn_edit.isClicked() == true) {
            lcd.setCursor(tmp_lcd_col, tmp_lcd_row);
            lcd.print(str_param);

            param_idx += 1;
            sprintf(str_param, "%02u", rtc_current[param_idx]);

            btn_edit.resetClicked();
        }
    }
}

void blink_parameter(uint8_t lcd_col, uint8_t lcd_row, char *str_param)
{
    const char *str_blank_space = "  ";
    static uint8_t param_txt_flag = 0x01;
    static uint16_t blink_current_time = 0;
    static uint16_t blink_prev_time = 0;

    blink_current_time = millis();

    /* After the time has elapsed, switch between the blank space or the parameter's characters */
    if ( (blink_current_time - blink_prev_time) >= BLINKING_PARAM_MS) {
        blink_prev_time = blink_current_time;
        param_txt_flag ^= 1;
        lcd.setCursor(lcd_col, lcd_row);

        if (param_txt_flag == BLINK_CLEAR_PARAM) {
            lcd.print(str_blank_space);
        } else {
            lcd.print(str_param);
        }
    }
}
