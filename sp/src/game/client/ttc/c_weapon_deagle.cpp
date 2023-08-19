//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "c_basehlcombatweapon.h"
#include "beamdraw.h"
#include "view.h"
#include "debugoverlay_shared.h"

class C_WeaponDeagle : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS(C_WeaponDeagle, C_BaseHLCombatWeapon);
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual int DrawModel(int flags);
	virtual void ClientThink(void);

	virtual bool ShouldUseLargeViewModelVROverride() OVERRIDE { return true; }
private:
	bool m_bGuiding;

	CMaterialReference pMat;
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_deagle, C_WeaponDeagle);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponDeagle, DT_WeaponDeagle, CWeaponDeagle)
	RecvPropBool(RECVINFO(m_bGuiding)),
END_RECV_TABLE();

void C_WeaponDeagle::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		pMat.Init("effects/combinemuzzle1", TEXTURE_GROUP_CLIENT_EFFECTS);
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}
void C_WeaponDeagle::ClientThink(void)
{
	BaseClass::ClientThink();

	C_BasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;
	
	// Get the sprite rendering position.
	Vector vecEndPos;
	const float fSize = 8;
	color32 color = { 255, 255, 255, 255 };

	if (!pPlayer->IsDormant())
	{
		Vector vecSrc, vecDir, vecLength;

		// Always draw the dot in front of our faces when in first-person.
		if (pPlayer->IsLocalPlayer())
		{
			// Take our view position and orientation
			vecSrc = CurrentViewOrigin();
			vecDir = CurrentViewForward();
		}
		else
		{
			// Take the owning player eye position and direction.
			vecSrc = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors(angles, &vecDir);
		}

		trace_t	trace;
		UTIL_TraceLine(vecSrc, vecSrc + (vecDir * 8192), MASK_SHOT /*MASK_SOLID*/, pPlayer, COLLISION_GROUP_NONE, &trace);

		// Backup off the hit plane, towards the source
		vecEndPos = trace.endpos + vecDir * -4;

		vecLength = vecSrc - vecEndPos;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}


	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(pMat, this);

	DrawSprite(vecEndPos, fSize, fSize, color);

	//NDebugOverlay::Box(vecEndPos, Vector(-4, -4, -4), Vector(4, 4, 4), 255, 0, 0, 64, 0.5);
}
int C_WeaponDeagle::DrawModel(int flags)
{
	return BaseClass::DrawModel(flags);
}
