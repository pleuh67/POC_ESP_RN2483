# Procédure de boot — ESP32-S3 DevKitC-1

> Séquence complète du démarrage : de la mise sous tension à la première trame envoyée.

---

## Problème résolu

Après un flash ou un reset, l'ESP32-S3 démarre immédiatement.
Si le terminal série n'est pas encore ouvert, les messages de `setup()` sont perdus.

Cette procédure ajoute une fenêtre de **10 secondes** (LED orange clignotante)
pendant laquelle le programme attend. L'OLED affiche le journal en temps réel
même sans port série ouvert.

---

## Séquence de démarrage

```
Mise sous tension
  └─ bootWait()          LED orange clignotante 10 s + bannière série
      └─ initSensors()
          ├─ Wire.begin() + setClock(50 kHz)
          ├─ Diagnostic SDA/SCL (niveau au repos)
          ├─ scanI2C()            Affiche tous les périphériques détectés
          ├─ oledInit()           Tente 0x3C puis 0x3D
          ├─ loadConfig()         EEPROM → NodeConfig (CRC32 vérifié)
          ├─ bme.begin()          0x76 ou 0x77
          ├─ bh1750.begin()       0x23 ou 0x5C (sinon MAX44009)
          ├─ rtc.begin()
          ├─ ina219.begin()
          └─ scale.begin()        HX711 GPIO10/11, scale/offset depuis NodeConfig
      └─ runPeripheralTests()
          ├─ testLed()
          ├─ testRN2483()
          ├─ testDS3231()         → OLED : "DS3231 14:32:07"
          ├─ testBME280()         → OLED : "BME280 22.9C 58%"
          ├─ testLuxSensor()      → OLED : "LUX    423 lux"
          ├─ testINA219()         → OLED : "INA219 3.82V"
          ├─ testHX711()          → OLED : "HX711  13.39kg"
          └─ bilan               → OLED : "5 OK  0 FAIL"
      └─ initLoRa()
          ├─ resetRN2483()
          ├─ autobaud()
          ├─ hweui()              Lecture DevEUI hardware
          ├─ lookupDevice()       Recherche clés dans secret.h
          ├─ initOTAA() × 3      Tentatives jointure OTAA
          ├─ syncRtcFromLoRa()   Synchro RTC via mac get gpstime
          ├─ affichage date/heure sur OLED
          └─ sendRestartPayload() "Restart" ASCII sur port LoRa 2
loop()
  ├─ handleSerial()       Commandes c / h HH:MM / d DD/MM/YYYY / ?
  ├─ heartbeat LED        Flash vert 30 ms toutes les 5 s
  ├─ bouton GPIO7         Envoi forcé immédiat si appuyé
  └─ toutes les 5 min     readSensors() + sendPayload() (port 1)
```

---

## Séquences LED WS2812 (GPIO48)

| Phase | Couleur | Clignotement | Signification |
|---|---|---|---|
| Boot wait (10 s) | Orange | 250 ms ON / 750 ms OFF | Attente ouverture terminal |
| Démarrage | Vert fixe 1 s | — | `bootWait()` terminé |
| Tests périphériques OK | Vert fixe 1 s | — | Tous les capteurs répondent |
| Tests périphériques KO | Rouge 3× | 200 ms | Au moins un capteur en échec |
| Join OTAA en cours | Bleu fixe | — | Tentative jointure réseau |
| Join OTAA réussi | Vert fixe 2 s | — | Réseau joint |
| Échec jointure (tentative) | Rouge 1 s | — | Nouvelle tentative dans 10 s |
| Erreur fatale | Rouge fixe permanent | — | Boucle infinie |
| Envoi trame (port 1 ou 2) | Blanc bref | — | TX en cours |
| Heartbeat (loop) | Vert 30 ms | toutes les 5 s | Programme actif |

---

## Affichage série au démarrage

