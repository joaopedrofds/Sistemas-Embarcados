// Simple configuration
let apiUrl = localStorage.getItem("apiUrl") || "http://192.168.1.100:1337";
let updateInterval =
  parseInt(localStorage.getItem("updateInterval") || "2") * 1000;
let intervalId = null;

// Load settings
document.getElementById("apiUrl").value = apiUrl;
document.getElementById("updateInterval").value = updateInterval / 1000;

// Utility functions
function getSmokeStatus(smokeValue) {
  if (smokeValue < 1050)
    return {
      status: "Normal",
      color: "text-green-400",
      progress: (smokeValue / 4095) * 100,
    };
  if (smokeValue < 2047.5)
    return {
      status: "Elevado",
      color: "text-yellow-400",
      progress: (smokeValue / 4095) * 100,
    };
  return {
    status: "Alto",
    color: "text-red-400",
    progress: (smokeValue / 4095) * 100,
  };
}

function getTempFeelsLike(temp) {
  if (temp < 18) return "Frio";
  if (temp < 24) return "Confortável";
  if (temp < 28) return "Quente";
  return "Muito Quente";
}

function getHumidityLevel(humidity) {
  if (humidity < 30) return "Seco";
  if (humidity < 60) return "Confortável";
  if (humidity < 70) return "Moderado";
  return "Úmido";
}

// Update data function
async function updateData() {
  try {
    const response = await fetch(`${apiUrl}/data`);
    const data = await response.json();

    // Update temperature
    const temp = data.temperature || 0;
    document.getElementById("temp").textContent = temp.toFixed(1) + "°C";
    document.getElementById("tempFeels").textContent = getTempFeelsLike(temp);

    // Update smoke
    const smoke = data.smoke || 0;
    document.getElementById("smoke").textContent = smoke.toFixed(0);
    const smokeStatus = getSmokeStatus(smoke);
    document.getElementById("smokeStatus").textContent = smokeStatus.status;
    document.getElementById(
      "smokeStatus"
    ).className = `text-sm font-medium ${smokeStatus.color}`;
    const smokeQuality = document.getElementById("smokeQuality");
    if (smokeQuality) {
      smokeQuality.textContent = smokeStatus.status;
      smokeQuality.className = `px-3 py-1 rounded-full text-xs font-medium ${
        smokeStatus.status === "Normal"
          ? "bg-green-500/20 text-green-400"
          : smokeStatus.status === "Elevado"
          ? "bg-yellow-500/20 text-yellow-400"
          : "bg-red-500/20 text-red-400"
      }`;
    }

    // Update humidity
    const humidity = data.humidity || 0;
    document.getElementById("humidity").textContent = humidity.toFixed(0) + "%";
    document.getElementById("humidityLevel").textContent =
      getHumidityLevel(humidity);

    // Update status
    const statusDot = document.querySelector("#connectionStatus .w-2");
    if (statusDot) {
      statusDot.className = "w-2 h-2 bg-green-400 rounded-full pulse-online";
    }
    document.getElementById("statusText").textContent = "Conectado";
    document.getElementById("statusText").className = "text-gray-400 text-sm";

    // Update timestamps
    const now = new Date();
    const timeString = now.toLocaleTimeString("pt-BR", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
    document.getElementById("lastUpdate").textContent = timeString;
  } catch (error) {
    console.error("Erro ao buscar dados:", error);
    const statusDot = document.querySelector("#connectionStatus .w-2");
    if (statusDot) {
      statusDot.className = "w-2 h-2 bg-red-400 rounded-full pulse-offline";
    }
    document.getElementById("statusText").textContent = "Erro de Conexão";
    document.getElementById("statusText").className = "text-red-400 text-sm";
  }
}

// Apply settings
function applySettings() {
  apiUrl = document.getElementById("apiUrl").value;
  updateInterval =
    parseInt(document.getElementById("updateInterval").value) * 1000;

  localStorage.setItem("apiUrl", apiUrl);
  localStorage.setItem("updateInterval", updateInterval / 1000);

  // Restart interval
  if (intervalId) clearInterval(intervalId);
  intervalId = setInterval(updateData, updateInterval);

  // Show confirmation
  const button = event.target;
  const originalText = button.textContent;
  button.textContent = "✓ Configurações Salvas!";
  button.classList.remove("bg-blue-500", "hover:bg-blue-600");
  button.classList.add("bg-green-500", "hover:bg-green-600");

  setTimeout(() => {
    button.textContent = originalText;
    button.classList.remove("bg-green-500", "hover:bg-green-600");
    button.classList.add("bg-blue-500", "hover:bg-blue-600");
  }, 2000);
}

// Initialize
intervalId = setInterval(updateData, updateInterval);
updateData();
