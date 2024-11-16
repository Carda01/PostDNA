drop table t;
drop extension if exists postdna;

create extension postdna;

CREATE TABLE t (id integer, dna dna, kmer kmer);

INSERT INTO t VALUES
(0, 'CAT', 'tac'),
(1, 'CATG', 'ttt'),
(2, 'CATGG', 'cca'),
(3, 'TACAGATA', 'TTAcca'),
(5, 'TACAGATAA', 'ccaGG'),
(6, 'CAAAATAAGCGAAAT', 'CCcca'),
(7, 'CATATATATAGGGGGAAAATTTTTTT', 'CCAcca');

SELECT * FROM t;
