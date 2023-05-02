// Theoretically support 0-50celsius with +-2biases, 20-90%RH with +-5RH
// scale: Celsius | %RH
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

// lib to support dht11
#include <dht11.h>

#define DHT11PIN 2  // GPIO2 D4
// #define DHT11PIN 0 // GPIO0 D3

// Instantiatet DHT11
dht11 DHT11;

// Set Wiff access information
const char* ssid = "where is your dick";        // wifi name
const char* password = "boyulinzuishuai";       // wifi password

// MQTT connection config
// const char* mqttServer = "test.ranye-iot.net";  // mqtt server address
const char* mqttServer = "139.224.56.89";  // mqtt server address
const int mqttPort = 1883;                      // mqtt server port

const char* mqttUserName = "user";    // 服务端连接用户名(需要修改)
const char* mqttPassword = "123456";  // 服务端连接密码(需要修改)

String clientIdStr = "esp8266-" + WiFi.macAddress();
const char* clientId = clientIdStr.c_str();  // 客户端id (需要修改)

// const char* subTopic = "iotuser/led_kz";        // 订阅主题(需要修改)
// const char* pubTopic = "iotuser/led_zt";        // 订阅主题(需要修改)
// const char* willTopic = "iotuser/led_yz";       // 遗嘱主题名称(需要修改)

//遗嘱相关信息
// const char* willMsg = "esp8266 offline";        // 遗嘱主题信息
// const int willQos = 0;                          // 遗嘱QoS
// const int willRetain = false;                   // 遗嘱保留

// const int subQoS = 1;            // 客户端订阅主题时使用的QoS级别（截止2020-10-07，仅支持QoS = 1，不支持QoS = 2）
// const bool cleanSession = false; // 清除会话（如QoS>0必须要设为false）                   // mqtt server port

Ticker ticker;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int count;  //  variable that used in Ticker count time(sec)

float humidity = 0.0;
float temperature = 0.0;

int humidSensorSwitch = 1;  // 1 => open | 0 => close
int temperatureSensorSwitch = 1;  // 1 => open | 0 => close

void setup() {
  Serial.begin(9600);

  // Ticker定时对象
  ticker.attach(1, tickerCount);  // invoke the recall function tickerCount every second

  // Set ESP8266 work mode to wireless terminal mode
  WiFi.mode(WIFI_STA);

  // Connect Wifi
  connectWifi();

  // Set MQTT server and port
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(receiveCallback);

  // Connect MQTT Server
  connectMQTTServer();
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

// Get current state of DHT11 sensor
// (refresh before get current temperature and humidity)
void refreshState() {
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk) {
    case DHTLIB_OK:
      Serial.println("OK");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }
}

void getHumidity() {
  humidity = (float)DHT11.humidity;
}

void getTemperature() {
  temperature = (float)DHT11.temperature;
}

void tickerCount() {
  count++;
}

// open humid sensor, 1 => open
void openHumidSensor() {
  humidSensorSwitch = 1;
  pubMQTTmsg();
}

// close humid sensor, 0 => close
void closeHumidSensor() {
  humidSensorSwitch = 0;
  pubMQTTmsg();
}

// open temperature sensor, 1 => open
void openTemperatureSensor() {
  temperatureSensorSwitch = 1;
  pubMQTTmsg();
}

// close temperature sensor, 0 => close
void closeTemperatureSensor() {
  temperatureSensorSwitch = 0;
  pubMQTTmsg();
}


// ESP8266 connect MQTT server
void connectMQTTServer() {
  // according to ESP8266's mac address to generate client device ID (to avoid ID collision)
  // String clientId = "esp8266-" + WiFi.macAddress();

  // connect MQTT server
  // Serial.println("MQTT connection status: ");
  // Serial.println(String(mqttClient.connect(clientId.c_str())));
  // Serial.println(String(mqttClient.connect(clientId)));
  /* 连接MQTT服务器
  boolean connect(const char* id, const char* user, 
                  const char* pass, const char* willTopic, 
                  uint8_t willQos, boolean willRetain, 
                  const char* willMessage, boolean cleanSession); 
  若让设备在离线时仍然能够让qos1工作，则connect时的cleanSession需要设置为false                
  */
  if (mqttClient.connect(clientId, mqttUserName, mqttPassword)) {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId:");
    // Serial.println(clientId);
    Serial.println(clientIdStr);
    subscribeTopic();  // subscribe to specific topic
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(3000);
  }
}

