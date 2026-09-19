#include "rtc_app.h"

#include <avr/interrupt.h>

#include "../config.h"
#include "../hal/buzzer/buzzer_interface.h"
#include "../hal/rtc/ds3231_interface.h"
#include "../hal/keypad/keypad_interface.h"
#include "../hal/lcd/lcd_interface.h"
#include "../mcal/timer0/timer0_interface.h"

typedef enum
{
    APP_MODE_CLOCK = 0,
    APP_MODE_ALARM,
    APP_MODE_STOPWATCH,
    APP_MODE_COUNTDOWN,
    APP_MODE_SET_TIME,
    APP_MODE_SET_DATE,
    APP_MODE_EDIT_ALARM,
    APP_MODE_EDIT_COUNTDOWN
} AppMode;

typedef enum
{
    BUZZER_SOURCE_NONE = 0,
    BUZZER_SOURCE_ALARM,
    BUZZER_SOURCE_COUNTDOWN
} BuzzerSource;

typedef struct
{
    uint8 hour;
    uint8 minute;
    uint8 second;
} TimeFields;

typedef struct
{
    uint8 hour;
    uint8 minute;
} AlarmFields;

typedef struct
{
    uint8 hour;
    uint8 minute;
    uint8 second;
} CountdownFields;

static AppMode g_mode = APP_MODE_CLOCK;
static AppMode g_returnMode = APP_MODE_CLOCK;
static uint8 g_editField = 0U;
static uint8 g_numericDigits = 0U;
static uint16 g_previousFieldValue = 0U;
static uint8 g_alarmEnabled = 0U;
static uint8 g_alarmTriggeredToday = 0U;
static BuzzerSource g_buzzerSource = BUZZER_SOURCE_NONE;

static Ds3231DateTime g_currentTime = {0U, 0U, 0U, 1U, 1U, 1U, 2026U};
static Ds3231DateTime g_setTime = {0U, 0U, 0U, 1U, 1U, 1U, 2026U};
static Ds3231DateTime g_setDate = {0U, 0U, 0U, 1U, 1U, 1U, 2026U};
static AlarmFields g_alarm = {6U, 30U};
static AlarmFields g_alarmEdit = {6U, 30U};
static TimeFields g_stopwatch = {0U, 0U, 0U};
static CountdownFields g_countdownPreset = {0U, 1U, 0U};
static CountdownFields g_countdownEdit = {0U, 1U, 0U};
static uint32 g_stopwatchBaseMs = 0U;
static uint32 g_countdownBaseMs = 0U;
static uint8 g_stopwatchRunning = 0U;
static uint8 g_countdownRunning = 0U;
static uint32 g_buzzerStopAtMs = 0U;
static uint32 g_lastDisplayMs = 0U;
static uint32 g_lastRtcReadMs = 0U;
static uint32 g_lastCursorBlinkMs = 0U;
static uint8 g_lastShownMode = 0xFFU;
static uint8 g_screenDirty = 1U;
static uint8 g_cursorVisible = 1U;
static uint8 g_rtcError = 0U;
static uint8 g_inputError = 0U;
static uint32 g_inputErrorStartedAtMs = 0U;

static void App_stopBuzzer(void);

static void App_markScreenDirty(void)
{
    g_screenDirty = 1U;
}

static uint8 App_isEditMode(AppMode mode)
{
    return ((mode == APP_MODE_SET_TIME) || (mode == APP_MODE_SET_DATE) || (mode == APP_MODE_EDIT_ALARM) || (mode == APP_MODE_EDIT_COUNTDOWN)) ? 1U : 0U;
}

static uint8 App_isLeapYear(uint16 year);
static uint8 App_daysInMonth(uint8 month, uint16 year);
static uint8 App_isValidDateTime(const Ds3231DateTime *dateTime);

static void App_setInputError(void)
{
    g_inputError = 1U;
    g_inputErrorStartedAtMs = Timer0_getMillis();
    g_numericDigits = 0U;
    App_markScreenDirty();
}

static uint8 App_isInputErrorActive(void)
{
    return (g_inputError != 0U) && ((uint32)(Timer0_getMillis() - g_inputErrorStartedAtMs) < 2000UL);
}

static uint8 App_isCurrentFieldValid(void)
{
    if (g_mode == APP_MODE_SET_TIME)
    {
        if (g_editField == 0U) return (g_setTime.hour < 24U) ? 1U : 0U;
        if (g_editField == 1U) return (g_setTime.minute < 60U) ? 1U : 0U;
        return (g_setTime.second < 60U) ? 1U : 0U;
    }

    if (g_mode == APP_MODE_SET_DATE)
    {
        if (g_editField == 0U)
        {
            if ((g_setDate.month < 1U) || (g_setDate.month > 12U)) return 0U;
            return (g_setDate.date >= 1U) && (g_setDate.date <= App_daysInMonth(g_setDate.month, g_setDate.year));
        }
        if (g_editField == 1U)
        {
            if ((g_setDate.month < 1U) || (g_setDate.month > 12U)) return 0U;
            return (g_setDate.date >= 1U) && (g_setDate.date <= App_daysInMonth(g_setDate.month, g_setDate.year));
        }
        return (g_setDate.year >= 2000U) && (g_setDate.year <= 2099U);
    }

    if (g_mode == APP_MODE_EDIT_ALARM)
    {
        if (g_editField == 0U) return (g_alarmEdit.hour < 24U) ? 1U : 0U;
        return (g_alarmEdit.minute < 60U) ? 1U : 0U;
    }

    if (g_editField == 0U) return (g_countdownEdit.hour <= APP_COUNTDOWN_MAX_HOURS) ? 1U : 0U;
    if (g_editField == 1U) return (g_countdownEdit.minute < 60U) ? 1U : 0U;
    return (g_countdownEdit.second < 60U) ? 1U : 0U;
}

