#include "cbase.h"
#include "locksounds.h"
#include "buttons.h"

class CWrenchButton : public CBaseButton
{
	DECLARE_CLASS(CWrenchButton, CBaseButton);
	DECLARE_DATADESC();
public:
	void Spawn(void);
	void ButtonUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(button_wrench, CWrenchButton);

BEGIN_DATADESC(CWrenchButton)
	DEFINE_FUNCTION(ButtonUse),
END_DATADESC();

void CWrenchButton::Spawn(void)
{
	BaseClass::Spawn();
	DevMsg("%s spawned at %f %f %f\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
	AddEffects(EF_NODRAW);
}

void CWrenchButton::ButtonUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pCaller == NULL)
		return;

	if (pCaller->IsPlayer() == false)
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pCaller);
	if (pPlayer == NULL)
		return;

	if (pPlayer->GetActiveWeapon() == NULL)
		return;

	if (FStrEq(pPlayer->GetActiveWeapon()->GetClassname(), "weapon_wrench"))
	{
		pPlayer->GetActiveWeapon()->SendWeaponAnim(ACT_VM_WRENCH_TINKER);
	}

	BaseClass::ButtonUse(pActivator, pCaller, useType, value);
}
