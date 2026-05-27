// ============================================================
// POC_ESP_2483
// ESP32-S3 + RN2483 - LoRaWAN OTAA - Réseau Orange (EU868)
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>    // esp_log_level_set() — suppression sélective des logs ESP-IDF
#include <Adafruit_NeoPixel.h>
#include <rn2xx3.h>
#include <RTClib.h>
#include <Adafruit_BME280.h>
#include <Adafruit_INA219.h>
#include <HX711.h>
#include "config.h"
#include "node_data.h"
#include "node_config.h"
#include "functions.h"
#include "secret.h"   // table des modules + clés OTAA - non commité sur git
#include <I2C_eeprom.h>
#include <BH1750.h>
#include <Adafruit_SH110X.h>

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

// Capteur de luminosité détecté au boot
enum LuxSensor { LUX_NONE, LUX_BH1750, LUX_MAX44009 };
LuxSensor          g_luxSensor = LUX_NONE;
BH1750             bh1750;

// OLED SH1106 128×64 — log de démarrage et affichage d'erreurs
Adafruit_SH1106G   oled(128, 64, &Wire, -1);       // SDA/SCL partagés, pas de RST
static char        g_oledBuf[OLED_ROWS][OLED_COLS + 1];
static bool        g_oledReady = false;

// EEPROM AT24C32 (0x57) — config persistante
I2C_eeprom         eeprom(I2C_ADDR_EEPROM, 4096);

// Configuration active du nœud (chargée depuis EEPROM au boot)
NodeConfig         g_config;

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
// @brief  Initialise l'écran OLED SH1106 — tente 0x3C puis 0x3D
// @note   Appelé depuis initSensors() après Wire.begin()
// ---------------------------------------------------------------------------*
void oledInit()
{
  if (!oled.begin(I2C_ADDR_OLED, false) && !oled.begin(0x3D, false))
  {
    Serial.println("[OLED] ATTENTION : ecran non detecte (0x3C et 0x3D essayes)");
    return;
  }
  g_oledReady = true;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setTextWrap(false);
  memset(g_oledBuf, 0, sizeof(g_oledBuf));
  oled.display();
  Serial.println("[OLED] Initialise OK");
}

// ---------------------------------------------------------------------------*
// @brief  Ajoute une ligne au scroll OLED (printf-style, max OLED_COLS chars)
// @note   Toujours actif — permet de voir les erreurs sans port série
// ---------------------------------------------------------------------------*
void oledAddLine(const char* fmt, ...)
{
  char line[OLED_COLS + 1];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);

  if (!g_oledReady) return;

  // Scroll vers le haut
  for (uint8_t i = 0; i < OLED_ROWS - 1; i++)
    memcpy(g_oledBuf[i], g_oledBuf[i + 1], sizeof(g_oledBuf[0]));
  strncpy(g_oledBuf[OLED_ROWS - 1], line, OLED_COLS);
  g_oledBuf[OLED_ROWS - 1][OLED_COLS] = '\0';

  oled.clearDisplay();
  for (uint8_t i = 0; i < OLED_ROWS; i++)
  {
    oled.setCursor(0, i * 8);
    oled.print(g_oledBuf[i]);
  }
  oled.display();
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
  float t = bme.readTemperature();
  if (isnan(t))
  {
    Serial.println("[Test] BME280              ECHEC (NaN — verifier adresse 0x76/0x77 et cablage)");
    return false;
  }
  Serial.printf("[Test] BME280              OK (%.1f C  %.1f %%  %.1f hPa)\n",
                t, bme.readHumidity(), bme.readPressure() / 100.0f);
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
// @brief  Lit la luminosité via le capteur détecté au boot (BH1750 ou MAX44009)
// @return float  Luminosité en lux, ou -1.0 si aucun capteur disponible
// ---------------------------------------------------------------------------*
float readLux()
{
  switch (g_luxSensor)
  {
    case LUX_BH1750:   return bh1750.readLightLevel();
    case LUX_MAX44009: return max44009ReadLux();
    default:           return -1.0f;
  }
}

