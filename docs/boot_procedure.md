# Procédure de boot série — Standard ESP32-S3 DevKitC-1

> Cette procédure est à appliquer **au début de chaque nouveau projet**
> ESP32-S3 sous VSCode/PlatformIO pour garantir l'affichage des informations
> de démarrage dans le terminal série.

---

## Problème résolu

Après un flash ou un reset, l'ESP32-S3 démarre immédiatement.
Si le terminal série n'est pas encore ouvert, les messages de `setup()`
sont perdus et ne s'affichent jamais.

Cette procédure ajoute une fenêtre de **10 secondes** pendant laquelle
la LED clignote en orange, signalant que le programme attend.
Dès que le terminal est ouvert, les messages sont disponibles.

---

## Comportement visuel de la LED (GPIO48 — WS2812)

| Phase | Couleur | Clignotement | Signification |
|---|---|---|---|
| Boot wait (10 s) | Orange | 250 ms ON / 750 ms OFF | En attente d'ouverture du terminal |
| Démarrage | Vert fixe 1 s | — | Programme lancé |
| Lookup clés | — | — | Recherche dans secret.h (instantané) |
| Jointure OTAA en cours | Bleu fixe | — | Tentative de connexion réseau |
| Jointure réussie | Vert fixe 2 s | — | Réseau joint |
| Échec jointure | Rouge fixe | — | Erreur réseau (voir terminal) |
| Envoi trame | Blanc bref | — | TX en cours |
| Erreur fatale | Rouge fixe permanent | — | Blocage — vérifiez le terminal |

---

## Paramètres configurables (include/config.h)

```cpp
// LED
#define LED_PIN           48     // GPIO WS2812 (DevKitC-1)
#define LED_COUNT         1      // Nombre de LEDs

// Séquence de boot
#define BOOT_WAIT_MS      10000  // Durée d'attente totale (ms)
#define BOOT_LED_ON_MS    250    // LED allumée (ms)
#define BOOT_LED_OFF_MS   750    // LED éteinte (ms)

// Couleurs
#define BOOT_LED_R        255    // Orange pendant l'attente
#define BOOT_LED_G        80
#define BOOT_LED_B        0
#define BOOT_READY_R      0      // Vert au démarrage
#define BOOT_READY_G      200
#define BOOT_READY_B      0
```

---

## Informations affichées au démarrage

Une fois le terminal ouvert, la séquence complète s'affiche :

```
============================================================
  POC_ESP_2483 - ESP32-S3 + RN2483 OTAA - Orange EU868
============================================================
  INFORMATIONS DE COMPILATION
------------------------------------------------------------
  Date    : May 20 2026
  Heure   : 14:32:11
  Fichier : src/main.cpp
  Arduino : 10813
  IDF     : 5
  CPU MHz : 240 MHz
  Flash   : 8192 KB
  RAM     : 327 KB
  PSRAM   : 0 KB
------------------------------------------------------------
  DEBUG_HWEUI      : false
  SEND_INTERVAL_MS : 300000 ms
  RN_UART_NUM      : 1
  RN_PIN_RX        : GPIO4
  RN_PIN_TX        : GPIO5
  RN_PIN_RST       : GPIO6
============================================================

[LoRa] Initialisation du RN2483...
[LoRa] Firmware RN2483 : 1.0.5
[LoRa] DevEUI hardware : 0004A30B001A2B3C
[Keys] Recherche des cles pour DevEUI : 0004A30B001A2B3C
[Keys] OK Module trouve    : Module-01
[Keys]    DevEUI           : 0004A30B001A2B3C
[Keys]    AppEUI           : 0000000000000001
[Keys]    AppKey           : 2B7E151628AED2A6ABF7158809CF4F3C
[LoRa] Tentative jointure OTAA 1/3 (module : Module-01)...
[LoRa] OK Jointure OTAA reussie ! (module : Module-01)
```

Si le DevEUI n'est pas dans `secret.h`, le programme continue avec les clés par défaut :

