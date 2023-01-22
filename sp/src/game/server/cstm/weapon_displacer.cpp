#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "in_buttons.h"

#include "rumble_shared.h"
#include "prop_combine_ball.h"

//#define MOV_BUTTONS IN_LEFT | IN_RIGHT | IN_FORWARD | IN_BACK | IN_JUMP

//how much ammo we want to remove
#define AMMOCOUNT_PRIMARY    3
#define AMMOCOUNT_SECONDARY    5

//bunch of macros to make the life easier, can be changed to whatever you want*
#define ENTNAME            "point_displace"                                //name of the entity to get the key-value from!!!
#define FADEINCOLOUR    { 0, 200, 0, 255 }                                //fade-in colour
#define RADIUS            64.0f 

ConVar sk_weapon_dp_alt_fire_radius("sk_weapon_dp_alt_fire_radius", "10");
ConVar sk_weapon_dp_alt_fire_duration("sk_weapon_dp_alt_fire_duration", "2");
ConVar sk_weapon_dp_alt_fire_mass("sk_weapon_dp_alt_fire_mass", "150");

//Displacer_sv = Displacer_serverside <3... yes I know
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
	void DelayedAttack();

private:

	CBaseEntity* pFound;			// base-entity-point pointer, working with it throughout the code

	bool bInThink		= false;	//are we charging?
	float fNextThink	= 0;		//when's it gonna get charged?

	float					m_flDelayedFire;
	bool					m_bShotDelayed;
	int						m_nVentPose;
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
END_DATADESC();

CDisplacer::CDisplacer(void)
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

	fNextThink			= 0;
	pFound				= NULL;
}

void CDisplacer::PrimaryAttack()
{
	if (m_bShotDelayed)
		return;

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) < AMMOCOUNT_PRIMARY)
	{
		WeaponSound(SPECIAL3);
		pPlayer->SetNextAttack(gpGlobals->curtime + 1.5f);
		return;
	}

	// Cannot fire underwater
	if (GetOwner() && GetOwner()->GetWaterLevel() == 3)
	{
		SendWeaponAnim(ACT_VM_PRIMARYATTACK);
		BaseClass::WeaponSound(EMPTY);
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	m_bShotDelayed = true;

	if (pPlayer)
	{
		pPlayer->RumbleEffect(RUMBLE_AR2_ALT_FIRE, 0, RUMBLE_FLAG_RESTART);
#ifdef MAPBASE
		pPlayer->SetAnimation(PLAYER_ATTACK2);
#endif
	}

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	WeaponSound(SPECIAL1);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flDelayedFire = gpGlobals->curtime + SequenceDuration();
	m_iSecondaryAttacks++;
}

void CDisplacer::DelayedAttack()
{
	m_bShotDelayed = false;

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Deplete the clip completely
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	m_flNextSecondaryAttack = pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();

	// Register a muzzleflash for the AI
	pOwner->DoMuzzleFlash();
	pOwner->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	WeaponSound(WPN_DOUBLE);

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

	angles.x += random->RandomInt(-4, 4);
	angles.y += random->RandomInt(-4, 4);
	angles.z = 0;

	pOwner->SnapEyeAngles(angles);

	pOwner->ViewPunch(QAngle(random->RandomInt(-8, -12), random->RandomInt(1, 2), 0));

	// Decrease ammo
	pOwner->RemoveAmmo(AMMOCOUNT_PRIMARY, m_iPrimaryAmmoType);

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
}

void CDisplacer::SecondaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer) return;

	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) < AMMOCOUNT_SECONDARY)
	{
		WeaponSound(SPECIAL3);
		pPlayer->SetNextAttack(gpGlobals->curtime + 1.5f);
		return;
	}

	auto pEnt = gEntList.FindEntityByClassnameNearest(ENTNAME, pPlayer->GetAbsOrigin(), RADIUS);	//trying to find "point_displace" in 64 unit radius
	if (pEnt && !bInThink)
	{
		pFound = pEnt; //assign point_displace-or-whatever pointer

		bInThink = true; //we don't want to switch current weapon now, do we?

		DevWarning("waiting for itempostframe()!\n");	//spews an error in console, can be removed

		pPlayer->RemoveAmmo(AMMOCOUNT_SECONDARY, m_iPrimaryAmmoType);

		fNextThink = gpGlobals->curtime + 1.25f; //nextthink for when the charging starts

		pPlayer->SetMoveType(MOVETYPE_FLYGRAVITY);	//hack to freeze player movement!

		SendWeaponAnim(ACT_VM_PRIMARYATTACK);
		WeaponSound(SPECIAL1);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	WeaponSound(SPECIAL3);
	pPlayer->m_flNextAttack = gpGlobals->curtime + 1.5f;
}
bool CDisplacer::CanHolster(void)
{
	if (bInThink)
		return FALSE; //we don't want the player to switch weapons while charging!

	return BaseClass::CanHolster(); //otherwise default behavior
}
void CDisplacer::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	// See if we need to fire off our secondary round
	if (m_bShotDelayed && gpGlobals->curtime > m_flDelayedFire)
	{
		DelayedAttack();
	}

	if (bInThink) //should we be charging?
	{
		if (fNextThink < gpGlobals->curtime) //check if some time has passed
		{
			variant_t temp;

			pFound->ReadKeyField("target", &temp); //much better, doesn't care about entity's name as long as it has its pointer

			const char *keyval = temp.String(); //maybe make it constn't?
//			^^^ finally, no more depression from GetKeyValue()!

			DevWarning("The entity name is %s.\n", pFound->GetDebugName()); //spews warning into console, can be removed

			DevWarning("keyval is %s.\n", keyval); //spews warning into console, can be removed

			auto ptemp = gEntList.FindEntityByName(NULL, keyval); //trying to find the last entity e.g. the entity to teleport to.
			if (!ptemp)
			{
				DevWarning("Invalid entity!\n");

				bInThink = false; //stop thinking!

				return;
			}

			CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

			pPlayer->SetAbsOrigin(ptemp->GetAbsOrigin() - Vector(0.0f, 0.0f, 16.0f)); //push the player a bit downwards...
			pPlayer->SetAbsAngles(ptemp->GetAbsAngles()); //doesn't seem to work, wtf?

			pPlayer->SetAbsVelocity(Vector(0, 0, 0));
			pPlayer->SetAbsAngles(QAngle(0, 0, 0));

			WeaponSound(SPECIAL2);

			UTIL_ScreenFade(pPlayer, FADEINCOLOUR, 0.5f, 0.1f, FFADE_IN);

			pPlayer->SetMoveType(MOVETYPE_WALK); //reset player's movement to default

			DevWarning("itempostframe() portion done!\n"); //spews error in the console, can be removed

			bInThink = false; //stop thinking!
		}
	}
}
void CDisplacer::WeaponIdle(void) //made it so it ain't crap
{
	if (HasWeaponIdleTimeElapsed() && !bInThink)
	{
		SendWeaponAnim(ACT_VM_IDLE);

		SetWeaponIdleTime(gpGlobals->curtime + 3.0f);
	}
}