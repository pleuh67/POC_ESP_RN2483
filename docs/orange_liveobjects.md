# Enregistrement sur Orange Live Objects

## Prérequis

- Un compte Orange Live Objects actif
  (inscription sur https://liveobjects.orange-business.com)
- Le **DevEUI** matériel de votre RN2483
  (récupéré via l'étape 1 de la mise en route — voir README.md)

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
| **Name** | `POC_ESP_2483` | Nom libre |
| **DevEUI** | `0004A30B001A2B3C` | Lu sur le port série (étape 1) |
| **AppEUI** | valeur de votre choix | 8 octets hex, ex : `0000000000000001` |
| **AppKey** | valeur de votre choix | 16 octets hex, clé AES-128 |
| **Profile** | `EU868` | Obligatoire pour la France |

> AppEUI et AppKey sont générés par vos soins et doivent être recopiés
> exactement dans `include/secret.h`.

### 4. Valider et sauvegarder

Cliquez sur **Create**. Le device apparaît dans la liste avec le statut **Inactive**
(normal — il passera à **Active** dès la première jointure OTAA réussie).

### 5. Renseigner les clés dans secret.h

Copiez AppEUI et AppKey dans `include/secret.h` :

```cpp
static const LoraDevice LORA_DEVICES[] =
{
    { "0004A30B001A2B3C", "0000000000000001", "2B7E151628AED2A6ABF7158809CF4F3C", "Module-01" },
    { nullptr, nullptr, nullptr, nullptr }
};
```

---

## Vérifier la réception des trames

Après une jointure OTAA réussie, les trames remontées par le device
sont visibles dans :

**Devices → POC_ESP_2483 → Messages (onglet Data)**

Chaque trame affiche :
- L'horodatage de réception
- Le port applicatif (`fPort`)
- Le payload brut en hexadécimal
- Le RSSI et le SNR (qualité du signal)

---

## Configurer un codec de décodage (optionnel)

Pour décoder le payload hex en valeurs lisibles, créez un codec JavaScript
dans **Applications → votre application → Codec** :

```javascript
// Exemple : décodage d'un compteur 16 bits
function decodeUplink(input) {
  var bytes = input.bytes;
  var counter = (bytes[0] << 8) | bytes[1];
  return {
    data: {
      counter: counter
    }
  };
}
```

---

## Notes sur la couverture réseau Orange

Le réseau Orange LoRaWAN couvre les principales agglomérations françaises.
Pour vérifier la couverture à votre emplacement :
https://iotjourney.orange.com/fr-FR/reseau/couverture-reseau

En cas de doute, le module RN2483 affichera un échec de jointure OTAA
et le message d'erreur sera visible sur le port série.
