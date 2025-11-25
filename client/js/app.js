// --- CONFIGURAÇÕES DO HIVEMQ (MESMAS DO ARDUINO) ---
const mqttHost = "a75536483f004b7d9dc969b95d17de8d.s1.eu.hivemq.cloud"; 
const mqttPort = 8884; // Porta WSS (Secure WebSockets)
const mqttUser = "failsafe";
const mqttPass = "Admin12345";

// Tópicos
const TOPIC_MQ2 = "failsafe/mq2";
const TOPIC_TEMP = "failsafe/dht11/t";
const TOPIC_HUMI = "failsafe/dht11/h";

// --- INICIALIZAÇÃO DO CLIENTE MQTT ---
const client = new Paho.MQTT.Client(mqttHost, mqttPort, "web_client_" + new Date().getTime());

// Definindo os callbacks
client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

// Iniciar conexão ao carregar
connectToBroker();

function connectToBroker() {
    updateStatus("connecting");
    
    client.connect({
        useSSL: true, // WSS requer SSL
        userName: mqttUser,
        password: mqttPass,
        onSuccess: onConnect,
        onFailure: onFailure,
        keepAliveInterval: 60
    });
}

function onConnect() {
    console.log("Conectado ao HiveMQ!");
    updateStatus("connected");

    // Inscrever nos tópicos para receber os dados
    client.subscribe(TOPIC_MQ2);
    client.subscribe(TOPIC_TEMP);
    client.subscribe(TOPIC_HUMI);
}

function onFailure(responseObject) {
    console.log("Falha na conexão: " + responseObject.errorMessage);
    updateStatus("error");
    // Tentar reconectar em 5 segundos
    setTimeout(connectToBroker, 5000);
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Conexão perdida: " + responseObject.errorMessage);
        updateStatus("error");
        setTimeout(connectToBroker, 5000);
    }
}

// --- CHEGADA DE MENSAGENS ---
function onMessageArrived(message) {
    const topic = message.destinationName;
    const payload = message.payloadString;
    
    //console.log(`Msg recebida [${topic}]: ${payload}`); // Debug se precisar
    updateTimestamp();

    // Roteamento
    if (topic === TOPIC_MQ2) {
        updateSmokeUI(parseFloat(payload));
    } else if (topic === TOPIC_TEMP) {
        updateTempUI(parseFloat(payload));
    } else if (topic === TOPIC_HUMI) {
        updateHumidityUI(parseFloat(payload));
    }
}

// --- FUNÇÕES DE INTERFACE (UI) ---

function updateSmokeUI(value) {
    const elValue = document.getElementById("smoke");
    const elStatus = document.getElementById("smokeStatus");
    const elBadge = document.getElementById("smokeQuality");

    if(isNaN(value)) return;

    elValue.innerText = value.toFixed(0);

    // Lógica visual baseada no nível de fumaça
    if (value < 1050) {
        elStatus.innerText = "Normal";
        elStatus.className = "text-sm font-medium text-green-400";
        elBadge.innerText = "Seguro";
        elBadge.className = "px-3 py-1 rounded-full text-xs font-medium bg-green-500/20 text-green-400";
    } else if (value < 2048) {
        elStatus.innerText = "Atenção";
        elStatus.className = "text-sm font-medium text-yellow-400";
        elBadge.innerText = "Alerta";
        elBadge.className = "px-3 py-1 rounded-full text-xs font-medium bg-yellow-500/20 text-yellow-400";
    } else {
        elStatus.innerText = "PERIGO";
        elStatus.className = "text-sm font-medium text-red-500 font-bold";
        elBadge.innerText = "Crítico";
        elBadge.className = "px-3 py-1 rounded-full text-xs font-medium bg-red-500/20 text-red-400 animate-pulse";
    }
}

function updateTempUI(value) {
    const elValue = document.getElementById("temp");
    const elFeels = document.getElementById("tempFeels");
    
    if(isNaN(value)) return;

    elValue.innerText = value.toFixed(1) + "°C";

    // Lógica simples de sensação térmica
    let sensation = "Confortável";
    if(value < 18) sensation = "Frio";
    else if(value > 28) sensation = "Quente";
    else if(value > 32) sensation = "Muito Quente";
    
    elFeels.innerText = sensation;
}

function updateHumidityUI(value) {
    const elValue = document.getElementById("humidity");
    const elLevel = document.getElementById("humidityLevel");

    if(isNaN(value)) return;

    elValue.innerText = value.toFixed(0) + "%";

    let level = "Confortável";
    if(value < 30) level = "Seco";
    else if(value > 70) level = "Úmido";

    elLevel.innerText = level;
}

// Atualiza o relógio da "Última Mensagem"
function updateTimestamp() {
    const now = new Date();
    document.getElementById("lastUpdate").innerText = now.toLocaleTimeString("pt-BR");
}

// Atualiza o indicador de status de conexão (bolinha colorida)
function updateStatus(state) {
    const dot = document.getElementById("statusDot");
    const text = document.getElementById("statusText");

    if (state === "connected") {
        dot.className = "w-2 h-2 bg-green-400 rounded-full pulse-online";
        text.innerText = "Conectado ao HiveMQ";
        text.className = "text-green-400 text-sm";
    } else if (state === "connecting") {
        dot.className = "w-2 h-2 bg-yellow-400 rounded-full pulse-connecting";
        text.innerText = "Conectando...";
        text.className = "text-yellow-400 text-sm";
    } else {
        dot.className = "w-2 h-2 bg-red-500 rounded-full pulse-offline";
        text.innerText = "Desconectado";
        text.className = "text-red-500 text-sm";
    }
}