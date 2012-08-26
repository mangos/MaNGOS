ALTER TABLE db_version CHANGE COLUMN required_12112_05_mangos_npc_vendor_template required_12112_06_mangos_mangos_string bit;

REPLACE INTO `mangos_string` (`entry`, `content_default`) VALUES
(209, 'Item \'%i\' (isCurrency: %u) not found in vendor list.'),
(210, 'Item \'%i\' (isCurrency: %u, with extended cost %i) already in vendor list.'),
(269, 'Currency \'%i\' not found.'),
(283, 'Meta currency \'%i\' is not allowed in vendors.'),
(1509, 'Can\'t add item %u to vendor with unknown item type %u'),
(1510, 'Currency %u has maxCount = 0, but for currencies maxCount = buyCount, so it can\'t be 0 or less than that\'s currency precision (%u).');
