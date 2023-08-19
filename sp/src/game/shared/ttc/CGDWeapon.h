#ifndef CGDWEAPON_H
#define CGDWEAPON_H
#ifdef _WIN32
#pragma once
#endif
#include "cbase.h"
#include "basehlcombatweapon_shared.h"
#include "CDot.h"

#ifdef CLIENT_DLL
#include "c_basehlplayer.h"
#else
#include "player.h"
#endif

#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CGDWeapon C_GDWeapon
#endif


#define SPRITE_MATERIAL "sprites/redglow_mp1.vmt"
#define SPRITE_TRANS kRenderWorldGlow, 255, 0, 0, 255, kRenderFxNoDissipation

#define VECTORGOOD Vector(0.001f, 0.001f, 0.001f)
#define VECTORNOAIM Vector(0.1f, 0.1f, 0.1f)

class CGDWeapon : public  CBaseHLCombatWeapon
{
	DECLARE_CLASS(CGDWeapon, CBaseHLCombatWeapon);
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CGDWeapon(void);
	~CGDWeapon(void);

	CNetworkVar(bool, bDot);
	CNetworkVar(bool, bWasOn);
	CNetworkVar(bool, bInReload);

	bool Deploy(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Precache(void);
	bool Reload(void);
	void ItemPostFrame(void);
	bool Holster(CBaseCombatWeapon *pSwitchingTo);
	void FinishReload(void);
	void WeaponIdle(void);

//---dot-handling---
	inline void ToggleDot(void);
	inline void UpdateDotPos(void);
	inline void SpawnDot(void);
	inline void RemoveDot(void);

	inline bool CanHolster(void);

	inline bool IsDotOn(void);
	inline bool CheckForErrors(void);

	CHandle<CGuidedDot>pSprite;
};

IMPLEMENT_NETWORKCLASS_ALIASED(GDWeapon, DT_GDWeapon);

BEGIN_NETWORK_TABLE(CGDWeapon, DT_GDWeapon)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(bInReload)),
RecvPropBool(RECVINFO(bWasOn)),
RecvPropBool(RECVINFO(bDot)),
#else
SendPropBool(SENDINFO(bInReload)),
SendPropBool(SENDINFO(bWasOn)),
SendPropBool(SENDINFO(bDot)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CGDWeapon)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(bInReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(bWasOn, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(bDot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()


LINK_ENTITY_TO_CLASS(weapon_guided2, CGDWeapon);

PRECACHE_WEAPON_REGISTER(weapon_guided2);

BEGIN_DATADESC(CGDWeapon)
DEFINE_FIELD(bInReload,	FIELD_BOOLEAN),
DEFINE_FIELD(bWasOn,	FIELD_BOOLEAN),
DEFINE_FIELD(bDot,		FIELD_BOOLEAN),
DEFINE_FIELD(pSprite,	FIELD_EHANDLE),
END_DATADESC();
#endif //CGDWEAPON_H