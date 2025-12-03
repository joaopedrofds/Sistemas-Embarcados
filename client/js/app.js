// --- CONFIGURAÇÕES DO HIVEMQ (MESMAS DO ARDUINO) ---
const mqttHost = "a75536483f004b7d9dc969b95d17de8d.s1.eu.hivemq.cloud";
const mqttPort = 8884; // Porta WSS (Secure WebSockets)
const mqttUser = "failsafe";
const mqttPass = "Admin12345";

// Tópicos
const TOPIC_MQ2 = "failsafe/mq2";
const TOPIC_TEMP = "failsafe/dht11/t";
const TOPIC_HUMI = "failsafe/dht11/h";

// --- CONFIGURAÇÕES DE HISTÓRICO ---
const MAX_DATA_POINTS = 100; // Máximo de pontos no gráfico
const STORAGE_KEY = "failsafe_history";

// Arrays para armazenar histórico em memória
let smokeHistory = [];
let tempHistory = [];
let humidityHistory = [];
let timeLabels = [];

// Objetos dos gráficos
let smokeChart = null;
let tempChart = null;
let humidityChart = null;

// --- INICIALIZAÇÃO DO CLIENTE MQTT ---
const client = new Paho.MQTT.Client(
  mqttHost,
  mqttPort,
  "web_client_" + new Date().getTime()
);

// Definindo os callbacks
client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

// Iniciar conexão ao carregar
loadHistoryFromStorage();
// Aguardar um pouco para garantir que o DOM está pronto
setTimeout(() => {
  initializeCharts();
  connectToBroker();
}, 100);

function connectToBroker() {
  updateStatus("connecting");

  client.connect({
    useSSL: true, // WSS requer SSL
    userName: mqttUser,
    password: mqttPass,
    onSuccess: onConnect,
    onFailure: onFailure,
    keepAliveInterval: 60,
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
    const value = parseFloat(payload);
    if (!isNaN(value)) {
      addDataPoint("smoke", value);
      updateSmokeUI(value);
      // Usar uma cópia dos arrays para garantir que o gráfico receba os dados atualizados
      updateChart(smokeChart, [...smokeHistory], [...timeLabels]);
    }
  } else if (topic === TOPIC_TEMP) {
    const value = parseFloat(payload);
    if (!isNaN(value)) {
      addDataPoint("temp", value);
      updateTempUI(value);
      updateChart(tempChart, [...tempHistory], [...timeLabels]);
    }
  } else if (topic === TOPIC_HUMI) {
    const value = parseFloat(payload);
    if (!isNaN(value)) {
      addDataPoint("humidity", value);
      updateHumidityUI(value);
      updateChart(humidityChart, [...humidityHistory], [...timeLabels]);
    }
  }

  // Salvar histórico periodicamente
  if (
    (smokeHistory.length + tempHistory.length + humidityHistory.length) % 30 ===
    0
  ) {
    saveHistoryToStorage();
  }
}

// --- FUNÇÕES DE HISTÓRICO E ARMAZENAMENTO ---

function loadHistoryFromStorage() {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored) {
      const data = JSON.parse(stored);
      smokeHistory = data.smoke || [];
      tempHistory = data.temp || [];
      humidityHistory = data.humidity || [];
      timeLabels = data.labels || [];

      // Garantir que todos os arrays tenham o mesmo tamanho
      const maxLength = Math.max(
        smokeHistory.length,
        tempHistory.length,
        humidityHistory.length,
        timeLabels.length
      );

      // Preencher arrays menores com null para manter sincronização
      while (smokeHistory.length < maxLength) smokeHistory.push(null);
      while (tempHistory.length < maxLength) tempHistory.push(null);
      while (humidityHistory.length < maxLength) humidityHistory.push(null);
      while (timeLabels.length < maxLength) timeLabels.push("");

      // Limitar ao máximo de pontos
      if (maxLength > MAX_DATA_POINTS) {
        const start = maxLength - MAX_DATA_POINTS;
        smokeHistory = smokeHistory.slice(start);
        tempHistory = tempHistory.slice(start);
        humidityHistory = humidityHistory.slice(start);
        timeLabels = timeLabels.slice(start);
      }
    }
  } catch (e) {
    console.error("Erro ao carregar histórico:", e);
  }
}

function saveHistoryToStorage() {
  try {
    const data = {
      smoke: smokeHistory,
      temp: tempHistory,
      humidity: humidityHistory,
      labels: timeLabels,
      timestamp: Date.now(),
    };
    localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
  } catch (e) {
    console.error("Erro ao salvar histórico:", e);
  }
}

