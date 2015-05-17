/**
 * ScriptDev2 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * ScriptData
 * SDName:      Eastern_Plaguelands
 * SD%Complete: 100
 * SDComment:   Quest support: 7622.
 * SDCategory:  Eastern Plaguelands
 * EndScriptData
 */

/**
 * ContentData
 * npc_eris_havenfire
 * EndContentData
 */

#include "precompiled.h"

/*######
## npc_eris_havenfire
######*/

enum
{
    SAY_PHASE_HEAL                      = -1000815,
    SAY_EVENT_END                       = -1000816,
    SAY_EVENT_FAIL_1                    = -1000817,
    SAY_EVENT_FAIL_2                    = -1000818,
    SAY_PEASANT_APPEAR_1                = -1000819,
    SAY_PEASANT_APPEAR_2                = -1000820,
    SAY_PEASANT_APPEAR_3                = -1000821,

    // SPELL_DEATHS_DOOR                 = 23127,           // damage spells cast on the peasants
    // SPELL_SEETHING_PLAGUE             = 23072,
    SPELL_ENTER_THE_LIGHT_DND           = 23107,
    SPELL_BLESSING_OF_NORDRASSIL        = 23108,

    NPC_INJURED_PEASANT                 = 14484,
    NPC_PLAGUED_PEASANT                 = 14485,
    NPC_SCOURGE_ARCHER                  = 14489,
    NPC_SCOURGE_FOOTSOLDIER             = 14486,
    NPC_THE_CLEANER                     = 14503,            // can be summoned if the priest has more players in the party for help. requires further research

    QUEST_BALANCE_OF_LIGHT_AND_SHADOW   = 7622,

    MAX_PEASANTS                        = 12,
    MAX_ARCHERS                         = 8,
};

static const float aArcherSpawn[8][4] =
{
    {3327.42f, -3021.11f, 170.57f, 6.01f},
    {3335.4f,  -3054.3f,  173.63f, 0.49f},
    {3351.3f,  -3079.08f, 178.67f, 1.15f},
    {3358.93f, -3076.1f,  174.87f, 1.57f},
    {3371.58f, -3069.24f, 175.20f, 1.99f},
    {3369.46f, -3023.11f, 171.83f, 3.69f},
    {3383.25f, -3057.01f, 181.53f, 2.21f},
    {3380.03f, -3062.73f, 181.90f, 2.31f},
};

static const float aPeasantSpawnLoc[3] = {3360.12f, -3047.79f, 165.26f};
static const float aPeasantMoveLoc[3] = {3335.0f, -2994.04f, 161.14f};

static const int32 aPeasantSpawnYells[3] = {SAY_PEASANT_APPEAR_1, SAY_PEASANT_APPEAR_2, SAY_PEASANT_APPEAR_3};

struct npc_eris_havenfire : public CreatureScript
{
    npc_eris_havenfire() : CreatureScript("npc_eris_havenfire") {}

    struct npc_eris_havenfireAI : public ScriptedAI
    {
        npc_eris_havenfireAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiEventTimer;
        uint32 m_uiSadEndTimer;
        uint8 m_uiPhase;
        uint8 m_uiCurrentWave;
        uint8 m_uiKillCounter;
        uint8 m_uiSaveCounter;

        ObjectGuid m_playerGuid;
        GuidList m_lSummonedGuidList;

        void Reset() override
        {
            m_uiEventTimer = 0;
            m_uiSadEndTimer = 0;
            m_uiPhase = 0;
            m_uiCurrentWave = 0;
            m_uiKillCounter = 0;
            m_uiSaveCounter = 0;

            m_playerGuid.Clear();
            m_lSummonedGuidList.clear();
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_INJURED_PEASANT:
            case NPC_PLAGUED_PEASANT:
                float fX, fY, fZ;
                pSummoned->GetRandomPoint(aPeasantMoveLoc[0], aPeasantMoveLoc[1], aPeasantMoveLoc[2], 10.0f, fX, fY, fZ);
                pSummoned->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                m_lSummonedGuidList.push_back(pSummoned->GetObjectGuid());
                break;
            case NPC_SCOURGE_FOOTSOLDIER:
            case NPC_THE_CLEANER:
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                {
                    pSummoned->AI()->AttackStart(pPlayer);
                }
                break;
            case NPC_SCOURGE_ARCHER:
                // ToDo: make these ones attack the peasants
                break;
            }

