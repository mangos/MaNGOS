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

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Corpse.h"
#include "Player.h"
#include "MapManager.h"
#include "Transports.h"
#include "BattleGround.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"

/*Movement anticheat DEBUG defines */
//#define MOVEMENT_ANTICHEAT_DEBUG true
/*end Movement anticheate defines*/
void WorldSession::HandleMoveWorldportAckOpcode( WorldPacket & /*recv_data*/ )
{
    sLog.outDebug( "WORLD: got MSG_MOVE_WORLDPORT_ACK." );
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // get the teleport destination
    WorldLocation &loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check
    if(!MapManager::IsValidMapCoord(loc.mapid,loc.x,loc.y,loc.z,loc.o))
    {
        LogoutPlayer(false);
        return;
    }
    //reset falltimer at teleport
    GetPlayer()->m_anti_justteleported = 1;
    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.mapid);
    InstanceTemplate const* mInstance = objmgr.GetInstanceTemplate(loc.mapid);

    // reset instance validity, except if going to an instance inside an instance
    if(GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;

    GetPlayer()->SetSemaphoreTeleport(false);

    // relocate the player to the teleport destination
    GetPlayer()->SetMapId(loc.mapid);
    GetPlayer()->Relocate(loc.x, loc.y, loc.z, loc.o);

    // since the MapId is set before the GetInstance call, the InstanceId must be set to 0
    // to let GetInstance() determine the proper InstanceId based on the player's binds
    GetPlayer()->SetInstanceId(0);

    // check this before Map::Add(player), because that will create the instance save!
    bool reset_notify = (GetPlayer()->GetBoundInstance(GetPlayer()->GetMapId(), GetPlayer()->GetDifficulty()) == NULL);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if(!GetPlayer()->GetMap()->Add(GetPlayer()))
    {
        sLog.outDebug("WORLD: teleport of player %s (%d) to location %d,%f,%f,%f,%f failed", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.mapid, loc.x, loc.y, loc.z, loc.o);
        // teleport the player home
        GetPlayer()->SetDontMove(false);
        if(!GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation()))
        {
            // the player must always be able to teleport home
            sLog.outError("WORLD: failed to teleport player %s (%d) to homebind location %d,%f,%f,%f,%f!", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
            assert(false);
        }
        return;
    }
    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // flight fast teleport case
    if(GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType()==FLIGHT_MOTION_TYPE)
    {
        if(!_player->InBattleGround())
        {
            // short preparations to continue flight
            GetPlayer()->SetDontMove(false);
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Initialize(*GetPlayer());
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
            GetPlayer()->SaveToDB();
        }
    }

    if(mEntry->IsRaid() && mInstance)
    {
        if(reset_notify)
        {
            uint32 timeleft = sInstanceSaveManager.GetResetTimeFor(GetPlayer()->GetMapId()) - time(NULL);
            GetPlayer()->SendInstanceResetWarning(GetPlayer()->GetMapId(), timeleft); // greeting at the entrance of the resort raid instance
        }
    }

    // mount allow check
    if(!mEntry->IsMountAllowed())
        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

    // battleground state prepare
    // only add to bg group and object, if the player was invited (else he entered through command)
    if(_player->InBattleGround() && _player->IsInvitedForBattleGroundInstance(_player->GetBattleGroundId()))
    {
        BattleGround *bg = _player->GetBattleGround();
        if(bg)
        {
            bg->AddPlayer(_player);
            if(bg->GetMapId() == _player->GetMapId())       // we teleported to bg
            {
                // get the team this way, because arenas might 'override' the teams.
                uint32 team = bg->GetPlayerTeam(_player->GetGUID());
                if(!team)
                    team = _player->GetTeam();
                if(!bg->GetBgRaid(team))      // first player joined
                {
                    Group *group = new Group;
                    bg->SetBgRaid(team, group);
                    group->Create(_player->GetGUIDLow(), _player->GetName());
                }
                else                                        // raid already exist
                {
                    bg->GetBgRaid(team)->AddMember(_player->GetGUID(), _player->GetName());
                }
            }
        }
    }

    // honorless target
    if(GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);

    // resummon pet
    if(GetPlayer()->m_temporaryUnsummonedPetNumber)
    {
        Pet* NewPet = new Pet;
        if(!NewPet->LoadPetFromDB(GetPlayer(), 0, GetPlayer()->m_temporaryUnsummonedPetNumber, true))
            delete NewPet;

        GetPlayer()->m_temporaryUnsummonedPetNumber = 0;
    }

    GetPlayer()->SetDontMove(false);
}

