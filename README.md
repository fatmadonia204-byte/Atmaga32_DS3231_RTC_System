# ATmega32 DS3231 Real-Time Clock (RTC) System

A high-precision Real-Time Clock (RTC) system implemented using the **ATmega32** microcontroller and **DS3231 RTC module** over the **I2C / TWI** communication protocol, developed in **Embedded C**.

---

## 📌 System Architecture & Features
* **Precise Timekeeping:** Displays hours, minutes, and seconds accurately using the DS3231 hardware module.
* **I2C Hardware Interfacing:** Low-level TWI/I2C MCAL driver developed to communicate with the RTC module.
* **User Display Interface:** Real-time clock updates rendered on a Character LCD via HAL drivers.
* **Layered Software Architecture:** Built following strict Embedded C software layering (MCAL, HAL, App).

---

## 🛠️ Tech Stack & Hardware Components
* **Microcontroller:** ATmega32
* **RTC Module:** DS3231 (High-Precision RTC via I2C)
* **Peripherals:** Character LCD (16x2)
* **Communication Protocol:** I2C / TWI (Two-Wire Interface)
* **Programming Language:** Embedded C
* **Simulation & Tools:** Proteus VSM, Microchip Studio / Eclipse IDE

---

## 📁 Project Structure

```text
├── app/                  # Main Application logic
├── hal/                  # Hardware Abstraction Layer (LCD, DS3231)
├── mcal/                 # Microcontroller Abstraction Layer (DIO, TWI/I2C)
├── lib/                  # Standard types and bit math macros
├── main.c                # System Entry point
