#include <string.h>
#include "cmor.h"

extern void cmor_cat_unique_string (char* dest, char* src);

int test_cat_unique_string(void) {
  char dest[128];
  char src[128];
  char expected[128];
  strcpy(dest, "");
  strcpy(src, "rumble");
  strcpy(expected, "rumble");

  /* 1. simple test: add string to blank */
  printf("running tests\n");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 1;
  }

  /* 2. simple test: add string to itself. Should be identical to above */
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 2;
  }

  /* 3. simple test: add string to non-blank, unique*/
  strcpy(src, "jungle");
  strcpy(expected, "rumble jungle");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 3;
  }

  /* 4. simple test: add string that exists within another word */
  strcpy(dest, "rumble jungle");
  strcpy(src, "umb");
  strcpy(expected, "rumble jungle umb");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 4;
  }

  /* 5. simple test: add string to blank */
  strcpy(dest, "rumble");
  strcpy(src, "rum");
  strcpy(expected, "rumble rum");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 5;
  }

  /* 6. simple test: add string to blank */
  strcpy(dest, "rumble");
  strcpy(src, "ble");
  strcpy(expected, "rumble ble");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 6;
  }

  /* 7. simple test: add string to blank */
  strcpy(dest, "rumble jungle happy");
  strcpy(src, "ppy");
  strcpy(expected, "rumble jungle happy ppy");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 7;
  }

  /* 8. simple test: add string to blank */
  strcpy(dest, "rumble jungle happy");
  strcpy(src, "gle");
  strcpy(expected, "rumble jungle happy gle");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 8;
  }

  /* 9. simple test: add string to blank */
  strcpy(dest, "rumble jungle happy");
  strcpy(src, "jung");
  strcpy(expected, "rumble jungle happy jung");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 9;
  }
  /* 10. simple test: add string to blank */
  strcpy(dest, "rumble jumble ble");
  strcpy(src, "ble");
  strcpy(expected, "rumble jumble ble");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 9;
  }

  /* 11. simple test: add latest */
  strcpy(dest, "rumble jumble");
  strcpy(src, "jumble");
  strcpy(expected, "rumble jumble");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 10;
  }

  /* 12. simple test: add latest */
  strcpy(dest, "rumble jumble");
  strcpy(src, "rumble");
  strcpy(expected, "rumble jumble");
  cmor_cat_unique_string(dest,src);
  if (strcmp(dest,expected)) {
    return 11;
  }

  return 0;

}

int main(int argc, char **argv) {
  test_cat_unique_string();
 
}
