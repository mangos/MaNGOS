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
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "World.h"

#define ANTICHEAT_DEFAULT_DELTA 2000

static AntiCheatCheckEntry AntiCheatCheckList[] =
{
// Checks
    { true,  CHECK_NULL,                    &AntiCheat::CheckNull          },
    { true,  CHECK_MOVEMENT,                &AntiCheat::CheckMovement      },
    { true,  CHECK_SPELL,                   &AntiCheat::CheckSpell         },
    { true,  CHECK_QUEST,                   &AntiCheat::CheckQuest         },
    { true,  CHECK_TRANSPORT,               &AntiCheat::CheckOnTransport   },
    { true,  CHECK_DAMAGE,                  &AntiCheat::CheckDamage        },
// Subchecks
    { true,  CHECK_MOVEMENT_SPEED,          &AntiCheat::CheckSpeed         },
    { true,  CHECK_MOVEMENT_FLY,            &AntiCheat::CheckFly           },
    { true,  CHECK_MOVEMENT_MOUNTAIN,       &AntiCheat::CheckMountain      },
    { true,  CHECK_MOVEMENT_WATERWALKING,   &AntiCheat::CheckWaterWalking  },
    { true,  CHECK_MOVEMENT_TP2PLANE,       &AntiCheat::CheckTp2Plane      },
    { true,  CHECK_MOVEMENT_AIRJUMP,        &AntiCheat::CheckAirJump       },
    { true,  CHECK_MOVEMENT_TELEPORT,       &AntiCheat::CheckTeleport      },
    { true,  CHECK_MOVEMENT_FALL,           &AntiCheat::CheckFall          },
    { true,  CHECK_DAMAGE_SPELL,            &AntiCheat::CheckSpellDamage   },
    { true,  CHECK_DAMAGE_MELEE,            &AntiCheat::CheckMeleeDamage   },
    { true,  CHECK_SPELL_VALID,             &AntiCheat::CheckSpellValid    },
    { true,  CHECK_SPELL_ONDEATH,           &AntiCheat::CheckSpellOndeath  },
    { true,  CHECK_SPELL_FAMILY,            &AntiCheat::CheckSpellFamily   },
    { true,  CHECK_SPELL_INBOOK,            &AntiCheat::CheckSpellInbook   },
    // Finish for search
    { false, CHECK_MAX,                     NULL }
};


AntiCheat::AntiCheat(Player* player)
{
    m_player              = player;
    m_MovedLen            = 0.0f;
    m_isFall              = false;
    m_isActiveMover       = true;
    //
    m_currentmovementInfo = NULL;
    m_currentMover        = ((Unit*)player);
    m_currentspellID      = 0;
    m_currentOpcode       = 0;
    m_currentConfig       = NULL;
    m_currentDelta        = 0.0f;
    m_currentDeltaZ       = 0.0f;
    m_lastfalltime        = 0;
    m_lastfallz           = 0.0f;
    //
    m_immuneTime          = WorldTimer::getMSTime();
    m_lastClientTime      = WorldTimer::getMSTime();
    m_lastLiveState       = ALIVE;
    //
    m_currentCheckResult.clear();
    m_counters.clear();
    m_oldCheckTime.clear();
    m_lastalarmtime.clear();
    m_lastactiontime.clear();
    SetImmune(ANTICHEAT_DEFAULT_DELTA);

};

AntiCheat::~AntiCheat()
{
};

bool AntiCheat::CheckNull()
{
    DEBUG_LOG("AntiCheat: CheckNull called");
    return true;
};

AntiCheatCheckEntry* AntiCheat::_FindCheck(AntiCheatCheck checktype)
{

    for(uint16 i = 0; AntiCheatCheckList[i].checkType != CHECK_MAX; ++i)
    {
        AntiCheatCheckEntry* pEntry = &AntiCheatCheckList[i];
        if (pEntry->checkType == checktype)
            return &AntiCheatCheckList[i];
    }

    return NULL;
};

AntiCheatConfig const* AntiCheat::_FindConfig(AntiCheatCheck checkType)
{
    return sObjectMgr.GetAntiCheatConfig(uint32(checkType));
};