            m_lSummonedGuidList.push_back(pSummoned->GetObjectGuid());
        }

        void SummonedMovementInform(Creature* pSummoned, uint32 uiMotionType, uint32 uiPointId) override
        {
            if (uiMotionType != POINT_MOTION_TYPE || !uiPointId)
            {
                return;
            }

            if (uiPointId)
            {
                ++m_uiSaveCounter;
                pSummoned->GetMotionMaster()->Clear();

                pSummoned->RemoveAllAuras();
                pSummoned->CastSpell(pSummoned, SPELL_ENTER_THE_LIGHT_DND, false);
                pSummoned->ForcedDespawn(10000);

                // Event ended
                if (m_uiSaveCounter >= 50 && m_uiCurrentWave == 5)
                {
                    DoBalanceEventEnd();
                }
                // Phase ended
                else if (m_uiSaveCounter + m_uiKillCounter == m_uiCurrentWave * MAX_PEASANTS)
                {
                    DoHandlePhaseEnd();
                }
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_INJURED_PEASANT || pSummoned->GetEntry() == NPC_PLAGUED_PEASANT)
            {
                ++m_uiKillCounter;

                // If more than 15 peasants have died, then fail the quest
                if (m_uiKillCounter == MAX_PEASANTS)
                {
                    if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                    {
                        pPlayer->FailQuest(QUEST_BALANCE_OF_LIGHT_AND_SHADOW);
                    }

                    DoScriptText(SAY_EVENT_FAIL_1, m_creature);
                    m_uiSadEndTimer = 4000;
                }
                else if (m_uiSaveCounter + m_uiKillCounter == m_uiCurrentWave * MAX_PEASANTS)
                {
                    DoHandlePhaseEnd();
                }
            }
        }

