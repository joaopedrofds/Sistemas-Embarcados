#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFiClientSecure.h>

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
  Wi-fi / Broker Settings
*/
const char* ssid = "uaifai-tiradentes";
const char* password = "bemvindoaocesar";

const char* broker_ssid = "a75536483f004b7d9dc969b95d17de8d.s1.eu.hivemq.cloud";
const int broker_port = 8883;
const char* mqtt_username = "failsafe";
const char* mqtt_password = "Admin12345";

WiFiClientSecure espClient; 
PubSubClient client(espClient);

void vTaskMQ2(void *param);
void vTaskDHT11(void *param);
void vTaskLed(void *param);
void vTaskExhaust(void *param);
void wifiConnect(const char* ssid, const char* password);
void brokerConnect();


Servo servo;
DHT dht(DHT_PIN, DHTTYPE);

TaskHandle_t taskLedkHandle;
TaskHandle_t taskDHTHandle;
TaskHandle_t taskExhaustHandle;
TaskHandle_t taskMQ2Handle;

QueueHandle_t queueMQ2Handle;
QueueHandle_t queueTEMPHandle;
QueueHandle_t queueHUMHandle;

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
  
  espClient.setInsecure();
  
  wifiConnect(ssid, password);
  client.setServer(broker_ssid, broker_port);
  brokerConnect();

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
  brokerConnect();
  
  if (client.connected()) {
    client.loop();
  }
  
  vTaskDelay(pdMS_TO_TICKS(150));
}

void wifiConnect(const char* ssid, const char* password) {
  if (WiFi.status() == WL_CONNECTED) { return; }
  
  Serial.print("Connecting to WiFi . ");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.print(". ");
  }

  Serial.printf("\n\tWiFi Connected @ %s\n", ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
} 

void brokerConnect() {
  if (client.connected()) { return; }
  if (WiFi.status() != WL_CONNECTED) { return; }
  
  Serial.print("Connecting to HiveMQ Cloud... ");
  
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("Connected to HiveMQ Cloud!");
    
  } else {
    Serial.print("Failed to connect, rc=");
    Serial.print(client.state());
    Serial.println(" Check your credentials and try again");
  }
} 

void vTaskMQ2(void *param) {
  float mq2 = 0;
  
  for (;;) {
    mq2 = analogRead(MQ2_PIN);
    
    xQueueOverwrite(queueMQ2Handle, &mq2);
    
    if (client.connected()) {
      client.publish(BROKER_TOPIC_MQ2, String(mq2).c_str(), true);
      Serial.printf("Published MQ2: %.2f to topic: %s\n", mq2, BROKER_TOPIC_MQ2);
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void vTaskDHT11(void *param) {
  float t = 0;
  float h = 0;

  for (;;) {
    t = dht.readTemperature();
    h = dht.readHumidity();
    
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      xQueueOverwrite(queueTEMPHandle, &t);
      xQueueOverwrite(queueHUMHandle, &h);
      
      if (client.connected()) {
        client.publish(BROKER_TOPIC_DHT11_TEMP, String(t).c_str(), true);
        client.publish(BROKER_TOPIC_DHT11_HUMI, String(h).c_str(), true);
        Serial.printf("Published Temp: %.2f°C, Humidity: %.2f%%\n", t, h);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void vTaskLed(void *param) {
  float mq2Peek = 0.0;
  float tempPeek = 0.0;
  float humPeek = 0.0;

  float mq2Value = 0.0;
  float tempValue = 0.0;
  float humValue = 0.0;

  bool ledState = LOW;

  for(;;) {
    if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0)) {
      mq2Value = mq2Peek;
    }
    if (xQueuePeek(queueTEMPHandle, &tempPeek, 0)) {
      tempValue = tempPeek;
    }
    if (xQueuePeek(queueHUMHandle, &humPeek, 0)) {
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

    if (mq2Value >= (4095 / 2)) {
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      ledState = HIGH; 
    }
    else if (mq2Value >= 1050) {
      ledState = !ledState; 
      digitalWrite(LED_R, ledState);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
    }
    else {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, LOW);
      ledState = LOW;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}

void vTaskExhaust(void *param) {
  for(;;) {
    Serial.println("......................");
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}