bool AntiCheat::_DoAntiCheatCheck(AntiCheatCheck checktype)
{
    m_currentConfig = _FindConfig(checktype);

    if (!m_currentConfig)
        return true;

    AntiCheatCheckEntry* _check = _FindCheck(checktype);

    if (!_check)
        return true;

    bool checkpassed = true;

    if (_check->active && CheckTimer(checktype) && CheckNeeded(checktype))
    {
        if (m_counters.find(checktype) == m_counters.end())
            m_counters.insert(std::make_pair(checktype, 0));

        if (!(this->*(_check->Handler))() && !isImmune())
        {
            if (m_currentConfig->disableOperation)
                checkpassed = false;
            ++m_counters[checktype];

            if (m_lastalarmtime.find(checktype) == m_lastalarmtime.end())
                m_lastalarmtime.insert(std::make_pair(checktype, 0));

            m_lastalarmtime[checktype] = WorldTimer::getMSTime();

            if (m_counters[checktype] >= m_currentConfig->alarmsCount)
            {
                m_currentCheckResult.insert(0,m_currentConfig->description.c_str());
                DoAntiCheatAction(checktype, m_currentCheckResult);
                m_counters[checktype] = 0;
                m_currentCheckResult.clear();
            }
        }
        else
        {
            if (WorldTimer::getMSTimeDiff(m_lastalarmtime[checktype],WorldTimer::getMSTime()) > sWorld.getConfig(CONFIG_UINT32_ANTICHEAT_ACTION_DELAY)
                || (m_currentConfig->checkParam[0] > 0 && m_currentConfig->alarmsCount > 1 && WorldTimer::getMSTimeDiff(m_lastalarmtime[checktype],WorldTimer::getMSTime()) > m_currentConfig->checkParam[0]))
            {
                m_counters[checktype] = 0;
            }
        }
        m_oldCheckTime[checktype] = WorldTimer::getMSTime();
    }


    // Subchecks, if exist
    if (checktype < 100 && _check->active && CheckNeeded(checktype))
    {
        for (int i=1; i < 99; ++i )
        {
            uint32 subcheck = checktype * 100 + i;

            if (AntiCheatConfig const* config = _FindConfig(AntiCheatCheck(subcheck)))
            {
                checkpassed |= _DoAntiCheatCheck(AntiCheatCheck(subcheck));
            }
            else
                break;
        }
    }
    // If any of checks fail, return false
    return checkpassed;
};

bool AntiCheat::CheckTimer(AntiCheatCheck checkType)
{
    AntiCheatConfig const* config = _FindConfig(checkType);

    if (!config->checkPeriod)
        return true;

    const uint32 currentTime = WorldTimer::getMSTime();

    if (m_oldCheckTime.find(checkType) == m_oldCheckTime.end())
        m_oldCheckTime.insert(std::make_pair(checkType, currentTime));

    if (currentTime - m_oldCheckTime[checkType] >= config->checkPeriod)
        return true;

    return false;
}

