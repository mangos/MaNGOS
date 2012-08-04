/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
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

#include "Common.h"

char const* localeNames[MAX_LOCALE] = {
  "enUS",                                                   // also enGB
  "koKR",
  "frFR",
  "deDE",
  "zhCN",
  "zhTW",
  "esES",
  "esMX",
  "ruRU"
};

// used for search by name or iterate all names
LocaleNameStr const fullLocaleNameList[] =
{
    { "enUS", LOCALE_enUS },
    { "enGB", LOCALE_enUS },
    { "koKR", LOCALE_koKR },
    { "frFR", LOCALE_frFR },
    { "deDE", LOCALE_deDE },
    { "zhCN", LOCALE_zhCN },
    { "zhTW", LOCALE_zhTW },
    { "esES", LOCALE_esES },
    { "esMX", LOCALE_esMX },
    { "ruRU", LOCALE_ruRU },
    { NULL,   LOCALE_enUS }
};

LocaleConstant GetLocaleByName(const std::string& name)
{
    for(LocaleNameStr const* itr = &fullLocaleNameList[0]; itr->name; ++itr)
        if (name==itr->name)
            return itr->locale;

    return LOCALE_enUS;                                     // including enGB case
}
