#include "ControlServer.h"

ControlServer::ControlServer(uint32_t port)
{
  server = new WebSocketsServer(port);
}

ControlServer::~ControlServer()
{
  delete server;
}

void ControlServer::loop()
{
  server->loop();
}

void ControlServer::begin(
    char *ssid, char *password,
    std::function<void(uint8_t num, WStype_t type, uint8_t *payload, size_t length)> callback)
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server->begin();
  server->onEvent(callback);
}