void AntiCheat::DoAntiCheatAction(AntiCheatCheck checkType, std::string reason)
{
    AntiCheatConfig const* config = _FindConfig(checkType);

    if (!config)
        return;

    if (m_lastactiontime.find(checkType) == m_lastactiontime.end())
        m_lastactiontime.insert(std::make_pair(checkType, 0));

    if (WorldTimer::getMSTime() - m_lastactiontime[checkType] >= sWorld.getConfig(CONFIG_UINT32_ANTICHEAT_ACTION_DELAY) * 1000)
    {
        m_lastactiontime[checkType] = WorldTimer::getMSTime();

        std::string name = GetPlayer()->GetName();
        std::string namechat;
        MapEntry const* mapEntry = sMapStore.LookupEntry(GetPlayer()->GetMapId());
        uint32 zone_id, area_id;
        GetPlayer()->GetZoneAndAreaId(zone_id,area_id);
        AreaTableEntry const* zoneEntry = GetAreaEntryByAreaID(zone_id);
        AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(area_id);
        char buffer[255];

        namechat.clear();
        namechat.append(" |cddff0000|Hplayer:");
        namechat.append(name);
        namechat.append("|h[");
        namechat.append(name);
        namechat.append("]|h|r ");
        sprintf(buffer," Map %u (%s), Zone %u (%s) Area |cbbdd0000|Harea:%u|h[%s]|h|r ",
        GetPlayer()->GetMapId(), (mapEntry ? mapEntry->name[sWorld.GetDefaultDbcLocale()] : "<unknown>" ),
        zone_id, (zoneEntry ? zoneEntry->area_name[sWorld.GetDefaultDbcLocale()] : "<unknown>" ),
        area_id, (areaEntry ? areaEntry->area_name[sWorld.GetDefaultDbcLocale()] : "<unknown>" ));

        if (m_currentspellID)
        {
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(m_currentspellID);
            if (spellInfo)
                sprintf(buffer,", last spell |cbbee0000|Hspell:%u|h[%s]|h|r  ",
                    m_currentspellID, spellInfo->SpellName[sWorld.GetDefaultDbcLocale()]);
        }

        namechat.append(buffer);

        for (int i=0; i < ANTICHEAT_ACTIONS; ++i )
        {
            AntiCheatAction actionID = AntiCheatAction(config->actionType[i]);


            switch(actionID)
            {
                case    ANTICHEAT_ACTION_KICK:
                        GetPlayer()->GetSession()->KickPlayer();
                break;

                case    ANTICHEAT_ACTION_BAN:
                    sWorld.BanAccount(BAN_CHARACTER, name.c_str(), config->actionParam[i], reason, "AntiCheat");
                break;

                case    ANTICHEAT_ACTION_SHEEP:
                    {
                        uint32 sheepAura = 28272;
                        switch (urand(0,6))
                        {
                            case 0:
                                sheepAura = 118;
                            break;
                            case 1:
                                sheepAura = 28271;
                            break;
                            case 2:
                                sheepAura = 28272;
                            break;
                            case 3:
                                sheepAura = 61025;
                            break;
                            case 4:
                                sheepAura = 61721;
                            break;
                            case 5:
                                sheepAura = 71319;
                            break;
                            default:
                            break;
                        }
                        GetPlayer()->_AddAura(sheepAura,config->actionParam[i]);

                        if (checkType == CHECK_MOVEMENT_FLY ||
                            GetPlayer()->HasAuraType(SPELL_AURA_FLY))
                            {
                                GetPlayer()->CastSpell(GetPlayer(), 55001, true);
                            }
                    }
                break;

                case    ANTICHEAT_ACTION_STUN:
                        GetPlayer()->_AddAura(13005,config->actionParam[i]);
                break;

                case    ANTICHEAT_ACTION_SICKNESS:
                        GetPlayer()->_AddAura(15007,config->actionParam[i]);
                break;

                case    ANTICHEAT_ACTION_ANNOUNCE_GM:
                        sWorld.SendWorldTextWithSecurity(AccountTypes(config->actionParam[i]), config->messageNum, namechat.c_str(), config->description.c_str());
                break;

                case    ANTICHEAT_ACTION_ANNOUNCE_ALL:
                        sWorld.SendWorldText(config->messageNum, name.c_str(), config->description.c_str());
                break;

                case    ANTICHEAT_ACTION_LOG:
                case    ANTICHEAT_ACTION_NULL:
                default:
                break;

            }
        }
    }

    if (config->actionType[0] != ANTICHEAT_ACTION_NULL)
    {
        if (reason == "" )
        {
            sLog.outError("AntiCheat action log: Missing Reason parameter!");
            return;
        }

        const char* playerName = GetPlayer()->GetName();

        if ( !playerName )
        {
           sLog.outError("AntiCheat action log: Player with no name?");
           return;
        }

        CharacterDatabase.PExecute("REPLACE INTO `anticheat_log` (`guid`, `playername`, `checktype`, `alarm_time`, `action1`, `action2`, `reason`)"
                                   "VALUES ('%u','%s','%u',NOW(),'%u','%u','%s')",
                                   GetPlayer()->GetObjectGuid().GetCounter(),
                                   playerName,
                                   checkType,
                                   config->actionType[0],
                                   config->actionType[1],
                                   reason.c_str());
    }

}

