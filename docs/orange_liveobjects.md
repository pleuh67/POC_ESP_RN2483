# Enregistrement sur Orange Live Objects

## Prérequis

- Un compte Orange Live Objects actif
  (inscription sur https://liveobjects.orange-business.com)
- Le **DevEUI** matériel de votre RN2483
  (affiché dans les **TESTS PERIPHERIQUES** au démarrage — voir README.md étape 1)

---

## Étapes

### 1. Connexion

Rendez-vous sur https://liveobjects.orange-business.com et connectez-vous.

### 2. Créer un device LoRaWAN

Dans le menu de gauche :
**Devices → Connected devices → + Add**

Sélectionnez le type : **LoRa device (OTAA)**

### 3. Renseigner les paramètres du device

| Champ | Valeur | Remarque |
|---|---|---|
| **Name** | `RUCHE_01` | Nom libre |
| **DevEUI** | `0004A30B001A2B3C` | Lu sur le port série au boot |
| **AppEUI** | valeur de votre choix | 8 octets hex, ex : `0000000000000001` |
| **AppKey** | valeur de votre choix | 16 octets hex, clé AES-128 |
| **Profile** | `EU868` | Obligatoire pour la France |

> AppEUI et AppKey sont générés par vos soins et doivent être recopiés
> exactement dans `include/secret.h`.

### 4. Valider et sauvegarder

Cliquez sur **Create**. Le device apparaît avec le statut **Inactive**
(normal — il passera à **Active** dès la première jointure OTAA réussie).

### 5. Renseigner les clés dans secret.h

```cpp
static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "0000000000000001", "2B7E151628AED2A6ABF7158809CF4F3C", "Ruche-01" },
    { nullptr, nullptr, nullptr, nullptr }
};
```

---

## Vérifier la réception des trames

Après une jointure OTAA réussie, les trames sont visibles dans :

**Devices → RUCHE_01 → Messages (onglet Data)**

Chaque trame affiche :
- L'horodatage de réception
- Le port applicatif (`fPort`) : **1** pour les données capteurs, **2** pour les redémarrages
- Le payload brut en hexadécimal
- Le RSSI et le SNR (qualité du signal)

### Filtrage par port

| Port | Contenu | Utilisation |
|---|---|---|
| 1 | Données capteurs (19 octets) | Mesures ruche — à décoder avec le codec |
| 2 | `Restart` ASCII (7 octets) | Notification de redémarrage du nœud |

Pour détecter les redémarrages côté application : filtrer `fPort = 2`.
Pour les données : filtrer `fPort = 1`.

---

## Configurer le codec de décodage

Pour décoder le payload (19 octets, port 1) en valeurs lisibles, copiez le codec JavaScript
depuis [`docs/codec_lora.md`](codec_lora.md) dans **Applications → votre application → Codec**.

Le payload port 2 (`Restart`) ne nécessite pas de codec : sa présence seule est l'information.

---

## Notes sur la couverture réseau Orange

Le réseau Orange LoRaWAN couvre les principales agglomérations françaises.
Pour vérifier la couverture à votre emplacement :
https://iotjourney.orange.com/fr-FR/reseau/couverture-reseau

En cas de couverture insuffisante, le RN2483 affichera un échec de jointure OTAA
après 3 tentatives et la LED restera rouge fixe.
