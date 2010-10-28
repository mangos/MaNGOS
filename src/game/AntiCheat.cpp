/*
 * Copyright (C) 2010 /dev/rsa for MaNGOS <http://getmangos.com/>
 * based on Xeross code
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

#include "Language.h"
#include "Player.h"
#include "World.h"

AntiCheat::AntiCheat(Player* player)
{
    m_player              = player;
    m_anti_lastmovetime   = 0;              //last movement time
    m_anti_NextLenCheck   = 0;
    m_anti_MovedLen       = 0.0f;
    m_anti_BeginFallZ     = INVALID_HEIGHT;
    m_anti_lastalarmtime  = 0;              //last time when alarm generated
    m_anti_alarmcount     = 0;              //alarm counter
    m_anti_TeleTime       = 0;
    m_CanFly              = false;
};

AntiCheat::~AntiCheat()
{
};

bool AntiCheat::CheckNeeded(Player* plMover)
{
    uint32 Anti_TeleTimeDiff = plMover ? time(NULL) - plMover->GetAntiCheat()->Anti__GetLastTeleTime() : time(NULL);
    static const uint32 Anti_TeleTimeIgnoreDiff = sWorld.GetMvAnticheatIgnoreAfterTeleport();

    if (plMover 
        && sWorld.GetMvAnticheatEnable() 
        && GetPlayer()->GetSession()->GetSecurity() <= sWorld.GetMvAnticheatGmLevel() 
        && !plMover->GetTransport() 
        && !GetPlayer()->HasMovementFlag(MOVEFLAG_ONTRANSPORT) 
        && GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE 
        && !GetPlayer()->IsTaxiFlying() 
        && Anti_TeleTimeDiff > Anti_TeleTimeIgnoreDiff)
        return true;

    return false;
}

bool AntiCheat::CheckMovement(Player* plMover, MovementInfo& movementInfo, uint32 opcode)
{
    const uint32 CurTime = getMSTime();

    if (getMSTimeDiff(m_anti_lastalarmtime,CurTime) > sWorld.GetMvAnticheatAlarmPeriod())
    {
        m_anti_alarmcount = 0;
    }
        /* I really don't care about movement-type yet (todo)
        UnitMoveType move_type;

        if (movementInfo.flags & MOVEMENTFLAG_FLYING) move_type = MOVE_FLY;
        else if (movementInfo.flags & MOVEMENTFLAG_SWIMMING) move_type = MOVE_SWIM;
        else if (movementInfo.flags & MOVEMENTFLAG_WALK_MODE) move_type = MOVE_WALK;
        else move_type = MOVE_RUN;*/

    float delta_x = GetPlayer()->GetPositionX() - movementInfo.GetPos()->x;
    float delta_y = GetPlayer()->GetPositionY() - movementInfo.GetPos()->y;
    float delta_z = GetPlayer()->GetPositionZ() - movementInfo.GetPos()->z;
    float delta = sqrt(delta_x * delta_x + delta_y * delta_y); // Len of movement-vector via Pythagoras (a^2+b^2=Len^2)
    float tg_z = 0.0f; //tangens
    float delta_t = getMSTimeDiff(m_anti_lastmovetime,CurTime);

    m_anti_lastmovetime = CurTime;
    m_anti_MovedLen += delta;

    if (delta_t > 15000.0f)
        delta_t = 15000.0f;

    // Tangens of walking angel
    if (!(movementInfo.GetMovementFlags() & (MOVEFLAG_FLYING | MOVEFLAG_SWIMMING)))
    {
        tg_z = ((delta !=0.0f) && (delta_z > 0.0f)) ? (atan((delta_z*delta_z) / delta) * 180.0f / M_PI) : 0.0f;
    }

    //antiOFF fall-damage, MOVEMENTFLAG_UNK4 seted by client if player try movement when falling and unset in this case the MOVEMENTFLAG_FALLING flag. 
    if ((!CanFly() && m_anti_BeginFallZ == INVALID_HEIGHT) &&
        (movementInfo.GetMovementFlags() & (MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR)) != 0)
    {
        m_anti_BeginFallZ=(float)(movementInfo.GetPos()->z);
    }

    if (m_anti_NextLenCheck <= CurTime)
    {
        // Check every 500ms is a lot more advisable then 1000ms, because normal movment packet arrives every 500ms
        uint32 OldNextLenCheck = m_anti_NextLenCheck;
        float delta_xyt = m_anti_MovedLen/(float)(getMSTimeDiff(OldNextLenCheck-500,CurTime));
        m_anti_NextLenCheck = CurTime+500;
        m_anti_MovedLen = 0.0f;
        static const float MaxDeltaXYT = sWorld.GetMvAnticheatMaxXYT();

        if (delta_xyt > MaxDeltaXYT && delta<=100.0f)
        {
            if (sWorld.GetMvAnticheatSpeedCheck())
                GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Speed hack",delta_xyt,LookupOpcodeName(opcode),
                (float)(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()),
                    (float)(getMSTimeDiff(OldNextLenCheck-500,CurTime)));
        }

        if (delta > 100.0f)
        {
            if (sWorld.GetMvAnticheatTeleportCheck())
                GetPlayer()->GetSession()->Anti__ReportCheat("Tele hack",delta,LookupOpcodeName(opcode));
        }

        // Check for waterwalking . Fix new way of checking for waterwalking by Darky88
        if (movementInfo.HasMovementFlag(MOVEFLAG_WATERWALKING) &&
            !(GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK) || GetPlayer()->HasAuraType(SPELL_AURA_GHOST)))
        {
            if(sWorld.GetMvAnticheatWaterCheck())
                GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Water walking",0.0f,NULL,0.0f,(uint32)(movementInfo.GetMovementFlags()));
        }

        // Check for walking upwards a mountain while not beeing able to do that, New check by Darky88 
        if ((delta_z < -2.3f) && (tg_z > 2.37f))
        {
            if (sWorld.GetMvAnticheatMountainCheck())
                GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Mountain hack",tg_z,NULL,delta,delta_z);
        }

        static const float DIFF_OVERGROUND = 10.0f;
        float Anti__GroundZ = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),MAX_HEIGHT);
        float Anti__FloorZ  = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ());
        float Anti__MapZ = ((Anti__FloorZ <= (INVALID_HEIGHT+5.0f)) ? Anti__GroundZ : Anti__FloorZ) + DIFF_OVERGROUND;

        if (CanFly() &&
            !GetPlayer()->HasAuraType(SPELL_AURA_FEATHER_FALL) &&
            !GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z-7.0f) &&
            Anti__MapZ < GetPlayer()->GetPositionZ() && Anti__MapZ > (INVALID_HEIGHT+DIFF_OVERGROUND + 5.0f))
        {
            static const float DIFF_AIRJUMP=25.0f; // 25 is realy high, but to many false positives...

            // Air-Jump-Detection definitively needs a better way to be detected...
            if ((movementInfo.GetMovementFlags() & (MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING | MOVEFLAG_ROOT)) != 0) // Fly Hack
            {
                // Fix Aura 55164
                if (!GetPlayer()->HasAura(55164))
                    if (sWorld.GetMvAnticheatFlyCheck())
                        GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Fly hack",
                            ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_FLY))) +
                            ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED))*2),
                            NULL,GetPlayer()->GetPositionZ() - Anti__MapZ);
            }

            // Need a better way to do that - currently a lot of fake alarms
            else if ((Anti__MapZ+DIFF_AIRJUMP < GetPlayer()->GetPositionZ() &&
                    (movementInfo.GetMovementFlags() & (MOVEFLAG_FALLINGFAR | MOVEFLAG_PENDINGSTOP))==0) ||
                    (Anti__MapZ < GetPlayer()->GetPositionZ() && opcode==MSG_MOVE_JUMP))
            {
                if (sWorld.GetMvAnticheatJumpCheck())
                    GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Possible Air Jump Hack",0.0f,LookupOpcodeName(opcode),0.0f,movementInfo.GetMovementFlags());
            }
        }

        /*if(Anti__FloorZ < -199900.0f && Anti__GroundZ >= -199900.0f &&
           GetPlayer()->GetPositionZ()+5.0f < Anti__GroundZ)
        {
            Anti__CheatOccurred(CurTime,"Teleport2Plane hack",
                                GetPlayer()->GetPositionZ(),NULL,Anti__GroundZ);
        }*/

        //Teleport To Plane checks
        if (movementInfo.GetPos()->z < 0.0001f && movementInfo.GetPos()->z > -0.0001f && (!movementInfo.HasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING))))
        {
            if(sWorld.GetMvAnticheatTeleport2PlaneCheck())
            {
                // Prevent using TeleportToPlan.
                Map *map = GetPlayer()->GetMap();
                if (map)
                {
                    float plane_z = map->GetHeight(movementInfo.GetPos()->x, movementInfo.GetPos()->y, MAX_HEIGHT) - movementInfo.GetPos()->z;
                    plane_z = (plane_z < -500.0f) ? 0 : plane_z; //check holes in heigth map
                    if(plane_z > 0.1f || plane_z < -0.1f)
                    {
                        if(sWorld.GetMvAnticheatTeleport2PlaneCheck())
                            GetPlayer()->GetSession()->Anti__CheatOccurred(CurTime,"Teleport2Plane hack",GetPlayer()->GetPositionZ(),NULL,plane_z);
                    }
                }
            }
        }
    }
    return true;
}

