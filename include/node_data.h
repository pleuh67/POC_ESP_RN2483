#pragma once

// ============================================================
// POC_ESP_2483 - Structure de données d'un nœud rucher
// Utilisée pour le payload LoRaWAN, l'API REST et l'OLED.
// ============================================================

#include <stdint.h>

// Taille du payload binaire LoRaWAN (octets)
// temp(2) + hum(2) + poids×4(8) + vbat(2) + vsol(2) + lux(2) + rucherID(1)
#define NODE_PAYLOAD_BYTES  19

// ---------------------------------------------------------------------------*
// @brief  Données consolidées d'un nœud capteur rucher.
//         Les champs online et last_seen sont réservés au maître
//         et ne sont pas encodés dans le payload LoRaWAN.
// ---------------------------------------------------------------------------*
// Champs dans l'ordre du payload LoRaWAN (little-endian, 19 octets) :
// rucherID(1) temp(2) hum(2) lux(2) vbat(2) vsol(2) poids×4(8)
struct NodeData
{
  uint8_t  rucher_id;       // Identifiant rucher   (0-255)        — octet 0
  float    temp_c;          // Température DHT22    (°C)           — octets 1-2
  float    hum_pct;         // Humidité DHT22       (%)            — octets 3-4
  uint16_t lux;             // Luminosité           (lux)          — octets 5-6
  float    vbat_v;          // Tension batterie     (V)            — octets 7-8
  float    vsol_v;          // Tension solaire      (V)            — octets 9-10
  float    poids1_kg;       // Poids Balance 1      (kg)           — octets 11-12
  float    poids2_kg;       // Poids Balance 2      (kg)           — octets 13-14
  float    poids3_kg;       // Poids Balance 3      (kg)           — octets 15-16
  float    poids4_kg;       // Poids Balance 4      (kg)           — octets 17-18
  char     id[16];          // Identifiant du nœud  (ex. "RUCHE_01") — hors payload
  bool     online;          // Nœud en ligne        — maître uniquement
  uint32_t last_seen;       // Timestamp dernière réception — maître uniquement
};