bool AntiCheat::CheckNeeded(AntiCheatCheck checktype)
{
    if (!sWorld.getConfig(CONFIG_BOOL_ANTICHEAT_ENABLE)
        || !GetPlayer()->IsInWorld()
        || GetPlayer()->IsBeingTeleported()
        || GetPlayer()->GetSession()->GetSecurity() > sWorld.getConfig(CONFIG_UINT32_ANTICHEAT_GMLEVEL))
        return false;

    if (GetMover()->HasAuraType(SPELL_AURA_MOD_CONFUSE))
        return false;

    AntiCheatCheck checkMainType =  (checktype >= 100) ? AntiCheatCheck(checktype / 100) : checktype;

    switch( checkMainType)
    {
        case CHECK_NULL:
            return false;
            break;
        case CHECK_MOVEMENT:
            if (   GetPlayer()->GetTransport()
                || GetPlayer()->HasMovementFlag(MOVEFLAG_ONTRANSPORT) 
                || GetMover()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE
                || GetPlayer()->IsTaxiFlying())
                return false;
            break;
        case CHECK_SPELL:
            break;
        case CHECK_QUEST:
            return false;
            break;
        case CHECK_TRANSPORT:
            if (!isActiveMover())
                return false;
            break;
        case CHECK_DAMAGE:
            break;
        default:
            return false;
            break;
    }

    if (checktype < 100 )
        return true;

    switch( checktype)
    {
        case CHECK_MOVEMENT_SPEED:
            if  (GetMover()->HasAura(56266))
                return false;
            break;
        case CHECK_MOVEMENT_FLY:
            if (isCanFly() || !GetMover())
                return false;
            break;
        case CHECK_MOVEMENT_WATERWALKING:
            if (!m_currentmovementInfo->HasMovementFlag(MOVEFLAG_WATERWALKING))
                return false;
            break;
        case CHECK_MOVEMENT_TP2PLANE:
            if (m_currentmovementInfo->HasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING)))
                return false;
            if (GetMover()->HasAura(60068) && GetMover()->GetTerrain()->IsUnderWater(m_currentmovementInfo->GetPos()->x, m_currentmovementInfo->GetPos()->y, m_currentmovementInfo->GetPos()->z-5.0f))
                return false;
            break;
        case CHECK_MOVEMENT_AIRJUMP:
            if (isCanFly() ||
                !isActiveMover() ||
                GetMover()->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
                GetMover()->GetTerrain()->IsUnderWater(m_currentmovementInfo->GetPos()->x, m_currentmovementInfo->GetPos()->y, m_currentmovementInfo->GetPos()->z-5.0f))
                return false;
            break;
        case CHECK_MOVEMENT_TELEPORT:
            if (!isActiveMover() || GetPlayer()->IsBeingTeleported())
                return false;
            break;
        case CHECK_MOVEMENT_FALL:
            if (isCanFly() || !isActiveMover())
                return false;
            break;
        case CHECK_MOVEMENT_MOUNTAIN:
            if (isCanFly() || !isActiveMover())
                return false;
            break;
        default:
            break;
    }

    return true;

}

// Movement checks
bool AntiCheat::CheckMovement()
{
    if (!GetPlayer()->IsSelfMover() && isActiveMover())
    {
        SetActiveMover(false);
        m_currentMover  = GetPlayer()->GetMover();
        m_MovedLen = 0.0f;
        SetImmune(ANTICHEAT_DEFAULT_DELTA);
    }
    else if (GetPlayer()->IsSelfMover() && !isActiveMover())
    {
        SetActiveMover(true);
        m_currentMover  = ((Unit*)GetPlayer());
        m_MovedLen = 0.0f;
        SetImmune(ANTICHEAT_DEFAULT_DELTA);
    }

    if (GetPlayer()->IsBeingTeleported())
        SetImmune(ANTICHEAT_DEFAULT_DELTA);

    SetLastLiveState(GetPlayer()->getDeathState());

    float delta_x   = GetMover()->GetPositionX() - m_currentmovementInfo->GetPos()->x;
    float delta_y   = GetMover()->GetPositionY() - m_currentmovementInfo->GetPos()->y;
    m_currentDeltaZ = GetMover()->GetPositionZ() - m_currentmovementInfo->GetPos()->z;

    m_currentDelta = sqrt(delta_x * delta_x + delta_y * delta_y);

    m_MovedLen += m_currentDelta;

    return true;
}

