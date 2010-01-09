DELETE FROM spell_chain WHERE first_spell = 8443;


/* Desecration */
DELETE FROM spell_chain WHERE spell_id in (55666,55667);
INSERT INTO spell_chain VALUES
(55666,0,55666,1,0),
(55667,55666,55666,2,0);
