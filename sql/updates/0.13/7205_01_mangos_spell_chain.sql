ALTER TABLE db_version CHANGE COLUMN required_7199_02_mangos_spell_proc_event required_7205_01_mangos_spell_chain bit;

DELETE FROM `spell_chain` WHERE `spell_id` IN (
  3034, 14279, 14280, 27018, 49008,
  5138, 6226, 11703, 11704, 27221, 30908, 47858,
  8129, 8131, 10874, 10875, 10876, 25379, 25380, 48128
);