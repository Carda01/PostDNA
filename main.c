#include "common.h"
#include "dna.h"
#include "kmer.h"
#include <stdint.h>
#include <stdio.h>

int main(int argc, char **argv) {
  char *dna_string1 = argv[1];
  char *dna_string2 = argv[2];

  DNA dna1 = dna_string_to_DNA(dna_string1);
  DNA dna2 = dna_string_to_DNA(dna_string2);
  printf("%s\n", dna_DNA_to_string(dna1));
  com_print_binary(dna1.data[0]);
  printf("%s\n", dna_DNA_to_string(dna2));
  com_print_binary(dna2.data[0]);
  printf("%d\n", dna_equals(dna1, dna2));

  KMER kmer1 = kmer_string_to_KMER(dna_string1);
  KMER kmer2 = kmer_string_to_KMER(dna_string2);
  printf("%s\n", kmer_KMER_to_string(kmer1));
  com_print_binary(kmer1.data[0]);
  printf("%s\n", kmer_KMER_to_string(kmer2));
  com_print_binary(kmer2.data[0]);
  printf("%d\n", kmer_equals(kmer1, kmer2));

  return 0;
}
