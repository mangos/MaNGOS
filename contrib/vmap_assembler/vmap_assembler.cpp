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

#include <string>
#include <iostream>

#include "TileAssembler.h"

//=======================================================
int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout << "usage: " << argv[0] << " <raw data dir> <vmap dest dir>" << std::endl;
        return 1;
    }

    std::string src = argv[1];
    std::string dest = argv[2];

    std::cout << "using " << src << " as source directory and writing output to " << dest << std::endl;

    VMAP::TileAssembler* ta = new VMAP::TileAssembler(src, dest);

    if(!ta->convertWorld2())
    {
        std::cout << "exit with errors" << std::endl;
        delete ta;
        return 1;
    }

    delete ta;
    std::cout << "Ok, all done" << std::endl;
    return 0;
}
