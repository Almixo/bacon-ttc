//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_CROWBAR_H
#define WEAPON_CROWBAR_H

#include "basebludgeonweapon.h"
#include "rumble_shared.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef HL2MP
#error weapon_crowbar.h must not be included in hl2mp. The windows compiler will use the wrong class elsewhere if it is.
#endif

#define	WRENCH_RANGE	75.0f
#define	WRENCH_REFIRE	1.0f //0.75f
#define WRENCH_RANGE_PRIMARY 90.0f
#define WRENCH_RANGE_SECONDARY 120.0f

//-----------------------------------------------------------------------------
// CWeaponCrowbar
//-----------------------------------------------------------------------------

class CWeaponWrench : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponWrench, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CWeaponWrench();

	float		GetRange( void )		{	return	WRENCH_RANGE;	}
	float		GetFireRate( void )		{	return	WRENCH_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
	void		SecondaryAttack(void);
	void		PrimaryAttack(void);
	void		ItemPostFrame(void);
	void		Swing(int bIsSecondary);
	void		Hit(trace_t& traceHit, Activity nHitActivity, bool bIsSecondary);

	// Animation event
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int			m_iWrenchStage;
	float		m_flStageUpdate;
	bool		m_bIsHoldingSecondary;
	float		m_flHoldUpdate;
	float		m_flHoldMultiplier;
	float		m_flHoldRange;

	float		m_bIsAttacking = false;
	float		m_flDelayedAttack = 0;

	struct something {
		trace_t		tHit;
		Activity	aHitAct;
		bool		bIsSec;
	};

	something shit;

#ifdef MAPBASE
	// Don't use backup activities
	acttable_t		*GetBackupActivityList() { return NULL; }
	int				GetBackupActivityListCount() { return 0; }
#endif

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
};

#endif // WEAPON_CROWBAR_H