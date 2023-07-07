#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "in_buttons.h"

#include "rumble_shared.h"
#include "prop_combine_ball.h"

//how much ammo we want to remove
constexpr int AMMOCOUNT_PRIMARY			= 3;
constexpr int AMMOCOUNT_SECONDARY		= 5;

//bunch of macros to make the life easier, can be changed to whatever you want*
constexpr auto			ENTNAME			= "point_displace";									//name of the entity to get the key-value from!!!
constexpr color32		FADEINCOLOUR	= { 0, 200, 0, 255 };								//fade-in colour
constexpr float			RADIUS			= 64.0f;											//search radius

constexpr auto CHARGEUP					= SPECIAL1;
constexpr auto BALLER					= WPN_DOUBLE;
constexpr auto AFTERCHRG				= SPECIAL2;
constexpr auto DENY						= SPECIAL3;

ConVar sk_weapon_dp_alt_fire_radius("sk_weapon_dp_alt_fire_radius", "10");
ConVar sk_weapon_dp_alt_fire_duration("sk_weapon_dp_alt_fire_duration", "2");
ConVar sk_weapon_dp_alt_fire_mass("sk_weapon_dp_alt_fire_mass", "150");

class CDisplacer : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CDisplacer, CBaseHLCombatWeapon );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:

	CDisplacer(void);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	bool CanHolster(void);
	void WeaponIdle(void);
	void ItemPostFrame(void);
	void DelayedAttack(void); //1 for primary, 2 for secondary

private:

	CBaseEntity* pFound;				// base-entity-point pointer, working with it throughout the code

	bool	bInThink		= false;	//are we charging?
	float	fNextThink		= 0;		//when's it gonna get charged?
	int		iModeSwitch		= 0;		//1 for primary, 2 for secondary

	enum
	{
		PRIMARY = 1,
		SECONDARY,
	};
};

IMPLEMENT_SERVERCLASS_ST( CDisplacer, DT_Displacer )
END_SEND_TABLE();

//change the name (weapon_displacer_sv) to fit your needs, don't forget to change it in stubs afterwards, also rename the script file!
LINK_ENTITY_TO_CLASS( weapon_displacer, CDisplacer );

PRECACHE_WEAPON_REGISTER( weapon_displacer );

//save-restore
BEGIN_DATADESC( CDisplacer )
DEFINE_FIELD(pFound, FIELD_CLASSPTR),
DEFINE_FIELD(bInThink, FIELD_BOOLEAN),
DEFINE_FIELD(fNextThink, FIELD_FLOAT),
DEFINE_FIELD(iModeSwitch, FIELD_INTEGER),
END_DATADESC();

CDisplacer::CDisplacer(void)
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

	pFound				= 0;
	bInThink			= false;
	fNextThink			= 0;
	iModeSwitch			= 0;
}

void CDisplacer::SecondaryAttack(void)
{
	if (bInThink)
		return;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
		return;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) < AMMOCOUNT_SECONDARY)
	{
		WeaponSound(DENY);
		pOwner->SetNextAttack(gpGlobals->curtime + 1.5f);

		return;
	}

	auto pEnt = gEntList.FindEntityByClassnameNearest(ENTNAME, pOwner->GetAbsOrigin(), RADIUS); //find our entity in a 64 unit radius
	if (!pEnt)
	{
		WeaponSound(DENY);
		pOwner->SetNextAttack(gpGlobals->curtime + 1.5f);

		return;
	}

	pOwner->RemoveAmmo(AMMOCOUNT_PRIMARY, m_iPrimaryAmmoType);
	pOwner->AddFlag( FL_FROZEN | FL_ATCONTROLS ); //disable player's movement

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	WeaponSound(CHARGEUP);

	fNextThink	= gpGlobals->curtime + 1.5f; //schedule teleport

	pFound		= pEnt;			//assign a pointer to it

	iModeSwitch = SECONDARY;	//set our mode to secondary attack

	bInThink	= true;			//thinking!

	//pOwner->SetNextAttack() breaks the thinking logic?!
	m_flNextSecondaryAttack = gpGlobals->curtime + 3.0f;
	m_flNextPrimaryAttack	= gpGlobals->curtime + 3.0f;
}

void CDisplacer::PrimaryAttack(void)
{
	if (bInThink)
		return;

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) < AMMOCOUNT_PRIMARY)
	{
		WeaponSound(DENY);
		pPlayer->SetNextAttack(gpGlobals->curtime + 1.5f);
		return;
	}

	pPlayer->RumbleEffect(RUMBLE_AR2_ALT_FIRE, 0, RUMBLE_FLAG_RESTART);
#ifdef MAPBASE
	pPlayer->SetAnimation(PLAYER_ATTACK2);
