import express from "express";
import { GoogleGenAI } from "@google/genai";
import crypto from "crypto";

const app = express();
app.use(express.json());
app.use(express.text());

const ai = new GoogleGenAI({ apiKey: process.env.GEMINI_API_KEY });
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN;
const TELEGRAM_CHAT_ID = process.env.TELEGRAM_CHAT_ID;
const ESP_API_KEY = process.env.ESP_API_KEY;

const rateLimitMap = new Map();
const RATE_LIMIT_WINDOW_MS = 120_000;
const RATE_LIMIT_MAX = 20;

function isRateLimited(ip) {
  const now = Date.now();
  const entry = rateLimitMap.get(ip) || { count: 0, start: now };

  if (now - entry.start > RATE_LIMIT_WINDOW_MS) {
    rateLimitMap.set(ip, { count: 1, start: now });
    return false;
  }
  entry.count++;
  rateLimitMap.set(ip, entry);
  return entry.count > RATE_LIMIT_MAX;
}
function espAuthGateway(req, res, next) {
  const clientIp = req.headers["x-forwarded-for"] || req.socket.remoteAddress;
  if (isRateLimited(clientIp)) {
    console.warn(`[GATEWAY] Rate limited — IP: ${clientIp}`);
    return res.status(429).json({ error: "Too many requests" });
  }
  const providedKey = req.headers["x-esp-key"];
  if (!providedKey) {
    console.warn(`[GATEWAY] Missing key — IP: ${clientIp}`);
    return res.status(401).json({ error: "Unauthorized: missing key" });
  }
  const expectedKey = ESP_API_KEY || "";
  let keysMatch = false;
  try {
    keysMatch =
      providedKey.length === expectedKey.length &&
      crypto.timingSafeEqual(
        Buffer.from(providedKey),
        Buffer.from(expectedKey)
      );
  } catch {
    keysMatch = false;
  }

  if (!keysMatch) {
    console.warn(`[GATEWAY] Wrong key — IP: ${clientIp}`);
    return res.status(403).json({ error: "Forbidden: invalid key" });
  }

  console.log(`[GATEWAY] Authenticated — IP: ${clientIp}`);
  next();
}

function markdownToTelegramHTML(text) {
  return (
    text
      // bold **text**
      .replace(/\*\*(.*?)\*\*/g, "<b>$1</b>")

      // headings ### → bold
      .replace(/^### (.*$)/gim, "<b>$1</b>")
      .replace(/^## (.*$)/gim, "<b>$1</b>")
      .replace(/^# (.*$)/gim, "<b>$1</b>")

      // bullet points * → •
      .replace(/^\* (.*$)/gim, "• $1")

      // horizontal line ---
      .replace(/---/g, "──────────────")

      // fix double line breaks
      .replace(/\n{3,}/g, "\n\n")
  );
}

async function sendToTelegram(text) {
  const url = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`;

  // limit message size (Telegram max = 4096)
  if (text.length > 4000) {
    text = text.substring(0, 4000);
  }

  const res = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      chat_id: TELEGRAM_CHAT_ID,
      text: text,
      parse_mode: "HTML",
    }),
  });

  if (!res.ok) throw new Error(`Telegram error: ${await res.text()}`);
  return res.json();
}
async function processWithGemini(inputString) {
  const response = await ai.models.generateContent({
    model: "gemma-4-26b-a4b-it",
    contents: inputString,
  });
  return response.text;
}
app.post("/process", espAuthGateway, async (req, res) => {
  try {
    let inputString =
      typeof req.body === "string"
        ? req.body
        : req.body?.data || req.body?.message || JSON.stringify(req.body);

    if (!inputString || inputString.trim() === "") {
      return res.status(400).json({ error: "Empty input string" });
    }

    console.log(`[ESP INPUT] ${inputString}`);
    const aiResponse = await processWithGemini(inputString);
    const formattedText = markdownToTelegramHTML(aiResponse);
    console.log(`[GEMINI]    ${aiResponse}`);

    const msg = `Farm report of Today \n \n${formattedText}`;
    await sendToTelegram(msg);
    console.log(`[TELEGRAM]  Sent`);
    res.json({ success: true, response: aiResponse });
  } catch (err) {
    console.error("[ERROR]", err.message);
    res.status(500).json({ error: err.message });
  }
});

app.get("/", (_req, res) =>
  res.send("ESP → Gemini → Telegram server is running ")
);
app.use((_req, res) => res.status(404).json({ error: "Not found" }));

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));
