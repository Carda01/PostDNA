drop table t;
drop extension if exists postdna;

create extension postdna;

CREATE TABLE t (id integer, sequence dna);

INSERT INTO t VALUES
(0, 'CAT'),
(1, 'CATG'),
(2, 'CATGG'),
(3, 'TACAGATA'),
(5, 'TACAGATAA'),
(6, 'CAAAATAAGCGAAAT'),
(7, 'CATATATATAGGGGGAAAATTTTTTT');

SELECT * FROM t;
