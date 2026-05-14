
For System Integrators & End Users

---

# Table of Contents

1. System Overview  
2. What's in the Box  
3. Quick Start Guide  
4. Hardware Setup  
5. Connecting to the Web Portal  
6. Configuration Walkthrough  
7. Using the Dashboard  
8. Pico Integration Guide  
9. LED Status Indicators  
10. Daily Operation  
11. Troubleshooting  
12. Technical Specifications  
13. Support & Contact  

---

# 1. System Overview

The **RAK3112 LoRaWAN Bridge** is a plug-and-play device that:

- Receives data from The Things Network (TTN) or any LoRaWAN network
- Forwards commands instantly to your Raspberry Pi Pico
- Operates 24/7 in Class C mode (always listening)
- Provides a web interface for configuration and monitoring

---

## Typical Use Cases

| Application | How It Works |
|---|---|
| Remote actuator control | Send `"RELAY_ON"` from TTN → RAK3112 → Pico turns on a pump |
| Sensor polling | Request sensor reading from cloud → RAK3112 triggers Pico |
| Over-the-air updates | Send firmware commands to Pico via LoRaWAN |
| Industrial automation | Control PLCs or valves remotely |

---

## What This Device Does NOT Do

- ❌ Send sensor data from Pico to cloud (currently unidirectional)
- ❌ Work as a LoRa gateway
- ❌ Operate on batteries (requires continuous power)

---

# 3. Quick Start Guide

## 3.1 60-Second Setup

1. Connect antenna
2. Power RAK3112 via USB-C
3. Connect Pico UART
4. Join WiFi `CRE8IOT_08440001`
5. Open `http://192.168.4.1`
6. Enter TTN credentials
7. Click **Join**

---

## 3.2 Prerequisites Checklist

Before starting:

- TTN application created
- Device registered in TTN
- Gateway within range
- Pico firmware uploaded
- Jumper wires ready

---

# 4. Hardware Setup

## 4.1 Antenna Installation

- Screw antenna onto SMA connector
- Hand-tighten only
- Keep antenna vertical

---

# 5. 🌐 Connecting to Web Portal (IMPORTANT)
## 5.1 What is 192.168.4.1?

When the RAK3112 starts, it creates its own WiFi network.This network is called **Access Point Mode (AP Mode)**.It acts like a mini router so you can configure the device.

---
## 5.2 Step-by-Step Connection
### Step 1 — Connect to WiFi After powering on the device, wait ~10–15 seconds. Look for this WiFi network:

```
CRE8IOT_08440001
```
 Click **Connect** (no password required)
---
### Step 2 — Open Web PortalOpen a browser (Chrome / Edge / Safari) and go to:
```
[http://192.168.4.1](http://192.168.4.1)
```

---
## 5.3 What You Will SeeAfter opening the page, you will see:- Login screen

- Portal password input - Enter:
```
2240624
```


---

## 5.4 Inside the Dashboard

After login, you can access:

### Configuration Tab
- LoRaWAN OTAA settings
- JoinEUI / DevEUI / AppKey
- Region selection
- TX power settings

---

### Downlink Monitor
- Shows TTN command status
- Shows if device can receive commands

---

### Live Log
- Real-time system messages
- Join success/failure
- Downlink received logs

---

### Downlink History
- Last received commands
- HEX + ASCII view

---

## 5.5 How to Use the Portal

### Typical workflow:

1. Connect WiFi → `CRE8IOT_08440001`
2. Open → `http://192.168.4.1`
3. Login → `2240624`
4. Configure TTN credentials
5. Click **Save**
6. Click **Join**
7. Wait for GREEN LED solid ON

---

## 5.6 Buttons Explained

| Button | Function |
|--------|----------|
| Save | Store settings |
| Join | Connect to TTN |
| Leave | Disconnect from TTN |
| Force Uplink | Test communication |

---

# 6. Configuration Walkthrough

## TTN Credentials Needed
- JoinEUI
- DevEUI
- AppKey

Enter them into the portal exactly as shown in TTN console.

---

# 7. Using the Dashboard

## Key Indicators

| Status | Meaning |
|--------|--------|
| GREEN ON | Connected to TTN |
| GREEN OFF | Not joined |
| RED ON | Fault |

---

# 8. Pico Integration Guide

## Minimal Code

```cpp
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    Serial.println(cmd);
  }
}