//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "ai_basenpc.h"
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"
#include "beam_shared.h"
#include "ttc/CDot.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponDeagle
//-----------------------------------------------------------------------------

#ifdef MAPBASE
extern acttable_t *GetDeagleActtable( );
extern int GetDeagleActtableCount( );
#endif

#define	RPG_BEAM_SPRITE		"effects/laser1_noz.vmt"

class CWeaponDeagle : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponDeagle, CBaseHLCombatWeapon );
public:

	CWeaponDeagle( void );
	~CWeaponDeagle( void );

	bool	Holster( CBaseCombatWeapon* pSwitchingTo = NULL );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	WeaponIdle( void );
	void	ItemPostFrame( void );

	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	float	WeaponAutoAimScale( ) { return 0.6f; }

	void	StartGuiding( void );
	void	StopGuiding( void );

	void	CreateLaser( void );

	bool	IsGuiding( void ) { return m_bGuiding; };
	void	ToggleGuiding( void ) { IsGuiding() ? StopGuiding() : StartGuiding(); };

#ifdef MAPBASE
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual int	GetMinBurst( ) { return 1; }
	virtual int	GetMaxBurst( ) { return 1; }
	virtual float	GetMinRestTime( void ) { return 1.0f; }
	virtual float	GetMaxRestTime( void ) { return 2.5f; }

	virtual float GetFireRate( void ) { return 1.0f; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_15DEGREES;
		if ( !GetOwner( ) || !GetOwner( )->IsNPC( ) )
			return cone;

		static Vector AllyCone = VECTOR_CONE_2DEGREES;
		static Vector NPCCone = VECTOR_CONE_5DEGREES;

		if ( GetOwner( )->MyNPCPointer( )->IsPlayerAlly( ) )
		{
			// 357 allies should be cooler
			return AllyCone;
		}

		return NPCCone;
	}

	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );

	virtual acttable_t		*GetBackupActivityList( ) { return GetDeagleActtable( ); }
	virtual int				GetBackupActivityListCount( ) { return GetDeagleActtableCount( ); }
#endif

	DECLARE_SERVERCLASS( );
	DECLARE_DATADESC( );
#ifdef MAPBASE
	DECLARE_ACTTABLE( );
#endif
private:
	bool m_bGuiding;
	bool m_bWasGuiding;

	CBeam *pBeam;
	CGuidedDot *pDot;
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );

PRECACHE_WEAPON_REGISTER( weapon_deagle );

IMPLEMENT_SERVERCLASS_ST( CWeaponDeagle, DT_WeaponDeagle )
END_SEND_TABLE( )

BEGIN_DATADESC( CWeaponDeagle )
END_DATADESC( )

#ifdef MAPBASE
acttable_t	CWeaponDeagle::m_acttable[ ] =
{
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_IDLE,						ACT_IDLE_REVOLVER,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_REVOLVER,				true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_REVOLVER,			true },
	{ ACT_RELOAD,					ACT_RELOAD_REVOLVER,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_REVOLVER,				true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_REVOLVER,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_REVOLVER,	true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_REVOLVER_LOW,				false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_REVOLVER_LOW,		false },
	{ ACT_COVER_LOW,				ACT_COVER_REVOLVER_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_REVOLVER_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_REVOLVER,			false },
	{ ACT_WALK,						ACT_WALK_REVOLVER,					true },
	{ ACT_RUN,						ACT_RUN_REVOLVER,					true },
#else
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },
#endif

	// 
	// Activities ported from weapon_alyxgun below
	// 

	// Readiness activities (not aiming)
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_PISTOL_STIMULATED,		false },
#else
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
#endif
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_WALK_RELAXED,				ACT_WALK_PISTOL_RELAXED,		false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_PISTOL_STIMULATED,		false },
#else
	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
#endif
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_RUN_RELAXED,				ACT_RUN_PISTOL_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_PISTOL_STIMULATED,		false },
#else
	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
#endif
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

#if EXPANDED_HL2_COVER_ACTIVITIES
	{ ACT_RANGE_AIM_MED,			ACT_RANGE_AIM_REVOLVER_MED,			false },
	{ ACT_RANGE_ATTACK1_MED,		ACT_RANGE_ATTACK_REVOLVER_MED,		false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_REVOLVER,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_REVOLVER,                    false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_REVOLVER,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,    false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_REVOLVER,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_REVOLVER,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,                    false },
#else
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,                    false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,        false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,                    false },
#endif
#endif
};


IMPLEMENT_ACTTABLE( CWeaponDeagle );

// Allows Weapon_BackupActivity() to access the 357's activity table.
acttable_t *GetDeagleActtable( )
{
	return CWeaponDeagle::m_acttable;
}

