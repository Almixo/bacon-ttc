#include "cbase.h"
#include "ai_basenpc.h"
#include "actanimating.h"
#include "npcevent.h"
#include "eventqueue.h"

constexpr auto ATTCKMISS	= "XenTree.AttackMiss";
constexpr auto ATTCKHIT		= "XenTree.AttackHit";

class CActAnimating : public CBaseAnimating
{
public:
	DECLARE_CLASS( CActAnimating, CBaseAnimating );

	void			SetActivity( Activity act );
	inline Activity	GetActivity( void ) { return m_Activity; }

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_DATADESC();

private:
	Activity	m_Activity;
};

class CXenTreeTrigger : public CBaseEntity
{
	DECLARE_CLASS( CXenTreeTrigger, CBaseEntity );
public:
	void		Touch( CBaseEntity *pOther );
	static CXenTreeTrigger *TriggerCreate( CBaseEntity *pOwner, const Vector &position );
};
LINK_ENTITY_TO_CLASS( xen_ttrigger, CXenTreeTrigger );

CXenTreeTrigger *CXenTreeTrigger::TriggerCreate( CBaseEntity *pOwner, const Vector &position )
{
	CXenTreeTrigger *pTrigger = CREATE_ENTITY( CXenTreeTrigger, "xen_ttrigger" ); 
	pTrigger->SetAbsOrigin( position );

	pTrigger->SetSolid( SOLID_BBOX );
	pTrigger->AddSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	pTrigger->SetMoveType( MOVETYPE_NONE );
	pTrigger->SetOwnerEntity( pOwner );

	return pTrigger;
}


void CXenTreeTrigger::Touch( CBaseEntity *pOther )
{
	if ( GetOwnerEntity() )
	{
		GetOwnerEntity()->Touch( pOther );
	}
}

#define TREE_AE_ATTACK		1

class CXenTree : public CActAnimating
{
	DECLARE_CLASS( CXenTree, CActAnimating );
public:
	void		Spawn( void );
	void		Precache( void );
	void		Touch( CBaseEntity *pOther );
	void		Think( void );
	int			OnTakeDamage( const CTakeDamageInfo &info ) { Attack(); return 0; }
	void		HandleAnimEvent( animevent_t *pEvent );
	void		Attack( void );	
	Class_T		Classify( void ) { return CLASS_BARNACLE; }

	DECLARE_DATADESC();

private:
	CXenTreeTrigger	*m_pTrigger;
};

LINK_ENTITY_TO_CLASS( xen_tree, CXenTree );

BEGIN_DATADESC( CXenTree )
	DEFINE_FIELD( m_pTrigger, FIELD_CLASSPTR ),
END_DATADESC()

void CXenTree::Spawn( void )
{
	Precache();

	SetModel( "models/tree.mdl" );
	SetMoveType( MOVETYPE_NONE );
	SetSolid ( SOLID_BBOX );

	m_takedamage = DAMAGE_YES;

	SetSize( Vector(-30,-30,0), Vector(30,30,188) );
	SetActivity( ACT_IDLE );
	SetNextThink( gpGlobals->curtime + 0.1 );
	SetCycle( RandomFloat( 0,1 ) );
	m_flPlaybackRate = RandomFloat( 0.7, 1.4 );

	Vector triggerPosition, vForward;

	AngleVectors( GetAbsAngles(), &vForward );
	triggerPosition = GetAbsOrigin() + (vForward * 64);
	
	// Create the trigger
	m_pTrigger = CXenTreeTrigger::TriggerCreate( this, triggerPosition );
	UTIL_SetSize( m_pTrigger, Vector( -24, -24, 0 ), Vector( 24, 24, 128 ) );
}

void CXenTree::Precache( void )
{
	PrecacheModel( "models/tree.mdl" );
//	PrecacheModel( XEN_PLANT_GLOW_SPRITE ); //why even?

	PrecacheScriptSound( ATTCKMISS );
	PrecacheScriptSound( ATTCKHIT );
}


void CXenTree::Touch( CBaseEntity *pOther )
{
	if (!pOther)
		return;

	Attack();
}


void CXenTree::Attack( void )
{
	if ( GetActivity() == ACT_IDLE )
	{
		SetActivity( ACT_MELEE_ATTACK1 );
		m_flPlaybackRate = RandomFloat( 1.0, 1.4 );

		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), ATTCKMISS );
	}
}

void CXenTree::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case TREE_AE_ATTACK:
		{
			CBaseEntity *pList[8];
			bool sound = false;
			int count = UTIL_EntitiesInBox( pList, 8, m_pTrigger->GetAbsOrigin() + m_pTrigger->WorldAlignMins(), m_pTrigger->GetAbsOrigin() +  m_pTrigger->WorldAlignMaxs(), FL_NPC|FL_CLIENT );

			Vector forward;
			AngleVectors( GetAbsAngles(), &forward );

			for ( int i = 0; i < count; i++ )
			{
				if ( pList[i] != this )
				{
					if ( pList[i]->GetOwnerEntity() != this )
					{
						sound = true;
						pList[i]->TakeDamage( CTakeDamageInfo(this, this, 25, DMG_CRUSH | DMG_SLASH ) );
						pList[i]->ViewPunch( QAngle( 15, 0, 18 ) );

						pList[i]->SetAbsVelocity( pList[i]->GetAbsVelocity() + forward * 100 );
					}
				}
			}
					
			if ( sound )
			{
				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), ATTCKHIT );
			}
		}
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

void CXenTree::Think( void )
{
	StudioFrameAdvance();
	SetNextThink( gpGlobals->curtime + 0.1 );
	DispatchAnimEvents( this );

	switch( GetActivity() )
	{
	case ACT_MELEE_ATTACK1:
		if ( IsSequenceFinished() )
		{
			SetActivity( ACT_IDLE );
			m_flPlaybackRate = RandomFloat( 0.6f, 1.4f );
		}
		break;

	default:
	case ACT_IDLE:
		break;

	}
}