bool AntiCheat::CheckOnTransport(MovementInfo& movementInfo)
{
    float trans_rad = movementInfo.GetTransportPos()->x*movementInfo.GetTransportPos()->x + movementInfo.GetTransportPos()->y*movementInfo.GetTransportPos()->y + movementInfo.GetTransportPos()->z*movementInfo.GetTransportPos()->z;
    if (trans_rad > 3600.0f) // transport radius = 60 yards //cheater with on_transport_flag
        return false;

    return true;
}

/*
std::string FlagsToStr(const uint32 Flags)
{
    std::string Ret="";
    if(Flags==0)
    {
        Ret="None";
        return Ret;
    }

    if(Flags & MOVEFLAG_FORWARD)
    {   Ret+="FW "; }
    if(Flags & MOVEFLAG_BACKWARD)
    {   Ret+="BW "; }
    if(Flags & MOVEFLAG_STRAFE_LEFT)
    {   Ret+="STL ";    }
    if(Flags & MOVEFLAG_STRAFE_RIGHT)
    {   Ret+="STR ";    }
    if(Flags & MOVEFLAG_LEFT)
    {   Ret+="LF "; }
    if(Flags & MOVEFLAG_RIGHT)
    {   Ret+="RI "; }
    if(Flags & MOVEFLAG_PITCH_UP)
    {   Ret+="PTUP ";   }
    if(Flags & MOVEFLAG_PITCH_DOWN)
    {   Ret+="PTDW ";   }
    if(Flags & MOVEFLAG_WALK_MODE)
    {   Ret+="WALK ";   }
    if(Flags & MOVELAG_ONTRANSPORT)
    {   Ret+="TRANS ";  }
    if(Flags & MOVEFLAG_LEVITATING)
    {   Ret+="LEVI ";   }
    if(Flags & MOVEFLAG_FLY_UNK1)
    {   Ret+="FLYUNK1 ";    }
    if(Flags & MOVEFLAG_JUMPING)
    {   Ret+="JUMP ";   }
    if(Flags & MOVEFLAG_UNK4)
    {   Ret+="UNK4 ";   }
    if(Flags & MOVEFLAG_FALLING)
    {   Ret+="FALL ";   }
    if(Flags & MOVEFLAG_SWIMMING)
    {   Ret+="SWIM ";   }
    if(Flags & MOVEFLAG_FLY_UP)
    {   Ret+="FLYUP ";  }
    if(Flags & MOVEFLAG_CAN_FLY)
    {   Ret+="CFLY ";   }
    if(Flags & MOVEFLAG_FLYING)
    {   Ret+="FLY ";    }
    if(Flags & MOVEFLAG_FLYING2)
    {   Ret+="FLY2 ";   }
    if(Flags & MOVEFLAG_WATERWALKING)
    {   Ret+="WTWALK "; }
    if(Flags & MOVEFLAG_SAFE_FALL)
    {   Ret+="SAFE ";   }
    if(Flags & MOVEFLAG_UNK3)
    {   Ret+="UNK3 ";   }
    if(Flags & MOVEFLAG_SPLINE)
    {   Ret+="SPLINE ";     }
    if(Flags & MOVEFLAG_SPLINE2)
    {   Ret+="SPLINE2 ";    }

    return Ret;
}
*/

