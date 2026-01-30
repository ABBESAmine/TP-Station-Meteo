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

const wsUrlInput = $("#wsUrl");
const connectBtn = $("#connectBtn");

const STORAGE_KEY = "tp-station-meteo-ui:v3";

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
  wsUrl: "ws://localhost:8080",
  ws: null,
  connected: false,
  unit: "C",
  lastTempC: null,
  lastHum: null
};

const loadState = () => {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}");
    if (saved.wsUrl) state.wsUrl = String(saved.wsUrl);
  } catch {}
};

const saveState = () => {
  localStorage.setItem(
    STORAGE_KEY,
    JSON.stringify({
      wsUrl: state.wsUrl
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

const init = () => {
  loadState();

  wsUrlInput.value = state.wsUrl;

  connectBtn.addEventListener("click", () => {
    if (state.ws) disconnectWs();
    else connectWs();
  });

  render();
  setStatus("warn", "Hors ligne");
};

init();
