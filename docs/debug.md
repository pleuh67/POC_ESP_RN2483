# Debug — Signaux LED, OLED et terminal série

Tout le code LED se trouve dans `src/main.cpp`.
Les constantes de couleur et de durée sont définies dans `include/config.h`.

---

## Séquences LED WS2812 (GPIO48)

| Moment | Couleur | Signification |
|---|---|---|
| Boot (attente terminal) | Orange clignotant | Délai 10 s avant démarrage |
| Boot terminé | Vert 1 s | Programme lancé |
| Test LED (boot) | Rouge → Vert → Bleu 300 ms | Vérification visuelle WS2812 |
| Tests périphériques OK | Vert fixe 1 s | Tous les capteurs répondent |
| Tests périphériques KO | Rouge clignotant 3× | Au moins un capteur en échec |
| Join OTAA en cours | Bleu fixe | Tentative de jointure réseau |
| Join OTAA réussi | Vert 2 s | Jointure OK |
| Join OTAA échoué (tentative) | Rouge 1 s | Nouvelle tentative dans 10 s |
| Erreur fatale (3 échecs) | Rouge fixe permanent | Boucle infinie — vérifiez le terminal |
| Envoi payload (port 1 ou 2) | Blanc bref | Transmission en cours |
| Heartbeat | Vert 30 ms | Programme actif (toutes les 5 s) |

### Heartbeat

Un flash vert de 30 ms toutes les `HEARTBEAT_INTERVAL_MS` confirme que la boucle
tourne sans blocage. Régler à `0` dans `config.h` pour le désactiver.

```cpp
#define HEARTBEAT_INTERVAL_MS  5000UL   // 0 = désactivé
```

---

## Afficheur OLED SH1106

L'OLED affiche un journal déroulant de 8 lignes × 20 caractères.
Chaque nouvelle ligne repousse les précédentes vers le haut.
L'affichage fonctionne **même sans port série ouvert**.

### Contenu affiché

| Phase | Ligne OLED typique |
|---|---|
| Démarrage | `POC ESP RN2483` |
| EEPROM | `EEPROM OK` ou `EEPROM CRC FAIL` |
| Tests boot | `DS3231 14:32:07` / `BME280 22.9C 58%` / `LUX 423 lux` |
| Tests boot | `INA219 3.82V` / `HX711 13.39kg` / `2 OK 0 FAIL` |
| Join LoRa | `LoRa join 1/3...` → `LoRa JOIN OK` |
| Synchro RTC | `RTC 14:32 UTC+2` |
| Date/heure | `27/05/2026 14:32` |
| Payload OK | `14:32 13.5kg 22.9C` |
| Payload KO | `14:32 13.5kg 22.9C KO` |
| Bouton | `BTN: envoi force` |

---

## Affichage des mesures en temps réel (port série)

Toutes les `MEASURE_INTERVAL_MS` (défaut 10 s), une ligne `[DATA]` est affichée :

```
[DATA] R:42 | T:+23.4C H:61.2% Lux: 320 | Vbat:3.82V Vsol:4.20V | P1: 18.35kg P2: 15.12kg P3: 20.05kg P4: 25.01kg
```

| Champ | Signification |
|---|---|
| `R` | RucherID |
| `T` | Température BME280 (°C) |
| `H` | Humidité BME280 (%) |
| `Lux` | Luminosité BH1750 ou MAX44009 (lux) |
| `Vbat` | Tension batterie INA219 (V) |
| `Vsol` | Tension solaire calculée DS3231 (V) |
| `P1–P4` | Poids balances (kg) — P1 HX711, P2/3/4 simulées |

---

## Commandes série

Taper dans le terminal PlatformIO pendant l'exécution.
Activer `monitor_echo = yes` dans `platformio.ini` pour voir ce que l'on tape.

| Commande | Action |
|---|---|
| `c` + Entrée | Calibration HX711 (2 points, sauvegarde EEPROM) |
| `h HH:MM` + Entrée | Règle l'heure du DS3231 (ex : `h 14:30`) |
| `d DD/MM/YYYY` + Entrée | Règle la date du DS3231 (ex : `d 27/05/2026`) |
| `?` + Entrée | Affiche la liste des commandes |

### Procédure de calibration HX711

```
c
  Etape 1/2 : balance a VIDE
  -> Retirez tout poids, puis appuyez sur Entree...
                          [Entrée]
  Tare OK  - valeur brute a vide : -1538424

  Etape 2/2 : posez un poids de reference sur la balance
  -> Saisissez le poids en kg [11.000] puis Entree : 11.0
                          [Entrée]
  Nouveau scale : -1044858.81
  Verification  : 11.000 kg (doit etre 11.000 kg)
  [Config] Config sauvegardee en EEPROM OK
```

Les valeurs sont automatiquement sauvegardées en EEPROM — elles survivent au redémarrage.

---

## Bouton d'envoi forcé (GPIO7)

Un appui sur le bouton (GPIO7 → GND) déclenche immédiatement :
1. Lecture de tous les capteurs
2. Affichage `[DATA]` sur le port série
3. Envoi LoRaWAN (port 1)
4. Remise à zéro du chrono d'envoi automatique

---

## Suppression des logs Wire ESP-IDF

Les messages `[E][Wire.cpp:499] requestFrom(): i2cWriteReadNonStop returned Error -1`
proviennent du HAL Arduino quand un périphérique I2C ne répond pas (NACK, absent,
adresse incorrecte). Ils sont supprimés dès le début de `setup()` :

```cpp
esp_log_level_set("Wire",     ESP_LOG_NONE);   // filtre tag "Wire"
esp_log_level_set("Wire.cpp", ESP_LOG_NONE);   // filtre tag "Wire.cpp" (HAL Arduino)
```

Pour les réactiver temporairement (diagnostic câblage) :
```cpp
esp_log_level_set("Wire.cpp", ESP_LOG_ERROR);
```

---

## Synchro RTC depuis le réseau LoRa

Après jointure OTAA et à chaque downlink reçu, la commande `mac get gpstime`
est envoyée au RN2483 (firmware >= 1.0.5 requis). La réponse est convertie
en heure locale et appliquée au DS3231 :

```
GPS epoch (6 jan 1980) + 315964800 − 18 (leap s) + TIMEZONE_OFFSET_H × 3600
```

Si la commande retourne `0` ou `invalid`, la synchro est ignorée sans erreur.
