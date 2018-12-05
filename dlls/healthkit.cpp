/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "wallcharger.h"

extern int gmsgItemPickup;

class CHealthKit : public CItem
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
/*
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
*/
};

LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit )

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] =
{

};

IMPLEMENT_SAVERESTORE( CHealthKit, CItem )
*/

void CHealthKit::Spawn( void )
{
	Precache();
	SetMyModel( "models/w_medkit.mdl" );

	CItem::Spawn();
}

void CHealthKit::Precache( void )
{
	PrecacheMyModel( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if( pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING( pev->classname ) );
		MESSAGE_END();

		EMIT_SOUND( ENT( pPlayer->pev ), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM );

		if( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove( this );
		}

		return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------
// Base class for wall chargers
//-------------------------------------------------------------

#define SF_WALLCHARGER_STARTOFF 1

void CWallCharger::Spawn()
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );		// set size and link into world
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	if (FBitSet(pev->spawnflags, SF_WALLCHARGER_STARTOFF))
	{
		m_iJuice = 0;
		pev->frame = 1;
	}
	else
	{
		m_iJuice = ChargerCapacity();
		pev->frame = 0;
	}
}

void CWallCharger::Precache()
{
	PRECACHE_SOUND( ChargeStartSound() );
	PRECACHE_SOUND( DenySound() );
	PRECACHE_SOUND( LoopingSound() );
	const char* rechargeSound = RechargeSound();
	if (rechargeSound)
		PRECACHE_SOUND(rechargeSound);
}

int CWallCharger::ObjectCaps( void )
{
	return ( CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION;
}

void CWallCharger::Off()
{
	// Stop looping sound.
	if( m_iOn > 1 )
		STOP_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound() );

	m_iOn = 0;

	SetThink( &CBaseEntity::SUB_DoNothing );
	if ( m_iJuice <= 0 )
	{
		if ( ( m_iReactivate = RechargeTime() ) > 0 )
		{
			pev->nextthink = pev->ltime + m_iReactivate;
			SetThink( &CWallCharger::Recharge );
		}
	}
}

void CWallCharger::Recharge( void )
{
	if (m_triggerOnRecharged)
	{
		FireTargets( STRING( m_triggerOnRecharged ), this, this, USE_TOGGLE, 0 );
	}
	const char* rechargeSound = RechargeSound();
	if (rechargeSound)
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, rechargeSound, 1.0, ATTN_NORM );
	m_iJuice = ChargerCapacity();
	pev->frame = 0;
	SetThink( &CBaseEntity::SUB_DoNothing );
}

const char* CWallCharger::LoopingSound()
{
	return pev->noise ? STRING(pev->noise) : DefaultLoopingSound();
}
const char* CWallCharger::DenySound()
{
	return pev->noise1 ? STRING(pev->noise1) : DefaultDenySound();
}
const char* CWallCharger::ChargeStartSound()
{
	return pev->noise2 ? STRING(pev->noise2) : DefaultChargeStartSound();
}
const char* CWallCharger::RechargeSound()
{
	return pev->noise3 ? STRING(pev->noise3) : DefaultRechargeSound();
}

TYPEDESCRIPTION CWallCharger::m_SaveData[] =
{
	DEFINE_FIELD( CWallCharger, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_iReactivate, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( CWallCharger, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallCharger, m_triggerOnFirstUse, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnEmpty, FIELD_STRING ),
	DEFINE_FIELD( CWallCharger, m_triggerOnRecharged, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CWallCharger, CBaseEntity )

void CWallCharger::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "style" ) ||
		FStrEq( pkvd->szKeyName, "height" ) ||
		FStrEq( pkvd->szKeyName, "value1" ) ||
		FStrEq( pkvd->szKeyName, "value2" ) ||
		FStrEq( pkvd->szKeyName, "value3" ) )
	{
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "dmdelay" ) )
	{
		m_iReactivate = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnEmpty" ) )
	{
		m_triggerOnEmpty = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnRecharged" ) )
	{
		m_triggerOnRecharged = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TriggerOnFirstUse" ) )
	{
		m_triggerOnFirstUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CWallCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!pActivator || !pActivator->IsPlayer())
	{
		if (useType == USE_TOGGLE)
		{
			useType = m_iJuice > 0 ? USE_OFF : USE_ON;
		}
		switch (useType) {
		case USE_OFF:
			if (m_iJuice > 0)
			{
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
				m_iJuice = 0;
				pev->frame = 1;
				Off();
			}
			return;
		case USE_ON:
			if (m_iJuice <= 0)
			{
				Recharge();
			}
			return;
		default:
			return;
		}
	}

	// if there is no juice left, turn it off
	if( m_iJuice <= 0 )
	{
		if (m_triggerOnEmpty && pev->frame == 0)
		{
			FireTargets( STRING( m_triggerOnEmpty ), this, this, USE_TOGGLE, 0 );
		}
		pev->frame = 1;
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink( &CWallCharger::Off );

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;

	// charge the player
	if( GiveCharge(pActivator) )
	{
		if (m_triggerOnFirstUse)
		{
			FireTargets( STRING( m_triggerOnFirstUse ), this, this, USE_TOGGLE, 0 );
			m_triggerOnFirstUse = 0;
		}
		m_iJuice--;
	}
	else
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, DenySound(), SoundVolume(), ATTN_NORM );
		}
		if( m_iOn > 1 )
			STOP_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound() );
		m_iOn = 0;
		return;
	}

	// Play the on sound or the looping charging sound
	if( !m_iOn )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, ChargeStartSound(), 1.0, ATTN_NORM );
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if( ( m_iOn == 1 ) && ( m_flSoundTime <= gpGlobals->time ) )
	{
		m_iOn++;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, LoopingSound(), 1.0, ATTN_NORM );
	}
}

