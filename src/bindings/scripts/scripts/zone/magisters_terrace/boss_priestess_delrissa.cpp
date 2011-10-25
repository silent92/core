/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Priestess_Delrissa
SD%Complete: 85
SDComment: Verify timers, long and tiring debugging :)
SDCategory: Magister's Terrace
EndScriptData */

#include "precompiled.h"
#include "def_magisters_terrace.h"

struct Speech
{
    int32 id;
};

static Speech LackeyDeath[]=
{
    {-1585013},
    {-1585014},
    {-1585015},
    {-1585016},
};

static Speech PlayerDeath[]=
{
    {-1585017},
    {-1585018},
    {-1585019},
    {-1585020},
    {-1585021},
};

#define SAY_AGGRO                   -1585012
#define SAY_DEATH                   -1585022

#define SPELL_DISPEL_MAGIC          27609
#define SPELL_FLASH_HEAL            17843
#define SPELL_PHYSIC_SCREAM         27610
#define SPELL_WEAKENED_SOUL         6788
#define SPELL_SHIELD                (HeroicMode?46193:44175)
#define SPELL_SW_PAIN               (HeroicMode?15654:14032)
#define SPELL_RENEW                 (HeroicMode?46192:44174)
#define SPELL_MEDALION_OF_IMMUNITY  46227

#define ORIENT                      4.98
#define POS_Z                       -19.9215

float LackeyLocations[4][2]=
{
    {123.77, 17.6007},
    {131.731, 15.0827},
    {121.563, 15.6213},
    {129.988, 17.2355},
};

const uint32 AddEntry[8]=
{
    24557,                                                  //Kagani Nightstrike
    24558,                                                  //Elris Duskhallow
    24554,                                                  //Eramas Brightblaze
    24561,                                                  //Yazzaj
    24559,                                                  //Warlord Salaris
    24555,                                                  //Garaxxas
    24553,                                                  //Apoko
    24556,                                                  //Zelfan
};

struct Add
{
    Add(uint32 _entry, uint64 _guid)
    {
        entry = _entry;
        guid  = _guid;
    }

    uint32 entry;
    uint64 guid;
};

