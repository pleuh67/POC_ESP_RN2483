# Tests unitaires — Guide pratique PlatformIO + Unity

---

## Principe

Un **test unitaire** vérifie qu'une fonction produit le bon résultat pour des entrées connues,
sans dépendance au matériel.

PlatformIO intègre le framework **Unity** (C) pour les tests embarqués et natifs.
L'environnement `native` compile et exécute les tests directement sur le PC.

---

## Structure du projet

```
test/
└── test_lookup/              <- suite de tests (un dossier = une suite)
    └── test_lookup.cpp       <- fichier de tests Unity
```

Chaque sous-dossier de `test/` est une suite indépendante.
PlatformIO compile et exécute chaque suite séparément.

---

## Lancer les tests

### Dans le terminal VSCode (Ctrl+`)

```bash
# Lancer tous les tests sur l'environnement natif (PC, sans matériel)
pio test -e native

# Lancer uniquement une suite précise
pio test -e native -f test_lookup

# Afficher la sortie détaillée (verbose)
pio test -e native -v
```

### Via l'interface PlatformIO

Dans la barre latérale PlatformIO → **Test** → sélectionner `native` → cliquer **Run Tests**.

---

## Lire les résultats

Sortie typique en cas de succès :

```
test/test_lookup/test_lookup.cpp:62:test_trouve_premier_module       [PASSED]
test/test_lookup/test_lookup.cpp:63:test_trouve_dernier_module       [PASSED]
test/test_lookup/test_lookup.cpp:64:test_insensible_casse_minuscules [PASSED]
test/test_lookup/test_lookup.cpp:65:test_insensible_casse_mixte      [PASSED]
test/test_lookup/test_lookup.cpp:66:test_deveui_absent               [PASSED]
test/test_lookup/test_lookup.cpp:67:test_deveui_partiel              [PASSED]
test/test_lookup/test_lookup.cpp:68:test_tableau_vide                [PASSED]

-----------------------
7 Tests 0 Failures 0 Ignored
OK
```

Sortie en cas d'échec (exemple) :

```
test/test_lookup/test_lookup.cpp:62:test_trouve_premier_module       [FAILED]
Expected 0 Was 1
```

Le fichier et le numéro de ligne indiquent exactement le test qui a échoué.

---

## Anatomie d'un fichier de tests

```cpp
#include <unity.h>       // framework de tests

// Fonction appelée avant chaque test (initialisation)
void setUp(void)    {}

// Fonction appelée après chaque test (nettoyage)
void tearDown(void) {}

// Un test = une fonction void sans paramètre
void test_mon_cas(void)
{
  // TEST_ASSERT_EQUAL(valeur_attendue, valeur_obtenue)
  TEST_ASSERT_EQUAL(42, maFonction(21));
}

// Point d'entrée
int main(int argc, char** argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_mon_cas);   // enregistrer chaque test
  return UNITY_END();
}
```

### Macros Unity courantes

| Macro | Usage |
|---|---|
| `TEST_ASSERT_EQUAL(attendu, obtenu)` | Égalité entière |
| `TEST_ASSERT_EQUAL_INT8(a, b)` | Égalité int8_t |
| `TEST_ASSERT_EQUAL_FLOAT(a, b)` | Égalité float |
| `TEST_ASSERT_EQUAL_STRING(a, b)` | Égalité chaîne C |
| `TEST_ASSERT_TRUE(condition)` | Condition vraie |
| `TEST_ASSERT_FALSE(condition)` | Condition fausse |
| `TEST_ASSERT_NULL(ptr)` | Pointeur nul |
| `TEST_ASSERT_NOT_NULL(ptr)` | Pointeur non nul |

---

## Quelles fonctions tester ?

### Testables (logique pure, sans hardware)

- Fonctions de conversion, calcul, formatage
- Fonctions de recherche dans des tableaux (ex : `lookupDevice`)
- Encodage/décodage de payload LoRaWAN
- Validation de paramètres

### Non testables en natif (dépendance hardware)

- Fonctions qui appellent `Serial`, `digitalWrite`, `delay`
- Fonctions qui utilisent des objets Arduino (`HardwareSerial`, `Adafruit_NeoPixel`)
- Jointure LoRaWAN, envoi de trame

> Pour les fonctions hardware, les tester directement sur la carte via `pio test -e esp32s3`
> (nécessite la carte branchée).

---

## Ajouter une nouvelle suite de tests

1. Créer un dossier `test/test_<nom>/`
2. Créer `test/test_<nom>/test_<nom>.cpp`
3. Inclure `<unity.h>`, écrire les fonctions de test, ajouter `RUN_TEST()` dans `main()`
4. Lancer `pio test -e native`

Aucune modification de `platformio.ini` n'est nécessaire :
PlatformIO découvre automatiquement tous les sous-dossiers de `test/`.

---

## Bonnes pratiques

- Un test = un seul comportement vérifié (nom explicite : `test_deveui_absent`)
- Tester les cas limites : tableau vide, valeur nulle, longueur incorrecte
- Les tests doivent être **indépendants** : l'ordre d'exécution ne doit pas avoir d'importance
- Si une fonction est difficile à tester, c'est souvent qu'elle fait trop de choses :
  envisager de la découper en une partie logique pure + une partie hardware

---

## Références

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](http://www.throwtheswitch.org/unity)
