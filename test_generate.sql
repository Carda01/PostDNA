drop table t;
drop extension if exists postdna;

create extension postdna;

\t
SELECT 'SELECT generate_kmers("CTGAAATT", 6)';
SELECT generate_kmers('CTGAAATT', 6);
SELECT 'SELECT generate_kmers("CGTTAAAAAAACC", 4);';
SELECT generate_kmers('CGTTAAAAAAACC', 4);
SELECT 'SELECT generate_kmers("ATTTTTTTTCCCGTGCTAAAAAAATGACTGGGTTTTC", 32);';
SELECT generate_kmers('ATTTTTTTTCCCGTGCTAAAAAAATGACTGGGTTTTC', 32);
SELECT 'SELECT generate_kmers("CTGCCTTAAT", 1);';
SELECT generate_kmers('CTGCCTTAAT', 1);

SELECT 'SELECT generate_kmers("ATTTTTTTTCCCGTGCTAAAAAAATGACTGGGTTTTC", 33);';
SELECT generate_kmers('ATTTTTTTTCCCGTGCTAAAAAAATGACTGGGTTTTC', 33);
SELECT 'SELECT generate_kmers("ATTTTTTC", 0);';
SELECT generate_kmers('ATTTTTTC', 0);
SELECT 'SELECT generate_kmers("ATTTTTTC", 10);';
SELECT generate_kmers('ATTTTTTC', 10);

-- ***********************************************************************
-- Length Tests
-- ***********************************************************************

-- Test the length function for DNA
SELECT 'Testing length on "CTGAAATT" as DNA';
SELECT length('CTGAAATT'::dna);  -- length of DNA sequence

-- Test the length function for kmer
SELECT 'Testing length on "CTGAAATT" as kmer';
SELECT length('CTGAAATT'::kmer);  -- length of kmer sequence

-- Test the length function for Qkmer
SELECT 'Testing length on "CTGAAAT" as Qkmer';
SELECT length('CTGAAATT'::qkmer);  -- length of Qkmer sequence





\t