bool AntiCheat::CheckSpeed()
{
    float speedRate   = 1.0f;
    int   serverDelta = WorldTimer::getMSTimeDiff(m_oldCheckTime[CHECK_MOVEMENT_SPEED],WorldTimer::getMSTime());

    if (m_currentTimeSkipped > 0 && (float)m_currentTimeSkipped < serverDelta)
    {
        serverDelta += m_currentTimeSkipped;
        m_currentTimeSkipped = 0;
    }
    else if (m_currentTimeSkipped > 0 && (float)m_currentTimeSkipped > serverDelta)
    {
        m_currentTimeSkipped = 0;
        return true;
    }
    uint32 clientTime  = m_currentmovementInfo->time;
    int clientDelta = clientTime - m_lastClientTime;

    m_lastClientTime   = clientTime;

    float delta_t     = float(std::max(clientDelta,serverDelta));

    float moveSpeed = m_MovedLen / delta_t;

    m_MovedLen = 0.0f;

    std::string mode;

    if      (m_currentmovementInfo->GetMovementFlags() & MOVEFLAG_FLYING)
        {
            speedRate = GetMover()->GetSpeed(MOVE_FLIGHT);
            mode = "MOVE_FLIGHT";
        }
    else if (m_currentmovementInfo->GetMovementFlags() & MOVEFLAG_SWIMMING)
        {
            speedRate = GetMover()->GetSpeed(MOVE_SWIM);
            mode = "MOVE_SWIM";
        }
    else if (m_currentmovementInfo->GetMovementFlags() & MOVEFLAG_WALK_MODE)
        {
            speedRate = GetMover()->GetSpeed(MOVE_WALK);
            mode = "MOVE_WALK";
        }
    else
        {
            speedRate = GetMover()->GetSpeed(MOVE_RUN);
            mode = "MOVE_RUN";
        }

    if ( moveSpeed / speedRate <= m_currentConfig->checkFloatParam[0] )
        return true;

    char buffer[255];
    sprintf(buffer," Speed is %f but allowed %f Mode is %s, opcode is %s, client delta is %d, server delta is %d",
                 moveSpeed / speedRate, m_currentConfig->checkFloatParam[0],mode.c_str(), LookupOpcodeName(m_currentOpcode), clientDelta, serverDelta);
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;
}


bool AntiCheat::CheckWaterWalking()
{
    if  (   GetMover()->HasAuraType(SPELL_AURA_WATER_WALK)
        ||  GetMover()->HasAura(60068)
        ||  GetMover()->HasAura(61081)
        ||  GetMover()->HasAuraType(SPELL_AURA_GHOST)
        )
        return true;

    m_currentCheckResult.clear();

    return false;
}

bool AntiCheat::CheckTeleport()
{

    if (m_currentDelta < m_currentConfig->checkFloatParam[0])
        return true;

    char buffer[255];
    sprintf(buffer," Moved with with one tick on %e but allowed %e",
                 m_currentDelta, m_currentConfig->checkFloatParam[0]);
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);

    return false;
}

bool AntiCheat::CheckMountain()
{
    if (m_currentmovementInfo->HasMovementFlag(MovementFlags(MOVEFLAG_FLYING | MOVEFLAG_SWIMMING)))
        return true;

    if ( m_currentDeltaZ > 0 )
        return true;

    int  serverDelta = WorldTimer::getMSTimeDiff(m_oldCheckTime[CHECK_MOVEMENT_MOUNTAIN],WorldTimer::getMSTime());

    float zSpeed = - m_currentDeltaZ / serverDelta;

    float tg_z = (m_currentDelta > 0.0f) ? (-m_currentDeltaZ / m_currentDelta) : -99999;

    if (tg_z < m_currentConfig->checkFloatParam[1] || zSpeed < m_currentConfig->checkFloatParam[0] )
        return true;

    char buffer[255];
    sprintf(buffer," deltaZ %e, angle %e, speedZ %e ",
                 m_currentDeltaZ, tg_z, zSpeed);
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);

    return false;
}

