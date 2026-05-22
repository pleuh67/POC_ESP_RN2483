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
struct NodeData
{
  char     id[16];          // Identifiant du nœud  (ex. "RUCHE_01")
  float    temp_c;          // Température DHT22    (°C)
  float    hum_pct;         // Humidité DHT22       (%)
  float    poids1_kg;       // Poids Balance 1      (kg)
  float    poids2_kg;       // Poids Balance 2      (kg)
  float    poids3_kg;       // Poids Balance 3      (kg)
  float    poids4_kg;       // Poids Balance 4      (kg)
  float    vbat_v;          // Tension batterie     (V)
  float    vsol_v;          // Tension solaire      (V)
  uint16_t lux;             // Luminosité           (lux)
  uint8_t  rucher_id;       // Identifiant rucher   (0-255)
  bool     online;          // Nœud en ligne        — maître uniquement
  uint32_t last_seen;       // Timestamp dernière réception — maître uniquement
};
