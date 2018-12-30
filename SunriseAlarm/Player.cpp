#include "Player.h"

#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

DFRobotDFPlayerMini myDFPlayer;

Player::Player(uint8_t busy_pin, uint8_t hw_serial_number, Settings settings) : busy_pin(busy_pin), settings(settings)
{
    serial_controls = new HardwareSerial(hw_serial_number);
}

Player::~Player()
{
    delete serial_controls;
}

void Player::begin(uint8_t serial_rx_pin, uint8_t serial_tx_pin)
{
    pinMode(busy_pin, INPUT);
    serial_controls->begin(9600, SERIAL_8N1, serial_rx_pin, serial_tx_pin);
    delay(100);

    if (!myDFPlayer.begin(*serial_controls))
    {
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
        while (true)
            ;
    }
    myDFPlayer.volume(0);
    Serial.println(F("DFPlayer Mini online."));
}

void Player::play()
{
    if (playing)
        return;
    playing = true;
    myDFPlayer.play(1);
}

void Player::stop()
{
    if (!playing)
        return;
    playing = false;
    myDFPlayer.stop();
}

void Player::set_volume(uint8_t value)
{
    uint8_t new_volume = map(value, 0, 255, 0, settings.player_max_volume);
    if (volume == new_volume)
        return;

    volume = new_volume;
    myDFPlayer.volume(volume);
}

uint8_t Player::get_volume()
{
    return volume;
}

void Player::seconds_tick()
{
    if (playing && digitalRead(busy_pin))
    {
        playing = false;
        play();
    }
}