bool AntiCheat::CheckFall()
{
    if (!m_isFall)
    {
        m_lastfalltime = m_currentmovementInfo->GetFallTime();
        m_lastfallz    = m_currentmovementInfo->GetPos()->z;
        SetInFall(true);
    }
    else
    {
        if (m_lastfallz - m_currentmovementInfo->GetPos()->z >= 0.0f)
            SetInFall(false);
    }
    return true;
}

bool AntiCheat::CheckFly()
{
    if (GetMover()->GetTerrain()->IsUnderWater(m_currentmovementInfo->GetPos()->x, m_currentmovementInfo->GetPos()->y, m_currentmovementInfo->GetPos()->z - 2.0f))
        return true;

    if (!m_currentmovementInfo->HasMovementFlag(MovementFlags(MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING | MOVEFLAG_ROOT)))
        return true;

    if (GetMover()->HasAuraType(SPELL_AURA_FEATHER_FALL))
        return true;

    float ground_z = GetMover()->GetTerrain()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),MAX_HEIGHT);
    float floor_z  = GetMover()->GetTerrain()->GetHeight(GetPlayer()->GetPositionX(),GetPlayer()->GetPositionY(),GetPlayer()->GetPositionZ());
    float map_z    = ((floor_z <= (INVALID_HEIGHT+5.0f)) ? ground_z : floor_z);

    if (map_z + m_currentConfig->checkFloatParam[0] > GetPlayer()->GetPositionZ() && map_z > (INVALID_HEIGHT + m_currentConfig->checkFloatParam[0] + 5.0f))
        return true;

    if (m_currentDeltaZ > 0.0f)
        return true;

    char buffer[255];
    sprintf(buffer," flying without fly auras on height %e but allowed %e",
                 GetPlayer()->GetPositionZ(), map_z + m_currentConfig->checkFloatParam[0]);
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);

    return false;
}

bool AntiCheat::CheckAirJump()
{

    float ground_z = GetMover()->GetTerrain()->GetHeight(GetMover()->GetPositionX(),GetMover()->GetPositionY(),MAX_HEIGHT);
    float floor_z  = GetMover()->GetTerrain()->GetHeight(GetMover()->GetPositionX(),GetMover()->GetPositionY(),GetMover()->GetPositionZ());
    float map_z    = ((floor_z <= (INVALID_HEIGHT+5.0f)) ? ground_z : floor_z);

    if  (!((map_z + m_currentConfig->checkFloatParam[0] + m_currentConfig->checkFloatParam[1] < GetPlayer()->GetPositionZ() &&
         (m_currentmovementInfo->GetMovementFlags() & (MOVEFLAG_FALLINGFAR |MOVEFLAG_PENDINGSTOP)) == 0) ||
         (map_z + m_currentConfig->checkFloatParam[0] < GetMover()->GetPositionZ() && m_currentOpcode == MSG_MOVE_JUMP)))
        return true;

    if (m_currentDeltaZ > 0.0f)
        return true;

    char buffer[255];
    sprintf(buffer," Map Z = %f, player Z = %f, opcode %s",
                 map_z, GetPlayer()->GetPositionZ(), LookupOpcodeName(m_currentOpcode));

    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;
}

bool AntiCheat::CheckTp2Plane()
{
    if (m_currentmovementInfo->GetPos()->z > m_currentConfig->checkFloatParam[0] || m_currentmovementInfo->GetPos()->z < -m_currentConfig->checkFloatParam[0])
        return true;

    if (GetMover()->HasAuraType(SPELL_AURA_GHOST))
        return true;

    float plane_z = 0.0f;

    plane_z = GetMover()->GetTerrain()->GetHeight(m_currentmovementInfo->GetPos()->x, m_currentmovementInfo->GetPos()->y, MAX_HEIGHT) - m_currentmovementInfo->GetPos()->z;
    plane_z = (plane_z < -500.0f) ? 0 : plane_z; //check holes in heigth map
    if(plane_z < m_currentConfig->checkFloatParam[1] && plane_z > -m_currentConfig->checkFloatParam[1])
            return true;

    char buffer[255];
    sprintf(buffer," Plane Z = %e, player Z = %e, opcode %s",
                 plane_z, GetPlayer()->GetPositionZ(), LookupOpcodeName(m_currentOpcode));

    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;

}

