//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Crowbar - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "basebludgeonweapon.h"
#include "vstdlib/random.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "weapon_crowbar.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_plr_dmg_crowbar		( "sk_plr_dmg_crowbar","0");
ConVar    sk_npc_dmg_crowbar		( "sk_npc_dmg_crowbar","0");

//-----------------------------------------------------------------------------
// CWeaponWrench
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CWeaponCrowbar, DT_WeaponCrowbar)
END_SEND_TABLE()

#define BLUDGEON_HULL_DIM		16

static const Vector g_bludgeonMins(-BLUDGEON_HULL_DIM, -BLUDGEON_HULL_DIM, -BLUDGEON_HULL_DIM);
static const Vector g_bludgeonMaxs(BLUDGEON_HULL_DIM, BLUDGEON_HULL_DIM, BLUDGEON_HULL_DIM);


#ifndef HL2MP
LINK_ENTITY_TO_CLASS( weapon_crowbar, CWeaponCrowbar );
PRECACHE_WEAPON_REGISTER( weapon_crowbar );
#endif

acttable_t CWeaponCrowbar::m_acttable[] = 
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_RUN,				ACT_RUN_MELEE,			false },
	{ ACT_WALK,				ACT_WALK_MELEE,			false },

	{ ACT_ARM,				ACT_ARM_MELEE,			false },
	{ ACT_DISARM,			ACT_DISARM_MELEE,		false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_RANGE_ATTACK1,                ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_MELEE,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_MELEE,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_MELEE,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_MELEE,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_MELEE,            false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_MELEE,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_MELEE,		false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_MELEE,						false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CWeaponCrowbar);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCrowbar::CWeaponCrowbar( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponCrowbar::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_crowbar.GetFloat();

	return sk_npc_dmg_crowbar.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponCrowbar::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}


//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
ConVar sk_crowbar_lead_time( "sk_crowbar_lead_time", "0.9" );

int CWeaponCrowbar::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponCrowbar::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if ( pEnemy )
	{
		Vector vecDelta;
		VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
		VectorNormalize( vecDelta );
		
		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize( vecDelta2D );
		if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
		{
			vecDirection = vecDelta;
		}
	}

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), sk_npc_dmg_crowbar.GetFloat(), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponCrowbar::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

void CWeaponCrowbar::PrimaryAttack()
{
	if(m_iCrowbarStage == 0)
		Swing(false);
}

void CWeaponCrowbar::SecondaryAttack()
{
	// amogus
}

void CWeaponCrowbar::ItemPostFrame()
{
	BaseClass::ItemPostFrame(); // call the original

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	// player pressed the button, start charging...
	if ((pPlayer->m_nButtons & IN_ATTACK2) && m_iCrowbarStage == 0 && m_flStageUpdate < gpGlobals->curtime)
	{
		m_iCrowbarStage = 1;
		m_flHoldMultiplier = 1.0f;
		m_flHoldRange = 0;
		SendWeaponAnim(ACT_VM_HAULBACK);
		m_flStageUpdate = gpGlobals->curtime + SequenceDuration();
	}

	// start calculating how long its being held
	if (m_iCrowbarStage == 1 && m_flHoldUpdate < gpGlobals->curtime)
	{
		if (m_flHoldMultiplier < 2.0f)
			m_flHoldMultiplier += 0.03125f;

		if (m_flHoldRange < WRENCH_RANGE_SECONDARY - WRENCH_RANGE_PRIMARY)
			m_flHoldRange += (WRENCH_RANGE_SECONDARY - WRENCH_RANGE_PRIMARY) * 0.03125f;

		m_flHoldUpdate = gpGlobals->curtime + 0.1f;
		Msg("holding the wrench!, multiplier: %f\n\n\n", m_flHoldRange);
	}

	// player released the button, what to do now?
	if (!(pPlayer->m_nButtons & IN_ATTACK2) && m_iCrowbarStage == 1 && m_flStageUpdate < gpGlobals->curtime)
	{
		m_iCrowbarStage = 0;
		Swing(true);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
	}

}

