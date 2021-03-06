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

const int pir_events_before_active = 5;
const long pir_verification_window = 5000;
int pir_triggers = 0;

unsigned long last_motion_detected = 0;
unsigned long last_motion_verified = 0;
unsigned long last_stillness_reported = 0;
unsigned long last_reconnect = 0;

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
  updateMotion(false);
}

void loop() {
  unsigned long currentMS = millis();

  if (WiFi.status() != WL_CONNECTED && (currentMS > last_reconnect + 10000)){
    last_reconnect = currentMS;
    setupWiFi();
  }
  if (!mqttClient.connected() && (currentMS > last_reconnect + 10000)){
    last_reconnect = currentMS;
    connectMQTT();
  }

  ArduinoOTA.handle();
  mqttClient.loop();

  int pir_val = digitalRead(PIR_PIN);

  // If we've run out of time to verify motion, reset the counter
  if (currentMS > last_motion_detected + pir_verification_window){
    pir_triggers = 0;
    if (currentMS > last_stillness_reported + pir_verification_window){
      // Debounce inactivity reporting
      last_stillness_reported = currentMS;
      updateMotion(false);
    }
  }

  // Since we want sustained motion over 5 seconds, make sure that this event happened 1+ sec since the last one
  if (pir_val == HIGH && currentMS > last_motion_detected + 1000){
    Serial.print("Seen ");
    Serial.print((pir_triggers + 1));
    Serial.println();
    last_motion_detected = currentMS;
    if (++pir_triggers == pir_events_before_active){
      pir_triggers = 0;
      last_motion_verified = currentMS;
      updateRelay(true);
      updateMotion(true);
    }
  }

  // Check to see if we haven't had a significant motion event lately
  if (currentMS > last_motion_verified + AUTO_OFF_MS)
    updateRelay(false);
}

void updateRelay(bool bOn){
  Serial.print("Turning relay ");
  Serial.print((bOn ? "ON" : "OFF"));
  Serial.println();
  digitalWrite(RELAY_PIN, (bOn ? HIGH : LOW));
  mqttClient.publish(RELAY_STATE_TOPIC, (bOn ? "ON" : "OFF"));
}

void updateMotion(bool bOn){
  Serial.print("Updating mqtt motion ");
  Serial.print((bOn ? "ON" : "OFF"));
  Serial.println();
  mqttClient.publish(PIR_STATE_TOPIC, (bOn ? "ON" : "OFF"));
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
    // Use this timestamp as a 'verified' event, should allow for normal sleep functions.
    if (payloadString == "ON")
      last_motion_verified = millis();
  }
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.waitForConnectResult();
  // Disabled this reboot loop in case our WiFi is down.
  /*
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  */
}

void connectMQTT() {
  // Not sure if this will crash without a working WiFi connection
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
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

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
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
