ALTER TABLE db_version CHANGE COLUMN required_9034_01_mangos_spell_proc_event required_9045_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event WHERE entry IN (48545,48544,48539);
INSERT INTO spell_proc_event (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `procFlags`, `procEx`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(48539, 0x00000000, 7, 0x00000010, 0x04000000, 0x00000000, 0x00040000, 0x00000000, 0.000000, 0.000000, 0);
