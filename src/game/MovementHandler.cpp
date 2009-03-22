/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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
#include "MapManager.h"
#include "Transports.h"
#include "BattleGround.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "World.h"

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
    //movement anticheat
    GetPlayer()->m_anti_JustTeleported = 1;
    //<<< movement anticheat

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

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if(_player->InBattleGround())
    {
        // cleanup seting if outdated
        if(!mEntry->IsBattleGroundOrArena())
        {
            _player->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE); // We're not in BG.
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
    uint32 opcode = recv_data.GetOpcode();
    sLog.outDebug("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);

    if(GetPlayer()->GetDontMove()){
        // movement anticheat
        GetPlayer()->m_anti_JustTeleported = 1;
        // <<< movement anticheat
        return;
    }

    /* extract packet */
    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);
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
    if (movementInfo.flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if( movementInfo.t_x > 60 || movementInfo.t_y > 60 || movementInfo.t_x < -60 ||  movementInfo.t_y < -60 )
            return;

        if( !MaNGOS::IsValidMapCoord(movementInfo.x+movementInfo.t_x, movementInfo.y+movementInfo.t_y,
            movementInfo.z+movementInfo.t_z, movementInfo.o+movementInfo.t_o) )
            return;
        if ((GetPlayer()->m_anti_TransportGUID == 0) && (movementInfo.t_guid !=0))
        {
            // if we boarded a transport, add us to it
            if (!GetPlayer()->m_transport)
            {
                // elevators also cause the client to send MOVEMENTFLAG_ONTRANSPORT - just unmount if the guid can be found in the transport list
                for (MapManager::TransportSet::iterator iter = MapManager::Instance().m_Transports.begin(); iter != MapManager::Instance().m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetGUID() == movementInfo.t_guid)
                    {
                        GetPlayer()->m_transport = (*iter);
                        (*iter)->AddPassenger(GetPlayer());
                        break;
                    }
                }
            }
            //movement anticheat;
            //Correct finding GO guid in DB (thanks to GriffonHeart)
            GameObject *obj = HashMapHolder<GameObject>::Find(movementInfo.t_guid);
            if(obj)
                GetPlayer()->m_anti_TransportGUID = obj->GetDBTableGUIDLow();
            else
                GetPlayer()->m_anti_TransportGUID = GUID_LOPART(movementInfo.t_guid);
            // <<< movement anticheat
        }
    }
    else if (GetPlayer()->m_anti_TransportGUID != 0)
    {
        if (GetPlayer()->m_transport)    // if we were on a transport, leave it
        {
            GetPlayer()->m_transport->RemovePassenger(GetPlayer());
            GetPlayer()->m_transport = NULL;
        }
        movementInfo.t_x = 0.0f;
        movementInfo.t_y = 0.0f;
        movementInfo.t_z = 0.0f;
        movementInfo.t_o = 0.0f;
        movementInfo.t_time = 0;
        movementInfo.t_seat = -1;
        GetPlayer()->m_anti_TransportGUID = 0;
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == MSG_MOVE_FALL_LAND && !GetPlayer()->isInFlight())
    {
        //movement anticheat
        GetPlayer()->m_anti_JustJumped = 0;
        GetPlayer()->m_anti_JumpBaseZ = 0;
        //<<< movement anticheat
        GetPlayer()->HandleFall(movementInfo);
    }

    if(((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != GetPlayer()->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        GetPlayer()->SetInWater( !GetPlayer()->IsInWater() || GetPlayer()->GetBaseMap()->IsUnderWater(movementInfo.x, movementInfo.y, movementInfo.z) );
    }

    /*----------------------*/
    //---- anti-cheat features -->>>
    bool check_passed = true;
    #ifdef MOVEMENT_ANTICHEAT_DEBUG
    sLog.outBasic("MA-%s > client-time:%d fall-time:%d | xyzo: %f,%f,%fo(%f) flags[%X] opcode[%s]| transport (xyzo): %f,%f,%fo(%f)",
                    GetPlayer()->GetName(),movementInfo.time,movementInfo.fallTime,movementInfo.x,movementInfo.y,movementInfo.z,movementInfo.o,
                    movementInfo.flags, LookupOpcodeName(opcode),movementInfo.t_x,movementInfo.t_y,movementInfo.t_z,movementInfo.t_o);
    sLog.outBasic("MA-%s Transport > server GUID: %d |  client GUID: (lo)%d - (hi)%d",
                    GetPlayer()->GetName(),GetPlayer()->m_anti_TransportGUID, GUID_LOPART(movementInfo.t_guid), GUID_HIPART(movementInfo.t_guid));
    #endif

    if (World::GetEnableMvAnticheat())
    {
        //calc time deltas
        int32 cClientTimeDelta = 1500;
        if (GetPlayer()->m_anti_LastClientTime !=0){
            cClientTimeDelta = movementInfo.time - GetPlayer()->m_anti_LastClientTime;
            GetPlayer()->m_anti_DeltaClientTime += cClientTimeDelta;
            GetPlayer()->m_anti_LastClientTime = movementInfo.time;
        } else {
            GetPlayer()->m_anti_LastClientTime = movementInfo.time;
        }

        uint32 cServerTime=getMSTime();
        uint32 cServerTimeDelta = 1500;
        if (GetPlayer()->m_anti_LastServerTime != 0){
            cServerTimeDelta = cServerTime - GetPlayer()->m_anti_LastServerTime;
            GetPlayer()->m_anti_DeltaServerTime += cServerTimeDelta;
            GetPlayer()->m_anti_LastServerTime = cServerTime;
        } else {
            GetPlayer()->m_anti_LastServerTime = cServerTime;
        }

        //resync times on client login (first 15 sec for heavy areas)
        if (GetPlayer()->m_anti_DeltaServerTime < 15000 && GetPlayer()->m_anti_DeltaClientTime < 15000)
            GetPlayer()->m_anti_DeltaClientTime = GetPlayer()->m_anti_DeltaServerTime;

        int32 sync_time = GetPlayer()->m_anti_DeltaClientTime - GetPlayer()->m_anti_DeltaServerTime;

        #ifdef MOVEMENT_ANTICHEAT_DEBUG
        sLog.outBasic("MA-%s Time > cClientTimeDelta: %d, cServerTime: %d || deltaC: %d - deltaS: %d || SyncTime: %d",
                        GetPlayer()->GetName(),cClientTimeDelta, cServerTime,
                        GetPlayer()->m_anti_DeltaClientTime, GetPlayer()->m_anti_DeltaServerTime, sync_time);
        #endif

        //mistiming checks
        int32 gmd = World::GetMistimingDelta();
        if (sync_time > gmd || sync_time < -gmd){
            cClientTimeDelta = cServerTimeDelta;
            GetPlayer()->m_anti_MistimingCount++;

            sLog.outError("MA-%s, mistaming exception. #:%d, mistiming: %dms ",
                            GetPlayer()->GetName(), GetPlayer()->m_anti_MistimingCount, sync_time);

            if (GetPlayer()->m_anti_MistimingCount > World::GetMistimingAlarms())
            {
                GetPlayer()->GetSession()->KickPlayer();
                return;
            }
            check_passed = false;
        }
        // <<< mistiming checks


        uint32 curDest = GetPlayer()->m_taxi.GetTaxiDestination(); //check taxi flight
        if ((GetPlayer()->m_anti_TransportGUID == 0) && !curDest)
        {
            UnitMoveType move_type;

            // calculating section ---------------------
            //current speed
            if (movementInfo.flags & MOVEMENTFLAG_FLYING) move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_FLIGHT_BACK : MOVE_FLIGHT;
            else if (movementInfo.flags & MOVEMENTFLAG_SWIMMING) move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_SWIM;
            else if (movementInfo.flags & MOVEMENTFLAG_WALK_MODE) move_type = MOVE_WALK;
            //hmm... in first time after login player has MOVE_SWIMBACK instead MOVE_WALKBACK
            else move_type = movementInfo.flags & MOVEMENTFLAG_BACKWARD ? MOVE_SWIM_BACK : MOVE_RUN;

            float current_speed = GetPlayer()->GetSpeed(move_type);
            // <<< current speed

            // movement distance
            float allowed_delta= 0;

            float delta_x = GetPlayer()->GetPositionX() - movementInfo.x;
            float delta_y = GetPlayer()->GetPositionY() - movementInfo.y;
            float delta_z = GetPlayer()->GetPositionZ() - movementInfo.z;
            float real_delta = delta_x * delta_x + delta_y * delta_y;
            float tg_z = -99999; //tangens
            // <<< movement distance

            if (cClientTimeDelta < 0) {cClientTimeDelta = 0;}
            float time_delta = (cClientTimeDelta < 1500) ? (float)cClientTimeDelta/1000 : 1.5f; //normalize time - 1.5 second allowed for heavy loaded server

            if (!(movementInfo.flags & (MOVEMENTFLAG_FLYING | MOVEMENTFLAG_SWIMMING)))
              tg_z = (real_delta !=0) ? (delta_z*delta_z / real_delta) : -99999;

            if (current_speed < GetPlayer()->m_anti_Last_HSpeed)
            {
                allowed_delta = GetPlayer()->m_anti_Last_HSpeed;
                if (GetPlayer()->m_anti_LastSpeedChangeTime == 0 )
                    GetPlayer()->m_anti_LastSpeedChangeTime = movementInfo.time + (uint32)floor(((GetPlayer()->m_anti_Last_HSpeed / current_speed) * 1500)) + 100; //100ms above for random fluctuating =)))
            } else {
                allowed_delta = current_speed;
            }
            allowed_delta = allowed_delta * time_delta;
            allowed_delta = allowed_delta * allowed_delta + 2;
            if (tg_z > 2.2)
                allowed_delta = allowed_delta + (delta_z*delta_z)/2.37; // mountain fall allowed speed

            if (movementInfo.time>GetPlayer()->m_anti_LastSpeedChangeTime)
            {
                GetPlayer()->m_anti_Last_HSpeed = current_speed; // store current speed
                GetPlayer()->m_anti_Last_VSpeed = -2.3f;
                if (GetPlayer()->m_anti_LastSpeedChangeTime != 0) GetPlayer()->m_anti_LastSpeedChangeTime = 0;
            }
            // <<< end calculating section ---------------------

            //AntiGravitation (thanks to Meekro)
            float JumpHeight = GetPlayer()->m_anti_JumpBaseZ - movementInfo.z;
            if ((GetPlayer()->m_anti_JumpBaseZ != 0)
                    && !(movementInfo.flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2))
                    && (JumpHeight < GetPlayer()->m_anti_Last_VSpeed))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, GraviJump exception. JumpHeight = %f, Allowed Veritcal Speed = %f",
                                GetPlayer()->GetName(), JumpHeight, GetPlayer()->m_anti_Last_VSpeed);
                #endif
                check_passed = false;
            }

            //multi jump checks
            if (opcode == MSG_MOVE_JUMP && !GetPlayer()->IsInWater())
            {
                if (GetPlayer()->m_anti_JustJumped >= 1){
                    check_passed = false; //don't process new jump packet
                } else {
                    GetPlayer()->m_anti_JustJumped += 1;
                    GetPlayer()->m_anti_JumpBaseZ = movementInfo.z;
                }
            } else if (GetPlayer()->IsInWater()) {
                 GetPlayer()->m_anti_JustJumped = 0;
            }

            //speed hack checks
            if ((real_delta > allowed_delta)) // && (delta_z < 0))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, speed exception | cDelta=%f aDelta=%f | cSpeed=%f lSpeed=%f deltaTime=%f",
                                GetPlayer()->GetName(), real_delta, allowed_delta, current_speed, GetPlayer()->m_anti_Last_HSpeed,time_delta);
                #endif
                check_passed = false;
            }
            //teleport hack checks
            if ((real_delta>4900.0f) && !(real_delta < allowed_delta))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, is teleport exception | cDelta=%f aDelta=%f | cSpeed=%f lSpeed=%f deltaToime=%f",
                                GetPlayer()->GetName(),real_delta, allowed_delta, current_speed, GetPlayer()->m_anti_Last_HSpeed,time_delta);
                #endif
                check_passed = false;
            }

            //mountian hack checks // 1.56f (delta_z < GetPlayer()->m_anti_Last_VSpeed))
            if ((delta_z < 0) && (GetPlayer()->m_anti_JustJumped == 0) && (tg_z > 2.37f))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, mountain exception | tg_z=%f", GetPlayer()->GetName(),tg_z);
                #endif
                check_passed = false;
            }
            //Fly hack checks
            if (((movementInfo.flags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2)) != 0)
                  && !GetPlayer()->isGameMaster()
                  && !(GetPlayer()->HasAuraType(SPELL_AURA_FLY) || GetPlayer()->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, flight exception. {SPELL_AURA_FLY=[%X]} {SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED=[%X]} {SPELL_AURA_MOD_SPEED_FLIGHT=[%X]} {SPELL_AURA_MOD_FLIGHT_SPEED_ALWAYS=[%X]} {SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK=[%X]}",
                   GetPlayer()->GetName(),
                   GetPlayer()->HasAuraType(SPELL_AURA_FLY), GetPlayer()->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED),
                   GetPlayer()->HasAuraType(SPELL_AURA_MOD_SPEED_FLIGHT), GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_ALWAYS),
                   GetPlayer()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK));
                #endif
                check_passed = false;
            }
            //Water-Walk checks
            if (((movementInfo.flags & MOVEMENTFLAG_WATERWALKING) != 0)
                  && !GetPlayer()->isGameMaster()
                  && !(GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK) | GetPlayer()->HasAuraType(SPELL_AURA_GHOST)))
            {
                #ifdef MOVEMENT_ANTICHEAT_DEBUG
                sLog.outError("MA-%s, water-walk exception. [%X]{SPELL_AURA_WATER_WALK=[%X]}",
                                GetPlayer()->GetName(), movementInfo.flags, GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK));
                #endif
                check_passed = false;
            }
            //Teleport To Plane checks
            if (movementInfo.z < 0.0001f && movementInfo.z > -0.0001f
                && ((movementInfo.flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_FLYING2)) == 0)
                && !GetPlayer()->isGameMaster())
            {
                // Prevent using TeleportToPlan.
                Map *map = GetPlayer()->GetMap();
                if (map){
                    float plane_z = map->GetHeight(movementInfo.x, movementInfo.y, MAX_HEIGHT) - movementInfo.z;
                    plane_z = (plane_z < -500.0f) ? 0 : plane_z; //check holes in heigth map
                    if(plane_z > 0.1f || plane_z < -0.1f)
                    {
                        GetPlayer()->m_anti_TeleToPlane_Count++;
                        check_passed = false;
                        #ifdef MOVEMENT_ANTICHEAT_DEBUG
                        sLog.outDebug("MA-%s, teleport to plan exception. plane_z: %f ",
                                        GetPlayer()->GetName(), plane_z);
                        #endif
                        if (GetPlayer()->m_anti_TeleToPlane_Count > World::GetTeleportToPlaneAlarms())
                        {
                            sLog.outError("MA-%s, teleport to plan exception. Exception count: %d ",
                                            GetPlayer()->GetName(), GetPlayer()->m_anti_TeleToPlane_Count);
                            GetPlayer()->GetSession()->KickPlayer();
                            return;
                        }
                    }
                }
            } else {
                if (GetPlayer()->m_anti_TeleToPlane_Count != 0)
                    GetPlayer()->m_anti_TeleToPlane_Count = 0;
            }
        } else if (movementInfo.flags & MOVEMENTFLAG_ONTRANSPORT) {
            //antiwrap checks
            if (GetPlayer()->m_transport)
            {
                float trans_rad = movementInfo.t_x*movementInfo.t_x + movementInfo.t_y*movementInfo.t_y + movementInfo.t_z*movementInfo.t_z;
                if (trans_rad > 3600.0f){
                    check_passed = false;
                    #ifdef MOVEMENT_ANTICHEAT_DEBUG
                    sLog.outError("MA-%s, leave transport.", GetPlayer()->GetName());
                    #endif
                }
            } else {
                if (GameObjectData const* go_data = objmgr.GetGOData(GetPlayer()->m_anti_TransportGUID))
                {
                    float delta_gox = go_data->posX - movementInfo.x;
                    float delta_goy = go_data->posY - movementInfo.y;
                    float delta_goz = go_data->posZ - movementInfo.z;
                    int mapid = go_data->mapid;
                    #ifdef MOVEMENT_ANTICHEAT_DEBUG
                    sLog.outDebug("MA-%s, transport movement. GO xyzo: %f,%f,%f",
                                    GetPlayer()->GetName(), go_data->posX,go_data->posY,go_data->posZ);
                    #endif
                    if (GetPlayer()->GetMapId() != mapid){
                        check_passed = false;
                    } else if (mapid !=369) {
                        float delta_go = delta_gox*delta_gox + delta_goy*delta_goy;
                        if (delta_go > 3600.0f) {
                            check_passed = false;
                            #ifdef MOVEMENT_ANTICHEAT_DEBUG
                            sLog.outError("MA-%s, leave transport. GO xyzo: %f,%f,%f",
                                            GetPlayer()->GetName(), go_data->posX,go_data->posY,go_data->posZ);
                            #endif
                        }
                    }

                } else {
                    #ifdef MOVEMENT_ANTICHEAT_DEBUG
                    sLog.outDebug("MA-%s, undefined transport.", GetPlayer()->GetName());
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
                    GetPlayer()->m_anti_TransportGUID = 0;
            }
        }
    }
    /* process position-change */
    if (check_passed)
    {

    Unit *mover = _player->m_mover;
    recv_data.put<uint32>(6, getMSTime());                  // fix time, offset flags(4) + unk(2)
    WorldPacket data(recv_data.GetOpcode(), (mover->GetPackGUID().size()+recv_data.size()));
    data.append(_player->m_mover->GetPackGUID());           // use mover guid
    data.append(recv_data.contents(), recv_data.size());
    GetPlayer()->SendMessageToSet(&data, false);

    if(!_player->GetCharmGUID())                            // nothing is charmed
    {
        _player->SetPosition(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
        _player->m_movementInfo = movementInfo;
        _player->SetUnitMovementFlags(movementInfo.flags);
    }
    else
    {
        if(mover->GetTypeId() != TYPEID_PLAYER)             // unit, creature, pet, vehicle...
        {
            if(Map *map = mover->GetMap())
                map->CreatureRelocation((Creature*)mover, movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
            mover->SetUnitMovementFlags(movementInfo.flags);
        }
        else                                                // player
        {
            ((Player*)mover)->SetPosition(movementInfo.x, movementInfo.y, movementInfo.z, movementInfo.o);
            ((Player*)mover)->m_movementInfo = movementInfo;
            ((Player*)mover)->SetUnitMovementFlags(movementInfo.flags);
        }
    }

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
    //movement anticheat >>>
        if (GetPlayer()->m_anti_AlarmCount > 0){
            sLog.outError("MA-%s produce %d anticheat alarms",GetPlayer()->GetName(),GetPlayer()->m_anti_AlarmCount);
            GetPlayer()->m_anti_AlarmCount = 0;
        }
    } else {
        GetPlayer()->m_anti_AlarmCount++;
        WorldPacket data;
        GetPlayer()->SetUnitMovementFlags(0);
        GetPlayer()->BuildTeleportAckMsg(&data, GetPlayer()->GetPositionX(), GetPlayer()->GetPositionY(), GetPlayer()->GetPositionZ(), GetPlayer()->GetOrientation());
        GetPlayer()->GetSession()->SendPacket(&data);
        GetPlayer()->BuildHeartBeatMsg(&data);
        GetPlayer()->SendMessageToSet(&data, true);
    }
    // <<< movement anticheat
}

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(recv_data.GetOpcode()), recv_data.GetOpcode(), recv_data.GetOpcode());

    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8+4);

    /* extract packet */
    uint64 guid;
    uint32 unk1;
    float  newspeed;

    recv_data >> guid;

    // now can skip not our packet
    if(_player->GetGUID() != guid)
        return;

    // continue parse packet

    recv_data >> unk1;                                      // counter or moveEvent

    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);

    // recheck
    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+4);

    recv_data >> newspeed;
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

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
    recv_data.hexlike();

    CHECK_PACKET_SIZE(recv_data, 8);

    uint64 guid;
    recv_data >> guid;

    if(_player->m_mover->GetGUID() != guid)
    {
        sLog.outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is " I64FMT " and should be " I64FMT, _player->m_mover->GetGUID(), guid);
        return;
    }
}

