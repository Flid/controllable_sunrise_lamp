#include <Arduino.h>
#include <EEPROM.h>
#include "settings.h"

void save_settings(Settings &s)
{
    Serial.println(s.version);
    for (unsigned int t = 0; t < sizeof(s); t++)
    {
        EEPROM.write(SETTINGS_START_ADDR + t, *((char *)&s + t));
    }
    EEPROM.commit();
    Serial.println("Successfully saved settings");
}

void load_settings(Settings &s)
{
    if (EEPROM.read(SETTINGS_START_ADDR + 0) == SETTINGS_VERSION[0] &&
        EEPROM.read(SETTINGS_START_ADDR + 1) == SETTINGS_VERSION[1] &&
        EEPROM.read(SETTINGS_START_ADDR + 2) == SETTINGS_VERSION[2])
    {

        for (unsigned int t = 0; t < sizeof(s); t++)
        {
            *((char *)&s + t) = EEPROM.read(SETTINGS_START_ADDR + t);
        }
        Serial.println("Successfully loaded settings");
    }
    else
    {
        Serial.println("Failed to load settings");
    }
    s.alarm_hour = 7;
    s.alarm_minute = 0;
}