#include <stdio.h>
#include <string.h>
#include "dna.h"


int main(int argc, char **argv) {
    char* dna_string = argv[1];
    
    printf("%s\n", dna_string);
    size_t m_len = strlen(dna_string);
    uint8_t* m_data = generateDna(dna_string, m_len);
    printf("%s\n", to_string(m_data, m_len));
    return 0;
}