```
============================================================
  POC_ESP_2483 - ESP32-S3 + RN2483 OTAA - Orange EU868
============================================================
  INFORMATIONS DE COMPILATION
------------------------------------------------------------
  Date    : May 27 2026
  Heure   : 14:32:11
  CPU MHz : 240 MHz
  Flash   : 8192 KB
  RAM     : 327 KB
------------------------------------------------------------
  DEBUG_HWEUI      : false
  SEND_INTERVAL_MS : 300000 ms
  RN_PIN_RX        : GPIO4
============================================================

[I2C] Bus initialise (SDA=GPIO35, SCL=GPIO36, 100kHz)
[I2C] SDA=HIGH  SCL=HIGH
[I2C] Scan du bus I2C :
[I2C] +---------+----------------------+
[I2C] | Adresse | Peripherique         |
[I2C] +---------+----------------------+
[I2C] |  0x23   | BH1750 (ADDR=0)      |
[I2C] |  0x3C   | SH1106 OLED          |
[I2C] |  0x40   | INA219               |
[I2C] |  0x57   | AT24C32 EEPROM       |
[I2C] |  0x68   | DS3231 RTC           |
[I2C] |  0x76   | BME280               |
[I2C] +---------+----------------------+
[I2C] 6 peripherique(s) trouve(s)
[OLED] Initialise OK
[Config] Chargement depuis EEPROM...
  CONFIG EEPROM OK
  Label          : RUCHE_01
  HX711 scale    : -1044858.81
  ...
------------------------------------------------------------
  TESTS PERIPHERIQUES
------------------------------------------------------------
[Test] LED WS2812          OK (verifier visuellement)
[Test] RN2483              OK (firmware : 1.0.5, DevEUI : 0004A30B001A2B3C)
[Test] DS3231              OK (2026-05-27 14:32:07)
[Test] BME280              OK (22.9 C  58.0 %  1013.2 hPa)
[Test] BH1750              OK (423.0 lux)
[Test] INA219              OK (3.82 V  12.3 mA)
[Test] HX711               OK (raw=-1625200  13.390 kg)
  Bilan : 7 OK  0 ECHEC

[LoRa] Tentative jointure OTAA 1/3 (module : RUCHE_01)...
[LoRa] OK Jointure OTAA reussie !
[RTC] Synchro OK : 2026-05-27 14:32:15 (UTC+2)
[LoRa] Envoi notification de redemarrage (port 2)...
[LoRa] Notification redemarrage : OK
```

---

## Paramètres configurables (include/config.h)

```cpp
// LED
#define LED_PIN           48
#define LED_COUNT         1

// Boot
#define BOOT_WAIT_MS      10000   // durée d'attente (ms)
#define BOOT_LED_ON_MS    250     // LED allumée (ms)
#define BOOT_LED_OFF_MS   750     // LED éteinte (ms)

// Heartbeat
#define HEARTBEAT_INTERVAL_MS  5000UL   // 0 = désactivé

// Bouton
#define BUTTON_PIN        7             // GPIO bouton envoi forcé
#define BUTTON_DEBOUNCE_MS  50          // anti-rebond (ms)

// Envoi
#define DEFAULT_SEND_INTERVAL_MIN  5    // intervalle LoRa (minutes)
#define TIMEZONE_OFFSET_H          2    // UTC+2 été, UTC+1 hiver
```

---

## Gestion multi-modules (secret.h)

Le fichier `secret.h` contient un tableau de structures `LoraDevice`.
Le DevEUI hardware est lu via `hweui()`, la correspondance est cherchée dans le tableau.

```cpp
static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "0000000000000001", "2B7E...CF4F3C", "Ruche-01" },
    { "0004A30B001A2B3D", "0000000000000001", "3C8F...5G4D",   "Ruche-02" },
    { nullptr, nullptr, nullptr, nullptr }   // marqueur de fin obligatoire
};
```

---

## Checklist pour un nouveau module

- [ ] Lire le DevEUI avec `DEBUG_HWEUI true`
- [ ] Enregistrer le device sur Orange Live Objects
- [ ] Renseigner les clés dans `secret.h`
- [ ] Passer `DEBUG_HWEUI` à `false`
- [ ] Calibrer la balance HX711 (`c` dans le terminal)
- [ ] Vérifier l'heure RTC (`h HH:MM` et `d DD/MM/YYYY` si nécessaire)
- [ ] Confirmer la réception des trames sur Orange Live Objects
