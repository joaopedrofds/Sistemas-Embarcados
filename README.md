# FailSafe - Sistema de Monitoramento de Qualidade do Ar e Temperatura

Sistema embarcado de monitoramento em tempo real de qualidade do ar utilizando ESP32, sensores MQ2 e DHT11, com interface web para visualização dos dados via MQTT.

## 📋 Descrição

O FailSafe é um sistema de monitoramento que coleta dados de sensores ambientais (fumaça/gás, temperatura e umidade) com a finalidade de monitoramento 24/7 para ambientes insalubres. Através de um ESP32, os dados são coletados e transmitidos via MQTT para uma interface web em tempo real. O sistema utiliza FreeRTOS para gerenciamento de múltiplas tarefas e HiveMQ Cloud como broker MQTT.

## 🛠️ Tecnologias

### Firmware (ESP32)
- **Plataforma**: PlatformIO
- **Framework**: Arduino
- **Microcontrolador**: ESP32 DevKit
- **Sensores**: MQ2 (fumaça/gás), DHT11 (temperatura/umidade)
- **Comunicação**: WiFi, MQTT (TLS)
- **RTOS**: FreeRTOS

### Cliente Web
- **Frontend**: HTML5, CSS3, JavaScript
- **Framework CSS**: Tailwind CSS
- **Bibliotecas**: Paho MQTT (WebSockets), Chart.js
- **Protocolo**: MQTT over WebSockets (WSS)

## 📁 Estrutura do Projeto

```
Sistemas-Embarcados/
├── failsafe-esp32/          # Firmware ESP32
│   ├── src/
│   │   └── failsafe.cpp    # Código principal
│   └── platformio.ini       # Configuração PlatformIO
├── client/                   # Interface web
│   ├── index.html           # Página principal
│   ├── js/
│   │   └── app.js           # Lógica MQTT e UI
│   └── css/
│       └── style.css        # Estilos customizados
└── README.md
```

## 🔧 Funcionalidades

### ESP32
- Leitura contínua dos sensores MQ2 e DHT11
- Publicação de dados via MQTT em tópicos dedicados
- Controle de LEDs indicadores baseado nos níveis de fumaça
- Gerenciamento de conexão WiFi e MQTT com reconexão automática
- Execução multi-tarefa com FreeRTOS

### Interface Web
- Dashboard em tempo real com visualização dos dados dos sensores
- Indicadores visuais de status (normal, alerta, crítico)
- Conexão MQTT via WebSockets com reconexão automática
- Interface responsiva e moderna

## 📡 Tópicos MQTT

- `failsafe/mq2` - Dados do sensor MQ2 (ppm)
- `failsafe/dht11/t` - Temperatura do DHT11 (°C)
- `failsafe/dht11/h` - Umidade do DHT11 (%)

## 🚀 Como Usar

### Configuração do ESP32

1. Instale o PlatformIO
2. Abra o projeto em `failsafe-esp32/`
3. Configure as credenciais WiFi e MQTT no código:
   ```cpp
   const char* ssid = "SEU_WIFI";
   const char* password = "SUA_SENHA";
   const char* broker_ssid = "SEU_BROKER";
   const char* mqtt_username = "SEU_USUARIO";
   const char* mqtt_password = "SUA_SENHA";
   ```
4. Compile e faça upload para o ESP32

### Executar Interface Web

1. Abra `client/index.html` em um navegador moderno
2. A interface se conectará automaticamente ao broker MQTT configurado
3. Os dados serão exibidos em tempo real conforme recebidos

## 👥 Contribuidores & Github

|   Contribuidor | Github |
| ------ | ------ |
| **JOÃO PEDRO ARAÚJO**                    |   @joaopedrofds |  
| **MARIA JULIA PESSOA CUNHA**             |   @             |  
| **ROBERTO ALMEIDA BURLAMAQUE CATUNDA**   |   @grutex       |  
| **VINÍCIUS DE MELO VENTURA**             |   @vinivent     |  

## 📝 Licença

Este projeto foi desenvolvido para fins acadêmicos.
