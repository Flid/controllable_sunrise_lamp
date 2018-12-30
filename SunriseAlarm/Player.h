#ifndef __PLAYER_H
#define __PLAYER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "settings.h"

class Player
{
  public:
    Player(uint8_t busy_pin, uint8_t hw_serial_number, Settings settings);
    ~Player();

    void seconds_tick();
    void play();
    void stop();
    void set_volume(uint8_t value);
    uint8_t get_volume();
    void begin(uint8_t serial_rx_pin, uint8_t serial_tx_pin);

  private:
    uint8_t busy_pin;
    HardwareSerial *serial_controls;
    uint8_t volume;
    Settings settings;
    bool playing = false;
};

#endif // __PLAYER_H
