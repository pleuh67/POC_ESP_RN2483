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
#define HX711_SCALE     420.0f  // facteur d'étalonnage (à ajuster par pesée)

// Adresses I2C des périphériques
#define I2C_ADDR_DS3231   0x68  // RTC temps réel
#define I2C_ADDR_EEPROM   0x57  // AT24C32 — stockage persistant
#define I2C_ADDR_BME280   0x76  // Temp / Humidité / Pression
#define I2C_ADDR_LUX      0x4A  // MAX44009 — luminosité (adresse par défaut)
#define I2C_ADDR_INA219   0x40  // Tension / Courant batterie

// ============================================================
// Paramètres d'envoi
// ============================================================
#define INTERVAL_PAYLOAD  5                             // Intervalle d'envoi en minutes
#define SEND_INTERVAL_MS  (INTERVAL_PAYLOAD * 60000UL) // Converti en ms pour delay()
// Note : le port LoRaWAN est hardcodé à 1 dans la librairie rn2xx3 (txCommand "mac tx uncnf 1")

// ============================================================
// Identifiant du rucher (à adapter par installation)
// ============================================================
#define RUCHER_ID  42    // Identifiant rucher envoyé dans le payload (0-255)

// ============================================================
// Debug
// Mettre à true au premier démarrage pour lire le DevEUI
// matériel du RN2483, puis repasser à false.
// ============================================================
#define DEBUG_HWEUI  true
