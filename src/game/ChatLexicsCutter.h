/*
 * Copyright (C) 2005,2006,2007 MaNGOS <http://www.mangosproject.org/>
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

#ifndef MANGOSSERVER_CHATLEXICSCUTTER_H
#define MANGOSSERVER_CHATLEXICSCUTTER_H

typedef std::vector< std::string > LC_AnalogVector;
typedef std::map< std::string, LC_AnalogVector > LC_AnalogMap;
typedef std::set< std::string > LC_LetterSet;
typedef std::vector< LC_LetterSet > LC_WordVector;
typedef std::vector< LC_WordVector > LC_WordList;
typedef std::multimap< std::string, unsigned int > LC_WordMap;

static int trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

class LexicsCutter
{
    protected:
        LC_AnalogMap AnalogMap;
        LC_WordList WordList;
        LC_WordMap WordMap;

        std::string InvalidChars;

    public:
        LexicsCutter();

        static bool ReadUTF8(std::string& in, std::string& out, unsigned int& pos);

        std::string trim(std::string& s, const std::string& drop = " ");
        bool Read_Letter_Analogs(std::string& FileName);
        bool Read_Innormative_Words(std::string& FileName);
        void Map_Innormative_Words();
        bool Compare_Word(std::string& str, unsigned int pos, LC_WordVector word);
        bool Check_Lexics(std::string& Phrase);

        std::vector< std::pair< unsigned int, unsigned int > > Found;
        bool IgnoreMiddleSpaces;
        bool IgnoreLetterRepeat;
};

#endif
