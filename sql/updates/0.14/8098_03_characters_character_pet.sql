ALTER TABLE character_db_version CHANGE COLUMN required_8098_01_characters_character_action required_8098_03_characters_character_pet bit;

UPDATE character_pet
   SET abdata = CONCAT(
     (SUBSTRING(abdata, 1, length(SUBSTRING_INDEX(abdata, ' ', 1))) >> 8),' ',
     SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 1))+2, length(SUBSTRING_INDEX(abdata, ' ', 2))-length(SUBSTRING_INDEX(abdata, ' ', 1))-1),' ',
     (SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 2))+2, length(SUBSTRING_INDEX(abdata, ' ', 3))-length(SUBSTRING_INDEX(abdata, ' ', 2))-1) >> 8),' ',
     SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 3))+2, length(SUBSTRING_INDEX(abdata, ' ', 4))-length(SUBSTRING_INDEX(abdata, ' ', 3))-1),' ',
     (SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 4))+2, length(SUBSTRING_INDEX(abdata, ' ', 5))-length(SUBSTRING_INDEX(abdata, ' ', 4))-1) >> 8),' ',
     SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 5))+2, length(SUBSTRING_INDEX(abdata, ' ', 6))-length(SUBSTRING_INDEX(abdata, ' ', 5))-1),' ',
     (SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 6))+2, length(SUBSTRING_INDEX(abdata, ' ', 7))-length(SUBSTRING_INDEX(abdata, ' ', 6))-1) >> 8),' ',
     SUBSTRING(abdata, length(SUBSTRING_INDEX(abdata, ' ', 7))+2, length(SUBSTRING_INDEX(abdata, ' ', 8))-length(SUBSTRING_INDEX(abdata, ' ', 7))-1),' '
  );
