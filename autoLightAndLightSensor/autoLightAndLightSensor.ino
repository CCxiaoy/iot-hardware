// D0, D4, D8 reserved, Cannot be used
// degree will be preprocessed before published (from 0 - 1023 => 0 - 99)
// LED brightness depends on low/0 => light | high/255 => dark
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

// Define GPIO13 | D7 pin to analog read pin
int readBrightnessPin = 13;

// Pin to control LED
int R = 12;  // D6
int G = 14;  // D5
int B = 4;   // D2

int brightness = 255;  // how bright the LED is // 0 => open | 255 => close

int brightnessSensorSwitch = 1;  // 1 => open | 0 => close

// Set Wiff access information
const char* ssid = "where is your dick";        // wifi name
const char* password = "boyulinzuishuai";       // wifi password
const char* mqttServer = "test.ranye-iot.net";  // mqtt server address
const int mqttPort = 1883;                      // mqtt server port

Ticker ticker;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int count;  //  variable that used in Ticker count time(sec)

void setup() {
  // Define Baud Rate
  Serial.begin(9600);

  // Ticker Timing Object
  ticker.attach(1, tickerCount);  // invoke the recall function tickerCount every second

  pinMode(readBrightnessPin, INPUT);  // Connect light perception D0 port to D7

  // Set LED Pin's work mode to output
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);

  // Set ESP8266 work mode to wireless terminal mode
  WiFi.mode(WIFI_STA);

  // Connect Wifi
  connectWifi();

  // Set MQTT server and port
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(receiveCallback);

  // Connect MQTT Server
  connectMQTTServer();

  // close light default
  brightness = 255;
  controlLED(brightness, brightness, brightness);
}

void loop() {
  // keep connect heartbeat mechanism
  if (mqttClient.connected()) {  // if the board connect the server successfully
    // publish message every 5 seconds
    if (count >= 5) {
      pubMQTTmsg();
      count = 0;
    }
    // keep heart beat
    mqttClient.loop();
  } else {                // if the connection between board and server is failed
    connectMQTTServer();  // then try to connect to the server
  }
}

void tickerCount() {
  count++;
}

// function that return the light degree
// range from 0 - 1023 to 0 - 99
int shine() {
  int i = analogRead(A0);  // Connect light perception analog port to A0 port
  i = 1023 - i;
  i = i / 10.23 - 1;  // 0-99 Map Light degree from 0-1023 to 0-99
  return i;
}

// Switch LED state
void switchLED() {
  // if led is light then close
  if (brightness == 0) {
    closeLED();
  }
  // if led is close then open
  else {
    openLED();
  }
}

// Open LED
void openLED() {
  brightness = 0;
  controlLED(brightness, brightness, brightness);
  // publish the new satate after changing
  pubMQTTmsg();
}
// Close LED
void closeLED() {
  brightness = 255;
  controlLED(brightness, brightness, brightness);
  // publish the new satate after changing
  pubMQTTmsg();
}
// Control LED precisely
void controlLED(int RV, int GV, int BV) {
  analogWrite(R, RV);
  analogWrite(G, GV);
  analogWrite(B, BV);
}

// open bright sensor, 1 => open
void openBrightSensor() {
  brightnessSensorSwitch = 1;
  pubMQTTmsg();
}

// close bright sensor, 0 => close
void closeBrightSensor() {
  brightnessSensorSwitch = 0;
  pubMQTTmsg();
}

// ESP8266 connect MQTT server
void connectMQTTServer() {
  // according to ESP8266's mac address to generate client device ID (to avoid ID collision)
  String clientId = "esp8266-" + WiFi.macAddress();

  // connect MQTT server
  Serial.println("MQTT connection status: ");
  Serial.println(String(mqttClient.connect(clientId.c_str())));
  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId:");
    Serial.println(clientId);
    subscribeTopic();  // subscribe to specific topic
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(3000);
  }
}

// Publish messages
void pubMQTTmsg() {
  pubLightStatus();
  pubLightConnectionStatus();
  pubLightPowerDegree();

  pubBrightnessSensorStatus();
  pubBrightnessSensorConnectionStatus();
  if (brightnessSensorSwitch == 1) {
    pubBrightnessDegree();
  }
}

