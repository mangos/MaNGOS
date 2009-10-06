ALTER TABLE db_version CHANGE COLUMN required_8589_03_mangos_item_template required_8589_05_mangos_battleground_template bit;

delete from battleground_template where id in(30, 32);
insert into `battleground_template`(`id`,`MinPlayersPerTeam`,`MaxPlayersPerTeam`,`MinLvl`,`MaxLvl`,`AllianceStartLoc`,`AllianceStartO`,`HordeStartLoc`,`HordeStartO`) values (30,20,40,71,80,1485,0,1486,0);
insert into `battleground_template`(`id`,`MinPlayersPerTeam`,`MaxPlayersPerTeam`,`MinLvl`,`MaxLvl`,`AllianceStartLoc`,`AllianceStartO`,`HordeStartLoc`,`HordeStartO`) values (32,0,40,0,80,0,0,0,0);