int GetDeagleActtableCount( )
{
	return ARRAYSIZE( CWeaponDeagle::m_acttable );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeagle::CWeaponDeagle( void )
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;

	m_bGuiding = false;
	m_bWasGuiding = false;

#ifdef MAPBASE
	m_fMinRange1 = 24;
	m_fMaxRange1 = 1000;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CWeaponDeagle::~CWeaponDeagle( void )
{
	m_bGuiding = false;
	m_bWasGuiding = false;

	if ( pBeam != nullptr )
	{
		pBeam->Remove();
		pBeam = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner( ) );

	switch ( pEvent->event )
	{
	case EVENT_WEAPON_RELOAD:
	{
		CEffectData data;

		// Emit six spent shells
		for ( int i = 0; i < 6; i++ )
		{
			data.m_vOrigin = pOwner->WorldSpaceCenter( ) + RandomVector( -4, 4 );
			data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
			data.m_nEntIndex = entindex( );

			DispatchEffect( "ShellEject", data );
		}

		break;
	}
#ifdef MAPBASE
	case EVENT_WEAPON_PISTOL_FIRE:
	{
		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition( );

		CAI_BaseNPC *npc = pOperator->MyNPCPointer( );
		ASSERT( npc != NULL );

		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

		FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
	}
	break;
	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
#endif
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin( ), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy( ) );

	WeaponSound( SINGLE_NPC );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
	pOperator->DoMuzzleFlash( );
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: Some things need this. (e.g. the new Force(X)Fire inputs or blindfire actbusy)
//-----------------------------------------------------------------------------
void CWeaponDeagle::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner( ) );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload( );
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname( ) );

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash( );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );


	Vector spread;

	if ( m_bGuiding )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;
		spread = vec3_origin;
	}
	else
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.25;
		spread = Vector( 0.3, 0.3, 0.3 );
	}

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition( );
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	pPlayer->FireBullets( 1, vecSrc, vecAiming, spread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles( );

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

	pPlayer->SnapEyeAngles( angles );

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin( ), 600, 0.2, GetOwner( ) );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}
}

void CWeaponDeagle::SecondaryAttack( void )
{
	ToggleGuiding();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;

	Warning( "Toggled? %s.\n", m_bGuiding ? "yes" : "no" );
}

void CWeaponDeagle::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bGuiding )
	{
		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
		QAngle qAng;
		Vector vOrigin, vDir, vEnd;
		trace_t tr;

		GetAttachment(LookupAttachment("laser_start"), vOrigin, qAng);
		AngleVectors(qAng, &vDir);

		UTIL_TraceLine(vOrigin, vOrigin + (vDir * MAX_TRACE_LENGTH), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

		vEnd = tr.endpos + vDir * -4;

		NDebugOverlay::Sphere(vEnd, 4, 255, 0, 0, true, 0.5);

		pDot->SetDotPosition(vEnd, tr.plane.normal);
	}
}

bool CWeaponDeagle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bGuiding )
	{
		StopGuiding();
		m_bWasGuiding = true;
	}
	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponDeagle::WeaponIdle( void )
{
	BaseClass::WeaponIdle();

	if ( !m_bGuiding && m_bWasGuiding )
	{
		StartGuiding();
		m_bWasGuiding = false;
	}
}

void CWeaponDeagle::StartGuiding( void )
{
	m_bGuiding = true;

	CreateLaser();

	if ( pBeam )
		pBeam->SetBrightness(255);

	if ( pDot )
		pDot->TurnOn();

	DevWarning("Started Guiding.\n");

	WeaponSound( SPECIAL1 );
}

void CWeaponDeagle::StopGuiding( void )
{
	m_bGuiding = false;

	if ( pBeam )
		pBeam->SetBrightness(0);

	if ( pDot )
		pDot->TurnOff();

	DevWarning("Ended Guiding.\n");

	WeaponSound( SPECIAL2 );
}

void CWeaponDeagle::CreateLaser( void )
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	if (pBeam == NULL)
	{
		pBeam = CBeam::BeamCreate(RPG_BEAM_SPRITE, 1.0f);

		if (pBeam == NULL)
		{
			// We were unable to create the beam
			Assert(0);
			return;
		}

		CBaseViewModel *pBeamEnt = pOwner->GetViewModel();

		pBeam->EntsInit(pBeamEnt, pBeamEnt);

		int	startAttachment = LookupAttachment("laser_start");
		int endAttachment = LookupAttachment("laser_end");

		pBeam->FollowEntity(pBeamEnt);
		pBeam->SetStartAttachment(startAttachment);
		pBeam->SetEndAttachment(endAttachment);
		pBeam->SetNoise(0);
		pBeam->SetColor(255, 0, 0);
		pBeam->SetScrollRate(0);
		pBeam->SetWidth(0.5f);
		pBeam->SetEndWidth(0.5f);
		pBeam->SetBrightness(m_bGuiding ? 255 : 0);
		pBeam->SetBeamFlags(SF_BEAM_SHADEIN);
	}

	if (pDot == NULL)
	{
		pDot = CGuidedDot::Create(vec3_origin, pOwner, m_bGuiding, true);

		if ( pDot == NULL )
			assert(0);
	}
}