        void DoSummonWave(uint32 uiSummonId = 0)
        {
            float fX, fY, fZ;

            if (!uiSummonId)
            {
                for (uint8 i = 0; i < MAX_PEASANTS; ++i)
                {
                    uint32 uiSummonEntry = roll_chance_i(70) ? NPC_INJURED_PEASANT : NPC_PLAGUED_PEASANT;
                    m_creature->GetRandomPoint(aPeasantSpawnLoc[0], aPeasantSpawnLoc[1], aPeasantSpawnLoc[2], 10.0f, fX, fY, fZ);
                    if (Creature* pTemp = m_creature->SummonCreature(uiSummonEntry, fX, fY, fZ, 0, TEMPSUMMON_DEAD_DESPAWN, 0))
                    {
                        // Only the first mob needs to yell
                        if (!i)
                        {
                            DoScriptText(aPeasantSpawnYells[urand(0, 2)], pTemp);
                        }
                    }
                }

                ++m_uiCurrentWave;
            }
            else if (uiSummonId == NPC_SCOURGE_FOOTSOLDIER)
            {
                uint8 uiRand = urand(2, 3);
                for (uint8 i = 0; i < uiRand; ++i)
                {
                    m_creature->GetRandomPoint(aPeasantSpawnLoc[0], aPeasantSpawnLoc[1], aPeasantSpawnLoc[2], 15.0f, fX, fY, fZ);
                    m_creature->SummonCreature(NPC_SCOURGE_FOOTSOLDIER, fX, fY, fZ, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                }
            }
            else if (uiSummonId == NPC_SCOURGE_ARCHER)
            {
                for (uint8 i = 0; i < MAX_ARCHERS; ++i)
                {
                    m_creature->SummonCreature(NPC_SCOURGE_ARCHER, aArcherSpawn[i][0], aArcherSpawn[i][1], aArcherSpawn[i][2], aArcherSpawn[i][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                }
            }
        }

        void DoHandlePhaseEnd()
        {
            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
            {
                pPlayer->CastSpell(pPlayer, SPELL_BLESSING_OF_NORDRASSIL, true);
            }

            DoScriptText(SAY_PHASE_HEAL, m_creature);

            // Send next wave
            if (m_uiCurrentWave < 5)
            {
                DoSummonWave();
            }
        }

        void DoStartBalanceEvent(Player* pPlayer)
        {
            m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            m_playerGuid = pPlayer->GetObjectGuid();
            m_uiEventTimer = 5000;
        }

        void DoBalanceEventEnd()
        {
            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
            {
                pPlayer->AreaExploredOrEventHappens(QUEST_BALANCE_OF_LIGHT_AND_SHADOW);
            }

            DoScriptText(SAY_EVENT_END, m_creature);
            DoDespawnSummons(true);
            EnterEvadeMode();
        }

        void DoDespawnSummons(bool bIsEventEnd = false)
        {
            for (GuidList::const_iterator itr = m_lSummonedGuidList.begin(); itr != m_lSummonedGuidList.end(); ++itr)
            {
                if (Creature* pTemp = m_creature->GetMap()->GetCreature(*itr))
                {
                    if (bIsEventEnd && (pTemp->GetEntry() == NPC_INJURED_PEASANT || pTemp->GetEntry() == NPC_PLAGUED_PEASANT))
                    {
                        continue;
                    }

                    pTemp->ForcedDespawn();
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiEventTimer)
            {
                if (m_uiEventTimer <= uiDiff)
                {
                    switch (m_uiPhase)
                    {
                    case 0:
                        DoSummonWave(NPC_SCOURGE_ARCHER);
                        m_uiEventTimer = 5000;
                        break;
                    case 1:
                        DoSummonWave();
                        m_uiEventTimer = urand(60000, 80000);
                        break;
                    default:
                        // The summoning timer of the soldiers isn't very clear
                        DoSummonWave(NPC_SCOURGE_FOOTSOLDIER);
                        m_uiEventTimer = urand(5000, 30000);
                        break;
                    }
                    ++m_uiPhase;
                }
                else
                {
                    m_uiEventTimer -= uiDiff;
                }
            }

            // Handle event end in case of fail
            if (m_uiSadEndTimer)
            {
                if (m_uiSadEndTimer <= uiDiff)
                {
                    DoScriptText(SAY_EVENT_FAIL_2, m_creature);
                    m_creature->ForcedDespawn(5000);
                    DoDespawnSummons();
                    m_uiSadEndTimer = 0;
                }
                else
                {
                    m_uiSadEndTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_eris_havenfireAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_BALANCE_OF_LIGHT_AND_SHADOW)
        {
            if (npc_eris_havenfireAI* pErisAI = dynamic_cast<npc_eris_havenfireAI*>(pCreature->AI()))
            {
                pErisAI->DoStartBalanceEvent(pPlayer);
                return true;
            }
        }

        return false;
    }
};




/*######
## Quest: Battle of Darrowshire
######*/

enum
{
    ALLIANCE_SIDE = 0,

    // ALLIANCE
    NPC_DARROWSHIRE_DEFENDER = 10948,
    NPC_DAVIL_LIGHTFIRE = 10944,
    NPC_SILVER_HAND_DISCIPLE = 10949,

    SCOURGE_SIDE = 1,

    // SCOURGE
    NPC_MARAUDING_SKELETON = 10952,
    NPC_MARAUDING_CORPSE = 10951,
    NPC_HORGUS_THE_RAVAGER = 10946,
    NPC_SERVANT_OF_HORGUS = 10953,
};

struct CreatureLocation
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const CreatureLocation aDarrowshireAllianceLocation[] =
{
    { 1496.91f, -3717.48f, 81.35f, 5.79f },                      // GROUP 1: Alliance spawn point
    { 1492.82f, -3685.71f, 80.41f, 0.45f },                      // GROUP 2: Alliance spawn point
    { 1478.42f, -3661.77f, 81.09f, 0.26f },                      // GROUP 3: Alliance spawn point
    { 1447.79f, -3744.29f, 87.03f, 4.63f },                      // GROUP 4: Alliance spawn point

    { 1480.36f, -3681.40f, 79.06f, 0.11f },                      // David Lightfire spawn point

};

static const CreatureLocation aDarrowshireScourgeLocation[] =
{
    { 1504.72f, -3722.62f, 84.09f, 2.19f },                      // GROUP 1: Scourge spawn point
    { 1506.91f, -3687.94f, 83.14f, 1.63f },                      // GROUP 2: Scourge spawn point
    { 1484.04f, -3653.21f, 85.8f, 3.98f },                       // GROUP 3: Scourge spawn point
    { 1446.32f, -3763.41f, 97.19f, 1.69f },                      // GROUP 4: Scourge spawn point

    { 1522.29f, -3678.4f, 84.2f, 3.32f },                        // Horgus the Ravager
};

// Darrowshire Defender
#define SPELL_CAST_STRIKE 11976
#define SPELL_SHIELD_BLOCK 12169

#define SPELL_SCOURGE_CAST_STRIKE 13584
#define SPELL_CAST_HAMSTRING 11972

// Davil Lightfire
#define SPELL_CAST_HOLY_STRIKE 17284
#define SPELL_CAST_HAMMER_OF_JUSTICE 13005
#define SPELL_CAST_DEVOTION_AURA 17232

// Silver Hand Disciple
// still need to implement the heal spell !!!!!!!!!!!!!!!!!!
#define SPELL_CAST_CRUSADER_STRIKE 14518

// this is used to prevent more than one instance of the event running at the same time
bool EVENT_BATTLE_OF_DARROWSHIRE = false;

uint32 GROUP_TO_SPAWN_TO = 0;

enum
{
    PHASE_1 = 1,    // Marauding Skeletons and Marauding Corpses
    PHASE_2 = 2,    // Davil Lightfire and Silver Hand Disciples (the use of the Relic Bundle starts a timer that will spawn Davil and his adds)
    PHASE_3 = 3,    // Horgus the Ravager and Servants of Horgus (Davil Lightfire spawn starts off a timer that will spawn Horgus and his adds)

    GROUP_0 = 0,
    GROUP_1 = 1,
    GROUP_2 = 2,
    GROUP_3 = 3
};

// PHASE 2
// Daril Lightfire (10944) and Silver Hand Disciples (10949)

uint32 m_uPhase2Timer = 0; // time till Davil Lightfore is spawned
uint32 m_uPhase3Timer = 0; // time till Horgus the Ravager is spawned
uint32 uCurrentPhase = 0;  // see above fot the different phases of the battle




void SpawnScourgeCreature(Unit* pUnit, uint32 NPC_TYPE, uint32 uGroup)
{
    // alter spawn points slightly
    float fSpawnPointX = aDarrowshireScourgeLocation[uGroup].m_fX + rand() % 5;
    float fSpawnPointY = aDarrowshireScourgeLocation[uGroup].m_fY + rand() % 5;

    Creature* pCreature = pUnit->SummonCreature(NPC_TYPE, fSpawnPointX, fSpawnPointY, aDarrowshireScourgeLocation[uGroup].m_fZ, aDarrowshireScourgeLocation[uGroup].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
}

void SpawnAllianceCreature(Unit* pUnit, uint32 NPC_TYPE, uint32 uGroup)
{
    // alter spawn points slightly
    float fSpawnPointX = aDarrowshireAllianceLocation[uGroup].m_fX + rand() % 5;
    float fSpawnPointY = aDarrowshireAllianceLocation[uGroup].m_fY + rand() % 5;

    Creature* pCreature = pUnit->SummonCreature(NPC_TYPE, fSpawnPointX, fSpawnPointY, aDarrowshireAllianceLocation[uGroup].m_fZ, aDarrowshireAllianceLocation[uGroup].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
}


void SpawnScourge(Unit* pUnit, uint32 uGroup)
{
    // Randomly choose the number of Scourge to spawn (2 or 3)
    uint32 uTotalToSpawn = rand() % 2 + 2;
    for (int iCount = 0; iCount < uTotalToSpawn; iCount++)
    {
        // randomly choose which Scourge mob to spawn
        uint32 uScourgeType = rand() % 2;

        switch (uScourgeType)
        {
        case 0:
            SpawnScourgeCreature(pUnit, NPC_MARAUDING_SKELETON, uGroup);
            break;
        default:
            SpawnScourgeCreature(pUnit, NPC_MARAUDING_CORPSE, uGroup);
            break;
        }
    }
}

void SpawnAlliance(Unit* pUnit, uint32 uGroup)
{
    // Randmomly choose the number of Alliance to spawn (2 or 3)
    uint32 uTotalToSpawn = rand() % 2 + 2;
    for (int iCount = 0; iCount < uTotalToSpawn; iCount++)
    {
        SpawnAllianceCreature(pUnit, NPC_DARROWSHIRE_DEFENDER, uGroup);
    }
}


void SpawnFirstWave(Player* pPlayer)
{
    // iterate through the groups
    for (uint32 iGroup = 0; iGroup < 4; iGroup++)
    {
        GROUP_TO_SPAWN_TO = iGroup;
        // Randmomly choose the number of Scourge to spawn (2 or 3)
        uint32 uTotalToSpawn = rand() % 2 + 2;
        for (int iCount = 0; iCount < uTotalToSpawn; iCount++)
        {
            // randomly choose which Scourge mob to spawn
            uint32 uScourgeType = rand() % 2;

            switch (uScourgeType)
            {
            case 0:
                SpawnScourgeCreature(pPlayer, NPC_MARAUDING_SKELETON, iGroup);
                break;
            default:
                SpawnScourgeCreature(pPlayer, NPC_MARAUDING_CORPSE, iGroup);
                break;
            }
        }

        // Randmomly choose the number of Alliance to spawn (2 or 3)
        uTotalToSpawn = rand() % 2 + 2;
        for (int iCount = 0; iCount < uTotalToSpawn; iCount++)
        {
            SpawnAllianceCreature(pPlayer, NPC_DARROWSHIRE_DEFENDER, iGroup);
        }

    }
}




struct go_relic_bundle : public GameObjectScript
{
    go_relic_bundle() : GameObjectScript("go_relic_bundle") {}	
	

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
		// make sure the event is not currently running
		if (!EVENT_BATTLE_OF_DARROWSHIRE)
		{
			GROUP_TO_SPAWN_TO = 1;
			EVENT_BATTLE_OF_DARROWSHIRE = true; // this is set back to false once the player interacts with Redpath
			// Spawn Darrowshire Defender
			Creature* pDarrowshireDefender = pPlayer->SummonCreature(NPC_DARROWSHIRE_DEFENDER, 1444.09f, -3699.77f, 77.30f, 0.47f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 7200000);
			// send him on his merry way
			pDarrowshireDefender->GetMotionMaster()->MovePoint(0, 1451.54f, -3694.28f, 76.76f);
			// yell “Darrowshire, to arms! The Scourge approach!”
			pDarrowshireDefender->MonsterYell("Darrowshire, to arms! The Scourge approach!", LANG_COMMON, NULL);
			// continue on route
			pDarrowshireDefender->GetMotionMaster()->MovePoint(0, 1467.15f, -3686.50f, 77.57f);
			pDarrowshireDefender->GetMotionMaster()->MovePoint(0, 1483.31f, -3678.11f, 79.62f);
			pDarrowshireDefender->GetMotionMaster()->MoveIdle();

			// spawn First Wave (Marauder Skeletons and Marauder Corpses)
			SpawnFirstWave(pPlayer);
		}
		else // output that the event is currently running
			pPlayer->MonsterTextEmote("Cannot do that, as the Battle is currently in progress", pPlayer, false);

		// Start the timer that will spawn David Lightfire + Silver Hand Disciples
		m_uPhase2Timer = 60000; // 1 minute ???
		uCurrentPhase = PHASE_1;
		
		return true;
		
	}
	
};


Creature* LocateEnemy(Creature* m_creature)
{
    switch (m_creature->GetEntry())
    {
    case NPC_MARAUDING_SKELETON:
    case NPC_MARAUDING_CORPSE:
        // get nearest enemy       
        if (Creature* pEnemy = GetClosestCreatureWithEntry(m_creature, NPC_DARROWSHIRE_DEFENDER, 25.0f))
        {
            m_creature->SetFacingToObject(pEnemy);
            return pEnemy;
        }
        else if (Creature* pEnemy = GetClosestCreatureWithEntry(m_creature, NPC_DAVIL_LIGHTFIRE, 25.0f))
        {
            m_creature->SetFacingToObject(pEnemy);
            return pEnemy;
        }
        else if (Creature* pEnemy = GetClosestCreatureWithEntry(m_creature, NPC_SILVER_HAND_DISCIPLE, 25.0f))
        {
            m_creature->SetFacingToObject(pEnemy);
            return pEnemy;
        }
        break;
    case NPC_DARROWSHIRE_DEFENDER:
    case NPC_DAVIL_LIGHTFIRE:
    case NPC_SILVER_HAND_DISCIPLE:
        if (Creature* pEnemy = GetClosestCreatureWithEntry(m_creature, NPC_MARAUDING_SKELETON, 25.0f))
        {
            m_creature->SetFacingToObject(pEnemy);
            return pEnemy;
        }
        else if (Creature* pEnemy = GetClosestCreatureWithEntry(m_creature, NPC_MARAUDING_CORPSE, 25.0f))
        {
            m_creature->SetFacingToObject(pEnemy);
            return pEnemy;
        }
        break;
    }

    return NULL; // enemy not found
}


// This assigns a recently spawned creature to a group, based on it proximity to the spawn locations of the group to its currently location.
// It will fight as part of that group.
uint32 AssignCreatureToGroup(Unit* m_creature)
{
    float fCreatureX;
    float fCreatureY;
    float fCreatureZ;
    m_creature->GetPosition(fCreatureX, fCreatureY, fCreatureZ);

    if (m_creature->GetEntry() == NPC_DARROWSHIRE_DEFENDER)
    {
        // Group 0
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireAllianceLocation[0].m_fX, aDarrowshireAllianceLocation[0].m_fY, aDarrowshireAllianceLocation[0].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_0;
        }

        // Group 1
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireAllianceLocation[1].m_fX, aDarrowshireAllianceLocation[1].m_fY, aDarrowshireAllianceLocation[1].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_1;
        }

        // Group 2
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireAllianceLocation[2].m_fX, aDarrowshireAllianceLocation[2].m_fY, aDarrowshireAllianceLocation[2].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_2;
        }

        // Group 3
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireAllianceLocation[3].m_fX, aDarrowshireAllianceLocation[3].m_fY, aDarrowshireAllianceLocation[3].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_3;
        }

    }
    else // Scourge creature
    {
        // Group 0
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireScourgeLocation[0].m_fX, aDarrowshireScourgeLocation[0].m_fY, aDarrowshireScourgeLocation[0].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_0;
        }
        // Group 1
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireScourgeLocation[1].m_fX, aDarrowshireScourgeLocation[1].m_fY, aDarrowshireScourgeLocation[1].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_1;
        }
        // Group 2
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireScourgeLocation[2].m_fX, aDarrowshireScourgeLocation[2].m_fY, aDarrowshireScourgeLocation[2].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_2;
        }
        // Group 3
        if (m_creature->IsNearWaypoint(fCreatureX, fCreatureY, fCreatureZ,
            aDarrowshireScourgeLocation[3].m_fX, aDarrowshireScourgeLocation[3].m_fY, aDarrowshireScourgeLocation[3].m_fZ,
            15, 15, 15)) // area to search from creature's location
        {
            return GROUP_3;
        }

    }

    return GROUP_1; // set to the 3rd group by default

}

