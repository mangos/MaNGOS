ALTER TABLE db_version CHANGE COLUMN required_9692_03_mangos_spell_proc_event required_9704_01_mangos_achievement_reward bit;

ALTER TABLE achievement_reward
  ADD COLUMN gender TINYINT(3) DEFAULT '2' after entry,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (entry,gender);

ALTER TABLE locales_achievement_reward
  ADD COLUMN gender TINYINT(3) DEFAULT '2' after entry,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (entry,gender);