static void App_restoreCurrentField(void)
{
    if (g_mode == APP_MODE_SET_TIME)
    {
        if (g_editField == 0U) g_setTime.hour = (uint8)g_previousFieldValue;
        else if (g_editField == 1U) g_setTime.minute = (uint8)g_previousFieldValue;
        else g_setTime.second = (uint8)g_previousFieldValue;
    }
    else if (g_mode == APP_MODE_SET_DATE)
    {
        if (g_editField == 0U) g_setDate.date = (uint8)g_previousFieldValue;
        else if (g_editField == 1U) g_setDate.month = (uint8)g_previousFieldValue;
        else g_setDate.year = g_previousFieldValue;
    }
    else if (g_mode == APP_MODE_EDIT_ALARM)
    {
        if (g_editField == 0U) g_alarmEdit.hour = (uint8)g_previousFieldValue;
        else g_alarmEdit.minute = (uint8)g_previousFieldValue;
    }
    else
    {
        if (g_editField == 0U) g_countdownEdit.hour = (uint8)g_previousFieldValue;
        else if (g_editField == 1U) g_countdownEdit.minute = (uint8)g_previousFieldValue;
        else g_countdownEdit.second = (uint8)g_previousFieldValue;
    }
}

static uint8 App_isEditedValueValid(void)
{
    if (g_numericDigits != 0U)
    {
        return 0U;
    }

    if (g_mode == APP_MODE_SET_TIME)
    {
        return (g_setTime.hour < 24U) && (g_setTime.minute < 60U) && (g_setTime.second < 60U);
    }

    if (g_mode == APP_MODE_SET_DATE)
    {
        return App_isValidDateTime(&g_setDate);
    }

    if (g_mode == APP_MODE_EDIT_ALARM)
    {
        return (g_alarmEdit.hour < 24U) && (g_alarmEdit.minute < 60U);
    }

    return (g_countdownEdit.hour <= APP_COUNTDOWN_MAX_HOURS) && (g_countdownEdit.minute < 60U) && (g_countdownEdit.second < 60U);
}

static uint8 App_numericFieldWidth(void)
{
    return ((g_mode == APP_MODE_SET_DATE) && (g_editField == 2U)) ? 4U : 2U;
}

static uint8 App_handleEditNavigation(char key)
{
    uint8 fieldCount;

    if ((App_isEditMode(g_mode) == 0U) || ((key != '#') && (key != '*')))
    {
        return 0U;
    }

    fieldCount = (g_mode == APP_MODE_EDIT_ALARM) ? 2U : 3U;
    if (key == '#')
    {
        g_editField = (uint8)((g_editField + 1U) % fieldCount);
    }
    else
    {
        g_editField = (g_editField == 0U) ? (uint8)(fieldCount - 1U) : (uint8)(g_editField - 1U);
    }

    g_numericDigits = 0U;
    App_markScreenDirty();
    return 1U;
}

static uint8 App_handleEditStep(char key)
{
    uint8 increment;

    if ((App_isEditMode(g_mode) == 0U) || ((key != 'B') && (key != 'C')))
    {
        return 0U;
    }

    increment = (key == 'B') ? 1U : 0U;
    g_numericDigits = 0U;
    g_inputError = 0U;

    if (g_mode == APP_MODE_SET_TIME)
    {
        if (g_editField == 0U)
            g_setTime.hour = increment ? (uint8)((g_setTime.hour + 1U) % 24U) : ((g_setTime.hour == 0U) ? 23U : (uint8)(g_setTime.hour - 1U));
        else if (g_editField == 1U)
            g_setTime.minute = increment ? (uint8)((g_setTime.minute + 1U) % 60U) : ((g_setTime.minute == 0U) ? 59U : (uint8)(g_setTime.minute - 1U));
        else
            g_setTime.second = increment ? (uint8)((g_setTime.second + 1U) % 60U) : ((g_setTime.second == 0U) ? 59U : (uint8)(g_setTime.second - 1U));
    }
    else if (g_mode == APP_MODE_SET_DATE)
    {
        if (g_editField == 0U)
        {
            uint8 days = App_daysInMonth(g_setDate.month, g_setDate.year);
            g_setDate.date = increment ? ((g_setDate.date >= days) ? 1U : (uint8)(g_setDate.date + 1U)) : ((g_setDate.date <= 1U) ? days : (uint8)(g_setDate.date - 1U));
        }
        else if (g_editField == 1U)
        {
            g_setDate.month = increment ? ((g_setDate.month >= 12U) ? 1U : (uint8)(g_setDate.month + 1U)) : ((g_setDate.month <= 1U) ? 12U : (uint8)(g_setDate.month - 1U));
            if (g_setDate.date > App_daysInMonth(g_setDate.month, g_setDate.year)) g_setDate.date = App_daysInMonth(g_setDate.month, g_setDate.year);
        }
        else
        {
            g_setDate.year = increment ? ((g_setDate.year >= 2099U) ? 2000U : (uint16)(g_setDate.year + 1U)) : ((g_setDate.year <= 2000U) ? 2099U : (uint16)(g_setDate.year - 1U));
            if (g_setDate.date > App_daysInMonth(g_setDate.month, g_setDate.year)) g_setDate.date = App_daysInMonth(g_setDate.month, g_setDate.year);
        }
    }
    else if (g_mode == APP_MODE_EDIT_ALARM)
    {
        if (g_editField == 0U)
            g_alarmEdit.hour = increment ? (uint8)((g_alarmEdit.hour + 1U) % 24U) : ((g_alarmEdit.hour == 0U) ? 23U : (uint8)(g_alarmEdit.hour - 1U));
        else
            g_alarmEdit.minute = increment ? (uint8)((g_alarmEdit.minute + 1U) % 60U) : ((g_alarmEdit.minute == 0U) ? 59U : (uint8)(g_alarmEdit.minute - 1U));
    }
    else
    {
        if (g_editField == 0U)
            g_countdownEdit.hour = increment ? ((g_countdownEdit.hour >= APP_COUNTDOWN_MAX_HOURS) ? 0U : (uint8)(g_countdownEdit.hour + 1U)) : ((g_countdownEdit.hour == 0U) ? APP_COUNTDOWN_MAX_HOURS : (uint8)(g_countdownEdit.hour - 1U));
        else if (g_editField == 1U)
            g_countdownEdit.minute = increment ? (uint8)((g_countdownEdit.minute + 1U) % 60U) : ((g_countdownEdit.minute == 0U) ? 59U : (uint8)(g_countdownEdit.minute - 1U));
        else
            g_countdownEdit.second = increment ? (uint8)((g_countdownEdit.second + 1U) % 60U) : ((g_countdownEdit.second == 0U) ? 59U : (uint8)(g_countdownEdit.second - 1U));
    }

    App_markScreenDirty();
    return 1U;
}

