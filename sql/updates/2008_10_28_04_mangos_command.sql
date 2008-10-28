DELETE FROM command WHERE name IN ('respawn');

INSERT INTO command VALUES
('respawn',3,'Syntax: .respawn\r\n\r\nRespawn selected creature or respawn all nearest creatures (if none selected) and GO without waiting respawn time expiration.');
