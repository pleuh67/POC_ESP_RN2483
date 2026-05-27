# POC_ESP_2483

> Surveillance de ruchers LoRaWAN OTAA sur le réseau **Orange Live Objects**
> avec un **ESP32-S3** et un module **RN2483 (Microchip)**, développé sous
> **VSCode + PlatformIO**.

---

## Table des matières

1. [Matériel requis](#1-matériel-requis)
2. [Architecture du projet](#2-architecture-du-projet)
3. [Câblage résumé](#3-câblage-résumé)
4. [Installation et configuration](#4-installation-et-configuration)
5. [Mise en route pas à pas](#5-mise-en-route-pas-à-pas)
6. [Utilisation](#6-utilisation)
7. [Format du payload LoRaWAN](#7-format-du-payload-lorawan)
8. [Contraintes réseau EU868](#8-contraintes-réseau-eu868)
9. [Dépendances](#9-dépendances)
10. [Sécurité des clés](#10-sécurité-des-clés)
11. [Références](#11-références)

---

## 1. Matériel requis

| Composant | Référence | Rôle |
|---|---|---|
| MCU | ESP32-S3 DevKitC-1 | Microcontrôleur principal |
| Module LoRa | Microchip RN2483 (868 MHz) | Stack LoRaWAN intégré |
| RTC | DS3231 (0x68) | Horloge temps réel + AT24C32 EEPROM (0x57) |
| Capteur T/H/P | BME280 (0x76) | Température, Humidité, Pression |
| Luminosité | BH1750 (0x23) ou MAX44009 (0x4A) | Ensoleillement (auto-détecté) |
| Balance | HX711 (GPIO10/11) | Cellule de charge poids ruche |
| Tension/Courant | INA219 (0x40) | Surveillance batterie |
| Afficheur | OLED SH1106 128×64 (0x3C) | Statut sans port série |
| Bouton | Poussoir vers GND | Envoi forcé LoRa |
| Réseau | Orange Live Objects | LoRaWAN EU868 |

> Le RN2483 fonctionne **exclusivement en 3.3V** — ne jamais connecter en 5V.

---

## 2. Architecture du projet

```
POC_ESP_2483/
├── docs/
│   ├── boot_procedure.md       <- Séquence de boot, LED d'état, OLED
│   ├── cablage.md              <- Schéma complet de câblage
│   ├── capteurs.md             <- Description de chaque capteur I2C / HX711
│   ├── codec_lora.md           <- Format payload 19 octets + codec JS Orange
│   ├── debug.md                <- LED d'état, affichage série, commandes
│   ├── orange_liveobjects.md   <- Procédure enregistrement Orange
│   ├── troubleshooting.md      <- Guide de dépannage
│   └── unit_testing.md         <- Guide tests unitaires PlatformIO
├── include/
│   ├── config.h                <- Broches, paramètres (commité)
│   ├── functions.h             <- Prototypes des fonctions
│   ├── node_config.h           <- Structure NodeConfig (config EEPROM)
│   ├── node_data.h             <- Structure NodeData + NODE_PAYLOAD_BYTES
│   ├── secret.h                <- Clés OTAA (NON commité — .gitignore)
│   └── secret.h.example        <- Template des clés (commité, sans valeurs)
├── src/
│   └── main.cpp                <- Code principal
├── test/
│   └── test_lookup/            <- Tests unitaires (pio test -e native)
├── CLAUDE.md                   <- Conventions pour Claude Code
├── .gitignore
├── platformio.ini
└── README.md
```

---

## 3. Câblage résumé

Voir [docs/cablage.md](docs/cablage.md) pour le schéma complet.

| Broche ESP32-S3 | Périphérique | Rôle |
|---|---|---|
| GPIO 4 | RN2483 TX | UART RX vers ESP32-S3 |
| GPIO 5 | RN2483 RX | UART TX vers RN2483 |
| GPIO 6 | RN2483 RST | Reset matériel (LOW = reset) |
| GPIO 7 | Bouton → GND | Envoi forcé LoRa (INPUT_PULLUP) |
| GPIO 10 | HX711 DOUT | Données cellule de charge |
| GPIO 11 | HX711 SCK | Horloge HX711 |
| GPIO 35 | I2C SDA | Bus capteurs I2C |
| GPIO 36 | I2C SCL | Bus capteurs I2C |
| GPIO 48 | LED WS2812 | LED RGB d'état intégrée |

Tous les capteurs I2C (DS3231, EEPROM, BME280, BH1750/MAX44009, INA219, OLED) partagent le bus GPIO35/36.

---

## 4. Installation et configuration

### Prérequis

- [VSCode](https://code.visualstudio.com/) installé
- Extension [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) installée
- Accès à [Orange Live Objects](https://liveobjects.orange-business.com)

### Cloner et ouvrir le projet

```bash
git clone <url-du-repo> POC_ESP_2483
cd POC_ESP_2483
```

Ouvrez le dossier dans VSCode : **File → Open Folder → POC_ESP_2483**

### Créer le fichier secret.h

```bash
cp include/secret.h.example include/secret.h
```

Renseignez vos clés dans `include/secret.h` (voir étape 5 ci-dessous).

---

## 5. Mise en route pas à pas

### Étape 1 — Lire le DevEUI matériel du RN2483

Dans `include/config.h`, `DEBUG_HWEUI` doit être à `true` (valeur par défaut).

Compilez, flashez via PlatformIO, puis ouvrez le moniteur série (115200 bauds).
Le DevEUI s'affiche dans la section **TESTS PERIPHERIQUES** :

```
[Test] RN2483              OK (firmware : 1.0.5, DevEUI : 0004A30B001A2B3C)
```

### Étape 2 — Enregistrer le device sur Orange Live Objects

Voir [docs/orange_liveobjects.md](docs/orange_liveobjects.md).

### Étape 3 — Renseigner les clés dans secret.h

```cpp
static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "Ruche-01" },
    { nullptr, nullptr, nullptr, nullptr }
};
```

### Étape 4 — Désactiver le mode debug DevEUI

```cpp
// include/config.h
#define DEBUG_HWEUI  false
```

### Étape 5 — Calibrer la balance HX711

Au premier démarrage, taper `c` dans le terminal série et suivre les instructions.
Le facteur de calibration est sauvegardé automatiquement en EEPROM.

### Étape 6 — Flasher et vérifier la jointure

```
[LoRa] OK Jointure OTAA reussie ! (module : Ruche-01)
[RTC] Synchro OK : 2026-05-27 14:32:07 (UTC+2)
[LoRa] Envoi notification de redemarrage (port 2)...
[LoRa] Notification redemarrage : OK
```

---

## 6. Utilisation

### Envoi automatique

Le nœud envoie une trame LoRaWAN toutes les **5 minutes** (réglable via
`DEFAULT_SEND_INTERVAL_MIN` dans `config.h`). L'intervalle est sauvegardé
en EEPROM dans `NodeConfig`.

### Bouton d'envoi forcé

Un appui sur le bouton (GPIO7 → GND) déclenche immédiatement une lecture
de tous les capteurs et un envoi LoRaWAN. Le prochain envoi automatique
repart de ce moment.

### Commandes série (terminal PlatformIO)

| Commande | Effet |
|---|---|
| `c` + Entrée | Lance la calibration HX711 (2 points, sauvegarde EEPROM) |
| `h HH:MM` + Entrée | Règle l'heure du DS3231 (ex : `h 14:30`) |
| `d DD/MM/YYYY` + Entrée | Règle la date du DS3231 (ex : `d 27/05/2026`) |
| `?` + Entrée | Affiche la liste des commandes |

> Activer `monitor_echo = yes` dans `platformio.ini` pour voir ce que l'on tape.

### Afficheur OLED

L'OLED SH1106 (8 lignes × 20 caractères) affiche en défilement :
- Les résultats des tests au boot (capteur OK/FAIL + valeur mesurée)
- Le statut de la jointure LoRa
- La date/heure après synchro RTC
- Chaque transmission : `HH:MM poids temp` ou `HH:MM poids temp KO`

### Heartbeat LED

Un flash vert de 30 ms toutes les 5 s confirme que le programme s'exécute
(`HEARTBEAT_INTERVAL_MS` dans `config.h`, 0 = désactivé).

### Configuration persistante (EEPROM)

La structure `NodeConfig` (calibration HX711, facteurs de correction,
intervalle d'envoi…) est stockée en EEPROM AT24C32 avec CRC32.
Elle survit aux redémarrages. En cas de CRC invalide, les valeurs par défaut
de `config.h` sont appliquées.

---

## 7. Format du payload LoRaWAN

Le nœud envoie **19 octets** en little-endian sur **le port 1**.
Un payload de redémarrage (`Restart` ASCII, 7 octets) est envoyé sur **le port 2**
après chaque jointure OTAA.

| Octets | Donnée           | Type   | Facteur | Résolution |
|--------|------------------|--------|---------|------------|
| 0      | RucherID         | uint8  | ×1      | 1          |
| 1-2    | Température      | int16  | ×100    | 0.01 °C    |
| 3-4    | Humidité         | uint16 | ×100    | 0.01 %     |
| 5-6    | Luminosité       | uint16 | ×1      | 1 lux      |
| 7-8    | Tension batterie | uint16 | ×100    | 0.01 V     |
| 9-10   | Tension solaire  | uint16 | ×100    | 0.01 V     |
| 11-12  | Poids Balance 1  | int16  | ×100    | 0.01 kg    |
| 13-14  | Poids Balance 2  | int16  | ×100    | 0.01 kg    |
| 15-16  | Poids Balance 3  | int16  | ×100    | 0.01 kg    |
| 17-18  | Poids Balance 4  | int16  | ×100    | 0.01 kg    |

Le codec JavaScript de décodage est dans [`docs/codec_lora.md`](docs/codec_lora.md).

---

## 8. Contraintes réseau EU868

| Paramètre | Valeur |
|---|---|
| Bande de fréquence | 868 MHz (ISM, libre) |
| Duty cycle max | 1% → 36 s de TX max par heure par canal |
| Puissance max | 14 dBm (25 mW) |

> Ne pas descendre `DEFAULT_SEND_INTERVAL_MIN` sous **1 minute**.
> La valeur par défaut de 5 minutes est un bon compromis.

---

## 9. Dépendances

| Librairie | Rôle |
|---|---|
| `jpmeijers/RN2xx3 Arduino Library` | Communication UART RN2483 |
| `adafruit/Adafruit NeoPixel @ ^1.12.3` | LED RGB WS2812 (GPIO48) |
| `adafruit/RTClib @ ^2.1.4` | DS3231 RTC |
| `robtillaart/I2C_EEPROM @ ^1.8.3` | AT24C32 EEPROM |
| `adafruit/Adafruit BME280 Library @ ^2.2.4` | BME280 Temp/Hum/Pression |
| `claws/BH1750` | BH1750 luminosité |
| `adafruit/Adafruit SH110X` | OLED SH1106 128×64 |
| `adafruit/Adafruit GFX Library` | Moteur graphique Adafruit |
| `adafruit/Adafruit INA219 @ ^1.2.3` | INA219 tension/courant |
| `bogde/HX711 @ ^0.7.5` | HX711 cellule de charge |

Installées automatiquement par PlatformIO à la première compilation.

---

## 10. Sécurité des clés

`include/secret.h` est listé dans `.gitignore` et **ne sera jamais commité**.

| Fichier | Commité | Contenu |
|---|---|---|
| `include/config.h` | oui | Broches, paramètres techniques |
| `include/secret.h` | NON | DevEUI, AppEUI, AppKey |
| `include/secret.h.example` | oui | Template vide, aucune valeur réelle |

---

## 11. Références

- [Microchip RN2483 Datasheet](https://www.microchip.com/en-us/product/RN2483)
- [Microchip RN2483 Command Reference](https://ww1.microchip.com/downloads/en/DeviceDoc/RN2483-LoRa-Technology-Module-Command-Reference-User-Guide-DS40001784G.pdf)
- [Orange Live Objects - Documentation](https://liveobjects.orange-business.com/doc/html/lo_manual.html)
- [jpmeijers/RN2483-Arduino-Library](https://github.com/jpmeijers/RN2483-Arduino-Library)
- [LoRaWAN Regional Parameters EU868](https://lora-alliance.org/resource_hub/rp2-1-0-3-lorawan-regional-parameters/)
- [PlatformIO ESP32-S3 boards](https://docs.platformio.org/en/latest/boards/espressif32/index.html)