static uint8 App_handleNumericEdit(char key)
{
    uint8 digit;
    uint8 width;

    if ((App_isEditMode(g_mode) == 0U) || (key < '0') || (key > '9'))
    {
        return 0U;
    }

    digit = (uint8)(key - '0');
    width = App_numericFieldWidth();

    if (g_numericDigits == 0U)
    {
        if (g_mode == APP_MODE_SET_TIME)
        {
            if (g_editField == 0U) g_previousFieldValue = g_setTime.hour;
            else if (g_editField == 1U) g_previousFieldValue = g_setTime.minute;
            else g_previousFieldValue = g_setTime.second;
            if (g_editField == 0U) g_setTime.hour = digit;
            else if (g_editField == 1U) g_setTime.minute = digit;
            else g_setTime.second = digit;
        }
        else if (g_mode == APP_MODE_SET_DATE)
        {
            if (g_editField == 0U) g_previousFieldValue = g_setDate.date;
            else if (g_editField == 1U) g_previousFieldValue = g_setDate.month;
            else g_previousFieldValue = g_setDate.year;
            if (g_editField == 0U) g_setDate.date = digit;
            else if (g_editField == 1U) g_setDate.month = digit;
            else g_setDate.year = digit;
        }
        else if (g_mode == APP_MODE_EDIT_ALARM)
        {
            if (g_editField == 0U) g_previousFieldValue = g_alarmEdit.hour;
            else g_previousFieldValue = g_alarmEdit.minute;
            if (g_editField == 0U) g_alarmEdit.hour = digit;
            else g_alarmEdit.minute = digit;
        }
        else
        {
            if (g_editField == 0U) g_previousFieldValue = g_countdownEdit.hour;
            else if (g_editField == 1U) g_previousFieldValue = g_countdownEdit.minute;
            else g_previousFieldValue = g_countdownEdit.second;
            if (g_editField == 0U) g_countdownEdit.hour = digit;
            else if (g_editField == 1U) g_countdownEdit.minute = digit;
            else g_countdownEdit.second = digit;
        }
    }
    else if (g_mode == APP_MODE_SET_TIME)
    {
        if (g_editField == 0U) g_setTime.hour = (uint8)(g_setTime.hour * 10U + digit);
        else if (g_editField == 1U) g_setTime.minute = (uint8)(g_setTime.minute * 10U + digit);
        else g_setTime.second = (uint8)(g_setTime.second * 10U + digit);
    }
    else if (g_mode == APP_MODE_SET_DATE)
    {
        if (g_editField == 0U) g_setDate.date = (uint8)(g_setDate.date * 10U + digit);
        else if (g_editField == 1U) g_setDate.month = (uint8)(g_setDate.month * 10U + digit);
        else g_setDate.year = (uint16)(g_setDate.year * 10U + digit);
    }
    else if (g_mode == APP_MODE_EDIT_ALARM)
    {
        if (g_editField == 0U) g_alarmEdit.hour = (uint8)(g_alarmEdit.hour * 10U + digit);
        else g_alarmEdit.minute = (uint8)(g_alarmEdit.minute * 10U + digit);
    }
    else
    {
        if (g_editField == 0U) g_countdownEdit.hour = (uint8)(g_countdownEdit.hour * 10U + digit);
        else if (g_editField == 1U) g_countdownEdit.minute = (uint8)(g_countdownEdit.minute * 10U + digit);
        else g_countdownEdit.second = (uint8)(g_countdownEdit.second * 10U + digit);
    }

    g_numericDigits++;
    if (g_numericDigits >= width)
    {
        if (App_isCurrentFieldValid() == 0U)
        {
            App_restoreCurrentField();
            App_setInputError();
            return 1U;
        }

        g_numericDigits = 0U;
        if (g_mode == APP_MODE_EDIT_ALARM)
        {
            g_editField = (uint8)((g_editField + 1U) % 2U);
        }
        else
        {
            g_editField = (uint8)((g_editField + 1U) % 3U);
        }
    }

    App_markScreenDirty();
    return 1U;
}

static void App_applyBlinkMask(char *text, uint8 start, uint8 length)
{
    uint8 index;

    if (g_cursorVisible != 0U)
    {
        return;
    }

    for (index = 0U; index < length; index++)
    {
        text[start + index] = ' ';
    }
}

