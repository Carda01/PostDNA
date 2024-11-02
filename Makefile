output: main.o dna.o kmer.o qkmer.o common.o
	gcc main.o dna.o kmer.o qkmer.o common.o -o output

main.o: main.c
	gcc -c main.c

dna.o: dna.c dna.h
	gcc -c dna.c

kmer.o: kmer.c kmer.h
	gcc -c kmer.c

qkmer.o: qkmer.c qkmer.h
	gcc -c qkmer.c

common.o: common.c common.h
	gcc -c common.c

clean: 
	rm *.o output


