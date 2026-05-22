# Câblage — ESP32-S3 + RN2483

## Principe

Le RN2483 communique avec l'ESP32-S3 via une liaison **UART série** à 57 600 bauds.
La communication est **full-duplex** (TX/RX) et complétée par une broche de **reset matériel**.

> ⚠️ Le RN2483 fonctionne **exclusivement en 3.3V**.
> Ne jamais connecter VDD ou les broches data en 5V.

---

## Tableau de connexion

| RN2483 (broche) | ESP32-S3 (GPIO) | Détail |
|---|---|---|
| **TX** (pin 3) | **GPIO 4** (entrée RX du Serial1) | Données module → MCU |
| **RX** (pin 4) | **GPIO 5** (sortie TX du Serial1) | Données MCU → module |
| **RST** (pin 5) | **GPIO 6** | Reset matériel (LOW = reset) |
| **VDD** (pin 1) | **3.3V** | Alimentation |
| **GND** (pin 2) | **GND** | Masse commune |

---

## Schéma ASCII

```
  ESP32-S3                         RN2483
  ┌─────────────────┐              ┌──────────────────┐
  │                 │              │                  │
  │  3.3V  ─────────┼──────────────┼─ VDD  (pin 1)    │
  │  GND   ─────────┼──────────────┼─ GND  (pin 2)    │
  │                 │              │                  │
  │  GPIO4 (RX1) ───┼──────────────┼─ TX   (pin 3)    │
  │  GPIO5 (TX1) ───┼──────────────┼─ RX   (pin 4)    │
  │  GPIO6       ───┼──────────────┼─ RST  (pin 5)    │
  │                 │              │                  │
  └─────────────────┘              └──────────┬───────┘
                                              │
                                          Antenne 868 MHz
```

---

## Notes importantes

### Niveaux de tension
Le RN2483 opère en **3.3V logique**. L'ESP32-S3 est également en 3.3V :
la connexion directe est possible **sans level-shifter**.

### Broche RST
La broche RST est active à l'état **LOW**.
Dans le code, la séquence de reset est :
```
RST → LOW  (200 ms)
RST → HIGH (500 ms d'attente)
```

### Antenne
Le RN2483 nécessite une **antenne 868 MHz** connectée sur sa broche ANT.
Sans antenne : risque de dommage du module et portée nulle.

### Courant
Le RN2483 consomme jusqu'à **38.9 mA** en émission (mode TX).
Vérifiez que votre régulateur 3.3V peut fournir ce courant.

---

## Modification des broches

Les broches sont définies dans `include/config.h` :

```cpp
#define RN_UART_NUM   1   // UART1 de l'ESP32-S3
#define RN_PIN_RX     4   // GPIO reçoit le TX du RN2483
#define RN_PIN_TX     5   // GPIO envoie vers le RX du RN2483
#define RN_PIN_RST    6   // GPIO contrôle le reset
```

Sur ESP32-S3, les UART hardware sont remappables sur la quasi-totalité
des GPIO disponibles via la matrice IO.
