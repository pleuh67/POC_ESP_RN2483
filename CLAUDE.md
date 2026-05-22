# Instructions pour Claude Code

## Commits
- Rappeler régulièrement de committer, surtout après toute modification du code radio critique : TCXO, LoRaWAN (join, RX windows), sleep callback, NVS session/nonce.
- Proposer un commit dès qu'un état fonctionnel est atteint.

## Conventions de code

- **Accolades** : alignement vertical (style Allman)
- **Indentation** : 2 espaces
- **Nommage** : fonctions et variables en camelCase, `#define` en MAJUSCULES
- **Commentaires** : en français avec accents ; commenter chaque option de `platformio.ini` et chaque `#define` de `config.h` en fin de ligne
- **Tests unitaires** : pour toute fonction à logique pure (sans dépendance hardware), générer un test dans `test/test_<nom>/test_<nom>.cpp` avec le framework Unity (PlatformIO natif)
- **Entête de fonction** : toujours ajouter un bloc avant chaque fonction (format ci-dessous)

```cpp
// ---------------------------------------------------------------------------*
// @brief  Description courte
// @param  nomParam  Rôle du paramètre
// @return Type  Ce qui est retourné (omettre si void)
// @note   Contrainte ou précision importante (optionnel)
// ---------------------------------------------------------------------------*
```
- **Textes affichés** (OLED/série) : français sans accents
- **Debug** : `Serial.printf()` — ne pas utiliser `debugSerial` ni `SerialUSB`
- **Types** : toujours `uint8_t`, `int16_t`, `float` — jamais `int` seul
- **ISR** : décorer avec `IRAM_ATTR`, flag uniquement — pas de `Serial` ni I2C dans les ISR
- **Deep sleep** : sauvegarder l'état en RTC RAM (`RTC_DATA_ATTR`)

