#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"

//#define MOV_BUTTONS IN_LEFT | IN_RIGHT | IN_FORWARD | IN_BACK | IN_JUMP

//how much ammo we want to remove
constexpr auto AMMOCOUNT			= 20;

//bunch of macros to make the life easier, can be changed to whatever you want*
constexpr auto ENTNAME				= "point_displace";		//name of the entity to get the key-value from!!!
constexpr color32 FADEINCOLOUR		= { 0, 200, 0, 255 };	//fade-in colour
constexpr auto RADIUS				= 64.0f;				//radius used in primaryattack()

//Displacer_sv = Displacer_serverside <3... yes I know
class CDisplacer : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CDisplacer, CBaseHLCombatWeapon );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:

	CDisplacer(void);

	void PrimaryAttack(void);
	bool CanHolster(void);
	void WeaponIdle(void);
	void ItemPostFrame(void);

private:

	CBaseEntity* pFound;			// base-entity-point pointer, working with it throughout the code

	bool bInThink		= false;	//are we charging?
	float fNextThink	= 0;		//when's it gonna get charged?
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
void CDisplacer::PrimaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer) return;

	if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 20 )
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

		pPlayer->RemoveAmmo(AMMOCOUNT, m_iPrimaryAmmoType);

		fNextThink = gpGlobals->curtime + 1.25f; //nextthink for when the charging starts

		pPlayer->SetMoveType(MOVETYPE_FLYGRAVITY);	//hack to freeze player movement!

		SendWeaponAnim(ACT_VM_PRIMARYATTACK);
		WeaponSound(SPECIAL1);

		m_flNextPrimaryAttack = gpGlobals->curtime + 3.0f;
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