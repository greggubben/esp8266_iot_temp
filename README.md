# ESP8266 IoT Temp
This will turn an ESP8266 into an IoT device reporting Temperature and Humidity using a DHT11 sensor.

The device will publish at a specified frequency the temperature and humidity received from the DHT11 sensor.

###Dependencies
  - For reading DHT values the Adafruit library at https://github.com/adafruit/DHT-sensor-library was used.
  - ArduinoJSON was used to work with JSON strings and can be found at https://github.com/bblanchon/ArduinoJson.
  - For comminicating using MQTT the PubSubClient from https://github.com/knolleary/pubsubclient is used.

### License
This code is released under the MIT License.
