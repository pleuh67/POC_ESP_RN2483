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
// Paramètres d'envoi
// ============================================================
#define SEND_INTERVAL_MS  60000  // 60 s (respecte duty cycle 1% EU868)
// Note : le port LoRaWAN est hardcodé à 1 dans la librairie rn2xx3 (txCommand "mac tx uncnf 1")

// ============================================================
// Debug
// Mettre à true au premier démarrage pour lire le DevEUI
// matériel du RN2483, puis repasser à false.
// ============================================================
#define DEBUG_HWEUI  true