struct TRINITY_DLL_DECL boss_priestess_delrissaAI : public ScriptedAI
{
    boss_priestess_delrissaAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = (c->GetInstanceData());
        me->GetPosition(wLoc);
        Adds.clear();
    }

    ScriptedInstance* pInstance;
    WorldLocation wLoc;

    std::vector<Add> Adds;

    uint8 LackeysKilled;
    uint8 PlayersKilled;

    bool canFear;
    bool canUseMedalion;
    bool aggroSpeach;

    uint32 Heal_Timer;
    uint32 Renew_Timer;
    uint32 Scream_Cooldown;
    uint32 Medalion_Cooldown;
    uint32 Shield_Timer;
    uint32 SWPain_Timer;
    uint32 Dispel_Timer;
    uint32 Check_Timer;

    void Reset()
    {
        m_creature->SetCorpseDelay(60*60*1000);
        LackeysKilled = 0;
        PlayersKilled = 0;

        canFear = true;
        canUseMedalion = true;
        aggroSpeach = false;

        Heal_Timer   = 15000;
        Renew_Timer  = 10000;
        Scream_Cooldown = HeroicMode?15000:30000;
        Medalion_Cooldown = 30000;
        Shield_Timer = 2000;
        SWPain_Timer = 5000;
        Dispel_Timer = 7500;
        Check_Timer = 2000;

        CheckAdds();

        if(pInstance)
        {
            pInstance->SetData(DATA_DELRISSA_DEATH_COUNT, 0);
            pInstance->SetData(DATA_DELRISSA_EVENT, NOT_STARTED);
        }
    }

    void EnterCombat(Unit* who)
    {
        if(!aggroSpeach)
        {
            DoScriptText(SAY_AGGRO, me);
            aggroSpeach= true;
        }

        for(uint8 i = 0; i < Adds.size(); ++i)
            if(Unit* pAdd = m_creature->GetUnit(Adds[i].guid))
                pAdd->AddThreat(who, 1.0f);
    }

    void SummonAdds()
    {
        std::set<uint32> AddList;

        while(AddList.size() < 4)
        {
            uint8 i = urand(0, 7); 
            if(AddList.find(AddEntry[i]) == AddList.end())
                AddList.insert(AddEntry[i]);
        }

        uint8 i = 0;

        for(std::set<uint32>::iterator iter = AddList.begin(); iter != AddList.end(); ++iter)
        {
            Creature* pAdd = m_creature->SummonCreature((*iter), LackeyLocations[i][0], LackeyLocations[i][1], POS_Z, ORIENT, TEMPSUMMON_DEAD_DESPAWN, 0);
            if(pAdd)
            {
                Add nAdd((*iter), pAdd->GetGUID());
                Adds.push_back(nAdd);
                ++i;
            }
        }
    }

    void CheckAdds()
    {
        if(Adds.empty())
        {
            SummonAdds();
            return;
        }
        for(uint8 i = 0; i < Adds.size(); ++i)
        {
            Creature* pAdd = m_creature->GetCreature(Adds[i].guid);
            if(pAdd && pAdd->isAlive())
            {
                pAdd->AI()->EnterEvadeMode();
                pAdd->GetMotionMaster()->MovePoint(0,LackeyLocations[i][0], LackeyLocations[i][1], POS_Z);
            }
            if(!pAdd || (pAdd && pAdd->isDead()))
            {
                if(pAdd)
                    pAdd->RemoveCorpse();//looks stupid if mob is alive but has a dead corpse in front of him :)
                Creature* pAdd = m_creature->SummonCreature(Adds[i].entry, LackeyLocations[i][0], LackeyLocations[i][1], POS_Z, ORIENT, TEMPSUMMON_DEAD_DESPAWN, 0);
                if(pAdd)
                    Adds[i].guid = pAdd->GetGUID();
            }
        }
    }

    void KilledUnit(Unit* victim)
    {
        if(victim->GetTypeId() != TYPEID_PLAYER)
            return;

        DoScriptText(PlayerDeath[PlayersKilled].id, m_creature);
        if( PlayersKilled < 4 )
            ++PlayersKilled;
    }

    void KilledLackey()
    {
        if(m_creature->isAlive())
        {
            DoScriptText(LackeyDeath[LackeysKilled].id, m_creature);
            if(LackeysKilled < 3)
                ++LackeysKilled;
        }
        CheckLootable();
    }

    void JustDied(Unit* killer)
    {
        DoScriptText(SAY_DEATH, m_creature);
        CheckLootable();
        if(pInstance)
            pInstance->SetData(DATA_DELRISSA_DEATH_COUNT, 1);
    }

    void CheckLootable()
    {
        if(pInstance && pInstance->GetData(DATA_DELRISSA_DEATH_COUNT) >= 4)
        {
            m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            pInstance->SetData(DATA_DELRISSA_EVENT, DONE);
        }
        else
            m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
    }

    Unit* SelectLowestHpFriendlyMissingBuff(float range, uint32 spellid)
    {
        std::list<Creature*> missingBuff = FindFriendlyMissingBuff(range, spellid);
        float healthPct = 100;
        float newHealthPct;
        Unit* target = NULL;

        for(std::list<Creature*>::iterator it = missingBuff.begin(); it != missingBuff.end(); ++it)
        {
            Creature *c = *it;
            newHealthPct = ((float)c->GetHealth()) / c->GetMaxHealth();
            if(c->isAlive() && !c->HasAura(spellid, 0) && newHealthPct < healthPct)
            {
                healthPct = newHealthPct;
                target = c;
            }
        }
        return target;
    }

    void RegenMana()
    {
        uint32 maxMana = me->GetMaxPower(POWER_MANA);
        uint32 Mana = me->GetPower(POWER_MANA);
        me->SetPower(POWER_MANA, Mana+(HeroicMode?0.08:0.05)*maxMana);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(Check_Timer < diff)
        {
            DoZoneInCombat();
            RegenMana();
            if(!me->IsWithinDistInMap(&wLoc, 60.0))
            {
                EnterEvadeMode();
                return;
            }
            if(canFear && me->IsWithinMeleeRange(me->getVictim()))
            {
                ForceSpellCast(SPELL_PHYSIC_SCREAM, CAST_SELF, INTERRUPT_AND_CAST);
                canFear = false;
            }
            Check_Timer = 2000;
        }
        else
            Check_Timer -= diff;

        if(!canFear)
        {
            if(Scream_Cooldown < diff)
            {
                canFear = true;
                Scream_Cooldown = HeroicMode?15000:30000;
            }
            else
                Scream_Cooldown -= diff;
        }

        if(HeroicMode)
        {
            if(canUseMedalion)
            {
                DoCast(me, SPELL_MEDALION_OF_IMMUNITY, true);
                canUseMedalion = false;
            }
            else
            {
                if(Medalion_Cooldown < diff)
                {
                    canUseMedalion = true;
                    Medalion_Cooldown = 30000;
                }
                else
                    Medalion_Cooldown -= diff;
            }
        }

        if(Heal_Timer < diff)
        {
            if(Unit* target = SelectLowestHpFriendly(40, 200))
            {
                AddSpellToCast(target, SPELL_FLASH_HEAL);
                if(target->GetHealth() * 100 / target->GetMaxHealth() < 20)
                    Heal_Timer = 3000;
                else
                    Heal_Timer = urand(4000, 10000);
            }
            else
                Heal_Timer = 2000;
        }
        else
            Heal_Timer -= diff;

        if(Renew_Timer < diff)
        {
            if(Unit *target = SelectLowestHpFriendlyMissingBuff(40, SPELL_RENEW))
            {
                AddSpellToCast(target, SPELL_RENEW);
                if(target->GetHealth() * 100 / target->GetMaxHealth() < 20)
                    Renew_Timer = 4000;
                else
                    Renew_Timer = 8000;
            }
            else
                Renew_Timer = 1000;
        }
        else
            Renew_Timer -= diff;

        if(Shield_Timer < diff)
        {
            if(Unit *target = SelectLowestHpFriendlyMissingBuff(40, SPELL_WEAKENED_SOUL))
            {
                ForceSpellCast(target, SPELL_SHIELD, INTERRUPT_AND_CAST);
                Shield_Timer = 15000;
            }
            else
                Shield_Timer = 1000;
        }
        else
            Shield_Timer -= diff;

        if(Dispel_Timer < diff)
        {
            Unit* target = NULL;
            std::list<Creature*> friendlyCC;
            switch(rand()%3)
            {
                case 0:
                    target = SelectUnit(SELECT_TARGET_RANDOM, 0, 30, true);
                    if(target)
                        break;
                case 1:
                    friendlyCC = FindFriendlyCC(30);
                    if(!friendlyCC.empty())
                    {
                        target = *friendlyCC.begin();
                        break;
                    }
                case 2:
                    target = SelectLowestHpFriendly(30);
                    break;
            }

            if(target)
            {
                AddSpellToCast(target, SPELL_DISPEL_MAGIC);
                Dispel_Timer = 10000;
            }
            else
                Dispel_Timer = 1000;
        }
        else
            Dispel_Timer -= diff;

        if(SWPain_Timer < diff)
        {
            SWPain_Timer = 1000;
            if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30, true, -SPELL_SW_PAIN))
            {
                if(!target->hasUnitState(UNIT_STAT_LOST_CONTROL))
                {
                    AddSpellToCast(target,SPELL_SW_PAIN);
                    SWPain_Timer = 10000;
                }
            }
        }
        else
            SWPain_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

#define SPELL_HEALING_POTION        15503

