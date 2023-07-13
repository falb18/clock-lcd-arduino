#include <stdio.h>

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <AbleButtons.h>

#include "timer_module.h"

/* Macros for DS3231 functions */
#define HR_12_FRMT false
#define PM_FRMT false

/* Macros for AbleButtons functions: */

/* Button to edit the date and time */
#define BUTTON_EDIT_PIN 2

/* Button to confirm the value for date and time */
#define BUTTON_SET_PIN 3

/* Application macros: */

/* The total number of editable paramaters in date and time strings */
#define NUM_DATE_TIME_PARAMS 6

#define DATE_IDX 0
#define MONTH_IDX 1
#define YEAR_IDX 2
#define WEEK_IDX 3
#define HOUR_IDX 4
#define MIN_IDX 5
#define SEC_IDX 6

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

bool h12_flag = HR_12_FRMT;
bool pm_flag = PM_FRMT;
bool century = false;

char *str_days_week[] = {
    "MON",
    "TUE",
    "WEN",
    "THU",
    "FRI",
    "SAT",
    "SUN"
};

struct timer_t timer;

struct lcd_position {
    uint8_t col;
    uint8_t row;
};

struct rtc_date_t {
    /* 10 characters for the date + space + 3 characters for the day of the week + null character */
    char str_date[15];

    uint8_t date;
    uint8_t month;
    uint8_t year;
    uint8_t day_week;

    uint8_t str_lcd_col;
    uint8_t str_lcd_row;

    lcd_position pos_date_date;
    lcd_position pos_date_month;
    lcd_position pos_date_year;
    lcd_position pos_day_week;
}rtc_date;

struct rtc_time_t {
    char str_time[9]; /* 8 characters for the time + null character */

    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    uint8_t str_lcd_col;
    uint8_t str_lcd_row;

    lcd_position pos_time_hr;
    lcd_position pos_time_min;
    lcd_position pos_time_sec;
}rtc_time;

struct lcd_position *date_time_positions[7] = {
    &rtc_date.pos_date_date,
    &rtc_date.pos_date_month,
    &rtc_date.pos_date_year,
    &rtc_date.pos_day_week,
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

    reset_timer(&timer, 1000UL);

    /* Initialize variables for current date and time */
    rtc_date.str_date[11] = {0};
    rtc_date.date = 0;
    rtc_date.month = 0;
    rtc_date.year = 0;
    rtc_date.str_lcd_col = 1;
    rtc_date.str_lcd_row = 0;
    rtc_date.pos_date_date = {1, 0};
    rtc_date.pos_date_month = {4, 0};
    rtc_date.pos_date_year = {9, 0};
    rtc_date.pos_day_week = {12, 0};

    rtc_time.str_time[9] = {0};
    rtc_time.hour = 0;
    rtc_time.min = 0;
    rtc_time.sec = 0;
    rtc_time.str_lcd_col = 4;
    rtc_time.str_lcd_row = 1;
    rtc_time.pos_time_hr = {4, 1};
    rtc_time.pos_time_min = {7, 1};
    rtc_time.pos_time_sec = {10, 1};

    rtc_time.hour = rtc.getHour(h12_flag, pm_flag);
    rtc_time.min = rtc.getMinute();
    rtc_time.sec = rtc.getSecond();

    rtc_date.date = rtc.getDate();
    rtc_date.month = rtc.getMonth(century);
    rtc_date.year = rtc.getYear();
    rtc_date.day_week = rtc.getDoW();

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
    rtc_time.hour = rtc.getHour(h12_flag, pm_flag);
    rtc_time.min = rtc.getMinute();
    rtc_time.sec = rtc.getSecond();

    if( (rtc_time.hour == 0) && (rtc_time.min == 0) ) {
        rtc_date.date = rtc.getDate();
        rtc_date.month = rtc.getMonth(century);
        rtc_date.year = rtc.getYear();
    }

