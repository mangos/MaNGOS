/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

const char CreatureInfosrcfmt[]="iiiiiiiiiisssiiiiiiiiiiifffiffiifiiiiiiiiiiffiiiiiiiiiiiiiiiiiiisiiffliiiiiiiliiiis";
const char CreatureInfodstfmt[]="iiiiiiiiiisssiiiiiiiiiiifffiffiifiiiiiiiiiiffiiiiiiiiiiiiiiiiiiisiiffliiiiiiiliiiii";
const char CreatureDataAddonInfofmt[]="iiilliis";
const char CreatureModelfmt[]="iffbii";
const char CreatureInfoAddonInfofmt[]="iiilliis";
const char EquipmentInfofmt[]="iiii";
const char GameObjectInfosrcfmt[]="iiissssiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiis";
const char GameObjectInfodstfmt[]="iiissssiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii";
const char ItemPrototypesrcfmt[]="iiiisiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffiffiiiiiiiiiifiiifiiiiiifiiiiiifiiiiiifiiiiiifiiiisiiiiiiiiiiiiiiiiiiiiiiiiifiiisiiiii";
const char ItemPrototypedstfmt[]="iiiisiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffiffiiiiiiiiiifiiifiiiiiifiiiiiifiiiiiifiiiiiifiiiisiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiii";
const char PageTextfmt[]="isi";
const char InstanceTemplatesrcfmt[]="iiiis";
const char InstanceTemplatedstfmt[]="iiiii";

SQLStorage sCreatureStorage(CreatureInfosrcfmt, CreatureInfodstfmt, "entry","creature_template");
SQLStorage sCreatureDataAddonStorage(CreatureDataAddonInfofmt,"guid","creature_addon");
SQLStorage sCreatureModelStorage(CreatureModelfmt,"modelid","creature_model_info");
SQLStorage sCreatureInfoAddonStorage(CreatureInfoAddonInfofmt,"entry","creature_template_addon");
SQLStorage sEquipmentStorage(EquipmentInfofmt,"entry","creature_equip_template");
SQLStorage sGOStorage(GameObjectInfosrcfmt, GameObjectInfodstfmt, "entry","gameobject_template");
SQLStorage sItemStorage(ItemPrototypesrcfmt, ItemPrototypedstfmt, "entry","item_template");
SQLStorage sPageTextStore(PageTextfmt,"entry","page_text");
SQLStorage sInstanceTemplate(InstanceTemplatesrcfmt, InstanceTemplatedstfmt, "map","instance_template");

void SQLStorage::EraseEntry(uint32 id)
{
    uint32 offset=0;
    for(uint32 x=0;x<iNumFields;x++)
        if (dst_format[x]==FT_STRING)
        {
            if(pIndex[id])
                delete [] *(char**)((char*)(pIndex[id])+offset);

            offset += sizeof(char*);
        }
        else if (dst_format[x]==FT_LOGIC)
            offset += sizeof(bool);
        else if (dst_format[x]==FT_BYTE)
            offset += sizeof(char);
        else
            offset += 4;

    pIndex[id] = NULL;
}

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
