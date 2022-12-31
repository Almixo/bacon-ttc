//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"
#include "weapon_rpg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeapon357
//-----------------------------------------------------------------------------

#ifdef MAPBASE
extern acttable_t *GetPistolActtable();
extern int GetPistolActtableCount();
#endif

const char* g_pDeLaserDotThink = "DeLaserThinkContext";

#define	RPG_BEAM_SPRITE		"effects/laser1_noz.vmt"
#define	RPG_LASER_SPRITE	"sprites/redglow1.vmt"

//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------
class CDeLaserDot : public CSprite
{
	DECLARE_CLASS(CDeLaserDot, CSprite);
public:

	CDeLaserDot(void);
	~CDeLaserDot(void);

	static CDeLaserDot* Create(const Vector& origin, CBaseEntity* pOwner = NULL, bool bVisibleDot = true);

	void	SetTargetEntity(CBaseEntity* pTarget) { m_hTargetEnt = pTarget; }
	CBaseEntity* GetTargetEntity(void) { return m_hTargetEnt; }

	void	SetLaserPosition(const Vector& origin, const Vector& normal);
	Vector	GetChasePosition();
	void	TurnOn(void);
	void	TurnOff(void);
	bool	IsOn() const { return m_bIsOn; }

	void	Toggle(void);

	void	LaserThink(void);

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	void	MakeInvisible(void);

protected:
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;
	bool				m_bVisibleLaserDot;
	bool				m_bIsOn;

	DECLARE_DATADESC();
public:
	CDeLaserDot* m_pNext;
};


// a list of laser dots to search quickly
CEntityClassList<CDeLaserDot> g_DeLaserDotList;
template <>  CDeLaserDot* CEntityClassList<CDeLaserDot>::m_pClassList = NULL;
CDeLaserDot* GetDeLaserDotList()
{
	return g_DeLaserDotList.m_pClassList;
}

class CWeapon357 : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeapon357, CBaseHLCombatWeapon );
public:

	CWeapon357( void );

	void	Spawn();
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);
	void	PrimaryAttack( void );

	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	ItemPostFrame(void);
	void	ItemBusyFrame(void);
	bool	Reload();
	float	WeaponAutoAimScale()	{ return 0.6f; }

	//laser crpa
	void	SuppressGuiding(bool state = true);
	void	CreateLaserPointer(void);
	void	UpdateLaserPosition(Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin);
	Vector	GetLaserPosition(void);
	void	StartGuiding(void);
	void	StopGuiding(void);
	void	ToggleGuiding(void);
	bool	IsGuiding(void);
	bool	Deploy(void);
	//bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);
	void	StartLaserEffects(void);
	void	StopLaserEffects(void);
	void	UpdateLaserEffects(void);

#ifdef MAPBASE
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual int	GetMinBurst() { return 1; }
	virtual int	GetMaxBurst() { return 1; }
	virtual float	GetMinRestTime( void ) { return 1.0f; }
	virtual float	GetMaxRestTime( void ) { return 2.5f; }

	virtual float GetFireRate( void ) { return 1.0f; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_15DEGREES;
		if (!GetOwner() || !GetOwner()->IsNPC())
			return cone;

		static Vector AllyCone = VECTOR_CONE_2DEGREES;
		static Vector NPCCone = VECTOR_CONE_5DEGREES;

		if( GetOwner()->MyNPCPointer()->IsPlayerAlly() )
		{
			// 357 allies should be cooler
			return AllyCone;
		}

		return NPCCone;
	}

	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );

	virtual acttable_t		*GetBackupActivityList() { return GetPistolActtable(); }
	virtual int				GetBackupActivityListCount() { return GetPistolActtableCount(); }
#endif

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
#ifdef MAPBASE
	DECLARE_ACTTABLE();
#endif

protected:

	CHandle<CSprite>	m_hLaserMuzzleSprite;
	CHandle<CBeam>		m_hLaserBeam;
	CHandle<CDeLaserDot>	m_hLaserDot;
	bool				m_bInitialStateUpdate;
	bool				m_bGuiding;
	bool				m_bHideGuiding;
};

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );

PRECACHE_WEAPON_REGISTER( weapon_357 );

