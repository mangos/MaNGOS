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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "Vehicle.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Transports.h"
#include "BattleGround.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Language.h"

//#define __ANTI_DEBUG__

#ifdef __ANTI_DEBUG__
#include "Chat.h"
std::string FlagsToStr(const uint32 Flags)
{
    std::string Ret="";
    if(Flags==0)
    {
        Ret="None";
        return Ret;
    }

    if(Flags & MOVEMENTFLAG_FORWARD)
    {   Ret+="FW "; }
    if(Flags & MOVEMENTFLAG_BACKWARD)
    {   Ret+="BW "; }
    if(Flags & MOVEMENTFLAG_STRAFE_LEFT)
    {   Ret+="STL ";    }
    if(Flags & MOVEMENTFLAG_STRAFE_RIGHT)
    {   Ret+="STR ";    }
    if(Flags & MOVEMENTFLAG_LEFT)
    {   Ret+="LF "; }
    if(Flags & MOVEMENTFLAG_RIGHT)
    {   Ret+="RI "; }
    if(Flags & MOVEMENTFLAG_PITCH_UP)
    {   Ret+="PTUP ";   }
    if(Flags & MOVEMENTFLAG_PITCH_DOWN)
    {   Ret+="PTDW ";   }
    if(Flags & MOVEMENTFLAG_WALK_MODE)
    {   Ret+="WALK ";   }
    if(Flags & MOVEMENTFLAG_ONTRANSPORT)
    {   Ret+="TRANS ";  }
    if(Flags & MOVEMENTFLAG_LEVITATING)
    {   Ret+="LEVI ";   }
    if(Flags & MOVEMENTFLAG_FLY_UNK1)
    {   Ret+="FLYUNK1 ";    }
    if(Flags & MOVEMENTFLAG_JUMPING)
    {   Ret+="JUMP ";   }
    if(Flags & MOVEMENTFLAG_UNK4)
    {   Ret+="UNK4 ";   }
    if(Flags & MOVEMENTFLAG_FALLING)
    {   Ret+="FALL ";   }
    if(Flags & MOVEMENTFLAG_SWIMMING)
    {   Ret+="SWIM ";   }
    if(Flags & MOVEMENTFLAG_FLY_UP)
    {   Ret+="FLYUP ";  }
    if(Flags & MOVEMENTFLAG_CAN_FLY)
    {   Ret+="CFLY ";   }
    if(Flags & MOVEMENTFLAG_FLYING)
    {   Ret+="FLY ";    }
    if(Flags & MOVEMENTFLAG_FLYING2)
    {   Ret+="FLY2 ";   }
    if(Flags & MOVEMENTFLAG_WATERWALKING)
    {   Ret+="WTWALK "; }
    if(Flags & MOVEMENTFLAG_SAFE_FALL)
    {   Ret+="SAFE ";   }
   if(Flags & MOVEMENTFLAG_UNK3)
    {   Ret+="UNK3 ";   }
    if(Flags & MOVEMENTFLAG_SPLINE)
    {   Ret+="SPLINE ";     }
    if(Flags & MOVEMENTFLAG_SPLINE2)
    {   Ret+="SPLINE2 ";    }

    return Ret;
}
#endif // __ANTI_DEBUG__

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
        sWorld.BanAccount(BAN_CHARACTER,Player,sWorld.GetMvAnticheatBanTime(),"Cheat","Anticheat");
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
                sWorld.BanAccount(BAN_IP,LastIP,sWorld.GetMvAnticheatBanTime(),"Cheat","Anticheat");
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

    GetPlayer()->m_anti_lastalarmtime = CurTime;
    GetPlayer()->m_anti_alarmcount = GetPlayer()->m_anti_alarmcount + 1;

    if (GetPlayer()->m_anti_alarmcount > sWorld.GetMvAnticheatAlarmCount())
    {
        Anti__ReportCheat(Reason,Speed,Op,Val1,Val2);
        if (sWorld.GetMvAnticheatAnnounce())
            sWorld.SendWorldText(LANG_ANNOUNCE_CHEAT, GetPlayer()->GetName(), Reason);
        return true;
    }
    return false;
}

