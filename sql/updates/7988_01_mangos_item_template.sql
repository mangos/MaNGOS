ALTER TABLE db_version CHANGE COLUMN required_7980_01_mangos_item_required_target required_7988_01_mangos_item_template bit;

alter table `item_template`
    drop column `dmg_type3`,
    drop column `dmg_max3`,
    drop column `dmg_min3`,
    drop column `dmg_type4`,
    drop column `dmg_max4`,
    drop column `dmg_min4`,
    drop column `dmg_type5`,
    drop column `dmg_max5`,
    drop column `dmg_min5`;
