#ifndef __ALARM_H
#define __ALARM_H

#include <Arduino.h>
#include <time.h>
#include "settings.h"

enum AlarmState
{
  WAITING,
  TURNING_ON,
  CONTINUOUS_ON,
  TURNING_OFF,
  WAITING_FOR_MINUTE_SWITCH,
};
void get_current_time(struct tm *timeinfo);

class Alarm
{
public:
  Alarm(Settings &settings);
  void seconds_tick(uint8_t &progress);
  void begin(char *timezone_name);
  void stop();

private:
  Settings &settings;
  AlarmState state;
};

#endif // __ALARM_H