bool WorldSession::Anti__ReportCheat(const char* Reason,float Speed,const char* Op,float Val1,uint32 Val2)
{
    if(!Reason)
    {
        sLog.outError("Anti__ReportCheat: Missing Reason parameter!");
        return false;
    }
    const char* Player=GetPlayer()->GetName();
    uint32 Acc=GetPlayer()->GetSession()->GetAccountId();
    uint32 Map=GetPlayer()->GetMapId();
    if(!Player)
    {
        sLog.outError("Anti__ReportCheat: Player with no name?!?");
        return false;
    }

    QueryResult *Res=CharacterDatabase.PQuery("SELECT speed,Val1 FROM cheaters WHERE player='%s' AND reason LIKE '%s' AND Map='%u' AND last_date >= NOW()-300",Player,Reason,Map);
    if(Res)
    {
        Field* Fields = Res->Fetch();

        std::stringstream Query;
        Query << "UPDATE cheaters SET count=count+1,last_date=NOW()";
        Query.precision(5);
        if(Speed>0.0f && Speed > Fields[0].GetFloat())
        {
            Query << ",speed='";
            Query << std::fixed << Speed;
            Query << "'";
        }

        if(Val1>0.0f && Val1 > Fields[1].GetFloat())
        {
            Query << ",Val1='";
            Query << std::fixed << Val1;
            Query << "'";
        }

        Query << " WHERE player='" << Player << "' AND reason='" << Reason << "' AND Map='" << Map << "' AND last_date >= NOW()-300 ORDER BY entry DESC LIMIT 1";

        CharacterDatabase.Execute(Query.str().c_str());
        delete Res;
    }
    else
    {
        if(!Op)
        {   Op="";  }
        std::stringstream Pos;
        Pos << "OldPos: " << GetPlayer()->GetPositionX() << " " << GetPlayer()->GetPositionY() << " "
            << GetPlayer()->GetPositionZ();
        CharacterDatabase.PExecute("INSERT INTO cheaters (player,acctid,reason,speed,count,first_date,last_date,`Op`,Val1,Val2,Map,Pos,Level) "
                                   "VALUES ('%s','%u','%s','%f','1',NOW(),NOW(),'%s','%f','%u','%u','%s','%u')",
                                   Player,Acc,Reason,Speed,Op,Val1,Val2,Map,
                                   Pos.str().c_str(),GetPlayer()->getLevel());
    }

    if(sWorld.GetMvAnticheatKill() && GetPlayer()->isAlive())
    {
        GetPlayer()->DealDamage(GetPlayer(), GetPlayer()->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
    }
    if(sWorld.GetMvAnticheatKick())
    {
        GetPlayer()->GetSession()->KickPlayer();
    }
    if(sWorld.GetMvAnticheatBan() & 1)
    {
        uint32 duration_secs = TimeStringToSecs(sWorld.GetMvAnticheatBanTime());
        sWorld.BanAccount(BAN_CHARACTER,Player,duration_secs,(char*)"Cheat",(char*)"Anticheat");
    }
    if(sWorld.GetMvAnticheatBan() & 2)
    {
        QueryResult *result = LoginDatabase.PQuery("SELECT last_ip FROM account WHERE id=%u", Acc);
        if(result)
        {

            Field *fields = result->Fetch();
            std::string LastIP = fields[0].GetCppString();
            if(!LastIP.empty())
            {
                uint32 duration_secs = TimeStringToSecs(sWorld.GetMvAnticheatBanTime());
                sWorld.BanAccount(BAN_IP,LastIP,duration_secs,(char*)"Cheat",(char*)"Anticheat");
            }
            delete result;
        }
    }
    return true;
}

bool WorldSession::Anti__CheatOccurred(uint32 CurTime,const char* Reason,float Speed,const char* Op, float Val1,uint32 Val2)
{
    if(!Reason)
    {
        sLog.outError("Anti__CheatOccurred: Missing Reason parameter!");
        return false;
    }

    GetPlayer()->GetAntiCheat()->m_anti_lastalarmtime = CurTime;
    GetPlayer()->GetAntiCheat()->m_anti_alarmcount = GetPlayer()->GetAntiCheat()->m_anti_alarmcount + 1;

    if (GetPlayer()->GetAntiCheat()->m_anti_alarmcount > sWorld.GetMvAnticheatAlarmCount())
    {
        Anti__ReportCheat(Reason,Speed,Op,Val1,Val2);
        if (sWorld.GetMvAnticheatAnnounce())
            sWorld.SendWorldText(LANG_ANNOUNCE_CHEAT, GetPlayer()->GetName(), Reason);
        return true;
    }
    return false;
}
