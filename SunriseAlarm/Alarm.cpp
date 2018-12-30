#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "nvs_flash.h"

#include "apps/sntp/sntp.h"
#include "Alarm.h"

Alarm::Alarm(Settings &settings) : settings(settings) {}

void Alarm::begin(char *timezone_name)
{
    setenv("TZ", timezone_name, 1);
    tzset();
}

void Alarm::seconds_tick(uint8_t &progress)
{
    /* Here we decide if alarm has to be triggered now. 
       Also if alarm is running now, we control the "progress" - 0-255 number. */
    static uint32_t action_started_at = 0;

    struct tm timeinfo;
    get_current_time(&timeinfo);

    int32_t current_value = 0;

    switch (state)
    {
    case WAITING:
        if (timeinfo.tm_hour == settings.alarm_hour && timeinfo.tm_min == settings.alarm_minute && settings.alarm_enabled)
        {
            state = TURNING_ON;
            action_started_at = millis();
        }
        break;
    case TURNING_ON:
        current_value = (millis() - action_started_at) * 256 / (settings.alarm_turn_on_duration * 1000);
        if (current_value >= 255)
        {
            current_value = 255;
            state = CONTINUOUS_ON;
            action_started_at = millis();
        }
        break;
    case CONTINUOUS_ON:
        if (millis() - action_started_at > settings.alarm_stay_on_duration * 1000)
        {
            state = TURNING_OFF;
            action_started_at = millis();
        }
        current_value = 255;
        break;
    case TURNING_OFF:
        current_value = 255 - (millis() - action_started_at) * 256 / (settings.alarm_turn_off_duration * 1000);
        if (current_value <= 0)
        {
            current_value = 0;
            state = WAITING_FOR_MINUTE_SWITCH;
            action_started_at = millis();
        }
        break;
    case WAITING_FOR_MINUTE_SWITCH:
        if (timeinfo.tm_hour != settings.alarm_hour || timeinfo.tm_min != settings.alarm_minute)
        {
            state = WAITING;
        }
        break;
    }

    progress = current_value;
}

void Alarm::stop()
{
    state = WAITING_FOR_MINUTE_SWITCH;
}

static void initialize_sntp(void)
{
    Serial.println("Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void obtain_time(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
    {
        Serial.println("Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void get_current_time(struct tm *timeinfo)
{
    /* In order to track time, we obtain it from an NTP server,
  then we'll use local timer to track the change. It's precise enough for our case. */
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo->tm_year < (2018 - 1900))
    {
        Serial.println("Time is not set yet. Getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
        localtime_r(&now, timeinfo);
    }
}