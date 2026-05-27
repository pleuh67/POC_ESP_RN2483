# Debug — Signaux LED WS2812

Tout le code LED se trouve dans `src/main.cpp`.  
Les constantes de couleur et de durée sont définies dans `include/config.h`.

## Séquences de clignotement

| Moment                   | Couleur              | Signification                          |
|--------------------------|----------------------|----------------------------------------|
| Boot (attente terminal)  | Orange clignotant    | Délai d'attente avant démarrage        |
| Boot terminé             | Vert 1 s             | Programme démarré                      |
| Test LED (boot)          | Rouge → Vert → Bleu (300 ms chacun) | Vérification visuelle WS2812 |
| Tests périphériques OK   | Vert fixe            | Tous les périphériques répondent       |
| Tests périphériques KO   | Rouge clignotant 3×  | Au moins un périphérique en échec      |
| Join OTAA en cours       | Bleu fixe            | Tentative de jointure                  |
| Join OTAA réussi         | Vert 1 s             | Jointure OK                            |
| Join OTAA échoué         | Rouge 1 s            | Tentative ratée                        |
| Erreur fatale            | Rouge fixe           | Impossible de rejoindre — boucle infinie |
| Envoi payload            | Blanc court          | Transmission en cours                  |

## Fonctions principales

| Fonction        | Rôle                                      |
|-----------------|-------------------------------------------|
| `ledSet(r,g,b)` | Applique une couleur RGB à la LED WS2812  |
| `ledOff()`      | Éteint la LED                             |
| `testLed()`     | Cycle rouge/vert/bleu au boot (vérification visuelle) |

---

## Affichage des mesures en temps réel

Toutes les `MEASURE_INTERVAL_MS` (défaut 10 s), une ligne est affichée :

```
[DATA] R:42 | T:+23.4C H:61.2% Lux: 320 | Vbat:3.82V Vsol:4.20V | P1: 18.35kg P2: 15.12kg P3: 20.05kg P4: 25.01kg
```

| Champ | Signification |
|-------|---------------|
| `R`   | RucherID |
| `T`   | Température BME280 (°C) |
| `H`   | Humidité BME280 (%) |
| `Lux` | Luminosité MAX44009 (lux) |
| `Vbat`| Tension batterie INA219 (V) |
| `Vsol`| Tension solaire calculée DS3231 (V) |
| `P1–P4` | Poids balances (kg) — P1 HX711, P2/3/4 simulées |

Modifier `MEASURE_INTERVAL_MS` dans `config.h` pour changer la fréquence.

---

## Commandes série

Taper dans le terminal PlatformIO pendant l'exécution :

| Commande | Action |
|----------|--------|
| `c` + Entrée | Lance la calibration HX711 (2 points) |
| `?` + Entrée | Affiche la liste des commandes disponibles |

### Procédure de calibration HX711

```
c  ← taper dans le terminal
  Etape 1/2 : balance a VIDE
  -> Retirez tout poids, puis appuyez sur Entree...
                          ← Entrée
  Tare OK  - valeur brute a vide : 84320

  Etape 2/2 : posez 2.50 kg sur la balance
  -> Poids en place, appuyez sur Entree...
                          ← Entrée
  Nouveau scale : 41920.00
  -> Mettez a jour HX711_SCALE dans include/config.h :
     #define HX711_SCALE  41920.00f
```

Reporter la valeur `HX711_SCALE` dans `include/config.h` et recompiler.

---

## Messages Wire `requestFrom(): i2cWriteReadNonStop returned Error -1`

Ces messages proviennent du framework Wire (pas du code applicatif). Ils apparaissent quand un périphérique I2C ne répond pas (NACK ou absent) : scanner au boot, capteur non branché, adresse incorrecte.

Ils sont supprimés dès le début de `setup()` par :

```cpp
esp_log_level_set("Wire", ESP_LOG_NONE);
```

Pour les réactiver temporairement (diagnostic câblage), remplacer `ESP_LOG_NONE` par `ESP_LOG_ERROR`.
