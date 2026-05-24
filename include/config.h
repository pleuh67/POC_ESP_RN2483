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
#define HX711_SCALE   -1044326.38f  // facteur d'étalonnage   (mis à jour par calibrateHX711())
#define HX711_OFFSET          0L    // offset tare à vide      (mis à jour par calibrateHX711())
#define HX711_CALIB_KG        2.5f  // poids de référence pour la calibration (kg)

// Adresses I2C des périphériques
#define I2C_ADDR_DS3231   0x68  // RTC temps réel
#define I2C_ADDR_EEPROM   0x57  // AT24C32 — stockage persistant
#define I2C_ADDR_BME280   0x76  // Temp / Humidité / Pression
#define I2C_ADDR_LUX      0x4A  // MAX44009 — luminosité (adresse par défaut)
#define I2C_ADDR_INA219   0x40  // Tension / Courant batterie

// ============================================================
// Paramètres d'envoi
// ============================================================
#define INTERVAL_PAYLOAD     5                              // Intervalle d'envoi LoRa en minutes
#define SEND_INTERVAL_MS     (INTERVAL_PAYLOAD * 60000UL)  // Converti en ms
#define MEASURE_INTERVAL_MS  10000UL                        // Affichage terminal (dev)
// Note : le port LoRaWAN est hardcodé à 1 dans la librairie rn2xx3 (txCommand "mac tx uncnf 1")

// ============================================================
// Valeurs par défaut de NodeConfig
// Utilisées si l'EEPROM est vierge, corrompue ou absente.
// En mode développement : modifier ici, puis recalibrer et sauver en EEPROM.
// ============================================================
#define DEFAULT_RUCHER_ID          42         // Identifiant rucher (0-255)
#define DEFAULT_LABEL              "RUCHE_01" // Nom du nœud
#define DEFAULT_HX711_SCALE        -1044326.38f // Facteur étalonnage HX711
#define DEFAULT_HX711_OFFSET       0L         // Offset tare HX711 à vide
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