static void App_hideSelectedField(char *text, uint8 fieldIndex, uint8 fieldCount, uint8 fieldWidth, uint8 separatorWidth)
{
    uint8 start;

    if (fieldIndex >= fieldCount)
    {
        return;
    }

    start = (uint8)(fieldIndex * (fieldWidth + separatorWidth));
    App_applyBlinkMask(text, start, fieldWidth);
}

static void App_hideSelectedDateField(char *text, uint8 fieldIndex)
{
    switch (fieldIndex)
    {
    case 0U:
        App_applyBlinkMask(text, 0U, 2U);
        break;
    case 1U:
        App_applyBlinkMask(text, 3U, 2U);
        break;
    case 2U:
        App_applyBlinkMask(text, 6U, 4U);
        break;
    default:
        break;
    }
}

static uint8 App_isLeapYear(uint16 year)
{
    if ((year % 400U) == 0U)
    {
        return 1U;
    }

    if ((year % 100U) == 0U)
    {
        return 0U;
    }

    return ((year % 4U) == 0U) ? 1U : 0U;
}

static uint8 App_daysInMonth(uint8 month, uint16 year)
{
    static const uint8 days[] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};

    if ((month == 2U) && (App_isLeapYear(year) != 0U))
    {
        return 29U;
    }

    if ((month >= 1U) && (month <= 12U))
    {
        return days[month - 1U];
    }

    return 31U;
}

static uint8 App_isValidDateTime(const Ds3231DateTime *dateTime)
{
    if ((dateTime->second >= 60U) || (dateTime->minute >= 60U) || (dateTime->hour >= 24U))
    {
        return 0U;
    }

    if ((dateTime->month < 1U) || (dateTime->month > 12U) || (dateTime->date < 1U))
    {
        return 0U;
    }

    if (dateTime->date > App_daysInMonth(dateTime->month, dateTime->year))
    {
        return 0U;
    }

    return (dateTime->year >= 2000U) ? 1U : 0U;
}

static uint8 App_dayOfWeek(uint16 year, uint8 month, uint8 day)
{
    static const uint8 offset[] = {0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U};

    if (month < 3U)
    {
        year--;
    }

    return (uint8)(((year + (year / 4U) - (year / 100U) + (year / 400U) + offset[month - 1U] + day) % 7U) + 1U);
}

static uint32 App_timeToSeconds(const CountdownFields *countdown)
{
    return (((uint32)countdown->hour * 3600UL) + ((uint32)countdown->minute * 60UL) + (uint32)countdown->second);
}

static void App_secondsToCountdown(uint32 totalSeconds, CountdownFields *countdown)
{
    countdown->hour = (uint8)(totalSeconds / 3600UL);
    totalSeconds %= 3600UL;
    countdown->minute = (uint8)(totalSeconds / 60UL);
    countdown->second = (uint8)(totalSeconds % 60UL);
}

static void App_secondsToStopwatch(uint32 totalSeconds, TimeFields *stopwatch)
{
    stopwatch->hour = (uint8)(totalSeconds / 3600UL);
    totalSeconds %= 3600UL;
    stopwatch->minute = (uint8)(totalSeconds / 60UL);
    stopwatch->second = (uint8)(totalSeconds % 60UL);
}

static uint32 App_stopwatchToSeconds(const TimeFields *stopwatch)
{
    return (((uint32)stopwatch->hour * 3600UL) + ((uint32)stopwatch->minute * 60UL) + (uint32)stopwatch->second);
}

static void App_formatTime(char *buffer, uint8 hour, uint8 minute, uint8 second)
{
    buffer[0] = (char)('0' + (hour / 10U));
    buffer[1] = (char)('0' + (hour % 10U));
    buffer[2] = ':';
    buffer[3] = (char)('0' + (minute / 10U));
    buffer[4] = (char)('0' + (minute % 10U));
    buffer[5] = ':';
    buffer[6] = (char)('0' + (second / 10U));
    buffer[7] = (char)('0' + (second % 10U));
    buffer[8] = '\0';
}

static void App_formatDate(char *buffer, uint8 date, uint8 month, uint16 year)
{
    buffer[0] = (char)('0' + (date / 10U));
    buffer[1] = (char)('0' + (date % 10U));
    buffer[2] = '/';
    buffer[3] = (char)('0' + (month / 10U));
    buffer[4] = (char)('0' + (month % 10U));
    buffer[5] = '/';
    buffer[6] = (char)('0' + (year / 1000U));
    buffer[7] = (char)('0' + ((year / 100U) % 10U));
    buffer[8] = (char)('0' + ((year / 10U) % 10U));
    buffer[9] = (char)('0' + (year % 10U));
    buffer[10] = '\0';
}

static void App_formatHourMinute(char *buffer, uint8 hour, uint8 minute)
{
    buffer[0] = (char)('0' + (hour / 10U));
    buffer[1] = (char)('0' + (hour % 10U));
    buffer[2] = ':';
    buffer[3] = (char)('0' + (minute / 10U));
    buffer[4] = (char)('0' + (minute % 10U));
    buffer[5] = '\0';
}

static void App_formatCountdown(char *buffer, const CountdownFields *countdown)
{
    App_formatTime(buffer, countdown->hour, countdown->minute, countdown->second);
}

static void App_formatStopwatch(char *buffer, const TimeFields *stopwatch)
{
    App_formatTime(buffer, stopwatch->hour, stopwatch->minute, stopwatch->second);
}