// ---------------------------------------------------------------------------*
// @brief  Test du capteur de luminosité détecté (BH1750 ou MAX44009)
// @return bool  True si une mesure valide est obtenue
// ---------------------------------------------------------------------------*
bool testLuxSensor()
{
  const char* name = (g_luxSensor == LUX_BH1750) ? "BH1750" : "MAX44009";
  Serial.printf("[Test] %s...\n", name);

  float lux = readLux();
  if (lux < 0.0f)
  {
    Serial.printf("[Test] %-10s          ECHEC (aucun capteur detecte)\n", name);
    return false;
  }
  Serial.printf("[Test] %-10s          OK (%.1f lux)\n", name, lux);
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
// @brief  CRC32 (polynôme zip) sur un bloc mémoire
// @param  data  Pointeur sur les données
// @param  len   Nombre d'octets
// @return uint32_t  Valeur CRC32
// ---------------------------------------------------------------------------*
static uint32_t crc32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc >> 1) ^ (0xEDB88320UL & -(crc & 1));
  }
  return ~crc;
}

// ---------------------------------------------------------------------------*
// @brief  Remplit g_config avec les valeurs par défaut de config.h
// ---------------------------------------------------------------------------*
void initDefaultConfig()
{
  g_config.rucher_id         = DEFAULT_RUCHER_ID;
  strncpy(g_config.label, DEFAULT_LABEL, sizeof(g_config.label) - 1);
  g_config.label[sizeof(g_config.label) - 1] = '\0';
  g_config.hx711_scale       = DEFAULT_HX711_SCALE;
  g_config.hx711_offset      = DEFAULT_HX711_OFFSET;
  g_config.vbat_factor       = DEFAULT_VBAT_FACTOR;
  g_config.vsol_factor       = DEFAULT_VSOL_FACTOR;
  g_config.send_interval_min = DEFAULT_SEND_INTERVAL_MIN;
  memset(g_config._reserved, 0, sizeof(g_config._reserved));
  g_config.crc = crc32((const uint8_t*)&g_config, NODE_CONFIG_DATA_SIZE);
}