struct TRINITY_DLL_DECL boss_priestess_guestAI : public ScriptedAI
{
    boss_priestess_guestAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = (c->GetInstanceData());
    }

    ScriptedInstance* pInstance;

    bool resetThreat;
    bool canUseMedalion;
    bool usedPotion;
    uint32 ResetThreatTimer;
    uint32 Medalion_Cooldown;
    float targetRange;

    void Reset()
    {
        usedPotion = false;
        resetThreat = true;
        canUseMedalion = true;
        ResetThreatTimer = urand(8000, 20000);             // These guys like to switch targets often, and are not meant to be tanked.
        Medalion_Cooldown = 30000;
        targetRange = 100;

        if(pInstance)
        {
            if(Creature *boss = m_creature->GetCreature(pInstance->GetData64(DATA_DELRISSA)))
            {
                if(boss->isDead())
                    boss->Respawn();
            }
        }
    }

    void EnterCombat(Unit* who)
    {
        DoZoneInCombat();
        if(pInstance)
        {
            if(Creature* Delrissa = m_creature->GetCreature(pInstance->GetData64(DATA_DELRISSA)))
                ((boss_priestess_delrissaAI*)Delrissa->AI())->EnterCombat(who);
        }
    }

    void JustDied(Unit* killer)
    {
        if(pInstance)
        {
            pInstance->SetData(DATA_DELRISSA_DEATH_COUNT, 1);
            if(Creature* Delrissa = m_creature->GetCreature(pInstance->GetData64(DATA_DELRISSA)))
                ((boss_priestess_delrissaAI*)Delrissa->AI())->KilledLackey();
        }
    }

    void KilledUnit(Unit* victim)
    {
        if(pInstance)
        {
            if(Creature* Delrissa = m_creature->GetCreature(pInstance->GetData64(DATA_DELRISSA)))
                Delrissa->AI()->KilledUnit(victim);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(HealthBelowPct(25) && !usedPotion)
        {
            AddSpellToCast(m_creature, SPELL_HEALING_POTION);
            usedPotion = true;
        }

        if(HeroicMode)
        {
            if(canUseMedalion)
            {
                DoCast(me, SPELL_MEDALION_OF_IMMUNITY, true);
                canUseMedalion = false;
            }
            else
            {
                if(Medalion_Cooldown < diff)
                {
                    canUseMedalion = true;
                    Medalion_Cooldown = 30000;
                }
                else
                    Medalion_Cooldown -= diff;
            }
        }

        if(resetThreat)
        {
            if(ResetThreatTimer < diff)
            {
                DoResetThreat();
                if(Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 0, targetRange, true))
                {
                    AttackStart(target);
                    me->AddThreat(target, 10000);
                }
                ResetThreatTimer = urand(8000, 20000);
            }
            else
                ResetThreatTimer -= diff;
        }
    }
};

#define SPELL_GOUGE              12540
#define SPELL_KICK               27613
#define SPELL_KIDNEY_SHOT        27615
#define SPELL_VANISH             44290
#define SPELL_BACKSTAB           (HeroicMode?15582:15657)
#define SPELL_EVISCERATE         (HeroicMode?46189:27611)
#define SPELL_DUALWIELD          29651
#define SPELL_CRIPPLING_POISON   44289

struct TRINITY_DLL_DECL boss_kagani_nightstrikeAI : public boss_priestess_guestAI
{
    //Rogue
    boss_kagani_nightstrikeAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 Gouge_Timer;
    uint32 Kick_Cooldown;
    uint32 Vanish_Timer;
    uint32 Eviscerate_Timer;
    uint32 Backstab_Timer;
    uint32 Check_Timer;

    bool canKick;
    bool InVanish;

    void Reset()
    {
        boss_priestess_guestAI::Reset();
        Gouge_Timer = 5500;
        Kick_Cooldown = 7000;
        Vanish_Timer = urand(5000, 15000);
        Eviscerate_Timer = urand(6000, 12000);
        Backstab_Timer = 3000;
        Check_Timer = 0;
        canKick = true;
        InVanish = false;
        resetThreat = false;

        DoCast(m_creature, SPELL_DUALWIELD);
    }

    void DamageMade(Unit* target, uint32 & damage, bool direct_damage)
    {
        if(target->HasAura(SPELL_CRIPPLING_POISON, 0))
            return;

        if(damage && direct_damage && roll_chance_f(50))
            DoCast(target, SPELL_CRIPPLING_POISON, true);
    }

    void OnAuraRemove(Aura *aur, bool stackRemove)
    {
        if(aur->GetId() == 26888)
            InVanish = false;
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(InVanish)
        {
            if(Check_Timer < diff)
            {
                if(me->IsWithinMeleeRange(me->getVictim()))
                {
                    if(!me->getVictim()->HasInArc(M_PI, me))
                        AddSpellToCast(me->getVictim(), SPELL_BACKSTAB);

                    AddSpellToCast(SPELL_KIDNEY_SHOT, CAST_TANK);
                    me->RemoveAurasDueToSpell(SPELL_VANISH);
                    Gouge_Timer = urand(10000, 15000);
                }
                Check_Timer = 2000;
            }
            else
                Check_Timer -= diff;

            CastNextSpellIfAnyAndReady();
            return;
        }

        boss_priestess_guestAI::UpdateAI(diff);

        if(Vanish_Timer < diff)
        {
            ForceSpellCast(me, SPELL_VANISH, INTERRUPT_AND_CAST);
            DoResetThreat();
            if(Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 0, 100, true))
            {
                float x, y, z;
                me->GetNearPoint(target, x, y, z, 0, 5.0, frand(0, 2*M_PI));
                AttackStart(target);
                me->AddThreat(target, 10000);
                me->GetMotionMaster()->MovePoint(0, x, y, z);
            }
            InVanish = true;
            Check_Timer = 2000;
            Vanish_Timer = urand(20000, 30000);
        }
        else
            Vanish_Timer -= diff;

        if(!canKick)
        {
            if(Kick_Cooldown < diff)
            {
                canKick = true;
                Kick_Cooldown = urand(15000, 18000);
            }
            else
                Kick_Cooldown -= diff;
        }

        if(canKick && (me->getVictim()->IsNonMeleeSpellCasted(true) || roll_chance_f(15.0)))
        {
            if(me->IsWithinMeleeRange(me->getVictim()))
            {
                ForceSpellCast(SPELL_KICK, CAST_TANK);
                canKick = false;
            }
        }

        if(Backstab_Timer < diff)
        {
            if(!m_creature->getVictim()->HasInArc(M_PI, m_creature))
                ForceSpellCast(SPELL_BACKSTAB, CAST_TANK, INTERRUPT_AND_CAST);
            Backstab_Timer = 3000;
        }
        else
            Backstab_Timer -= diff;

        if(Gouge_Timer < diff)
        {
            AddSpellToCast(SPELL_GOUGE, CAST_TANK);
            //DoModifyThreatPercent(m_creature->getVictim(),-100);
            Gouge_Timer = urand(12000, 25000);
        }
        else
            Gouge_Timer -= diff;

