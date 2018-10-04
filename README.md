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
const char* MQTT_USERNAME = "";
const char* MQTT_PASSWORD = "";
const long AUTO_OFF_MS = 10 * 60 * 1000; // 10 minutes
const char* PIR_STATE_TOPIC = "home/kitchen/motion-sensor";
const char* RELAY_STATE_TOPIC = "home/kitchen/counter-led-strip";
const char* RELAY_SET_TOPIC = "home/kitchen/counter-led-strip/set";
```
3. Open the project in the Arduino IDE.
4. Flash to your ESP8266
5. ???
6. Profit!


## Config fields explained
*WIFI_SSID* - Your WiFi network name

*WIFI_PASSWORD* - Your WiFi network password

*HOSTNAME* - The hostname you want this ESP to be called (optional)

*MQTT_SERVER* - The IP address or hostname of your MQTT broker (expected version is 3.1.1)

*MQTT_USERNAME* - Username for the MQTT Broker (optional)

*MQTT_PASSWORD* - Password for the MQTT Broker (optional)

*AUTO_OFF_MS* - Time in milliseconds before automatically turning off the relay. If set to 0, the relay won't turn off on it's own.

*PIR_STATE_TOPIC* - MQTT Topic that gets updated when PIR is active / inactive

*RELAY_STATE_TOPIC* - MQTT Topic that reflects the current state of the relay. We push our updates to this topic to reflect changes we make to the state of the relay.

*RELAY_SET_TOPIC* - MQTT Topic that is published to by clients wishing to change the state of the relay. We subscribe to changes to this topic and act upon them accordingly. 
