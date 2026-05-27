# Guide de dépannage — POC_ESP_2483

---

## Le firmware RN2483 n'est pas détecté

**Symptôme :**
```
[LoRa] Firmware RN2483 : non detecte - verifiez le cablage
```

| Cause | Vérification |
|---|---|
| TX et RX inversés | TX du RN2483 → GPIO4 (RX ESP32), RX du RN2483 → GPIO5 (TX ESP32) |
| Mauvaise alimentation | VDD du RN2483 doit être entre 3.0 V et 3.6 V |
| Broche RST câblée à GND | RST doit être HIGH au repos |
| GPIO incorrecte | Vérifiez `RN_PIN_RX` et `RN_PIN_TX` dans `config.h` |

---

## Échec de la jointure OTAA (3 tentatives)

**Symptôme :**
```
[LoRa] ERREUR : Jointure impossible apres 3 tentatives.
```

| Cause | Vérification |
|---|---|
| Clés incorrectes | Comparez DevEUI/AppEUI/AppKey dans `secret.h` avec Orange Live Objects |
| Device non enregistré | Vérifiez qu'il est bien créé sur Orange Live Objects |
| Pas de couverture réseau | Vérifiez la couverture Orange LoRaWAN sur votre site |
| Antenne absente | Vérifiez la connexion de l'antenne 868 MHz sur le RN2483 |
| Plan de fréquences | `setFrequencyPlan(TTN_EU)` doit être dans `initLoRa()` |

---

## La jointure réussit mais les trames n'apparaissent pas sur Orange

- Dans Orange Live Objects, vérifiez l'onglet **Messages** du device.
- Les trames de données sont sur le **port 1**, les notifications de redémarrage sur le **port 2**.
- Attendez au moins 5 minutes entre deux trames (`DEFAULT_SEND_INTERVAL_MIN` dans `config.h`).
- RSSI en dessous de -120 dBm = signal très faible.

---

## Erreur TX_FAIL après jointure réussie

**Symptôme :**
```
[LoRa] ERREUR envoi - verifiez la connexion au reseau
```

| Cause | Vérification |
|---|---|
| Déconnexion réseau | Un re-join peut être nécessaire après un long silence |
| Duty cycle dépassé | Augmentez `DEFAULT_SEND_INTERVAL_MIN` dans `config.h` |
| Signal trop faible | Approchez-vous d'une zone couverte |

---

## Valeurs NaN sur le BME280

**Symptôme :**
```
[DATA] R:42 | T:nanC H:nan%
```

Le BME280 n'a pas été initialisé. Vérifiez :
1. Que le BME280 apparaît dans le scan I2C au boot (0x76 ou 0x77).
2. Que l'adresse correspond à `I2C_ADDR_BME280` dans `config.h` (le code tente les deux automatiquement).
3. Le câblage SDA/SCL et l'alimentation 3.3V.

---

## Aucun capteur de luminosité détecté

**Symptôme :**
```
[LUX] ATTENTION : aucun capteur de luminosite detecte
```

Le code tente dans l'ordre : BH1750 (0x23), BH1750 (0x5C), MAX44009 (0x4A).
Vérifiez l'adresse dans le scan I2C au boot et le câblage.

---

## L'OLED ne s'affiche pas

**Symptôme :** écran vide ou message :
```
[OLED] ATTENTION : ecran non detecte (0x3C et 0x3D essayes)
```

| Cause | Vérification |
|---|---|
| Adresse incorrecte | Vérifiez l'adresse dans le scan I2C (0x3C si SA0=GND, 0x3D si SA0=VCC) |
| OLED initialisé avant scan | L'ordre dans `initSensors()` est : scan → oledInit — ne pas inverser |
| Alimentation | L'OLED consomme ~15 mA — vérifiez le 3.3V |
| Librairie | `adafruit/Adafruit SH110X` doit être dans `lib_deps` |

---

## Messages Wire `Error -1` dans le terminal

**Symptôme :**
```
[E][Wire.cpp:499] requestFrom(): i2cWriteReadNonStop returned Error -1
```

Ces messages indiquent qu'un périphérique I2C ne répond pas au démarrage.
Ils sont normalement **supprimés** par :
```cpp
esp_log_level_set("Wire.cpp", ESP_LOG_NONE);
```
Si vous les voyez encore, vérifiez que cette ligne est bien au début de `setup()`.

Cause courante : `ina219.begin()` ou `rtc.begin()` pas encore appelé quand
une lecture est tentée. Ces appels doivent être dans `initSensors()`.

---

## La config EEPROM n'est pas chargée

**Symptôme :**
```
[Config] CRC invalide (lu=0x... calc=0x...) — valeurs par defaut
```

Normal au premier démarrage ou après modification de la structure `NodeConfig`.
Lancez une calibration (`c` dans le terminal) pour sauvegarder une config valide.

---

## Le bouton n'envoie pas de payload

Vérifications :
1. GPIO7 correctement câblé vers GND (un fil suffit, pas de résistance externe).
2. `pinMode(BUTTON_PIN, INPUT_PULLUP)` bien présent dans `setup()` après `initLoRa()`.
3. Le bouton doit être appuyé après la jointure OTAA — avant, aucun payload ne peut partir.

---

## Rien ne s'affiche sur le port série (LED de boot OK)

**Cause probable : port USB natif sans USB CDC activé.**

| Port | Puce | Flag requis |
|---|---|---|
| COM (UART) | CH340 / CP210x | aucun |
| USB (natif) | ESP32-S3 USB | `-DARDUINO_USB_CDC_ON_BOOT=1` |

Vérifiez dans `platformio.ini` :
```ini
build_flags =
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_USB_MODE=1
```

---

## Port série non disponible (Windows)

- Vérifiez le numéro de port COM dans le Gestionnaire de périphériques.
- Si deux cartes sont connectées, préciser dans `platformio.ini` :
  ```ini
  upload_port = COM5
  monitor_port = COM5
  ```

---

## Vérifier que secret.h ne sera pas commité

```bash
git check-ignore -v include/secret.h
# Doit retourner : .gitignore:XX:include/secret.h   include/secret.h
```

Si le fichier a déjà été commité par erreur :
```bash
git rm --cached include/secret.h
git commit -m "Suppression de secret.h du suivi git"
```