static void App_padLine(char *line, const char *text, uint8 width)
{
    uint8 index = 0U;

    while ((text[index] != '\0') && (index < width))
    {
        line[index] = text[index];
        index++;
    }

    while (index < width)
    {
        line[index] = ' ';
        index++;
    }

    line[width] = '\0';
}

static void App_writeLine(uint8 row, const char *text)
{
    char line[17];

    App_padLine(line, text, 16U);
    LCD_displayStringRowColumn(row, 0U, line);
}

static void App_writeLine2(uint8 row, const char *left, const char *right)
{
    char line[17];
    uint8 index = 0U;
    uint8 charIndex = 0U;

    while ((left[charIndex] != '\0') && (index < 8U))
    {
        line[index] = left[charIndex];
        index++;
        charIndex++;
    }

    while (index < 8U)
    {
        line[index] = ' ';
        index++;
    }

    charIndex = 0U;
    while ((right[charIndex] != '\0') && (index < 16U))
    {
        line[index] = right[charIndex];
        index++;
        charIndex++;
    }

    while (index < 16U)
    {
        line[index] = ' ';
        index++;
    }

    line[16] = '\0';
    LCD_displayStringRowColumn(row, 0U, line);
}

static void App_renderClock(void)
{
    char line[17];
    char dateLine[17];
    uint8 index;

    App_formatTime(line, g_currentTime.hour, g_currentTime.minute, g_currentTime.second);
    App_writeLine2(0U, "Time:", line);
    App_formatDate(dateLine, g_currentTime.date, g_currentTime.month, g_currentTime.year);

    for (index = 10U; index > 0U; index--)
    {
        dateLine[index + 5U] = dateLine[index - 1U];
    }

    dateLine[0] = 'D';
    dateLine[1] = 'a';
    dateLine[2] = 't';
    dateLine[3] = 'e';
    dateLine[4] = ':';
    dateLine[5] = ' ';
    dateLine[16] = '\0';
    App_writeLine(1U, dateLine);
}

static void App_renderAlarm(void)
{
    char line[17];
    char timeLine[6];
    const char *status = g_alarmEnabled ? "ENABLED" : "DISABLED";
    uint8 index;

    App_formatHourMinute(timeLine, g_alarm.hour, g_alarm.minute);
    for (index = 0U; index < 16U; index++)
    {
        line[index] = ' ';
    }
    line[0] = 'A';
    line[1] = 'L';
    line[2] = 'A';
    line[3] = 'R';
    line[4] = 'M';
    for (index = 0U; index < 5U; index++)
    {
        line[index + 11U] = timeLine[index];
    }
    line[16] = '\0';
    LCD_displayStringRowColumn(0U, 0U, line);

    App_padLine(line, status, 16U);
    line[11] = 'B';
    line[12] = '=';
    line[13] = 'T';
    line[14] = 'G';
    line[15] = 'L';
    LCD_displayStringRowColumn(1U, 0U, line);
}

static void App_renderStopwatch(void)
{
    char line[17];
    char timeLine[9];
    const char *status = g_stopwatchRunning ? "RUNNING" : "STOPPED";
    uint8 index;

    for (index = 0U; index < 16U; index++)
    {
        line[index] = ' ';
    }
    line[0] = 'S';
    line[1] = 'T';
    line[2] = 'O';
    line[3] = 'P';
    line[4] = 'W';
    line[5] = 'A';
    line[6] = 'T';
    line[7] = 'C';
    line[8] = 'H';
    line[11] = 'D';
    line[12] = '=';
    line[13] = 'R';
    line[14] = 'S';
    line[15] = 'T';
    line[16] = '\0';
    LCD_displayStringRowColumn(0U, 0U, line);

    App_formatStopwatch(timeLine, &g_stopwatch);
    App_padLine(line, status, 16U);
    for (index = 0U; index < 8U; index++)
    {
        line[index + 8U] = timeLine[index];
    }
    LCD_displayStringRowColumn(1U, 0U, line);
}

static void App_renderCountdown(void)
{
    char line[17];

    App_writeLine(0U, "COUNTDOWN  #=SET");
    App_formatCountdown(line, &g_countdownPreset);
    App_writeLine(1U, line);
}

static void App_renderEditMode(const char *title, const Ds3231DateTime *value)
{
    char line[17];

    App_writeLine(0U, (App_isInputErrorActive() != 0U) ? "INVALID VALUE" : ((g_rtcError != 0U) ? "RTC ERR D=RETRY" : title));

    if (g_mode == APP_MODE_SET_TIME)
    {
        App_formatTime(line, value->hour, value->minute, value->second);
        App_hideSelectedField(line, g_editField, 3U, 2U, 1U);
    }
    else
    {
        App_formatDate(line, value->date, value->month, value->year);
        App_hideSelectedDateField(line, g_editField);
    }

    App_writeLine(1U, line);
}

static void App_renderEditAlarm(void)
{
    char line[17];

    App_writeLine(0U, (App_isInputErrorActive() != 0U) ? "INVALID VALUE" : "ALARM     D=SAVE");
    App_formatHourMinute(line, g_alarmEdit.hour, g_alarmEdit.minute);
    App_hideSelectedField(line, g_editField, 2U, 2U, 1U);
    App_writeLine(1U, line);
}

static void App_renderEditCountdown(void)
{
    char line[17];

    App_writeLine(0U, (App_isInputErrorActive() != 0U) ? "INVALID VALUE" : "EDIT COUNTDWN");
    App_formatCountdown(line, &g_countdownEdit);
    App_hideSelectedField(line, g_editField, 3U, 2U, 1U);
    App_writeLine(1U, line);
}

