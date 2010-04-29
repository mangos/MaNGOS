ALTER TABLE db_version CHANGE COLUMN required_9794_02_mangos_command required_9803_01_mangos_spell_bonus_data bit;

delete from `spell_bonus_data` where `entry` = 64085;
insert into `spell_bonus_data` values (64085, 1.2, 0, 0, 'Priest - Vampiric Touch Dispel');