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
