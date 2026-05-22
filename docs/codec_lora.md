# Codec LoRaWAN — Décodage payload Orange Live Objects

## Format payload (19 octets, big-endian)

| Octets | Donnée           | Type    | Facteur | Résolution | Plage           |
|--------|------------------|---------|---------|------------|-----------------|
| 0-1    | Température      | int16   | ×100    | 0.01 °C    | ±327.67 °C      |
| 2-3    | Humidité         | uint16  | ×100    | 0.01 %     | 0–655.35 %      |
| 4-5    | Poids Balance 1  | int16   | ×100    | 0.01 kg    | ±327.67 kg      |
| 6-7    | Poids Balance 2  | int16   | ×100    | 0.01 kg    | ±327.67 kg      |
| 8-9    | Poids Balance 3  | int16   | ×100    | 0.01 kg    | ±327.67 kg      |
| 10-11  | Poids Balance 4  | int16   | ×100    | 0.01 kg    | ±327.67 kg      |
| 12-13  | Tension batterie | uint16  | ×100    | 0.01 V     | 0–655.35 V      |
| 14-15  | Tension solaire  | uint16  | ×100    | 0.01 V     | 0–655.35 V      |
| 16-17  | Luminosité       | uint16  | ×1      | 1 lux      | 0–65535 lux     |
| 18     | RucherID         | uint8   | ×1      | 1          | 0–255           |

---

## Codec JavaScript — Orange Live Objects

À coller dans **Applications → votre application → Codec → Decode function** :

```javascript
function decodeUplink(input) {
  var b = input.bytes;
  if (b.length < 19) {
    return { errors: ["payload trop court : " + b.length + " octets (attendu 19)"] };
  }

  function readInt16(i)  {
    var v = (b[i] << 8) | b[i + 1];
    return v > 32767 ? v - 65536 : v;
  }
  function readUInt16(i) { return (b[i] << 8) | b[i + 1]; }

  return {
    data: {
      temperature_c:  readInt16(0)   / 100.0,
      humidite_pct:   readUInt16(2)  / 100.0,
      poids1_kg:      readInt16(4)   / 100.0,
      poids2_kg:      readInt16(6)   / 100.0,
      poids3_kg:      readInt16(8)   / 100.0,
      poids4_kg:      readInt16(10)  / 100.0,
      vbat_v:         readUInt16(12) / 100.0,
      vsol_v:         readUInt16(14) / 100.0,
      luminosite_lux: readUInt16(16),
      rucher_id:      b[18]
    }
  };
}
```

---

## Exemple de décodage

Payload reçu : `09F727101388138813881388148215E013880001`

| Champ           | Octets bruts | Calcul          | Valeur décodée |
|-----------------|-------------|-----------------|----------------|
| temperature_c   | `09F7`      | 2551 / 100      | **25.51 °C**   |
| humidite_pct    | `2710`      | 10000 / 100     | **100.00 %**   |
| poids1_kg       | `1388`      | 5000 / 100      | **50.00 kg**   |
| poids2_kg       | `1388`      | 5000 / 100      | **50.00 kg**   |
| poids3_kg       | `1388`      | 5000 / 100      | **50.00 kg**   |
| poids4_kg       | `1388`      | 5000 / 100      | **50.00 kg**   |
| vbat_v          | `1482`      | 5250 / 100      | **52.50 V**    |
| vsol_v          | `15E0`      | 5600 / 100      | **56.00 V**    |
| luminosite_lux  | `1388`      | 5000            | **5000 lux**   |
| rucher_id       | `01`        | 1               | **1**          |

---

## Encodage côté ESP32-S3 (rappel)

```cpp
// Température 25.51°C → int16 ×100 = 2551 = 0x09F7
int16_t temp_encoded = (int16_t)(temperature * 100);
payload[0] = (temp_encoded >> 8) & 0xFF;   // MSB : 0x09
payload[1] =  temp_encoded        & 0xFF;  // LSB : 0xF7
```
