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
 * SDName:      Stranglethorn_Vale
 * SD%Complete: 100
 * SDComment:   Quest support: 592.
 * SDCategory:  Stranglethorn Vale
 * EndScriptData
 */

/**
 * ContentData
 * mob_yenniku
 * EndContentData
 */

#include "precompiled.h"

/*######
## mob_yenniku
######*/

enum
{
    SPELL_YENNIKUS_RELEASE      = 3607,

    QUEST_ID_SAVING_YENNIKU     = 592,

    FACTION_ID_HORDE_GENERIC    = 83,                       // Note: faction may not be correct!
};

struct mob_yenniku : public CreatureScript
{
    mob_yenniku() : CreatureScript("mob_yenniku") {}

    struct mob_yennikuAI : public ScriptedAI
    {
        mob_yennikuAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiResetTimer;

        void Reset() override { m_uiResetTimer = 0; }

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pSpell->Id == SPELL_YENNIKUS_RELEASE && pCaster->GetTypeId() == TYPEID_PLAYER)
            {
                if (!m_uiResetTimer && ((Player*)pCaster)->GetQuestStatus(QUEST_ID_SAVING_YENNIKU) == QUEST_STATUS_INCOMPLETE)
                {
                    m_uiResetTimer = 60000;
                    EnterEvadeMode();
                }
            }
        }

        void EnterEvadeMode() override
        {
            if (m_uiResetTimer)
            {
                m_creature->RemoveAllAurasOnEvade();
                m_creature->DeleteThreatList();
                m_creature->CombatStop(true);
                m_creature->LoadCreatureAddon(true);

                m_creature->SetLootRecipient(NULL);

                m_creature->HandleEmote(EMOTE_STATE_STUN);
                m_creature->SetFactionTemporary(FACTION_ID_HORDE_GENERIC, TEMPFACTION_RESTORE_REACH_HOME);
            }
            else
            {
                ScriptedAI::EnterEvadeMode();
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiResetTimer)
            {
                if (m_uiResetTimer <= uiDiff)
                {
                    m_creature->HandleEmote(EMOTE_STATE_NONE);
                    m_uiResetTimer = 0;
                    EnterEvadeMode();
                }
                else
                {
                    m_uiResetTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* _Creature) override
    {
        return new mob_yennikuAI(_Creature);
    }
};

/*######
## Night time event where the creatures sleep
######*/

#define SPELL_CLAW 16832 

struct mob_creature : public CreatureScript
{        
    mob_creature() : CreatureScript("mob_creature") {}

    struct mob_creatureAI : public ScriptedAI
    {
        mob_creatureAI(Creature* pCreature) : ScriptedAI(pCreature) { }        

        void Reset() override {  }

        void UpdateAI(const uint32 uiDiff) 
        {
             DEBUG_LOG("******************    UPDATE       ****************************");
			Map::PlayerList const& players = m_creature->GetMap()->GetPlayers();
			for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
			{
				if (Player* pPlayer = itr->getSource())
				{
					// Acquire player's coordinates
					float fPlayerXposition = pPlayer->GetPositionX();
					float fPlayerYposition = pPlayer->GetPositionY();
					float fPlayerZposition = pPlayer->GetPositionZ();
             DEBUG_LOG("Looking for players");
					// Check if player is near the creature
					// If a player is near, then we do not need to check other player locations, therefore stop checking - break out of this
					if (pPlayer->IsNearWaypoint(fPlayerXposition, fPlayerYposition, fPlayerZposition, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 4, 4, 4))
					{
		//				m_creature->clearUnitState(UNIT_STAT_CAN_NOT_MOVE);
						m_creature->SetStandState(UNIT_STAND_STATE_STAND);
     //                   m_creature->addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
						m_creature->GetMotionMaster()->MoveChase(pPlayer);
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
             DEBUG_LOG("*************************** PLAYER IS NEARBY ************************");
						return;
					}
				}
			}
            
	    // go to sleep if it is night time
    //        time_t t = m_creature->GetGameTime();
            time_t t = sWorld.GetGameTime();
            struct tm *tmp = gmtime(&t);
            if (!m_creature->IsDying() && tmp->tm_hour > 5)
            {
                m_creature->SetStandState(UNIT_STAND_STATE_SLEEP);
            //    m_creature->addUnitState(UNIT_STAT_ROOT);
      //          m_creature->addUnitState(UNIT_STAT_CAN_NOT_MOVE);
				m_creature->GetMotionMaster()->MoveIdle();
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
                return;

            if (m_creature->isAttackReady())
            {
  //              m_creature->clearUnitState(UNIT_STAT_CAN_NOT_MOVE);
  //              m_creature->addUnitState(UNIT_STAT_MOVING);
  //              m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                m_creature->CastSpell(m_creature->getVictim(), SPELL_CLAW, true);
                m_creature->resetAttackTimer();
            }

        }

    };

		
    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new mob_creatureAI(pCreature);
    }
};


/*######
##
######*/

void AddSC_stranglethorn_vale()
{
    Script* s;
    s = new mob_yenniku();
    s->RegisterSelf();
    s = new mob_creature();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "mob_yenniku";
    //pNewScript->GetAI = &GetAI_mob_yenniku;
    //pNewScript->RegisterSelf();
}
