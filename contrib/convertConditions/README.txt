#
# Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
#

This small Python script is intended to help with the convertion
of the old condition-system to the new conditions system.

Requirements:
* Python 2.5 (at least tested with this version)
* The MySQLdb module (found on http://sourceforge.net/projects/mysql-python/)

You need to edit the ConvertConditions.py file for configure.

First variables that must be set are related to the database connection (host, user, passw).

Then you must specify which databases you want to convert (databases)
For portability, you must provide the name of the database, and the client expansion for which it is.
(2 == WotLK, 1 == TBC, 0 == classic)

You can also provide a list of databases to process multiple in one run.

With the next option, you can specify if you want to load a current `conditions` table (loadOldConditions)
Setting = 1 will load old conditions (default = 0).
In case you want to load from a database, which is not the first database you want the script to work on,
you can provide its name with the database variable.

The script will create (and overwrite existing!) sql-files:
conditions_dump.sql <-- this file stores a possible dump for the conditions table for the old conditions of your databases. (including the loaded)

<DB_NAME>_updates.sql <-- these files store the UPDATE statements to fill the condition_id fields for the tables.
                          One file is created for each provided database.

Backup your data, before you change anything!
