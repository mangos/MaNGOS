ALTER TABLE db_version CHANGE COLUMN required_9753_01_mangos_instance_template required_9755_01_mangos_instance_template bit;

ALTER TABLE instance_template
  DROP COLUMN startLocX,
  DROP COLUMN startLocY,
  DROP COLUMN startLocZ,
  DROP COLUMN startLocO;
