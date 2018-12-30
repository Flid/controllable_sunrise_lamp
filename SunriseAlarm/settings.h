#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#define SETTINGS_VERSION "ls1"
#define SETTINGS_START_ADDR 32

struct Settings
{
    char version[4];
    uint8_t alarm_hour;
    uint8_t alarm_minute;
    bool alarm_enabled;
    uint16_t alarm_turn_on_duration;
    uint16_t alarm_turn_off_duration;
    uint16_t alarm_stay_on_duration;
    uint8_t player_max_volume;
};

void save_settings(Settings &s);
void load_settings(Settings &s);

#endif