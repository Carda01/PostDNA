output: main.o dna.o kmer.o common.o
	gcc main.o dna.o kmer.o common.o -o output

main.o: main.c
	gcc -c main.c

dna.o: dna.c dna.h
	gcc -c dna.c

kmer.o: kmer.c kmer.h
	gcc -c kmer.c

common.o: common.c common.h
	gcc -c common.c

clean: 
	rm *.o output


