---
publishDate: 2026-05-09
 
title: KhetGPT — AI-Powered Daily Agricultural Advisory System
 
excerpt: A MYOSA Mini field node that reads all onboard sensors, pipes the data to a cloud server, calls a Gemini LLM, and delivers a personalised daily farm advisory to the farmer on Telegram — automatically, every day.
 
image: khetgpt-cover.jpg
 
tags:
  - Agriculture
  - AI
  - LLM
  - IoT
  - ESP32
  - Gemini
  - Telegram
  - BMP180
  - APDS9960
  - Social Impact
---

> Every morning before sunrise, your field speaks — KhetGPT listens and tells you exactly what to do.
 
---
## Acknowledgements
 
Built by **Team DARC From TKMCE** as part of the MYOSA Make Your Own Sensors Applications initiative under the IEEE Sensors Council. Special thanks to the MYOSA team for the sensor platform and to the farming communities of Kollam whose real problems shaped this project. 

---

## Overview
 
Smallholder farmers make daily decisions — when to irrigate, when to spray, when to harvest with no real-time, hyper-local data. National weather forecasts work at district scale, not field scale. Extension bulletins arrive weekly, not daily. There is no personalised, affordable advisory system built for the individual farm.
 
**KhetGPT** solves this by turning the MYOSA Mini into an always-on field intelligence node. Every 10 minutes it reads temperature, barometric pressure, ambient light, and canopy colour from the BMP180 and APDS9960 sensors. That data is posted securely to a cloud server, which enriches it with a live weather forecast and hands the combined context to Google Gemini. The LLM generates a plain-language farm advisory — under 500 words — and delivers it to the farmer's Telegram messenger automatically.
 
No dashboard to check. No app to install. No technical knowledge needed. Just a message every morning in the farmer's own language telling them what happened in their field overnight and what to do today.

**Key features:**
 
* All MYOSA Mini sensors active and contributing to the advisory
* Gemini LLM generates crop-aware, field-specific advice — not generic tips
* Delivery via Telegram bot — works on any smartphone, no extra app
* Secure ESP-to-server communication with API key authentication and rate limiting
* Fully automated — farmer receives the advisory without touching any device
* Language-configurable — Supports All languages as per the farmers need
---
 



