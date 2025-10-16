# TriMic Acoustic Radar Node – Raspberry Pi 5

## 🔎 Overview
This project implements an **acoustic radar node** based on **three industrial-grade microphones** connected to a **Raspberry Pi 5**.  
Each node captures sound intensity levels (in dB) from its surroundings and transmits processed data via **LoRa wireless communication** to a **master station**, which generates a distributed acoustic map from multiple radar nodes.

---

## ⚙️ Key Technologies
- **Raspberry Pi 5** for local signal acquisition and processing  
- **Industrial RS485 sound sensors** (10–30V DC) for noise level measurement  
- **LoRa SX127x communication modules** for long-range, low-power data transmission  
- **Python 3** for data handling, event detection, and local visualization  
- **Matplotlib & NumPy** for real-time plotting and numerical analysis  

---

## 🧠 System Function
1. Each node continuously reads sound intensity from three microphones.  
2. When a sound exceeds a configurable threshold, the node estimates **relative direction and timing differences**.  
3. The processed data is transmitted via **LoRa** to a central **master unit**, which aggregates information from several nodes.  
4. The master system renders a **2D acoustic map**, highlighting the estimated positions of detected events.  

--

## 📡 Master System
The master LoRa receiver collects data from several acoustic nodes and performs:
- Event localization based on triangulation between nodes  
- Visualization of the **acoustic field map**  
- Optional integration with machine learning algorithms for sound classification  

---

## 🧰 Technologies Used
- **Python 3.11+**
- **MinimalModbus / PySerial**
- **NumPy**
- **Matplotlib**
- **LoRa (SX127x / E32 / RFM9x)**
- **RS485 communication (via USB adapters)**
