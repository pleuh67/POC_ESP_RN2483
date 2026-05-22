# Guide de dépannage — POC_ESP_2483

---

## Le firmware RN2483 n'est pas détecté

**Symptôme sur le port série :**
```
[LoRa] Firmware RN2483 : non détecté - vérifiez le câblage
```

**Causes possibles et vérifications :**

| Cause | Vérification |
|---|---|
| TX et RX inversés | Vérifiez que le TX du RN2483 est connecté au RX de l'ESP32-S3 (GPIO4) et vice-versa |
| Mauvaise alimentation | Mesurez la tension sur la broche VDD du RN2483 : doit être entre 3.0V et 3.6V |
| Broche RST câblée à GND | La broche RST doit être HIGH au repos |
| Broche GPIO incorrecte | Vérifiez `RN_PIN_RX` et `RN_PIN_TX` dans `config.h` |
| Module défaillant | Testez avec un autre RN2483 si possible |

---

## Échec de la jointure OTAA (3 tentatives)

**Symptôme sur le port série :**
```
[LoRa] ERREUR : Jointure impossible apres 3 tentatives.
```

**Causes possibles et vérifications :**

| Cause | Vérification |
|---|---|
| Clés incorrectes | Vérifiez DEVEUI, APPEUI, APPKEY dans `secret.h` : valeurs identiques à celles sur Orange Live Objects |
| Device non enregistré | Confirmez que le device est bien créé sur Orange Live Objects avec ce DevEUI |
| Pas de couverture réseau | Vérifiez la couverture Orange à votre emplacement : https://iotjourney.orange.com/fr-FR/reseau/couverture-reseau |
| Antenne absente ou défaillante | Vérifiez la connexion de l'antenne 868 MHz sur le RN2483 |
| Plan de fréquences incorrect | Vérifiez que `setFrequencyPlan(TTN_EU)` est bien présent dans `initLoRa()` |

---

## La jointure réussit mais les trames n'apparaissent pas sur Orange

**Vérifications :**

- Dans Orange Live Objects, vérifiez l'onglet **Messages** du device (et non la page d'accueil).
- Vérifiez que le port applicatif `LORA_PORT` dans `config.h` est entre 1 et 223.
- Attendez au moins 60 secondes entre deux trames (duty cycle).
- Vérifiez le RSSI affiché dans les messages : en dessous de -120 dBm, le signal est très faible.

---

## Erreur TX_FAIL après jointure réussie

**Symptôme sur le port série :**
```
[LoRa] ERREUR envoi - verifiez la connexion au reseau
```

**Causes possibles :**

| Cause | Vérification |
|---|---|
| Déconnexion du réseau | Le module peut se déconnecter après un long silence — un re-join peut être nécessaire |
| Duty cycle dépassé | Augmentez `SEND_INTERVAL_MS` dans `config.h` |
| Signal trop faible | Approchez-vous d'une zone couverte ou réduisez le Spreading Factor |

---

## Le port série ne s'ouvre pas dans PlatformIO

**Vérifications :**

- Vérifiez que `monitor_speed = 115200` est bien dans `platformio.ini`.
- Sur Linux : ajoutez votre utilisateur au groupe `dialout` :
  ```bash
  sudo usermod -a -G dialout $USER
  # puis déconnectez/reconnectez la session
  ```
- Sur Windows : vérifiez le numéro de port COM dans le Gestionnaire de périphériques.

---

## Vérifier que secret.h ne sera pas commité

```bash
git status
```

`include/secret.h` ne doit **pas** apparaître dans les fichiers à commiter.

Pour vérifier que le `.gitignore` est bien pris en compte :

```bash
git check-ignore -v include/secret.h
# Doit retourner : .gitignore:12:include/secret.h   include/secret.h
```

Si le fichier a déjà été accidentellement commité, retirez-le du suivi :

```bash
git rm --cached include/secret.h
git commit -m "Suppression de secret.h du suivi git"
```
