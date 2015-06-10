//-----------------------------------------------------------------------------
// Marble Blast
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Monster Studios, Inc.
//-----------------------------------------------------------------------------

#include "game/marble/marble.h"

#include "console/consoleTypes.h"

IMPLEMENT_CO_DATABLOCK_V1(MarbleData);

MarbleData::MarbleData()
{
	for (S32 i = 0; i < MaxSounds; i++)
      sound[i] = 0;
}

MarbleData::~MarbleData()
{
}

bool MarbleData::preload(bool server, char errorBuffer[256])
{
	if (!Parent::preload(server, errorBuffer))
      return false;
	
	if (!server)
	{
		for (S32 i = 0; i < MaxSounds; i++)
		{
			if (sound[i])
				Sim::findObject(SimObjectId(sound[i]), sound[i]);
		}
	}
	
	return true;
}

void MarbleData::initPersistFields()
{
	// Physics
	ConsoleObject::addField("maxRollVelocity", TypeF32, Offset(mMaxRollVelocity, MarbleData));
	ConsoleObject::addField("angularAcceleration", TypeF32, Offset(mAngularAcceleration, MarbleData));
	ConsoleObject::addField("brakingAcceleration", TypeF32, Offset(mBrakingAcceleration, MarbleData));
	ConsoleObject::addField("staticFriction", TypeF32, Offset(mStaticFriction, MarbleData));
	ConsoleObject::addField("kineticFriction", TypeF32, Offset(mKineticFriction, MarbleData));
	ConsoleObject::addField("bounceKineticFriction", TypeF32, Offset(mBounceKineticFriction, MarbleData));
	ConsoleObject::addField("gravity", TypeF32, Offset(mGravity, MarbleData));
	ConsoleObject::addField("maxDotSlide", TypeF32, Offset(mMaxDotSlide, MarbleData));
	ConsoleObject::addField("bounceRestitution", TypeF32, Offset(mBounceRestitution, MarbleData));
	ConsoleObject::addField("airAcceleration", TypeF32, Offset(mAirAcceleration, MarbleData));
	ConsoleObject::addField("energyRechargeRate", TypeF32, Offset(mEnergyRechargeRate, MarbleData));
	ConsoleObject::addField("jumpImpulse", TypeF32, Offset(mJumpImpulse, MarbleData));
	ConsoleObject::addField("maxForceRadius", TypeF32, Offset(mMaxForceRadius, MarbleData));
	ConsoleObject::addField("cameraDistance", TypeF32, Offset(mCameraDistance, MarbleData));
	ConsoleObject::addField("minBounceVel", TypeF32, Offset(mMinBounceVel, MarbleData));
	ConsoleObject::addField("minTrailSpeed", TypeF32, Offset(mMinTrailSpeed, MarbleData));
	ConsoleObject::addField("minBounceSpeed", TypeF32, Offset(mMinBounceSpeed, MarbleData));

	// Particles
	ConsoleObject::addField("bounceEmitter", TypeParticleEmitterDataPtr, Offset(mBounceEmitter, MarbleData));
	ConsoleObject::addField("trailEmitter", TypeParticleEmitterDataPtr, Offset(mTrailEmitter, MarbleData));
	ConsoleObject::addField("powerUpEmitter", TypeParticleEmitterDataPtr, Offset(mPowerUpEmitter, MarbleData));

	ConsoleObject::addField("powerUpTime", TypeS32, Offset(mPowerUpTime, MarbleData));

	// Sounds
	ConsoleObject::addField("RollHardSound", TypeAudioProfilePtr, Offset(sound[RollHardSound], MarbleData));
	ConsoleObject::addField("SlipSound", TypeAudioProfilePtr, Offset(sound[SlipSound], MarbleData));
	ConsoleObject::addField("Bounce1", TypeAudioProfilePtr, Offset(sound[Bounce1], MarbleData));
	ConsoleObject::addField("Bounce2", TypeAudioProfilePtr, Offset(sound[Bounce2], MarbleData));
	ConsoleObject::addField("Bounce3", TypeAudioProfilePtr, Offset(sound[Bounce3], MarbleData));
	ConsoleObject::addField("Bounce4", TypeAudioProfilePtr, Offset(sound[Bounce4], MarbleData));
	ConsoleObject::addField("JumpSound", TypeAudioProfilePtr, Offset(sound[JumpSound], MarbleData));

	Parent::initPersistFields();
}

IMPLEMENT_CO_NETOBJECT_V1(Marble);

Marble::Marble()
{
}

Marble::~Marble()
{
}

void Marble::initPersistFields()
{
	Parent::initPersistFields();
}

void Marble::consoleInit()
{
	Parent::consoleInit();
}