// ---------------------------------------------------------------------------*
// @brief  Charge la config depuis l'EEPROM AT24C32 ; repli sur valeurs par défaut
// @return bool  True si CRC valide, false si valeurs par défaut appliquées
// @note   Wire doit être initialisé avant d'appeler cette fonction
// ---------------------------------------------------------------------------*
bool loadConfig()
{
  Serial.println("[Config] Chargement depuis EEPROM...");

  if (!eeprom.begin())
  {
    Serial.println("[Config] EEPROM absente — valeurs par defaut appliquees");
    oledAddLine("EEPROM absent!");
    initDefaultConfig();
    return false;
  }

  eeprom.readBlock(EEPROM_ADDR_CONFIG, (uint8_t*)&g_config, sizeof(NodeConfig));

  uint32_t expected = crc32((const uint8_t*)&g_config, NODE_CONFIG_DATA_SIZE);
  if (expected != g_config.crc)
  {
    Serial.printf("[Config] CRC invalide (lu=0x%08X calc=0x%08X) — valeurs par defaut\n",
                  g_config.crc, expected);
    oledAddLine("EEPROM CRC FAIL");
    initDefaultConfig();
    return false;
  }

  oledAddLine("EEPROM OK");
  Serial.println("------------------------------------------------------------");
  Serial.println("  CONFIG EEPROM OK");
  Serial.println("------------------------------------------------------------");
  Serial.printf("  Label          : %s\n",    g_config.label);
  Serial.printf("  Rucher ID      : %u\n",    g_config.rucher_id);
  Serial.printf("  HX711 scale    : %.2f\n",  g_config.hx711_scale);
  Serial.printf("  HX711 offset   : %ld\n",   g_config.hx711_offset);
  Serial.printf("  Vbat factor    : %.3f\n",  g_config.vbat_factor);
  Serial.printf("  Vsol factor    : %.3f\n",  g_config.vsol_factor);
  Serial.printf("  Send interval  : %u min\n", g_config.send_interval_min);
  Serial.println("------------------------------------------------------------");
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Sauvegarde g_config dans l'EEPROM AT24C32 avec CRC recalculé
// @return bool  True si écriture réussie
// ---------------------------------------------------------------------------*
bool saveConfig()
{
  g_config.crc = crc32((const uint8_t*)&g_config, NODE_CONFIG_DATA_SIZE);

  if (!eeprom.begin())
  {
    Serial.println("[Config] ERREUR : EEPROM absente — sauvegarde impossible");
    return false;
  }

  eeprom.writeBlock(EEPROM_ADDR_CONFIG, (uint8_t*)&g_config, sizeof(NodeConfig));
  Serial.println("[Config] Config sauvegardee en EEPROM OK");
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

  scanI2C();       // scan avant toute init — garantit la détection de tous les périphériques
  oledInit();      // init OLED après le scan
  oledAddLine("POC ESP RN2483");
  loadConfig();    // charge g_config depuis EEPROM (ou valeurs par défaut)

  // BME280 — tente 0x76 puis 0x77 (selon câblage broche SDO)
  if (!bme.begin(0x76) && !bme.begin(0x77))
  {
    Serial.println("[BME280] ATTENTION : capteur non detecte (0x76 et 0x77 essayes)");
    oledAddLine("BME280 absent!");
  }
  else
  {
    Serial.println("[BME280] Initialise OK");
  }

  // Luminosité — BH1750 prioritaire, fallback MAX44009
  if (bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, I2C_ADDR_BH1750_LOW, &Wire))
  {
    g_luxSensor = LUX_BH1750;
    Serial.printf("[LUX] BH1750 detecte (0x%02X)\n", I2C_ADDR_BH1750_LOW);
  }
  else if (bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, I2C_ADDR_BH1750_HIGH, &Wire))
  {
    g_luxSensor = LUX_BH1750;
    Serial.printf("[LUX] BH1750 detecte (0x%02X)\n", I2C_ADDR_BH1750_HIGH);
  }
  else if (max44009ReadLux() >= 0.0f)
  {
    g_luxSensor = LUX_MAX44009;
    Serial.printf("[LUX] MAX44009 detecte (0x%02X)\n", I2C_ADDR_MAX44009);
  }
  else
  {
    g_luxSensor = LUX_NONE;
    Serial.println("[LUX] ATTENTION : aucun capteur de luminosite detecte");
    oledAddLine("LUX absent!");
  }

  // DS3231 — requis avant computeVsol() et rtc.now()
  if (!rtc.begin())
    Serial.println("[DS3231] ATTENTION : RTC non detectee sur I2C 0x68");
  else
    Serial.println("[DS3231] Initialisee OK");

  // INA219 — requis avant getBusVoltage_V() / getCurrent_mA()
  if (!ina219.begin())
  {
    Serial.println("[INA219] ATTENTION : capteur non detecte sur I2C 0x40");
    oledAddLine("INA219 absent!");
  }
  else
  {
    Serial.println("[INA219] Initialisee OK");
  }

  scale.begin(HX711_PIN_DOUT, HX711_PIN_SCK);
  scale.set_scale(g_config.hx711_scale);
  scale.set_offset(g_config.hx711_offset);
  Serial.printf("[HX711] Balance 1 initialisee (DOUT=GPIO%d, SCK=GPIO%d, scale=%.1f, offset=%ld)\n",
                HX711_PIN_DOUT, HX711_PIN_SCK, g_config.hx711_scale, g_config.hx711_offset);
}

