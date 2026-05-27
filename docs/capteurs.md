# Capteurs — POC_ESP_2483

Tous les capteurs I2C partagent le bus GPIO35 (SDA) / GPIO36 (SCL) à 50 kHz.
Le HX711 utilise un bus dédié GPIO10 (DOUT) / GPIO11 (SCK).

---

## Vue d'ensemble

| Capteur      | Adresse  | Grandeur mesurée                  | Champ NodeData      |
|--------------|----------|-----------------------------------|---------------------|
| BME280       | 0x76/77  | Température, Humidité, Pression   | `temp_c`, `hum_pct` |
| BH1750       | 0x23/5C  | Luminosité                        | `lux`               |
| MAX44009     | 0x4A     | Luminosité (fallback BH1750)      | `lux`               |
| INA219       | 0x40     | Tension batterie, Courant         | `vbat_v`            |
| DS3231       | 0x68     | Horloge temps réel                | *(calcul vsol_v)*   |
| AT24C32      | 0x57     | EEPROM 4 Ko                       | *(NodeConfig)*      |
| SH1106 OLED  | 0x3C/3D  | Afficheur 128×64                  | *(affichage)*       |
| HX711        | —        | Poids balance 1                   | `poids1_kg`         |

---

## BME280 — Température / Humidité / Pression

- **Bibliothèque** : `adafruit/Adafruit BME280 Library`
- **Adresse** : 0x76 (SDO=LOW) ou 0x77 (SDO=HIGH) — auto-détecté au boot
- **Résolution** : 0.01 °C, 0.008 %, 0.18 Pa

```cpp
bme.readTemperature()        // °C
bme.readHumidity()           // %
bme.readPressure() / 100.0f  // hPa
```

---

## BH1750 — Luminosité (prioritaire)

- **Bibliothèque** : `claws/BH1750`
- **Adresse** : 0x23 (ADDR=GND) ou 0x5C (ADDR=VCC) — auto-détecté au boot
- **Plage** : 1–65535 lux
- **Mode** : `CONTINUOUS_HIGH_RES_MODE` (résolution 1 lux, mesure toutes les 120 ms)

Le BH1750 est testé en premier. Si détecté, il est utilisé pour toute la session.
Le MAX44009 n'est essayé qu'en l'absence de BH1750.

```cpp
bh1750.readLightLevel()   // lux (float)
```

---

## MAX44009 — Luminosité (fallback)

- **Bibliothèque** : aucune — lecture I2C directe via `max44009ReadLux()`
- **Adresse** : 0x4A (ADDR=GND) ou 0x4B
- **Plage** : 0.045–188 000 lux
- **Formule** : `lux = 2^exp × mantisse × 0.045`
- **Registres** : 0x03 (high byte), 0x04 (low byte)

Utilisé uniquement si aucun BH1750 n'est détecté au démarrage.

---

## INA219 — Tension / Courant batterie

- **Bibliothèque** : `adafruit/Adafruit INA219`
- **Adresse** : 0x40 (A0=A1=GND)
- **Plage tension** : 0–26 V (bus), résolution 4 mV
- **Plage courant** : ±3.2 A (shunt 0.1 Ω par défaut)

```cpp
ina219.getBusVoltage_V()   // tension bus (vbat_v)
ina219.getCurrent_mA()     // courant en mA
```

---

## DS3231 — Horloge temps réel

- **Bibliothèque** : `adafruit/RTClib`
- **Adresse** : 0x68 (fixe)
- **Précision** : ±2 ppm (±1 min/an)

Usages dans le projet :
- Fournit l'heure à `computeVsol()` pour le modèle solaire jour/nuit
- Synchronisé après jointure OTAA via `mac get gpstime` (RN2483 >= 1.0.5)
- Réglable manuellement via les commandes série `h HH:MM` et `d DD/MM/YYYY`

```cpp
DateTime now = rtc.now();
now.hour()    // heure (0-23)
now.minute()  // minutes
now.second()  // secondes
```

---

## AT24C32 — EEPROM 32 kbit (config persistante)

- **Bibliothèque** : `robtillaart/I2C_EEPROM`
- **Adresse** : 0x57 (A0=A1=A2=HIGH — valeur par défaut des breakouts DS3231)
- **Capacité** : 4096 octets
- **Usage** : stockage de `NodeConfig` avec CRC32 (offset 0)

La structure `NodeConfig` contient :
| Champ | Type | Rôle |
|---|---|---|
| `rucher_id` | uint8_t | Identifiant rucher |
| `label[16]` | char | Nom du nœud |
| `hx711_scale` | float | Facteur d'étalonnage cellule de charge |
| `hx711_offset` | long | Offset tare à vide |
| `vbat_factor` | float | Correctif tension batterie |
| `vsol_factor` | float | Correctif tension solaire |
| `send_interval_min` | uint8_t | Intervalle d'envoi LoRa (min) |
| `_reserved[32]` | — | Réservé extensions futures |
| `crc` | uint32_t | CRC32 (polynôme 0xEDB88320) |

En cas de CRC invalide ou d'EEPROM absente, les valeurs par défaut de `config.h`
sont appliquées automatiquement.

---

## SH1106 OLED — Afficheur 128×64

- **Bibliothèque** : `adafruit/Adafruit SH110X`
- **Adresse** : 0x3C (SA0=GND) ou 0x3D (SA0=VCC) — auto-détecté
- **Résolution** : 128×64 pixels, police 6×8 → 20 colonnes × 8 lignes
- **Mode** : défilement vertical — chaque nouvelle ligne pousse les précédentes vers le haut

Contenu affiché :
- Boot : résultats de chaque test (`DS3231 14:32:07`, `BME280 22.9C 58%`, …)
- Jointure LoRa : `LoRa join 1/3...` puis `LoRa JOIN OK`
- Date/heure après synchro RTC
- Chaque TX : `14:32 13.5kg 22.9C` (OK) ou `14:32 13.5kg 22.9C KO` (échec)

---

## HX711 — Balance 1 (cellule de charge)

- **Bibliothèque** : `bogde/HX711`
- **Broches** : DOUT=GPIO10, SCK=GPIO11
- **Gain** : 128 (canal A, par défaut)

**Procédure de calibration (commande `c` dans le terminal) :**
1. Balance à vide → Entrée → tare automatique
2. Poser le poids de référence, saisir sa valeur en kg → Entrée
3. Le facteur `hx711_scale` et l'offset sont calculés et sauvegardés en EEPROM

Les valeurs par défaut (`DEFAULT_HX711_SCALE`, `DEFAULT_HX711_OFFSET`) dans `config.h`
sont utilisées si l'EEPROM est vierge.

---

## Balances 2, 3, 4 — Bluetooth (non implémenté)

Valeurs simulées (aléatoires) en attendant l'intégration Bluetooth.
Champs concernés : `poids2_kg`, `poids3_kg`, `poids4_kg`.