IMPLEMENT_SERVERCLASS_ST( CWeapon357, DT_Weapon357 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeapon357 )
END_DATADESC()

#ifdef MAPBASE
acttable_t	CWeapon357::m_acttable[] =
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


IMPLEMENT_ACTTABLE( CWeapon357 );

// Allows Weapon_BackupActivity() to access the 357's activity table.
acttable_t *Get357Acttable()
{
	return CWeapon357::m_acttable;
}

int Get357ActtableCount()
{
	return ARRAYSIZE(CWeapon357::m_acttable);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

#ifdef MAPBASE
	m_fMinRange1		= 24;
	m_fMaxRange1		= 1000;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six spent shells
				for ( int i = 0; i < 6; i++ )
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
					data.m_nEntIndex = entindex();

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
#ifdef MAPBASE
		case EVENT_WEAPON_PISTOL_FIRE:
			{
				Vector vecShootOrigin, vecShootDir;
				vecShootOrigin = pOperator->Weapon_ShootPosition();

				CAI_BaseNPC *npc = pOperator->MyNPCPointer();
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
void CWeapon357::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	WeaponSound( SINGLE_NPC );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1 );
	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: Some things need this. (e.g. the new Force(X)Fire inputs or blindfire actbusy)
//-----------------------------------------------------------------------------
void CWeapon357::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
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
void CWeapon357::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );


	Vector spread;

	if (m_bGuiding)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;
		spread = vec3_origin;
	}
	else
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.25;
		spread = Vector(0.3, 0.3, 0.3);
	}

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	pPlayer->FireBullets( 1, vecSrc, vecAiming, spread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

	pPlayer->SnapEyeAngles( angles );

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}

void CWeapon357::Spawn()
{
	return BaseClass::Spawn();
}

bool CWeapon357::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	StopGuiding();
	return BaseClass::Holster(pSwitchingTo);
}

bool CWeapon357::Reload()
{
	//m_bLaserEnabled = false;

	return BaseClass::Reload();
}

void CWeapon357::ItemBusyFrame()
{
	//if(IsGuiding())
		//SuppressGuiding(true);

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if(pOwner)
	{
		int attachment = pOwner->GetViewModel(0)->LookupAttachment("muzzle");
		Vector cameraOrigin = Vector(0, 0, 0);
		QAngle cameraAngles = QAngle(0, 0, 0);
		QAngle ShootAngle;
		Vector Shootforward;
		pOwner->EyeVectors(&Shootforward);
		VectorAngles(Shootforward, ShootAngle);

		if(attachment != -1)
		{
			pOwner->GetViewModel(0)->GetAttachmentLocal(attachment, cameraOrigin, cameraAngles);
		}

		cameraAngles += ShootAngle;

		Vector forward;
		AngleVectors(cameraAngles, &forward);
		Vector vecMuzzlePos = pOwner->Weapon_ShootPosition();
		Vector vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);

		UpdateLaserPosition(vecMuzzlePos, vecEndPos);
	}


	BaseClass::ItemBusyFrame();
}

void CWeapon357::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if(pOwner)
	{
		// toggle right click
		if (m_flNextSecondaryAttack < gpGlobals->curtime && pOwner->m_afButtonPressed & IN_ATTACK2)
		{
			ToggleGuiding();
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		}
	}

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ((m_bInitialStateUpdate) && (GetActivity() != ACT_VM_DRAW))
	{
		StartGuiding();
		m_bInitialStateUpdate = false;
	}
	
	if (IsGuiding())
	{
		SuppressGuiding(false);
	}

	//Move the laser
	if (pOwner)
	{
		int attachment = pOwner->GetViewModel(0)->LookupAttachment("muzzle");
		Vector cameraOrigin = Vector(0, 0, 0);
		QAngle cameraAngles = QAngle(0, 0, 0);
		QAngle ShootAngle;
		Vector Shootforward;
		pOwner->EyeVectors(&Shootforward);
		VectorAngles(Shootforward, ShootAngle);

		if (attachment != -1)
		{
			pOwner->GetViewModel(0)->GetAttachmentLocal(attachment, cameraOrigin, cameraAngles);
		}

		cameraAngles += ShootAngle;

		Vector forward;
		AngleVectors(cameraAngles, &forward);
		Vector vecMuzzlePos = pOwner->Weapon_ShootPosition();
		Vector vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);

		UpdateLaserPosition(vecMuzzlePos, vecEndPos);
	}
	UpdateLaserEffects();
	
}