static void App_showCurrentScreen(void)
{
    g_screenDirty = 0U;

    switch (g_mode)
    {
    case APP_MODE_CLOCK:
        App_renderClock();
        break;
    case APP_MODE_ALARM:
        App_renderAlarm();
        break;
    case APP_MODE_STOPWATCH:
        App_renderStopwatch();
        break;
    case APP_MODE_COUNTDOWN:
        App_renderCountdown();
        break;
    case APP_MODE_SET_TIME:
        App_renderEditMode("SET TIME  D=SAVE", &g_setTime);
        break;
    case APP_MODE_SET_DATE:
        App_renderEditMode("SET DATE  D=SAVE", &g_setDate);
        break;
    case APP_MODE_EDIT_ALARM:
        App_renderEditAlarm();
        break;
    case APP_MODE_EDIT_COUNTDOWN:
        App_renderEditCountdown();
        break;
    default:
        break;
    }
}

static void App_enterMode(AppMode mode)
{
    g_mode = mode;
    g_editField = 0U;
    g_numericDigits = 0U;
    g_inputError = 0U;
    g_cursorVisible = 1U;
    g_lastCursorBlinkMs = Timer0_getMillis();
    App_markScreenDirty();
}

static void App_cycleMode(void)
{
    if (g_mode == APP_MODE_CLOCK)
    {
        App_enterMode(APP_MODE_ALARM);
    }
    else if (g_mode == APP_MODE_ALARM)
    {
        App_enterMode(APP_MODE_STOPWATCH);
    }
    else if (g_mode == APP_MODE_STOPWATCH)
    {
        App_enterMode(APP_MODE_COUNTDOWN);
    }
    else if (g_mode == APP_MODE_COUNTDOWN)
    {
        App_enterMode(APP_MODE_CLOCK);
    }
}

static void App_exitEditMode(void)
{
    g_mode = g_returnMode;
    g_editField = 0U;
    g_cursorVisible = 1U;
    App_markScreenDirty();
}

static uint8 App_saveEditTime(void)
{
    g_setTime.dayOfWeek = App_dayOfWeek(g_setTime.year, g_setTime.month, g_setTime.date);

    if (Ds3231_setDateTime(&g_setTime) == 0U)
    {
        g_rtcError = 1U;
        return 0U;
    }

    g_currentTime = g_setTime;
    g_rtcError = 0U;
    return 1U;
}

static uint8 App_saveEditDate(void)
{
    g_setDate.dayOfWeek = App_dayOfWeek(g_setDate.year, g_setDate.month, g_setDate.date);

    if (Ds3231_setDateTime(&g_setDate) == 0U)
    {
        g_rtcError = 1U;
        return 0U;
    }

    g_currentTime = g_setDate;
    g_rtcError = 0U;
    return 1U;
}

static void App_saveAlarm(void)
{
    g_alarm = g_alarmEdit;
    if (g_buzzerSource == BUZZER_SOURCE_ALARM)
    {
        App_stopBuzzer();
    }
    g_alarmEnabled = 1U;
    g_alarmTriggeredToday = 0U;
}

static void App_saveCountdownPreset(void)
{
    g_countdownPreset = g_countdownEdit;
    g_countdownRunning = 0U;
    if (g_buzzerSource == BUZZER_SOURCE_COUNTDOWN)
    {
        App_stopBuzzer();
    }
}

static void App_startAlarmIfNeeded(void)
{
    if ((g_alarmEnabled == 0U) || (g_buzzerSource != BUZZER_SOURCE_NONE))
    {
        return;
    }

    if ((g_currentTime.hour == g_alarm.hour) && (g_currentTime.minute == g_alarm.minute))
    {
        if (g_alarmTriggeredToday == 0U)
        {
            g_buzzerSource = BUZZER_SOURCE_ALARM;
            g_buzzerStopAtMs = Timer0_getMillis() + 3000UL;
            Buzzer_start();
            g_alarmTriggeredToday = 1U;
        }
    }
    else if ((g_currentTime.hour != g_alarm.hour) || (g_currentTime.minute != g_alarm.minute))
    {
        g_alarmTriggeredToday = 0U;
    }
}

static void App_stopBuzzer(void)
{
    g_buzzerSource = BUZZER_SOURCE_NONE;
    g_buzzerStopAtMs = 0U;
    Buzzer_stop();
}

static uint8 App_updateStopwatch(uint32 currentMs)
{
    uint32 elapsed;

    if (g_stopwatchRunning == 0U)
    {
        return 0U;
    }

    elapsed = currentMs - g_stopwatchBaseMs;

    if (elapsed >= 1000UL)
    {
        uint32 totalSeconds = App_stopwatchToSeconds(&g_stopwatch) + (elapsed / 1000UL);
        if (totalSeconds > (APP_STOPWATCH_MAX_HOURS * 3600UL + 59UL * 60UL + 59UL))
        {
            totalSeconds = 0UL;
        }

        App_secondsToStopwatch(totalSeconds, &g_stopwatch);
        g_stopwatchBaseMs = currentMs - (elapsed % 1000UL);
        return 1U;
    }

    return 0U;
}

static uint8 App_updateCountdown(uint32 currentMs)
{
    uint32 elapsed;
    uint32 totalSeconds;

    if (g_countdownRunning == 0U)
    {
        return 0U;
    }

    elapsed = currentMs - g_countdownBaseMs;
    if (elapsed < 1000UL)
    {
        return 0U;
    }

    totalSeconds = App_timeToSeconds(&g_countdownPreset);
    totalSeconds = (elapsed / 1000UL >= totalSeconds) ? 0UL : (totalSeconds - (elapsed / 1000UL));
    App_secondsToCountdown(totalSeconds, &g_countdownPreset);
    g_countdownBaseMs = currentMs - (elapsed % 1000UL);

    if (totalSeconds == 0UL)
    {
        g_countdownRunning = 0U;
        if (g_buzzerSource == BUZZER_SOURCE_NONE)
        {
            g_buzzerSource = BUZZER_SOURCE_COUNTDOWN;
            g_buzzerStopAtMs = currentMs + 2000UL;
            Buzzer_start();
        }
    }

    return 1U;
}

