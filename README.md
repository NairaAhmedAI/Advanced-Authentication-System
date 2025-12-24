# 🔒 Advanced 2FA Biometric & RFID Access Control System

This project implements a high-security **Two-Factor Authentication (2FA)** system using **Arduino**. It integrates contactless **RFID** technology with **Biometric Fingerprint** scanning to provide a robust security solution for doors, safes, and restricted areas.

---

## 🚀 Overview
The system evolved from a simple password-based entry to a more advanced architecture using an RFID reader and an I2C LCD screen. Access is only granted if the user possesses a registered RFID tag **AND** provides a matching fingerprint.

### 🛠️ Hardware Components
* **Arduino UNO**: Main control unit.
* **R307 Fingerprint Sensor**: Biometric identity verification.
* **RC522 RFID Reader**: Contactless card identification.
* **I2C LCD (16x2)**: User interface and real-time instructions.
* **Buzzer & LEDs**: Audio-visual feedback for system status.
  ![Circuit Diagram](<img width="1612" height="697" alt="Screenshot 2025-12-24 201859" src="https://github.com/user-attachments/assets/2b2a1573-5987-4085-ade0-cc5700128461" /
*Figure 1: System Wiring Diagram*



---

## 🔌 Circuit Connections
| Component | Arduino Pin | Connection |
| :--- | :--- | :--- |
| **Fingerprint Sensor** | D2 (TX), D3 (RX) | SoftwareSerial |
| **RFID Reader** | D10 (SS), D9 (RST), D11, D12, D13 | SPI Protocol |
| **I2C LCD Display** | A4 (SDA), A5 (SCL) | I2C Protocol |
| **Buzzer** | D8 | Digital Output |
| **Green LED** | D11 | Success Indicator |
| **Red LED** | D12 | Error Indicator |



---

## 🧠 System Logic (Algorithm)
1.  **Standby**: System displays "Scan RFID Card".
2.  **RFID Verification**: Reads the card's UID. If it's the **Admin Card**, it opens the management menu (Enroll/Delete).
3.  **Fingerprint Prompt**: If a valid user card is scanned, it asks for a fingerprint scan.
4.  **Final Authentication**: If the fingerprint matches the database, the **Green LED** lights up, a success tone plays, and access is granted.
5.  **Security Feature**: An automatic 30-second timeout resets the system from Admin Mode to Standby for security.

---

## 📺 Project Demo
The system is built inside a compact black enclosure for portability and protection.
* **Admin Mode**: Handled via Serial Monitor for user management.
* **User Mode**: Fully standalone via the LCD and sensors.


---

## 🌟 Innovation & Future Work
* **Iteration**: Successfully replaced the numeric keypad with an RFID reader for better durability and security.
* **Future Improvement 1 (IoT)**: Adding a Wi-Fi module (ESP8266) to send access alerts to a mobile app.
* **Future Improvement 2 (Actuator)**: Connecting a Solenoid lock or Servo motor to physically unlock a door.
* **Future Improvement 3 (Logging)**: Storing entry logs on an SD card with timestamps.

---

## 👥 Team Members
* Naira Gamal
* Basma Sameh
* Mariam Wael
* Nourhan Khalid
* Shaimaa Mohi
* Yasmin Abdelaziz

**Supervised by:** Dr. Lamiaa Said & Eng. Hossam Eldin Abdelhamed.