```
[Keys] AVERTISSEMENT : DevEUI absent de secret.h
[Keys]   -> Cles par defaut utilisees (label : DEFAULT)
[Keys]   -> Ajoutez ce DevEUI dans include/secret.h pour
            utiliser des cles specifiques a ce module.
```

---

## Gestion multi-modules (secret.h)

Le fichier `secret.h` contient un tableau de structures `LoraDevice`.
Le programme lit le DevEUI hardware du RN2483 au démarrage, cherche
une correspondance dans le tableau et charge automatiquement les bonnes clés.

```cpp
struct LoraDevice
{
    const char* deveui;   // DevEUI hardware lu via hweui()
    const char* appeui;   // AppEUI sur Orange Live Objects
    const char* appkey;   // AppKey AES-128
    const char* label;    // Nom libre pour les logs
};

static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "0000000000000001", "2B7E...CF4F3C", "Module-01" },
    { "0004A30B001A2B3D", "0000000000000001", "3C8F...5G4D",   "Module-02" },
    // ... jusqu'à 10 entrées
    { nullptr, nullptr, nullptr, nullptr }   // marqueur de fin obligatoire
};

static const LoraDevice LORA_DEFAULT =
{
    "0000000000000000",
    "XXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "DEFAULT"
};
```

---

## Librairie requise

La LED WS2812 nécessite la librairie **Adafruit NeoPixel**.
À ajouter dans `platformio.ini` :

```ini
lib_deps =
  adafruit/Adafruit NeoPixel @ ^1.12.3
```

---

## Checklist pour un nouveau projet

- [ ] Copier `ledSet()`, `ledOff()`, `bootWait()`, `lookupDevice()` dans `main.cpp`
- [ ] Copier le bloc LED et le bloc BOOT dans `config.h`
- [ ] Copier la structure `LoraDevice` et les tableaux dans `secret.h`
- [ ] Ajouter `Adafruit NeoPixel` dans `lib_deps` de `platformio.ini`
- [ ] Ajouter `secret.h` dans `.gitignore`
- [ ] Appeler `bootWait()` en **première instruction** de `setup()`
- [ ] Appeler `lookupDevice(hwDeveui)` avant `initOTAA()`
- [ ] Adapter la bannière (nom du projet, description) dans `bootWait()`

---

## Extraits de code réutilisables

### config.h — bloc à copier

```cpp
// LED RGB WS2812 intégrée (GPIO48 sur ESP32-S3-DevKitC-1)
#define LED_PIN           48
#define LED_COUNT         1

// Séquence de boot
#define BOOT_WAIT_MS      10000
#define BOOT_LED_ON_MS    250
#define BOOT_LED_OFF_MS   750
#define BOOT_LED_R        255
#define BOOT_LED_G        80
#define BOOT_LED_B        0
#define BOOT_READY_R      0
#define BOOT_READY_G      200
#define BOOT_READY_B      0
```

### secret.h — structure à copier

```cpp
struct LoraDevice
{
    const char* deveui;
    const char* appeui;
    const char* appkey;
    const char* label;
};

static const LoraDevice LORA_DEVICES[] =
{
    { "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "Module-01" },
    // { "XXXXXXXXXXXXXXXX", ... },
    { nullptr, nullptr, nullptr, nullptr }   // ne pas supprimer
};

static const LoraDevice LORA_DEFAULT =
{
    "0000000000000000",
    "XXXXXXXXXXXXXXXX",
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "DEFAULT"
};
```

### main.cpp — blocs à copier

