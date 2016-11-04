//-----------------------------------------------------------------------------
// Marble Blast
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Monster Studios, Inc.
//-----------------------------------------------------------------------------

#ifndef _MARBLE_H_
#define _MARBLE_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

#ifndef _AUDIODATABLOCK_H_
#include "audio/audioDataBlock.h"
#endif

#ifndef _H_PARTICLEEMITTER
#include "game/fx/particleEngine.h"
#endif

//----------------------------------------------------------------------------
class MarbleData : public ShapeBaseData {
	typedef ShapeBaseData Parent;
public:
	
	DECLARE_CONOBJECT(MarbleData);

	F32 mMaxRollVelocity;
	F32 mAngularAcceleration;
	F32 mBrakingAcceleration;
	F32 mStaticFriction;
	F32 mKineticFriction;
	F32 mBounceKineticFriction;
	F32 mGravity;
	F32 mMaxDotSlide;
	F32 mBounceRestitution;
	F32 mAirAcceleration;
	F32 mEnergyRechargeRate;
	F32 mJumpImpulse;
	F32 mMaxForceRadius;
	F32 mCameraDistance;
	F32 mMinBounceVel;
	F32 mMinTrailSpeed;
	F32 mMinBounceSpeed;

	ParticleEmitterData mBounceEmitter;
	ParticleEmitterData mTrailEmitter;
	ParticleEmitterData mPowerUpEmitter;

	S32 mPowerUpTime;

	enum Sounds {
		RollHardSound,
		SlipSound,
		Bounce1,
		Bounce2,
		Bounce3,
		Bounce4,
		JumpSound,
		MaxSounds
	};

	AudioProfile* sound[MaxSounds];
	
	MarbleData();
	~MarbleData();
	
	bool preload(bool server, char errorBuffer[256]);
	static void initPersistFields();
	/*
	virtual void packData(BitStream* stream);
	virtual void unpackData(BitStream* stream);*/
};

//----------------------------------------------------------------------------
/// Implements a marble object.
class Marble : public ShapeBase {
	typedef ShapeBase Parent;
	
	enum MaskBits {
		MoveMask		 = Parent::NextFreeMask << 0,
		CamMask		 = Parent::NextFreeMask << 1,
		NextFreeMask	 = Parent::NextFreeMask << 2
	};
	
	struct StateDelta {
		Point3F pos;
		Point3F rot;
		VectorF posVec;
		VectorF rotVec;
	};
	
	enum MarbleMode {
		Start,
		Normal,
		Victory
	};
	
	Point3D mRot;
	StateDelta delta;
	
	Point3F mVelocity;
	Point3F mPosition;

	bool mControllable;

	float mPitch;
	float mYaw;

	//ConcretePolyList* mPolyList;
	/*
	void setTransform(const MatrixF& mat);
	
	F32 getWhiteOut() const;
	*/
public:
	DECLARE_CONOBJECT(Marble);
	
	Marble();
	~Marble();
	static void initPersistFields();
	static void consoleInit();
	
	bool onAdd();
	void onRemove();
	void interpolateTick(F32 delta);
	void getCameraTransform(F32* pos,MatrixF* mat);

	void advancePhysics(const Move *move, U32 unknown);
	
	void writePacketData(GameConnection *conn, BitStream *stream);
	void readPacketData(GameConnection *conn, BitStream *stream);
	U32  packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
	void unpackUpdate(NetConnection *conn, BitStream *stream);
	//Point3F &getPosition();

	void setVelocity(VectorF velocity);
	VectorF getVelocity();

	void setTransform(const MatrixF& mat);
	
	void setMode(MarbleMode mode);
	void processTick(const Move *move);
	void setPosition(const Point3F& pos, const AngAxisF& rot, float rot1);
	void setPosition(const Point3F& pos);
	bool onNewDataBlock(GameBaseData* dptr);
};

#endif