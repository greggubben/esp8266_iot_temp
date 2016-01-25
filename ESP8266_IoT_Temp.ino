/*******************************************************************************
 * A simple IoT temp and humidity data publisher using an ESP8266 and a DHT11. 
 *
 * Gregg Ubben
 * http://greggubben.com
 *******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"

extern "C" {
  #include "user_interface.h"
}

#define MAX_JSON_SIZE MQTT_MAX_PACKET_SIZE

/*
 * Wifi Settings
 */
//const char* ssid     = "see config.h";
//const char* password = "see config.h";
WiFiClient wifiClient;

/*
 * MQTT Settings
 */
IPAddress mqtt_server = IPAddress(192,168,48,66);
uint16_t mqtt_port = 1883;
const char* mqtt_commandHelloTopic = "command/hello";
const char* mqtt_commandWhoAmITopic = "command/whoami";
const char* mqtt_YouAreSubTopic = "/youare";
const char* mqtt_dataTopic = "data/tandh";
char mqtt_subscribedTopic[50];

//PubSubClient mqttClient(wifiClient);

/*
 * Web Server Settings
 */
//ESP8266WebServer webServer(80);

/*
 * ThingSpeak Settings
 */
//char thingspeak_key[20] = "see config.h";
const char* thingspeak_host = "api.thingspeak.com";
const char* getThingSpeakSettings_host = "192.168.48.66";

/*
 * DHT Settings
 */
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float h;    // Humidity
float f;    // Fahrenheit
float c;    // Celcius
float hif;  // Heat Index in Fahrenheit
float hic;  // Heat Index in Celcius
char sensorJSON[MAX_JSON_SIZE];

/*
 * System Settings
 */
ADC_MODE(ADC_VCC);
float v;   // Voltage
long timeBetweenUpdates = 300000; // in Millis
char lastUpdatedTimestamp[40] = "";
char clientName[50];
char sensorName[50];
char mac[20];
unsigned long lastMsg;

/**********************************************************************/

/*
 * Set up the ESP8266
 */
void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();

  wifi_set_sleep_type(NONE_SLEEP_T);
  //wifi_set_sleep_type(MODEM_SLEEP_T);
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  
  // Set up Wifi connection
  wifiConnect();

  // Set up System
  uint8_t mac_parts[6];
  WiFi.macAddress(mac_parts);
  strcpy(mac,macToStr(mac_parts).c_str());
  
  generateClientName(clientName);
  Serial.print("Client Name = ");
  Serial.println(clientName);
  // Until we hear back from Who Am I, assume the Sesnor Name is the same as the Client Name
  strcpy(sensorName,clientName);

  // Set up MQTT connection
  //strcpy(mqtt_subscribedTopic, mac);      // Subscribed Topic will be the same as the MAC address
  //mqttClient.setServer(mqtt_server, mqtt_port);
  //mqttClient.setCallback(mqttCallback);

  // Set up Thing Speak connection
  getThingSpeakKey();

  // Set up Web Server
  //webServer.on("/", webServer_root);
  //webServer.on("/json", webServer_json);
  //webServer.begin();

  lastMsg = 0;
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

/*
 * Continue to send Tempurature and Humidity data
 */
void loop()
{
/* 
  if (!mqttClient.connected()) {
    mqttConnect();

    Serial.println();
    char messageJSON[MAX_JSON_SIZE];

    // Once connected and subscribed, publish an announcement
    generateHello(messageJSON, sizeof(messageJSON));
    //Serial.print("Hello request = ");
    //Serial.println(messageJSON);
    mqttSend(mqtt_commandHelloTopic, messageJSON);
    
    delay(2000);  // Give the server time to process the hello message

    generateWhoAmI(messageJSON, sizeof(messageJSON));
    //Serial.print("Who Am I request = ");
    //Serial.println(messageJSON);
    mqttSend(mqtt_commandWhoAmITopic, messageJSON);

    delay(2000);  // Give the server time to process the who am i message
  }
*/
  //mqttClient.loop();
  //webServer.handleClient();

  unsigned long now = millis();
  if (now - lastMsg > timeBetweenUpdates) {
    wifi_set_sleep_type(NONE_SLEEP_T);
    lastMsg = now;
    Serial.println();
    
    // Time to update readings...
    uint16_t v_int = ESP.getVcc();
    v = v_int / 1000.0;
    getDhtValues();
    buildSensorJSON(sensorJSON, sizeof(sensorJSON));
    printValues();

    // Send the Sensor data to MQTT broker
    //mqttSend(mqtt_dataTopic, sensorJSON);

    // Send the Sensor data to Thing Speak
    sendThingSpeak();
    wifi_set_sleep_type(LIGHT_SLEEP_T);
  }

  delay(10000);    // Wait between loops
}