void WorldSession::HandleMovementOpcodes( WorldPacket & recv_data )
{
    CHECK_PACKET_SIZE(recv_data, 4+1+4+4+4+4+4);

    if(GetPlayer()->GetDontMove()){
        GetPlayer()->m_anti_justteleported = 1;
        return;
    }

    //get opcode
    uint16 opcode = recv_data.GetOpcode();

    /* extract packet */
    MovementInfo movementInfo;
    uint32 MovementFlags;

    recv_data >> MovementFlags;
    recv_data >> movementInfo.unk1;
    recv_data >> movementInfo.time;
    recv_data >> movementInfo.x;
    recv_data >> movementInfo.y;
    recv_data >> movementInfo.z;
    recv_data >> movementInfo.o;

    //Save movement flags
    _player->SetUnitMovementFlags(MovementFlags);

    if(MovementFlags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8+4+4+4+4+4);

        recv_data >> movementInfo.t_guid;
        recv_data >> movementInfo.t_x;
        recv_data >> movementInfo.t_y;
        recv_data >> movementInfo.t_z;
        recv_data >> movementInfo.t_o;
        recv_data >> movementInfo.t_time;
    }

    if(MovementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING2))
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

        recv_data >> movementInfo.s_pitch;                  // pitch, -1.55=looking down, 0=looking straight forward, +1.55=looking up
    }

    // recheck
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

    recv_data >> movementInfo.fallTime;                     // duration of last jump (when in jump duration from jump begin to now)

    if(MovementFlags & MOVEMENTFLAG_JUMPING)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4+4+4+4);

        recv_data >> movementInfo.j_unk;                    // constant, but different when jumping in water and on land?
        recv_data >> movementInfo.j_sinAngle;               // sin of angle between orientation0 and players orientation
        recv_data >> movementInfo.j_cosAngle;               // cos of angle between orientation0 and players orientation
        recv_data >> movementInfo.j_xyspeed;                // speed of xy movement
    }

    if(MovementFlags & MOVEMENTFLAG_SPLINE)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

        recv_data >> movementInfo.u_unk1;                   // unknown
    }
    /*----------------*/

    if(recv_data.size() != recv_data.rpos())
    {
        sLog.outError("MovementHandler: player %s (guid %d, account %u) sent a packet (opcode %u) that is %u bytes larger than it should be. Kicked as cheater.", _player->GetName(), _player->GetGUIDLow(), _player->GetSession()->GetAccountId(), recv_data.GetOpcode(), recv_data.size() - recv_data.rpos());
        KickPlayer();
        return;
    }

    if (!MaNGOS::IsValidMapCoord(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o))
        return;

    /* handle special cases */
    if (MovementFlags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if( movementInfo.t_x > 60 || movementInfo.t_y > 60 || movementInfo.t_z > 60 )
            return;

        if( !MaNGOS::IsValidMapCoord(movementInfo.x+movementInfo.t_x, movementInfo.y+movementInfo.t_y,
            movementInfo.z+movementInfo.t_z, movementInfo.o+movementInfo.t_o) )
            return;
        if ((GetPlayer()->m_anti_transportGUID == 0) && (movementInfo.t_guid !=0))
        {
            // if we boarded a transport, add us to it
            if (!GetPlayer()->m_transport)
            {
                // elevators also cause the client to send MOVEMENTFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
                for (MapManager::TransportSet::iterator iter = MapManager::Instance().m_Transports.begin(); iter != MapManager::Instance().m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetGUID() == movementInfo.t_guid)
                    {
                        // unmount before boarding
                        _player->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

                        GetPlayer()->m_transport = (*iter);
                        (*iter)->AddPassenger(GetPlayer());
                        break;
                    }
                }
            }
            ///GetPlayer()->m_anti_transportGUID = GUID_LOPART(movementInfo.t_guid);
            //Correct finding GO guid in DB (thanks to GriffonHeart)
            GameObject *obj = HashMapHolder<GameObject>::Find(movementInfo.t_guid);
            if(obj)
                GetPlayer()->m_anti_transportGUID = obj->GetDBTableGUIDLow();
            else
                GetPlayer()->m_anti_transportGUID = GUID_LOPART(movementInfo.t_guid);
        }
    }
    else if (GetPlayer()->m_anti_transportGUID != 0)
    {
        if (GetPlayer()->m_transport)                      // if we were on a transport, leave
        {
            GetPlayer()->m_transport->RemovePassenger(GetPlayer());
            GetPlayer()->m_transport = NULL;
        }
        movementInfo.t_x = 0.0f;
        movementInfo.t_y = 0.0f;
        movementInfo.t_z = 0.0f;
        movementInfo.t_o = 0.0f;
        movementInfo.t_time = 0;
        GetPlayer()->m_anti_transportGUID = 0;
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (recv_data.GetOpcode() == MSG_MOVE_FALL_LAND && !GetPlayer()->isInFlight())
    {
        // calculate total z distance of the fall
        float z_diff = GetPlayer()->m_lastFallZ - movementInfo.z;
        sLog.outDebug("zDiff = %f", z_diff);
        Player *target = GetPlayer();

        /* //movement anticheat "No Fall Damage"
        if (target->m_anti_beginfalltime != 0)
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: alternate FallTime %d | client falltime %d | m_anti_justteleported = %d",movementInfo.time - GetPlayer()->m_anti_beginfalltime, movementInfo.fallTime, GetPlayer()->m_anti_justteleported );
            #endif
            if (target->m_anti_justteleported != 1){
                uint32 alt_falltime = movementInfo.time - target->m_anti_beginfalltime;
                movementInfo.fallTime = (alt_falltime < movementInfo.fallTime) ? movementInfo.fallTime : alt_falltime;
            } else {
                target->m_anti_justteleported = 0;
            }
            target->m_anti_beginfalltime = 0;
        }
        #ifdef MOVEMENT_ANTICHEAT_DEBUG
        sLog.outError("Movement anticheat: FallTime %d", movementInfo.fallTime);
        #endif
        //end movement anticheate */

        //Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
        // 14.57 can be calculated by resolving damageperc formular below to 0
        if (z_diff >= 14.57f && !target->isDead() && !target->isGameMaster() &&
            !target->HasAuraType(SPELL_AURA_HOVER) && !target->HasAuraType(SPELL_AURA_FEATHER_FALL) &&
            !target->HasAuraType(SPELL_AURA_FLY) && !target->IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL,true) )
        {
            //Safe fall, fall height reduction
            int32 safe_fall = target->GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

            float damageperc = 0.018f*(z_diff-safe_fall)-0.2426f;

            if(damageperc >0 )
            {
                uint32 damage = (uint32)(damageperc * target->GetMaxHealth()*sWorld.getRate(RATE_DAMAGE_FALL));

                float height = movementInfo.z;
                target->UpdateGroundPositionZ(movementInfo.x,movementInfo.y,height);

                if (damage > 0)
                {
                    //Prevent fall damage from being more than the player maximum health
                    if (damage > target->GetMaxHealth())
                        damage = target->GetMaxHealth();

                    // Gust of Wind
                    if (target->GetDummyAura(43621))
                        damage = target->GetMaxHealth()/2;

                    target->EnvironmentalDamage(target->GetGUID(), DAMAGE_FALL, damage);
                }

                //Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
                DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d" , movementInfo.z, height, target->GetPositionZ(), movementInfo.fallTime, height, damage, safe_fall);
            }
        }

        //handle fall and logout at the same time (logout started before fall finished)
        /* outdated and create problems with sit at stun sometime
        if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_ROTATE))
        {
            target->SetStandState(PLAYER_STATE_SIT);
            // Can't move
            WorldPacket data( SMSG_FORCE_MOVE_ROOT, 12 );
            data.append(target->GetPackGUID());
            data << (uint32)2;
            SendPacket( &data );
        }
        */
    }

    if(((MovementFlags & MOVEMENTFLAG_SWIMMING) != 0) != GetPlayer()->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        GetPlayer()->SetInWater( !GetPlayer()->IsInWater() || GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.x, movementInfo.y, movementInfo.z) );
    }

    #ifdef MOVEMENT_ANTICHEAT_DEBUG
    sLog.outBasic("%s newcoord: tm:%d ftm:%d | %f,%f,%fo(%f) [%X][%s]| transport: %f,%f,%fo(%f)",GetPlayer()->GetName(),movementInfo.time,movementInfo.fallTime,movementInfo.x,movementInfo.y,movementInfo.z,movementInfo.o,MovementFlags, LookupOpcodeName(opcode),movementInfo.t_x,movementInfo.t_y,movementInfo.t_z,movementInfo.t_o);
    sLog.outBasic("Transport: %d |  tguid: %d - %d", GetPlayer()->m_anti_transportGUID, GUID_LOPART(movementInfo.t_guid), GUID_HIPART(movementInfo.t_guid));
    #endif
    /*----------------------*/
    //---- anti-cheat features -->>>
    bool check_passed = true;
    uint32 curDest = GetPlayer()->m_taxi.GetTaxiDestination(); //check taxi flight
    if ((GetPlayer()->m_anti_transportGUID == 0) && World::GetEnableMvAnticheat() && !curDest)
    {
        UnitMoveType move_type;

        if (MovementFlags & MOVEMENTFLAG_FLYING) move_type = MovementFlags & MOVEMENTFLAG_BACKWARD ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
        else if (MovementFlags & MOVEMENTFLAG_SWIMMING) move_type = MovementFlags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_SWIM;
        else if (MovementFlags & MOVEMENTFLAG_WALK_MODE) move_type = MOVE_WALK;
        //hmm... in first time after login player has MOVE_SWIMBACK instead MOVE_WALKBACK
        else move_type = MovementFlags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_RUN;

        float allowed_delta= 0;
        float current_speed = GetPlayer()->GetSpeed(move_type);
        float delta_x = GetPlayer()->GetPositionX() - movementInfo.x;
        float delta_y = GetPlayer()->GetPositionY() - movementInfo.y;
        float delta_z = GetPlayer()->GetPositionZ() - movementInfo.z;
        float real_delta = delta_x * delta_x + delta_y * delta_y;
        float tg_z = -99999; //tangens
        float time_delta = movementInfo.time - GetPlayer()->m_anti_lastmovetime;
        if (time_delta > 0) {
            GetPlayer()->m_anti_lastmovetime = movementInfo.time;
        } else {time_delta = 0;}
        time_delta = (time_delta < 1500) ? time_delta/1000 : 1.5f; //normalize time - 1.5 second allowed for heavy loaded server

        if (!(MovementFlags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)))
          tg_z = (real_delta !=0) ? (delta_z*delta_z / real_delta) : -99999;

        /* //antiOFF fall-damage, MOVEMENTFLAG_UNK4 seted by client if player try movement when falling and unset in this case the MOVEMENTFLAG_FALLING flag.
        if (MovementFlags & (MOVEMENTFLAG_FALLING | MOVEMENTFLAG_UNK4 | MOVEMENTFLAG_JUMPING))
        {
            if (GetPlayer()->m_anti_beginfalltime == 0)
            {
                GetPlayer()->m_anti_beginfalltime = movementInfo.time;
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outDebug("Movement anticheat: begin fall-time %d",GetPlayer()->m_anti_beginfalltime);
                #endif
            }
        } else {
            if (GetPlayer()->m_anti_beginfalltime != 0)
            {
                GetPlayer()->m_anti_beginfalltime = 0; // reset timer if we landed without MSG_MOVE_FALL_LAND opcode
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outDebug("Movement anticheat: end fall-time %d",GetPlayer()->m_anti_beginfalltime);
                #endif
            }
        }
        */

        if (current_speed < GetPlayer()->m_anti_last_hspeed)
        {
            allowed_delta = GetPlayer()->m_anti_last_hspeed;
            if (GetPlayer()->m_anti_lastspeed_changetime == 0 )
                GetPlayer()->m_anti_lastspeed_changetime = movementInfo.time + (uint32)floor(((GetPlayer()->m_anti_last_hspeed / current_speed) * 1000)) + 100; //100ms above for random fluctuating =)))
        } else allowed_delta = current_speed;
        allowed_delta = allowed_delta * time_delta;
        allowed_delta = allowed_delta * allowed_delta + 2;

       // static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "Walkback", "Swim", "Swimback", "Turn", "Fly", "Flyback" };
       // sLog.outBasic("%s newcoord: tm:%d ftm:%d | %f,%f,%fo(%f) [%X][%s]$%s",GetPlayer()->GetName(),movementInfo.time,movementInfo.fallTime,movementInfo.x,movementInfo.y,movementInfo.z,movementInfo.o,MovementFlags, LookupOpcodeName(opcode),move_type_name[move_type]);
       // sLog.outBasic("%f",tg_z);

        if ((real_delta > allowed_delta)) //&& (delta_z < 1)
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: %s is speed exception. {real_delta=%f allowed_delta=%f | current_speed=%f preview_speed=%f time=%f}(%f %f %f %d)[%s]",GetPlayer()->GetName(),real_delta, allowed_delta, current_speed, GetPlayer()->m_anti_last_hspeed,time_delta,GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ(), GetPlayer()->GetMapId(),LookupOpcodeName(opcode));
            #endif
            check_passed = false;
        }
        if ((real_delta>4900.0f) && !(real_delta < allowed_delta))
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: %s is teleport exception. {real_delta=%f allowed_delta=%f | current_speed=%f preview_speed=%f time=%f}(%f %f %f %d)",GetPlayer()->GetName(),real_delta, allowed_delta, current_speed, GetPlayer()->m_anti_last_hspeed,time_delta,GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ(), GetPlayer()->GetMapId());
            #endif
            check_passed = false;
        }
        if (movementInfo.time>GetPlayer()->m_anti_lastspeed_changetime)
        {
            GetPlayer()->m_anti_last_hspeed = current_speed; // store current speed
            GetPlayer()->m_anti_last_vspeed = -3.2f;
            if (GetPlayer()->m_anti_lastspeed_changetime != 0) GetPlayer()->m_anti_lastspeed_changetime = 0;
        }

        if ((tg_z > 1.56f) && (delta_z < GetPlayer()->m_anti_last_vspeed))
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: %s is mountain exception. {tg_z=%f} (%f %f %f %d)",GetPlayer()->GetName(),tg_z, GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ(), GetPlayer()->GetMapId());
            #endif
            check_passed = false;
        }
        if (((MovementFlags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2)) != 0) && !GetPlayer()->isGameMaster() && !(GetPlayer()->HasAuraType(SPELL_AURA_FLY) || GetPlayer()->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)))
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: %s is fly cheater. {SPELL_AURA_FLY=[%X]} {SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED=[%X]} {SPELL_AURA_MOD_SPEED_FLIGHT=[%X]} {SPELL_AURA_MOD_FLIGHT_SPEED_ALWAYS=[%X]} {SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK=[%X]}",
               GetPlayer()->GetName(),
               GetPlayer()->HasAuraType(SPELL_AURA_FLY), GetPlayer()->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED),
               GetPlayer()->HasAuraType(SPELL_AURA_MOD_SPEED_FLIGHT), GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_ALWAYS),
               GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK));
            #endif
            check_passed = false;
        }
        if (((MovementFlags & MOVEMENTFLAG_WATERWALKING) != 0) && !GetPlayer()->isGameMaster() && !(GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK) | GetPlayer()->HasAuraType(SPELL_AURA_GHOST)))
        {
            #ifdef MOVEMENT_ANTICHEAT_DEBUG
            sLog.outDebug("Movement anticheat: %s is water-walk exception. [%X]{SPELL_AURA_WATER_WALK=[%X]}", GetPlayer()->GetName(), MovementFlags, GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK));
            #endif
            check_passed = false;
        }
        if(movementInfo.z < 0.0001f && movementInfo.z > -0.0001f && ((MovementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2)) == 0) && !GetPlayer()->isGameMaster() )
        {
            // Prevent using TeleportToPlan.
            Map *map = GetPlayer()->GetMap();
            if (map){
                float plane_z = map->GetHeight(movementInfo.x, movementInfo.y, MAX_HEIGHT) - movementInfo.z;
                plane_z = (plane_z < -500.0f) ? 0 : plane_z; //check holes in heigth map
                if(plane_z > 0.1f || plane_z < -0.1f)
                {
                    GetPlayer()->m_anti_teletoplane_count++;
                    check_passed = false;
                    #ifdef MOVEMENT_ANTICHEAT_DEBUG
                    sLog.outDebug("Movement anticheat: %s is teleport to plan exception. plane_z: %f ", GetPlayer()->GetName(), plane_z);
                    #endif
                    if (GetPlayer()->m_anti_teletoplane_count > World::GetTeleportToPlaneAlarms())
                    {
                        GetPlayer()->GetSession()->KickPlayer();
                        sLog.outError("Movement anticheat: %s is teleport to plan exception. Exception count: %d ", GetPlayer()->GetName(), GetPlayer()->m_anti_teletoplane_count);
                    }
                }
            }
        } else {
            if (GetPlayer()->m_anti_teletoplane_count !=0)
                GetPlayer()->m_anti_teletoplane_count = 0;
        }
    } else if (MovementFlags & MOVEMENTFLAG_ONTRANSPORT) {
            //antiwrap =)
        if (GetPlayer()->m_transport)
        {
            float trans_rad = movementInfo.t_x*movementInfo.t_x + movementInfo.t_y*movementInfo.t_y + movementInfo.t_z*movementInfo.t_z;
            if (trans_rad > 3600.0f)
                check_passed = false;
        } else {
            if (GameObjectData const* go_data = objmgr.GetGOData(GetPlayer()->m_anti_transportGUID))
            {
                float delta_gox = go_data->posX - movementInfo.x;
                float delta_goy = go_data->posY - movementInfo.y;
                float delta_goz = go_data->posZ - movementInfo.z;
                int mapid = go_data->mapid;
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outDebug("Movement anticheat: %s on some transport. xyzo: %f,%f,%f", GetPlayer()->GetName(), go_data->posX,go_data->posY,go_data->posZ);
                #endif
                if (GetPlayer()->GetMapId() != mapid){
                    check_passed = false;
                } else if (mapid !=369) {
                    float delta_go = delta_gox*delta_gox + delta_goy*delta_goy;
                    if (delta_go > 3600.0f)
                        check_passed = false;
                }

            } else {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outDebug("Movement anticheat: %s on undefined transport.", GetPlayer()->GetName());
                #endif
                check_passed = false;
            }
        }
        if (!check_passed){
            if (GetPlayer()->m_transport)
                {
                    GetPlayer()->m_transport->RemovePassenger(GetPlayer());
                    GetPlayer()->m_transport = NULL;
                }
                movementInfo.t_x = 0.0f;
                movementInfo.t_y = 0.0f;
                movementInfo.t_z = 0.0f;
                movementInfo.t_o = 0.0f;
                movementInfo.t_time = 0;
                GetPlayer()->m_anti_transportGUID = 0;
        }
    }
    /* process position-change */
    if (check_passed)
    {
        recv_data.put<uint32>(5, getMSTime());                  // offset flags(4) + unk(1)
        WorldPacket data(recv_data.GetOpcode(), (GetPlayer()->GetPackGUID().size()+recv_data.size()));
        data.append(GetPlayer()->GetPackGUID());
        data.append(recv_data.contents(), recv_data.size());
        GetPlayer()->SendMessageToSet(&data, false);

    GetPlayer()->SetPosition(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
    GetPlayer()->m_movementInfo = movementInfo;
    if (GetPlayer()->m_lastFallTime >= movementInfo.fallTime || GetPlayer()->m_lastFallZ <=movementInfo.z || recv_data.GetOpcode() == MSG_MOVE_FALL_LAND)
        GetPlayer()->SetFallInformation(movementInfo.fallTime, movementInfo.z);

        if(GetPlayer()->isMovingOrTurning())
            GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

    if(movementInfo.z < -500.0f)
    {
        if(GetPlayer()->InBattleGround()
            && GetPlayer()->GetBattleGround()
            && GetPlayer()->GetBattleGround()->HandlePlayerUnderMap(_player))
        {
            // do nothing, the handle already did if returned true
        }
        else
        {
            // NOTE: this is actually called many times while falling
            // even after the player has been teleported away
            // TODO: discard movement packets after the player is rooted
            if(GetPlayer()->isAlive())
            {
                GetPlayer()->EnvironmentalDamage(GetPlayer()->GetGUID(),DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
                // change the death state to CORPSE to prevent the death timer from
                // starting in the next player update
                GetPlayer()->KillPlayer();
                GetPlayer()->BuildPlayerRepop();
            }

            // cancel the death timer here if started
            GetPlayer()->RepopAtGraveyard();
        }
    }

        if (GetPlayer()->m_anti_alarmcount > 0){
            sLog.outError("Movement anticheat: %s produce %d anticheat alarms",GetPlayer()->GetName(),GetPlayer()->m_anti_alarmcount);
            GetPlayer()->m_anti_alarmcount = 0;
        }
    } else {
        GetPlayer()->m_anti_alarmcount++;
        WorldPacket data;
        GetPlayer()->SetUnitMovementFlags(0);
        GetPlayer()->BuildTeleportAckMsg(&data, GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY(), GetPlayer()->GetPositionZ(), GetPlayer()->GetOrientation());
        GetPlayer()->GetSession()->SendPacket(&data);
        GetPlayer()->BuildHeartBeatMsg(&data);
        GetPlayer()->SendMessageToSet(&data, true);
    }
}

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recv_data)
{
    CHECK_PACKET_SIZE(recv_data, 8+4+4+1+4+4+4+4+4);

    /* extract packet */
    uint64 guid;
    uint8  unkB;
    uint32 unk1, flags, time, fallTime;
    float x, y, z, orientation;

    uint64 t_GUID;
    float  t_x, t_y, t_z, t_o;
    uint32 t_time;
    float  s_pitch;
    float  j_unk1, j_sinAngle, j_cosAngle, j_xyspeed;
    float  u_unk1;
    float  newspeed;

    recv_data >> guid;

    // now can skip not our packet
    if(_player->GetGUID() != guid)
        return;

    // continue parse packet

    recv_data >> unk1;
    recv_data >> flags >> unkB >> time;
    recv_data >> x >> y >> z >> orientation;
    if (flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8+4+4+4+4+4);

        recv_data >> t_GUID;
        recv_data >> t_x >> t_y >> t_z >> t_o >> t_time;
    }
    if (flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING2))
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

        recv_data >> s_pitch;                               // pitch, -1.55=looking down, 0=looking straight forward, +1.55=looking up
    }

    // recheck
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

    recv_data >> fallTime;                                  // duration of last jump (when in jump duration from jump begin to now)

    if ((flags & MOVEMENTFLAG_JUMPING) || (flags & MOVEMENTFLAG_FALLING))
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4+4+4+4);

        recv_data >> j_unk1;                                // ?constant, but different when jumping in water and on land?
        recv_data >> j_sinAngle >> j_cosAngle;              // sin + cos of angle between orientation0 and players orientation
        recv_data >> j_xyspeed;                             // speed of xy movement
    }

    if(flags & MOVEMENTFLAG_SPLINE)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

        recv_data >> u_unk1;                                // unknown
    }

    // recheck
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

    recv_data >> newspeed;
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack" };

    uint16 opcode = recv_data.GetOpcode();
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
            _player->SetSpeed(move_type,_player->GetSpeedRate(move_type),true);
        }
        else                                                // must be lesser - cheating
        {
            sLog.outBasic("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(),_player->GetSession()->GetAccountId(),_player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_SET_ACTIVE_MOVER");

    CHECK_PACKET_SIZE(recv_data,8);

    uint64 guid;
    recv_data >> guid;

    WorldPacket data(SMSG_TIME_SYNC_REQ, 4);                // new 2.0.x, enable movement
    data << uint32(0x00000000);                             // on blizz it increments periodically
    SendPacket(&data);
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvdata*/)
{
    //sLog.outDebug("WORLD: Recvd CMSG_MOUNTSPECIAL_ANIM");

    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << uint64(GetPlayer()->GetGUID());

    GetPlayer()->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveKnockBackAck( WorldPacket & recv_data )
{
    // CHECK_PACKET_SIZE(recv_data,?);
    sLog.outDebug("CMSG_MOVE_KNOCK_BACK_ACK");
    // Currently not used but maybe use later for recheck final player position
    // (must be at call same as into "recv_data >> x >> y >> z >> orientation;"

    CHECK_PACKET_SIZE(recv_data, 8+4+4+1+4+4+4+4+4);
    MovementInfo movementInfo;

    uint64 guid;
    uint32 sequence;
    uint32 MovementFlags;
    uint32 ukn1;
    float xdirection,ydirection,hspeed,vspeed;

    recv_data >> guid;          //8
    recv_data >> sequence;      //4
    recv_data >> MovementFlags >> movementInfo.unk1 >> movementInfo.time; //4+1+4
    recv_data >> movementInfo.x >> movementInfo.y >> movementInfo.z >> movementInfo.o; //4+4+4+4

    //Save movement flags
    _player->SetUnitMovementFlags(MovementFlags);

    if(MovementFlags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // recheck
        CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8+4+4+4+4+4);

        recv_data >> movementInfo.t_guid;
        recv_data >> movementInfo.t_x;
        recv_data >> movementInfo.t_y;
        recv_data >> movementInfo.t_z;
        recv_data >> movementInfo.t_o;
        recv_data >> movementInfo.t_time;
    }

    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4+4+4+4+4);
    recv_data >> ukn1; //unknown //4
    recv_data >> vspeed >> xdirection >> ydirection >> hspeed; //4+4+4+4

    #ifdef MOVEMENT_ANTICHEAT_DEBUG
    sLog.outDebug("%s CMSG_MOVE_KNOCK_BACK_ACK: tm:%d ftm:%d | %f,%f,%fo(%f) [%X]",GetPlayer()->GetName(),movementInfo.time,movementInfo.fallTime,movementInfo.x,movementInfo.y,movementInfo.z,movementInfo.o,MovementFlags);
    sLog.outDebug("%s CMSG_MOVE_KNOCK_BACK_ACK additional: vspeed:%f, hspeed:%f, xdir:%f ydir:%f",GetPlayer()->GetName(), vspeed, hspeed, xdirection, ydirection);
    #endif

    // skip not personal message;
    if(GetPlayer()->GetGUID()!=guid)
        return;

    GetPlayer()->m_movementInfo = movementInfo;
    GetPlayer()->m_anti_last_hspeed = hspeed;
    GetPlayer()->m_anti_last_vspeed = vspeed < 3.2f ? vspeed - 1.0f : 3.2f;
    GetPlayer()->m_anti_lastspeed_changetime = movementInfo.time + 1750;
}

void WorldSession::HandleMoveHoverAck( WorldPacket& /*recv_data*/ )
{
    sLog.outDebug("CMSG_MOVE_HOVER_ACK");
}

void WorldSession::HandleMoveWaterWalkAck(WorldPacket& /*recv_data*/)
{
    sLog.outDebug("CMSG_MOVE_WATER_WALK_ACK");
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    CHECK_PACKET_SIZE(recv_data,8+1);

    if(!_player->isAlive() || _player->isInCombat() )
        return;

    uint64 summoner_guid;
    bool agree;
    recv_data >> summoner_guid;
    recv_data >> agree;

    _player->SummonIfPossible(agree);
}
