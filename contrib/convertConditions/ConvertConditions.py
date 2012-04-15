#
# Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import MySQLdb as mdb
import sys

#global Variables (change as required)
host = "localhost"
user = "mangos"
passw = "mangos"
# databases format: list of [name, expansion]
databases = [ ["mangos", 2] ]
#databases = [ ["zero_db", 0], ["tbcdb", 1], ["udb_clean", 2], ["ytdb", 2] ]

# Should the current conditions table be loaded? (usefull for appending custom content)
loadOldConditions = 0
# database, from which the conditions table will be loaded
database = databases[0][0]
#database = "mangos_custom"

# be very chatty with debug output
debug = 0

# global variables for internal use
false = 0
true = 1
processNumConditions = 0
fUpdates = 0

# Some Helper functions, main code at the bottom
def isSameCondition(c1, v11, v12, c2, v21, v22):
    return (c1 == c2) and (v11 == v21) and (v12 == v22)
#

def compareCondition(c1, v11, v12, c2, v21, v22):
    if (c1 > c2):
        return true
    if (c1 == c2):
        if (v11 > v21):
            return true
        if (v11 == v21):
            if (v12 > v22):
                return true

    return false
#

def insertCondition(c, v1, v2):
    global old_max
    old_max = old_max + 1
    linkedList.append( [old_max, c, v1, v2, database] )
    if (debug):
        print "Inserted: [%d, %d, %d, %d], (%s)" % (old_max, c, v1, v2, database)
#

def findCondition(c, v1, v2):
    for entry in linkedList:
        if (isSameCondition(c, v1, v2, entry[1], entry[2], entry[3])):
            return entry[0]
    return 0
#

# Function that processes table tableName for keys keyName1, keyName2, parses the conditions of conditionString, which must select numberOfConditions conditions
def progressTable(tableName, keyName1, keyName2, conditionString, numberOfConditions):
    global old_max
    global processNumConditions
    global fUpdates

    try:
        con = mdb.connect(host, user, passw, database);
        cur = con.cursor()
        cur.execute('SELECT %s, %s, %s FROM %s; ' % (keyName1, keyName2, conditionString, tableName))

        result = cur.fetchall()

        if (debug):
            print 'ProgressTable %s in database %s' % (tableName, database)

        for row in result:
            key1 = row[0]
            key2 = row[1]

            c1 = v11 = v12 = c2 = v21 = v22 = c3= v31 =v32 = 0

            c1 = row[2]
            v11 = row[3]
            v12 = row[4]
            if (numberOfConditions >= 2):
                c2 = row[5]
                v21 = row[6]
                v22 = row[7]
            if (numberOfConditions >= 3):
                c3 = row[8]
                v31 = row[9]
                v32 = row[10]

            # Order the conditions of one row from big to slow
            if (numberOfConditions >= 2) and (compareCondition(c2, v21, v22, c1, v11, v12)):
                c1, v11, v12, c2, v21, v22 = c2, v21, v22, c1, v11, v12
            if (numberOfConditions >= 3):
                if (compareCondition(c3, v31, v32, c2, v21, v22)):
                    c2, v21, v22, c3, v31, v32 = c3, v31, v32, c2, v21, v22
                if (compareCondition(c2, v21, v22, c1, v11, v12)):
                    c1, v11, v12, c2, v21, v22 = c2, v21, v22, c1, v11, v12

            # How many conditions do we have?
            rowConditionNumber = 0
            if (c1 > 0):
                rowConditionNumber = rowConditionNumber + 1
            if (c2 > 0):
                rowConditionNumber = rowConditionNumber + 1
            if (c3 > 0):
                rowConditionNumber = rowConditionNumber + 1

            if (rowConditionNumber == 0): #nothing to do
                continue;

            if (debug):
                print "Condition(s) for Key (%d, %d): %d, %d, %d -- %d, %d, %d -- %d, %d, %d" % (key1, key2, c1, v11, v12, c2, v21, v22, c3, v31, v32)

            # Just insert
            if (processNumConditions == 0):
                if (rowConditionNumber >= 1 and findCondition(c1, v11, v12) == 0):
                    insertCondition(c1, v11, v12)
                if (rowConditionNumber >= 2 and findCondition(c2, v21, v22) == 0):
                    insertCondition(c2, v21, v22)
                if (rowConditionNumber >= 3 and findCondition(c3, v31, v32) == 0):
                    insertCondition(c3, v31, v32)
                continue
            #

            # Currently processing?
            if (processNumConditions != rowConditionNumber):
                continue

            founds = [0, 0, 0]
            countFound = 0 # helper for error
            if (rowConditionNumber >= 1):
                founds[0] = findCondition(c1, v11, v12)
                if (founds[0] > 0):
                    countFound = countFound + 1
            if (rowConditionNumber >= 2):
                founds[1] = findCondition(c2, v21, v22)
                if (founds[1] > 0):
                    countFound = countFound + 1
            if (rowConditionNumber >= 3):
                founds[2] = findCondition(c3, v31, v32)
                if (founds[2] > 0):
                    countFound = countFound + 1

            if (countFound != rowConditionNumber):
                print 'An error happened for: Condition(s) for Key (%d, %d): %d, %d, %d -- %d, %d, %d -- %d, %d, %d' % (key1, key2, c1, v11, v12, c2, v21, v22, c3, v31, v32)
                continue

            last_point = 0
            #3-vector condition
            if (rowConditionNumber == 3):
                # search for 2 match
                notSearched = [0, 0, 0]
                notSearched[2] = findCondition(-1, founds[0], founds[1])
                if (notSearched[2] == 0):
                    notSearched[2] = findCondition(-1, founds[1], founds[0])
                notSearched[1] = findCondition(-1, founds[0], founds[2])
                if (notSearched[1] == 0):
                    notSearched[1] = findCondition(-1, founds[2], founds[0])
                notSearched[0] = findCondition(-1, founds[1], founds[2])
                if (notSearched[0] == 0):
                    notSearched[0] = findCondition(-1, founds[2], founds[1])

                if (notSearched == [0, 0, 0]): # nothing found
                    insertCondition(-1, founds[1], founds[2])
                    notSearched[0] = old_max
                for i in range(0, 3):
                    if (notSearched[i] > 0):
                        last_point = findCondition(-1, notSearched[i], founds[i])
                        if (last_point == 0):
                            last_point = findCondition(-1, founds[i], notSearched[i])
                        if (last_point > 0):
                            break
                if (last_point == 0):
                    for i in range(0, 3):
                        if (notSearched[i] > 0):
                            insertCondition(-1, founds[i], notSearched[i])
                            last_point = old_max
                            break

            #2-vector condition
            if (rowConditionNumber == 2):
                # search for 2 match
                last_point = findCondition(-1, founds[1], founds[0])
                if (last_point == 0):
                    last_point = findCondition(-1, founds[0], founds[1])
                if (last_point == 0):
                    #Not found, insert list
                    insertCondition(-1, founds[1], founds[0])
                    last_point = old_max

            #1-vector condition
            if (rowConditionNumber == 1):
                last_point = founds[0]

            # Now we must have last_point > 0 (for a condition), and linking to proper place
            if (last_point > 0 and processNumConditions > 0):
                #cur.execute('UPDATE %s SET condition_id=%d WHERE %s=%d AND %s=%d; ' % (tableName, last_point, keyName1, key1, keyName2, key2))
                print >> fUpdates, 'UPDATE %s SET condition_id=%d WHERE %s=%d AND %s=%d;' % (tableName, last_point, keyName1, key1, keyName2, key2)

    except mdb.Error, e:
        print 'Error %d, %s' % (e.args[0], e.args[1])
        sys.exit(1)

    finally:
        if con:
            con.close()
