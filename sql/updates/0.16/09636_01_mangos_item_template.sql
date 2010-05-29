ALTER TABLE db_version CHANGE COLUMN required_9622_01_mangos_gameobject required_9636_01_mangos_item_template bit;

ALTER TABLE item_template
  CHANGE COLUMN NonConsumable ExtraFlags tinyint(1) UNSIGNED DEFAULT '0' NOT NULL;

UPDATE item_template
  SET ExtraFlags = ExtraFlags | 0x2 WHERE Duration < 0 ;

UPDATE item_template
  SET Duration = abs(Duration);

ALTER TABLE item_template
  CHANGE COLUMN Duration Duration int(11) UNSIGNED DEFAULT '0' NOT NULL;
