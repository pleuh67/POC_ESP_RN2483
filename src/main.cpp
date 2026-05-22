// ============================================================
// POC_ESP_2483
// ESP32-S3 + RN2483 - LoRaWAN OTAA - Réseau Orange (EU868)
// ============================================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <rn2xx3.h>
#include "config.h"
#include "secret.h"   // table des modules + clés OTAA - non commité sur git

// ============================================================
// Objets globaux
// ============================================================
Adafruit_NeoPixel    led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial       rn_serial(RN_UART_NUM);
rn2xx3               myLora(rn_serial);

// Clés actives — renseignées au boot par lookupDevice()
String               g_deveui;
String               g_appeui;
String               g_appkey;
String               g_label;

// ============================================================
// LED helpers
// ============================================================
void ledSet(uint8_t r, uint8_t g, uint8_t b)
{
    led.setPixelColor(0, led.Color(r, g, b));
    led.show();
}

void ledOff()
{
    ledSet(0, 0, 0);
}

// ============================================================
// Séquence de boot
// - Clignote en orange pendant BOOT_WAIT_MS (250ms ON / 750ms OFF)
// - Affiche les infos de compilation dès Serial disponible
// - Passe au vert quand le programme démarre
// ============================================================
void bootWait()
{
    led.begin();
    led.setBrightness(38);   // 0-255 : 38 = 15% (WS2812 très lumineuse)
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

            if (ledState)
            {
                ledSet(BOOT_LED_R, BOOT_LED_G, BOOT_LED_B);
            }
            else
            {
                ledOff();
            }
        }
    }

    ledOff();

    // ── Bannière et infos de compilation ─────────────────────
    Serial.println();
    Serial.println("============================================================");
    Serial.println("  POC_ESP_2483 - ESP32-S3 + RN2483 OTAA - Orange EU868");
    Serial.println("============================================================");
    Serial.println("  INFORMATIONS DE COMPILATION");
    Serial.println("------------------------------------------------------------");
    Serial.print  ("  Date    : "); Serial.println(__DATE__);
    Serial.print  ("  Heure   : "); Serial.println(__TIME__);
    Serial.print  ("  Fichier : "); Serial.println(__FILE__);
    Serial.print  ("  Arduino : "); Serial.println(ARDUINO);
    Serial.print  ("  IDF     : "); Serial.println(ESP_IDF_VERSION_MAJOR);
    Serial.print  ("  CPU MHz : "); Serial.print(getCpuFrequencyMhz());          Serial.println(" MHz");
    Serial.print  ("  Flash   : "); Serial.print(ESP.getFlashChipSize() / 1024); Serial.println(" KB");
    Serial.print  ("  RAM     : "); Serial.print(ESP.getHeapSize()      / 1024); Serial.println(" KB");
    Serial.print  ("  PSRAM   : "); Serial.print(ESP.getPsramSize()     / 1024); Serial.println(" KB");
    Serial.println("------------------------------------------------------------");
    Serial.print  ("  DEBUG_HWEUI      : "); Serial.println(DEBUG_HWEUI ? "true" : "false");
    Serial.print  ("  SEND_INTERVAL_MS : "); Serial.print(SEND_INTERVAL_MS); Serial.println(" ms");
    Serial.print  ("  RN_UART_NUM      : "); Serial.println(RN_UART_NUM);
    Serial.print  ("  RN_PIN_RX        : GPIO"); Serial.println(RN_PIN_RX);
    Serial.print  ("  RN_PIN_TX        : GPIO"); Serial.println(RN_PIN_TX);
    Serial.print  ("  RN_PIN_RST       : GPIO"); Serial.println(RN_PIN_RST);
    Serial.println("============================================================");
    Serial.println();

    // LED verte : programme démarré
    ledSet(BOOT_READY_R, BOOT_READY_G, BOOT_READY_B);
    delay(1000);
    ledOff();
}

// ============================================================
// Reset matériel du RN2483 via la broche RST
// ============================================================
void resetRN2483()
{
    pinMode(RN_PIN_RST, OUTPUT);
    digitalWrite(RN_PIN_RST, LOW);
    delay(200);
    digitalWrite(RN_PIN_RST, HIGH);
    delay(500);
}

// ============================================================
// Recherche des clés OTAA correspondant au DevEUI lu
// dans le module RN2483.
// Remplit les variables globales g_deveui / g_appeui /
// g_appkey / g_label.
// Si le DevEUI n'est pas dans la table, utilise LORA_DEFAULT.
// ============================================================
void lookupDevice(const String& hwDeveui)
{
    Serial.println("[Keys] Recherche des cles pour DevEUI : " + hwDeveui);

    // Parcours du tableau jusqu'au marqueur de fin (nullptr)
    for (int i = 0; LORA_DEVICES[i].deveui != nullptr; i++)
    {
        if (hwDeveui.equalsIgnoreCase(String(LORA_DEVICES[i].deveui)))
        {
            g_deveui = String(LORA_DEVICES[i].deveui);
            g_appeui = String(LORA_DEVICES[i].appeui);
            g_appkey = String(LORA_DEVICES[i].appkey);
            g_label  = String(LORA_DEVICES[i].label);

            Serial.println("[Keys] OK Module trouve    : " + g_label);
            Serial.println("[Keys]    DevEUI           : " + g_deveui);
            Serial.println("[Keys]    AppEUI           : " + g_appeui);
            Serial.println("[Keys]    AppKey           : " + g_appkey);
            return;
        }
    }

    // DevEUI non trouvé → clés par défaut
    g_deveui = hwDeveui;                          // on garde le vrai DevEUI hardware
    g_appeui = String(LORA_DEFAULT.appeui);
    g_appkey = String(LORA_DEFAULT.appkey);
    g_label  = String(LORA_DEFAULT.label);

    Serial.println("[Keys] AVERTISSEMENT : DevEUI absent de secret.h");
    Serial.println("[Keys]   -> Cles par defaut utilisees (label : " + g_label + ")");
    Serial.println("[Keys]   -> Ajoutez ce DevEUI dans include/secret.h pour");
    Serial.println("[Keys]      utiliser des cles specifiques a ce module.");
}

