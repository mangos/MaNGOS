ALTER TABLE db_version CHANGE COLUMN required_10342_01_mangos_mangos_string required_10342_02_mangos_command bit;

DELETE FROM command WHERE name IN (
  'achievement','achievement add','achievement remove','achievement criteria add','achievement criteria remove'
);

INSERT INTO command (name, security, help) VALUES
('achievement',3,'Syntax: .achievement $playername #achivementid\r\n\r\nShow state achievment #achivmentid (can be shift link) and list of achievement criteria with progress data for selected player in game or by player name.'),
('achievement add',3,'Syntax: .achievement add $playername #achivementid\r\n\r\nComplete achievement and all it\'s criteria for selected player in game or by player name. Command can\'t be used for counter achievements.'),
('achievement remove',3,'Syntax: .achievement remove $playername #achivementid\r\n\r\nRemove complete state for achievement #achivmentid and reset all achievement\'s criteria for selected player in game or by player name. Also command can be used for reset counter achievements.'),
('achievement criteria add',3,'Syntax: .achievement criteria add $playername #criteriaid #change\r\n\r\nIncrease progress for non-completed criteria at #change for selected player in game or by player name. If #chnage not provided then non-counter criteria progress set to completed state. For counter criteria increased at 1.'),
('achievement criteria remove',3,'Syntax: .achievement criteria remove $playername #criteriaid #change\r\n\r\necrease progress for criteria at #change for selected player in game or by player name. If #chnage not provided then criteria progress reset to 0.');