## End of Helper function

linkedList = []
old_max = 0
linkedList.append( [0, 0, 0, 0, 'initial fill'] )

# Extract old conditions
if (loadOldConditions):
    try:
        con = mdb.connect(host, user, passw, database);
        cur = con.cursor()
        cur.execute('SELECT condition_entry, type, value1, value2 FROM conditions')

        for row in cur:
            linkedList.append( [row[0], row[1], row[2], row[3], 'reloaded from %s' % database ] )
            old_max = old_max + 1
            if (row[0] != old_max):
                print 'An error happened at old_max=%d, entry=%d' % (old_max, row[0])

        print 'Loaded %d values from %s conditions table' % (old_max, database)

    except mdb.Error, e:
        print 'Error %d, %s' % (e.args[0], e.args[1])
        sys.exit(1)

    finally:
        if con:
            con.close()
#
start_entry=old_max

def doTables(db):
    global processNumConditions
    global fUpdates
    global database

    database = db[0]
    print 'Processing database %s (%d vector conditions)' % (database, processNumConditions)
    try:
        if (processNumConditions == 0):
            fUpdates = open("%s_updates.sql" % database, "w")
        else:
            fUpdates = open("%s_updates.sql" % database, "a")

        if (processNumConditions <= 1):
            progressTable("reference_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("creature_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("gameobject_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("pickpocketing_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("item_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("fishing_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("skinning_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("disenchant_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            progressTable("mail_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            # Not all expansions have all tables
            if (db[1] >= 1):
                progressTable("prospecting_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
            if (db[1] >= 2):
                progressTable("spell_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)
                progressTable("milling_loot_template", "entry", "item", "lootcondition, condition_value1, condition_value2", 1)

        if (processNumConditions < 3):
            progressTable("gossip_menu", "entry", "text_id", "cond_1, cond_1_val_1, cond_1_val_2, cond_2, cond_2_val_1, cond_2_val_2", 2)
        progressTable("gossip_menu_option", "menu_id", "id", "cond_1, cond_1_val_1, cond_1_val_2, cond_2, cond_2_val_1, cond_2_val_2, cond_3, cond_3_val_1, cond_3_val_2", 3)

    except:
        print "An error happened here"
        sys.exit(1)

    finally:
        fUpdates.close()

# end of helper function doTables

try:
    fConditions = open("conditions_dump.sql", "w")
    if (debug):
        print 'Opened conditions_dump.sql successfully'

    for i in range (0, 4):
        processNumConditions = i
        for db in databases:
            doTables(db)
            print 'Inserted %d rows for database %s' % (old_max - start_entry, database)
            start_entry = old_max

    print 'Processed database(s): %s' % databases
    #create dump
    print >> fConditions, 'TRUNCATE conditions;'
    print >> fConditions, 'INSERT INTO conditions VALUES'
    for i in range(1, old_max):
        if (linkedList[i][0] != i):
            print 'AN ERROR HAPPENED for i=%d, liLi[i].entry=%d' % (i, linkedList[i][0])
        print >> fConditions, '(%d, %d, %d, %d), -- %s' % (linkedList[i][0], linkedList[i][1], linkedList[i][2], linkedList[i][3], linkedList[i][4])

    i = old_max
    print >> fConditions, '(%d, %d, %d, %d); -- %s' % (linkedList[i][0], linkedList[i][1], linkedList[i][2], linkedList[i][3], linkedList[i][4])

except:
    print "An error happened"
    sys.exit(1)

finally:
    fConditions.close()
