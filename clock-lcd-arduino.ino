#include <stdio.h>

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <AbleButtons.h>

#include "timer_module.h"

/* Macros for DS3231 functions: */

/* Flag for 12h (true) or 24h (false) format */
#define HR_12_FRMT true

/* AM (false) PM (true) */
#define AM_PM_FRMT false

/* Macros for AbleButtons functions: */

/* Button to edit the date and time */
#define BUTTON_EDIT_PIN 2

/* Button to confirm the value for date and time */
#define BUTTON_SET_PIN 3

/* Application macros: */

/* The total number of editable paramaters in date and time strings */
#define NUM_DATE_TIME_PARAMS 7

#define DATE_IDX 0
#define MONTH_IDX 1
#define YEAR_IDX 2
#define DAY_WEEK_IDX 3
#define HOUR_IDX 4
#define MIN_IDX 5
#define SEC_IDX 6
#define HOUR_MODE_IDX 7

#define MONDAY_IDX 0
#define TUESDAY_IDX 1
#define WENSDAY_IDX 2
#define THURSDAY_IDX 3
#define FRIDAY_IDX 4
#define SATURDAY_IDX 5
#define SUNDAY_IDX 6

/* Each flag determines if it needs to be updated on the RTC module or not */
#define UPDATE_DATE 0x01
#define UPDATE_MONTH 0x02
#define UPDATE_YEAR 0x04
#define UPDATE_WEEK 0x08
#define UPDATE_HOUR 0x10
#define UPDATE_MIN 0x20
#define UPDATE_SEC 0x40
#define UPDATE_HOUR_MODE 0x80

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

bool century = false;

/* According to the datasheet the values for the days of the week goes from 1 - 7 */
const char *str_days_week[] = {NULL, "MON", "TUE", "WEN", "THU", "FRI", "SAT", "SUN"};

const char *str_hour_modes[] = {"24h", "12h"};

const char *str_am_pm[] = {"AM", "PM"};

/* 10 characters for the date + space + 3 characters for the day of the week + null character */
char str_date[15];

/* 8 characters for the time + space character + 3 characters for the hour mode + null character */
char str_time[13];

struct timer_t timer;

struct date_time_t {
    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t day_week;

    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    
    uint8_t hour_mode;
    bool h12_flag;
    bool pm_flag;
} date_time;

struct lcd_position {
    uint8_t col;
    uint8_t row;
};

lcd_position pos_date_day {1, 0};
lcd_position pos_date_month {4, 0};
lcd_position pos_date_year {9, 0};
lcd_position pos_day_week {12, 0};

lcd_position pos_time_hr {4, 1};
lcd_position pos_time_min {7, 1};
lcd_position pos_time_sec {10, 1};
lcd_position pos_hour_mode {13, 1};

struct lcd_position *date_time_positions[] = {
    &pos_date_day,
    &pos_date_month,
    &pos_date_year,
    &pos_day_week,
    &pos_time_hr,
    &pos_time_min,
    &pos_time_sec,
    &pos_hour_mode
};

void setup(void)
{
    lcd.init();
    lcd.backlight();

    btnList.begin();
    btn_edit.setDebounceTime(20);
    btn_set.setDebounceTime(20);

    reset_timer(&timer, 1000UL);

    /* Initialize variables for current date and time */
    str_date[11] = {0};
    str_time[13] = {0};

    date_time.sec = rtc.getSecond();
    date_time.min = rtc.getMinute();
    date_time.hour = rtc.getHour(date_time.h12_flag, date_time.pm_flag);

    date_time.date = rtc.getDate();
    date_time.month = rtc.getMonth(century);
    date_time.year = rtc.getYear();
    date_time.day_week = rtc.getDoW();

    display_date_time();
}

void loop(void)
{
    btnList.handle();
    timer_timeout(&timer);
    
    if (timer.timeout == true) {
        display_date_time();
        timer.timeout = RESET_TIMER_VALUE;
    }

    if (btn_edit.isClicked() == true) {
        btn_edit.resetClicked();
        edit_date_time();
    }
}

