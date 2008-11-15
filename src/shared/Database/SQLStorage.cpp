/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
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

#include "SQLStorage.h"
#include "SQLStorageImpl.h"

#ifdef DO_POSTGRESQL
extern DatabasePostgre  WorldDatabase;
#else
extern DatabaseMysql  WorldDatabase;
#endif

const char CreatureInfofmt[]="iiiiiisssiiiiiiiiiiffiffiiiiiiiiiiiffiiiiiiiiiiiiiiiiiiisiilliiis";
const char CreatureDataAddonInfofmt[]="iiiiiiis";
const char CreatureModelfmt[]="iffbi";
const char CreatureInfoAddonInfofmt[]="iiiiiiis";
const char EquipmentInfofmt[]="iiiiiiiiii";
const char GameObjectInfofmt[]="iiissiifiiiiiiiiiiiiiiiiiiiiiiiis";
const char ItemPrototypefmt[]="iiiisiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffiffiffiffiffiiiiiiiiiifiiifiiiiiifiiiiiifiiiiiifiiiiiifiiiisiiiiiiiiiiiiiiiiiiiiiiiiifsiiiii";
const char PageTextfmt[]="isi";
const char SpellThreatfmt[]="ii";
const char InstanceTemplatefmt[]="iiiiiiffffs";

SQLStorage sCreatureStorage(CreatureInfofmt,"entry","creature_template");
SQLStorage sCreatureDataAddonStorage(CreatureDataAddonInfofmt,"guid","creature_addon");
SQLStorage sCreatureModelStorage(CreatureModelfmt,"modelid","creature_model_info");
SQLStorage sCreatureInfoAddonStorage(CreatureInfoAddonInfofmt,"entry","creature_template_addon");
SQLStorage sEquipmentStorage(EquipmentInfofmt,"entry","creature_equip_template");
SQLStorage sGOStorage(GameObjectInfofmt,"entry","gameobject_template");
SQLStorage sItemStorage(ItemPrototypefmt,"entry","item_template");
SQLStorage sPageTextStore(PageTextfmt,"entry","page_text");
SQLStorage sSpellThreatStore(SpellThreatfmt,"entry","spell_threat");
SQLStorage sInstanceTemplate(InstanceTemplatefmt,"map","instance_template");

void SQLStorage::Free ()
{
    uint32 offset=0;
    for(uint32 x=0;x<iNumFields;x++)
        if (dst_format[x]==FT_STRING)
        {
            for(uint32 y=0;y<MaxEntry;y++)
                if(pIndex[y])
                    delete [] *(char**)((char*)(pIndex[y])+offset);

            offset += sizeof(char*);
        }
        else if (dst_format[x]==FT_LOGIC)
            offset += sizeof(bool);
        else if (dst_format[x]==FT_BYTE)
            offset += sizeof(char);
        else
            offset += 4;

    delete [] pIndex;
    delete [] data;
}

void SQLStorage::Load()
{
    SQLStorageLoader loader;
    loader.Load(*this);
}