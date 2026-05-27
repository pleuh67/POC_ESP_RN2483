# Câblage — ESP32-S3 + RN2483 + Capteurs

## Principe

Le RN2483 communique avec l'ESP32-S3 via une liaison **UART série** à 57 600 bauds.
Tous les capteurs I2C partagent un bus unique **GPIO35 (SDA) / GPIO36 (SCL)** à 50 kHz.
Le HX711 dispose d'un bus dédié **GPIO10 / GPIO11**.

> Le RN2483 fonctionne **exclusivement en 3.3V**.
> Ne jamais connecter VDD ou les broches data en 5V.

---

## RN2483 — UART

| RN2483 (broche) | ESP32-S3 (GPIO) | Détail |
|---|---|---|
| **TX** (pin 3) | **GPIO 4** (RX du Serial1) | Données module → MCU |
| **RX** (pin 4) | **GPIO 5** (TX du Serial1) | Données MCU → module |
| **RST** (pin 5) | **GPIO 6** | Reset matériel (LOW = reset) |
| **VDD** (pin 1) | **3.3V** | Alimentation |
| **GND** (pin 2) | **GND** | Masse commune |

```
  ESP32-S3                         RN2483
  ┌─────────────────┐              ┌──────────────────┐
  │  3.3V  ─────────┼──────────────┼─ VDD  (pin 1)    │
  │  GND   ─────────┼──────────────┼─ GND  (pin 2)    │
  │  GPIO4 (RX1) ───┼──────────────┼─ TX   (pin 3)    │
  │  GPIO5 (TX1) ───┼──────────────┼─ RX   (pin 4)    │
  │  GPIO6       ───┼──────────────┼─ RST  (pin 5)    │
  └─────────────────┘              └──────────┬───────┘
                                          Antenne 868 MHz
```

---

## Bus I2C — Capteurs et OLED

Tous les périphériques ci-dessous partagent GPIO35 (SDA) et GPIO36 (SCL).

| Périphérique | Adresse I2C | Détail |
|---|---|---|
| **DS3231 RTC** | 0x68 | Horloge temps réel |
| **AT24C32 EEPROM** | 0x57 | Stockage config NodeConfig (4 Ko) |
| **BME280** | 0x76 (ou 0x77) | Température / Humidité / Pression |
| **BH1750** | 0x23 (ou 0x5C) | Luminosité — prioritaire si détecté |
| **MAX44009** | 0x4A | Luminosité — fallback si pas de BH1750 |
| **INA219** | 0x40 | Tension / Courant batterie |
| **SH1106 OLED** | 0x3C (ou 0x3D) | Afficheur 128×64 |

```
  ESP32-S3
  ┌──────────┐
  │  GPIO35  ├─── SDA ──┬── DS3231
  │  GPIO36  ├─── SCL ──┤   AT24C32
  │  3.3V    ├──────────┤   BME280
  │  GND     ├──────────┤   BH1750 / MAX44009
  └──────────┘          ├── INA219
                        └── SH1106 OLED
```

> Pull-ups 4.7 kΩ requis entre SDA/SCL et 3.3V si les modules n'en ont pas.
> La plupart des breakouts en intègrent.

---

## HX711 — Balance 1 (cellule de charge)

| HX711 | ESP32-S3 (GPIO) | Détail |
|---|---|---|
| **DOUT** | **GPIO 10** | Données série HX711 |
| **SCK**  | **GPIO 11** | Horloge HX711 |
| **VCC**  | **3.3V** | Alimentation |
| **GND**  | **GND** | Masse |

---

## Bouton poussoir — Envoi forcé LoRa

| Bouton | ESP32-S3 (GPIO) | Détail |
|---|---|---|
| Borne 1 | **GPIO 7** | Entrée avec pull-up interne activé |
| Borne 2 | **GND** | Masse |

Pas de résistance externe nécessaire : le pull-up interne de l'ESP32-S3 est activé
dans le code (`pinMode(BUTTON_PIN, INPUT_PULLUP)`).

```
  ESP32-S3
  ┌──────────┐
  │  GPIO7   ├─────[ Bouton ]─────┐
  │  (pull-up│                    │
  │  interne)│                   GND
  └──────────┘
```

---

## LED WS2812 intégrée

| Signal | GPIO | Détail |
|---|---|---|
| Data | **GPIO 48** | LED RGB WS2812 intégrée sur DevKitC-1 |

---

## Vue d'ensemble — Récapitulatif GPIO

| GPIO | Périphérique | Sens |
|---|---|---|
| 4 | RN2483 TX → ESP32 RX | Entrée |
| 5 | ESP32 TX → RN2483 RX | Sortie |
| 6 | RN2483 RST | Sortie |
| 7 | Bouton (vers GND) | Entrée |
| 10 | HX711 DOUT | Entrée |
| 11 | HX711 SCK | Sortie |
| 35 | I2C SDA | Bidirectionnel |
| 36 | I2C SCL | Sortie |
| 48 | LED WS2812 | Sortie |

---

## Notes importantes

### Niveaux de tension
RN2483 et ESP32-S3 sont tous les deux en **3.3V logique** : connexion directe,
**pas de level-shifter nécessaire**.

### Broche RST
Active à l'état **LOW**. Séquence de reset dans le code :
```
RST → LOW  (200 ms)
RST → HIGH (500 ms d'attente)
```

### Antenne
Le RN2483 nécessite une **antenne 868 MHz** sur sa broche ANT.
Sans antenne : risque de dommage et portée nulle.

### Courant RN2483
Jusqu'à **38.9 mA** en émission TX.
Vérifiez que votre régulateur 3.3V peut fournir ce courant.

### Modification des broches
Toutes les broches sont définies dans `include/config.h` :
```cpp
#define RN_PIN_RX      4
#define RN_PIN_TX      5
#define RN_PIN_RST     6
#define BUTTON_PIN     7
#define HX711_PIN_DOUT 10
#define HX711_PIN_SCK  11
#define I2C_PIN_SDA    35
#define I2C_PIN_SCL    36
#define LED_PIN        48
```
