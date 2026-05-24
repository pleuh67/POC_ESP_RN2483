#pragma once

#include <stdint.h>

// ============================================================
// POC_ESP_2483 — Configuration persistante du nœud rucher
//
// Stockée en EEPROM (AT24C32, adresse EEPROM_ADDR_CONFIG).
// Modifiable via le serveur web embarqué (à venir).
// Fallback sur les valeurs de config.h si EEPROM vierge ou CRC invalide.
//
// Ajout d'un paramètre :
//   1. Ajouter le champ dans la section appropriée ci-dessous
//   2. Initialiser sa valeur par défaut dans node_config.cpp (initDefaultConfig)
//   3. L'exposer dans le serveur web (à venir)
// ============================================================

struct NodeConfig
{
  // ── Identification du nœud ───────────────────────────────
  uint8_t  rucher_id;              // Identifiant rucher (0-255)
  char     label[16];              // Nom libre du nœud (ex. "RUCHE_01")

  // ── Balance 1 — HX711 ────────────────────────────────────
  float    hx711_scale;            // Facteur d'étalonnage cellule de charge
  long     hx711_offset;           // Offset tare à vide

  // ── Calibrations tensions ─────────────────────────────────
  float    vbat_factor;            // Correctif multiplicatif INA219 → Vbat réelle
  float    vsol_factor;            // Correctif multiplicatif ADC → Vsol réelle

  // ── Envoi LoRaWAN ─────────────────────────────────────────
  uint8_t  send_interval_min;      // Intervalle d'envoi (minutes, 1-255)

  // ── Réservé — paramètres futurs ───────────────────────────
  uint8_t  _reserved[32];          // Espace pour extensions sans casser le CRC

  // ── Intégrité ─────────────────────────────────────────────
  uint32_t crc;                    // CRC32 calculé sur tous les champs précédents
};

// Adresse de début en EEPROM (AT24C32 = 4096 octets)
#define EEPROM_ADDR_CONFIG  0

// Taille utile hors CRC (pour le calcul CRC)
#define NODE_CONFIG_DATA_SIZE  (sizeof(NodeConfig) - sizeof(uint32_t))
