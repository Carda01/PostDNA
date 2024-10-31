#ifndef DNA_H
#define DNA_H

#include <stdint.h>
#include <stdlib.h>

uint8_t *generateDna(const char *dna_str, const size_t dna_len);
char *to_string(uint8_t *m_data, size_t m_len);

#endif