// Publish aulight status (On / Off)
void pubLightStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoLight-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoLight
  String messageString;
  if (brightness == 0) {
    messageString = "On";
  } else {
    messageString = "Off";
  }
  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish autolight connection status (Online / Offline)
void pubLightConnectionStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoLight-connection-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the connection state of the autoLight
  String messageString = "Online";

  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish autoLight power Degree
void pubLightPowerDegree() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoLight-power-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoFan
  String messageString;

  if (brightness == 0) {
    messageString = "100";
  } else {
    messageString = "0";
  }

  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish bright sensor status (On / Off)
void pubBrightnessSensorStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "lightSensor-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoLight
  String messageString;
  if (brightnessSensorSwitch == 1) {
    messageString = "On";
  } else {
    messageString = "Off";
  }
  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish bright sensor connection status (Online / Offline)
void pubBrightnessSensorConnectionStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "lightSensor-connection-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the connection state of the bright sensor
  String messageString = "Online";

  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish Brightness Degree
void pubBrightnessDegree() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "lightSensor-power-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoFan
  String messageString;
  messageString = String(shine());
  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// subscribe autoLight
void subscribeAutoLightTopic() {
  // Create subscrib topic with postfix with Mac address
  // to avoid topic collision when similar devices connect to the same devices
  String topicString = "autoLight-" + WiFi.macAddress();
  char subTopic[topicString.length() + 1];  // plus one is a settled operation, you can see as the string has a hide one at the end
  strcpy(subTopic, topicString.c_str());

  // Through the serial monitor to check if subscribe the topic successfully and the name info about this topic
  // method subscribe beneath require arguments to be a char array
  if (mqttClient.subscribe(subTopic)) {
    Serial.println("Subscribe Topic:");
    Serial.println(subTopic);
  } else {
    Serial.print("Subscribe Fail...");
  }  
}

// subscribe bright sensor
void subscribeBrightnessSensorTopic() {
  // Create subscrib topic with postfix with Mac address
  // to avoid topic collision when similar devices connect to the same devices
  String topicString = "lightSensor-" + WiFi.macAddress();
  char subTopic[topicString.length() + 1];  // plus one is a settled operation, you can see as the string has a hide one at the end
  strcpy(subTopic, topicString.c_str());

  // Through the serial monitor to check if subscribe the topic successfully and the name info about this topic
  // method subscribe beneath require arguments to be a char array
  if (mqttClient.subscribe(subTopic)) {
    Serial.println("Subscribe Topic:");
    Serial.println(subTopic);
  } else {
    Serial.print("Subscribe Fail...");
  }  
}

// Subscrib specific topic
// 后续改进 能不能通过外部传参 topic的前缀来制定订阅的topic名字
void subscribeTopic() {
  subscribeAutoLightTopic();
  subscribeBrightnessSensorTopic();  
}

// autoLight callback
void autoLightCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  Serial.print("Message Length(Bytes) ");
  Serial.println(length);

  if ((char)payload[0] == '1') {  // if the message received started with 1
    openLED();                    // Then light is opened.
  } else {
    closeLED();  // Otherwise the light is closed.
  }
}

// bright sensor callback
void brightSensorCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  Serial.print("Message Length(Bytes) ");
  Serial.println(length);

  if ((char)payload[0] == '1') {  // if the message received started with 1
    openBrightSensor();                    // Then sensor is opened.
  } else {
    closeBrightSensor();  // Otherwise the sensor is closed.
  }
}

// Callback after receive message from MQTT server
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  String comingTopic = topic;
  String autoLightTopicString = "autoLight-" + WiFi.macAddress();
  String lightSensorTopicString = "lightSensor-" + WiFi.macAddress();
  // autolight callback
  if (comingTopic == autoLightTopicString) {
    autoLightCallback(topic, payload, length);
  }
  // lightSensor callback
  if (comingTopic == lightSensorTopicString) {
    brightSensorCallback(topic, payload, length);
  }
}

// ESP8266 connect wifi
void connectWifi() {
  WiFi.begin(ssid, password);

  // Wait until the connection success, then output the successful message
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("");
}