/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PLAYER_DUMP_H
#define _PLAYER_DUMP_H

#include <string>
#include <map>
#include <set>

enum DumpTableType
{
    DTT_CHARACTER,      //    -> guid, name                 // characters

    DTT_CHAR_TABLE,     //                                  // character_account_data, character_achievement,
                                                            // character_achievement_progress, character_action,
                                                            // character_aura, character_glyphs,
                                                            // character_homebind, character_queststatus,
                                                            // character_reputation, character_skills, character_spell,
                                                            // character_spell_cooldown, character_talent, character_ticket

    DTT_CHAR_NAME_TABLE,// <- guid, name                    // character_declinedname

    DTT_EQSET_TABLE,    // <- guid                          // character_equipmentsets

    DTT_INVENTORY,      //    -> item guids collection      // character_inventory

    DTT_MAIL,           //    -> mail ids collection        // mail
                        //    -> item_text

    DTT_MAIL_ITEM,      // <- mail ids                      // mail_items
                        //    -> item guids collection

    DTT_ITEM,           // <- item guids                    // item_instance

    DTT_ITEM_GIFT,      // <- item guids                    // character_gifts

    DTT_ITEM_LOOT,      // <- item guids                    // item_loot

    DTT_PET,            //    -> pet guids collection       // character_pet
    DTT_PET_TABLE,      // <- pet guids                     // pet_aura, pet_spell, pet_spell_cooldown
    DTT_PET_DECL,       // <- pet guids                     // character_pet_declinedname
};

enum DumpReturn
{
    DUMP_SUCCESS,
    DUMP_FILE_OPEN_ERROR,
    DUMP_TOO_MANY_CHARS,
    DUMP_UNEXPECTED_END,
    DUMP_FILE_BROKEN,
};

class PlayerDump
{
    protected:
        PlayerDump() {}
};

class PlayerDumpWriter : public PlayerDump
{
    public:
        PlayerDumpWriter() {}

        std::string GetDump(uint32 guid);
        DumpReturn WriteDump(const std::string& file, uint32 guid);
    private:
        typedef std::set<uint32> GUIDs;

        void DumpTableContent(std::string& dump, uint32 guid, char const*tableFrom, char const*tableTo, DumpTableType type);
        std::string GenerateWhereStr(char const* field, GUIDs const& guids, GUIDs::const_iterator& itr);
        std::string GenerateWhereStr(char const* field, uint32 guid);

        GUIDs pets;
        GUIDs mails;
        GUIDs items;
};

class PlayerDumpReader : public PlayerDump
{
    public:
        PlayerDumpReader() {}

        DumpReturn LoadDump(const std::string& file, uint32 account, std::string name, uint32 guid);
};

#endif