void WorldSession::HandleMoveWorldportAckOpcode( WorldPacket & /*recv_data*/ )
{
    DEBUG_LOG( "WORLD: got MSG_MOVE_WORLDPORT_ACK." );
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if(!GetPlayer()->IsBeingTeleportedFar())
        return;

    // get the teleport destination
    WorldLocation &loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check
    if(!MapManager::IsValidMapCoord(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation))
    {
        sLog.outError("WorldSession::HandleMoveWorldportAckOpcode: player %s (%d) was teleported far to a not valid location. (map:%u, x:%f, y:%f, "
            "z:%f) We port him to his homebind instead..", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);
        // stop teleportation else we would try this again and again in LogoutPlayer...
        GetPlayer()->SetSemaphoreTeleportFar(false);
        // and teleport the player to a valid place
        GetPlayer()->TeleportToHomebind();
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.mapid);
    InstanceTemplate const* mInstance = ObjectMgr::GetInstanceTemplate(loc.mapid);

    // reset instance validity, except if going to an instance inside an instance
    if(GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;

    GetPlayer()->SetSemaphoreTeleportFar(false);

    // relocate the player to the teleport destination
    GetPlayer()->SetMap(sMapMgr.CreateMap(loc.mapid, GetPlayer()));
    GetPlayer()->Relocate(loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation);
    GetPlayer()->m_anti_TeleTime=time(NULL);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if(!GetPlayer()->GetMap()->Add(GetPlayer()))
    {
        //if player wasn't added to map, reset his map pointer!
        GetPlayer()->ResetMap();

        sLog.outError("WorldSession::HandleMoveWorldportAckOpcode: player %s (%d) was teleported far but couldn't be added to map. (map:%u, x:%f, y:%f, "
            "z:%f) We port him to his homebind instead..", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);
        // teleport the player home
        GetPlayer()->TeleportToHomebind();
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if(_player->InBattleGround())
    {
        // cleanup setting if outdated
        if(!mEntry->IsBattleGroundOrArena())
        {
            // We're not in BG
            _player->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);
            // reset destination bg team
            _player->SetBGTeam(0);
        }
        // join to bg case
        else if(BattleGround *bg = _player->GetBattleGround())
        {
            if(_player->IsInvitedForBattleGroundInstance(_player->GetBattleGroundId()))
                bg->AddPlayer(_player);
        }
    }

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // flight fast teleport case
    if(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
    {
        if(!_player->InBattleGround())
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Reset(*GetPlayer());
            return;
        }

        // battleground state prepare, stop flight
        GetPlayer()->GetMotionMaster()->MovementExpired();
        GetPlayer()->m_taxi.ClearTaxiDestinations();
    }

    // resurrect character at enter into instance where his corpse exist after add to map
    Corpse *corpse = GetPlayer()->GetCorpse();
    if (corpse && corpse->GetType() != CORPSE_BONES && corpse->GetMapId() == GetPlayer()->GetMapId())
    {
        if( mEntry->IsDungeon() )
        {
            GetPlayer()->ResurrectPlayer(0.5f);
            GetPlayer()->SpawnCorpseBones();
        }
    }

    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if(MapDifficulty const* mapDiff = GetMapDifficultyData(mEntry->MapID,diff))
        {
            if (mapDiff->resetTime)
            {
                if (time_t timeReset = sInstanceSaveMgr.GetScheduler().GetResetTimeFor(mEntry->MapID,diff))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->MapID, diff, timeleft);
                }
            }
        }
    }

    // mount allow check
    if(!mEntry->IsMountAllowed())
        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

    // honorless target
    if(GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();
    GetPlayer()->Anti__SetLastTeleTime(::time(NULL));
    GetPlayer()->m_anti_BeginFallZ=INVALID_HEIGHT;

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMoveTeleportAck(WorldPacket& recv_data)
{
    DEBUG_LOG("MSG_MOVE_TELEPORT_ACK");

    ObjectGuid guid;

    recv_data >> guid.ReadAsPacked();

    uint32 flags, time;
    recv_data >> flags >> time;
    DEBUG_LOG("Guid: %s", guid.GetString().c_str());
    DEBUG_LOG("Flags %u, time %u", flags, time/IN_MILLISECONDS);

    Unit *mover = _player->GetMover();
    Player *plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    if(!plMover || !plMover->IsBeingTeleportedNear())
        return;

    if(guid != plMover->GetObjectGuid())
        return;

    plMover->SetSemaphoreTeleportNear(false);

    uint32 old_zone = plMover->GetZoneId();

    WorldLocation const& dest = plMover->GetTeleportDest();

    plMover->SetPosition(dest.coord_x, dest.coord_y, dest.coord_z, dest.orientation, true);

    uint32 newzone, newarea;
    plMover->GetZoneAndAreaId(newzone, newarea);
    plMover->UpdateZone(newzone, newarea);

    // new zone
    if(old_zone != newzone)
    {
        // honorless target
        if(plMover->pvpInfo.inHostileArea)
            plMover->CastSpell(plMover, 2479, true);
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();
    if(plMover)
    {
        plMover->Anti__SetLastTeleTime(::time(NULL));
        plMover->m_anti_BeginFallZ=INVALID_HEIGHT;
    }

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMovementOpcodes( WorldPacket & recv_data )
{
    uint32 opcode = recv_data.GetOpcode();
    DEBUG_LOG("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);
    recv_data.hexlike();

    Unit *mover = _player->GetMover();
    Player *plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if(plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    /* extract packet */
    ObjectGuid guid;
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> movementInfo;
    /*----------------*/

    if (!MaNGOS::IsValidMapCoord(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z, movementInfo.GetPos()->o))
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    /* handle special cases */
    if (movementInfo.HasMovementFlag(MOVEFLAG_ONTRANSPORT))
    {
        // transports size limited
        // (also received at zeppelin/lift leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if( movementInfo.GetTransportPos()->x > 50 || movementInfo.GetTransportPos()->y > 50 || movementInfo.GetTransportPos()->z > 100 )
        {
            recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
            return;
        }

        if( !MaNGOS::IsValidMapCoord(movementInfo.GetPos()->x + movementInfo.GetTransportPos()->x, movementInfo.GetPos()->y + movementInfo.GetTransportPos()->y,
            movementInfo.GetPos()->z + movementInfo.GetTransportPos()->z, movementInfo.GetPos()->o + movementInfo.GetTransportPos()->o) )
        {
            recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
            return;
        }

        // if we boarded a transport, add us to it
        if (plMover && !plMover->m_transport)
        {
            float trans_rad = movementInfo.GetTransportPos()->x*movementInfo.GetTransportPos()->x + movementInfo.GetTransportPos()->y*movementInfo.GetTransportPos()->y + movementInfo.GetTransportPos()->z*movementInfo.GetTransportPos()->z;
            if (trans_rad > 3600.0f) // transport radius = 60 yards //cheater with on_transport_flag
            {
 	            return;
            }
            // elevators also cause the client to send MOVEFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
            for (MapManager::TransportSet::const_iterator iter = sMapMgr.m_Transports.begin(); iter != sMapMgr.m_Transports.end(); ++iter)
            {
                if ((*iter)->GetObjectGuid() == movementInfo.GetTransportGuid())
                {
                    plMover->m_transport = (*iter);
                    (*iter)->AddPassenger(plMover);
                    break;
                }
            }
        }
    }
    else if (plMover && plMover->m_transport)               // if we were on a transport, leave
    {
        plMover->m_transport->RemovePassenger(plMover);
        plMover->m_transport = NULL;
        movementInfo.ClearTransportData();
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == MSG_MOVE_FALL_LAND && plMover && !plMover->isInFlight())
        plMover->HandleFall(movementInfo);

    if (plMover && (movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING) != plMover->IsInWater()))
    {
        // now client not include swimming flag in case jumping under water
        plMover->SetInWater( !plMover->IsInWater() || plMover->GetBaseMap()->IsUnderWater(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z) );
        if(plMover->GetBaseMap()->IsUnderWater(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z-7.0f))
        {
            plMover->m_anti_BeginFallZ=INVALID_HEIGHT;
        }
    }

    // ---- anti-cheat features -->>>
    uint32 Anti_TeleTimeDiff=plMover ? time(NULL) - plMover->Anti__GetLastTeleTime() : time(NULL);
    static const uint32 Anti_TeleTimeIgnoreDiff=sWorld.GetMvAnticheatIgnoreAfterTeleport();
    if (plMover && (plMover->m_transport == 0) && sWorld.GetMvAnticheatEnable() &&
        GetPlayer()->GetSession()->GetSecurity() <= sWorld.GetMvAnticheatGmLevel() &&
        GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()!=FLIGHT_MOTION_TYPE &&
        Anti_TeleTimeDiff>Anti_TeleTimeIgnoreDiff)
    {
        const uint32 CurTime=getMSTime();
        if (getMSTimeDiff(GetPlayer()->m_anti_lastalarmtime,CurTime) > sWorld.GetMvAnticheatAlarmPeriod())
        {
            GetPlayer()->m_anti_alarmcount = 0;
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
        float delta_t = getMSTimeDiff(GetPlayer()->m_anti_lastmovetime,CurTime);

        GetPlayer()->m_anti_lastmovetime = CurTime;
        GetPlayer()->m_anti_MovedLen += delta;

        if (delta_t > 15000.0f)
        {   delta_t = 15000.0f;   }

        // Tangens of walking angel
        if (!(movementInfo.GetMovementFlags() & (MOVEFLAG_FLYING | MOVEFLAG_SWIMMING)))
        {
            tg_z = ((delta !=0.0f) && (delta_z > 0.0f)) ? (atan((delta_z*delta_z) / delta) * 180.0f / M_PI) : 0.0f;
        }

        //antiOFF fall-damage, MOVEMENTFLAG_UNK4 seted by client if player try movement when falling and unset in this case the MOVEMENTFLAG_FALLING flag. 
        if ((!GetPlayer()->CanFly() && GetPlayer()->m_anti_BeginFallZ == INVALID_HEIGHT) &&
            (movementInfo.GetMovementFlags() & (MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR)) != 0)
        {
            GetPlayer()->m_anti_BeginFallZ=(float)(movementInfo.GetPos()->z);
        }

        if (GetPlayer()->m_anti_NextLenCheck <= CurTime)
        {
            // Check every 500ms is a lot more advisable then 1000ms, because normal movment packet arrives every 500ms
            uint32 OldNextLenCheck=GetPlayer()->m_anti_NextLenCheck;
            float delta_xyt=GetPlayer()->m_anti_MovedLen/(float)(getMSTimeDiff(OldNextLenCheck-500,CurTime));
            GetPlayer()->m_anti_NextLenCheck = CurTime+500;
            GetPlayer()->m_anti_MovedLen = 0.0f;
            static const float MaxDeltaXYT = sWorld.GetMvAnticheatMaxXYT();

            if (delta_xyt > MaxDeltaXYT && delta<=100.0f && GetPlayer()->GetZoneId() != 2257)
            {
                if (sWorld.GetMvAnticheatSpeedCheck())
                    Anti__CheatOccurred(CurTime,"Speed hack",delta_xyt,LookupOpcodeName(opcode),
                    (float)(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()),
                    (float)(getMSTimeDiff(OldNextLenCheck-500,CurTime)));
            }
        }

        if (delta > 100.0f && GetPlayer()->GetZoneId() != 2257)
        {
            if (sWorld.GetMvAnticheatTeleportCheck())
                Anti__ReportCheat("Tele hack",delta,LookupOpcodeName(opcode));
        }

        // Check for waterwalking . Fix new way of checking for waterwalking by Darky88
        if (movementInfo.HasMovementFlag(MOVEFLAG_WATERWALKING) &&
            !(GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK) || GetPlayer()->HasAuraType(SPELL_AURA_GHOST)))
        {
            if(sWorld.GetMvAnticheatWaterCheck())
                Anti__CheatOccurred(CurTime,"Water walking",0.0f,NULL,0.0f,(uint32)(movementInfo.GetMovementFlags()));
        }

        // Check for walking upwards a mountain while not beeing able to do that, New check by Darky88 
        if ((delta_z < -2.3f) && (tg_z > 2.37f))
        {
            if (sWorld.GetMvAnticheatMountainCheck())
                Anti__CheatOccurred(CurTime,"Mountain hack",tg_z,NULL,delta,delta_z);
        }

        static const float DIFF_OVERGROUND = 10.0f;
        float Anti__GroundZ = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),MAX_HEIGHT);
        float Anti__FloorZ  = GetPlayer()->GetMap()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ());
        float Anti__MapZ = ((Anti__FloorZ <= (INVALID_HEIGHT+5.0f)) ? Anti__GroundZ : Anti__FloorZ) + DIFF_OVERGROUND;
         
        if (!GetPlayer()->CanFly() &&
            !GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z-7.0f) &&
            Anti__MapZ < GetPlayer()->GetPositionZ() && Anti__MapZ > (INVALID_HEIGHT+DIFF_OVERGROUND + 5.0f))
        {
            static const float DIFF_AIRJUMP=25.0f; // 25 is realy high, but to many false positives...

            // Air-Jump-Detection definitively needs a better way to be detected...
            if ((movementInfo.GetMovementFlags() & (MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING | MOVEFLAG_ROOT)) != 0) // Fly Hack
            {
                // Fix Aura 55164
                if (!GetPlayer()->HasAura(55164) || !GetPlayer()->HasAuraType(SPELL_AURA_FEATHER_FALL))
                    if (sWorld.GetMvAnticheatFlyCheck())
                        Anti__CheatOccurred(CurTime,"Fly hack",
                            ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_FLY))) +
                            ((uint8)(GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED))*2),
                            NULL,GetPlayer()->GetPositionZ()-Anti__MapZ);
            }

            // Need a better way to do that - currently a lot of fake alarms
            else if ((Anti__MapZ+DIFF_AIRJUMP < GetPlayer()->GetPositionZ() &&
                    (movementInfo.GetMovementFlags() & (MOVEFLAG_FALLINGFAR | MOVEFLAG_PENDINGSTOP))==0) ||
                    (Anti__MapZ < GetPlayer()->GetPositionZ() && opcode==MSG_MOVE_JUMP) &&
                    !GetPlayer()->HasAuraType(SPELL_AURA_FEATHER_FALL))
            {
                if (sWorld.GetMvAnticheatJumpCheck())
                    Anti__CheatOccurred(CurTime,"Possible Air Jump Hack",0.0f,LookupOpcodeName(opcode),0.0f,movementInfo.GetMovementFlags());
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
                            Anti__CheatOccurred(CurTime,"Teleport2Plane hack",GetPlayer()->GetPositionZ(),NULL,plane_z);
                    }
                }
            }
        }
    }
    // <<---- anti-cheat features

    /* process position-change */
    movementInfo.UpdateTime(getMSTime());

    WorldPacket data(opcode, recv_data.size());
    data.appendPackGUID(mover->GetGUID());                  // write guid
    movementInfo.Write(data);                               // write data
    mover->SendMessageToSetExcept(&data, _player);

    if(plMover)                                             // nothing is charmed, or player charmed
    {
        plMover->SetPosition(movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z, movementInfo.GetPos()->o);
        plMover->m_movementInfo = movementInfo;
        plMover->UpdateFallInformationIfNeed(movementInfo, opcode);

        // after move info set
        if ((opcode == MSG_MOVE_SET_WALK_MODE || opcode == MSG_MOVE_SET_RUN_MODE))
            plMover->UpdateWalkMode(plMover, false);

        if(plMover->isMovingOrTurning())
            plMover->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        if(movementInfo.GetPos()->z < -500.0f)
        {
            if(plMover->InBattleGround()
                && plMover->GetBattleGround()
                && plMover->GetBattleGround()->HandlePlayerUnderMap(_player))
            {
                // do nothing, the handle already did if returned true
            }
            else
            {
                // NOTE: this is actually called many times while falling
                // even after the player has been teleported away
                // TODO: discard movement packets after the player is rooted
                if(plMover->isAlive())
                {
                    plMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, plMover->GetMaxHealth());
                    // pl can be alive if GM/etc
                    if(!plMover->isAlive())
                    {
                        // change the death state to CORPSE to prevent the death timer from
                        // starting in the next player update
                        plMover->KillPlayer();
                        plMover->BuildPlayerRepop();
                    }
                }

                // cancel the death timer here if started
                plMover->RepopAtGraveyard();
            }
        }
    }
    else                                                    // creature charmed
    {
        if(mover->IsInWorld())
            mover->GetMap()->CreatureRelocation((Creature*)mover, movementInfo.GetPos()->x, movementInfo.GetPos()->y, movementInfo.GetPos()->z, movementInfo.GetPos()->o);
    }
}

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recv_data)
{
    uint32 opcode = recv_data.GetOpcode();
    DEBUG_LOG("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);
    /* extract packet */
    ObjectGuid guid;
    MovementInfo movementInfo;
    float  newspeed;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // counter or moveEvent
    recv_data >> movementInfo;
    recv_data >> newspeed;

    // now can skip not our packet
    if(_player->GetObjectGuid() != guid)
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    switch(opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:          move_type = MOVE_WALK;          force_move_type = MOVE_WALK;        break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:           move_type = MOVE_RUN;           force_move_type = MOVE_RUN;         break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:      move_type = MOVE_RUN_BACK;      force_move_type = MOVE_RUN_BACK;    break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:          move_type = MOVE_SWIM;          force_move_type = MOVE_SWIM;        break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:     move_type = MOVE_SWIM_BACK;     force_move_type = MOVE_SWIM_BACK;   break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK:           move_type = MOVE_TURN_RATE;     force_move_type = MOVE_TURN_RATE;   break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:        move_type = MOVE_FLIGHT;        force_move_type = MOVE_FLIGHT;      break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:   move_type = MOVE_FLIGHT_BACK;   force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK:          move_type = MOVE_PITCH_RATE;    force_move_type = MOVE_PITCH_RATE;  break;
        default:
            sLog.outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if(_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if(_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }

    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if(_player->GetSpeed(move_type) > newspeed)         // must be greater - just correct
        {
            sLog.outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeedRate(move_type,_player->GetSpeedRate(move_type),true);
        }
        else                                                // must be lesser - cheating
        {
            BASIC_LOG("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(),_player->GetSession()->GetAccountId(),_player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_SET_ACTIVE_MOVER");
    recv_data.hexlike();

    ObjectGuid guid;
    recv_data >> guid;

    if (Unit *pMover = ObjectAccessor::GetUnit(*GetPlayer(), guid))
        GetPlayer()->SetMover(pMover);
    else
        GetPlayer()->SetMover(NULL);
}

void WorldSession::HandleMoveNotActiveMover(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");
    recv_data.hexlike();

    ObjectGuid old_mover_guid;
    MovementInfo mi;

    recv_data >> old_mover_guid.ReadAsPacked();
    recv_data >> mi;

    if(_player->GetMover()->GetObjectGuid() == old_mover_guid)
    {
        sLog.outError("HandleMoveNotActiveMover: incorrect mover guid: mover is %s and should be %s instead of %s",
            _player->GetMover()->GetObjectGuid().GetString().c_str(),
            _player->GetObjectGuid().GetString().c_str(),
            old_mover_guid.GetString().c_str());
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    _player->m_movementInfo = mi;
}

void WorldSession::HandleDismissControlledVehicle(WorldPacket &recv_data)
{
    DEBUG_LOG("WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    ObjectGuid guid;
    MovementInfo mi;

    recv_data >> guid.ReadAsPacked();
    recv_data >> mi;

    uint64 vehicleGUID = _player->GetCharmGUID();

    if(!vehicleGUID)                                        // something wrong here...
        return;

    _player->m_movementInfo = mi;

    // using charm guid, because we don't have vehicle guid...
    if(Vehicle *vehicle = _player->GetMap()->GetVehicle(vehicleGUID))
    {
        // Aura::HandleAuraControlVehicle will call Player::ExitVehicle
        vehicle->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
    }
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvdata*/)
{
    //DEBUG_LOG("WORLD: Recvd CMSG_MOUNTSPECIAL_ANIM");

    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << uint64(GetPlayer()->GetGUID());

    GetPlayer()->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveKnockBackAck( WorldPacket & recv_data )
{
    DEBUG_LOG("CMSG_MOVE_KNOCK_BACK_ACK");

    ObjectGuid guid;                                        // guid - unused
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // unk
    recv_data >> movementInfo;
}

void WorldSession::HandleMoveHoverAck( WorldPacket& recv_data )
{
    DEBUG_LOG("CMSG_MOVE_HOVER_ACK");

    ObjectGuid guid;                                        // guid - unused
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // unk1
    recv_data >> movementInfo;
    recv_data >> Unused<uint32>();                          // unk2
}

void WorldSession::HandleMoveWaterWalkAck(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_MOVE_WATER_WALK_ACK");

    ObjectGuid guid;                                        // guid - unused
    MovementInfo movementInfo;

    recv_data >> guid.ReadAsPacked();
    recv_data >> Unused<uint32>();                          // unk1
    recv_data >> movementInfo;
    recv_data >> Unused<uint32>();                          // unk2
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    if(!_player->isAlive() || _player->isInCombat() )
        return;

    uint64 summoner_guid;
    bool agree;
    recv_data >> summoner_guid;
    recv_data >> agree;

    _player->SummonIfPossible(agree);
}