void display_date_time(void)
{
    uint8_t am_pm_idx = 0;
    
    date_time.hour = rtc.getHour(date_time.h12_flag, date_time.pm_flag);
    (date_time.h12_flag == true) ? (date_time.hour_mode = 1) : (date_time.hour_mode = 0);
    date_time.min = rtc.getMinute();
    date_time.sec = rtc.getSecond();

    if( (date_time.hour == 0) && (date_time.min == 0) ) {
        date_time.date = rtc.getDate();
        date_time.month = rtc.getMonth(century);
        date_time.year = rtc.getYear();
    }

    if (date_time.h12_flag == true) {
        am_pm_idx = (date_time.pm_flag == true) ? 1 : 0;
        sprintf(str_time, "%02u:%02u:%02u %s",
                date_time.hour, date_time.min, date_time.sec, str_am_pm[am_pm_idx]);
    } else {
        sprintf(str_time, "%02u:%02u:%02u", date_time.hour, date_time.min, date_time.sec);
    }
    
    sprintf(str_date, "%02u/%02u/20%02u %s",
                date_time.date, date_time.month, date_time.year,
                str_days_week[date_time.day_week]);

    lcd.setCursor(pos_date_day.col, pos_date_day.row);
    lcd.print(str_date);

    lcd.setCursor(pos_time_hr.col, pos_time_hr.row);
    lcd.print(str_time);
}

void edit_date_time(void)
{
    uint8_t rtc_current[] = {
        date_time.date,
        date_time.month,
        date_time.year,
        date_time.day_week,
        date_time.hour,
        date_time.min,
        date_time.sec,
        date_time.hour_mode
    };

    /* 2 chars for date or time, 3 chars for day of the week + null character */
    char str_param[4] = {0};

    uint8_t param_idx = DATE_IDX;
    uint8_t tmp_lcd_col = 0;
    uint8_t tmp_lcd_row = 0;
    uint8_t date_time_flags = 0x00;
    uint8_t str_week_idx = 0;
    uint8_t str_hour_mode_idx = 0;

    reset_timer(&timer, BLINKING_PARAM_MS);

    /* In "Edit mode" display the 12/24 hour mode */
    lcd.setCursor(pos_hour_mode.col, pos_hour_mode.row);
    lcd.print(str_hour_modes[rtc_current[HOUR_MODE_IDX]]);

    /* Loop through each parameter in the date and time
     * Exit from the "Edit mode" only after last parameter is reached
     */
    while(param_idx <= NUM_DATE_TIME_PARAMS) {
        btnList.handle();

        tmp_lcd_col = date_time_positions[param_idx]->col;
        tmp_lcd_row = date_time_positions[param_idx]->row;

        /* Get the string of the parameter which is going to blink */
        if (param_idx == DAY_WEEK_IDX) {
            str_week_idx = rtc_current[param_idx];
            sprintf(str_param, "%s", str_days_week[str_week_idx]);
        } else if (param_idx == HOUR_MODE_IDX) {
            str_hour_mode_idx = rtc_current[param_idx];
            sprintf(str_param, "%s", str_hour_modes[str_hour_mode_idx]);
        } else {
            sprintf(str_param, "%02u", rtc_current[param_idx]);
        }

        blink_parameter(param_idx, tmp_lcd_col, tmp_lcd_row, str_param);

        /* Edit the next parameter on the LCD after the button edit is clicked */
        if(btn_edit.isClicked() == true) {
            /* First, stop blinking animation on the current parameter before blinking the next one */
            lcd.setCursor(tmp_lcd_col, tmp_lcd_row);
            lcd.print(str_param);

            param_idx += 1;

            btn_edit.resetClicked();
            continue;
        }

        /* Update the parameter with the new value and show it on the display */
        if (btn_set.isClicked() == true) {
            increment_param(param_idx, &rtc_current[param_idx], &date_time_flags);
            
            lcd.setCursor(tmp_lcd_col, tmp_lcd_row);
            lcd.print(str_param);
            
            btn_set.resetClicked();
        }
    }

    /* Stop showing the 12/24 hour mode */
    lcd.setCursor(pos_hour_mode.col, pos_hour_mode.row);
    lcd.print("   ");

    update_date_time(date_time_flags, rtc_current);
}

