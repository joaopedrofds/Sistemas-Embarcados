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

#define DHTTYPE DHT11
#define DHT_PIN 32

#define BROKER_TOPIC_MQ2 "failsafe/mq2"
#define BROKER_TOPIC_DHT11_TEMP "failsafe/dht11/t"
#define BROKER_TOPIC_DHT11_HUMI "failsafe/dht11/h"


/*
WiFi / Broker Settings
*/
void wifiConnect(const char* ssid, const char* password);
void brokerConnect(const char* broker_ssid, const int broker_port);

const char* ssid = "uaifai-apolo";
const char* password = "bemvindoaocesar";

const char* broker_ssid = "PLACEHOLDER";
const int broker_port = 1883;
const char* broker_username = "PLACEHOLDER";
const char* broker_password = "PLACEHOLDER";  

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

  wifiConnect(ssid, password);
  brokerConnect(broker_ssid, broker_port);

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
  
  BaseType_t xReturned;
  xReturned = xTaskCreate(vTaskLed, "TASK_LED", 4096, NULL, 1, &taskLedkHandle);
  xReturned = xTaskCreate(vTaskMQ2, "TASK_MQ2", 4096, NULL, 2, &taskMQ2Handle);
  xReturned = xTaskCreate(vTaskDHT11, "TASK_DHT11", 4096, NULL, 2, &taskDHTHandle);
  xReturned = xTaskCreate(vTaskExhaust, "TASK_EXHAUST", 4096, NULL, 10, &taskExhaustHandle);

  queueMQ2Handle = xQueueCreate(1, sizeof(float));
  queueTEMPHandle = xQueueCreate(1, sizeof(float));
  queueHUMHandle = xQueueCreate(1, sizeof(float));
}


void loop() {
  wifiConnect(ssid, password);
  brokerConnect(broker_ssid, broker_port);
  vTaskDelay(pdMS_TO_TICKS(50));
}


void wifiConnect(const char* ssid, const char* password)
{
  if (WiFi.status() == WL_CONNECTED) { return; }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.println("Connecting to WiFi . ");
  while (WiFi.status() != WL_CONNECTED  )
  {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(". ");
  }

  Serial.printf("\n\t!!! WiFi Connected @ %s !!!\n", WiFi.localIP().toString().c_str());
} 


void brokerConnect(const char* broker_ssid, const int broker_port) 
{
  Serial.println("\t!!! FAÇA O CODIGO DE CONEXAO COM O BROKER. . . !!!");
}


void vTaskMQ2(void *param)
{
  float mq2 = 0;
  
  for (;;)
  {
    mq2 = analogRead(MQ2_PIN);
    
    xQueueOverwrite(queueMQ2Handle, &mq2);
    
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
    Serial.println(" ppm");

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