        if(Eviscerate_Timer < diff)
        {
            AddSpellToCast(SPELL_EVISCERATE, CAST_TANK);
            Eviscerate_Timer = urand(4000, 10000);
        }
        else
            Eviscerate_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

#define SPELL_CURSE_OF_AGONY         (HeroicMode?46190:14875)
#define SPELL_IMMOLATE               (HeroicMode?46191:44267)
#define SPELL_SHADOW_BOLT            (HeroicMode?15232:12471)
#define SPELL_SEED_OF_CORRUPTION     44141
#define SPELL_DEATH_COIL             44142
#define SPELL_FEAR                   38595

#define SPELL_SUMMON_IMP             44163
#define SPELL_IMP_FIREBALL           44164

#define NPC_FIZZLE                   24656

enum EllrisSpec
{
    SPEC_SHADOW = 0,
    SPEC_FIRE   = 1
};

struct TRINITY_DLL_DECL boss_ellris_duskhallowAI : public boss_priestess_guestAI
{
    //Warlock
    boss_ellris_duskhallowAI(Creature *c) : boss_priestess_guestAI(c)
    {
        spec = urand(0,1);
    }

    uint8 spec;
    uint32 Check_Timer;
    uint32 SummonImp_Timer;
    uint32 Seed_of_Corruption_Timer;
    uint32 Curse_of_Agony_Timer;
    uint32 Fear_Timer;

    void Reset()
    {
        Check_Timer = 2000;
        SummonImp_Timer = 5000;
        Seed_of_Corruption_Timer = 2000;
        Curse_of_Agony_Timer = 1000;
        Fear_Timer = 10000;

        boss_priestess_guestAI::Reset();
    }

    void AttackStart(Unit* who)
    {
        ScriptedAI::AttackStartNoMove(who);
        switch(spec)
        {
            case SPEC_FIRE:
                SetAutocast(SPELL_IMMOLATE, 1900, true);
                break;
            case SPEC_SHADOW:
                SetAutocast(SPELL_SHADOW_BOLT, 2900, true);
                break;
        }
    }

    void JustDied(Unit* killer)
    {
        boss_priestess_guestAI::JustDied(killer);
    }

    void RegenMana()
    {
        uint32 maxMana = me->GetMaxPower(POWER_MANA);
        uint32 Mana = me->GetPower(POWER_MANA);
        me->SetPower(POWER_MANA, Mana+0.02*maxMana);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!me->isInCombat())
        {
            if(SummonImp_Timer < diff)
            {
                // check if still having pet nearby;]
                Unit* Fizzle = FindCreature(NPC_FIZZLE, 60.0, me);

                if(!Fizzle)
                    DoCast(m_creature, SPELL_SUMMON_IMP, false);

                if(Fizzle && !Fizzle->isAlive())
                {
                    ((Creature*)Fizzle)->RemoveCorpse();
                    DoCast(m_creature, SPELL_SUMMON_IMP, false);
                }
                SummonImp_Timer = 15000;
            }
            else
                SummonImp_Timer -= diff;
        }

        if(!UpdateVictim())
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(Check_Timer < diff)
        {
            if(me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                if(!me->IsWithinDistInMap(me->getVictim(), 29))
                    me->GetMotionMaster()->MoveChase(me->getVictim(), 28.0);
            }
            else
            {
                if(me->IsWithinDistInMap(me->getVictim(), 30))
                    me->GetMotionMaster()->MoveIdle();
            }
            RegenMana();
            Check_Timer = 2000;
        }
        else
            Check_Timer -= diff;

        if(Seed_of_Corruption_Timer < diff)
        {
            AddSpellToCast(SPELL_SEED_OF_CORRUPTION, CAST_RANDOM);
            Seed_of_Corruption_Timer = 10000;
        }
        else
            Seed_of_Corruption_Timer -= diff;

        if(Curse_of_Agony_Timer < diff)
        {
            AddSpellToCast(SPELL_CURSE_OF_AGONY, CAST_TANK);
            Curse_of_Agony_Timer = urand(12000, 15000);
        }
        else
            Curse_of_Agony_Timer -= diff;

        if(Fear_Timer < diff)
        {
            AddSpellToCast(SPELL_FEAR, CAST_RANDOM);
            Fear_Timer = urand(8000, 12000);
        }
        else
            Fear_Timer -= diff;

        CastNextSpellIfAnyAndReady(diff);
    }
};

struct TRINITY_DLL_DECL mob_fizzleAI : public ScriptedAI
{
    mob_fizzleAI(Creature *c) : ScriptedAI(c) { }

    void Reset() { }

    void AttackStart(Unit* who)
    {
        ScriptedAI::AttackStartNoMove(who);
        SetAutocast(SPELL_IMP_FIREBALL, 1900, true);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

      CastNextSpellIfAnyAndReady(diff);
    }
};

#define SPELL_KNOCKDOWN            (HeroicMode?46183:11428)
#define SPELL_SNAP_KICK            46182
#define SPELL_FISTS_OF_ARCANE_FURY 44120
#define SPELL_CHACRA_DRAIN         44121

struct TRINITY_DLL_DECL boss_eramas_brightblazeAI : public boss_priestess_guestAI
{
    //Monk
    boss_eramas_brightblazeAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 Knockdown_Timer;
    uint32 Snap_Kick_Timer;
    uint32 ChacraDrain_Timer;

    void Reset()
    {
        Knockdown_Timer = urand(10000, 20000);
        Snap_Kick_Timer = urand(4000, 8000);
        ChacraDrain_Timer = urand(10000, 20000);

        boss_priestess_guestAI::Reset();
    }

    void DamageMade(Unit* target, uint32 & damage, bool direct_damage)
    {
        if(damage && direct_damage && roll_chance_f(50))
            AddSpellToCast(target, SPELL_FISTS_OF_ARCANE_FURY, true);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim() )
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(Knockdown_Timer < diff)
        {
            if(me->IsWithinMeleeRange(me->getVictim()))
            {
                AddSpellToCast(SPELL_KNOCKDOWN, CAST_TANK);
                Knockdown_Timer = 6000;
            }
        }
        else
            Knockdown_Timer -= diff;

        if(Snap_Kick_Timer < diff)
        {
            if(me->IsWithinMeleeRange(me->getVictim()))
            {
                AddSpellToCast(SPELL_SNAP_KICK, CAST_TANK);
                Snap_Kick_Timer  = 12000;
            }
        }
        else
            Snap_Kick_Timer -= diff;

