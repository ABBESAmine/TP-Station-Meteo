const $ = (selector) => {
  const el = document.querySelector(selector);
  if (!el) throw new Error(`Missing element: ${selector}`);
  return el;
};

const statusText = $("#statusText");
const lastUpdate = $("#lastUpdate");

const tempValue = $("#tempValue");
const tempUnit = $("#tempUnit");
const tempMeta = $("#tempMeta");
const humValue = $("#humValue");
const humMeta = $("#humMeta");
const unitPill = $("#unitPill");
const payloadEl = $("#payload");

const modeLiveBtn = $("#modeLive");
const modeSimBtn = $("#modeSim");
const liveControls = $("#liveControls");
const simControls = $("#simControls");

const wsUrlInput = $("#wsUrl");
const connectBtn = $("#connectBtn");

const simToggleBtn = $("#simToggle");
const simUnitBtn = $("#simUnit");
const simRateInput = $("#simRate");
const simRateLabel = $("#simRateLabel");

const STORAGE_KEY = "tp-station-meteo-ui:v2";

const clamp = (n, min, max) => Math.min(max, Math.max(min, n));
const round1 = (n) => Math.round(n * 10) / 10;
const nowText = () => new Date().toLocaleString("fr-FR", { hour12: false });
const cToF = (c) => (c * 9) / 5 + 32;
const fToC = (f) => ((f - 32) * 5) / 9;

const parseUnit = (raw) => {
  if (!raw) return null;
  const s = String(raw).trim().toUpperCase();
  if (s === "C" || s === "°C") return "C";
  if (s === "F" || s === "°F") return "F";
  return null;
};

const tryParseJson = (maybeJson) => {
  if (typeof maybeJson !== "string") return null;
  try {
    return JSON.parse(maybeJson);
  } catch {
    return null;
  }
};

const normalizeMessage = (data) => {
  const root = typeof data === "string" ? tryParseJson(data) : data;
  if (!root || typeof root !== "object") return null;

  let obj = root;
  if (typeof root.payload === "string") {
    const p = tryParseJson(root.payload);
    if (p && typeof p === "object") obj = p;
  }

  const t = obj.temperature ?? obj.temp ?? obj.t;
  const h = obj.humidity ?? obj.hum ?? obj.h;
  const unit = parseUnit(obj.unit ?? obj.u ?? obj.mode);

  const temperature = Number.isFinite(Number(t)) ? Number(t) : null;
  const humidity = Number.isFinite(Number(h)) ? Number(h) : null;

  if (temperature == null && humidity == null && !unit) return null;
  return { temperature, humidity, unit, raw: obj };
};

const state = {
  mode: "live",
  wsUrl: "ws://localhost:8080",
  ws: null,
  connected: false,
  simTimer: null,
  simRunning: false,
  simRateMs: 1000,
  unit: "C",
  lastTempC: null,
  lastHum: null
};

const loadState = () => {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}");
    if (saved.wsUrl) state.wsUrl = String(saved.wsUrl);
    if (saved.mode === "live" || saved.mode === "sim") state.mode = saved.mode;
    if (Number.isFinite(saved.simRateMs)) state.simRateMs = clamp(saved.simRateMs, 500, 5000);
  } catch {}
};

const saveState = () => {
  localStorage.setItem(
    STORAGE_KEY,
    JSON.stringify({
      wsUrl: state.wsUrl,
      mode: state.mode,
      simRateMs: state.simRateMs
    })
  );
};

const setStatus = (kind, text) => {
  statusText.classList.toggle("ok", kind === "ok");
  statusText.classList.toggle("warn", kind === "warn");
  statusText.classList.toggle("bad", kind === "bad");
  statusText.textContent = text;
};

const render = () => {
  unitPill.textContent = state.unit;

  if (state.lastTempC == null) {
    tempValue.textContent = "—";
    tempUnit.textContent = "°C";
    tempMeta.textContent = "—";
  } else {
    const shown = state.unit === "C" ? state.lastTempC : cToF(state.lastTempC);
    const other = state.unit === "C" ? cToF(state.lastTempC) : state.lastTempC;
    tempValue.textContent = String(round1(shown));
    tempUnit.textContent = state.unit === "C" ? "°C" : "°F";
    tempMeta.textContent = `(${round1(other)} ${state.unit === "C" ? "°F" : "°C"})`;
  }

  if (state.lastHum == null) {
    humValue.textContent = "—";
    humMeta.textContent = "—";
  } else {
    const clamped = clamp(state.lastHum, 0, 100);
    humValue.textContent = String(round1(clamped));
    humMeta.textContent = clamped < 30 ? "sec" : clamped < 60 ? "ok" : "humide";
  }
};