void CWeaponCrowbar::Swing(int bIsSecondary)
{
	trace_t traceHit;

	// Try a ray
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	pOwner->RumbleEffect(RUMBLE_CROWBAR_SWING, 0, RUMBLE_FLAG_RESTART);

	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;

	forward = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT, GetRange());

	Vector swingEnd = swingStart + forward * (WRENCH_RANGE_PRIMARY + m_flHoldRange);
	UTIL_TraceLine(swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
	Activity nHitActivity = bIsSecondary ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo(GetOwner(), GetOwner(), GetDamageForActivity(nHitActivity), DMG_CLUB);

	triggerInfo.SetDamagePosition(traceHit.startpos);
	triggerInfo.SetDamageForce(forward);
	TraceAttackToTriggers(triggerInfo, traceHit.startpos, traceHit.endpos, forward);

	if (traceHit.fraction == 1.0)
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull(swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
		if (traceHit.fraction < 1.0 && traceHit.m_pEnt)
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize(vecToTarget);

			float dot = vecToTarget.Dot(forward);

			// YWB:  Make sure they are sort of facing the guy at least...
			if (dot < 0.70721f)
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				nHitActivity = ChooseIntersectionPointAndActivity(traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner);
			}
		}
	}

	if (!bIsSecondary)
	{
		m_iPrimaryAttacks++;
	}
	else
	{
		m_iSecondaryAttacks++;
	}

	gamestats->Event_WeaponFired(pOwner, !bIsSecondary, GetClassname());

	// -------------------------
	//	Miss
	// -------------------------
	if (traceHit.fraction == 1.0f)
	{
		nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER : ACT_VM_MISSRIGHT2;

		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();

#ifdef MAPBASE
		// Sound has been moved here since we're using the other melee sounds now
		WeaponSound(SINGLE);
#endif

		// See if we happened to hit water
		ImpactWater(swingStart, testEnd);
	}
	else
	{
#ifdef MAPBASE
		// Other melee sounds
		if (traceHit.m_pEnt && traceHit.m_pEnt->IsWorld())
			WeaponSound(MELEE_HIT_WORLD);
		else if (traceHit.m_pEnt && !traceHit.m_pEnt->PassesDamageFilter(triggerInfo))
			WeaponSound(MELEE_MISS);
		else
			WeaponSound(MELEE_HIT);
#endif

		Hit(traceHit, nHitActivity, bIsSecondary ? true : false);
	}

	// Send the anim
	SendWeaponAnim(nHitActivity);

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

#ifndef MAPBASE
	//Play swing sound
	WeaponSound(SINGLE);
#endif

#ifdef MAPBASE
	pOwner->SetAnimation(PLAYER_ATTACK1);
#endif
}

//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CWeaponCrowbar::Hit(trace_t& traceHit, Activity nHitActivity, bool bIsSecondary)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	//Do view kick
	AddViewKick();

	//Make sound for the AI
	CSoundEnt::InsertSound(SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer);

	// This isn't great, but it's something for when the crowbar hits.
	pPlayer->RumbleEffect(RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART);

	CBaseEntity* pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if (pHitEntity != NULL)
	{
		Vector hitDirection;
		pPlayer->EyeVectors(&hitDirection, NULL, NULL);
		VectorNormalize(hitDirection);

		CTakeDamageInfo info(GetOwner(), GetOwner(), GetDamageForActivity(nHitActivity), DMG_CLUB);

		// multiply damage by two times if charging - WIP
		if (bIsSecondary)
		{
			info.SetDamage(info.GetDamage() * m_flHoldMultiplier);
		}

		if (pPlayer && pHitEntity->IsNPC())
		{
			// If bonking an NPC, adjust damage.
			info.AdjustPlayerDamageInflictedForSkillLevel();
		}

		CalculateMeleeDamageForce(&info, hitDirection, traceHit.endpos);

		pHitEntity->DispatchTraceAttack(info, hitDirection, &traceHit);
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers(info, traceHit.startpos, traceHit.endpos, hitDirection);

		if (ToBaseCombatCharacter(pHitEntity))
		{
			gamestats->Event_WeaponHit(pPlayer, !bIsSecondary, GetClassname(), info);
		}
	}

	// Apply an impact effect
	ImpactEffect(traceHit);
}