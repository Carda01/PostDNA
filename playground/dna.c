#include "dna.h"
#include <stdio.h>
#include <string.h>

#define BASE_MASK 0x3 // Binary: 11.

// Binary codes assigned to each DNA base.
enum {
  BASE_A = 0x0, // Binary: 00.
  BASE_C = 0x1, // Binary: 01.
  BASE_G = 0x2, // Binary: 10.
  BASE_T = 0x3, // Binary: 11.
};

uint8_t *generateDna(const char *dna_str, const size_t dna_len) {
  uint8_t *m_data;
  size_t m_len = dna_len;

  // Number of bytes necessary to store dna_str in a bitset.
  size_t dna_bytes = (dna_len / 4) + (dna_len % 4 != 0);

  m_data = malloc(sizeof(uint8_t) * dna_bytes);
  memset(m_data, 0, dna_bytes);

  // For each base in the DNA sequence...
  for (size_t i = 0; i < dna_len; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);

    switch (dna_str[i]) {
    case 'A':
      m_data[i / 4] |= BASE_A << shift;
      break;
    case 'C':
      m_data[i / 4] |= BASE_C << shift;
      break;
    case 'G':
      m_data[i / 4] |= BASE_G << shift;
      break;
    case 'T':
      m_data[i / 4] |= BASE_T << shift;
      break;
    default:
      printf("Invalid DNA base");
    }

    shift = (shift == 0) ? 6 : shift - 2;
  }
  return m_data;
}

/**
 * @brief Returns the stored DNA sequence as an ASCII string.
 */
char *to_string(uint8_t *m_data, size_t m_len) {
  char *dna_str = malloc(sizeof(char) * (m_len + 1));

  // For each base in the DNA sequence...
  for (size_t i = 0; i < m_len; ++i) {
    uint8_t shift = 6 - 2 * (i % 4);
    uint8_t mask = BASE_MASK << shift;

    // Get the i-th DNA base.
    uint8_t base = (m_data[i / 4] & mask) >> shift;

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

  dna_str[m_len] = '\0';
  return dna_str;
}
