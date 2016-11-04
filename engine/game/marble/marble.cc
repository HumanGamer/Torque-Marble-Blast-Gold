//-----------------------------------------------------------------------------
// Marble Blast
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Monster Studios, Inc.
//-----------------------------------------------------------------------------

#include "game/marble/marble.h"


#include "game/gameConnection.h"
#include "console/consoleTypes.h"
#include "math/mathIO.h"

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
	//mPolyList = new ConcretePolyList();
	this->mPitch = 0;
	this->mYaw = 0;
	this->mVelocity = VectorF(0, 0, 0);
}

Marble::~Marble()
{
}

void Marble::initPersistFields()
{
	ConsoleObject::addField("Controllable", TypeBool, Offset(mControllable, Marble));
	Parent::initPersistFields();
}

void Marble::consoleInit()
{
	Parent::consoleInit();
}

bool Marble::onAdd()
{
	if (!Parent::onAdd() || !mDataBlock)
		return false;

    //mObjToWorld.getColumn(3,&delta.pos);

	addToScene();
	if (isServerObject())
    {
       scriptOnAdd();
    }
	//SceneGraph::addShadowOccluder();

	return true;
}

void Marble::onRemove()
{
	scriptOnRemove();

	//SceneGraph::removeShadowOccluder();
	removeFromScene();

	//if (?)
	//	alxStop(?);

	//if (?)
	//	alxStop(?);

	Parent::onRemove();
}

void Marble::setMode(MarbleMode mode)
{
	// insert setmode here
	//setMaskBits(0x40000000);
}

void Marble::interpolateTick(F32 delta)
{
	Parent::interpolateTick(delta);
}

void Marble::processTick(const Move *move)
{
	Parent::processTick(move);
	if (isServerObject())
	{
		this->advancePhysics(move, 0); // unknown = 0?
	} else {
		if (move != NULL)
		{
			this->mPitch += move->pitch;//MoveManager::mPitch;
			this->mYaw += move->yaw;//MoveManager::mYaw;
			if (mRadToDeg(this->mPitch) > 90)
				this->mPitch = mDegToRad((float)90);
			if (mRadToDeg(this->mPitch) < -90)
				this->mPitch = mDegToRad((float)-90);
		}
	}
}

void Marble::advancePhysics(const Move *move, U32 unknown)
{
	Point3F currentPos = this->getPosition();

	MarbleData *datablock = dynamic_cast<MarbleData*>(this->mDataBlock);

	float gravity = datablock->mGravity / 1000;
	float gravity2 = datablock->mGravity / 10;

	this->mVelocity += VectorF(0, 0, -gravity);

	if (this->mVelocity.z < -gravity2)
	{
		this->mVelocity.z = -gravity2;
	}

	Point3F newPos = currentPos + this->mVelocity;//Point3F(0, 0, 1);

	//this->setPosition(newPos);

	/*MatrixF mat;
	mat.setColumn(3,newPos);
	Parent::setTransform(mat);
	Parent::setRenderTransform(mat);*/

	this->setPosition(newPos);
	setMaskBits(MoveMask);

	//updateContainer();
}

void Marble::setVelocity(VectorF velocity)
{
	this->mVelocity = velocity;
}

VectorF Marble::getVelocity()
{
	return this->mVelocity;
}

void Marble::getCameraTransform(F32* pos, MatrixF* mat)
{
	// Jeff: get variables from script
	F32 pitch = this->mPitch;
	F32 yaw   = this->mYaw;

	// Jeff: distance
	F32 distance = 2.3f;//mDataBlock->cameraMaxDist;
	F32 horizontalDist = mCos(pitch) * distance;
	F32 verticalDist = mSin(pitch) * distance;

	// Jeff: make the camera "orbit" around the marble
	Point3F ortho(-mSin(yaw) * horizontalDist,
						-mCos(yaw) * horizontalDist,
						verticalDist);

	// Jeff: add the ortho position to the object's current position
	Point3F position = getPosition() + ortho;

	disableCollision();

	//HiGuy: Do a raycast so we don't have the camera clipping the everything
	RayInfo info;

	if (mContainer->castRay(getPosition(), position, ~(WaterObjectType | GameBaseObjectType | DefaultObjectType), &info)) {
		// S22: -measure difference of collision point and marble location
		// -normalize (make it into a unit vector)
		// -subtract .001 * unit vector from original position of rayhit
		Point3F dist = Point3F(info.point - getPosition());
		dist.normalize();
		dist *= 0.01f;

		// combine vectors
		position = info.point - dist;
	}

	// Jeff: calculate rotation of the camera by doing matrix multiplies
	AngAxisF rot1(Point3F(0.0f, 0.0f, 1.0f), yaw);
	AngAxisF rot2(Point3F(1.0f, 0.0f, 0.0f), pitch);
	MatrixF mat1, mat2;

	rot1.setMatrix(&mat1);
	rot2.setMatrix(&mat2);

	// Jeff: set position and apply rotation
	mat1.mul(mat2);
	mat1.setPosition(position);

	enableCollision();

	*mat = mat1;

	// Apply Camera FX.
	//mat->mul( gCamFXMgr.getTrans() );
}

void Marble::setPosition(const Point3F& pos, const AngAxisF& rot, float rot1)
{
	this->mVelocity.set(0, 0, 0);

	MatrixF mat;
	rot.setMatrix(&mat);
	//mat.set(rot.angle * rot.axis);
	mat.setColumn(3,pos);
	Parent::setTransform(mat);
	//Parent::setRenderTransform(mat);
	//rot1.setMatrix(&mat);
	//Parent::setPosition(pos);

	this->mPitch = rot1;
	Point3F axis = rot.axis;
	axis *= -rot.angle;
	this->mYaw = axis.z;

	setMaskBits(CamMask);

	//setMaskBits(MoveMask);
}

