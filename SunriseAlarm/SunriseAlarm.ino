// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2018
// MIT License

#include <HardwareSerial.h>
#include <EEPROM.h>
#include <algorithm>

#include "Alarm.h"
#include "Player.h"
#include "ControlServer.h"
#include "settings.h"
#include "config.h"

#define WITH_SCREEN 1

#define DIMMER_RX_PIN 16
#define DIMMER_TX_PIN 17
#define PLAYER_RX_PIN 23
#define PLAYER_TX_PIN 19

Settings settings = {
    /*version*/ SETTINGS_VERSION,
    /*alarm_hour*/ 7,
    /*alarm_minute*/ 0,
    /*alarm_enabled*/ true,
    /*alarm_turn_on_duration*/ 30,
    /*alarm_turn_off_duration*/ 10,
    /*alarm_stay_on_duration*/ 30,
    /*max_volume*/ 20,
};

#ifdef WITH_SCREEN
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /*reset=*/16, /*clock=*/15, /*data=*/4);
#define FONT_6 u8g2_font_5x7_mr
#define FONT_8 u8g2_font_profont12_mr
#define FONT_8_BOLD u8g2_font_t0_12b_mf
#define FONT_12 u8g2_font_t0_15_mn
#define FONT_12_BOLD u8g2_font_t0_15b_mn
#define FONT_16 u8g2_font_inr16_mr
#define FONT_16_BOLD u8g2_font_inb16_mr
#endif

HardwareSerial DimmerSerial(2);

Alarm alarm_manager(settings);
Player player(/*busy_pin*/ 0, /*hw_serial_number*/ 1, settings);
ControlServer control_server(/*port*/ 81);

uint8_t globalBrightness = 0;
uint8_t alarmProgress = 0;

void render_screen()
{
  display.clearBuffer();
  uint8_t line_height = display.getMaxCharHeight();

  display.setFont(FONT_6);
  display.setCursor(0, line_height);
  display.print(WiFi.localIP());

  struct tm timeinfo;
  get_current_time(&timeinfo);
  char buffer[128];
  strftime(buffer, sizeof(buffer), "%X", &timeinfo);
  display.setCursor(128 - display.getStrWidth(buffer), line_height);
  display.print(buffer);

  if (settings.alarm_enabled)
  {
    sprintf(buffer, "Alarm: %.02d:%.02d (%d)", settings.alarm_hour, settings.alarm_minute, alarmProgress);
  }
  else
  {
    sprintf(buffer, "Alarm: OFF");
  }
  display.drawStr(0, 64 - display.getMaxCharHeight() - 2, buffer);

  sprintf(buffer, "%d %d %d", settings.alarm_turn_on_duration, settings.alarm_stay_on_duration, settings.alarm_turn_off_duration);
  display.drawStr(0, 64, buffer);

  sprintf(buffer, "Br: %d", std::max(globalBrightness, alarmProgress));
  display.drawStr(128 - display.getStrWidth(buffer), 64 - display.getMaxCharHeight() - 2, buffer);

  sprintf(buffer, "Vol: %d", map(player.get_volume(), 0, 30, 0, 255));
  display.drawStr(128 - display.getStrWidth(buffer), 64, buffer);

  display.sendBuffer();
}

void process_control_event(JsonObject &root)
{
  String command = root.get<String>("command");
  JsonObject &data = root.get<JsonObject>("data");

  if (command == "set_brightness")
  {
    globalBrightness = data.get<uint8_t>("value");
  }
  else if (command == "update_settings")
  {
    JsonObject &settings_obj = root.get<JsonObject>("data");
    if (settings_obj.containsKey("alarm_hour"))
      settings.alarm_hour = settings_obj.get<uint8_t>("alarm_hour");

    if (settings_obj.containsKey("alarm_minute"))
      settings.alarm_minute = settings_obj.get<uint8_t>("alarm_minute");

    if (settings_obj.containsKey("alarm_enabled"))
      settings.alarm_enabled = settings_obj.get<bool>("alarm_enabled");

    if (settings_obj.containsKey("alarm_turn_on_duration"))
      settings.alarm_turn_on_duration = settings_obj.get<uint16_t>("alarm_turn_on_duration");

    if (settings_obj.containsKey("alarm_turn_off_duration"))
      settings.alarm_turn_off_duration = settings_obj.get<uint16_t>("alarm_turn_off_duration");

    if (settings_obj.containsKey("alarm_stay_on_duration"))
      settings.alarm_stay_on_duration = settings_obj.get<uint16_t>("alarm_stay_on_duration");

    if (settings_obj.containsKey("player_max_volume"))
      settings.player_max_volume = settings_obj.get<uint8_t>("player_max_volume");

    save_settings(settings);
    Serial.println("Settings updated");
  }
  else
  {
    Serial.println("Unhandled command");
  }
}

void on_control_event(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  StaticJsonBuffer<1024> json_buffer;

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    Serial.println("New connection");
  }
  break;
  case WStype_TEXT:
    Serial.printf("[%u] Command received %s\n", num, payload);
    process_control_event(json_buffer.parseObject(payload));
    break;
  default:
    break;
  }
}

void setup()
{
  Serial.begin(115200);

  DimmerSerial.begin(19200, SERIAL_8N1, DIMMER_RX_PIN, DIMMER_TX_PIN);

  alarm_manager.begin(TIMEZONE);
  player.begin(PLAYER_RX_PIN, PLAYER_TX_PIN);
  control_server.begin(WIFI_SSID, WIFI_PASSWORD, on_control_event);

  if (!EEPROM.begin(256))
  {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  load_settings(settings);

#ifdef WITH_SCREEN
  display.begin();
#endif
}

void seconds_ticker()
{
  static bool player_running = false;
  alarm_manager.seconds_tick(alarmProgress);
  player.seconds_tick();

  DimmerSerial.write(std::max(alarmProgress, globalBrightness));

  if (player_running && alarmProgress == 0)
  {
    player.stop();
    player_running = false;
  }

  if (alarmProgress > 0)
  {
    if (!player_running)
    {
      player_running = true;
      player.play();
    }
    player.set_volume(alarmProgress);
  }

#ifdef WITH_SCREEN
  render_screen();
#endif
}

void loop()
{
  static uint32_t last_triggered_time = 0;

  if (millis() - last_triggered_time > 1000)
  {
    last_triggered_time = millis();
    seconds_ticker();
  }

  control_server.loop();
}
