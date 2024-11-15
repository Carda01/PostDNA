drop table t;
drop extension if exists postdna;

create extension postdna;

CREATE TABLE t (id integer, sequence dna);

INSERT INTO t VALUES
(1, 'CATGG'),
(2, 'TACAGATA'),
(3, 'CAAAATAAGCGAAAT'),
(4, 'CATATATATAGGGGGAAAATTTTTTT');

SELECT * FROM t;