void Marble::setPosition(const Point3F& pos)
{
	MatrixF mat;
	mat.setColumn(3,pos);
	Parent::setTransform(mat);
	Parent::setRenderTransform(mat);
	//rot1.setMatrix(&mat);
	//Parent::setPosition(pos);

	//setMaskBits(MoveMask);
}

bool Marble::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<MarbleData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   scriptOnNewDataBlock();
   //resetWorldBox();
   return true;
}

void Marble::writePacketData(GameConnection *conn, BitStream *stream)
{
	Parent::writePacketData(conn, stream);

	/*Point3F pos = getPosition();
    stream->setCompressionPoint(pos);
	mathWrite(*stream, pos);

    //stream->writeAffineTransform(mObjToWorld);
    mathWrite(*stream, mObjScale);
	mathWrite(*stream, mVelocity);*/

	stream->writeAffineTransform(mObjToWorld);
       mathWrite(*stream, mObjScale);
	   mathWrite(*stream, mVelocity);
}

void Marble::readPacketData(GameConnection *conn, BitStream *stream)
{
	Parent::readPacketData(conn, stream);

	MatrixF mat;
    stream->readAffineTransform(&mat);
    Parent::setTransform(mat);
    //Parent::setRenderTransform(mat);

    VectorF scale;
    mathRead(*stream, &scale);
    setScale(scale);

	VectorF velocity;
	mathRead(*stream, &velocity);
	setVelocity(velocity);

	//MatrixF mat;
    //stream->readAffineTransform(&mat);
    //Parent::setTransform(mat);
    //Parent::setRenderTransform(mat);

	/*Point3F pos, rot;
    mathRead(*stream, &pos);

    stream->setCompressionPoint(pos);

    setPosition(pos);

    VectorF scale;
    mathRead(*stream, &scale);
    setScale(scale);

	VectorF velocity;
	mathRead(*stream, &velocity);
	setVelocity(velocity);*/

	/*MatrixF trans;
	mathRead(*stream, &trans);
	Parent::setTransform(trans);

	Point3F vel;
	mathRead(*stream, &vel);
	this->setVelocity(vel);*/
}

U32 Marble::packUpdate(NetConnection *connection, U32 mask, BitStream *bstream)
{
   U32 retMask = Parent::packUpdate(connection,mask,bstream);

   if (bstream->writeFlag(getControllingClient() == connection && (mask & CamMask)))
   {
	   bstream->write(this->mPitch);
	   bstream->write(this->mYaw);
   }

   // The rest of the data is part of the control object packet update.
   // If we're controlled by this client, we don't need to send it.
   if (bstream->writeFlag(getControllingClient() == connection && !(mask & InitialUpdateMask)))
      return retMask;

   if (bstream->writeFlag(mask & MoveMask)) {
	   //mathWrite(*bstream, mObjToWorld);
	   //bstream->writeCompressedPoint(mPosition);
       bstream->writeAffineTransform(mObjToWorld);
       mathWrite(*bstream, mObjScale);
	   mathWrite(*bstream, mVelocity);
   }

   return retMask;
}

void Marble::unpackUpdate(NetConnection *connection, BitStream *bstream)
{
   Parent::unpackUpdate(connection,bstream);

   if (bstream->readFlag())
   {
	   bstream->read(&this->mPitch);
	   bstream->read(&this->mYaw);
   }

   if (bstream->readFlag())
      return;

   if (bstream->readFlag()) {
      MatrixF mat;
	  //mathRead(*bstream, &mat);
      bstream->readAffineTransform(&mat);
      Parent::setTransform(mat);
      Parent::setRenderTransform(mat);

	   //Point3F pos;
	   //bstream->readCompressedPoint(&pos);
	   //Parent::setPosition(pos);


      VectorF scale;
      mathRead(*bstream, &scale);
      setScale(scale);

	  VectorF velocity;
	  mathRead(*bstream, &velocity);
	  setVelocity(velocity);
   }
}

void Marble::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   setMaskBits(MoveMask);
}

ConsoleMethod( Marble, setPosition, void, 4, 4, "(Position P, Rotation r)")
{
   Point3F pos;
   const MatrixF& tmat = object->getTransform();
   tmat.getColumn(3,&pos);
   AngAxisF aa(tmat);

   dSscanf(argv[2],"%f %f %f %f %f %f %f",
           &pos.x,&pos.y,&pos.z,&aa.axis.x,&aa.axis.y,&aa.axis.z,&aa.angle);
			  
   F32 rotVal = dAtof(argv[3]);

   //AngAxisF rot(Point3F(1, 0, 0), rotVal);

   MatrixF mat;
   aa.setMatrix(&mat);
   mat.setColumn(3,pos);
   object->setPosition(pos, aa, rotVal);
}

ConsoleMethod( Marble, getVelocity, const char *, 2, 2, "")
{
   const VectorF& vel = object->getVelocity();
   char* buff = Con::getReturnBuffer(100);
   dSprintf(buff,100,"%g %g %g",vel.x,vel.y,vel.z);
   return buff;
}

ConsoleMethod( Marble, setVelocity, bool, 3, 3, "(Vector3F vel)")
{
   VectorF vel(0,0,0);
   dSscanf(argv[2],"%f %f %f",&vel.x,&vel.y,&vel.z);
   object->setVelocity(vel);
   return true;
}
