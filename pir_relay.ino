#include <ESP8266mDNS.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

// Load the following vars from a config file:
// WIFI_SSID, WIFI_PASSWORD, HOSTNAME, MQTT_SERVER, AUTO_OFF_MS,
// PIR_STATE_TOPIC, RELAY_STATE_TOPIC
#include "config.h"

#define RELAY_PIN 4
#define PIR_PIN 5

int pir_events_before_change = 5;
int pir_event_count = 0;
int pir_check_interval = 0;
int last_pir_val = 0;
long sleep_timer = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupWiFi();
  setupOTA();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  connectMQTT();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  // Assume power up event should trigger power on
  updateRelay(true);
}

void loop() {
  if (!mqttClient.connected())
    connectMQTT();
  mqttClient.loop();

  int pir_val = digitalRead(PIR_PIN);
  pir_event_count = pir_val == HIGH ? pir_val+1 : 0;
  if (pir_event_count >= pir_events_before_change)
    updateRelay(true);
  if (AUTO_OFF_MS > 0){
    sleep_timer -= pir_check_interval;
    if (sleep_timer <= 0)
      updateRelay(false);
  }
  if (pir_val != last_pir_val){
    last_pir_val = pir_val;
    mqttClient.publish(PIR_STATE_TOPIC, (pir_val == HIGH ? "ON" : "OFF"));
  }

  delay(pir_check_interval);
}

void updateRelay(bool bOn){
  Serial.print("Turning relay ");
  Serial.print((bOn ? "ON" : "OFF"));
  Serial.println();
  digitalWrite(RELAY_PIN, (bOn ? HIGH : LOW));
  if (bOn)
    sleep_timer = AUTO_OFF_MS;
  if (!mqttClient.connected()){
    Serial.println("Reconnecting in updateRelay");
    connectMQTT();
  }
  mqttClient.publish(RELAY_STATE_TOPIC, (bOn ? "ON" : "OFF"));
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadString = "";
  for (int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  Serial.print(payloadString);
  Serial.println();

  if (String(topic) == String(RELAY_SET_TOPIC)){
    updateRelay((payloadString == "ON"));
  }
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}

void connectMQTT() {
  Serial.println("Attempting connection to MQTT server...");
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttCallback);
  if (mqttClient.connect(HOSTNAME, MQTT_USERNAME, MQTT_PASSWORD)){
    Serial.println("Connected to MQTT server!");
    mqttClient.subscribe(RELAY_SET_TOPIC);
  }
  else {
    Serial.println("Couldn't connect to MQTT server, error code: ");
    Serial.println(mqttClient.state());
    if (mqttClient.state() < 0){
      delay(5000);
      connectMQTT();
    }
  }
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