```cpp
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "secret.h"

Adafruit_NeoPixel  led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Clés actives chargées au boot
String  g_deveui, g_appeui, g_appkey, g_label;

// ---------------------------------------------------------------------------*
// @brief  Applique une couleur RGB à la LED WS2812
// @param  r  Composante rouge (0-255)
// @param  g  Composante verte (0-255)
// @param  b  Composante bleue (0-255)
// ---------------------------------------------------------------------------*
void ledSet(uint8_t r, uint8_t g, uint8_t b)
{
  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

// ---------------------------------------------------------------------------*
// @brief  Éteint la LED WS2812
// ---------------------------------------------------------------------------*
void ledOff()
{
  ledSet(0, 0, 0);
}

// ---------------------------------------------------------------------------*
// @brief  Séquence de boot : clignote en orange, affiche les infos de
//         compilation, puis LED verte 1 s
// @note   Durée totale pilotée par BOOT_WAIT_MS
// ---------------------------------------------------------------------------*
void bootWait()
{
  led.begin();
  led.setBrightness(38);
  led.show();
  Serial.begin(115200);

  uint32_t start    = millis();
  uint32_t deadline = start + BOOT_WAIT_MS;
  bool     ledState = false;
  uint32_t ledTimer = start;

  while (millis() < deadline)
  {
    uint32_t now      = millis();
    uint32_t interval = ledState ? BOOT_LED_ON_MS : BOOT_LED_OFF_MS;

    if (now - ledTimer >= interval)
    {
      ledState = !ledState;
      ledTimer = now;

      if (ledState) { ledSet(BOOT_LED_R, BOOT_LED_G, BOOT_LED_B); }
      else          { ledOff(); }
    }
  }

  ledOff();

  Serial.println();
  Serial.println("============================================================");
  Serial.println("  NOM_PROJET - DESCRIPTION");
  Serial.println("============================================================");
  Serial.println("  INFORMATIONS DE COMPILATION");
  Serial.println("------------------------------------------------------------");
  Serial.printf("  Date    : %s\n",     __DATE__);
  Serial.printf("  Heure   : %s\n",     __TIME__);
  Serial.printf("  Fichier : %s\n",     __FILE__);
  Serial.printf("  Arduino : %d\n",     ARDUINO);
  Serial.printf("  IDF     : %d\n",     ESP_IDF_VERSION_MAJOR);
  Serial.printf("  CPU MHz : %u MHz\n", getCpuFrequencyMhz());
  Serial.printf("  Flash   : %u KB\n",  ESP.getFlashChipSize() / 1024);
  Serial.printf("  RAM     : %u KB\n",  ESP.getHeapSize()      / 1024);
  Serial.printf("  PSRAM   : %u KB\n",  ESP.getPsramSize()     / 1024);
  Serial.println("------------------------------------------------------------");
  // Serial.printf("  MON_PARAM : %d\n", MON_PARAM);
  Serial.println("============================================================");
  Serial.println();

  ledSet(BOOT_READY_R, BOOT_READY_G, BOOT_READY_B);
  delay(1000);
  ledOff();
}

// ---------------------------------------------------------------------------*
// @brief  Recherche les clés OTAA correspondant au DevEUI hardware
// @param  hwDeveui  DevEUI lu via myLora.hweui() (16 caractères hex)
// @note   Remplit g_deveui, g_appeui, g_appkey, g_label
// @note   Si absent de LORA_DEVICES[], utilise LORA_DEFAULT
// ---------------------------------------------------------------------------*
void lookupDevice(const String& hwDeveui)
{
  Serial.printf("[Keys] Recherche des cles pour DevEUI : %s\n", hwDeveui.c_str());

  for (uint8_t i = 0; LORA_DEVICES[i].deveui != nullptr; i++)
  {
    if (hwDeveui.equalsIgnoreCase(String(LORA_DEVICES[i].deveui)))
    {
      g_deveui = String(LORA_DEVICES[i].deveui);
      g_appeui = String(LORA_DEVICES[i].appeui);
      g_appkey = String(LORA_DEVICES[i].appkey);
      g_label  = String(LORA_DEVICES[i].label);

      Serial.printf("[Keys] OK Module trouve : %s\n", g_label.c_str());
      return;
    }
  }

  g_deveui = hwDeveui;
  g_appeui = String(LORA_DEFAULT.appeui);
  g_appkey = String(LORA_DEFAULT.appkey);
  g_label  = String(LORA_DEFAULT.label);

  Serial.println("[Keys] AVERTISSEMENT : DevEUI absent de secret.h");
  Serial.printf("[Keys]   -> Cles par defaut (label : %s)\n", g_label.c_str());
}

void setup()
{
  bootWait();
  // ... initLoRa() ou équivalent
}
```
