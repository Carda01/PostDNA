#include "dna.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

size_t dna_get_length(DNA dna) { return dna.length; }

DNA dna_string_to_DNA(const char *dna_str) {
  DNA dna;
  dna.length = strlen(dna_str);
  dna.data = com_encode(dna_str, dna.length);
  return dna;
}


char *dna_DNA_to_string(DNA dna) {
  return com_decode(dna.data, dna.length);
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
