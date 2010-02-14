ALTER TABLE character_db_version CHANGE COLUMN required_9374_01_characters_character_glyphs required_9375_01_characters_character_glyphs bit;

DROP TABLE IF EXISTS `character_glyphs`;
CREATE TABLE `character_glyphs` (
  `guid` int(11) unsigned NOT NULL,
  `spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `slot` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `glyph` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spec`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Extract values from data blob fields and insert them into character_glyphs
INSERT INTO `character_glyphs` SELECT `guid`, 0, 0, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1319),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;
INSERT INTO `character_glyphs` SELECT `guid`, 0, 1, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1320),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;
INSERT INTO `character_glyphs` SELECT `guid`, 0, 2, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1321),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;
INSERT INTO `character_glyphs` SELECT `guid`, 0, 3, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1322),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;
INSERT INTO `character_glyphs` SELECT `guid`, 0, 4, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1323),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;
INSERT INTO `character_glyphs` SELECT `guid`, 0, 5, (CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1324),  ' ', -1) AS UNSIGNED)) AS `glyph` FROM `characters`;

DELETE FROM character_glyphs WHERE glyph = 0;
