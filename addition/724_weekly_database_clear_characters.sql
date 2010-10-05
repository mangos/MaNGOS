DELETE FROM `characters`.`item_instance` WHERE 
SUBSTRING_INDEX(SUBSTRING_INDEX(`characters`.`item_instance`.`data`,' ',4),' ',-1) IN 
(SELECT `mangos`.`item_template`.`entry`
FROM `mangos`.`item_template` 
WHERE `mangos`.`item_template`.`name` LIKE "% test%");

DELETE FROM `characters`.`auction` USING `characters`.`auction` LEFT JOIN `characters`.`item_instance` ON `characters`.`auction`.`itemguid` = `characters`.`item_instance`.`guid` WHERE `characters`.`item_instance`.`guid` IS NULL;
DELETE FROM `characters`.`auction` USING `characters`.`auction` LEFT JOIN `mangos`.`item_template` ON `auction`.`item_template` = `mangos`.`item_template`.`entry` WHERE `mangos`.`item_template`.`entry` IS NULL;

LOCK TABLES `item_instance` WRITE ;
START TRANSACTION;
DELETE
FROM `item_instance` 
WHERE `guid` NOT IN (SELECT `item` FROM `character_inventory`) 
AND `guid` NOT IN (SELECT `itemguid` FROM `auction`) 
AND `guid` NOT IN (SELECT `item_guid` FROM `guild_bank_item`) 
AND `guid` NOT IN (SELECT `item_guid` FROM `mail_items`) 
AND `guid` NOT IN (SELECT `item_guid` FROM `character_gifts`)
;
COMMIT;

UNLOCK TABLES;