#endif

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	WeaponSound(CHARGEUP);

	// Decrease ammo
	pPlayer->RemoveAmmo(AMMOCOUNT_PRIMARY, m_iPrimaryAmmoType);

	fNextThink	= gpGlobals->curtime + SequenceDuration(); //schedule ball firing

	iModeSwitch = PRIMARY;		//set our mode to primary attack

	bInThink	= true;			//thinking!

	pPlayer->SetNextAttack(gpGlobals->curtime + SequenceDuration());
}

void CDisplacer::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (bInThink && fNextThink < gpGlobals->curtime)
	{
		DelayedAttack();
	}
}
void CDisplacer::DelayedAttack(void)
{
	switch (iModeSwitch)
	{
		case PRIMARY:
		{
			CBasePlayer *pOwner = ToBasePlayer(GetOwner());

			if (!pOwner)
				return;

			// Deplete the clip completely
			SendWeaponAnim(ACT_VM_PRIMARYATTACK);

			// Register a muzzleflash for the AI
			pOwner->DoMuzzleFlash();
			pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

			WeaponSound(BALLER);

			pOwner->RumbleEffect(RUMBLE_SHOTGUN_DOUBLE, 0, RUMBLE_FLAG_RESTART);

			// Fire the bullets
			Vector vecSrc = pOwner->Weapon_ShootPosition();
			Vector vecAiming = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
			Vector impactPoint = vecSrc + (vecAiming * MAX_TRACE_LENGTH);

			// Fire the bullets
			Vector vecVelocity = vecAiming * 1000.0f;

			// Fire the combine ball
			auto pBalls = CreateCombineBall(vecSrc,
				vecVelocity,
				sk_weapon_dp_alt_fire_radius.GetFloat(),
				sk_weapon_dp_alt_fire_mass.GetFloat(),
				sk_weapon_dp_alt_fire_duration.GetFloat(),
				pOwner);

			if (pBalls)
			{
				auto pBallsReal = static_cast<CPropCombineBall*>(pBalls);
				if (pBallsReal)
				{
					pBallsReal->SetMaxBounces(-1);
					pBallsReal->SetModelScale(20);
					pBallsReal->SetIsFromDisplacer(true);
				}
			}

			// View effects
			color32 white = { 255, 255, 255, 64 };
			UTIL_ScreenFade(pOwner, white, 0.1, 0, FFADE_IN);

			//Disorient the player
			QAngle angles = pOwner->GetLocalAngles();

			angles.x += random->RandomFloat(-4, 4);
			angles.y += random->RandomFloat(-4, 4);
			angles.z = 0;

			pOwner->SnapEyeAngles(angles);

			pOwner->ViewPunch(QAngle(RandomInt(-8, -12), RandomInt(1, 2), 0));

			// Can shoot again immediately
			pOwner->SetNextAttack(gpGlobals->curtime + 1.0f);

			bInThink = false;

			fNextThink	= 0;
			iModeSwitch = 0;

			break;
		}
		case SECONDARY:
		{
			variant_t temp;

			pFound->ReadKeyField("target", &temp);

			auto keyval = temp.String();

			DevWarning("The entity name is %s.\n", pFound->GetDebugName());

			DevWarning("keyval is %s.\n", keyval);

			auto ptemp = gEntList.FindEntityByName(NULL, keyval);
			if (!ptemp)
			{
				DevWarning("Invalid entity!\n");

				bInThink = false; //stop thinking!

				fNextThink	= 0;
				iModeSwitch = 0;

				return;
			}

			CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

			pPlayer->SetAbsOrigin(ptemp->GetAbsOrigin() - Vector(0.0f, 0.0f, 16.0f)); //push the player a bit downwards...

			pPlayer->SetAbsVelocity(Vector(0, 0, 0));
			pPlayer->SetAbsAngles(QAngle(0, 0, 0));

			WeaponSound(AFTERCHRG);

			UTIL_ScreenFade(pPlayer, FADEINCOLOUR, 0.5f, 0.1f, FFADE_IN);

			pPlayer->RemoveFlag( FL_FROZEN | FL_ATCONTROLS);						//reset back player's movement

			bInThink = false;

			fNextThink	= 0;
			iModeSwitch = 0;

			break;
		}
	}
}

bool CDisplacer::CanHolster(void)
{
	if (bInThink)
		return false;

	return BaseClass::CanHolster();
}

void CDisplacer::WeaponIdle(void)
{
	if (HasWeaponIdleTimeElapsed() && !bInThink)
	{
		SendWeaponAnim(ACT_VM_IDLE);

		SetWeaponIdleTime(gpGlobals->curtime + 3.0f);
	}
}