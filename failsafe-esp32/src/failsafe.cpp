#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "DHT.h"
#include "MQ2_LPG.h"

/*
  Globals
*/
#define EXHAUST_PIN 25
#define DHT_PIN 32
#define MQ2_PIN 34

#define LED_R 18
#define LED_G 19
#define LED_B 21

#define ALERT_STATE 800
#define HIGH_ALERT_STATE 1200

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

void wifiConnect(const char* ssid, const char* password);
void brokerConnect();


/*
  Controls
*/
void vTaskMQ2(void *param);
void vTaskDHT11(void *param);
void vTaskLed(void *param);
void vTaskExhaust(void *param);

DHT dht(DHT_PIN, DHTTYPE);
MQ2Sensor mq2(MQ2_PIN);

TaskHandle_t taskLedkHandle;
TaskHandle_t taskDHTHandle;
TaskHandle_t taskExhaustHandle;
TaskHandle_t taskMQ2Handle;

QueueHandle_t queueMQ2Handle;
QueueHandle_t queueTEMPHandle;
QueueHandle_t queueHUMHandle;


/*
  MQ-2 Gas Calibration Data
*/
#define RL_Value 100 // 100K ohm
#define x1_Value 199.150007852152
#define x2_Value 797.3322752256328
#define y1_Value 1.664988323698715
#define y2_Value 0.8990240080541785
#define x_Value 497.4177875376839
#define y_Value 1.0876679972710004
#define Ro_Value 6.02
#define Voltage_Value 5.0
#define bitADC_Value 4095.0 // development board adc resolution

void calibration();


/*
  Code Start
*/
void setup() {
  Serial.begin(115200);
  
  pinMode(LED_B, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);

  pinMode(MQ2_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(EXHAUST_PIN, OUTPUT);

  digitalWrite(LED_B, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_R, LOW);

  dht.begin();
  mq2.begin();
  
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
  xReturned = xTaskCreate(vTaskExhaust, "TASK_EXHAUST", 4096, NULL, 3, &taskExhaustHandle);
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
  
  float gas = 0.0;

  for (;;) {
    calibration();
    gas = mq2.readGas();
    //mq2.viewGasData();
    xQueueOverwrite(queueMQ2Handle, &gas);
    
    
    if (client.connected()) {
      client.publish(BROKER_TOPIC_MQ2, String(gas).c_str(), true);
      Serial.printf("Published MQ2: %.2f to topic: %s\n", gas, BROKER_TOPIC_MQ2);
    }
    
    
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void vTaskDHT11(void *param) {
  float t = 0.0;
  float h = 0.0;

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

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

// void vTaskLed(void *param) {
//   float mq2Peek = 0.0;
//   float tempPeek = 0.0;
//   float humPeek = 0.0;

//   float gasValue = 0.0;
//   float tempValue = 0.0;
//   float humValue = 0.0;

//   bool ledState = LOW;
  
//   int velocidade = 0;

//   for(;;) {
//     if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0)) {
//       gasValue = mq2Peek;
//     }
//     if (xQueuePeek(queueTEMPHandle, &tempPeek, 0)) {
//       tempValue = tempPeek;
//     }
//     if (xQueuePeek(queueHUMHandle, &humPeek, 0)) {
//       humValue = humPeek;
//     }
    
    
//     Serial.print("[MQ2: GAS] -- ");
//     Serial.print(gasValue);
//     Serial.println(" ppm");
    

//     Serial.print("[DHT: TEMP] -- ");
//     Serial.print(tempValue);
//     Serial.println(" ºC");

//     Serial.print("[DHT: HUMI] -- ");
//     Serial.print(humValue);
//     Serial.println(" RH");  


//     if (gasValue >= ALERT_STATE) {
//       digitalWrite(LED_R, HIGH);
//       digitalWrite(LED_G, LOW);
//       digitalWrite(LED_B, LOW);
//       ledState = HIGH;

//     }
//     else if (gasValue >= HIGH_ALERT_STATE) {
//       ledState = !ledState; 
//       digitalWrite(LED_R, ledState);
//       digitalWrite(LED_G, LOW);
//       digitalWrite(LED_B, LOW);

//     }
//     else {
//       digitalWrite(LED_R, LOW);
//       digitalWrite(LED_G, HIGH);
//       digitalWrite(LED_B, LOW);
//       ledState = LOW;
//     }
    
//     vTaskDelay(pdMS_TO_TICKS(3000));
//   }
// }
void vTaskLed(void *param) {
  float mq2Peek = 0.0;
  float gasValue = 0.0;
  bool ledState = LOW;

  for(;;) {
    if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0)) {
      gasValue = mq2Peek;
    }

    if (gasValue >= HIGH_ALERT_STATE) {
      // Estado Crítico: PISCAR O LED VERMELHO
      ledState = !ledState; 
      digitalWrite(LED_R, ledState);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      
      vTaskDelay(pdMS_TO_TICKS(200)); 
    }
    else if (gasValue >= ALERT_STATE) {
      // Estado de Alerta: VERMELHO FIXO
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      ledState = HIGH;
      
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    else {
      // Estado Normal: VERDE
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, LOW);
      ledState = LOW;
      
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// void vTaskExhaust(void *param) {
  
//   int defaultSpeed = 0;
//   int speed = defaultSpeed;
//   int maxSpeed = 255;

//   float mq2Peek = 0.0;
//   float mq2Value = 0.0;

//   for(;;) {

//     if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0)) {
//       mq2Value = mq2Peek;
//     }


//     if (mq2Value >= HIGH_ALERT_STATE) {
//       speed = maxSpeed;
//       analogWrite(EXHAUST_PIN, speed);
//     }
//     else if (mq2Value >= ALERT_STATE)
//     {
//       speed = maxSpeed;
//       analogWrite(EXHAUST_PIN, speed);
//     }
//     else {
//       speed = defaultSpeed;
//       analogWrite(EXHAUST_PIN, speed);
//     }
    

//   }
// }
void vTaskExhaust(void *param) {
  
  int defaultSpeed = 0;
  int speed = defaultSpeed;
  int maxSpeed = 4095;

  float mq2Peek = 0.0;
  float mq2Value = 0.0;

  for(;;) {

    if (xQueuePeek(queueMQ2Handle, &mq2Peek, 0)) {
      mq2Value = mq2Peek;
    }

    // Simplificação da lógica
    if (mq2Value >= ALERT_STATE) { // Acima de 1000 já liga no máximo
      speed = maxSpeed;
    }
    else {
      speed = defaultSpeed;
    }
    
    analogWrite(EXHAUST_PIN, speed);

    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}


void calibration(){
  mq2.RL(RL_Value); // resistance load setting
  mq2.Ro(Ro_Value); // reverse osmosis setting
  mq2.Volt(Voltage_Value); // voltage sensor setting
  mq2.BitADC(bitADC_Value); // development board adc resolution setting
  mq2.mCurve(x1_Value, x2_Value, y1_Value, y2_Value); // mCurve setting
  mq2.bCurve(x_Value, y_Value); // bCurve setting
  mq2.getCalibrationData(); // get data calibration
  mq2.viewCalibrationData(); // print to serial monitor: data calibration
}