// Transport checks
bool AntiCheat::CheckOnTransport()
{

    if  (GetMover()->HasAura(56266))
        return true;

    float trans_rad = sqrt(m_currentmovementInfo->GetTransportPos()->x * m_currentmovementInfo->GetTransportPos()->x + m_currentmovementInfo->GetTransportPos()->y * m_currentmovementInfo->GetTransportPos()->y + m_currentmovementInfo->GetTransportPos()->z * m_currentmovementInfo->GetTransportPos()->z);
    if (trans_rad < + m_currentConfig->checkFloatParam[0])
        return true;

    char buffer[255];
    sprintf(buffer," Transport radius = %f, opcode = %s ",
                 trans_rad, LookupOpcodeName(m_currentOpcode));

    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;
}

// Spell checks
bool AntiCheat::CheckSpell()
{
// in process
    return true;
}

bool AntiCheat::CheckSpellValid()
{
// in process
    return true;
}

bool AntiCheat::CheckSpellOndeath()
{

    if (GetPlayer()->getDeathState() == ALIVE)
        return true;

    char buffer[255];
    sprintf(buffer," player is not in ALIVE state, but cast spell %u ",
                 m_currentspellID);

}

bool AntiCheat::CheckSpellFamily()
{
// in process
    if (!m_currentspellID)
        return true;

    bool checkPassed = true;
    std::string mode = "";

    SkillLineAbilityMapBounds skill_bounds = sSpellMgr.GetSkillLineAbilityMapBounds(m_currentspellID);

    for(SkillLineAbilityMap::const_iterator _spell_idx = skill_bounds.first; _spell_idx != skill_bounds.second; ++_spell_idx)
    {
        SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(_spell_idx->second->skillId);

        if (!pSkill)
            continue;

        if (pSkill->id == 769)
        {
            checkPassed = false;
            mode = " it is GM spell!";
        }
    }

    if (checkPassed)
        return true;

    char buffer[255];
    sprintf(buffer," spell %u, reason: %s", m_currentspellID,mode.c_str());
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;

}

bool AntiCheat::CheckSpellInbook()
{
// in process
    return true;
}

// Quest checks
bool AntiCheat::CheckQuest()
{
// in process
    return true;
}

// Damage checks
bool AntiCheat::CheckDamage()
{
// in process
    return true;
}

bool AntiCheat::CheckSpellDamage()
{
    if (!m_currentspellID)
        return true;

    if (m_currentDamage < m_currentConfig->checkParam[1])
        return true;

    uint32 calcdamage = 0;
/*
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(m_currentspellID);
    if (spellInfo)
    {
        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            calcdamage +=;
        }

    }
*/

    char buffer[255];
    sprintf(buffer," Spell %d deal damage is %d, but allowed %d",
                 m_currentspellID, m_currentDamage, m_currentConfig->checkFloatParam[1]);

    m_currentspellID = 0;
    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;
}

bool AntiCheat::CheckMeleeDamage()
{
    if (m_currentspellID)
        return true;

    if (m_currentDamage < m_currentConfig->checkParam[1])
        return true;

    char buffer[255];
    sprintf(buffer," Dealed melee damage %d, but allowed %d",
                 m_currentDamage, m_currentConfig->checkFloatParam[1]);

    m_currentCheckResult.clear();
    m_currentCheckResult.append(buffer);
    return false;
}

bool AntiCheat::isCanFly()
{
    if (   GetMover()->HasAuraType(SPELL_AURA_FLY)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING)
        || GetMover()->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING)
       )
        return true;

    return false;
}

bool AntiCheat::isInFall()
{
    return m_isFall;
}

bool AntiCheat::isImmune()
{
    if (m_immuneTime > WorldTimer::getMSTime())
        return true;
    else
        return false;
}

void AntiCheat::SetImmune(uint32 timeDelta)
{
    m_immuneTime = WorldTimer::getMSTime() + timeDelta;
}

void AntiCheat::SetLastLiveState(DeathState state)
{
    if (state  != m_lastLiveState)
    {
        m_lastLiveState = state;
        SetImmune(ANTICHEAT_DEFAULT_DELTA);
    }
}
