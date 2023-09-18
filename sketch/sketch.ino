#include <PubSubClient.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


hw_timer_t *Timer0_Cfg = NULL;

const int DHT_PIN = 13;                                   // define pin numbers
const int LDR_PIN = 35;
const int BUZZER_PIN = 33;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient espClient; //who is the wifi signal provider 
PubSubClient mqttClient(espClient);
DHTesp dhtSensor;
Servo MyServo;

char tempAr[6];                                          // define character arrays 
char humidAr[6];
char ldrAr[5];
char timeAr[9];
char dayAr[2];


int Pos = 0;                                             // define integer variable
int Del = 0;
int freq = 0;
int days = 0;
int nature = 0;
int timer = 0;

int t_1 = 0;
int t_2 = 0;
int t_3 = 0;

float alarm_1 = 0;                                       // define float variables
float alarm_2 = 0;
float alarm_3 = 0;
float part = 0;


void IRAM_ATTR Timer0_ISR()                          // interrupts function
{
  tone(BUZZER_PIN, freq);
    if (nature == 0){                                // continuous alarms
      tone(BUZZER_PIN, freq);
  
    }else if (nature == 1) {                         // repeated on-off function
      if (timer%2 == 0){
        noTone(BUZZER_PIN);
      }
      else{
        tone(BUZZER_PIN, freq);
      }

    }
    timer = timer + 1 ;                              // timer for repeated on-off function                                

}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);                                    // set analog resolution to 12 bits
  setupWifi();
  setupMqtt();
  timeClient.begin();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);                     // setup the DHT sensor pin
  MyServo.attach(27);                                          // setup the servo pin

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Timer0_Cfg = timerBegin(0, 80, true);                        // set the interrupts
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);

  mqttClient.publish("Buzzer_190350F", (uint8_t*)"", 0, true);  // clear the retain messages from the node red relavent to the buzzer and servo
  mqttClient.publish("Servo_190350F", (uint8_t*)"", 0, true);

  
}

void loop() {
  if (!mqttClient.connected()){
    connectToBroker();
  }
  
  mqttClient.loop();//get available messages or published values
  
  updateTime();                                        // call the time updating function
  alarm();                                             // call the alarm updating function
  updateTemperatureAndHumidity();                      // call the temperature and humidity updating function
  updateLDRValue();                                    // call the LDR updating function
  updateAngle();                                       // call the angle updating function
  mqttClient.publish("Temp_190350F", tempAr);          // publish the temperature humidity and the LDR values
  mqttClient.publish("Humid_190350F", humidAr);
  mqttClient.publish("LDR_190350F", ldrAr);

  delay(1000);
}


void updateTime(){                                                                                       // time update function

  timeClient.update();
  timeClient.setTimeOffset(19800);                                                                       // add time offset to the grinich time (5.30 h)
  String(timeClient.getFormattedTime()).toCharArray(timeAr, 9);
  Serial.println(timeAr);
  part = ((timeAr[1] - '0')+(timeAr[0] - '0')*10)*3600+((timeAr[4] - '0')+(timeAr[3] - '0')*10)*60;      // take the time in second        


  String(days).toCharArray(dayAr, 2);
  mqttClient.publish("Remaining_days_190350F", dayAr);

  if (String(timeAr) == "00:00:00"){
    if (days>0){
      days = days - 1;
      String(days).toCharArray(dayAr, 2);
      mqttClient.publish("Remaining_days_190350F", dayAr);
    }else{
      String(0).toCharArray(dayAr, 2);
      mqttClient.publish("Remaining_days_190350F", dayAr);
    }                                                                     // reduce the days by one when time reach to 00:00:00
    
  }
  
}

void settime1(){
  alarm_1 = (alarm_1/1000);                // convert the ms to s
}

void settime2(){
  alarm_2 = (alarm_2/1000);                // convert the ms to s
}

void settime3(){
  alarm_3 = (alarm_3/1000);                // convert the ms to s
}