// ============================================================
// Initialisation et jointure OTAA sur Orange (EU868)
// Bloque jusqu'à jointure réussie (3 tentatives max)
// ============================================================
void initLoRa()
{
    Serial.println("[LoRa] Initialisation du RN2483...");

    resetRN2483();

    rn_serial.begin(57600, SERIAL_8N1, RN_PIN_RX, RN_PIN_TX);
    delay(1000);

    myLora.autobaud();
    delay(500);

    String version = myLora.sysver();
    Serial.print("[LoRa] Firmware RN2483 : ");
    Serial.println(version.length() > 0 ? version : "non detecte - verifiez le cablage");

    // ── Lecture du DevEUI hardware ────────────────────────────
    String hwDeveui = myLora.hweui();
    hwDeveui.toUpperCase();

    Serial.println("[LoRa] DevEUI hardware : " + hwDeveui);

#if DEBUG_HWEUI
    Serial.println("------------------------------------------------------------");
    Serial.println("[LoRa] Mode DEBUG_HWEUI actif");
    Serial.println("[LoRa] -> Ajoutez ce DevEUI dans include/secret.h");
    Serial.println("[LoRa] -> puis passez DEBUG_HWEUI a false dans config.h");
    Serial.println("------------------------------------------------------------");
#endif

    // ── Recherche des clés dans secret.h ─────────────────────
    lookupDevice(hwDeveui);

    myLora.setFrequencyPlan(TTN_EU);

    const int MAX_ATTEMPTS = 3;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
    {
        Serial.printf("[LoRa] Tentative jointure OTAA %d/%d (module : %s)...\n",
                      attempt, MAX_ATTEMPTS, g_label.c_str());

        ledSet(0, 0, 180);   // LED bleue : tentative en cours

        bool joined = myLora.initOTAA(g_appeui, g_appkey, g_deveui);

        if (joined)
        {
            Serial.println("[LoRa] OK Jointure OTAA reussie ! (module : " + g_label + ")");
            ledSet(0, 200, 0);   // LED verte : succès
            delay(2000);
            ledOff();
            return;
        }

        ledSet(200, 0, 0);   // LED rouge : échec
        Serial.println("[LoRa] Echec jointure - nouvelle tentative dans 10s");
        delay(10000);
        ledOff();
    }

    Serial.println("[LoRa] ERREUR : Jointure impossible apres 3 tentatives.");
    Serial.println("[LoRa]   Verifiez :");
    Serial.println("[LoRa]   - Les cles du module '" + g_label + "' dans secret.h");
    Serial.println("[LoRa]   - L'enregistrement du device sur Orange Live Objects");
    Serial.println("[LoRa]   - La couverture reseau Orange LoRaWAN sur votre site");

    ledSet(200, 0, 0);   // LED rouge fixe : erreur fatale

    while (true)
    {
        delay(5000);
    }
}

// ============================================================
// Envoi du payload applicatif
// Adaptez cette fonction à vos données métier
// ============================================================
void sendPayload()
{
    static uint16_t counter = 0;

    char payload[5];
    snprintf(payload, sizeof(payload), "%04X", counter);

    Serial.printf("[LoRa] Envoi payload : %s (compteur=%d, module=%s)\n",
                  payload, counter, g_label.c_str());

    ledSet(60, 60, 60);   // LED blanche courte : envoi en cours

    TX_RETURN_TYPE result = myLora.txHex(String(payload), false);

    ledOff();

    switch (result)
    {
        case TX_SUCCESS:
            Serial.println("[LoRa] OK Trame envoyee");
            counter++;
            break;

        case TX_SUCCESS_WITH_RX:
            Serial.println("[LoRa] OK Trame envoyee + downlink recu");
            counter++;
            break;

        case TX_FAIL:
            Serial.println("[LoRa] ERREUR envoi - verifiez la connexion au reseau");
            break;

        default:
            Serial.printf("[LoRa] Resultat inattendu : %d\n", (int)result);
            break;
    }
}

// ============================================================
// Setup & Loop
// ============================================================
void setup()
{
    bootWait();   // attente 10s + affichage infos de compilation
    initLoRa();   // lecture DevEUI + lookup clés + jointure OTAA
}

void loop()
{
    sendPayload();
    delay(SEND_INTERVAL_MS);
}
