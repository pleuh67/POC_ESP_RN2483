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
Adafruit_NeoPixel  led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial     rnSerial(RN_UART_NUM);
rn2xx3             myLora(rnSerial);

// Clés actives — renseignées au boot par lookupDevice()
String  g_deveui;
String  g_appeui;
String  g_appkey;
String  g_label;

// ---------------------------------------------------------------------------*
// @brief  Applique une couleur RGB à la LED WS2812
// @param  r  Composante rouge   (0-255)
// @param  g  Composante verte   (0-255)
// @param  b  Composante bleue   (0-255)
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
// @brief  Séquence de boot : clignote en orange le temps d'ouvrir le terminal,
//         affiche les infos de compilation, puis LED verte 1 s
// @note   Durée totale pilotée par BOOT_WAIT_MS (défaut 10 s)
// ---------------------------------------------------------------------------*
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
  Serial.printf("  DEBUG_HWEUI      : %s\n",    DEBUG_HWEUI ? "true" : "false");
  Serial.printf("  SEND_INTERVAL_MS : %d ms\n", SEND_INTERVAL_MS);
  Serial.printf("  RN_UART_NUM      : %d\n",    RN_UART_NUM);
  Serial.printf("  RN_PIN_RX        : GPIO%d\n", RN_PIN_RX);
  Serial.printf("  RN_PIN_TX        : GPIO%d\n", RN_PIN_TX);
  Serial.printf("  RN_PIN_RST       : GPIO%d\n", RN_PIN_RST);
  Serial.println("============================================================");
  Serial.println();

  // LED verte : programme démarré
  ledSet(BOOT_READY_R, BOOT_READY_G, BOOT_READY_B);
  delay(1000);
  ledOff();
}

// ---------------------------------------------------------------------------*
// @brief  Reset matériel du RN2483 via la broche RST (LOW 200 ms, puis HIGH)
// ---------------------------------------------------------------------------*
void resetRN2483()
{
  pinMode(RN_PIN_RST, OUTPUT);
  digitalWrite(RN_PIN_RST, LOW);
  delay(200);
  digitalWrite(RN_PIN_RST, HIGH);
  delay(500);
}

