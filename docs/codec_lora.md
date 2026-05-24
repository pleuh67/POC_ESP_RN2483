# Codec LoRaWAN — Décodage payload Orange Live Objects

## Format payload (19 octets, little-endian)

| Octets | Donnée           | Type   | Facteur | Résolution | Plage          |
|--------|------------------|--------|---------|------------|----------------|
| 0      | RucherID         | uint8  | ×1      | 1          | 0–255          |
| 1-2    | Température      | int16  | ×100    | 0.01 °C    | ±327.67 °C     |
| 3-4    | Humidité         | uint16 | ×100    | 0.01 %     | 0–655.35 %     |
| 5-6    | Luminosité       | uint16 | ×1      | 1 lux      | 0–65535 lux    |
| 7-8    | Tension batterie | uint16 | ×100    | 0.01 V     | 0–655.35 V     |
| 9-10   | Tension solaire  | uint16 | ×100    | 0.01 V     | 0–655.35 V     |
| 11-12  | Poids Balance 1  | int16  | ×100    | 0.01 kg    | ±327.67 kg     |
| 13-14  | Poids Balance 2  | int16  | ×100    | 0.01 kg    | ±327.67 kg     |
| 15-16  | Poids Balance 3  | int16  | ×100    | 0.01 kg    | ±327.67 kg     |
| 17-18  | Poids Balance 4  | int16  | ×100    | 0.01 kg    | ±327.67 kg     |

---

## Codec JavaScript — Orange Live Objects

À coller dans **Applications → votre application → Codec → Decode function** :

```javascript
function decodeUplink(input) {
  var b = input.bytes;
  if (b.length < 19) {
    return { errors: ["payload trop court : " + b.length + " octets (attendu 19)"] };
  }

  function readInt16LE(i) {
    var v = b[i] | (b[i + 1] << 8);
    return v > 32767 ? v - 65536 : v;
  }
  function readUInt16LE(i) { return b[i] | (b[i + 1] << 8); }

  return {
    data: {
      rucher_id:      b[0],
      temperature_c:  readInt16LE(1)   / 100.0,
      humidite_pct:   readUInt16LE(3)  / 100.0,
      luminosite_lux: readUInt16LE(5),
      vbat_v:         readUInt16LE(7)  / 100.0,
      vsol_v:         readUInt16LE(9)  / 100.0,
      poids1_kg:      readInt16LE(11)  / 100.0,
      poids2_kg:      readInt16LE(13)  / 100.0,
      poids3_kg:      readInt16LE(15)  / 100.0,
      poids4_kg:      readInt16LE(17)  / 100.0
    }
  };
}
```

---

## Exemple de décodage

Payload reçu : `2AF7096419B0047C01C201E803DC05D007C409`

| Champ           | Octets (LE)   | Calcul      | Valeur décodée |
|-----------------|---------------|-------------|----------------|
| rucher_id       | `2A`          | 42          | **42**         |
| temperature_c   | `F7 09`       | 2551 / 100  | **25.51 °C**   |
| humidite_pct    | `64 19`       | 6500 / 100  | **65.00 %**    |
| luminosite_lux  | `B0 04`       | 1200        | **1200 lux**   |
| vbat_v          | `7C 01`       | 380  / 100  | **3.80 V**     |
| vsol_v          | `C2 01`       | 450  / 100  | **4.50 V**     |
| poids1_kg       | `E8 03`       | 1000 / 100  | **10.00 kg**   |
| poids2_kg       | `DC 05`       | 1500 / 100  | **15.00 kg**   |
| poids3_kg       | `D0 07`       | 2000 / 100  | **20.00 kg**   |
| poids4_kg       | `C4 09`       | 2500 / 100  | **25.00 kg**   |

---

## Encodage côté ESP32-S3 (rappel)

```cpp
// Température 25.51°C → int16 ×100 = 2551 = 0x09F7, little-endian
int16_t temp_encoded = (int16_t)(temperature * 100);
buf[1] =  temp_encoded        & 0xFF;  // LSB : 0xF7
buf[2] = (temp_encoded >> 8)  & 0xFF;  // MSB : 0x09
```