        if(ChacraDrain_Timer < diff)
        {
            if(HeroicMode)
                AddSpellToCast(SPELL_CHACRA_DRAIN, CAST_RANDOM);
            ChacraDrain_Timer = urand(10000, 20000);
        }
        else
            ChacraDrain_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

#define SPELL_POLYMORPH             13323
#define SPELL_ICE_BLOCK             27619
#define SPELL_FROST_NOVA            44177
#define SPELL_BLINK                 14514
#define SPELL_BLIZZARD              (HeroicMode?46195:44178)
#define SPELL_ICE_LANCE             (HeroicMode?46194:44176)
#define SPELL_CONE_OF_COLD          (HeroicMode?38384:12611)
#define SPELL_FROSTBOLT             (HeroicMode?15530:15043)

struct TRINITY_DLL_DECL boss_yazzaiAI : public boss_priestess_guestAI
{
    //Mage
    boss_yazzaiAI(Creature *c) : boss_priestess_guestAI(c)  {}

    bool canFroze;
    bool canCoC;
    bool canBlink;
    bool hasIceBlocked;

    uint32 FrostNova_Cooldown;
    uint32 ConeOfCold_Cooldown;
    uint32 Blink_Cooldown;

    uint32 MeleeCheck_Timer;
    uint32 Polymorph_Timer;
    uint32 Ice_Block_Timer;
    uint32 Blizzard_Timer;
    uint32 Ice_Lance_Timer;

    void Reset()
    {
        hasIceBlocked = false;
        canFroze = true;
        canCoC = true;
        canBlink = true;
        FrostNova_Cooldown = 25000;
        ConeOfCold_Cooldown = 10000;
        Blink_Cooldown = 15000;
        Polymorph_Timer = 0;
        MeleeCheck_Timer = 2000;
        Blizzard_Timer = urand(3000, 10000);
        SetAutocast(SPELL_FROSTBOLT, 3000, true);

        boss_priestess_guestAI::Reset();
    }

    void AttackStart(Unit* who)
    {
        if(who->IsWithinDistInMap(me, 40.0))
            ScriptedAI::AttackStartNoMove(who);
        else
        {
            float x, y, z;
            me->GetClosePoint(x, y, z, 0, me->GetDistance2d(who)/2, me->GetAngle(who));
            me->GetMotionMaster()->MovePoint(1, x, y, z);
        }
    }

    void MovementInform(uint32 Type, uint32 Id)
    {
        if(Type == POINT_MOTION_TYPE)
        {
            switch(Id)
            {
                case 1:
                    me->GetMotionMaster()->MoveIdle();
                    break;
                default:
                    break;
            }
        }
    }

    void RegenMana()
    {
        uint32 maxMana = me->GetMaxPower(POWER_MANA);
        uint32 Mana = me->GetPower(POWER_MANA);
        me->SetPower(POWER_MANA, Mana+0.05*maxMana);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim() )
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(!canFroze)
        {
            if(FrostNova_Cooldown < diff)
            {
                canFroze = true;
                FrostNova_Cooldown = 25000;
            }
            else
                FrostNova_Cooldown -= diff;
        }
        if(!canCoC)
        {
            if(ConeOfCold_Cooldown < diff)
            {
                canCoC = true;
                ConeOfCold_Cooldown = 10000;
            }
            else
                ConeOfCold_Cooldown -= diff;
        }
        if(!canBlink)
        {
            if(Blink_Cooldown < diff)
            {
                ClearCastQueue();
                SetAutocast(SPELL_FROSTBOLT, 3000, true);
                canBlink = true;
                Blink_Cooldown = 15000;
            }
            else
                Blink_Cooldown -= diff;
        }

        if(me->IsWithinMeleeRange(me->getVictim()))
        {
            if(MeleeCheck_Timer < diff)
            {
                if(canFroze)
                {
                    ForceSpellCast(SPELL_FROST_NOVA, CAST_SELF, INTERRUPT_AND_CAST);
                    ClearCastQueue();
                    SetAutocast(SPELL_ICE_LANCE, 1400, true);
                    canFroze = false;
                }
                else if(canCoC)
                {
                    ForceSpellCast(SPELL_CONE_OF_COLD, CAST_SELF, INTERRUPT_AND_CAST);
                    canCoC = false;
                }

                if(canBlink)
                {
                    ForceSpellCast(SPELL_BLINK, CAST_SELF);
                    MeleeCheck_Timer = urand(5000, 10000);
                    canBlink = false;
                    RegenMana();
                    return;
                }
                else
                {
                    float x, y, z;
                    me->GetClosePoint(x, y, z, 0, 7.0, me->GetAngle(me->getVictim()));
                    me->GetMotionMaster()->MovePoint(1, x, y, z);
                }
                RegenMana();
                MeleeCheck_Timer = 2000;
            }
            else
                MeleeCheck_Timer -= diff;
        }

        if(Polymorph_Timer < diff)
        {
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 30, true))
            {
                AddSpellToCast(target, SPELL_POLYMORPH);
                DoModifyThreatPercent(target,-100);
                Polymorph_Timer = urand(15000, 25000);
            }
            else
                Polymorph_Timer = 1000;
        }
        else
            Polymorph_Timer -= diff;

        if(HealthBelowPct(35) && !hasIceBlocked)
        {
            ForceSpellCast(SPELL_ICE_BLOCK, CAST_SELF);
            hasIceBlocked = true;
        }

        if(Blizzard_Timer < diff)
        {
            AddSpellToCast(SPELL_BLIZZARD, CAST_RANDOM);
            Blizzard_Timer = urand(16000, 24000);
        }
        else
            Blizzard_Timer -= diff;

        CastNextSpellIfAnyAndReady(diff);
    }
};

#define SPELL_BATTLE_SHOUT           27578
#define SPELL_INTERCEPT              27577
#define SPELL_DISARM                 27581
#define SPELL_PIERCING_HOWL          23600
#define SPELL_FRIGHTENING_SHOUT      19134
#define SPELL_HAMSTRING              27584
#define SPELL_MORTAL_STRIKE          44268

struct TRINITY_DLL_DECL boss_warlord_salarisAI : public boss_priestess_guestAI
{
    //Warrior
    boss_warlord_salarisAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 BattleShout_Timer;
    uint32 Intercept_Timer;
    uint32 Disarm_Timer;
    uint32 Piercing_Howl_Timer;
    uint32 Frightening_Shout_Timer;
    uint32 Hamstring_Timer;
    uint32 Mortal_Strike_Timer;

