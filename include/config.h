#pragma once

// ============================================================
// POC_ESP_2483 - Configuration matérielle et paramètres LoRa
// Ce fichier EST commité sur git.
// Les clés de connexion sont dans secret.h (non commité).
// ============================================================

// ============================================================
// LED RGB WS2812 intégrée (GPIO48 sur ESP32-S3-DevKitC-1)
// ============================================================
#define LED_PIN           48     // GPIO de la LED WS2812
#define LED_COUNT         1      // Nombre de LEDs sur la chaîne

// ============================================================
// Séquence de boot : attente ouverture terminal série
// ============================================================
#define BOOT_WAIT_MS      10000  // Durée totale d'attente (ms)
#define BOOT_LED_ON_MS    250    // LED allumée (ms)
#define BOOT_LED_OFF_MS   750    // LED éteinte (ms)
// Couleur de clignotement pendant l'attente : orange
#define BOOT_LED_R        255
#define BOOT_LED_G        80
#define BOOT_LED_B        0
// Couleur fixe quand le programme démarre : vert
#define BOOT_READY_R      0
#define BOOT_READY_G      200
#define BOOT_READY_B      0

// ============================================================
// Broches UART1 vers RN2483 (adaptez à votre câblage)
// ============================================================
#define RN_UART_NUM   1      // HardwareSerial(1) = UART1
#define RN_PIN_RX     4      // GPIO4  <- TX du RN2483
#define RN_PIN_TX     5      // GPIO5  <- RX du RN2483
#define RN_PIN_RST    6      // GPIO6  <- RST du RN2483

// ============================================================
// Bus I2C (SDA/SCL) — capteurs et périphériques I2C
// ============================================================
#define I2C_PIN_SDA   35     // GPIO35 — SDA
#define I2C_PIN_SCL   36     // GPIO36 — SCL

// Broches HX711 — Balance 1 (connectée directement à l'ESP32-S3)
#define HX711_PIN_DOUT  10   // GPIO10 — données HX711
#define HX711_PIN_SCK   11   // GPIO11 — horloge HX711
#define HX711_CALIB_KG       11.0f // 2.5f  // poids de référence pour la calibration (kg)

// Adresses I2C des périphériques
#define I2C_ADDR_OLED     0x3C  // SH1106 OLED 128×64 (SA0=GND → 0x3C, SA0=VCC → 0x3D)
#define OLED_COLS         20    // caractères par ligne (font 6×8, 128px)
#define OLED_ROWS          8    // lignes visibles (font 6×8, 64px)

#define I2C_ADDR_DS3231   0x68  // RTC temps réel
#define I2C_ADDR_EEPROM   0x57  // AT24C32 — stockage persistant
#define I2C_ADDR_BME280   0x76  // Temp / Humidité / Pression
#define I2C_ADDR_MAX44009     0x4A  // MAX44009 — luminosité (ADDR=GND)
#define I2C_ADDR_BH1750_LOW   0x23  // BH1750   — luminosité (ADDR=GND)
#define I2C_ADDR_BH1750_HIGH  0x5C  // BH1750   — luminosité (ADDR=VCC)
#define I2C_ADDR_LUX          I2C_ADDR_MAX44009  // alias générique (résolu à l'init)
#define I2C_ADDR_INA219   0x40  // Tension / Courant batterie

// ============================================================
// Bouton poussoir — envoi forcé d'un payload LoRa
// ============================================================
#define BUTTON_PIN        7      // GPIO7 — bouton vers GND (INPUT_PULLUP interne)
#define BUTTON_DEBOUNCE_MS 50    // Anti-rebond (ms)

// ============================================================
// Paramètres d'envoi
// ============================================================
#define SEND_INTERVAL_MS     (DEFAULT_SEND_INTERVAL_MIN * 60000UL)  // Converti en ms
#define MEASURE_INTERVAL_MS   10000UL                       // Affichage terminal (dev)
#define HEARTBEAT_INTERVAL_MS  5000UL                       // Flash vert LED (ms) — 0 = désactivé
#define TIMEZONE_OFFSET_H      2                            // Décalage UTC → heure locale (2=été, 1=hiver)
// Note : le port LoRaWAN est hardcodé à 1 dans la librairie rn2xx3 (txCommand "mac tx uncnf 1")

// ============================================================
// Valeurs par défaut de NodeConfig
// Utilisées si l'EEPROM est vierge, corrompue ou absente.
// En mode développement : modifier ici, puis recalibrer et sauver en EEPROM.
// ============================================================
#define DEFAULT_RUCHER_ID          42         // Identifiant rucher (0-255)
#define DEFAULT_LABEL              "RUCHE_01" // Nom du nœud
#define DEFAULT_HX711_SCALE        -1044858.81f // Facteur étalonnage HX711 (issu calibrateHX711())
#define DEFAULT_HX711_OFFSET       -1538424L    // Offset tare HX711 à vide  (issu calibrateHX711())
#define DEFAULT_VBAT_FACTOR        1.0f       // Correctif tension batterie INA219
#define DEFAULT_VSOL_FACTOR        1.0f       // Correctif tension solaire ADC
#define DEFAULT_SEND_INTERVAL_MIN  5          // Intervalle envoi LoRa (minutes)

// Alias pour compatibilité avec le code existant
#define RUCHER_ID          DEFAULT_RUCHER_ID
#define HX711_SCALE        DEFAULT_HX711_SCALE
#define HX711_OFFSET       DEFAULT_HX711_OFFSET
#define INTERVAL_PAYLOAD   DEFAULT_SEND_INTERVAL_MIN

// ============================================================
// Debug
// Mettre à true au premier démarrage pour lire le DevEUI
// matériel du RN2483, puis repasser à false.
// ============================================================
#define DEBUG_HWEUI  true