void SpawnSingleCreatureInEachGroup(Unit* pUnit)
{
    if (pUnit->GetEntry() == NPC_DARROWSHIRE_DEFENDER)
    {
        for (int iGroup = 0; iGroup < 4; iGroup++)
            SpawnScourge(pUnit, iGroup);
    }
    else
    {
        for (int iGroup = 0; iGroup < 4; iGroup++)
            SpawnAlliance(pUnit, iGroup);
    }
}

struct npc_darrowshire_defender : public CreatureScript
{
    npc_darrowshire_defender() : CreatureScript("npc_darrowshire_defender") {}

    struct npc_darrowshire_defenderAI : public ScriptedAI
    {
        npc_darrowshire_defenderAI(Creature* pCreature) : ScriptedAI(pCreature)
		{
			Reset();
		}

		uint32 uGroup;

		void Reset() override
		{
			// assign creature to group
			uGroup = AssignCreatureToGroup(m_creature);
		}

		void UpdateAI(const uint32 uiDiff)
		{
			if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
			{
				// locate another enemy
				Creature* pEnemy = LocateEnemy(m_creature);
				if (pEnemy)
				{
					// Move to and engage enemy in combat
	//				m_creature->SetWalk(false, true); // run!
	//				m_creature->GetMotionMaster()->MovePoint(0, pEnemy->GetPositionX() - 1, pEnemy->GetPositionY() - 1, pEnemy->GetPositionZ());
				}
				else // no enemies found
				{
					uint32 uSpawn = rand() % 50;
					// spawn Scourge
					if (uSpawn == 0)
					{
						uSpawn = rand() % 10;
						// spawn Scourge
						if (uSpawn == 0)
						{
							SpawnSingleCreatureInEachGroup(m_creature);
						}
						SpawnScourge(m_creature, uGroup);
					}

				}

				return;
			}
			
			if (m_creature->isAttackReady())
			{
				uint32 uiAttackAnilityToUse = rand() % 4;
				if (uiAttackAnilityToUse == 0) // use shield block about 25% of the time
				{
					m_creature->CastSpell(m_creature->getVictim(), SPELL_SHIELD_BLOCK, true);
				}
				else
					m_creature->CastSpell(m_creature->getVictim(), SPELL_CAST_STRIKE, true);
				m_creature->resetAttackTimer();
			}

			// Phase 2 - spawning of David Lightfire
			if (uCurrentPhase == PHASE_1 && uGroup == GROUP_1) // only allow group 1 to be in control of the spawning of Lightfire
			{
				// has the 1 minute timer expired?
				if (m_uPhase2Timer < uiDiff)
				{
					// spawn David Lightfire
					m_creature->SummonCreature(NPC_DAVIL_LIGHTFIRE, aDarrowshireAllianceLocation[4].m_fX, aDarrowshireAllianceLocation[4].m_fY, aDarrowshireAllianceLocation[4].m_fZ, aDarrowshireAllianceLocation[4].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 7200000);
					uCurrentPhase = PHASE_2;
				}
				else
				{
					m_uPhase2Timer -= uiDiff;
				}
			}
		}
		
	};

	
    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_darrowshire_defenderAI(pCreature);
    }
	
};

