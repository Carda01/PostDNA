EXTENSION = postdna 
MODULE_big = postdna

OBJS = dna.o kmer.o qkmer.o sequence.o
DATA = postdna--1.0.sql
HEADERS_postdna = dna.h kmer.h qkmer.h sequence.h

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)

CFLAGS += -flto -emit-llvm
LDFLAGS += -flto

include $(PGXS)