function addDataPoint(sensor, value) {
  const now = new Date();
  const timeLabel = now.toLocaleTimeString("pt-BR", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });

  // Adicionar novo ponto ao sensor correspondente
  if (sensor === "smoke") {
    smokeHistory.push(value);
  } else if (sensor === "temp") {
    tempHistory.push(value);
  } else if (sensor === "humidity") {
    humidityHistory.push(value);
  }

  // Obter o tamanho atual do maior array
  const currentMaxLength = Math.max(
    smokeHistory.length,
    tempHistory.length,
    humidityHistory.length
  );

  // Adicionar novo label de tempo se necessário
  if (timeLabels.length < currentMaxLength) {
    timeLabels.push(timeLabel);
  }

  // Garantir que todos os arrays tenham o mesmo tamanho (preencher com null se necessário)
  // Isso mantém a sincronização entre os arrays
  while (smokeHistory.length < currentMaxLength) {
    smokeHistory.push(null);
  }
  while (tempHistory.length < currentMaxLength) {
    tempHistory.push(null);
  }
  while (humidityHistory.length < currentMaxLength) {
    humidityHistory.push(null);
  }
  while (timeLabels.length < currentMaxLength) {
    timeLabels.push(timeLabel);
  }

  // Limitar ao máximo de pontos (manter apenas os últimos N)
  if (currentMaxLength > MAX_DATA_POINTS) {
    const removeCount = currentMaxLength - MAX_DATA_POINTS;
    smokeHistory.splice(0, removeCount);
    tempHistory.splice(0, removeCount);
    humidityHistory.splice(0, removeCount);
    timeLabels.splice(0, removeCount);
  }

  // Salvar no localStorage periodicamente (a cada 10 pontos)
  if (timeLabels.length % 10 === 0) {
    saveHistoryToStorage();
  }
}

// --- INICIALIZAÇÃO DOS GRÁFICOS ---

function initializeCharts() {
  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        display: false,
      },
      tooltip: {
        backgroundColor: "rgba(30, 41, 59, 0.9)",
        titleColor: "#e2e8f0",
        bodyColor: "#e2e8f0",
        borderColor: "rgba(255, 255, 255, 0.1)",
        borderWidth: 1,
        padding: 12,
        displayColors: false,
      },
    },
    scales: {
      x: {
        grid: {
          color: "rgba(255, 255, 255, 0.05)",
          drawBorder: false,
        },
        ticks: {
          color: "#94a3b8",
          font: {
            size: 11,
          },
          maxRotation: 45,
          minRotation: 0,
        },
      },
      y: {
        grid: {
          color: "rgba(255, 255, 255, 0.05)",
          drawBorder: false,
        },
        ticks: {
          color: "#94a3b8",
          font: {
            size: 11,
          },
        },
      },
    },
  };

  // Gráfico de Fumaça
  const smokeCtx = document.getElementById("smokeChart").getContext("2d");
  smokeChart = new Chart(smokeCtx, {
    type: "line",
    data: {
      labels: timeLabels,
      datasets: [
        {
          label: "Nível de Fumaça",
          data: smokeHistory,
          borderColor: "#ef4444",
          backgroundColor: "rgba(239, 68, 68, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.4,
          pointRadius: 0,
          pointHoverRadius: 4,
          pointHoverBackgroundColor: "#ef4444",
          pointHoverBorderColor: "#fff",
          pointHoverBorderWidth: 2,
          spanGaps: false,
        },
      ],
    },
    options: {
      ...chartOptions,
      scales: {
        ...chartOptions.scales,
        y: {
          ...chartOptions.scales.y,
          beginAtZero: false,
        },
      },
    },
  });

  // Gráfico de Temperatura
  const tempCtx = document.getElementById("tempChart").getContext("2d");
  tempChart = new Chart(tempCtx, {
    type: "line",
    data: {
      labels: timeLabels,
      datasets: [
        {
          label: "Temperatura",
          data: tempHistory,
          borderColor: "#3b82f6",
          backgroundColor: "rgba(59, 130, 246, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.4,
          pointRadius: 0,
          pointHoverRadius: 4,
          pointHoverBackgroundColor: "#3b82f6",
          pointHoverBorderColor: "#fff",
          pointHoverBorderWidth: 2,
          spanGaps: false,
        },
      ],
    },
    options: {
      ...chartOptions,
      scales: {
        ...chartOptions.scales,
        y: {
          ...chartOptions.scales.y,
          beginAtZero: false,
        },
      },
    },
  });

  // Gráfico de Umidade
  const humidityCtx = document.getElementById("humidityChart").getContext("2d");
  humidityChart = new Chart(humidityCtx, {
    type: "line",
    data: {
      labels: timeLabels,
      datasets: [
        {
          label: "Umidade",
          data: humidityHistory,
          borderColor: "#06b6d4",
          backgroundColor: "rgba(6, 182, 212, 0.1)",
          borderWidth: 2,
          fill: true,
          tension: 0.4,
          pointRadius: 0,
          pointHoverRadius: 4,
          pointHoverBackgroundColor: "#06b6d4",
          pointHoverBorderColor: "#fff",
          pointHoverBorderWidth: 2,
          spanGaps: false,
        },
      ],
    },
    options: {
      ...chartOptions,
      scales: {
        ...chartOptions.scales,
        y: {
          ...chartOptions.scales.y,
          beginAtZero: false,
          max: 100,
        },
      },
    },
  });

  // Atualizar gráficos com dados carregados do histórico
  if (timeLabels.length > 0) {
    updateChart(smokeChart, smokeHistory, timeLabels);
    updateChart(tempChart, tempHistory, timeLabels);
    updateChart(humidityChart, humidityHistory, timeLabels);
  }
}

