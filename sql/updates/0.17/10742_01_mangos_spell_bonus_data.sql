ALTER TABLE db_version CHANGE COLUMN required_10704_01_mangos_gossip_menu_option required_10742_01_mangos_spell_bonus_data bit;

ALTER TABLE spell_bonus_data
  ADD COLUMN ap_dot_bonus float NOT NULL default '0' AFTER ap_bonus;

DELETE FROM spell_bonus_data WHERE entry IN (50536, 26573);
INSERT INTO spell_bonus_data VALUES
(50536, 0,      0,       0,   0.013, 'Death Knight - Unholy Blight Triggered'),
(26573, 0,      0.04,    0,   0.04,  'Paladin - Consecration');
