ALTER TABLE character_db_version CHANGE COLUMN required_7988_02_characters_character_equipmentsets required_7988_07_characters_characters bit;

UPDATE characters SET data = REPLACE(data,'  ',' ');
UPDATE characters SET data = CONCAT(TRIM(data),' ');

UPDATE `characters` SET `data` = CONCAT(
    SUBSTRING_INDEX(`data`, ' ', 257 + 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 1), ' ', -261 + 260 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18 + 1), ' ', -261 - 18 + 260 + 18 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*2 + 1), ' ', -261 - 18*2 + 260 + 18*2 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*3 + 1), ' ', -261 - 18*3 + 260 + 18*3 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*4 + 1), ' ', -261 - 18*4 + 260 + 18*4 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*5 + 1), ' ', -261 - 18*5 + 260 + 18*5 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*6 + 1), ' ', -261 - 18*6 + 260 + 18*6 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*7 + 1), ' ', -261 - 18*7 + 260 + 18*7 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*8 + 1), ' ', -261 - 18*8 + 260 + 18*8 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*9 + 1), ' ', -261 - 18*9 + 260 + 18*9 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*10 + 1), ' ', -261 - 18*10 + 260 + 18*10 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*11 + 1), ' ', -261 - 18*11 + 260 + 18*11 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*12 + 1), ' ', -261 - 18*12 + 260 + 18*12 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*13 + 1), ' ', -261 - 18*13 + 260 + 18*13 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*14 + 1), ' ', -261 - 18*14 + 260 + 18*14 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*15 + 1), ' ', -261 - 18*15 + 260 + 18*15 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*16 + 1), ' ', -261 - 18*16 + 260 + 18*16 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*17 + 1), ' ', -261 - 18*17 + 260 + 18*17 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 261 + 18*18 + 1), ' ', -261 - 18*18 + 260 + 18*18 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 837 + 1), ' ', -837 + 600 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 937 + 1), ' ', -937 + 874 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1007 + 1), ' ', -1007 + 1002 - 1), ' ',
    '0 0 ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1688 + 1), ' ', -1688 + 1008 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1696 + 1), ' ', -1696 + 1691 - 1), ' ',
    SUBSTRING_INDEX(SUBSTRING_INDEX(`data`, ' ', 1700 + 1), ' ', -1700 + 1699 - 1), ' '
    )
WHERE length(SUBSTRING_INDEX(data, ' ', 1700)) < length(data) and length(SUBSTRING_INDEX(data, ' ', 1701)) >= length(data);

UPDATE characters SET data = REPLACE(data,'  ',' ');
UPDATE characters SET data = CONCAT(TRIM(data),' ');
