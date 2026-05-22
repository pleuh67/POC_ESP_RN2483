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
│   ├── cablage.md              <- Schéma et détail du câblage
│   ├── orange_liveobjects.md   <- Procédure enregistrement Orange
│   └── troubleshooting.md      <- Guide de dépannage
├── include/
│   ├── config.h                <- Broches, paramètres (commité)
│   ├── secret.h                <- Clés OTAA (NON commité - .gitignore)
│   └── secret.h.example        <- Template des clés (commité, sans valeurs)
├── src/
│   └── main.cpp                <- Code principal
├── lib/                        <- Librairies locales (vide = gestion PlatformIO)
├── test/                       <- Tests unitaires
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
============================================
  POC_ESP_2483 - ESP32-S3 + RN2483 OTAA
  Reseau : Orange LoRaWAN EU868
============================================
[LoRa] Initialisation du RN2483...
[LoRa] Firmware RN2483 : 1.0.5
--------------------------------------------
[LoRa] DevEUI hardware : 0004A30B001A2B3C
[LoRa] -> Copiez ce DevEUI dans Orange Live Objects
[LoRa] -> puis renseignez DEVEUI dans include/secret.h
--------------------------------------------
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
const String DEVEUI = "0004A30B001A2B3C";   // DevEUI lu à l'étape 1
const String APPEUI = "XXXXXXXXXXXXXXXX";   // AppEUI défini sur Orange
const String APPKEY = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // AppKey défini
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
[LoRa] Tentative jointure OTAA 1/3...
[LoRa] OK Jointure OTAA reussie !
[LoRa] Envoi payload : 0000 (compteur=0)
[LoRa] OK Trame envoyee
```

---

## 6. Utilisation

Une fois la jointure réussie, le device envoie une trame toutes les **60 secondes**
(paramètre `SEND_INTERVAL_MS` dans `config.h`).

Les trames sont visibles dans Orange Live Objects :
**Devices → votre device → Messages**

---

## 7. Adapter le payload à vos données

Le payload de démonstration envoie un simple compteur 16 bits.
Modifiez la fonction `sendPayload()` dans `src/main.cpp` :

```cpp
void sendPayload() {
  // Exemple : température (int16 x100) + humidité (uint8)
  int16_t temp_x100 = 2350;   // représente 23.50 °C
  uint8_t humidity  = 65;     // 65 %

  char payload[9];
  snprintf(payload, sizeof(payload), "%04X%02X",
           (uint16_t)temp_x100, humidity);
  // Résultat : "092041" (3 octets)

  myLora.txHex(String(payload), false);
}
```

Le décodage côté Orange Live Objects se configure via un **codec** JavaScript
dans les paramètres de l'application LoRaWAN.

---

## 8. Contraintes réseau EU868

| Paramètre | Valeur |
|---|---|
| Bande de fréquence | 868 MHz (ISM, libre) |
| Canaux obligatoires | 868.10 / 868.30 / 868.50 MHz (125 kHz BW) |
| Duty cycle max | 1% → 36 s de TX max par heure et par canal |
| Puissance max | 14 dBm (25 mW) |
| Spreading Factor | SF7 (rapide, courte portée) à SF12 (lent, longue portée) |

> Ne pas descendre `SEND_INTERVAL_MS` sous **36 000** ms.
> La valeur par défaut de 60 000 ms est un minimum raisonnable pour un POC.

---

## 9. Dépendances

| Librairie | Source | Rôle |
|---|---|---|
| `jpmeijers/RN2xx3 Arduino Library` | [PlatformIO Registry](https://registry.platformio.org/libraries/jpmeijers/RN2xx3%20Arduino%20Library) | Communication UART avec le RN2483 |

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