/**********************************************************************
 * MQTT Routines
 **********************************************************************/

/*
 * Process an inbound message.
 */
 /*
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  char message[MAX_JSON_SIZE];
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i] = payload[i];
  }
  message[length] = 0;
  Serial.println();
  Serial.print("Message = ");
  Serial.println(message);

  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;

  JsonObject& messageRoot = jsonBuffer.parseObject(message);

  if (!messageRoot.success()) {
    Serial.println("parseObject(message) failed");
    return;
  }

  const char* subtopic = strrchr(topic,'/');
  Serial.print("SubTopic = ");
  Serial.println(subtopic);

  if (strcmp(subtopic,mqtt_YouAreSubTopic) == 0) {
    Serial.println("updating...");
    String value;
    if (messageRoot.containsKey("sensor")) {
      value = messageRoot["sensor"].asString();
      strcpy(sensorName, value.c_str());
      Serial.print("New Sensor name = ");
      Serial.println(sensorName);
    }
    if (messageRoot.containsKey("i")) {
      value = messageRoot["i"].asString();
      timeBetweenUpdates = value.toInt();
      Serial.print("New Interval = ");
      Serial.println(timeBetweenUpdates);
    }
  }
}
*/
/*
 * Connect to the MQTT Broker and subscribe to our inbound topic.
 */
/*
void mqttConnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker at ");
    Serial.print(mqtt_server);
    Serial.print(" - ");
    // Attempt to connect
    if (mqttClient.connect(clientName)) {
      Serial.println("connected");
      // Subscribe to receive commands
      String subscribeTopic;
      subscribeTopic += String(mqtt_subscribedTopic) + "/#";
      mqttClient.subscribe(subscribeTopic.c_str());
      Serial.print("Subscribed to MQTT Topic [");
      Serial.print(subscribeTopic.c_str());
      Serial.println("]");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
*/
/*
 * Send a Message to the MQTT broker
 */
/*
void mqttSend(const char* topic, char* message) {
  Serial.print("Sending MQTT Message '");
  Serial.print(message);
  Serial.println("'");
  Serial.print("             Topic   [");
  Serial.print(topic);
  Serial.print("] - ");
  if (mqttClient.publish(topic, message)) {
    Serial.println("Sent");
  }
  else {
    Serial.print("Error: ");
    Serial.println(mqttClient.state());
  }
}
*/

/**********************************************************************
 * Thing Speak Routines
 **********************************************************************/

/*
 * Get Thing Speak API Key
 */
void getThingSpeakKey() {
  String getThingSpeakURL = "/iot_tandh/get_thingspeak_key.cgi?mac=" + String(mac) + "&key=" + String(thingspeak_key);
  String getThingSpeakResult = sendHttpGet(wifiClient, getThingSpeakURL, getThingSpeakSettings_host);
  char response[MAX_JSON_SIZE];
  strcpy(response,getThingSpeakResult.c_str());
  const char* message = strrchr(response,'{');
  char* endBracket = strrchr(response,'}');
  endBracket[1] = 0;
  
  Serial.print("Thing Speak Result = ");
  Serial.println(message);

  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
  JsonObject& messageRoot = jsonBuffer.parseObject(message);

  if (!messageRoot.success()) {
    Serial.println("parseObject(message) failed");
    return;
  }
  if (messageRoot.containsKey("key")) {
    String value;
    value = messageRoot["key"].asString();
    strcpy(thingspeak_key, value.c_str());
    Serial.print("New Thing Speak Key = ");
    Serial.println(thingspeak_key);
  }
}

/*
 * Send the Sensor data to Thing Speak
 */
void sendThingSpeak() {
  // We now create a URI for the request
  String thingspeakUrl = "/update?key="+ String(thingspeak_key) + "&field1="+String((int)f) + "&field2="+String((int)c) + "&field3="+String((int)h) + "&field4="+String(v);
  sendHttpGet(wifiClient, thingspeakUrl, thingspeak_host);
}


/**********************************************************************
 * System Routines
 **********************************************************************/

/*
 * Generate the Client name based on MAC address
 */
 void generateClientName(char* clientname) {
  String cn;

  cn += "esp8266-";
  cn += String(mac);
  strcpy(clientname, cn.c_str());
 }


 /*
  * Generate the Hello JSON message
  */
