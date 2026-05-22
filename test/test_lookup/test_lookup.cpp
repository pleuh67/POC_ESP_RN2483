// ============================================================
// Tests unitaires — logique de recherche de device LoRaWAN
// Cible : env:native (pio test -e native)
// Couvre : findDeviceIndex() — insensibilité casse, trouvé,
//          absent, tableau vide
// ============================================================

#include <unity.h>
#include <stdint.h>
#include <cctype>
#include <cstring>

// ---------------------------------------------------------------------------*
// Reproduction locale de la structure (pas de dépendance à secret.h)
// ---------------------------------------------------------------------------*
struct LoraDevice
{
  const char* deveui;
  const char* appeui;
  const char* appkey;
  const char* label;
};

// ---------------------------------------------------------------------------*
// @brief  Recherche l'index d'un DevEUI dans un tableau de LoraDevice
// @param  devices  Tableau terminé par un élément {nullptr,...}
// @param  deveui   DevEUI à chercher (insensible à la casse)
// @return int8_t   Index (>= 0) si trouvé, -1 si absent
// ---------------------------------------------------------------------------*
static int8_t findDeviceIndex(const LoraDevice* devices, const char* deveui)
{
  for (uint8_t i = 0; devices[i].deveui != nullptr; i++)
  {
    const char* a = devices[i].deveui;
    const char* b = deveui;
    bool match = true;

    while (*a && *b)
    {
      if (toupper((uint8_t)*a) != toupper((uint8_t)*b))
      {
        match = false;
        break;
      }
      a++;
      b++;
    }

    if (match && *a == '\0' && *b == '\0')
      return (int8_t)i;
  }
  return -1;
}

// ---------------------------------------------------------------------------*
// Données de test
// ---------------------------------------------------------------------------*
static const LoraDevice TEST_DEVICES[] =
{
  { "0004A30B001A1111", "AAAA000000000001", "AAAA0000000000000000000000000001", "Module-01" },
  { "0004A30B001A2222", "BBBB000000000002", "BBBB0000000000000000000000000002", "Module-02" },
  { "0004A30B001A3333", "CCCC000000000003", "CCCC0000000000000000000000000003", "Module-03" },
  { nullptr, nullptr, nullptr, nullptr }
};

static const LoraDevice EMPTY_DEVICES[] =
{
  { nullptr, nullptr, nullptr, nullptr }
};

// ---------------------------------------------------------------------------*
// Tests
// ---------------------------------------------------------------------------*
void test_trouve_premier_module(void)
{
  TEST_ASSERT_EQUAL_INT8(0, findDeviceIndex(TEST_DEVICES, "0004A30B001A1111"));
}

void test_trouve_dernier_module(void)
{
  TEST_ASSERT_EQUAL_INT8(2, findDeviceIndex(TEST_DEVICES, "0004A30B001A3333"));
}

void test_insensible_casse_minuscules(void)
{
  TEST_ASSERT_EQUAL_INT8(0, findDeviceIndex(TEST_DEVICES, "0004a30b001a1111"));
}

void test_insensible_casse_mixte(void)
{
  TEST_ASSERT_EQUAL_INT8(1, findDeviceIndex(TEST_DEVICES, "0004a30B001A2222"));
}

void test_deveui_absent(void)
{
  TEST_ASSERT_EQUAL_INT8(-1, findDeviceIndex(TEST_DEVICES, "FFFFFFFFFFFFFFFF"));
}

void test_deveui_partiel(void)
{
  // Préfixe commun mais différent — ne doit pas correspondre
  TEST_ASSERT_EQUAL_INT8(-1, findDeviceIndex(TEST_DEVICES, "0004A30B001A111"));
}

void test_tableau_vide(void)
{
  TEST_ASSERT_EQUAL_INT8(-1, findDeviceIndex(EMPTY_DEVICES, "0004A30B001A1111"));
}

// ---------------------------------------------------------------------------*
// Point d'entrée Unity
// ---------------------------------------------------------------------------*
void setUp(void)    {}
void tearDown(void) {}

int main(int argc, char** argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_trouve_premier_module);
  RUN_TEST(test_trouve_dernier_module);
  RUN_TEST(test_insensible_casse_minuscules);
  RUN_TEST(test_insensible_casse_mixte);
  RUN_TEST(test_deveui_absent);
  RUN_TEST(test_deveui_partiel);
  RUN_TEST(test_tableau_vide);
  return UNITY_END();
}
