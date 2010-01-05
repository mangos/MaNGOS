ALTER TABLE db_version CHANGE COLUMN required_8769_01_mangos_mail_level_reward required_8770_01_mangos_quest_template bit;

ALTER TABLE quest_template
  CHANGE COLUMN QuestLevel QuestLevel smallint(6) NOT NULL DEFAULT 0;
