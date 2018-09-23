# pir-relay-mqtt
## Another Motion Triggered Relay with MQTT
This Arduino sketch uses a PIR sensor to detect motion and trigger a relay. It also keeps an MQTT server in the loop on state changes, 
as well as allowing the relay to be turned on via an MQTT message. 
I decided to rewrite my old PIR lights project in Arduino to support having a local OTA update server so I wouldn't need to 
rely / pay for cloud OTA services.

## Getting Started
1. Download and extract the repo.
2. Create a `config.h` file in the folder you just extracted. This is an example based on my config...
```
const char* WIFI_SSID = "Your WiFi SSID";
const char* WIFI_PASSWORD = "Your WiFi Password";
const char* HOSTNAME = "kitchen_motion_light";
const char* MQTT_SERVER = "your.mqtt.service.ip";
const long AUTO_OFF_MS = 10 * 60 * 1000; // 10 minutes
const char* PIR_STATE_TOPIC = "home/kitchen/motion-sensor";
const char* RELAY_STATE_TOPIC = "home/kitchen/counter-led-strip";
```
3. Open the project in the Arduino IDE.
4. Flash to your ESP8266
5. ???
6. Profit!
