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

byte hours = 0;
byte minutes = 0;
byte seconds = 0;

byte date = 0;
byte month = 1;
byte year = 0;

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

    btnList.begin();
    btn_edit.setDebounceTime(20);
    btn_set.setDebounceTime(20);

    pinMode(LED_BUILTIN, OUTPUT);

    prev_time_ms = millis();
    current_time_ms = prev_time_ms;

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
    static char str_time[8] = {0};
    static char str_date[10] = {0};

    struct lcd_position pos_time = {4, 1};
    struct lcd_position pos_date = {3, 0};

    hours = rtc.getHour(h12_flag, pm_flag);
    minutes = rtc.getMinute();
    seconds = rtc.getSecond();
    sprintf(str_time, "%02d:%02d:%02d", hours, minutes, seconds);

    lcd.setCursor(pos_time.col, pos_time.row);
    lcd.print(str_time);

    sprintf(str_date, "%02d/%02d/20%02d", date, month, year);

    lcd.setCursor(pos_date.col, pos_date.row);
    lcd.print(str_date);
}

void edit_date_time(void)
{
    struct lcd_position pos_date_date = {3, 0};
    struct lcd_position pos_time_month = {6, 0};
    struct lcd_position pos_date_year = {11, 0};

    struct lcd_position pos_time_hr = {4, 1};
    struct lcd_position pos_time_min = {7, 1};
    struct lcd_position pos_time_sec = {10, 1};

    byte rtc_current[6] = {date, month, year, hours, minutes, seconds};
    struct lcd_position date_time_positions[6] = {
        pos_date_date,
        pos_time_month,
        pos_date_year,
        pos_time_hr,
        pos_time_min,
        pos_time_sec
    };

    char str_param[2] = {0};

    uint8_t param_idx = 0;
    uint8_t tmp_lcd_col = 0;
    uint8_t tmp_lcd_row = 0;

    sprintf(str_param, "%02u", rtc_current[param_idx]);

    /* Loop through each parameter in the date and time */
    while(param_idx <= NUM_DATE_TIME_PARAMS) {
        btnList.handle();

        tmp_lcd_col = date_time_positions[param_idx].col;
        tmp_lcd_row = date_time_positions[param_idx].row;
        blink_parameter(tmp_lcd_col, tmp_lcd_row, str_param);

        /* Edit the next parameter on the LCD after the button edit is clicked */
        if(btn_edit.isClicked() == true) {
            tmp_lcd_col = date_time_positions[param_idx].col;
            tmp_lcd_row = date_time_positions[param_idx].row;
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
