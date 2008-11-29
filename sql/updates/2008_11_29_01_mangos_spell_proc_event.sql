ALTER TABLE db_version CHANGE COLUMN required_2008_11_27_01_mangos_playercreateinfo_item required_2008_11_29_01_mangos_spell_proc_event bit;

DELETE FROM spell_proc_event where entry = 35080;
INSERT INTO spell_proc_event (entry, SchoolMask, Category, SkillID, SpellFamilyName, SpellFamilyMask, procFlags, ppmRate, cooldown) VALUES
(35080,0,0,0,0,0x0000000000000000,0x00080001,0,60);
