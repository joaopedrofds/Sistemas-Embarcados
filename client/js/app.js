// Configurações
let apiUrl = localStorage.getItem("apiUrl") || "http://192.168.1.100:1337";
let updateInterval =
  parseInt(localStorage.getItem("updateInterval") || "2") * 1000;
let intervalId = null;

// Dados do gráfico
const chartData = {
  labels: [],
  pm25Data: [],
  pm10Data: [],
};
const maxDataPoints = 20;

// Inicializar gráfico
const ctx = document.getElementById("chartPM").getContext("2d");
const chart = new Chart(ctx, {
  type: "line",
  data: {
    labels: chartData.labels,
    datasets: [
      {
        label: "PM 2.5",
        data: chartData.pm25Data,
        borderColor: "rgb(59, 130, 246)",
        backgroundColor: "rgba(59, 130, 246, 0.1)",
        tension: 0.4,
        fill: true,
      },
      {
        label: "PM 10",
        data: chartData.pm10Data,
        borderColor: "rgb(168, 85, 247)",
        backgroundColor: "rgba(168, 85, 247, 0.1)",
        tension: 0.4,
        fill: true,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: true,
    plugins: {
      legend: {
        display: true,
        position: "top",
      },
    },
    scales: {
      y: {
        beginAtZero: true,
        title: {
          display: true,
          text: "µg/m³",
        },
      },
      x: {
        title: {
          display: true,
          text: "Tempo",
        },
      },
    },
  },
});

// Carregar configurações
document.getElementById("apiUrl").value = apiUrl;
document.getElementById("updateInterval").value = updateInterval / 1000;

// Função para determinar qualidade do ar
function getQuality(pm25) {
  if (pm25 <= 12)
    return { class: "bg-green-100 text-green-800", text: "✓ Boa" };
  if (pm25 <= 35)
    return { class: "bg-yellow-100 text-yellow-800", text: "⚠ Moderada" };
  if (pm25 <= 55)
    return { class: "bg-orange-100 text-orange-800", text: "⚠ Ruim" };
  return { class: "bg-red-100 text-red-800", text: "✕ Péssima" };
}

// Atualizar dados
async function updateData() {
  try {
    const response = await fetch(`${apiUrl}/data`);
    const data = await response.json();

    // Atualizar valores
    document.getElementById("pm25").textContent = data.pm25.toFixed(1);
    document.getElementById("pm10").textContent = data.pm10.toFixed(1);
    document.getElementById("temp").textContent = data.temperature.toFixed(1);
    document.getElementById("humidity").textContent = data.humidity.toFixed(0);

    // Atualizar qualidade
    const pm25Quality = getQuality(data.pm25);
    const pm25Elem = document.getElementById("pm25Quality");
    pm25Elem.className = `px-4 py-2 rounded-lg text-center font-semibold text-sm ${pm25Quality.class}`;
    pm25Elem.textContent = pm25Quality.text;

    const pm10Quality = getQuality(data.pm10);
    const pm10Elem = document.getElementById("pm10Quality");
    pm10Elem.className = `px-4 py-2 rounded-lg text-center font-semibold text-sm ${pm10Quality.class}`;
    pm10Elem.textContent = pm10Quality.text;

    // Atualizar status
    document.getElementById("statusDot").className =
      "w-3 h-3 bg-green-400 rounded-full pulse-online";
    document.getElementById("statusText").textContent = "Online";

    // Atualizar timestamp
    document.getElementById("lastUpdate").textContent =
      new Date().toLocaleTimeString("pt-BR");

    // Atualizar gráfico
    const time = new Date().toLocaleTimeString("pt-BR", {
      hour: "2-digit",
      minute: "2-digit",
    });
    chartData.labels.push(time);
    chartData.pm25Data.push(data.pm25);
    chartData.pm10Data.push(data.pm10);

    // Limitar pontos do gráfico
    if (chartData.labels.length > maxDataPoints) {
      chartData.labels.shift();
      chartData.pm25Data.shift();
      chartData.pm10Data.shift();
    }

    chart.update();
  } catch (error) {
    console.error("Erro ao buscar dados:", error);
    document.getElementById("statusDot").className =
      "w-3 h-3 bg-red-400 rounded-full";
    document.getElementById("statusText").textContent = "Erro de conexão";
  }
}

// Aplicar configurações
function applySettings() {
  apiUrl = document.getElementById("apiUrl").value;
  updateInterval =
    parseInt(document.getElementById("updateInterval").value) * 1000;

  localStorage.setItem("apiUrl", apiUrl);
  localStorage.setItem("updateInterval", updateInterval / 1000);

  // Reiniciar intervalo
  if (intervalId) clearInterval(intervalId);
  intervalId = setInterval(updateData, updateInterval);

  alert("✓ Configurações aplicadas com sucesso!");
}

// Iniciar atualização automática
intervalId = setInterval(updateData, updateInterval);
updateData();
