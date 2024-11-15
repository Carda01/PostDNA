EXTENSION = postdna 
MODULE_big = postdna

OBJS = postdna.o dna.o kmer.o qkmer.o common.o
DATA = postdna--1.0.sql
HEADERS_postdna = dna.h kmer.h qkmer.h common.h

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)

CFLAGS += -flto -emit-llvm
LDFLAGS += -flto

include $(PGXS)