// ---------------------------------------------------------------------------*
// @brief  Scanne le bus I2C et affiche les périphériques détectés
// @note   Appelé après Wire.begin() — aide au diagnostic de câblage
// ---------------------------------------------------------------------------*
void scanI2C()
{
  struct { uint8_t addr; const char* label; } known[] =
  {
    { I2C_ADDR_OLED,   "SH1106 OLED"    },
    { 0x3D,            "SH1106 OLED"    },
    { I2C_ADDR_DS3231, "DS3231 RTC"     },
    { I2C_ADDR_EEPROM, "AT24C32 EEPROM" },
    { I2C_ADDR_BME280, "BME280"         },
    { I2C_ADDR_MAX44009,    "MAX44009"       },
    { I2C_ADDR_BH1750_LOW,  "BH1750 (ADDR=0)" },
    { I2C_ADDR_BH1750_HIGH, "BH1750 (ADDR=1)" },
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
  node.rucher_id = g_config.rucher_id;
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  node.temp_c  = isnan(t) ? 0.0f : t;
  node.hum_pct = isnan(h) ? 0.0f : h;

  float lux = readLux();
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
  bool    ok;

  ok = testLed();
  ok ? passed++ : failed++;
  oledAddLine("LED    %s", ok ? "OK" : "FAIL");

  ok = testRN2483();
  ok ? passed++ : failed++;
  oledAddLine("RN2483 %s", ok ? "OK" : "FAIL");

  ok = testDS3231();
  ok ? passed++ : failed++;
  if (ok) { DateTime t = rtc.now();
    oledAddLine("DS3231 %02d:%02d:%02d", t.hour(), t.minute(), t.second()); }
  else oledAddLine("DS3231 FAIL");

  ok = testBME280();
  ok ? passed++ : failed++;
  if (ok)
    oledAddLine("BME280 %.1fC %.0f%%", bme.readTemperature(), bme.readHumidity());
  else oledAddLine("BME280 FAIL");

  ok = testLuxSensor();
  ok ? passed++ : failed++;
  if (ok)
    oledAddLine("LUX    %.0f lux", readLux());
  else oledAddLine("LUX    FAIL");

  ok = testINA219();
  ok ? passed++ : failed++;
  if (ok)
    oledAddLine("INA219 %.2fV", ina219.getBusVoltage_V());
  else oledAddLine("INA219 FAIL");

  ok = testHX711();
  ok ? passed++ : failed++;
  if (ok)
    oledAddLine("HX711  %.2fkg", scale.get_units(3));
  else oledAddLine("HX711  FAIL");

  oledAddLine("%d OK  %d FAIL", passed, failed);

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
// @brief  Synchronise le DS3231 depuis l'heure GPS fournie par le réseau LoRaWAN
// @return bool  True si synchronisation réussie
// @note   Requiert firmware RN2483 >= 1.0.5 et support DeviceTimeReq côté serveur
// @note   Conversion : GPS epoch (6 jan 1980) + 315964800 − 18 leap s → Unix UTC
// ---------------------------------------------------------------------------*
bool syncRtcFromLoRa()
{
  Serial.println("[RTC] Tentative synchro heure via LoRa...");

  String response = myLora.sendRawCommand("mac get gpstime");
  response.trim();
  Serial.printf("[RTC] Reponse gpstime : '%s'\n", response.c_str());

  if (response.length() == 0 || response == "0" || response.startsWith("invalid"))
  {
    Serial.println("[RTC] GPS non disponible (firmware < 1.0.5 ou reseau non supporté)");
    oledAddLine("RTC: GPS indispo");
    return false;
  }

  uint32_t gpsSeconds = (uint32_t)strtoul(response.c_str(), nullptr, 10);

  // Valeur plausible : > 1er janvier 2023 en temps GPS ≈ 1 356 000 000 s
  if (gpsSeconds < 1356000000UL)
  {
    Serial.printf("[RTC] Valeur GPS hors plage (%lu) — synchro ignoree\n",
                  (unsigned long)gpsSeconds);
    oledAddLine("RTC: GPS invalide");
    return false;
  }

  // GPS epoch (6 jan 1980) → Unix UTC : +315 964 800 s, −18 leap seconds
  uint32_t unixLocal = gpsSeconds + 315964800UL - 18UL
                       + (uint32_t)(TIMEZONE_OFFSET_H * 3600L);

  DateTime dt(unixLocal);
  rtc.adjust(dt);

  Serial.printf("[RTC] Synchro OK : %04d-%02d-%02d %02d:%02d:%02d (UTC+%d)\n",
                dt.year(), dt.month(), dt.day(),
                dt.hour(), dt.minute(), dt.second(),
                TIMEZONE_OFFSET_H);
  oledAddLine("RTC %02d:%02d UTC+%d",
              dt.hour(), dt.minute(), TIMEZONE_OFFSET_H);
  return true;
}

// ---------------------------------------------------------------------------*
// @brief  Envoie une trame de notification de redémarrage sur le port LoRaWAN 2
// @note   Payload : 1 octet 0x01 — permet de détecter un redémarrage côté serveur
// @note   Port 2 distinct des trames data (port 1) pour filtrage applicatif
// ---------------------------------------------------------------------------*
void sendRestartPayload()
{
  Serial.println("[LoRa] Envoi notification de redemarrage (port 2)...");
  oledAddLine("TX Restart...");

  ledSet(60, 60, 60);   // LED blanche : envoi en cours
  TX_RETURN_TYPE result = myLora.txCommand("mac tx uncnf 2 ", "52657374617274", false);  // "Restart" ASCII
  ledOff();

  bool ok = (result == TX_SUCCESS || result == TX_WITH_RX);
  Serial.printf("[LoRa] Notification redemarrage : %s\n", ok ? "OK" : "KO");
  oledAddLine("Restart TX %s", ok ? "OK" : "KO");

  if (result == TX_WITH_RX)
    syncRtcFromLoRa();
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
    oledAddLine("LoRa join %d/%d...", attempt, MAX_ATTEMPTS);

    ledSet(0, 0, 180);   // LED bleue : tentative en cours

    bool joined = myLora.initOTAA(g_appeui, g_appkey, g_deveui);

    if (joined)
    {
      Serial.printf("[LoRa] OK Jointure OTAA reussie ! (module : %s)\n", g_label.c_str());
      oledAddLine("LoRa JOIN OK");
      ledSet(0, 200, 0);   // LED verte : succès
      delay(2000);
      ledOff();
      syncRtcFromLoRa();   // synchro RTC depuis l'heure réseau

      DateTime now = rtc.now();
      Serial.printf("[RTC] Date/heure : %04d-%02d-%02d %02d:%02d:%02d\n",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second());
      oledAddLine("%02d/%02d/%04d %02d:%02d",
                  now.day(), now.month(), now.year(),
                  now.hour(), now.minute());
      sendRestartPayload();
      return;
    }

    ledSet(200, 0, 0);   // LED rouge : échec
    Serial.println("[LoRa] Echec jointure - nouvelle tentative dans 10s");
    oledAddLine("LoRa echec %d/%d", attempt, MAX_ATTEMPTS);
    delay(10000);
    ledOff();
  }

  Serial.println("[LoRa] ERREUR : Jointure impossible apres 3 tentatives.");
  Serial.println("[LoRa]   Verifiez :");
  Serial.printf("[LoRa]   - Les cles du module '%s' dans secret.h\n", g_label.c_str());
  Serial.println("[LoRa]   - L'enregistrement du device sur Orange Live Objects");
  Serial.println("[LoRa]   - La couverture reseau Orange LoRaWAN sur votre site");
  oledAddLine("LoRa ECHEC!");
  oledAddLine("Verif cles/reseau");

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

  bool txOk = false;
  switch (result)
  {
    case TX_SUCCESS:
      Serial.println("[LoRa] OK Trame envoyee");
      txOk = true;
      break;

    case TX_WITH_RX:
      Serial.println("[LoRa] OK Trame envoyee + downlink recu");
      txOk = true;
      syncRtcFromLoRa();   // profite du downlink pour resynchroniser le RTC
      break;

    case TX_FAIL:
      Serial.println("[LoRa] ERREUR envoi - verifiez la connexion au reseau");
      break;

    default:
      Serial.printf("[LoRa] Resultat inattendu : %d\n", (int)result);
      break;
  }

  DateTime ts = rtc.now();
  if (txOk)
    oledAddLine("%02d:%02d %.1fkg %.1fC",
                ts.hour(), ts.minute(), node.poids1_kg, node.temp_c);
  else
    oledAddLine("%02d:%02d %.1fkg %.0fC KO",
                ts.hour(), ts.minute(), node.poids1_kg, node.temp_c);
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
  Serial.println("  Etape 2/2 : posez un poids de reference sur la balance");
  Serial.printf( "  -> Saisissez le poids en kg [%.3f] puis Entree : ", HX711_CALIB_KG);

  char    inputBuf[16] = {};
  uint8_t inputLen     = 0;
  while (true)
  {
    if (Serial.available())
    {
      char c = Serial.read();
      if (c == '\n' || c == '\r')
      {
        delay(5);
        while (Serial.available()) Serial.read();  // vide \r\n résiduel
        break;
      }
      if (inputLen < sizeof(inputBuf) - 1)
        inputBuf[inputLen++] = c;
    }
    delay(5);
  }

  float calibKg = (inputLen > 0) ? atof(inputBuf) : 0.0f;
  if (calibKg <= 0.0f)
    calibKg = HX711_CALIB_KG;
  Serial.printf("  -> Poids utilise : %.3f kg\n", calibKg);

  long  rawCharge = scale.read_average(10);
  float newScale  = (float)(rawCharge - rawTare) / calibKg;
  scale.set_scale(newScale);

  g_config.hx711_scale  = newScale;
  g_config.hx711_offset = rawTare;

  Serial.println("------------------------------------------------------------");
  Serial.printf("  Tare          : %ld\n",   rawTare);
  Serial.printf("  Brut charge   : %ld\n",   rawCharge);
  Serial.printf("  Nouveau scale : %.2f\n",  newScale);
  Serial.printf("  Verification  : %.3f kg (doit etre %.3f kg)\n",
                scale.get_units(5), calibKg);
  Serial.println("------------------------------------------------------------");
  saveConfig();
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
  esp_log_level_set("Wire",     ESP_LOG_NONE);  // supprime les erreurs Wire ESP-IDF
  esp_log_level_set("Wire.cpp", ESP_LOG_NONE);  // supprime les erreurs Wire Arduino HAL
  bootWait();             // attente 10s + affichage infos de compilation
  initSensors();          // bus I2C + HX711
  randomSeed(esp_random()); // graine pour balances BT simulées (2/3/4)
  runPeripheralTests();   // test LED, RN2483 et capteurs I2C
  initLoRa();             // lecture DevEUI + lookup clés + jointure OTAA
}

// ---------------------------------------------------------------------------*
// @brief  Traite les commandes reçues sur le port série (ligne complète)
// @note   c → calibration HX711
// @note   h HH:MM → régler l'heure du DS3231
// @note   d DD/MM/YYYY → régler la date du DS3231
// @note   ? → aide
// ---------------------------------------------------------------------------*
void handleSerial()
{
  if (!Serial.available())
    return;

  // Lecture ligne complète (même logique que la saisie de calibration)
  char    lineBuf[32] = {};
  uint8_t len         = 0;
  while (true)
  {
    if (Serial.available())
    {
      char c = Serial.read();
      if (c == '\r' || c == '\n')
      {
        delay(5);
        while (Serial.available()) Serial.read();
        break;
      }
      if (len < sizeof(lineBuf) - 1)
        lineBuf[len++] = c;
    }
  }

  if (len == 0) return;

  char        cmd = lineBuf[0];
  const char* arg = (len > 2) ? &lineBuf[2] : "";   // argument après "x "

  switch (cmd)
  {
    case 'c': case 'C':
      calibrateHX711();
      break;

    case 'h': case 'H':
    {
      int hh = 0, mm = 0;
      if (sscanf(arg, "%d:%d", &hh, &mm) == 2
          && hh >= 0 && hh < 24 && mm >= 0 && mm < 60)
      {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), hh, mm, 0));
        Serial.printf("[CMD] Heure reglée : %02d:%02d:00\n", hh, mm);
        oledAddLine("Heure %02d:%02d", hh, mm);
      }
      else
      {
        Serial.println("[CMD] Format invalide — usage : h HH:MM  (ex: h 23:45)");
      }
      break;
    }

    case 'd': case 'D':
    {
      int dd = 0, mo = 0, yyyy = 0;
      if (sscanf(arg, "%d/%d/%d", &dd, &mo, &yyyy) == 3
          && dd >= 1 && dd <= 31 && mo >= 1 && mo <= 12 && yyyy >= 2000)
      {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(yyyy, mo, dd, now.hour(), now.minute(), now.second()));
        Serial.printf("[CMD] Date reglée : %02d/%02d/%04d\n", dd, mo, yyyy);
        oledAddLine("Date %02d/%02d/%04d", dd, mo, yyyy);
      }
      else
      {
        Serial.println("[CMD] Format invalide — usage : d DD/MM/YYYY  (ex: d 27/05/2026)");
      }
      break;
    }

    case '?':
      Serial.println("[CMD] Commandes disponibles :");
      Serial.println("[CMD]   c              -> calibration HX711 (2 points)");
      Serial.println("[CMD]   h HH:MM        -> regler l'heure  (ex: h 23:45)");
      Serial.println("[CMD]   d DD/MM/YYYY   -> regler la date  (ex: d 27/05/2026)");
      Serial.println("[CMD]   ?              -> cette aide");
      break;

    default:
      break;
  }
}

void loop()
{
  static uint32_t lastMeasure   = 0;
  static uint32_t lastSend      = 0;
  static uint32_t lastHeartbeat = 0;
  uint32_t now = millis();

  handleSerial();

#if HEARTBEAT_INTERVAL_MS > 0
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS)
  {
    lastHeartbeat = now;
    ledSet(0, 80, 0);   // vert bref : programme actif
    delay(30);
    ledOff();
  }
#endif

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