// Publish humid sensor status (On / Off)
void pubHumidSensorStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "humidSensor-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoLight
  String messageString;
  if (humidSensorSwitch == 1) {
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

// Publish humid sensor connection status (Online / Offline)
void pubHumidSensorConnectionStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "humidSensor-connection-status-" + WiFi.macAddress();
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

// Publish humid sensor power Degree
void pubHumidDegree() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "humidSensor-power-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoFan
  String messageString;
  messageString = String(humidity);
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

// Publish temperature sensor status (On / Off)
void pubTemperatureSensorStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "tempSensor-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoLight
  String messageString;
  if (temperatureSensorSwitch == 1) {
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

// Publish temperature sensor connection status (Online / Offline)
void pubTemperatureSensorConnectionStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "tempSensor-connection-status-" + WiFi.macAddress();
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

// Publish temperature sensor power Degree
void pubTemperatureDegree() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "tempSensor-power-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoFan
  String messageString;
  messageString = String(temperature);
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

// Publish messages
void pubMQTTmsg() {
  pubHumidSensorStatus();
  pubHumidSensorConnectionStatus();
  pubTemperatureSensorStatus();
  pubTemperatureSensorConnectionStatus();
  
  // Get sensor information
  refreshState();
  // Get humidity data
  getHumidity();
  // Get humidity data
  getTemperature();
  
  if(humidSensorSwitch == 1) {
    pubHumidDegree();
  }
  if(temperatureSensorSwitch == 1) {
    pubTemperatureDegree();
  }
}

// subscribe humid sensor
void subscribeHumidSensorTopic() {
  // Create subscrib topic with postfix with Mac address
  // to avoid topic collision when similar devices connect to the same devices
  String topicString = "humidSensor-" + WiFi.macAddress();
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

// subscribe temperature sensor
void subscribeTemperatureSensorTopic() {
  // Create subscrib topic with postfix with Mac address
  // to avoid topic collision when similar devices connect to the same devices
  String topicString = "tempSensor-" + WiFi.macAddress();
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
  subscribeHumidSensorTopic();
  subscribeTemperatureSensorTopic();  
}

// humid sensor callback
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
    openHumidSensor();                    // Then sensor is opened.
  } else {
    closeHumidSensor();  // Otherwise the sensor is closed.
  }
}

// temperature sensor callback
void temperatureSensorCallback(char* topic, byte* payload, unsigned int length) {
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
    openTemperatureSensor();                    // Then sensor is opened.
  } else {
    closeTemperatureSensor();  // Otherwise the sensor is closed.
  }
}

// Callback after receive message from MQTT server
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  String comingTopic = topic;
  String humidSensorTopicString = "humidSensor-" + WiFi.macAddress();
  String tempSensorTopicString = "tempSensor-" + WiFi.macAddress();
  // autolight callback
  if (comingTopic == humidSensorTopicString) {
    brightSensorCallback(topic, payload, length);
  }
  // lightSensor callback
  if (comingTopic == tempSensorTopicString) {
    temperatureSensorCallback(topic, payload, length);
  }
}

// // Subscrib specific topic
// // 后续改进 能不能通过外部传参 topic的前缀来制定订阅的topic名字
// void subscribeTopic(){
//   // Create subscrib topic with postfix with Mac address
//   // to avoid topic collision when similar devices connect to the same devices
//   String topicString = "autoFan-" + WiFi.macAddress();
//   char subTopic[topicString.length() + 1]; // plus one is a settled operation, you can see as the string has a hide one at the end
//   strcpy(subTopic, topicString.c_str());

//   // Through the serial monitor to check if subscribe the topic successfully and the name info about this topic
//   // method subscribe beneath require arguments to be a char array
//   if(mqttClient.subscribe(subTopic)){
//     Serial.println("Subscribe Topic:");
//     Serial.println(subTopic);
//   } else {
//     Serial.print("Subscribe Fail...");
//   }
// }

// // Callback after receive message from MQTT server
// void receiveCallback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message Received [");
//   Serial.print(topic);
//   Serial.print("] ");
//   for (int i = 0; i < length; i++) {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println("");
//   Serial.print("Message Length(Bytes) ");
//   Serial.println(length);

//   if ((char)payload[0] == '1') {     // if the message received started with 1
//     fullSpeedFan(); // Then fan is opened.
//   } else {
//     closeFan(); // Otherwise the fan is closed.
//   }
// }

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

double Fahrenheit(double celsius) {
  return 1.8 * celsius + 32;
}  //摄氏温度度转化为华氏温度

double Kelvin(double celsius) {
  return celsius + 273.15;
}  //摄氏温度转化为开氏温度

// 露点（点在此温度时，空气饱和并产生露珠）
// 参考: http://wahiduddin.net/calc/density_algorithms.htm
// double dewPoint(double celsius, double humidity) {
//   double A0 = 373.15 / (273.15 + celsius);
//   double SUM = -7.90298 * (A0 - 1);
//   SUM += 5.02808 * log10(A0);
//   SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1);
//   SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1);
//   SUM += log10(1013.246);
//   double VP = pow(10, SUM - 3) * humidity;
//   double T = log(VP / 0.61078);  // temp var
//   return (241.88 * T) / (17.558 - T);
// }

// 快速计算露点，速度是5倍dewPoint()
// 参考: http://en.wikipedia.org/wiki/Dew_point
// double dewPointFast(double celsius, double humidity) {
//   double a = 17.271;
//   double b = 237.7;
//   double temp = (a * celsius) / (b + celsius) + log(humidity / 100);
//   double Td = (b * temp) / (a - temp);
//   return Td;
// }

// void printFullState() {
//   Serial.print("Humidity (%): ");
//   Serial.println((float)DHT11.humidity, 2);

//   Serial.print("Temperature (oC): ");
//   Serial.println((float)DHT11.temperature, 2);

//   Serial.print("Temperature (oF): ");
//   Serial.println(Fahrenheit(DHT11.temperature), 2);

//   Serial.print("Temperature (K): ");
//   Serial.println(Kelvin(DHT11.temperature), 2);

//   Serial.print("Dew Point (oC): ");
//   Serial.println(dewPoint(DHT11.temperature, DHT11.humidity));

//   Serial.print("Dew PointFast (oC): ");
//   Serial.println(dewPointFast(DHT11.temperature, DHT11.humidity));
// }