struct npc_marauding_scourge : public CreatureScript
{
    npc_marauding_scourge() : CreatureScript("npc_marauding_scourge") {}

    struct npc_marauding_scourgeAI : public ScriptedAI
    {
        npc_marauding_scourgeAI(Creature* pCreature) : ScriptedAI(pCreature)
		{
			Reset();
		}

		uint32 uGroup;

		void Reset() override
		{
			// assign creature to group
			uGroup = AssignCreatureToGroup(m_creature);
		}

		void UpdateAI(const uint32 uiDiff)
		{
			if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
			{
				// locate another enemy
				Creature* pEnemy = LocateEnemy(m_creature);
				if (pEnemy)
				{
					// Move to and engage enemy in combat
		//			m_creature->SetWalk(false, true); // run!
		//			m_creature->GetMotionMaster()->MovePoint(0, pEnemy->GetPositionX() - 1, pEnemy->GetPositionY() - 1, pEnemy->GetPositionZ());
				}
				else // no enemies found
				{
					uint32 uSpawn = rand() % 50;
					// spawn Scourge
					if (uSpawn == 0)
					{
						uSpawn = rand() % 10;
						// spawn Scourge
						if (uSpawn == 0)
						{
							SpawnSingleCreatureInEachGroup(m_creature);
						}
						SpawnAlliance(m_creature, uGroup);
					}

				}

				return;
			}
			
			if (m_creature->isAttackReady())
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_CAST_STRIKE, true);
				m_creature->resetAttackTimer();
			}
			
		}
		
	};

	
    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_marauding_scourgeAI(pCreature);
    }
	
};



void AddSC_eastern_plaguelands()
{
    Script* s;
    s = new npc_eris_havenfire();
    s->RegisterSelf();
    s = new go_relic_bundle();
    s->RegisterSelf();
    s = new npc_darrowshire_defender();
    s->RegisterSelf();
    s = new npc_marauding_scourge();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_eris_havenfire";
    //pNewScript->GetAI = &GetAI_npc_eris_havenfire;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_eris_havenfire;
    //pNewScript->RegisterSelf();
}