void generateHello(char* hello, int len) {
  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
  char localip[16];
  IPAddress ip = WiFi.localIP();
  sprintf(localip,"%d.%d.%d.%d", ip[0],ip[1],ip[2],ip[3]);

  JsonObject& helloRoot = jsonBuffer.createObject();
  helloRoot["mac"] = mac;
  helloRoot["ipaddr"] = localip;
  helloRoot["client"] = clientName;
  
  helloRoot.printTo(hello, len);
}

 /*
  * Generate the Who Am I JSON message
  */
void generateWhoAmI(char* whoami, int len) {
  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
  
  JsonObject& whoamiRoot = jsonBuffer.createObject();
  whoamiRoot["mac"] = mac;
  whoamiRoot["sensor"] = sensorName;
  whoamiRoot["i"] = timeBetweenUpdates;
  
  whoamiRoot.printTo(whoami, len);
}

/**********************************************************************
 * WiFi Routines
 **********************************************************************/

/*
 * Connect to the Wifi
 */
void wifiConnect() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

/*
 * Convert a Mac Address to a string.
 */
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 16) {
      result += "0";
    }
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/*
 * Send a message using HTTP GET.
 * This funciton has a side effect of updating the lastUpdatedTimestamp
 * from the Response Headers.
 */
String sendHttpGet(WiFiClient client, String url, const char* host)
{
  String response;
  
  Serial.print("Sending to URL: http://");
  Serial.print(host);
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println(" - connection failed");
    return response;
  }
  
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(3000);
  
  // Read all the lines of the reply from server and print them to Serial
  boolean responseData = false;
  while(client.available()){
    String line = client.readStringUntil('\r');
    //Serial.println(line);

    // Get the Date from the header
    if (line.indexOf("Date: ") != -1) {
      String dateTime = line.substring(7);
      strcpy(lastUpdatedTimestamp,dateTime.c_str());
    }

    if (responseData) {
      response += line;
    }
    // There is a blank line between the header and data portions of the response.
    if (line.length() == 1) {
      responseData = true;
    }
  }
  response.trim();

  return response;
}

/**********************************************************************
 * DHT Routines
 **********************************************************************/

/*
 * Read the values from the DHT Sensor
 */
void getDhtValues() {
  h = dht.readHumidity();
  // Read temperature as Celsius
  c = dht.readTemperature();
  // Read temperature as Fahrenheit
  f = dht.readTemperature(true);

  Serial.print("Reading DHT sensor");
  while(isnan(h) || isnan(c) || isnan(f)) {
    Serial.print(".");
    delay(5000);      // Wait 5 seconds before reading again
    h = dht.readHumidity();
    // Read temperature as Celsius
    c = dht.readCelcius();
    // Read temperature as Fahrenheit
    f = dht.readFahrenheit();
  }
  Serial.println();
  
  hif = dht.computeHeatIndexFahrenheit(f, h);
  hic = dht.computeHeatIndexCelcius(c, h);
 }

/*
 * Print the values read by the sensor for debugging.
 */
void printValues() {
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(c);
  Serial.print(" *C\t");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C\t");
  Serial.print(hif);
  Serial.print(" *F\t");
  Serial.print("Voltage: ");
  Serial.print(v);
  Serial.println();
}

/*
 * Build the JSON string for the 
 */
void buildSensorJSON(char* sensor, int len) {
  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;

  JsonObject& sensorRoot = jsonBuffer.createObject();
  sensorRoot["sensor"] = sensorName;
  sensorRoot["h"] = h;
  sensorRoot["c"] = c;
  sensorRoot["f"] = f;
  sensorRoot["hi_c"] = hic;
  sensorRoot["hi_f"] = hif;
  sensorRoot["v"] = v;
  sensorRoot.printTo(sensor, len);

}

/**********************************************************************
 * Web Server Routines
 **********************************************************************/
