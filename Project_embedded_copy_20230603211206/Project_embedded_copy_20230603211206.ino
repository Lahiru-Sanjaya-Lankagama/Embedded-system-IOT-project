#include <PubSubClient.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUDP.h>


#define LED_BUTTON 2

const int DHT_PIN = 13;
const int LDR_PIN = 35;
const int BUZZER_PIN = 33;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient espClient; //who is the wifi signal provider 
PubSubClient mqttClient(espClient);
DHTesp dhtSensor;
Servo MyServo;

char tempAr[6];
char humidAr[6];
char ldrAr[5]; 
char angleAr[3];
char timeAr[9];
char part1[5];


int Pos = 0;
int Del = 0;
int freq = 0;
float alarm_1 = 0;
float alarm_2 = 0;
float alarm_3 = 0;
float part = 0;



void setup() {
  Serial.begin(115200);
  analogReadResolution(10); // set analog resolution to 10 bits
  setupWifi();
  setupMqtt();
  timeClient.begin();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);
  MyServo.attach(27);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
}

void loop() {
  if (!mqttClient.connected()){
    connectToBroker();
  }
  
  mqttClient.loop();//get available messages or published values
  
  updateTime();
  alarm();
  updateTemperatureAndHumidity();
  updateLDRValue();
  updateAngle();
  mqttClient.publish("Temp", tempAr);
  mqttClient.publish("Humid", humidAr);
  mqttClient.publish("LDR", ldrAr);

  delay(1000);
}


void updateTime(){
  timeClient.update();
  timeClient.setTimeOffset(19800);
  String(timeClient.getFormattedTime()).toCharArray(timeAr, 9);
  mqttClient.publish("Time", timeAr); // string we can convert
  Serial.println(timeAr);
  part = ((timeAr[1] - '0')+(timeAr[0] - '0')*10)*3600+((timeAr[4] - '0')+(timeAr[3] - '0')*10)*60;
  Serial.println(part);

  
  
}

void settime1(){
  alarm_1 = (alarm_1/1000);
}

void settime2(){
  alarm_2 = (alarm_2/1000);
}

void settime3(){
  alarm_3 = (alarm_3/1000);
}

void alarm(){

  if (alarm_1 == part){
    Serial.println("Alarm on");
    tone(BUZZER_PIN, freq, Del);
  }else if (alarm_2 == part){
    Serial.println("Alarm on");
    tone(BUZZER_PIN, freq, Del);
  }else if (alarm_3 == part){
    Serial.println("Alarm on");
    tone(BUZZER_PIN, freq, Del);
  }
}


void setupWifi(){
  WiFi.begin("HUAWEI Y9 Prime 2019", "abcd1234"); // Dialog 4G 868, 91c5FEC1// Excel group_02, EX20200307 // HUAWEI Y9 Prime 2019, abcd1234  

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wifi connected");
  Serial.println("IP address: ");
  Serial.print(WiFi.localIP());

}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
  
}

void connectToBroker(){  
  while (!mqttClient.connected()){
    Serial.println("Attemting MQTT connection...");
    if (mqttClient.connect("San ESP32")){
      Serial.println("connected..");
      mqttClient.subscribe("Buzzer");
      mqttClient.subscribe("Servo");
      mqttClient.subscribe("minAngle");
      mqttClient.subscribe("Cfactor");
      mqttClient.subscribe("Alert_signal");
      mqttClient.subscribe("Alarm1");
      mqttClient.subscribe("Alarm2");
      mqttClient.subscribe("Alarm3");
      mqttClient.subscribe("Delay");
      mqttClient.subscribe("Frequency");

      
    }else{
      Serial.print("failed");
      Serial.print(mqttClient.state());
      delay(5000); // when fail to connect again again try to connect without delay is not good 
    }

  }
}

void updateTemperatureAndHumidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);//2 for two decimal places 6 is the length of the temp array
  String(data.humidity, 2).toCharArray(humidAr, 6);
}

void updateLDRValue(){
  int LDR_Reading = analogRead(LDR_PIN);
  String(LDR_Reading).toCharArray(ldrAr, 5);
}

void updateAngle(){

  for (int i = 0; i <=Pos; i++){
    MyServo.write(i);
    delay(500);
  }
  
}


void receiveCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char payloadChaAr[length];

  for (int i = 0; i<length; i++){
    Serial.print((char)payload[i]);
    payloadChaAr[i] = (char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, "Buzzer") == 0){
    if (payloadChaAr[0] == '1'){
      digitalWrite(BUZZER_PIN, LOW);
    }

  }
  else if (String(topic) == "Servo"){
    Pos = atoi(payloadChaAr);
    Serial.println(Pos);  
  }
  else if (String(topic) == "Alarm1"){
    alarm_1 = atoi(payloadChaAr);
    settime1();
  }
  else if (String(topic) == "Alarm2"){
    alarm_2 = atoi(payloadChaAr);
    settime2();
  }else if (String(topic) == "Alarm3"){
    alarm_3 = atoi(payloadChaAr);
    settime3();
  }
  else if (String(topic) == "Delay"){
    Del = atoi(payloadChaAr);
    Serial.println(Del);

  }else if (String(topic) == "Frequency"){
    freq = atoi(payloadChaAr);

  }
  
}









