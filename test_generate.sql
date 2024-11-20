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
\t