function updateChart(chart, data, labels) {
  if (!chart) {
    console.warn("Gráfico não inicializado");
    return;
  }

  if (!data || !labels || data.length === 0 || labels.length === 0) {
    console.warn("Dados ou labels vazios", {
      dataLength: data?.length,
      labelsLength: labels?.length,
    });
    return;
  }

  try {
    // Garantir que data e labels tenham o mesmo tamanho
    const minLength = Math.min(data.length, labels.length);
    const trimmedData = data.slice(0, minLength);
    const trimmedLabels = labels.slice(0, minLength);

    // Converter null para undefined (Chart.js lida melhor com undefined)
    const chartData = trimmedData.map((val) =>
      val !== null && val !== undefined ? val : undefined
    );

    chart.data.labels = trimmedLabels;
    chart.data.datasets[0].data = chartData;
    chart.update("none"); // 'none' para animação suave sem delay
  } catch (error) {
    console.error("Erro ao atualizar gráfico:", error);
  }
}

// --- FUNÇÕES DE INTERFACE (UI) ---

function updateSmokeUI(value) {
  const elValue = document.getElementById("smoke");
  const elStatus = document.getElementById("smokeStatus");
  const elBadge = document.getElementById("smokeQuality");

  if (isNaN(value)) return;

  elValue.innerText = value.toFixed(0);

  // Lógica visual baseada no nível de fumaça
  if (value < 1050) {
    elStatus.innerText = "Normal";
    elStatus.className = "text-sm font-medium text-green-400";
    elBadge.innerText = "Seguro";
    elBadge.className =
      "px-3 py-1 rounded-full text-xs font-medium bg-green-500/20 text-green-400";
  } else if (value < 2048) {
    elStatus.innerText = "Atenção";
    elStatus.className = "text-sm font-medium text-yellow-400";
    elBadge.innerText = "Alerta";
    elBadge.className =
      "px-3 py-1 rounded-full text-xs font-medium bg-yellow-500/20 text-yellow-400";
  } else {
    elStatus.innerText = "PERIGO";
    elStatus.className = "text-sm font-medium text-red-500 font-bold";
    elBadge.innerText = "Crítico";
    elBadge.className =
      "px-3 py-1 rounded-full text-xs font-medium bg-red-500/20 text-red-400 animate-pulse";
  }
}

function updateTempUI(value) {
  const elValue = document.getElementById("temp");
  const elFeels = document.getElementById("tempFeels");

  if (isNaN(value)) return;

  elValue.innerText = value.toFixed(1) + "°C";

  // Lógica simples de sensação térmica
  let sensation = "Confortável";
  if (value < 18) sensation = "Frio";
  else if (value > 28) sensation = "Quente";
  else if (value > 32) sensation = "Muito Quente";

  elFeels.innerText = sensation;
}

function updateHumidityUI(value) {
  const elValue = document.getElementById("humidity");
  const elLevel = document.getElementById("humidityLevel");

  if (isNaN(value)) return;

  elValue.innerText = value.toFixed(0) + "%";

  let level = "Confortável";
  if (value < 30) level = "Seco";
  else if (value > 70) level = "Úmido";

  elLevel.innerText = level;
}

// Atualiza o relógio da "Última Mensagem"
function updateTimestamp() {
  const now = new Date();
  document.getElementById("lastUpdate").innerText =
    now.toLocaleTimeString("pt-BR");
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
