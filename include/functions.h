#pragma once

// ============================================================
// POC_ESP_2483 — Prototypes des fonctions de main.cpp
// ============================================================

#include <Arduino.h>
#include "node_data.h"

// ── LED WS2812 ───────────────────────────────────────────────
void  ledSet(uint8_t r, uint8_t g, uint8_t b); // Applique une couleur RGB
void  ledOff();                                  // Éteint la LED

// ── Boot ─────────────────────────────────────────────────────
void  bootWait();                                // Clignotement orange + bannière série

// ── RN2483 ───────────────────────────────────────────────────
void  resetRN2483();                             // Reset matériel via broche RST
void  lookupDevice(const String& hwDeveui);      // Recherche les clés OTAA dans secret.h
void  initLoRa();                                // Init RN2483 + jointure OTAA

// ── Capteurs I2C ─────────────────────────────────────────────
void  scanI2C();                                 // Scan du bus I2C au boot
void  initSensors();                             // Init Wire + HX711 + scan I2C
void  readSensors(NodeData& node);               // Lecture de tous les capteurs

// ── Calcul tension solaire ───────────────────────────────────
float computeVsol();                             // Modèle horaire via DS3231 (0–6 V)

// ── Tests périphériques ──────────────────────────────────────
bool  testLed();                                 // Cycle RGB visuel WS2812
bool  testRN2483();                              // Autobaud + firmware + DevEUI
bool  testDS3231();                              // Présence + lecture date/heure
bool  testBME280();                              // Présence + lecture temp/hum/pression
float max44009ReadLux();                         // Lecture directe I2C MAX44009 (lux)
bool  testMAX44009();                            // Présence + lecture luminosité
bool  testINA219();                              // Présence + lecture tension/courant
bool  testHX711();                               // Présence + valeur brute + kg
void  runPeripheralTests();                      // Lance tous les tests, affiche bilan

// ── LoRaWAN ──────────────────────────────────────────────────
void  sendPayload(const NodeData& node);         // Encode et envoie 19 octets (LE)
