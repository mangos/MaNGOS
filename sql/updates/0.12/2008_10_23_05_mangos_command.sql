DELETE FROM command WHERE name IN ('help','transport');

INSERT INTO command VALUES
('help',0,'Syntax: .help [$command]\r\n\r\nDisplay usage instructions for the given $command. If no $command provided show list available commands.');