    void Reset()
    {
        boss_priestess_guestAI::Reset();
        BattleShout_Timer = 110000;
        Intercept_Timer = 1000;
        Disarm_Timer = urand(10000, 15000);
        Hamstring_Timer = urand(4000, 5000);
        Mortal_Strike_Timer = urand(8000, 12000);
        Piercing_Howl_Timer = urand(5000, 8000);
        Frightening_Shout_Timer = urand(15000, 24000);
        resetThreat = false;
    }

    void EnterCombat(Unit* who)
    {
        ForceSpellCast(SPELL_BATTLE_SHOUT, CAST_NULL);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(BattleShout_Timer < diff)
        {
            ForceSpellCast(SPELL_BATTLE_SHOUT, CAST_NULL);
            BattleShout_Timer = 110000;
        }
        else
            BattleShout_Timer -= diff;

        if(Intercept_Timer < diff)
        {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 25.0, true, 0, 8.0);
            if(target)
            {
                DoResetThreat();
                AttackStart(target);
                me->AddThreat(target, 10000);
                ForceSpellCast(target, SPELL_INTERCEPT, INTERRUPT_AND_CAST);
                Intercept_Timer = urand(15000, 25000);
            }
            else
                Intercept_Timer = 2000;
        }
        else
            Intercept_Timer -= diff;

        if(Disarm_Timer < diff)
        {
            AddSpellToCast(SPELL_DISARM, CAST_TANK);
            Disarm_Timer = 60000;
        }
        else
            Disarm_Timer -= diff;

        if(Hamstring_Timer < diff)
        {
            AddSpellToCast(SPELL_HAMSTRING, CAST_TANK);
            Hamstring_Timer = urand(4000, 5000);
        }
        else
            Hamstring_Timer -= diff;

        if(Mortal_Strike_Timer < diff)
        {
            AddSpellToCast(SPELL_MORTAL_STRIKE, CAST_TANK);
            Mortal_Strike_Timer = urand(8000, 12000);
        }
        else
            Mortal_Strike_Timer -= diff;

        if(Piercing_Howl_Timer < diff)
        {
            AddSpellToCast(SPELL_PIERCING_HOWL, CAST_NULL);
            Piercing_Howl_Timer = urand(20000, 35000);
        }
        else
            Piercing_Howl_Timer -= diff;

        if(Frightening_Shout_Timer < diff)
        {
            AddSpellToCast(SPELL_FRIGHTENING_SHOUT, CAST_NULL);
            Frightening_Shout_Timer = urand(15000, 40000);
        }
        else
            Frightening_Shout_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

#define SPELL_SHOOT                 (HeroicMode?22907:15620)
#define SPELL_MULTI_SHOT            (HeroicMode?44285:31942)
#define SPELL_AIMED_SHOT            44271
#define SPELL_CONCUSSIVE_SHOT       27634
#define SPELL_WING_CLIP             44286
#define SPELL_FREEZING_TRAP         44136

#define NPC_SLIVER                  24552

struct TRINITY_DLL_DECL boss_garaxxasAI : public boss_priestess_guestAI
{
    //Hunter
    boss_garaxxasAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 GetSliver_Timer;
    uint32 Aimed_Shot_Timer;
    uint32 Shoot_Timer;
    uint32 Concussive_Shot_Timer;
    uint32 Multi_Shot_Timer;
    uint32 Wing_Clip_Timer;
    uint32 Freezing_Trap_Timer;

    void Reset()
    {
        targetRange = 30;
        Aimed_Shot_Timer = 500;
        Shoot_Timer = 2500;
        Concussive_Shot_Timer = urand(6000, 8000);
        Multi_Shot_Timer = urand(16000, 20000);
        Wing_Clip_Timer = 4000;
        Freezing_Trap_Timer = 15000;

        boss_priestess_guestAI::Reset();
    }