    sprintf(rtc_time.str_time, "%02u:%02u:%02u", rtc_time.hour, rtc_time.min, rtc_time.sec);
    sprintf(rtc_date.str_date, "%02u/%02u/20%02u %s",
                rtc_date.date, rtc_date.month, rtc_date.year,
                str_days_week[rtc_date.day_week]);

    lcd.setCursor(rtc_date.str_lcd_col,rtc_date.str_lcd_row);
    lcd.print(rtc_date.str_date);

    lcd.setCursor(rtc_time.str_lcd_col, rtc_time.str_lcd_row);
    lcd.print(rtc_time.str_time);
}

void edit_date_time(void)
{
    byte rtc_current[7] = {
        rtc_date.date,
        rtc_date.month,
        rtc_date.year,
        rtc_date.day_week,
        rtc_time.hour,
        rtc_time.min,
        rtc_time.sec
    };

    /* 2 chars for date or time, 3 chars for day of the week + null character */
    char str_param[4] = {0};

    uint8_t param_idx = DATE_IDX;
    uint8_t tmp_lcd_col = 0;
    uint8_t tmp_lcd_row = 0;
    uint8_t date_time_flags = 0x00;
    uint8_t str_week_idx = 0;

    reset_timer(&timer, BLINKING_PARAM_MS);

    /* Loop through each parameter in the date and time */
    while(param_idx <= NUM_DATE_TIME_PARAMS) {
        btnList.handle();

        tmp_lcd_col = date_time_positions[param_idx]->col;
        tmp_lcd_row = date_time_positions[param_idx]->row;

        if(param_idx == WEEK_IDX) {
            str_week_idx = rtc_current[param_idx];
            sprintf(str_param, "%s", str_days_week[str_week_idx]);
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

    update_date_time(date_time_flags, rtc_current);
}

void update_date_time(uint8_t date_time_flags, uint8_t *current_date_time)
{
    uint8_t eval_flags;

    /* Loop through each flag to determine which date and time parameters we have to update */
    for( eval_flags = 0x01; (eval_flags <= UPDATE_SEC); eval_flags <<= 1) {
        switch ( (eval_flags & date_time_flags) )
        {
        case UPDATE_DATE:
            rtc_date.date = current_date_time[DATE_IDX];
            rtc.setDate(rtc_date.date);
            break;
        
        case UPDATE_MONTH:
            rtc_date.month = current_date_time[MONTH_IDX];
            rtc.setMonth(rtc_date.month);
            break;
        
        case UPDATE_YEAR:
            rtc_date.year = current_date_time[YEAR_IDX];
            rtc.setYear(rtc_date.year);
            break;
        
        case UPDATE_WEEK:
            rtc_date.day_week = current_date_time[WEEK_IDX];
            rtc.setDoW(rtc_date.day_week);
            break;
        
        case UPDATE_HOUR:
            rtc_time.hour = current_date_time[HOUR_IDX];
            rtc.setHour(rtc_time.hour);
            break;
        
        case UPDATE_MIN:
            rtc_time.min = current_date_time[MIN_IDX];
            rtc.setMinute(rtc_time.min);
            break;

        case UPDATE_SEC:
            rtc_time.sec = current_date_time[SEC_IDX];
            rtc.setSecond(rtc_time.sec);
            break;
        
        default:
            break;
        }
    }
}

void blink_parameter(uint8_t param_idx, uint8_t lcd_col, uint8_t lcd_row, char *str_param)
{
   char str_blank_space[4] = {0}; 
    static uint8_t param_txt_flag = 0x01;
 
    timer_timeout(&timer);
    if(timer.timeout == false) {
        return;
    }

    timer.timeout = RESET_TIMER_VALUE;
    param_txt_flag ^= 1;

    (param_idx != WEEK_IDX) ? sprintf(str_blank_space, "%s", "  ") : sprintf(str_blank_space, "%s", "   ");
    
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
    
    case WEEK_IDX:
        (*param == 6) ? (*param = 0) : (*param += 1);
        *date_time_flags |= UPDATE_WEEK;
        break;
    
    case HOUR_IDX:
        if (h12_flag == true) {
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
    
    default:
        break;
    }
}