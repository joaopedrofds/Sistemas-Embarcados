#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP32Servo.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "DHT.h"


/*
  Globals
*/
#define SERVO_PIN 2

#define MQ2_PIN 34

#define LED_R 18
#define LED_B 19
#define LED_G 21

#define DHT_PIN 32
#define DHTTYPE DHT11

#define BROKER_TOPIC_MQ2 "failsafe/mq2"
#define BROKER_TOPIC_DHT11_TEMP "failsafe/dht11/t"
#define BROKER_TOPIC_DHT11_HUMI "failsafe/dht11/h"


/*
WiFi / Broker Settings
*/
void wifiConnect(const char* ssid, const char* password);
void brokerConnect(const char* broker_ssid, const int broker_port);

const char* ssid = "uaifai-tiradentes";
const char* password = "bemvindoaocesar";

const char* broker_ssid = "172.26.69.78";
const int broker_port = 1883;


WiFiClient espClient;
PubSubClient client(espClient);


/*
  Control
*/
void vTaskMQ2(void *param);
void vTaskDHT11(void *param);
void vTaskLed(void *param);
void vTaskExhaust(void *param);

Servo servo;

DHT dht(DHT_PIN, DHTTYPE);

TaskHandle_t taskLedkHandle;
TaskHandle_t taskDHTHandle;
TaskHandle_t taskExhaustHandle;
TaskHandle_t taskMQ2Handle;

QueueHandle_t queueMQ2Handle;
QueueHandle_t queueTEMPHandle;
QueueHandle_t queueHUMHandle;


/*
Setup
*/
void setup() {
  Serial.begin(115200);
  
  pinMode(LED_B, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);

  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_R, LOW);

  servo.attach(SERVO_PIN);

  dht.begin();
  
  wifiConnect(ssid, password);
  client.setServer(broker_ssid, broker_port);
  brokerConnect(broker_ssid, broker_port);

  queueMQ2Handle = xQueueCreate(1, sizeof(float));
  queueTEMPHandle = xQueueCreate(1, sizeof(float));
  queueHUMHandle = xQueueCreate(1, sizeof(float));
  
  BaseType_t xReturned;
  xReturned = xTaskCreate(vTaskMQ2, "TASK_MQ2", 4096, NULL, 2, &taskMQ2Handle);
  xReturned = xTaskCreate(vTaskDHT11, "TASK_DHT11", 4096, NULL, 2, &taskDHTHandle);
  xReturned = xTaskCreate(vTaskLed, "TASK_LED", 4096, NULL, 1, &taskLedkHandle);
  xReturned = xTaskCreate(vTaskExhaust, "TASK_EXHAUST", 4096, NULL, 10, &taskExhaustHandle);
}


void loop() {
  wifiConnect(ssid, password);
  brokerConnect(broker_ssid, broker_port);
  
  if (client.connected())
  {
    client.loop();
  }
  

  vTaskDelay(pdMS_TO_TICKS(150));
}


void wifiConnect(const char* ssid, const char* password)
{
  if (WiFi.status() == WL_CONNECTED) { return; }
  
  Serial.print("Connecting to WiFi . ");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED  )
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.print(". ");
  }

  Serial.printf("\n\tWiFi Connected @ %s\n", ssid);
} 


void brokerConnect(const char* broker_ssid, const int broker_port) 
{
  if (client.connected()) { return; }
  if (WiFi.status() != WL_CONNECTED) { return; }
  

  client.setServer(broker_ssid, broker_port);
  Serial.print("Connecting to MQTT Broker... ");
  
  // Cria um ID aleatório para não conflitar
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("Connected!");
  } else {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Não use delay bloqueante aqui se possível, mas no loop principal já tem controle
  }
} 


void vTaskMQ2(void *param)
{
  float mq2 = 0;
  
  for (;;)
  {
    mq2 = analogRead(MQ2_PIN);
    
    xQueueOverwrite(queueMQ2Handle, &mq2);
    
    if (client.connected())
    {
      client.publish(BROKER_TOPIC_MQ2, String(mq2).c_str(), true);
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}


void vTaskDHT11(void *param)
{
  float t = 0;
  float h = 0;

  for (;;)
  {
    t = dht.readTemperature();
    h = dht.readHumidity();
    
    xQueueOverwrite(queueTEMPHandle, &t);
    xQueueOverwrite(queueHUMHandle, &h);
    
    if (client.connected()) 
    {
      client.publish(BROKER_TOPIC_DHT11_TEMP, String(t).c_str(), true);
      client.publish(BROKER_TOPIC_DHT11_HUMI, String(h).c_str(), true);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}


void vTaskLed(void *param)
{
  float mq2Peek = 0.0;
  float tempPeek = 0.0;
  float humPeek = 0.0;

  float mq2Value = 0.0;
  float tempValue = 0.0;
  float humValue = 0.0;

  bool ledState = LOW;   // Armazena o estado do LED para o blink

  for(;;)
  {
    if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0))
    {
      mq2Value = mq2Peek;
    }
    if (xQueuePeek(queueTEMPHandle, &tempPeek, 0))
    {
      tempValue = tempPeek;
    }
    if (xQueuePeek(queueHUMHandle, &humPeek, 0))
    {
      humValue = humPeek;
    }
    

    Serial.print("[MQ-2] -- ");
    Serial.print(mq2Value);
    Serial.println(" adc");

    Serial.print("[TEMP] -- ");
    Serial.print(tempValue);
    Serial.println(" ºC");

    Serial.print("[HUMI] -- ");
    Serial.print(humValue);
    Serial.println(" RH");  


  if (mq2Value >= (4095 / 2)) // 1. Nível mais alto (ex: 2047.5)
  {
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      ledState = HIGH; 
  }
  else if (mq2Value >= 1050) // 2. Nível de aviso
  {
      ledState = !ledState; 
      digitalWrite(LED_R, ledState);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
  }
  else // 3. Nível normal
  {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, LOW);
      ledState = LOW;
  }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}


void vTaskExhaust(void *param)
{
  for(;;)
  {
    Serial.println("......................");

    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}
