ALTER TABLE db_version CHANGE COLUMN required_8731_01_mangos_creature_template required_8749_01_mangos_mail_loot_template bit;

RENAME TABLE quest_mail_loot_template TO mail_loot_template;

UPDATE mail_loot_template, quest_template
  SET mail_loot_template.entry = quest_template.RewMailTemplateId WHERE mail_loot_template.entry = quest_template.entry;
