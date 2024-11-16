drop table t;
drop extension if exists postdna;

create extension postdna;

CREATE TABLE t (id integer, dna dna, kmer kmer, qkmer qkmer);

INSERT INTO t VALUES
(0, 'CAT', 'tac', 'CGTAN'),
(1, 'CATG', 'ttt', 'MNTA'),
(2, 'CATGG', 'cca', 'CCCGAAAA'),
(3, 'TACAGATA', 'TTAcca', 'RYBNCCCGGT'),
(5, 'TACAGATAA', 'ccaGG', 'VTA'),
(6, 'CAAAATAAGCGAAAT', 'CCcca', 'CG'),
(7, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca', 'TTTGCD');

SELECT * FROM t;