//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CWallCharger
{
public:
	const char* DefaultLoopingSound() { return "items/medcharge4.wav"; }
	int RechargeTime() { return (int)g_pGameRules->FlHealthChargerRechargeTime(); }
	const char* DefaultRechargeSound() { return "items/medshot4.wav"; }
	int ChargerCapacity() { return (int)gSkillData.healthchargerCapacity; }
	const char* DefaultDenySound() { return "items/medshotno1.wav"; }
	const char* DefaultChargeStartSound() { return "items/medshot4.wav"; }
	float SoundVolume() { return 1.0f; }
	bool GiveCharge(CBaseEntity* pActivator)
	{
		return pActivator->TakeHealth( 1, DMG_GENERIC ) > 0;
	}
};

LINK_ENTITY_TO_CLASS( func_healthcharger, CWallHealth )

//-------------------------------------------------------------
// Wall mounted health kit (PS2 && Decay)
//-------------------------------------------------------------

class CWallHealthJarDecay : public CBaseAnimating
{
public:
	void Spawn();
};

void CWallHealthJarDecay::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_both.mdl");
	pev->renderamt = 180;
	pev->rendermode = kRenderTransTexture;
	InitBoneControllers();
}

LINK_ENTITY_TO_CLASS(item_healthcharger_jar, CWallHealthJarDecay)

class CWallHealthDecay : public CBaseAnimating
{
public:
	void Spawn();
	void Precache(void);
	void EXPORT SearchForPlayer();
	void EXPORT Off( void );
	void EXPORT Recharge( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps( void ) { return ( CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION; }
	void TurnNeedleToPlayer(const Vector &player);
	void SetNeedleState(int state);
	void SetNeedleController(float yaw);
	void UpdateOnRemove();
	void UpdateJar()
	{
		if (m_jar)
		{
			const float jarBoneControllerValue = (m_iJuice / gSkillData.healthchargerCapacity) * 11 - 11;
			m_jar->SetBoneController(0,  jarBoneControllerValue );
		}
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	enum {
		Still,
		Deploy,
		Idle,
		GiveShot,
		Healing,
		RetractShot,
		RetractArm,
		Inactive
	};

	float m_flNextCharge;
	int m_iJuice;
	int m_iState;
	float m_flSoundTime;
	CWallHealthJarDecay* m_jar;
	BOOL m_playingChargeSound;

protected:
	void SetMySequence(const char* sequence);
};

TYPEDESCRIPTION CWallHealthDecay::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealthDecay, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_iJuice, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CWallHealthDecay, m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CWallHealthDecay, m_playingChargeSound, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE( CWallHealthDecay, CBaseAnimating )

void CWallHealthDecay::Spawn()
{
	m_iJuice = gSkillData.healthchargerCapacity;
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_body.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(pev, pev->origin);
	pev->skin = 0;

	InitBoneControllers();

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CWallHealthDecay::SearchForPlayer);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_iState = Inactive;
	}
}

LINK_ENTITY_TO_CLASS(item_healthcharger, CWallHealthDecay)

void CWallHealthDecay::Precache(void)
{
	PRECACHE_MODEL("models/health_charger_body.mdl");
	PRECACHE_MODEL("models/health_charger_both.mdl");
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
	PRECACHE_SOUND( "items/medcharge4.wav" );

	m_jar = GetClassPtr( (CWallHealthJarDecay *)NULL );
	if (m_jar)
	{
		m_jar->Spawn();
		UTIL_SetOrigin( m_jar->pev, pev->origin );
		m_jar->pev->angles = pev->angles;
		UpdateJar();
	}
}

void CWallHealthDecay::SearchForPlayer()
{
	StudioFrameAdvance();
	CBaseEntity* pEntity = 0;
	float delay = 0.05;
	UTIL_MakeVectors( pev->angles );
	while((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), 64)) != 0) { // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer() && pEntity->IsAlive()) {
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
				continue;
			}
			TurnNeedleToPlayer(pEntity->pev->origin);
			switch (m_iState) {
			case RetractShot:
				SetNeedleState(Idle);
				break;
			case RetractArm:
				SetNeedleState(Deploy);
				break;
			case Still:
				SetNeedleState(Deploy);
				delay = 0.1;
				break;
			case Deploy:
				SetNeedleState(Idle);
				break;
			case Idle:
				break;
			default:
				break;
			}
		}
		break;
	}
	if (!pEntity || !pEntity->IsPlayer()) {
		switch (m_iState) {
		case Deploy:
		case Idle:
		case RetractShot:
			SetNeedleState(RetractArm);
			delay = 0.2;
			break;
		case RetractArm:
			SetNeedleState(Still);
			break;
		case Still:
			break;
		default:
			break;
		}
	}
	pev->nextthink = gpGlobals->time + delay;
}