void WorldSession::HandleMoveNotActiveMover(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");
    recv_data.hexlike();

    CHECK_PACKET_SIZE(recv_data, recv_data.rpos()+8);

    uint64 old_mover_guid;
    recv_data >> old_mover_guid;

    if(_player->m_mover->GetGUID() == old_mover_guid)
    {
        sLog.outError("HandleMoveNotActiveMover: incorrect mover guid: mover is " I64FMT " and should be " I64FMT " instead of " I64FMT, _player->m_mover->GetGUID(), _player->GetGUID(), old_mover_guid);
        return;
    }

    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;
}

void WorldSession::HandleDismissControlledVehicle(WorldPacket &recv_data)
{
    sLog.outDebug("WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    uint64 vehicleGUID = _player->GetCharmGUID();

    if(!vehicleGUID)                                        // something wrong here...
        return;

    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;

    // using charm guid, because we don't have vehicle guid...
    if(Vehicle *vehicle = ObjectAccessor::GetVehicle(vehicleGUID))
    {
        _player->ExitVehicle(vehicle);
        vehicle->Dismiss();
    }
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

    /* extract packet */
    MovementInfo movementInfo;
    uint32 unk1,unk2,unk3;
    recv_data >> unk1 >> unk2 >> unk3;
    ReadMovementInfo(recv_data, &movementInfo);

    //Save movement flags
    _player->SetUnitMovementFlags(movementInfo.flags);

    #ifdef MOVEMENT_ANTICHEAT_DEBUG
    sLog.outBasic("%s CMSG_MOVE_KNOCK_BACK_ACK: tm:%d ftm:%d | %f,%f,%fo(%f) [%X]",GetPlayer()->GetName(),movementInfo.time,movementInfo.fallTime,movementInfo.x,movementInfo.y,movementInfo.z,movementInfo.o,movementInfo.flags);
    sLog.outBasic("%s CMSG_MOVE_KNOCK_BACK_ACK additional: vspeed:%f, hspeed:%f",GetPlayer()->GetName(), movementInfo.j_unk, movementInfo.j_xyspeed);
    #endif

    _player->m_movementInfo = movementInfo;
    _player->m_anti_Last_HSpeed = movementInfo.j_xyspeed;
    _player->m_anti_Last_VSpeed = movementInfo.j_unk < 3.2f ? movementInfo.j_unk - 1.0f : 3.2f;
    _player->m_anti_LastSpeedChangeTime = movementInfo.time + 1750;
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
