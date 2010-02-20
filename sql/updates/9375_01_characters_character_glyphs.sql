ALTER TABLE character_db_version CHANGE COLUMN required_9374_01_characters_character_glyphs required_9375_01_characters_character_glyphs bit;

RENAME TABLE `characters`.`character_glyphs` TO `characters`.`old_character_glyphs` ;

DROP TABLE IF EXISTS `character_glyphs`;
CREATE TABLE `character_glyphs` (
  `guid` int(11) unsigned NOT NULL,
  `spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `glyph` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spec`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 0, `glyph1` AS `glyph` FROM `characters`.`old_character_glyphs`;
INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 1, `glyph2` AS `glyph` FROM `characters`.`old_character_glyphs`;
INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 2, `glyph3` AS `glyph` FROM `characters`.`old_character_glyphs`;
INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 3, `glyph4` AS `glyph` FROM `characters`.`old_character_glyphs`;
INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 4, `glyph5` AS `glyph` FROM `characters`.`old_character_glyphs`;
INSERT INTO `character_glyphs` SELECT `guid`, `spec`, 5, `glyph6` AS `glyph` FROM `characters`.`old_character_glyphs`;

DELETE FROM character_glyphs WHERE glyph = 0;

DROP TABLE IF EXISTS `old_character_glyphs`;
