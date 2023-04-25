// D0, D4, D8 reserved, Cannot be used
// degree will be preprocessed before published (from 0 - 1023 => 0 - 99)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

int readBrightnessPin = 13; // Define GPIO13 | D7 pin to analog read pin

// Set Wiff access information
const char* ssid = "where is your dick"; // wifi name
const char* password = "boyulinzuishuai"; // wifi password
const char* mqttServer = "test.ranye-iot.net"; // mqtt server address
const int mqttPort = 1883; // mqtt server port

Ticker ticker;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


int count;    //  variable that used in Ticker count time(sec)

void setup() {               
  Serial.begin(9600);   //定义波特率 

  // Ticker定时对象
  ticker.attach(1, tickerCount); // invoke the recall function tickerCount every second

  pinMode(readBrightnessPin, INPUT); //将光感DO口接到D7

  // Set ESP8266 work mode to wireless terminal mode
  WiFi.mode(WIFI_STA);

  // Connect Wifi
  connectWifi();

  // Set MQTT server and port
  mqttClient.setServer(mqttServer, mqttPort);
  // mqttClient.setCallback(receiveCallback);

  // Connect MQTT Server
  connectMQTTServer();
}

void loop() {
  // int v=digitalRead(readBrightnessPin);  //读出D2的高低电平赋值给D7
  // if(v==0)
  // digitalWrite(LED,HIGH);//当DO输出电平时，控制内置led亮灭
  // else
  // digitalWrite(LED,LOW);

  // Serial.print("degree: ");
  // Serial.print(shine());  //串口监视器显示光照强度
  // Serial.print("\n");
  // delay(200);

  // keep connect heartbeat mechanism
  if (mqttClient.connected()) { // if the board connect the server successfully
    // publish message every 3 seconds
    if (count >= 3){
      pubMQTTmsg();
      count = 0;
    }    
    // keep heart beat
    mqttClient.loop();
  } else {                  // if the connection between board and server is failed
    connectMQTTServer();    // then try to connect to the server
  }   
}

void tickerCount(){
  count++;
}

// function that return the light degree
// range from 0 - 1023 to 0 - 99
int shine()
{
  int i=analogRead(A0);//光感模拟口接esp8266AO口
  i=1023-i;
  i=i/10.23-1;   //将光照强度设置为0-99
  return i;
}

// ESP8266 connect MQTT server
void connectMQTTServer(){
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
    // subscribeTopic(); // subscribe to specific topic
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(3000);
  }   
}

// Publish messages
void pubMQTTmsg(){
  // according to ESP8266's mac address to generate client device ID 
  // (to avoid ID collision) when different users publish messages 
  String topicString = "lightSensor-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];  
  strcpy(publishTopic, topicString.c_str());
 
  // publish the state of the autoFan
  String messageString;
  messageString = String(shine()) + " degree";
  char publishMsg[messageString.length() + 1];   
  strcpy(publishMsg, messageString.c_str());
  
  // achieve to publish message to MQTT server through ESP8266
  if(mqttClient.publish(publishTopic, publishMsg)){
    Serial.println("Publish Topic:");Serial.println(publishTopic);
    Serial.println("Publish message:");Serial.println(publishMsg);    
  } else {
    Serial.println("Message Publish Failed."); 
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