/*
void webServer_root() {
  char localip[16];
  IPAddress ip = WiFi.localIP();
  sprintf(localip,"%d.%d.%d.%d", ip[0],ip[1],ip[2],ip[3]);
  char mqttip[16];
  sprintf(mqttip,"%d.%d.%d.%d", mqtt_server[0],mqtt_server[1],mqtt_server[2],mqtt_server[3]);
  String response;
  response = "<HTML><HEAD>";
  response += "<TITLE>" + String(sensorName) + "</TITLE>";
  response += "</HEAD><BODY>";
  response += "<H1>Sensor: " + String(sensorName) + "</H1>";
  response += "<BR /><HR /><BR />";
  response += "<H1>Latest Temperature and Humidity Readings</H1>";
  response += "<TABLE BORDER=0>";
  response += "<TR><TH>Fahrenheit</TH><TD>" + String(f) + " &#8457;</TD></TR>";
  response += "<TR><TH>Celcius</TH><TD>" + String(c) + " &#8451;</TD></TR>";
  response += "<TR><TH>Humidity</TH><TD>" + String(h) + "  %</TD></TR>";
  response += "<TR><TH>Heat Index Fahrenheit</TH><TD>" + String(hif) + " &#8457;</TD></TR>";
  response += "<TR><TH>Heat Index Celcius</TH><TD>" + String(hic) + " &#8451;</TD></TR>";
  response += "<TR><TH>Voltage</TH><TD>" + String(v) + " V</TD></TR>";
  response += "<TR><TH>Last Update</TH><TD>" + String(lastUpdatedTimestamp) + "</TD></TR>";
  response += "</TABLE>";
  response += "<BR /><HR /><BR />";
  response += "<H1>Stats</H1>";
  response += "<TABLE BORDER=0>";
  response += "<TR><TH COLSPAN=2>IoT System</TH></TR>";
  response += "<TR><TH>Property</TH><TH>Value</TH></TR>";
  response += "<TR><TD>Client Name</TD><TD>" + String(clientName) + "</TD></TR>";
  response += "<TR><TD>IP Address</TD><TD>" + String(localip) + "</TD></TR>";
  response += "<TR><TH COLSPAN=2>&nbsp;</TH></TR>";
  response += "<TR><TH COLSPAN=2>Sensor</TH></TR>";
  response += "<TR><TD>Sensor Name</TD><TD>" + String(sensorName) + "</TD></TR>";
  response += "<TR><TD>Update Interval</TD><TD>" + String(timeBetweenUpdates) + " millis</TD></TR>";
  response += "<TR><TD>Last Update</TD><TD>" + String(lastUpdatedTimestamp) + "</TD></TR>";
  response += "<TR><TH COLSPAN=2>&nbsp;</TH></TR>";
  response += "<TR><TH COLSPAN=2>MQTT</TH></TR>";
  response += "<TR><TH>Property</TH><TH>Value</TH></TR>";
  response += "<TR><TD>Broker</TD><TD>" + String(mqttip) + "</TD></TR>";
  response += "<TR><TD>Port</TD><TD>" + String(mqtt_port) + "</TD></TR>";
  response += "<TR><TD>Hello Command Topic</TD><TD>" + String(mqtt_commandHelloTopic) + "</TD></TR>";
  response += "<TR><TD>Who Am I Command Topic</TD><TD>" + String(mqtt_commandWhoAmITopic) + "</TD></TR>";
  response += "<TR><TD>Data Topic</TD><TD>" + String(mqtt_dataTopic) + "</TD></TR>";
  response += "<TR><TD>Subscribed Topic</TD><TD>" + String(mqtt_subscribedTopic) + "/#</TD></TR>";
  response += "<TR><TH COLSPAN=2>&nbsp;</TH></TR>";
  response += "<TR><TH COLSPAN=2>Thing Speak</TH></TR>";
  response += "<TR><TH>Property</TH><TH>Value</TH></TR>";
  response += "<TR><TD>Key</TD><TD>" + String(thingspeak_key) + "</TD></TR>";
  response += "<TR><TD>Host</TD><TD>" + String(thingspeak_host) + "</TD></TR>";
  response += "</TABLE>";
  response += "<BR /><HR /><BR />";
  response += "<H1>Links</H1>";
//  response += "<a href='/temp'>Temperature</a><br>";
//  response += "<a href='/humidity'>Humidity</a><br>";
  response += "<a href='/json'>JSON</a><br>";
  response += "</BODY></HTML>";
  webServer.send(200, "text/html", response);
  delay(100);
}

void webServer_temp() {
  String response;
  response="Temperature: "+String(f)+" F";   // Arduino has a hard time with float to string
  webServer.send(200, "text/plain", response);            // send to someones browser when asked
}

void webServer_humidity() {
  String response;
  response="Humidity: "+String(h)+" %";   // Arduino has a hard time with float to string
  webServer.send(200, "text/plain", response);            // send to someones browser when asked
}

void webServer_json() {
  String response;
  response = String(sensorJSON);   // Arduino has a hard time with float to string
  webServer.send(200, "application/json", response);            // send to someones browser when asked
}
*/