void update_date_time(uint8_t date_time_flags, uint8_t *current_date_time)
{
    uint8_t eval_flags;

    /* Loop through each flag to determine which date and time parameters we have to update */
    for (eval_flags = 0x01; (eval_flags != 0x00); eval_flags <<= 1) {
        switch ( (eval_flags & date_time_flags) )
        {
        case UPDATE_DATE:
            date_time.date = current_date_time[DATE_IDX];
            rtc.setDate(date_time.date);
            break;
        
        case UPDATE_MONTH:
            date_time.month = current_date_time[MONTH_IDX];
            rtc.setMonth(date_time.month);
            break;
        
        case UPDATE_YEAR:
            date_time.year = current_date_time[YEAR_IDX];
            rtc.setYear(date_time.year);
            break;
        
        case UPDATE_WEEK:
            date_time.day_week = current_date_time[DAY_WEEK_IDX];
            rtc.setDoW(date_time.day_week);
            break;
        
        case UPDATE_HOUR:
            date_time.hour = current_date_time[HOUR_IDX];
            rtc.setHour(date_time.hour);
            break;
        
        case UPDATE_MIN:
            date_time.min = current_date_time[MIN_IDX];
            rtc.setMinute(date_time.min);
            break;

        case UPDATE_SEC:
            date_time.sec = current_date_time[SEC_IDX];
            rtc.setSecond(date_time.sec);
            break;
        
        case UPDATE_HOUR_MODE:
            (current_date_time[HOUR_MODE_IDX] == 1) ?
                (rtc.setClockMode(true)) : (rtc.setClockMode(false));
            break;

        default:
            break;
        }
    }
}

/* This function creates the blinking effect in each parameter */
void blink_parameter(uint8_t param_idx, uint8_t lcd_col, uint8_t lcd_row, char *str_param)
{
   char str_blank_space[4] = {0}; 
    static uint8_t param_txt_flag = 0x01;
 
    timer_timeout(&timer);
    if(timer.timeout == false) {
        return;
    }

    timer.timeout = RESET_TIMER_VALUE;
    /* Alternate between 0 and 1. 1 = show text, 0 = blank space */
    param_txt_flag ^= 1;

    (param_idx != DAY_WEEK_IDX) ? sprintf(str_blank_space, "%s", "  ") : sprintf(str_blank_space, "%s", "   ");
    
    lcd.setCursor(lcd_col, lcd_row);
    (param_txt_flag == BLINK_CLEAR_PARAM) ? lcd.print(str_blank_space) : lcd.print(str_param);
}

void increment_param(uint8_t param_idx, uint8_t *param, uint8_t *date_time_flags)
{
    /* Increment the value of the given parameter but always keep track when the variable has to be
     * reset after certain limit
     */
    switch (param_idx)
    {
    case DATE_IDX:
        (*param == 31) ? (*param = 1) : (*param += 1);
        *date_time_flags |= UPDATE_DATE;
        break;
    
    case MONTH_IDX:
        (*param == 12) ? (*param = 1) : (*param += 1);
        *date_time_flags |= UPDATE_MONTH;
        break;
    
    case YEAR_IDX:
        (*param == 99) ? (*param = 0) : (*param += 1);
        *date_time_flags |= UPDATE_YEAR;
        break;
    
    case DAY_WEEK_IDX:
        (*param == 7) ? (*param = 1) : (*param += 1);
        *date_time_flags |= UPDATE_WEEK;
        break;
    
    case HOUR_IDX:
        if (date_time.h12_flag == true) {
            (*param == 12) ? (*param = 0) : (*param += 1);
        } else {
            (*param == 23) ? (*param = 0) : (*param += 1);
        }
        *date_time_flags |= UPDATE_HOUR;
        break;
    
    case MIN_IDX:
        (*param == 59) ? (*param = 0) : (*param += 1);
        *date_time_flags |= UPDATE_MIN;
        break;
    
    case SEC_IDX:
        (*param == 59) ? (*param = 0) : (*param += 1);
        *date_time_flags |= UPDATE_SEC;
        break;

    case HOUR_MODE_IDX:
        (*param == 1) ? (*param = 0) : (*param += 1);
        *date_time_flags |= UPDATE_HOUR_MODE;
        break;
    
    default:
        break;
    }
}