const applyMessage = (m) => {
  if (!m) return;
  payloadEl.textContent = JSON.stringify(m.raw, null, 2);

  if (m.unit) state.unit = m.unit;

  if (m.temperature != null) {
    const unitForTemp = m.unit ?? state.unit;
    state.lastTempC = unitForTemp === "F" ? fToC(m.temperature) : m.temperature;
  }
  if (m.humidity != null) state.lastHum = m.humidity;

  lastUpdate.textContent = nowText();
  render();
};

const disconnectWs = () => {
  if (state.ws) {
    try {
      state.ws.close();
    } catch {}
  }
  state.ws = null;
  state.connected = false;
  connectBtn.textContent = "Connexion";
  setStatus("warn", "Hors ligne");
};

const connectWs = () => {
  disconnectWs();
  state.wsUrl = wsUrlInput.value.trim() || state.wsUrl;
  wsUrlInput.value = state.wsUrl;
  saveState();

  let ws;
  try {
    ws = new WebSocket(state.wsUrl);
  } catch {
    setStatus("bad", "URL invalide");
    return;
  }

  state.ws = ws;
  connectBtn.textContent = "Déconnexion";
  setStatus("warn", "Connexion…");

  ws.addEventListener("open", () => {
    state.connected = true;
    setStatus("ok", "Connecté");
  });

  ws.addEventListener("close", () => {
    disconnectWs();
  });

  ws.addEventListener("error", () => {
    setStatus("bad", "Erreur");
  });

  ws.addEventListener("message", (ev) => {
    const m = normalizeMessage(ev.data);
    if (m) applyMessage(m);
  });
};

const stopSim = () => {
  if (state.simTimer) clearInterval(state.simTimer);
  state.simTimer = null;
  state.simRunning = false;
  simToggleBtn.textContent = "Démarrer";
};

const randomWalk = (value, delta, min, max) => {
  const next = value + (Math.random() * 2 - 1) * delta;
  return clamp(next, min, max);
};

const simTick = () => {
  const baseTempC = state.lastTempC ?? 21;
  const baseHum = state.lastHum ?? 45;

  const tempC = randomWalk(baseTempC, 0.3, 16, 30);
  const hum = randomWalk(baseHum, 1.2, 25, 80);

  const outTemp = state.unit === "C" ? tempC : cToF(tempC);
  applyMessage({
    temperature: round1(outTemp),
    humidity: round1(hum),
    unit: state.unit,
    raw: {
      temperature: round1(outTemp),
      humidity: round1(hum),
      unit: state.unit,
      simulation: true
    }
  });
};

const startSim = () => {
  stopSim();
  disconnectWs();
  state.simRunning = true;
  simToggleBtn.textContent = "Arrêter";
  setStatus("ok", "Simulation");
  simTick();
  state.simTimer = setInterval(simTick, state.simRateMs);
};

const setMode = (mode) => {
  state.mode = mode;
  saveState();

  const live = mode === "live";
  modeLiveBtn.classList.toggle("isActive", live);
  modeSimBtn.classList.toggle("isActive", !live);
  liveControls.classList.toggle("hidden", !live);
  simControls.classList.toggle("hidden", live);

  if (live) {
    stopSim();
    setStatus(state.connected ? "ok" : "warn", state.connected ? "Connecté" : "Hors ligne");
  } else {
    disconnectWs();
    setStatus("ok", state.simRunning ? "Simulation" : "Simulation prête");
  }
};

const init = () => {
  loadState();

  wsUrlInput.value = state.wsUrl;
  simRateInput.value = String(state.simRateMs);
  simRateLabel.textContent = String(state.simRateMs);

  connectBtn.addEventListener("click", () => {
    if (state.ws) disconnectWs();
    else connectWs();
  });

  modeLiveBtn.addEventListener("click", () => setMode("live"));
  modeSimBtn.addEventListener("click", () => setMode("sim"));

  simRateInput.addEventListener("input", () => {
    state.simRateMs = clamp(Number(simRateInput.value), 500, 5000);
    simRateLabel.textContent = String(state.simRateMs);
    saveState();
    if (state.simRunning) startSim();
  });

  simToggleBtn.addEventListener("click", () => {
    if (state.simRunning) {
      stopSim();
      setStatus("ok", "Simulation prête");
    } else {
      startSim();
    }
  });

  simUnitBtn.addEventListener("click", () => {
    state.unit = state.unit === "C" ? "F" : "C";
    render();
    if (state.simRunning) simTick();
  });

  render();
  setMode(state.mode);
  setStatus("warn", "Hors ligne");
};

init();
