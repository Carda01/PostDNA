#include "dna.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t dna_get_length(DNA dna){
  return dna.length;
}


DNA dna_string_to_DNA(const char *dna_str) {
  DNA dna;
  dna.length = strlen(dna_str);

  // Number of bytes necessary to store dna_str in a bitset.
  const size_t dna_bytes = com_get_number_of_bytes(dna.length);

  dna.data = malloc(sizeof(uint8_t) * dna_bytes);
  memset(dna.data, 0, dna_bytes);

  // For each base in the DNA sequence...
  for (size_t i = 0; i < dna.length; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);

    switch (dna_str[i]) {
    case 'A':
      dna.data[i / 4] |= BASE_A << shift;
      break;
    case 'C':
      dna.data[i / 4] |= BASE_C << shift;
      break;
    case 'G':
      dna.data[i / 4] |= BASE_G << shift;
      break;
    case 'T':
      dna.data[i / 4] |= BASE_T << shift;
      break;
    default:
      printf("Invalid DNA base");
    }

    shift = (shift == 0) ? 6 : shift - 2;
  }
  return dna;
}


/**
 * @brief Returns the stored DNA sequence as an ASCII string.
 */
char *dna_DNA_to_string(DNA dna) {
  char *dna_str = malloc(sizeof(char) * (dna.length + 1));

  // For each base in the DNA sequence...
  for (size_t i = 0; i < dna.length; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);
    uint8_t mask = BASE_MASK << shift;

    // Get the i-th DNA base.
    uint8_t base = (dna.data[i / 4] & mask) >> shift;

    switch (base) {
    case BASE_A:
      dna_str[i] = 'A';
      break;
    case BASE_C:
      dna_str[i] = 'C';
      break;
    case BASE_G:
      dna_str[i] = 'G';
      break;
    case BASE_T:
      dna_str[i] = 'T';
      break;
    default:
      printf("Invalid DNA base");
    }
  }

  dna_str[dna.length] = '\0';
  return dna_str;
}


bool dna_equals(DNA dna1, DNA dna2) {
  if (dna1.length != dna2.length) {
    return false;
  }

  for (size_t i = 0; i < dna1.length; ++i) {
    if (dna1.data[i] != dna2.data[i]) {
      return false;
    }
  }

  return true;
}

