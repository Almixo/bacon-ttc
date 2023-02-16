#include "cbase.h"

#define VECTORMIN Vector(-48, -48, -48) // point_displace size
#define VECTORMAX Vector(48, 48, 48)	// point_displace size

class CDispPoint : public CPointEntity
{
	DECLARE_CLASS( CDispPoint, CPointEntity );
	DECLARE_DATADESC();

private:
	void Spawn( void );

	string_t string; // TODO: maybe replace this with const char* !
};

void CDispPoint::Spawn( void )
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetSize( VECTORMIN, VECTORMAX );

	Vector vOrg = GetAbsOrigin();
	DevWarning( "%s spawned at %g %g %g!\n", GetDebugName(), vOrg.x, vOrg.y, vOrg.z );

	SetNextThink( TICK_NEVER_THINK );
}

LINK_ENTITY_TO_CLASS( point_displace, CDispPoint ); //if you change the name(point_displace) to something else, don't forget to change it in displacer's code too!

BEGIN_DATADESC( CDispPoint )
DEFINE_KEYFIELD( string, FIELD_STRING, "target" ),
END_DATADESC();
