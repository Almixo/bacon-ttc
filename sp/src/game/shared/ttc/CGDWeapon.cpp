#include "cbase.h"
#include "CGDWeapon.h"
#ifdef CLIENT_DLL
#include "view.h"
#endif

#define TRend tr.endpos + (tr.plane.normal * 2.0f)

#define DOTON gpGlobals->curtime + 0.75f;
#define DOTOFF gpGlobals->curtime + 0.25f;
//#define GETPLAYER CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

CGDWeapon::CGDWeapon()
{
	m_bFiresUnderwater	= false;
	m_bReloadsSingly	= false;

	bDot				= false;
	bWasOn				= false;
	bInReload			= false;
}
CGDWeapon::~CGDWeapon()
{
	if (pSprite != NULL)
	{
		pSprite->TurnOff();
		pSprite->Remove();
		pSprite = NULL;
	}
}
void CGDWeapon::Precache(void)
{
	BaseClass::Precache();
}
bool CGDWeapon::Deploy(void)
{
	RemoveDot();

	return BaseClass::Deploy();
}
void CGDWeapon::WeaponIdle(void)
{
	BaseClass::WeaponIdle();

	bInReload = false;

	SpawnDot();

	if (!bDot && bWasOn)
	{
		UpdateDotPos();
		ToggleDot();

		bWasOn = false;
	}
}
void CGDWeapon::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer) return;

	if (m_iClip1 <= 0)
	{
		if (!m_bFireOnEmpty) Reload();

		else
		{
			WeaponSound(EMPTY);
			m_flNextEmptySoundTime = 1.0f;
			m_flNextPrimaryAttack = 0.15f;
		}

		return;
	}

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	Vector vSpread;
	float wait;

	if (bDot) {
		wait = DOTON
		vSpread = VECTORGOOD;
	}
	else {
		wait = DOTOFF;
		vSpread = VECTORNOAIM;
	}

	pPlayer->FireBullets(FireBulletsInfo_t(1, vecSrc, vecAiming, vSpread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType));

	pPlayer->m_flNextAttack = wait;


	//EjectShell(pPlayer, 0);

#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5f);
#endif

	pPlayer->ViewPunch(QAngle(-1.5, 0, 0));

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}
void CGDWeapon::SecondaryAttack(void)
{
	ToggleDot();

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75f;
	m_flNextEmptySoundTime = gpGlobals->curtime + 1.0f;
}
bool CGDWeapon::Reload(void)
{
	bool fRet;

	if (m_iClip1 == 0)
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_EMPTY);
	else
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

	if (fRet)
	{
		bInReload = true;
		
		WeaponSound(RELOAD);

		if (bDot && IsDotOn())
		{
			ToggleDot();

			bWasOn = true;
		}
	}
	return fRet;
}
void CGDWeapon::FinishReload(void)
{
	BaseClass::FinishReload();

	if (!bDot && bWasOn)
	{
		SpawnDot();
		UpdateDotPos();
		ToggleDot();

		bWasOn = false;
	}
}
void CGDWeapon::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	UpdateDotPos();
}
bool CGDWeapon::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	if (bDot)
	{
		ToggleDot();
		RemoveDot();

		bWasOn = true;
	}

	return BaseClass::Holster(pSwitchingTo);
}
void CGDWeapon::UpdateDotPos(void)
{
#ifndef CLIENT_DLL
	if (!CheckForErrors()) return;

	if (!bDot) return;

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	Vector vOrigin, vDir, vEnd;
	trace_t tr;

	vOrigin = pPlayer->Weapon_ShootPosition();
	AngleVectors(pPlayer->EyeAngles() + pPlayer->m_Local.m_vecPunchAngle, &vDir);

	UTIL_TraceLine(vOrigin, vOrigin + (vDir * MAX_TRACE_LENGTH), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	vEnd = tr.endpos - (vDir * 10);

	pSprite->SetDotPosition(vEnd, tr.plane.normal);
#endif
}
void CGDWeapon::SpawnDot(void)
{
#ifndef CLIENT_DLL
	if (!pSprite)
	{
		pSprite = CGuidedDot::Create(GetAbsOrigin(), GetOwner());

		if (bDot && bWasOn)
			WeaponSound(SPECIAL1);
	}
#endif
}
inline void CGDWeapon::ToggleDot(void)
{
	if (!CheckForErrors()) return;

	if (bDot && !IsDotOn())
	{
		DevWarning("Dot's broken in ToggleDot()! --> bDot && !IsDotOn()!\n");
		pSprite->TurnOn();
		return;
	}

	bDot = !bDot;

	if (bDot)
	{
		pSprite->TurnOn();
		WeaponSound(SPECIAL1);
	}
	if (!bDot)
	{
		pSprite->TurnOff();
		WeaponSound(SPECIAL2);
	}
}
inline bool CGDWeapon::IsDotOn(void)
{
	if (!pSprite)
	{
		DevWarning("Got error in IsDotOn()!\n");
		return false;
	}

	return pSprite->IsOn();
}
inline bool CGDWeapon::CheckForErrors(void)
{
	if (!pSprite)
	{
		return false;
	}

	if (bDot && !IsDotOn())
	{
		return false;
	}

	return true;
}
inline void CGDWeapon::RemoveDot(void)
{
	if (pSprite != NULL)
	{
		pSprite->Remove();
	}
}
inline bool CGDWeapon::CanHolster(void)
{
	if (bInReload)
		return FALSE;
	if (m_bInReload)
		return FALSE;

	return BaseClass::CanHolster();
}