void alarm(){

  if (days != 0){                                          // compare the remaining dates
    if (alarm_1 == part){                                  // compare the real time and the alarm 1 time
      if (t_1 == days){                          
        Serial.println("alarm 1 on");
        timerAlarmWrite(Timer0_Cfg, Del*1000000, true);
        timerAlarmEnable(Timer0_Cfg);
        t_1 = t_1 - 1;
      }

      

    }else if (alarm_2 == part){
      if (t_2 == days){                                     // compare the real time and the alarm 2 time
        Serial.println("alarm 2 on");
        timerAlarmWrite(Timer0_Cfg, Del*1000000, true);
        timerAlarmEnable(Timer0_Cfg);
        t_2 = t_2 - 1;
      }
  


    }else if (alarm_3 == part){
      if (t_3 == days){                                     // compare the real time and the alarm 3 time
        Serial.println("alarm 3 on");
        timerAlarmWrite(Timer0_Cfg, Del*1000000, true);
        timerAlarmEnable(Timer0_Cfg);
        t_3 = t_3 - 1 ;
      }  
    }
  }
}


void setupWifi(){
  WiFi.begin("Wokwi-GUEST", "");   

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");                        // these are for the connect when using physical implementation
  Serial.println("Wifi connected");
  Serial.println("IP address: ");
  Serial.print(WiFi.localIP());

}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);     // connect to the server
  mqttClient.setCallback(receiveCallback);
  
}

void connectToBroker(){  
  while (!mqttClient.connected()){
    Serial.println("Attemting MQTT connection...");
    if (mqttClient.connect("San ESP32")){
      Serial.println("connected..");                   
      mqttClient.subscribe("Buzzer_190350F");            // subscribe the all channels which are send messages from node red
      mqttClient.subscribe("Servo_190350F");
      mqttClient.subscribe("Alarm1_190350F");
      mqttClient.subscribe("Alarm2_190350F");
      mqttClient.subscribe("Alarm3_190350F");
      mqttClient.subscribe("Days_190350F");
      mqttClient.subscribe("Delay_190350F");
      mqttClient.subscribe("Frequency_190350F");
      mqttClient.subscribe("Nature_190350F");
      mqttClient.subscribe("Schedule_190350F");
      

      

      
    }else{
      Serial.print("failed");
      Serial.print(mqttClient.state());
      delay(5000); // when fail to connect again again try to connect without delay is not good 
    }

  }
}

void updateTemperatureAndHumidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();      // get the temperature and humidity data
  String(data.temperature, 2).toCharArray(tempAr, 6);         //2 for two decimal places 6 is the length of the temp array
  String(data.humidity, 2).toCharArray(humidAr, 6);           // convert the humidity value same as temperature 
}

void updateLDRValue(){
  int LDR_Reading = analogRead(LDR_PIN);        // get the LDR value
  String(LDR_Reading).toCharArray(ldrAr, 5);    // convert the Value to character array
}

void updateAngle(){

  MyServo.write(Pos); // write the motor angle and rotate the medibox 
   
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
  if (strcmp(topic, "Buzzer_190350F") == 0){
    if (atoi(payloadChaAr) == 0){
      noTone(BUZZER_PIN);
      timerAlarmWrite(Timer0_Cfg, Del*1000000, false);
      timerAlarmDisable(Timer0_Cfg); 
    }

  }
  else if (String(topic) == "Servo_190350F"){
    Pos = atoi(payloadChaAr); 
  }
  else if (String(topic) == "Alarm1_190350F"){
    if (payloadChaAr == "false"){
      alarm_1 = -100;

    }else{
      alarm_1 = atoi(payloadChaAr);
      settime1();
    }
    
  }
  else if (String(topic) == "Alarm2_190350F"){
    if (payloadChaAr == "false"){
      alarm_2 = -110;
    }else{
      alarm_2 = atoi(payloadChaAr);
      settime2();
    }
    
  }else if (String(topic) == "Alarm3_190350F"){
    if (payloadChaAr == "false"){
      alarm_3 = -120;
    }else{
      alarm_3 = atoi(payloadChaAr);
      settime3();
    }
    
  }else if (String(topic) == "Days_190350F"){
    
    days = atoi(payloadChaAr);
    t_1 = days;
    t_2 = days;
    t_3 = days;
    
  }
  else if (String(topic) == "Delay_190350F"){
    
    Del = atoi(payloadChaAr);
    
      
  }else if (String(topic) == "Frequency_190350F"){
    
    freq = atoi(payloadChaAr);
      
  }else if (String(topic) == "Nature_190350F"){
    nature = atoi(payloadChaAr);

  }
  else if (String(topic) == "Schedule_190350F"){
    if (payloadChaAr == "false"){
      alarm_1 = -100;
      alarm_2 = -110;
      alarm_3 = -120;

    }
  }
  
}









