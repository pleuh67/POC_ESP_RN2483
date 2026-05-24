// ============================================================
// POC_ESP_2483
// ESP32-S3 + RN2483 - LoRaWAN OTAA - Réseau Orange (EU868)
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <rn2xx3.h>
#include <RTClib.h>
#include <Adafruit_BME280.h>
#include <Adafruit_INA219.h>
#include <HX711.h>
#include "config.h"
#include "node_data.h"
#include "functions.h"
#include "secret.h"   // table des modules + clés OTAA - non commité sur git

// ============================================================
// Objets globaux
// ============================================================
Adafruit_NeoPixel  led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial     rnSerial(RN_UART_NUM);
rn2xx3             myLora(rnSerial);

// Capteurs I2C
RTC_DS3231         rtc;                            // 0x68 — horloge temps réel
Adafruit_BME280    bme;                            // 0x76 — temp / humidité / pression
Adafruit_INA219    ina219(I2C_ADDR_INA219);        // 0x40 — tension / courant batterie

// Balance 1 — HX711 (GPIO10/11)
HX711              scale;

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
// @brief  Test DS3231 : vérifie la présence et lit la date/heure courante
// @return bool  True si le module répond
// ---------------------------------------------------------------------------*
bool testDS3231()
{
  Serial.println("[Test] DS3231 RTC...");
  if (!rtc.begin())
  {
    Serial.println("[Test] DS3231              ECHEC (non detecte sur I2C 0x68)");
    return false;
  }
  DateTime now = rtc.now();
  Serial.printf("[Test] DS3231              OK (%04d-%02d-%02d %02d:%02d:%02d)\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Test BME280 : vérifie la présence et lit temp/humidité/pression
// @return bool  True si le capteur répond
// ---------------------------------------------------------------------------*
bool testBME280()
{
  Serial.println("[Test] BME280...");
  if (!bme.begin(I2C_ADDR_BME280))
  {
    Serial.println("[Test] BME280              ECHEC (non detecte sur I2C 0x76)");
    return false;
  }
  Serial.printf("[Test] BME280              OK (%.1f C  %.1f %%  %.1f hPa)\n",
                bme.readTemperature(), bme.readHumidity(),
                bme.readPressure() / 100.0f);
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Lit la luminosité du MAX44009 via I2C direct (registres 0x03/0x04)
// @return float  Luminosité en lux, ou -1.0 si erreur
// ---------------------------------------------------------------------------*
float max44009ReadLux()
{
  Wire.beginTransmission(I2C_ADDR_LUX);
  Wire.write(0x03);
  if (Wire.endTransmission(false) != 0)
    return -1.0f;

  Wire.requestFrom((uint8_t)I2C_ADDR_LUX, (uint8_t)2);
  if (Wire.available() < 2)
    return -1.0f;

  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  // Formule MAX44009 : lux = 2^exp × mantisse × 0.045
  uint8_t exp      = (hi >> 4) & 0x0F;
  uint8_t mantissa = ((hi & 0x0F) << 4) | (lo & 0x0F);
  return (float)(1 << exp) * mantissa * 0.045f;
}

// ---------------------------------------------------------------------------*
// @brief  Test MAX44009 : vérifie la présence et lit la luminosité
// @return bool  True si le capteur répond
// ---------------------------------------------------------------------------*
bool testMAX44009()
{
  Serial.println("[Test] MAX44009...");
  float lux = max44009ReadLux();
  if (lux < 0.0f)
  {
    Serial.printf("[Test] MAX44009            ECHEC (non detecte sur 0x%02X)\n", I2C_ADDR_LUX);
    return false;
  }
  Serial.printf("[Test] MAX44009            OK (%.1f lux)\n", lux);
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Test INA219 : vérifie la présence et lit tension/courant batterie
// @return bool  True si le capteur répond
// ---------------------------------------------------------------------------*
bool testINA219()
{
  Serial.println("[Test] INA219...");
  if (!ina219.begin())
  {
    Serial.println("[Test] INA219              ECHEC (non detecte sur I2C 0x40)");
    return false;
  }
  Serial.printf("[Test] INA219              OK (%.2f V  %.1f mA)\n",
                ina219.getBusVoltage_V(),
                ina219.getCurrent_mA());
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Test HX711 : vérifie que la cellule de charge répond
// @return bool  True si le module est prêt
// ---------------------------------------------------------------------------*
bool testHX711()
{
  Serial.println("[Test] HX711 balance 1...");
  if (!scale.is_ready())
  {
    Serial.println("[Test] HX711              ECHEC (pas de reponse DOUT/SCK)");
    Serial.printf("[Test]   -> Verifiez GPIO%d (DOUT) et GPIO%d (SCK)\n",
                  HX711_PIN_DOUT, HX711_PIN_SCK);
    return false;
  }
  long  raw = scale.read_average(5);
  float kg  = scale.get_units(5);
  Serial.printf("[Test] HX711              OK (raw=%ld  %.3f kg — etalonner HX711_SCALE)\n",
                raw, kg);
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Initialise le bus I2C, les capteurs et le HX711
// @note   Appelé dans setup() avant runPeripheralTests()
// ---------------------------------------------------------------------------*
void initSensors()
{
  Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
  Wire.setClock(50000);    // 50 kHz — réduit pour diagnostic
  delay(500);              // attente stabilisation alimentation des modules
  Serial.printf("[I2C] Bus initialise (SDA=GPIO%d, SCL=GPIO%d, 100kHz)\n",
                I2C_PIN_SDA, I2C_PIN_SCL);

  // Diagnostic : les lignes doivent être HIGH au repos (pull-ups requis)
  bool sdaHigh = digitalRead(I2C_PIN_SDA);
  bool sclHigh = digitalRead(I2C_PIN_SCL);
  Serial.printf("[I2C] SDA=%s  SCL=%s",
                sdaHigh ? "HIGH" : "LOW (pull-up manquant?)",
                sclHigh ? "HIGH" : "LOW (pull-up manquant?)");
  if (!sdaHigh || !sclHigh)
    Serial.print("  -> Ajoutez 4.7k entre SDA/SCL et 3.3V");
  Serial.println();

  scanI2C();

  scale.begin(HX711_PIN_DOUT, HX711_PIN_SCK);
  scale.set_scale(HX711_SCALE);    // pas de tare : la balance est en charge au démarrage
  scale.set_offset(HX711_OFFSET);  // offset de zéro issu de la calibration
  Serial.printf("[HX711] Balance 1 initialisee (DOUT=GPIO%d, SCK=GPIO%d, scale=%.1f, offset=%ld)\n",
                HX711_PIN_DOUT, HX711_PIN_SCK, HX711_SCALE, (long)HX711_OFFSET);
}

// ---------------------------------------------------------------------------*
// @brief  Scanne le bus I2C et affiche les périphériques détectés
// @note   Appelé après Wire.begin() — aide au diagnostic de câblage
// ---------------------------------------------------------------------------*
void scanI2C()
{
  struct { uint8_t addr; const char* label; } known[] =
  {
    { I2C_ADDR_DS3231, "DS3231 RTC"     },
    { I2C_ADDR_EEPROM, "AT24C32 EEPROM" },
    { I2C_ADDR_BME280, "BME280"         },
    { I2C_ADDR_LUX,    "MAX44009"       },
    { I2C_ADDR_INA219, "INA219"         },
    { 0x00, nullptr }
  };

  Serial.println("[I2C] Scan du bus I2C :");
  Serial.println("[I2C] +---------+----------------------+");
  Serial.println("[I2C] | Adresse | Peripherique         |");
  Serial.println("[I2C] +---------+----------------------+");
  uint8_t found = 0;

  for (uint8_t addr = 0x01; addr < 0x7F; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      const char* label = "inconnu";
      for (uint8_t i = 0; known[i].label != nullptr; i++)
      {
        if (known[i].addr == addr) { label = known[i].label; break; }
      }
      Serial.printf("[I2C] |  0x%02X   | %-20s |\n", addr, label);
      found++;
    }
  }

  Serial.println("[I2C] +---------+----------------------+");
  if (found == 0)
    Serial.println("[I2C] Aucun peripherique detecte — verifiez SDA/SCL et alimentation");
  else
    Serial.printf("[I2C] %u peripherique(s) trouve(s)\n", found);
}

// ---------------------------------------------------------------------------*
// @brief  Lit tous les capteurs I2C et remplit un NodeData
// @param  node  Structure à remplir
// @note   vsol_v calculé depuis l'heure DS3231 (modèle jour/nuit)
// ---------------------------------------------------------------------------*
void readSensors(NodeData& node)
{
  node.rucher_id = RUCHER_ID;
  node.temp_c    = bme.readTemperature();
  node.hum_pct   = bme.readHumidity();

  float lux = max44009ReadLux();
  node.lux = (lux >= 0.0f) ? (uint16_t)lux : 0;

  node.vbat_v  = ina219.getBusVoltage_V();
  node.vsol_v  = computeVsol();   // modèle horaire via DS3231

  // Balance 1 — HX711 local
  node.poids1_kg = scale.is_ready() ? scale.get_units(5) : 0.0f;

  // Balances 2, 3, 4 — Bluetooth (non implémenté, valeurs simulées)
  node.poids2_kg = 15.0f + random(-100, 101) / 100.0f;
  node.poids3_kg = 20.0f + random(-100, 101) / 100.0f;
  node.poids4_kg = 25.0f + random(-100, 101) / 100.0f;
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
  testDS3231()  ? passed++ : failed++;
  testBME280()  ? passed++ : failed++;
  testMAX44009() ? passed++ : failed++;
  testINA219()  ? passed++ : failed++;
  testHX711()   ? passed++ : failed++;

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
// @brief  Encode et envoie un NodeData sur LoRaWAN (19 octets, little-endian)
// @param  node  Données du nœud à transmettre
// @note   TX non confirmé, port 1 (hardcodé dans la librairie rn2xx3)
// @note   Format : rucherID(1) temp(2) hum(2) lux(2) vbat(2) vsol(2) poids×4(8)
// ---------------------------------------------------------------------------*
void sendPayload(const NodeData& node)
{
  uint8_t buf[NODE_PAYLOAD_BYTES];

  // Octet 0 : RucherID (uint8)
  buf[0] = node.rucher_id;

  // Octets 1-2 : Température DHT22 (int16 ×100, little-endian)
  int16_t temp = (int16_t)(node.temp_c * 100.0f);
  buf[1] =  temp       & 0xFF;
  buf[2] = (temp >> 8) & 0xFF;

  // Octets 3-4 : Humidité DHT22 (uint16 ×100, little-endian)
  uint16_t hum = (uint16_t)(node.hum_pct * 100.0f);
  buf[3] =  hum       & 0xFF;
  buf[4] = (hum >> 8) & 0xFF;

  // Octets 5-6 : Luminosité (uint16, little-endian)
  buf[5] =  node.lux       & 0xFF;
  buf[6] = (node.lux >> 8) & 0xFF;

  // Octets 7-8 : Tension batterie (uint16 ×100, little-endian)
  uint16_t vbat = (uint16_t)(node.vbat_v * 100.0f);
  buf[7] =  vbat       & 0xFF;
  buf[8] = (vbat >> 8) & 0xFF;

  // Octets 9-10 : Tension solaire (uint16 ×100, little-endian)
  uint16_t vsol = (uint16_t)(node.vsol_v * 100.0f);
  buf[9]  =  vsol       & 0xFF;
  buf[10] = (vsol >> 8) & 0xFF;

  // Octets 11-12 : Poids Balance 1 (int16 ×100, little-endian)
  int16_t p1 = (int16_t)(node.poids1_kg * 100.0f);
  buf[11] =  p1       & 0xFF;
  buf[12] = (p1 >> 8) & 0xFF;

  // Octets 13-14 : Poids Balance 2 (int16 ×100, little-endian)
  int16_t p2 = (int16_t)(node.poids2_kg * 100.0f);
  buf[13] =  p2       & 0xFF;
  buf[14] = (p2 >> 8) & 0xFF;

  // Octets 15-16 : Poids Balance 3 (int16 ×100, little-endian)
  int16_t p3 = (int16_t)(node.poids3_kg * 100.0f);
  buf[15] =  p3       & 0xFF;
  buf[16] = (p3 >> 8) & 0xFF;

  // Octets 17-18 : Poids Balance 4 (int16 ×100, little-endian)
  int16_t p4 = (int16_t)(node.poids4_kg * 100.0f);
  buf[17] =  p4       & 0xFF;
  buf[18] = (p4 >> 8) & 0xFF;

  // Conversion en chaîne hexadécimale pour txCommand
  char hexStr[NODE_PAYLOAD_BYTES * 2 + 1];
  for (uint8_t i = 0; i < NODE_PAYLOAD_BYTES; i++)
  {
    snprintf(&hexStr[i * 2], 3, "%02X", buf[i]);
  }

  Serial.printf("[LoRa] Envoi %d octets : %s (module=%s)\n",
                NODE_PAYLOAD_BYTES, hexStr, g_label.c_str());
  Serial.printf("       RucherID : %u\n",       node.rucher_id);
  Serial.printf("       Temp     : %.2f C\n",  node.temp_c);
  Serial.printf("       Hum      : %.2f %%\n", node.hum_pct);
  Serial.printf("       Lux      : %u lux\n",  node.lux);
  Serial.printf("       Vbat     : %.2f V\n",  node.vbat_v);
  Serial.printf("       Vsol     : %.2f V\n",  node.vsol_v);
  Serial.printf("       Poids 1  : %.2f kg\n", node.poids1_kg);
  Serial.printf("       Poids 2  : %.2f kg\n", node.poids2_kg);
  Serial.printf("       Poids 3  : %.2f kg\n", node.poids3_kg);
  Serial.printf("       Poids 4  : %.2f kg\n", node.poids4_kg);

  ledSet(60, 60, 60);   // LED blanche courte : envoi en cours

  TX_RETURN_TYPE result = myLora.txCommand("mac tx uncnf 1 ", String(hexStr), false);

  ledOff();

  switch (result)
  {
    case TX_SUCCESS:
      Serial.println("[LoRa] OK Trame envoyee");
      break;

    case TX_WITH_RX:
      Serial.println("[LoRa] OK Trame envoyee + downlink recu");
      break;

    case TX_FAIL:
      Serial.println("[LoRa] ERREUR envoi - verifiez la connexion au reseau");
      break;

    default:
      Serial.printf("[LoRa] Resultat inattendu : %d\n", (int)result);
      break;
  }
}

// ---------------------------------------------------------------------------*
// @brief  Calcule la tension du panneau solaire selon l'heure DS3231
// @return float  0.0V (nuit), 6.0V (jour), transition linéaire matin/soir
// @note   Transitions : montée 05h00→08h00, descente 20h00→22h00
// ---------------------------------------------------------------------------*
float computeVsol()
{
  DateTime now = rtc.now();
  float h = now.hour() + now.minute() / 60.0f;

  if (h < 5.0f || h >= 22.0f)
    return 0.0f;                                  // nuit
  if (h >= 8.0f && h < 20.0f)
    return 6.0f;                                  // plein jour
  if (h < 8.0f)
    return 6.0f * (h - 5.0f) / 3.0f;             // montée  05h→08h
  return 6.0f * (1.0f - (h - 20.0f) / 2.0f);    // descente 20h→22h
}

// ---------------------------------------------------------------------------*
// @brief  Calibration HX711 en deux points : tare (0 kg) + poids de référence
// @note   Mesure 10 échantillons à chaque étape — résultat à reporter dans config.h
// @note   Attend une saisie clavier entre les deux étapes
// ---------------------------------------------------------------------------*
void calibrateHX711()
{
  Serial.println();
  Serial.println("============================================================");
  Serial.println("  CALIBRATION HX711 - Balance 1");
  Serial.println("============================================================");

  // ── Étape 1 : tare à vide ─────────────────────────────────
  Serial.println("  Etape 1/2 : balance a VIDE");
  Serial.println("  -> Retirez tout poids, puis appuyez sur Entree...");
  while (!Serial.available()) { delay(10); }
  while (Serial.available())  { Serial.read(); }

  scale.set_scale();
  scale.tare(10);
  long rawTare = scale.get_offset();
  Serial.printf("  Tare OK  - valeur brute a vide : %ld\n", rawTare);

  // ── Étape 2 : poids de référence ──────────────────────────
  Serial.printf("  Etape 2/2 : posez %.2f kg sur la balance\n", HX711_CALIB_KG);
  Serial.println("  -> Poids en place, appuyez sur Entree...");
  while (!Serial.available()) { delay(10); }
  while (Serial.available())  { Serial.read(); }

  long  rawCharge = scale.read_average(10);
  float newScale  = (float)(rawCharge - rawTare) / HX711_CALIB_KG;
  scale.set_scale(newScale);

  Serial.println("------------------------------------------------------------");
  Serial.printf("  Tare          : %ld\n",   rawTare);
  Serial.printf("  Brut charge   : %ld\n",   rawCharge);
  Serial.printf("  Nouveau scale : %.2f\n",  newScale);
  Serial.printf("  Verification  : %.3f kg (doit etre %.2f kg)\n",
                scale.get_units(5), HX711_CALIB_KG);
  Serial.println("------------------------------------------------------------");
  Serial.println("  -> Mettez a jour dans include/config.h :");
  Serial.printf( "     #define HX711_SCALE   %.2ff\n", newScale);
  Serial.printf( "     #define HX711_OFFSET  %ldL\n",  rawTare);
  Serial.println("============================================================");
  Serial.println();
}

// ---------------------------------------------------------------------------*
// @brief  Affiche toutes les mesures sur une ligne formatée dans le terminal
// @param  node  Données à afficher
// ---------------------------------------------------------------------------*
void printMeasures(const NodeData& node)
{
  Serial.printf("[DATA] R:%u | T:%+.1fC H:%.1f%% Lux:%4u | Vbat:%.2fV Vsol:%.2fV | P1:%6.2fkg P2:%6.2fkg P3:%6.2fkg P4:%6.2fkg\n",
                node.rucher_id,
                node.temp_c, node.hum_pct, node.lux,
                node.vbat_v, node.vsol_v,
                node.poids1_kg, node.poids2_kg, node.poids3_kg, node.poids4_kg);
}

// ============================================================
// Setup & Loop
// ============================================================
void setup()
{
  bootWait();             // attente 10s + affichage infos de compilation
  initSensors();          // bus I2C + HX711
  randomSeed(esp_random()); // graine pour balances BT simulées (2/3/4)
  runPeripheralTests();   // test LED, RN2483 et capteurs I2C
  initLoRa();             // lecture DevEUI + lookup clés + jointure OTAA
}

// ---------------------------------------------------------------------------*
// @brief  Traite les commandes reçues sur le port série
// @note   'c' → calibration HX711   '?' → aide
// ---------------------------------------------------------------------------*
void handleSerial()
{
  if (!Serial.available())
    return;

  char cmd = Serial.read();
  while (Serial.available()) Serial.read();   // vide le buffer

  switch (cmd)
  {
    case 'c': case 'C':
      calibrateHX711();
      break;
    case '?':
      Serial.println("[CMD] Commandes disponibles :");
      Serial.println("[CMD]   c  -> calibration HX711 (2 points)");
      Serial.println("[CMD]   ?  -> cette aide");
      break;
    default:
      break;
  }
}

void loop()
{
  static uint32_t lastMeasure = 0;
  static uint32_t lastSend    = 0;
  uint32_t now = millis();

  handleSerial();

  if (now - lastMeasure >= MEASURE_INTERVAL_MS)
  {
    lastMeasure = now;
    NodeData node = {};
    readSensors(node);
    printMeasures(node);

    if (now - lastSend >= SEND_INTERVAL_MS)
    {
      lastSend = now;
      sendPayload(node);
    }
  }
}