static void App_handleMainKeys(char key)
{
    if (App_handleEditNavigation(key) != 0U)
    {
        return;
    }

    if (App_handleEditStep(key) != 0U)
    {
        return;
    }

    if (App_handleNumericEdit(key) != 0U)
    {
        return;
    }

    switch (g_mode)
    {
    case APP_MODE_CLOCK:
        if (key == 'A')
        {
            App_cycleMode();
        }
        else if (key == 'B')
        {
            g_alarmEnabled ^= 1U;
            if (g_alarmEnabled == 0U)
            {
                g_alarmTriggeredToday = 0U;
                if (g_buzzerSource == BUZZER_SOURCE_ALARM)
                {
                    App_stopBuzzer();
                }
            }
            App_markScreenDirty();
        }
        else if (key == 'D')
        {
            g_returnMode = APP_MODE_CLOCK;
            g_setTime = g_currentTime;
            g_rtcError = 0U;
            App_enterMode(APP_MODE_SET_TIME);
        }
        else if (key == '#')
        {
            g_returnMode = APP_MODE_CLOCK;
            g_setDate = g_currentTime;
            g_rtcError = 0U;
            App_enterMode(APP_MODE_SET_DATE);
        }
        break;

    case APP_MODE_ALARM:
        if (key == 'A')
        {
            App_cycleMode();
        }
        else if (key == 'B')
        {
            g_alarmEnabled ^= 1U;
            if (g_alarmEnabled == 0U)
            {
                g_alarmTriggeredToday = 0U;
                if (g_buzzerSource == BUZZER_SOURCE_ALARM)
                {
                    App_stopBuzzer();
                }
            }
            App_markScreenDirty();
        }
        else if (key == 'D')
        {
            g_returnMode = APP_MODE_ALARM;
            g_alarmEdit = g_alarm;
            App_enterMode(APP_MODE_EDIT_ALARM);
        }
        break;

    case APP_MODE_STOPWATCH:
        if (key == 'A')
        {
            App_cycleMode();
        }
        else if (key == 'B')
        {
            if (g_stopwatchRunning == 0U)
            {
                g_stopwatchRunning = 1U;
                g_stopwatchBaseMs = Timer0_getMillis();
            }
            else
            {
                g_stopwatchRunning = 0U;
            }

            App_markScreenDirty();
        }
        else if (key == 'D')
        {
            g_stopwatchRunning = 0U;
            g_stopwatch.hour = 0U;
            g_stopwatch.minute = 0U;
            g_stopwatch.second = 0U;
            App_markScreenDirty();
        }
        break;

    case APP_MODE_COUNTDOWN:
        if (key == 'A')
        {
            App_cycleMode();
        }
        else if (key == 'B')
        {
            if (g_countdownRunning == 0U)
            {
                if (App_timeToSeconds(&g_countdownPreset) > 0UL)
                {
                    g_countdownRunning = 1U;
                    g_countdownBaseMs = Timer0_getMillis();
                    if (g_buzzerSource == BUZZER_SOURCE_COUNTDOWN)
                    {
                        App_stopBuzzer();
                    }
                }
            }
            else
            {
                g_countdownRunning = 0U;
            }

            App_markScreenDirty();
        }
        else if (key == 'D')
        {
            g_countdownRunning = 0U;
            g_countdownPreset.hour = 0U;
            g_countdownPreset.minute = 1U;
            g_countdownPreset.second = 0U;
            if (g_buzzerSource == BUZZER_SOURCE_COUNTDOWN)
            {
                App_stopBuzzer();
            }
            App_markScreenDirty();
        }
        else if (key == '#')
        {
            g_returnMode = APP_MODE_COUNTDOWN;
            g_countdownEdit = g_countdownPreset;
            App_enterMode(APP_MODE_EDIT_COUNTDOWN);
        }
        break;

    case APP_MODE_SET_TIME:
        if (key == 'A')
        {
            App_exitEditMode();
        }
        else if (key == 'D')
        {
            if (App_isEditedValueValid() == 0U)
            {
                App_setInputError();
            }
            else if (App_saveEditTime() != 0U)
            {
                App_exitEditMode();
            }
            else
            {
                App_markScreenDirty();
            }
        }
        break;

    case APP_MODE_SET_DATE:
        if (key == 'A')
        {
            App_exitEditMode();
        }
        else if (key == 'D')
        {
            if (App_isEditedValueValid() == 0U)
            {
                App_setInputError();
            }
            else if (App_saveEditDate() != 0U)
            {
                App_exitEditMode();
            }
            else
            {
                App_markScreenDirty();
            }
        }
        break;

    case APP_MODE_EDIT_ALARM:
        if (key == 'A')
        {
            App_exitEditMode();
        }
        else if (key == 'D')
        {
            if (App_isEditedValueValid() == 0U)
            {
                App_setInputError();
            }
            else
            {
                App_saveAlarm();
                App_exitEditMode();
            }
        }
        break;

    case APP_MODE_EDIT_COUNTDOWN:
        if (key == 'A')
        {
            App_exitEditMode();
        }
        else if (key == 'D')
        {
            if (App_isEditedValueValid() == 0U)
            {
                App_setInputError();
            }
            else
            {
                App_saveCountdownPreset();
                App_exitEditMode();
            }
        }
        break;

    default:
        break;
    }

    if (key == 'D' && g_buzzerSource != BUZZER_SOURCE_NONE)
    {
        App_stopBuzzer();
    }
}

