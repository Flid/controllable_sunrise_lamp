#ifndef __CONTROL_SERVER_H
#define __CONTROL_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#include "Settings.h"
#include "Alarm.h"
#include "Player.h"

struct ServerCommandData
{
    int16_t new_brightness = -1;
};

class ControlServer
{
  public:
    ControlServer(uint32_t port);
    ~ControlServer();
    void process_requests(ServerCommandData &command_data);
    void begin(char *ssid, char *password, std::function<void(uint8_t num, WStype_t type, uint8_t *payload, size_t length)> callback);
    void loop();

  private:
    void update_settings(JsonObject &root);
    void get_settings(JsonObject &root);
    WebSocketsServer *server;
};

#endif // __CONTROL_SERVER_H