void CWeapon357::SuppressGuiding(bool state)
{
	m_bHideGuiding = state;

	if (m_hLaserDot == NULL)
	{
		StartGuiding();

		//STILL!?
		if (m_hLaserDot == NULL)
			return;
	}

	if (state)
	{
		m_hLaserDot->TurnOff();
		StopLaserEffects();
	}
	else
	{
		m_hLaserDot->TurnOn();
		StartLaserEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CWeapon357::GetLaserPosition(void)
{
	CreateLaserPointer();

	if (m_hLaserDot != NULL)
		return m_hLaserDot->GetAbsOrigin();

	//FIXME: The laser dot sprite is not active, this code should not be allowed!
	assert(0);
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true if the rocket is being guided, false if it's dumb
//-----------------------------------------------------------------------------
bool CWeapon357::IsGuiding(void)
{
	return m_bGuiding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeapon357::Deploy(void)
{
	m_bInitialStateUpdate = true;

	return BaseClass::Deploy();
}


//-----------------------------------------------------------------------------
// Purpose: Turn on the guiding laser
//-----------------------------------------------------------------------------
void CWeapon357::StartGuiding(void)
{
	// Don't start back up if we're overriding this
	if (m_bHideGuiding)
		return;

	m_bGuiding = true;

	WeaponSound(SPECIAL1);

	CreateLaserPointer();
	StartLaserEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Turn off the guiding laser
//-----------------------------------------------------------------------------
void CWeapon357::StopGuiding(void)
{
	m_bGuiding = false;

	WeaponSound(SPECIAL2);

	StopLaserEffects();

	// Kill the dot completely
	if (m_hLaserDot != NULL)
	{
		m_hLaserDot->TurnOff();
		UTIL_Remove(m_hLaserDot);
		m_hLaserDot = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the guiding laser
//-----------------------------------------------------------------------------
void CWeapon357::ToggleGuiding(void)
{
	if (IsGuiding())
	{
		StopGuiding();
		SuppressGuiding(true);
	}
	else
	{
		StartGuiding();
		SuppressGuiding(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon357::UpdateLaserPosition(Vector vecMuzzlePos, Vector vecEndPos)
{
	if (vecMuzzlePos == vec3_origin || vecEndPos == vec3_origin)
	{
		CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
		if (!pPlayer)
			return;

		vecMuzzlePos = pPlayer->Weapon_ShootPosition();
		Vector	forward;

		if (g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE)
		{
			forward = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
		}
		else
		{
			pPlayer->EyeVectors(&forward);
		}

		vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);
	}

	//Move the laser dot, if active
	trace_t	tr;

	// Trace out for the endpoint
#ifdef PORTAL
	g_bBulletPortalTrace = true;
	Ray_t rayLaser;
	rayLaser.Init(vecMuzzlePos, vecEndPos);
	UTIL_Portal_TraceRay(rayLaser, (MASK_SHOT & ~CONTENTS_WINDOW), this, COLLISION_GROUP_NONE, &tr);
	g_bBulletPortalTrace = false;
#else
	UTIL_TraceLine(vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), this, COLLISION_GROUP_NONE, &tr);
#endif

	// Move the laser sprite
	if (m_hLaserDot != NULL)
	{
		Vector	laserPos = tr.endpos;
		m_hLaserDot->SetLaserPosition(laserPos, tr.plane.normal);

		if (tr.DidHitNonWorldEntity())
		{
			CBaseEntity* pHit = tr.m_pEnt;

			if ((pHit != NULL) && (pHit->m_takedamage))
			{
				m_hLaserDot->SetTargetEntity(pHit);
			}
			else
			{
				m_hLaserDot->SetTargetEntity(NULL);
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity(NULL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::CreateLaserPointer(void)
{
	if (m_hLaserDot != NULL)
		return;

	m_hLaserDot = CDeLaserDot::Create(GetAbsOrigin(), GetOwnerEntity());
	m_hLaserDot->TurnOff();

	UpdateLaserPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Start the effects on the viewmodel of the RPG
//-----------------------------------------------------------------------------
void CWeapon357::StartLaserEffects(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	CBaseViewModel* pBeamEnt = static_cast<CBaseViewModel*>(pOwner->GetViewModel());

	if (m_hLaserBeam == NULL)
	{
		m_hLaserBeam = CBeam::BeamCreate(RPG_BEAM_SPRITE, 1.0f);

		if (m_hLaserBeam == NULL)
		{
			// We were unable to create the beam
			Assert(0);
			return;
		}

		m_hLaserBeam->EntsInit(pBeamEnt, pBeamEnt);

		// It starts at startPos
		m_hLaserBeam->SetStartPos(pOwner->Weapon_ShootPosition());

		// This sets up some things that the beam uses to figure out where
		// it should start and end
		m_hLaserBeam->PointEntInit(GetLaserPosition(), m_hLaserDot);

		// This makes it so that the laser appears to come from the muzzle of the pistol
		m_hLaserBeam->SetStartAttachment(LookupAttachment("Muzzle"));


		m_hLaserBeam->SetNoise(0);
		m_hLaserBeam->SetColor(255, 0, 0);
		m_hLaserBeam->SetScrollRate(0);
		m_hLaserBeam->SetWidth(0.5f);
		m_hLaserBeam->SetEndWidth(0.5f);
		m_hLaserBeam->SetBrightness(128);
		m_hLaserBeam->SetBeamFlags(SF_BEAM_SHADEIN);
#ifdef PORTAL
		m_hLaserBeam->m_bDrawInMainRender = true;
		m_hLaserBeam->m_bDrawInPortalRender = false;
#endif
	}
	else
	{
		m_hLaserBeam->SetBrightness(128);
	}

	if (m_hLaserMuzzleSprite == NULL)
	{
		m_hLaserMuzzleSprite = CSprite::SpriteCreate(RPG_LASER_SPRITE, GetAbsOrigin(), false);

		if (m_hLaserMuzzleSprite == NULL)
		{
			// We were unable to create the sprite
			Assert(0);
			return;
		}

#ifdef PORTAL
		m_hLaserMuzzleSprite->m_bDrawInMainRender = true;
		m_hLaserMuzzleSprite->m_bDrawInPortalRender = false;
#endif

		m_hLaserMuzzleSprite->SetAttachment(pOwner->GetViewModel(), LookupAttachment("laser"));
		m_hLaserMuzzleSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation);
		m_hLaserMuzzleSprite->SetBrightness(255, 0.5f);
		m_hLaserMuzzleSprite->SetScale(0.25f, 0.5f);
		m_hLaserMuzzleSprite->TurnOn();
	}
	else
	{
		m_hLaserMuzzleSprite->TurnOn();
		m_hLaserMuzzleSprite->SetScale(0.25f, 0.25f);
		m_hLaserMuzzleSprite->SetBrightness(255);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop the effects on the viewmodel of the RPG
//-----------------------------------------------------------------------------
void CWeapon357::StopLaserEffects(void)
{
	if (m_hLaserBeam != NULL)
	{
		m_hLaserBeam->SetBrightness(0);
		m_hLaserBeam->TurnOff();
		UTIL_Remove(m_hLaserBeam);
		m_hLaserBeam = NULL;
	}

	if (m_hLaserMuzzleSprite != NULL)
	{
		m_hLaserMuzzleSprite->SetScale(0.01f);
		m_hLaserMuzzleSprite->SetBrightness(0, 0.5f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pulse all the effects to make them more... well, laser-like
//-----------------------------------------------------------------------------
void CWeapon357::UpdateLaserEffects(void)
{
	if (!m_bGuiding)
		return;

	if (m_hLaserBeam != NULL)
	{
		m_hLaserBeam->SetBrightness(128 + random->RandomInt(-8, 8));
	}

	if (m_hLaserMuzzleSprite != NULL)
	{
		m_hLaserMuzzleSprite->SetScale(0.1f + random->RandomFloat(-0.025f, 0.025f));
	}
}

//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CLASS(env_delaserdot, CDeLaserDot);

BEGIN_DATADESC(CDeLaserDot)
DEFINE_FIELD(m_vecSurfaceNormal, FIELD_VECTOR),
DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
DEFINE_FIELD(m_bVisibleLaserDot, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsOn, FIELD_BOOLEAN),

//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),	// don't save - regenerated by constructor
DEFINE_THINKFUNC(LaserThink),
END_DATADESC()


//-----------------------------------------------------------------------------
// Finds missiles in cone
//-----------------------------------------------------------------------------
CBaseEntity* CreateDeLaserDot(const Vector& origin, CBaseEntity* pOwner, bool bVisibleDot)
{
	return CDeLaserDot::Create(origin, pOwner, bVisibleDot);
}

void SetDeLaserDotTarget(CBaseEntity* pLaserDot, CBaseEntity* pTarget)
{
	CDeLaserDot* pDot = assert_cast<CDeLaserDot*>(pLaserDot);
	pDot->SetTargetEntity(pTarget);
}

void EnableDeLaserDot(CBaseEntity* pLaserDot, bool bEnable)
{
	CDeLaserDot* pDot = assert_cast<CDeLaserDot*>(pLaserDot);
	if (bEnable)
	{
		pDot->TurnOn();
	}
	else
	{
		pDot->TurnOff();
	}
}

CDeLaserDot::CDeLaserDot(void)
{
	m_hTargetEnt = NULL;
	m_bIsOn = true;
	g_DeLaserDotList.Insert(this);
}

CDeLaserDot::~CDeLaserDot(void)
{
	g_DeLaserDotList.Remove(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CLaserDot
//-----------------------------------------------------------------------------
CDeLaserDot* CDeLaserDot::Create(const Vector& origin, CBaseEntity* pOwner, bool bVisibleDot)
{
	CDeLaserDot* pLaserDot = (CDeLaserDot*)CBaseEntity::Create("env_delaserdot", origin, QAngle(0, 0, 0));

	if (pLaserDot == NULL)
		return NULL;

	pLaserDot->m_bVisibleLaserDot = bVisibleDot;
	pLaserDot->SetMoveType(MOVETYPE_NONE);
	pLaserDot->AddSolidFlags(FSOLID_NOT_SOLID);
	pLaserDot->AddEffects(EF_NOSHADOW);
	UTIL_SetSize(pLaserDot, vec3_origin, vec3_origin);

	//Create the graphic
	pLaserDot->SpriteInit("sprites/redglow1.vmt", origin);

	pLaserDot->SetName(AllocPooledString("TEST"));

	pLaserDot->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation);
	pLaserDot->SetScale(0.5f);

	pLaserDot->SetOwnerEntity(pOwner);

	pLaserDot->SetContextThink(&CDeLaserDot::LaserThink, gpGlobals->curtime + 0.1f, g_pDeLaserDotThink);
	pLaserDot->SetSimulatedEveryTick(true);

	if (!bVisibleDot)
	{
		pLaserDot->MakeInvisible();
	}

	return pLaserDot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDeLaserDot::LaserThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.05f, g_pDeLaserDotThink);

	if (GetOwnerEntity() == NULL)
		return;

	Vector	viewDir = GetAbsOrigin() - GetOwnerEntity()->GetAbsOrigin();
	float	dist = VectorNormalize(viewDir);

	float	scale = RemapVal(dist, 32, 1024, 0.01f, 0.5f);
	float	scaleOffs = random->RandomFloat(-scale * 0.25f, scale * 0.25f);

	scale = clamp(scale + scaleOffs, 0.1f, 32.0f);

	SetScale(scale);
}

void CDeLaserDot::SetLaserPosition(const Vector& origin, const Vector& normal)
{
	SetAbsOrigin(origin);
	m_vecSurfaceNormal = normal;
}

Vector CDeLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDeLaserDot::TurnOn(void)
{
	m_bIsOn = true;
	if (m_bVisibleLaserDot)
	{
		BaseClass::TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDeLaserDot::TurnOff(void)
{
	m_bIsOn = false;
	if (m_bVisibleLaserDot)
	{
		BaseClass::TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDeLaserDot::MakeInvisible(void)
{
	BaseClass::TurnOff();
}