void CWallHealthDecay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if( !pActivator )
		return;
	// if it's not a player, ignore
	if( !pActivator->IsPlayer() )
		return;

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	// if there is no juice left, turn it off
	if( (m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0 )
	{
		pev->skin = 1;
		SetThink(&CWallHealthDecay::Off);
		pev->nextthink = gpGlobals->time;
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if( ( m_iJuice <= 0 ) || ( !( pActivator->pev->weapons & ( 1 << WEAPON_SUIT ) ) ) )
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	SetThink(&CWallHealthDecay::Off);
	pev->nextthink = gpGlobals->time + 0.25;

	// Time to recharge yet?
	if( m_flNextCharge >= gpGlobals->time )
		return;

	int soundType = 0;
	TurnNeedleToPlayer(pActivator->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetNeedleState(GiveShot);
		soundType = 1;
		break;
	case GiveShot:
		SetNeedleState(Healing);
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			soundType = 2;
			m_playingChargeSound = TRUE;
		}
		break;
	default:
		ALERT(at_console, "Unexpected healthcharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
		UpdateJar();

		if (soundType == 1)
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		}
		else if (soundType == 2)
		{
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
			m_playingChargeSound = TRUE;
		}
	}
	else
	{
		if( m_flSoundTime <= gpGlobals->time )
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		if (m_playingChargeSound) {
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );
			m_playingChargeSound = FALSE;
		}
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealthDecay::Recharge( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = gSkillData.healthchargerCapacity;
	UpdateJar();
	pev->skin = 0;
	SetNeedleState(Still);
	SetThink( &CWallHealthDecay::SearchForPlayer );
	pev->nextthink = gpGlobals->time;
}

void CWallHealthDecay::Off( void )
{
	StudioFrameAdvance();
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			STOP_SOUND( ENT( pev ), CHAN_STATIC, "items/medcharge4.wav" );
			m_playingChargeSound = FALSE;
		}
		SetNeedleState(RetractShot);
		pev->nextthink = gpGlobals->time + 0.1;
		break;
	case RetractShot:
		if (m_iJuice > 0) {
			SetNeedleState(Idle);
			SetThink( &CWallHealthDecay::SearchForPlayer );
			pev->nextthink = gpGlobals->time;
		} else {
			SetNeedleState(RetractArm);
			pev->nextthink = gpGlobals->time + 0.2;
		}
		break;
	case RetractArm:
	{
		if( ( m_iJuice <= 0 ) )
		{
			SetNeedleState(Inactive);
			const float rechargeTime = g_pGameRules->FlHealthChargerRechargeTime();
			if (rechargeTime > 0 ) {
				pev->nextthink = gpGlobals->time + rechargeTime;
				SetThink( &CWallHealthDecay::Recharge );
			}
		}
		break;
	}
	default:
		break;
	}
}

void CWallHealthDecay::SetMySequence(const char *sequence)
{
	pev->sequence = LookupSequence( sequence );
	if (pev->sequence == -1) {
		ALERT(at_error, "unknown sequence in %s: %s\n", STRING(pev->model), sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo( );
}

void CWallHealthDecay::SetNeedleState(int state)
{
	m_iState = state;
	if (state == RetractArm)
		SetNeedleController(0);
	switch (state) {
	case Still:
		SetMySequence("still");
		break;
	case Deploy:
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_shot");
		break;
	case GiveShot:
		SetMySequence("give_shot");
		break;
	case Healing:
		SetMySequence("shot_idle");
		break;
	case RetractShot:
		SetMySequence("retract_shot");
		break;
	case RetractArm:
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("inactive");
	default:
		break;
	}
}

void CWallHealthDecay::TurnNeedleToPlayer(const Vector& player)
{
	float yaw = UTIL_VecToYaw( player - pev->origin ) - pev->angles.y;

	if( yaw > 180 )
		yaw -= 360;
	if( yaw < -180 )
		yaw += 360;

	SetNeedleController( yaw );
}

void CWallHealthDecay::SetNeedleController(float yaw)
{
	SetBoneController(0, yaw);
}

void CWallHealthDecay::UpdateOnRemove()
{
	CBaseAnimating::UpdateOnRemove();
	UTIL_Remove(m_jar);
	m_jar = NULL;
}
