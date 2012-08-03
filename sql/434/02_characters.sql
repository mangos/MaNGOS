ALTER TABLE `characters`
    DROP COLUMN `power6`,
    DROP COLUMN `power7`,
    DROP COLUMN `power8`,
    DROP COLUMN `power9`,
    DROP COLUMN `power10`;

ALTER TABLE `character_stats`
    DROP COLUMN `maxpower6`,
    DROP COLUMN `maxpower7`,
    DROP COLUMN `maxpower8`,
    DROP COLUMN `maxpower9`,
    DROP COLUMN `maxpower10`;

ALTER TABLE `character_pet`
    DROP COLUMN `curhappiness`;