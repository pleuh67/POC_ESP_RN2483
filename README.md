# POC_ESP_2483

> Preuve de concept : connexion LoRaWAN OTAA sur le réseau **Orange Live Objects**
> avec un **ESP32-S3** et un module **RN2483 (Microchip)**, développé sous
> **VSCode + PlatformIO**.

---

## Table des matières

1. [Matériel requis](#1-matériel-requis)
2. [Architecture du projet](#2-architecture-du-projet)
3. [Câblage](#3-câblage)
4. [Installation et configuration](#4-installation-et-configuration)
5. [Mise en route pas à pas](#5-mise-en-route-pas-à-pas)
6. [Utilisation](#6-utilisation)
7. [Adapter le payload à vos données](#7-adapter-le-payload-à-vos-données)
8. [Contraintes réseau EU868](#8-contraintes-réseau-eu868)
9. [Dépendances](#9-dépendances)
10. [Sécurité des clés](#10-sécurité-des-clés)
11. [Références](#11-références)

---

## 1. Matériel requis

| Composant | Détail |
|---|---|
| MCU | ESP32-S3 (ex : DevKitC-1) |
| Module LoRa | Microchip RN2483 (868 MHz, stack LoRaWAN intégré) |
| Réseau | Orange Live Objects (LoRaWAN EU868) |
| Connexion RN2483 | UART hardware 3.3V — **ne pas connecter en 5V** |

---

## 2. Architecture du projet

```
POC_ESP_2483/
├── docs/
│   ├── boot_procedure.md       <- Procédure de boot et LED d'état
│   ├── cablage.md              <- Schéma et détail du câblage
│   ├── codec_lora.md           <- Format payload 19 octets + codec JS Orange
│   ├── orange_liveobjects.md   <- Procédure enregistrement Orange
│   ├── troubleshooting.md      <- Guide de dépannage
│   └── unit_testing.md         <- Guide tests unitaires PlatformIO
├── include/
│   ├── config.h                <- Broches, paramètres (commité)
│   ├── node_data.h             <- Structure NodeData + NODE_PAYLOAD_BYTES
│   ├── secret.h                <- Clés OTAA (NON commité - .gitignore)
│   └── secret.h.example        <- Template des clés (commité, sans valeurs)
├── src/
│   └── main.cpp                <- Code principal
├── test/
│   └── test_lookup/            <- Tests unitaires (pio test -e native)
├── CLAUDE.md                   <- Conventions et instructions pour Claude Code
├── .gitignore
├── platformio.ini
└── README.md
```

---

## 3. Câblage

Voir [docs/cablage.md](docs/cablage.md) pour le schéma complet.

Résumé rapide :

| RN2483 | ESP32-S3 GPIO | Rôle |
|---|---|---|
| TX | GPIO 4 | Données RN2483 → ESP32-S3 |
| RX | GPIO 5 | Données ESP32-S3 → RN2483 |
| RST | GPIO 6 | Reset matériel du RN2483 |
| VDD | 3.3V | Alimentation |
| GND | GND | Masse commune |

> Les broches GPIO sont modifiables dans `include/config.h`.

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

PlatformIO détecte automatiquement le `platformio.ini` et propose d'initialiser le projet.

### Créer le fichier secret.h

```bash
cp include/secret.h.example include/secret.h
```

Renseignez vos clés dans `include/secret.h` (voir étape 5 ci-dessous).

---

## 5. Mise en route pas à pas

### Étape 1 — Lire le DevEUI matériel du RN2483

Dans `include/config.h`, vérifiez que `DEBUG_HWEUI` est à `true` (valeur par défaut).

Compilez et flashez via PlatformIO (Upload), puis ouvrez le moniteur série (115200 bauds).

Sortie attendue :

```
============================================================
  POC_ESP_2483 - ESP32-S3 + RN2483 OTAA - Orange EU868
============================================================
  ...infos compilation...
------------------------------------------------------------
  DEBUG_HWEUI      : true
  ...
============================================================

------------------------------------------------------------
  TESTS PERIPHERIQUES
------------------------------------------------------------
[Test] LED WS2812          OK (verifier visuellement)
[Test] RN2483              OK (firmware : 1.0.5, DevEUI : 0004A30B001A2B3C)
------------------------------------------------------------
  Bilan : 2 OK  0 ECHEC
------------------------------------------------------------
```

Notez le **DevEUI**.

### Étape 2 — Enregistrer le device sur Orange Live Objects

Voir [docs/orange_liveobjects.md](docs/orange_liveobjects.md) pour la procédure détaillée.

En résumé :
1. Connectez-vous sur https://liveobjects.orange-business.com
2. **Devices → New device → LoRaWAN**
3. Renseignez le DevEUI lu à l'étape 1
4. Définissez un AppEUI (8 octets hex) et un AppKey (16 octets hex) de votre choix
5. Sauvegardez — notez AppEUI et AppKey

### Étape 3 — Renseigner les clés dans secret.h

Éditez `include/secret.h` :

```cpp
static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "Module-01" },
    { nullptr, nullptr, nullptr, nullptr }
};

static const LoraDevice LORA_DEFAULT =
{
    "0000000000000000",
    "XXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "DEFAULT"
};
```

### Étape 4 — Désactiver le mode debug DevEUI

Dans `include/config.h` :

```cpp
#define DEBUG_HWEUI  false
```

### Étape 5 — Flasher et vérifier la jointure

Recompilez, flashez et ouvrez le moniteur série.

Sortie attendue :

```
[LoRa] Tentative jointure OTAA 1/3 (module : Module-01)...
[LoRa] OK Jointure OTAA reussie ! (module : Module-01)
[LoRa] Envoi 19 octets : 000000000000000000000000000000000000 (module=Module-01)
[LoRa] OK Trame envoyee
```

---

## 6. Utilisation

Une fois la jointure réussie, le device envoie une trame toutes les **5 minutes**
(paramètre `INTERVAL_PAYLOAD` dans `config.h`, converti en ms via `SEND_INTERVAL_MS`).

Les trames sont visibles dans Orange Live Objects :
**Devices → votre device → Messages**

---

## 7. Adapter le payload à vos données

Le payload envoie **19 octets** (format fixe, big-endian) définis dans `include/node_data.h` :

| Octets | Donnée          | Type   | Facteur |
|--------|-----------------|--------|---------|
| 0-1    | Température     | int16  | ×100    |
| 2-3    | Humidité        | uint16 | ×100    |
| 4-11   | Poids 1→4       | int16  | ×100    |
| 12-13  | Tension batterie| uint16 | ×100    |
| 14-15  | Tension solaire | uint16 | ×100    |
| 16-17  | Luminosité      | uint16 | ×1      |
| 18     | RucherID        | uint8  | ×1      |

Renseignez les champs du `NodeData` dans `loop()` avec les lectures réelles de vos capteurs.

Le codec JavaScript de décodage est disponible dans [`docs/codec_lora.md`](docs/codec_lora.md).

---

## 8. Contraintes réseau EU868

| Paramètre | Valeur |
|---|---|
| Bande de fréquence | 868 MHz (ISM, libre) |
| Canaux obligatoires | 868.10 / 868.30 / 868.50 MHz (125 kHz BW) |
| Duty cycle max | 1% → 36 s de TX max par heure et par canal |
| Puissance max | 14 dBm (25 mW) |
| Spreading Factor | SF7 (rapide, courte portée) à SF12 (lent, longue portée) |

> Ne pas descendre `INTERVAL_PAYLOAD` sous **1 minute** (60 000 ms).
> La valeur par défaut de 5 minutes est un bon compromis pour un POC.

---

## 9. Dépendances

| Librairie | Source | Rôle |
|---|---|---|
| `jpmeijers/RN2xx3 Arduino Library` | [PlatformIO Registry](https://registry.platformio.org/libraries/jpmeijers/RN2xx3%20Arduino%20Library) | Communication UART avec le RN2483 |
| `adafruit/Adafruit NeoPixel` | [PlatformIO Registry](https://registry.platformio.org/libraries/adafruit/Adafruit%20NeoPixel) | LED RGB WS2812 (GPIO48) |

Installée automatiquement par PlatformIO à la première compilation.

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
