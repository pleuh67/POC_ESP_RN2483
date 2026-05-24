# Capteurs — POC_ESP_2483

Tous les capteurs I2C partagent le bus GPIO35 (SDA) / GPIO36 (SCL) à 50 kHz.  
Le HX711 utilise un bus dédié GPIO10 (DOUT) / GPIO11 (SCK).

---

## Vue d'ensemble

| Capteur   | Adresse | Grandeur mesurée              | Champ NodeData   |
|-----------|---------|-------------------------------|------------------|
| BME280    | 0x76    | Température, Humidité, Pression | `temp_c`, `hum_pct` |
| MAX44009  | 0x4A    | Luminosité                    | `lux`            |
| INA219    | 0x40    | Tension batterie, Courant     | `vbat_v`         |
| DS3231    | 0x68    | Horloge temps réel            | *(calcul vsol_v)*|
| AT24C32   | 0x57    | EEPROM 32 kbit                | *(stockage)*     |
| HX711     | —       | Poids balance 1               | `poids1_kg`      |

---

## BME280 — Température / Humidité / Pression

- **Bibliothèque** : `adafruit/Adafruit BME280 Library`
- **Adresse** : 0x76 (SDO=LOW) ou 0x77 (SDO=HIGH)
- **Résolution** : 0.01 °C, 0.008 %, 0.18 Pa
- **Remplace** : capteur DHT22 simulé

```cpp
bme.readTemperature()   // °C
bme.readHumidity()      // %
bme.readPressure()      // Pa (÷100 pour hPa)
```

---

## MAX44009 — Luminosité

- **Bibliothèque** : aucune — lecture I2C directe via `max44009ReadLux()`
- **Adresse** : 0x4A (par défaut) ou 0x4B
- **Plage** : 0.045 lux à 188 000 lux
- **Formule** : `lux = 2^exp × mantisse × 0.045`
- **Registres** : 0x03 (high byte), 0x04 (low byte)
- **Note** : module livré parfois comme "BH1750 compatible" — vérifier l'adresse au scan

---

## INA219 — Tension / Courant batterie

- **Bibliothèque** : `adafruit/Adafruit INA219`
- **Adresse** : 0x40 (A0=A1=GND)
- **Plage tension** : 0–26 V (bus), résolution 4 mV
- **Plage courant** : ±3.2 A (shunt 0.1 Ω par défaut)

```cpp
ina219.getBusVoltage_V()   // tension bus (vbat_v)
ina219.getCurrent_mA()     // courant en mA
ina219.getPower_mW()       // puissance en mW
```

---

## DS3231 — Horloge temps réel

- **Bibliothèque** : `adafruit/RTClib`
- **Adresse** : 0x68 (fixe)
- **Précision** : ±2 ppm (±1 min/an)
- **Usage** : fournit l'heure à `computeVsol()` pour le modèle jour/nuit

```cpp
DateTime now = rtc.now();
now.hour()    // heure (0-23)
now.minute()  // minutes
```

> **Première mise en service** : si l'heure est perdue, régler via :
> ```cpp
> rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
> ```

---

## AT24C32 — EEPROM 32 kbit

- **Bibliothèque** : `robtillaart/I2C_EEPROM`
- **Adresse** : 0x57 (A0=A1=A2=HIGH)
- **Capacité** : 4096 octets
- **Usage prévu** : stockage de configuration, compteurs de cycles, log
- **Intégré** sur les modules DS3231 breakout courants

---

## HX711 — Balance 1 (cellule de charge)

- **Bibliothèque** : `bogde/HX711`
- **Broches** : DOUT=GPIO10, SCK=GPIO11
- **Gain** : 128 (canal A, par défaut)
- **Étalonnage** : `HX711_SCALE` dans `config.h`

**Procédure d'étalonnage :**
1. Poser la balance à vide → noter `raw_0` (tare)
2. Poser un poids connu `P` kg → noter `raw_P`
3. `HX711_SCALE = (raw_P - raw_0) / P`
4. Mettre à jour `config.h` et recompiler

---

## Balances 2, 3, 4 — Bluetooth (non implémenté)

Valeurs simulées (aléatoires) en attendant l'intégration Bluetooth.  
Champs concernés : `poids2_kg`, `poids3_kg`, `poids4_kg`.