// ---------------------------------------------------------------------------*
// @brief  Test de la LED WS2812 : cycle rouge → vert → bleu (300 ms chacun)
// @return bool  Toujours true (pas de retour d'état possible sur WS2812)
// @note   Vérification visuelle uniquement — inspecter la LED pendant le boot
// ---------------------------------------------------------------------------*
bool testLed()
{
  Serial.println("[Test] LED WS2812...");
  ledSet(200, 0,   0);   delay(300);   // rouge
  ledSet(0,   200, 0);   delay(300);   // vert
  ledSet(0,   0,   200); delay(300);   // bleu
  ledOff();
  Serial.println("[Test] LED WS2812          OK (verifier visuellement)");
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Test du RN2483 : reset, autobaud, lecture firmware et DevEUI hardware
// @return bool  True si le module répond, false si absence de réponse
// @note   N'effectue pas la jointure OTAA — diagnostic uniquement
// ---------------------------------------------------------------------------*
bool testRN2483()
{
  Serial.println("[Test] RN2483...");

  resetRN2483();
  rnSerial.begin(57600, SERIAL_8N1, RN_PIN_RX, RN_PIN_TX);
  delay(500);
  myLora.autobaud();
  delay(300);

  String version = myLora.sysver();
  if (version.length() == 0)
  {
    Serial.println("[Test] RN2483              ECHEC (pas de reponse firmware)");
    Serial.println("[Test]   -> Verifiez le cablage UART et la tension 3.3V");
    return false;
  }

  String hweui = myLora.hweui();
  hweui.toUpperCase();
  Serial.printf("[Test] RN2483              OK (firmware : %s, DevEUI : %s)\n",
                version.c_str(), hweui.c_str());
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Lance tous les tests périphériques et affiche le bilan
// @note   Un échec n'arrête pas le boot — le bilan est affiché sur le port série
// @note   LED verte 1 s si tout OK, rouge clignotant 3 × si au moins un échec
// ---------------------------------------------------------------------------*
void runPeripheralTests()
{
  Serial.println("------------------------------------------------------------");
  Serial.println("  TESTS PERIPHERIQUES");
  Serial.println("------------------------------------------------------------");

  uint8_t passed = 0;
  uint8_t failed = 0;

  testLed()     ? passed++ : failed++;
  testRN2483()  ? passed++ : failed++;

  Serial.println("------------------------------------------------------------");
  Serial.printf("  Bilan : %d OK  %d ECHEC\n", passed, failed);
  Serial.println("------------------------------------------------------------");
  Serial.println();

  if (failed == 0)
  {
    ledSet(0, 200, 0);   // vert : tous les tests passés
    delay(1000);
    ledOff();
  }
  else
  {
    // rouge clignotant 3× : au moins un périphérique ne répond pas
    for (uint8_t i = 0; i < 3; i++)
    {
      ledSet(200, 0, 0);
      delay(200);
      ledOff();
      delay(200);
    }
  }
}

// ---------------------------------------------------------------------------*
// @brief  Recherche les clés OTAA correspondant au DevEUI hardware du RN2483
// @param  hwDeveui  DevEUI lu via myLora.hweui() (16 caractères hex)
// @note   Remplit g_deveui, g_appeui, g_appkey, g_label
// @note   Si le DevEUI est absent de LORA_DEVICES[], utilise LORA_DEFAULT
// ---------------------------------------------------------------------------*
void lookupDevice(const String& hwDeveui)
{
  Serial.printf("[Keys] Recherche des cles pour DevEUI : %s\n", hwDeveui.c_str());

  // Parcours du tableau jusqu'au marqueur de fin (nullptr)
  for (uint8_t i = 0; LORA_DEVICES[i].deveui != nullptr; i++)
  {
    if (hwDeveui.equalsIgnoreCase(String(LORA_DEVICES[i].deveui)))
    {
      g_deveui = String(LORA_DEVICES[i].deveui);
      g_appeui = String(LORA_DEVICES[i].appeui);
      g_appkey = String(LORA_DEVICES[i].appkey);
      g_label  = String(LORA_DEVICES[i].label);

      Serial.printf("[Keys] OK Module trouve    : %s\n", g_label.c_str());
      Serial.printf("[Keys]    DevEUI           : %s\n", g_deveui.c_str());
      Serial.printf("[Keys]    AppEUI           : %s\n", g_appeui.c_str());
      Serial.printf("[Keys]    AppKey           : %s\n", g_appkey.c_str());
      return;
    }
  }

  // DevEUI non trouvé → clés par défaut
  g_deveui = hwDeveui;                          // on garde le vrai DevEUI hardware
  g_appeui = String(LORA_DEFAULT.appeui);
  g_appkey = String(LORA_DEFAULT.appkey);
  g_label  = String(LORA_DEFAULT.label);

  Serial.println("[Keys] AVERTISSEMENT : DevEUI absent de secret.h");
  Serial.printf("[Keys]   -> Cles par defaut utilisees (label : %s)\n", g_label.c_str());
  Serial.println("[Keys]   -> Ajoutez ce DevEUI dans include/secret.h pour");
  Serial.println("[Keys]      utiliser des cles specifiques a ce module.");
}

// ---------------------------------------------------------------------------*
// @brief  Initialise le RN2483 et réalise la jointure OTAA sur Orange EU868
// @note   Bloque jusqu'à jointure réussie ou 3 tentatives épuisées
// @note   En cas d'échec définitif : LED rouge fixe + boucle infinie
// ---------------------------------------------------------------------------*
void initLoRa()
{
  Serial.println("[LoRa] Initialisation du RN2483...");

  resetRN2483();

  rnSerial.begin(57600, SERIAL_8N1, RN_PIN_RX, RN_PIN_TX);
  delay(1000);

  myLora.autobaud();
  delay(500);

  String version = myLora.sysver();
  Serial.printf("[LoRa] Firmware RN2483 : %s\n",
                version.length() > 0 ? version.c_str() : "non detecte - verifiez le cablage");

  // ── Lecture du DevEUI hardware ────────────────────────────
  String hwDeveui = myLora.hweui();
  hwDeveui.toUpperCase();

  Serial.printf("[LoRa] DevEUI hardware : %s\n", hwDeveui.c_str());

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

  const uint8_t MAX_ATTEMPTS = 3;

  for (uint8_t attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
  {
    Serial.printf("[LoRa] Tentative jointure OTAA %d/%d (module : %s)...\n",
                  attempt, MAX_ATTEMPTS, g_label.c_str());

    ledSet(0, 0, 180);   // LED bleue : tentative en cours

    bool joined = myLora.initOTAA(g_appeui, g_appkey, g_deveui);

    if (joined)
    {
      Serial.printf("[LoRa] OK Jointure OTAA reussie ! (module : %s)\n", g_label.c_str());
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
  Serial.printf("[LoRa]   - Les cles du module '%s' dans secret.h\n", g_label.c_str());
  Serial.println("[LoRa]   - L'enregistrement du device sur Orange Live Objects");
  Serial.println("[LoRa]   - La couverture reseau Orange LoRaWAN sur votre site");

  ledSet(200, 0, 0);   // LED rouge fixe : erreur fatale

  while (true)
  {
    delay(5000);
  }
}

// ---------------------------------------------------------------------------*
// @brief  Construit et envoie un payload LoRaWAN (compteur 16 bits en hex)
// @note   Remplacer le contenu par les données métier réelles
// @note   TX non confirmé, port 1 (hardcodé dans la librairie rn2xx3)
// ---------------------------------------------------------------------------*
void sendPayload()
{
  static uint16_t counter = 0;

  char payload[5];
  snprintf(payload, sizeof(payload), "%04X", counter);

  Serial.printf("[LoRa] Envoi payload : %s (compteur=%d, module=%s)\n",
                payload, counter, g_label.c_str());

  ledSet(60, 60, 60);   // LED blanche courte : envoi en cours

  TX_RETURN_TYPE result = myLora.txCommand("mac tx uncnf 1 ", String(payload), false);

  ledOff();

  switch (result)
  {
    case TX_SUCCESS:
      Serial.println("[LoRa] OK Trame envoyee");
      counter++;
      break;

    case TX_WITH_RX:
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
  bootWait();             // attente 10s + affichage infos de compilation
  runPeripheralTests();   // test LED et RN2483 avant jointure
  initLoRa();             // lecture DevEUI + lookup clés + jointure OTAA
}

void loop()
{
  sendPayload();
  delay(SEND_INTERVAL_MS);
}