    void AttackStart(Unit* who)
    {
        if(who->IsWithinDistInMap(me, 30.0))
            ScriptedAI::AttackStartNoMove(who);
        else if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 30.0, true, 0, 5.0))
            ScriptedAI::AttackStartNoMove(target);
        else
        {
            float x, y, z;
            me->GetClosePoint(x, y, z, 0, me->GetDistance2d(who)/2, me->GetAngle(who));
            me->GetMotionMaster()->MovePoint(1, x, y, z);
        }
    }

    void MovementInform(uint32 Type, uint32 Id)
    {
        if(Type == POINT_MOTION_TYPE)
        {
            switch(Id)
            {
                case 1:
                    me->GetMotionMaster()->MoveIdle();
                    break;
                default:
                    break;
            }
        }
    }

    void JustDied(Unit* killer)
    {
        boss_priestess_guestAI::JustDied(killer);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!me->isInCombat())
        {
            if(GetSliver_Timer < diff)
            {
                // check if still having pet ;]
                Unit* Sliver = FindCreature(NPC_SLIVER, 60.0, me);

                if(!Sliver)
                {
                    float x, y, z;
                    me->GetNearPoint(me, x, y, z, 0, 3.0, frand(0, 2*M_PI));
                    Creature* Sliver = m_creature->SummonCreature(NPC_SLIVER, x, y, z, me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                }

                if(Sliver && !Sliver->isAlive())
                {
                    ((Creature*)Sliver)->RemoveCorpse();
                    float x, y, z;
                    me->GetNearPoint(me, x, y, z, 0, 3.0, frand(0, 2*M_PI));
                    Creature* Sliver = m_creature->SummonCreature(NPC_SLIVER, x, y, z, me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                }
                GetSliver_Timer = 15000;
            }
            else
                GetSliver_Timer -= diff;
        }

        if(!UpdateVictim() )
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(m_creature->IsWithinDistInMap(m_creature->getVictim(), 5))
        {
            if(Wing_Clip_Timer < diff)
            {
                AddSpellToCast(SPELL_WING_CLIP, CAST_TANK);
                float x, y, z;
                me->GetClosePoint(x, y, z, 0, 8.0);
                me->UpdateAllowedPositionZ(x, y, z);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
                Wing_Clip_Timer = urand(4000, 8000);
            }
            else
                Wing_Clip_Timer -= diff;
        }
        else
        {
            if(Concussive_Shot_Timer < diff)
            {
                AddSpellToCast(SPELL_CONCUSSIVE_SHOT, CAST_RANDOM);
                Concussive_Shot_Timer = urand(8000, 12000);
            }
            else
                Concussive_Shot_Timer -= diff;

            if(Multi_Shot_Timer < diff)
            {
                AddSpellToCast(SPELL_MULTI_SHOT, CAST_TANK);
                Multi_Shot_Timer = urand(10000, 15000);
            }
            else
                Multi_Shot_Timer -= diff;

            if(Aimed_Shot_Timer < diff)
            {
                AddSpellToCast(SPELL_AIMED_SHOT, CAST_TANK);
                Aimed_Shot_Timer = urand(12000, 18000);
            }
            else
                Aimed_Shot_Timer -= diff;

            if(Shoot_Timer < diff)
            {
                if(me->IsWithinDistInMap(me->getVictim(), 30))
                    AddSpellToCast(me->getVictim(), SPELL_SHOOT);
                else
                    ResetThreatTimer = 0;
                Shoot_Timer = urand(2500, 6000);
            }
            else
                Shoot_Timer -= diff;

            if(Freezing_Trap_Timer < diff)
            {
                AddSpellToCast(SPELL_FREEZING_TRAP, CAST_SELF);
                DoModifyThreatPercent(m_creature->getVictim(),-100);
                float x, y, z;
                me->GetClosePoint(x, y, z, 0, 7.0);
                me->UpdateAllowedPositionZ(x, y, z);
                me->GetMotionMaster()->MovePoint(1, x, y, z);
                Freezing_Trap_Timer = urand(20000, 40000);
            }
            else
                Freezing_Trap_Timer -= diff;
        }

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

struct TRINITY_DLL_DECL mob_sliverAI : public ScriptedAI
{
    mob_sliverAI(Creature *c) : ScriptedAI(c) { }

    void Reset() { }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

      DoMeleeAttackIfReady();
    }
};

#define SPELL_WAR_STOMP              46026
#define SPELL_PURGE                  27626
#define SPELL_LESSER_HEALING_WAVE    44256
#define SPELL_FROST_SHOCK            21401
#define SPELL_WINDFURY_TOTEM         27621
#define SPELL_FIRE_NOVA_TOTEM        44257
#define SPELL_EARTHBIND_TOTEM        15786

#define NPC_WINDFURY_TOTEM            7484
#define SPELL_WINFURY_WEAPON         32911

struct TRINITY_DLL_DECL boss_apokoAI : public boss_priestess_guestAI
{
    //Shaman
    boss_apokoAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 Totem_Timer;
    uint8  Totem_Amount;
    uint32 War_Stomp_Timer;
    uint32 Purge_Timer;
    uint32 Healing_Wave_Cooldown;
    uint32 Frost_Shock_Timer;
    bool canHeal;

    void Reset()
    {
        Totem_Timer = urand(3000, 5000);
        War_Stomp_Timer = urand(2000, 10000);
        Healing_Wave_Cooldown = 5000;
        Purge_Timer = urand(8000, 15000);
        Frost_Shock_Timer = urand(5000, 10000);
        canHeal = true;

        boss_priestess_guestAI::Reset();
    }

    void JustSummoned(Creature* summon)
    {
        if(summon->GetEntry() == NPC_WINDFURY_TOTEM)
            summon->CastSpell(summon, SPELL_WINFURY_WEAPON, true);
    }

    void RegenMana()
    {
        uint32 maxMana = me->GetMaxPower(POWER_MANA);
        uint32 Mana = me->GetPower(POWER_MANA);
        me->SetPower(POWER_MANA, Mana+0.05*maxMana);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim() )
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(canHeal)
        {
            if(Unit* healTarget = SelectLowestHpFriendly(40.0f, 10000))
                ForceSpellCast(healTarget, SPELL_LESSER_HEALING_WAVE, INTERRUPT_AND_CAST);
            canHeal = false;
        }
        else
        {
            if(Healing_Wave_Cooldown < diff)
            {
                canHeal = true;
                DoCast(m_creature, SPELL_LESSER_HEALING_WAVE);
                Healing_Wave_Cooldown = (HeroicMode?urand(5000, 8000):urand(6000, 10000));
            }
            else
                Healing_Wave_Cooldown -= diff;
        }

        if(Totem_Timer < diff)
        {
            AddSpellToCast(RAND(SPELL_WINDFURY_TOTEM, SPELL_FIRE_NOVA_TOTEM, SPELL_EARTHBIND_TOTEM), CAST_SELF);
            RegenMana();
            Totem_Timer = urand(3000, 8000);
        }
        else
            Totem_Timer -= diff;

        if(War_Stomp_Timer < diff)
        {
            AddSpellToCast(SPELL_WAR_STOMP, CAST_NULL);
            War_Stomp_Timer = urand(8000, 15000);
        }
        else
            War_Stomp_Timer -= diff;

        if(Purge_Timer < diff)
        {
            AddSpellToCast(SPELL_PURGE, CAST_RANDOM);
            Purge_Timer = urand(8000, 15000);
        }
        else
            Purge_Timer -= diff;

        if(Frost_Shock_Timer < diff)
        {
            AddSpellToCast(SPELL_FROST_SHOCK, CAST_TANK);
            Frost_Shock_Timer = urand(4000, 8000);
        }
        else
            Frost_Shock_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

#define SPELL_GOBLIN_DRAGON_GUN        (HeroicMode?46185:44272)
#define SPELL_ROCKET_LAUNCH            (HeroicMode?46187:44137)
#define SPELL_FEL_IRON_BOMB            (HeroicMode?46184:46024)
#define SPELL_RECOMBOBULATE            44274
#define SPELL_HIGH_EXPLOSIVE_SHEEP     44276
#define SPELL_SHEEP_EXPLOSION          44279

#define CREATURE_EXPLOSIVE_SHEEP        24715

struct TRINITY_DLL_DECL boss_zelfanAI : public boss_priestess_guestAI
{
    //Engineer
    boss_zelfanAI(Creature *c) : boss_priestess_guestAI(c) {}

    uint32 Goblin_Dragon_Gun_Timer;
    uint32 Rocket_Launch_Timer;
    uint32 Recombobulate_Timer;
    uint32 High_Explosive_Sheep_Timer;
    uint32 Fel_Iron_Bomb_Timer;

    void Reset()
    {
        Goblin_Dragon_Gun_Timer = urand(5000, 20000);
        Rocket_Launch_Timer = 1000;
        Recombobulate_Timer = urand(3000, 6000);
        High_Explosive_Sheep_Timer = urand(8000, 15000);
        Fel_Iron_Bomb_Timer = urand(4000, 15000);

        boss_priestess_guestAI::Reset();
    }

    void JustSummoned(Creature* summon)
    {
        if(summon->GetEntry() != CREATURE_EXPLOSIVE_SHEEP)
            return;
        if(Unit* target = SelectUnit(SELECT_TARGET_NEAREST, 0, 100.0, true))
        {
            summon->AI()->AttackStart(target);
            summon->AddThreat(target, 50000);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim() )
            return;

        boss_priestess_guestAI::UpdateAI(diff);

        if(Goblin_Dragon_Gun_Timer < diff)
        {
            if (me->IsWithinDistInMap(me->getVictim(), 5))
            {
                ForceSpellCast(SPELL_GOBLIN_DRAGON_GUN, CAST_TANK, INTERRUPT_AND_CAST);
                Goblin_Dragon_Gun_Timer = urand(10000, 15000);
            }
            else
                Goblin_Dragon_Gun_Timer = 2000;
        }
        else
            Goblin_Dragon_Gun_Timer -= diff;

        if(Rocket_Launch_Timer < diff)
        {
            AddSpellToCast(SPELL_ROCKET_LAUNCH, CAST_RANDOM);
            Rocket_Launch_Timer = urand(6000, 9000);
        }
        else
            Rocket_Launch_Timer -= diff;

        if(Fel_Iron_Bomb_Timer < diff)
        {
            AddSpellToCast(SPELL_FEL_IRON_BOMB, CAST_RANDOM);
            Fel_Iron_Bomb_Timer = urand(15000, 20000);
        }
        else
            Fel_Iron_Bomb_Timer -= diff;

        if(Recombobulate_Timer < diff)
        {
            std::list<Creature*> CC_list = FindFriendlyCC(30.0);
            if(!CC_list.empty())
            {
                for(std::list<Creature*>::iterator i = CC_list.begin(); i != CC_list.end(); ++i)
                {
                    if((*i)->IsPolymorphed())
                    {
                        ForceSpellCast((*i), SPELL_RECOMBOBULATE, INTERRUPT_AND_CAST);
                        Recombobulate_Timer = urand(15000, 30000);
                        break;
                    }
                }
            }
            Recombobulate_Timer = 5000;
        }
        else
            Recombobulate_Timer -= diff;

        if(High_Explosive_Sheep_Timer < diff)
        {
            AddSpellToCast(SPELL_HIGH_EXPLOSIVE_SHEEP, CAST_SELF);
            High_Explosive_Sheep_Timer = urand(45000, 65000);
        }
        else
            High_Explosive_Sheep_Timer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

struct TRINITY_DLL_DECL mob_high_explosive_sheepAI : public ScriptedAI
{
    mob_high_explosive_sheepAI(Creature *c) : ScriptedAI(c) {}

    uint32 SelfDestro_Timer;
    uint32 Check_Timer;

    void Reset()
    {
        SelfDestro_Timer = 60000;
        Check_Timer = 1000;
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(Check_Timer < diff)
        {
            if(me->IsWithinMeleeRange(me->getVictim()))
                DoCast(me, SPELL_SHEEP_EXPLOSION);
            Check_Timer = 1000;
        }
        else
            Check_Timer -= diff;

        if(SelfDestro_Timer < diff)
            DoCast(me, SPELL_SHEEP_EXPLOSION);
        else
            SelfDestro_Timer -= diff;
    }
};

CreatureAI* GetAI_boss_priestess_delrissa(Creature *_Creature)
{
    return new boss_priestess_delrissaAI (_Creature);
}

CreatureAI* GetAI_boss_kagani_nightstrike(Creature *_Creature)
{
    return new boss_kagani_nightstrikeAI (_Creature);
}

CreatureAI* GetAI_ellris_duskhallow(Creature *_Creature)
{
    return new boss_ellris_duskhallowAI (_Creature);
}

CreatureAI* GetAI_mob_fizzle(Creature *_Creature)
{
    return new mob_fizzleAI (_Creature);
};

CreatureAI* GetAI_eramas_brightblaze(Creature *_Creature)
{
    return new boss_eramas_brightblazeAI (_Creature);
}

CreatureAI* GetAI_yazzai(Creature *_Creature)
{
    return new boss_yazzaiAI (_Creature);
}

CreatureAI* GetAI_warlord_salaris(Creature *_Creature)
{
    return new boss_warlord_salarisAI (_Creature);
}

CreatureAI* GetAI_garaxxas(Creature *_Creature)
{
    return new boss_garaxxasAI (_Creature);
}

CreatureAI* GetAI_mob_sliver(Creature *_Creature)
{
    return new mob_sliverAI (_Creature);
};

CreatureAI* GetAI_apoko(Creature *_Creature)
{
    return new boss_apokoAI (_Creature);
}

CreatureAI* GetAI_zelfan(Creature *_Creature)
{
    return new boss_zelfanAI (_Creature);
}

CreatureAI* GetAI_mob_high_explosive_sheep(Creature *_Creature)
{
    return new mob_high_explosive_sheepAI (_Creature);
};

void AddSC_boss_priestess_delrissa()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="boss_priestess_delrissa";
    newscript->GetAI = &GetAI_boss_priestess_delrissa;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_kagani_nightstrike";
    newscript->GetAI = &GetAI_boss_kagani_nightstrike;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_ellris_duskhallow";
    newscript->GetAI = &GetAI_ellris_duskhallow;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_fizzle";
    newscript->GetAI = &GetAI_mob_fizzle;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_eramas_brightblaze";
    newscript->GetAI = &GetAI_eramas_brightblaze;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_yazzai";
    newscript->GetAI = &GetAI_yazzai;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_warlord_salaris";
    newscript->GetAI = &GetAI_warlord_salaris;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_garaxxas";
    newscript->GetAI = &GetAI_garaxxas;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_sliver";
    newscript->GetAI = &GetAI_mob_sliver;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_apoko";
    newscript->GetAI = &GetAI_apoko;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_zelfan";
    newscript->GetAI = &GetAI_zelfan;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_high_explosive_sheep";
    newscript->GetAI = &GetAI_mob_high_explosive_sheep;
    newscript->RegisterSelf();
}