static void App_initDefaultDate(void)
{
    static const char monthName[] = __DATE__;
    static const char timeName[] = __TIME__;
    uint8 dayValue;
    uint16 year;

    if (monthName[0] == 'J' && monthName[1] == 'a' && monthName[2] == 'n')
        g_currentTime.month = 1U;
    else if (monthName[0] == 'F')
        g_currentTime.month = 2U;
    else if (monthName[0] == 'M' && monthName[2] == 'r')
        g_currentTime.month = (monthName[1] == 'a') ? 3U : 5U;
    else if (monthName[0] == 'A' && monthName[1] == 'p')
        g_currentTime.month = 4U;
    else if (monthName[0] == 'J' && monthName[2] == 'n')
        g_currentTime.month = 6U;
    else if (monthName[0] == 'J' && monthName[2] == 'l')
        g_currentTime.month = 7U;
    else if (monthName[0] == 'A' && monthName[1] == 'u')
        g_currentTime.month = 8U;
    else if (monthName[0] == 'S')
        g_currentTime.month = 9U;
    else if (monthName[0] == 'O')
        g_currentTime.month = 10U;
    else if (monthName[0] == 'N')
        g_currentTime.month = 11U;
    else
        g_currentTime.month = 12U;

    if (monthName[4] == ' ')
    {
        dayValue = (uint8)(monthName[5] - '0');
    }
    else
    {
        dayValue = (uint8)(((monthName[4] - '0') * 10U) + (monthName[5] - '0'));
    }

    g_currentTime.date = dayValue;
    year = (uint16)(((monthName[7] - '0') * 1000U) + ((monthName[8] - '0') * 100U) + ((monthName[9] - '0') * 10U) + (monthName[10] - '0'));

    g_currentTime.year = year;
    g_currentTime.hour = (uint8)(((timeName[0] - '0') * 10U) + (timeName[1] - '0'));
    g_currentTime.minute = (uint8)(((timeName[3] - '0') * 10U) + (timeName[4] - '0'));
    g_currentTime.second = (uint8)(((timeName[6] - '0') * 10U) + (timeName[7] - '0'));
    g_currentTime.dayOfWeek = App_dayOfWeek(g_currentTime.year, g_currentTime.month, g_currentTime.date);
}

void RTC_APP_init(void)
{
    uint8 oscillatorStopped = 0U;
    Ds3231DateTime rtcTime;

    LCD_init();
    Keypad_init();
    Buzzer_init();
    Ds3231_init();
    Timer0_init1ms();

    App_initDefaultDate();

    if ((Ds3231_isOscillatorStopped(&oscillatorStopped) != 0U) && (oscillatorStopped != 0U))
    {
        (void)Ds3231_setDateTime(&g_currentTime);
        (void)Ds3231_clearOscillatorStop();
    }

    if ((Ds3231_getDateTime(&rtcTime) != 0U) && (App_isValidDateTime(&rtcTime) != 0U))
    {
        g_currentTime = rtcTime;
    }
    g_setTime = g_currentTime;
    g_setDate = g_currentTime;
    g_alarm.hour = (uint8)((g_currentTime.hour + 1U) % 24U);
    g_alarm.minute = g_currentTime.minute;
    g_lastDisplayMs = 0U;
    g_lastRtcReadMs = 0U;
    g_lastCursorBlinkMs = 0U;
    g_buzzerSource = BUZZER_SOURCE_NONE;
    g_buzzerStopAtMs = 0U;
    g_alarmEnabled = 0U;
    g_alarmTriggeredToday = 0U;
    Buzzer_stop();
    App_markScreenDirty();

    App_showCurrentScreen();
}

void RTC_APP_task(void)
{
    uint32 now = Timer0_getMillis();
    char key = Keypad_getKey();

    if ((g_inputError != 0U) && ((uint32)(now - g_inputErrorStartedAtMs) >= 2000UL))
    {
        g_inputError = 0U;
        App_markScreenDirty();
    }

    if (App_updateCountdown(now) != 0U)
    {
        App_markScreenDirty();
    }

    if ((App_isEditMode(g_mode) != 0U) && ((now - g_lastCursorBlinkMs) >= 500UL))
    {
        g_lastCursorBlinkMs = now;
        g_cursorVisible ^= 1U;
        App_markScreenDirty();
    }

    if ((now - g_lastRtcReadMs) >= 1000UL)
    {
        Ds3231DateTime rtcTime;

        g_lastRtcReadMs = now;
        if ((Ds3231_getDateTime(&rtcTime) != 0U) && (App_isValidDateTime(&rtcTime) != 0U))
        {
            g_currentTime = rtcTime;
            App_startAlarmIfNeeded();
            App_markScreenDirty();
        }
    }

    if (App_updateStopwatch(now) != 0U)
    {
        App_markScreenDirty();
    }

    if ((g_buzzerSource != BUZZER_SOURCE_NONE) && (g_buzzerStopAtMs != 0U) && (now >= g_buzzerStopAtMs))
    {
        App_stopBuzzer();
        App_markScreenDirty();
    }

    if (key != 0)
    {
        App_handleMainKeys(key);
    }

    if ((g_lastShownMode != (uint8)g_mode) || (g_screenDirty != 0U))
    {
        g_lastShownMode = (uint8)g_mode;
        App_showCurrentScreen();
    }
}
