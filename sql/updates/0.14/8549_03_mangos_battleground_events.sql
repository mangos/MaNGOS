ALTER TABLE db_version CHANGE COLUMN required_8548_02_mangos_gameobject_battleground required_8549_03_mangos_battleground_events bit;

DROP TABLE IF EXISTS `battleground_events`;
CREATE TABLE `battleground_events` (
  `map` smallint(5) NOT NULL,
  `event1` tinyint(3) unsigned NOT NULL,
  `event2` tinyint(3) unsigned NOT NULL,
  `description` varchar(255) NOT NULL,
  PRIMARY KEY  (`map`,`event1`,`event2`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;



SET @WS_MAP = 489;
SET @AB_MAP = 529;
SET @EY_MAP = 566;
SET @NA_MAP = 559;
SET @BE_MAP = 562;
SET @RL_MAP = 572;

INSERT INTO battleground_events (map, event1, event2, description) VALUES
    (@WS_MAP, 0, 0, 'Alliance Flag'),
    (@WS_MAP, 1, 0, 'Horde Flag'),
    (@WS_MAP, 2, 0, 'Spirit Guides'),
    (@WS_MAP, 254, 0, 'Doors'),

-- arathi
    (@AB_MAP, 0, 0, 'Stables - neutral'),
    (@AB_MAP, 0, 1, 'Stables - alliance contested'),
    (@AB_MAP, 0, 2, 'Stables - horde contested'),
    (@AB_MAP, 0, 3, 'Stables - alliance owned'),
    (@AB_MAP, 0, 4, 'Stables - horde owned'),

    (@AB_MAP, 1, 0, 'Blacksmith - neutral'),
    (@AB_MAP, 1, 1, 'Blacksmith - alliance contested'),
    (@AB_MAP, 1, 2, 'Blacksmith - horde contested'),
    (@AB_MAP, 1, 3, 'Blacksmith - alliance owned'),
    (@AB_MAP, 1, 4, 'Blacksmith - horde owned'),

    (@AB_MAP, 2, 0, 'Farm - neutral'),
    (@AB_MAP, 2, 1, 'Farm - alliance contested'),
    (@AB_MAP, 2, 2, 'Farm - horde contested'),
    (@AB_MAP, 2, 3, 'Farm - alliance owned'),
    (@AB_MAP, 2, 4, 'Farm - horde owned'),

    (@AB_MAP, 3, 0, 'Lumber Mill - neutral'),
    (@AB_MAP, 3, 1, 'Lumber Mill - alliance contested'),
    (@AB_MAP, 3, 2, 'Lumber Mill - horde contested'),
    (@AB_MAP, 3, 3, 'Lumber Mill - alliance owned'),
    (@AB_MAP, 3, 4, 'Lumber Mill - horde owned'),

    (@AB_MAP, 4, 0, 'Gold Mine - neutral'),
    (@AB_MAP, 4, 1, 'Gold Mine - alliance contested'),
    (@AB_MAP, 4, 2, 'Gold Mine - horde contested'),
    (@AB_MAP, 4, 3, 'Gold Mine - alliance owned'),
    (@AB_MAP, 4, 4, 'Gold Mine - horde owned'),

    (@AB_MAP, 254, 0, 'doors'),
-- eye of the storm
    (@EY_MAP, 0, 0, 'Fel Reaver - alliance'),
    (@EY_MAP, 0, 1, 'Fel Reaver - horde'),
    (@EY_MAP, 0, 2, 'Fel Reaver - neutral'),

    (@EY_MAP, 1, 0, 'Blood Elf - alliance'),
    (@EY_MAP, 1, 1, 'Blood Elf - horde'),
    (@EY_MAP, 1, 2, 'Blood Elf - neutral'),

    (@EY_MAP, 2, 0, 'Draenei Ruins - alliance'),
    (@EY_MAP, 2, 1, 'Draenei Ruins - horde'),
    (@EY_MAP, 2, 2, 'Draenei Ruins - neutral'),

    (@EY_MAP, 3, 0, 'Mage Tower - alliance'),
    (@EY_MAP, 3, 1, 'Mage Tower - horde'),
    (@EY_MAP, 3, 2, 'Mage Tower - neutral'),

    (@EY_MAP, 4, 0, 'capture flag - Fel Reaver'),
    (@EY_MAP, 4, 1, 'capture flag - Blood Elf'),
    (@EY_MAP, 4, 2, 'capture flag - Draenei Ruins'),
    (@EY_MAP, 4, 3, 'capture flag - Mage Tower'),
    (@EY_MAP, 4, 4, 'capture flag - center'),

    (@EY_MAP, 254, 0, 'doors'),

-- arenas
    (@NA_MAP, 253, 0, 'buffs'),
    (@NA_MAP, 254, 0, 'doors'),

    (@RL_MAP, 253, 0, 'buffs'),
    (@RL_MAP, 254, 0, 'doors'),

    (@BE_MAP, 253, 0, 'buffs'),
    (@BE_MAP, 254, 0, 'doors');
