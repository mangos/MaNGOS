DROP TABLE IF EXISTS item_test;
CREATE TABLE item_test
SELECT mi.mail_id, mi.item_guid FROM mail_items as mi WHERE mi.mail_id NOT IN (SELECT id FROM mail);

DELETE item_instance FROM item_instance, item_test WHERE item_instance.guid = item_test.item_guid;
DELETE mail_items FROM mail_items, item_test WHERE mail_items.mail_id = item_test.mail_id;
DROP TABLE IF EXISTS item_test;
