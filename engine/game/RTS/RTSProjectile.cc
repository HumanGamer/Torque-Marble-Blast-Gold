//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/dgl.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "console/consoleTypes.h"
#include "console/typeValidators.h"
#include "core/bitStream.h"
#include "game/fx/explosion.h"
#include "game/shapeBase.h"
#include "ts/tsShapeInstance.h"
#include "game/RTS/RTSProjectile.h"
#include "audio/audio.h"
#include "math/mathUtils.h"
#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "game/fx/particleEngine.h"
#include "game/RTS/RTSUnit.h"

IMPLEMENT_CO_NETOBJECT_V1(RTSProjectile);

//const U32 RTSProjectile::csmStaticCollisionMask =  TerrainObjectType    |
//                                                   InteriorObjectType   |
//                                                   StaticObjectType;
//
//const U32 RTSProjectile::csmDynamicCollisionMask = PlayerObjectType        |
//                                                   VehicleObjectType       |
//                                                   DamagableItemObjectType;
//
//const U32 RTSProjectile::csmDamageableMask = RTSProjectile::csmDynamicCollisionMask;
//
//U32 RTSProjectile::smProjectileWarpTicks = 5;

//--------------------------------------------------------------------------
RTSProjectile::RTSProjectile()
{
   // Todo: ScopeAlways?
   mTypeMask |= ProjectileObjectType;

   mCurrPosition.set(0, 0, 0);
   mCurrVelocity.set(0, 0, 1);

   mSourceObjectId = -1;
   mSourceObjectSlot = -1;

   mCurrTick         = 0;

   mParticleEmitter   = NULL;

   mProjectileShape   = NULL;
   mActivateThread    = NULL;
   mMaintainThread    = NULL;

   mHidden           = false;
   mFadeValue        = 1.0;

   mDataBlock        = NULL;
   mTargetPos = Point3F();
   mSpeed = 0;
}

RTSProjectile::~RTSProjectile()
{
   delete mProjectileShape;
   mProjectileShape = NULL;
}

//--------------------------------------------------------------------------
void RTSProjectile::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Physics");
   addField("initialPosition",  TypePoint3F, Offset(mCurrPosition, RTSProjectile));
   addField("initialVelocity", TypePoint3F, Offset(mCurrVelocity, RTSProjectile));
   endGroup("Physics");

   addGroup("Source");
   addField("sourceObject",     TypeS32,     Offset(mSourceObjectId, RTSProjectile));
   addField("sourceSlot",       TypeS32,     Offset(mSourceObjectSlot, RTSProjectile));
   endGroup("Source");

   addField("targetPos", TypePoint3F, Offset(mTargetPos, RTSProjectile));
   addField("speed", TypeF32, Offset(mSpeed, RTSProjectile));
}

bool RTSProjectile::calculateImpact(float,
                                    Point3F& pointOfImpact,
                                    float&   impactTime)
{
   Con::warnf(ConsoleLogEntry::General, "RTSProjectile::calculateImpact: Should never be called");

   impactTime = 0;
   pointOfImpact.set(0, 0, 0);
   return false;
}

//--------------------------------------------------------------------------

bool RTSProjectile::onAdd()
{
   if(!Parent::onAdd())
      return false;

   ShapeBase* ptr;
   if (Sim::findObject(mSourceObjectId, ptr)) 
      mSourceObject = ptr;
   else 
   {
      if (mSourceObjectId != -1)
         Con::errorf(ConsoleLogEntry::General, "RTSProjectile::onAdd: mSourceObjectId is invalid");
      mSourceObject = NULL;
   }

   // If we're on the server, we need to inherit some of our parent's velocity
   //
   mCurrTick = 0;

   if (bool(mDataBlock->projectileShape))
   {
      mProjectileShape = new TSShapeInstance(mDataBlock->projectileShape, true);

      if (mDataBlock->activateSeq != -1)
      {
         mActivateThread = mProjectileShape->addThread();
         mProjectileShape->setTimeScale(mActivateThread, 1);
         mProjectileShape->setSequence(mActivateThread, mDataBlock->activateSeq, 0);
      }
   }
   if (mDataBlock->particleEmitter != NULL)
   {
      ParticleEmitter* pEmitter = new ParticleEmitter;
      pEmitter->onNewDataBlock(mDataBlock->particleEmitter);
      if (pEmitter->registerObject() == false)
      {
         Con::warnf(ConsoleLogEntry::General, "Could not register particle emitter for particle of class: %s", mDataBlock->getName());
         delete pEmitter;
         pEmitter = NULL;
      }
      mParticleEmitter = pEmitter;
   }

   if (mDataBlock->hasLight == true)
      Sim::getLightSet()->addObject(this);

   //if (bool(mSourceObject))
   //   processAfter(mSourceObject);

   // Setup our bounding box
   if (bool(mDataBlock->projectileShape) == true)
      mObjBox = mDataBlock->projectileShape->bounds;
   else
      mObjBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));

   mCurrVelocity = mTargetPos - mCurrPosition;
   mCurrVelocity.normalize();
   mCurrVelocity *= mSpeed;

   resetWorldBox();
   addToScene();

   return true;
}

//-----------------------------------------------------------------------------
class ObjectDeleteEvent : public SimEvent
{
public:
   void process(SimObject *object)
   {
      object->deleteObject();
   }
};

void RTSProjectile::explode(const Point3F& p, const Point3F& n, const U32 collideType )
{
   // Make sure we don't explode twice...
   if (mHidden == true)
      return;

   // Do what the server needs to do, damage the surrounding objects, etc.
   mExplosionPosition = p + (n*0.01);
   mExplosionNormal = n;

   char buffer[128];
   dSprintf(buffer, sizeof(buffer),  "%f %f %f", mExplosionPosition.x,
                                                 mExplosionPosition.y,
                                                 mExplosionPosition.z);
   Con::executef(mDataBlock, 4, "onClientExplode", scriptThis(), buffer, "1.0");

   Sim::postEvent(this, new ObjectDeleteEvent, Sim::getCurrentTime() + DeleteWaitTime);

   // Client just plays the explosion at the right place...
   //
   Explosion* pExplosion = NULL;

   F32 waterHeight;
   if (mDataBlock->explosion)
   {
      pExplosion = new Explosion;
      pExplosion->onNewDataBlock(mDataBlock->explosion);
   }

   if( pExplosion )
   {
      MatrixF xform(true);
      xform.setPosition(p);
      pExplosion->setTransform(xform);
      pExplosion->setInitialState(p, n);
      pExplosion->setCollideType( collideType );
      if (pExplosion->registerObject() == false)
      {
         Con::errorf(ConsoleLogEntry::General, "RTSProjectile(%s)::explode: couldn't register explosion",
            mDataBlock->getName() );
         delete pExplosion;
         pExplosion = NULL;
      }
   }

   // Client object
   Point3F noPoint(0,0,0);
   updateSound(); //noPoint, noPoint, false);

   mHidden = true;
}


void RTSProjectile::processTick(const Move* move)
{
   Parent::processTick(move);

   mCurrTick++;
   if(mSourceObject && mCurrTick > SourceIdTimeoutTicks)
   {
      mSourceObject = 0;
      mSourceObjectId = 0;
   }

   F32 timeLeft;
   RayInfo rInfo;
   Point3F oldPosition;
   Point3F newPosition;

   if (mCurrTick >= mDataBlock->lifetime)
   {
      deleteObject();
      return;
   }

   if (mHidden == true)
      return;

   // Otherwise, we have to do some simulation work.
   oldPosition = mCurrPosition;
   //if(mDataBlock->isBallistic)
   //   mCurrVelocity.z -= 9.81 * mDataBlock->gravityMod * (F32(TickMs) / 1000.0f);

   newPosition = oldPosition + mCurrVelocity * (F32(TickMs) / 1000.0f);
   if ((newPosition - mTargetPos).len() < mSpeed * TickMs / 1000.0f)
      explode(mTargetPos, VectorF(0,0,1), GameBaseObjectType);

   if (bool(mSourceObject))
      mSourceObject->disableCollision();

   timeLeft = 1.0;

   emitParticles(mCurrPosition, newPosition, mCurrVelocity, TickMs);

   mCurrDeltaBase = newPosition;
   mCurrBackDelta = mCurrPosition - newPosition;
   mCurrPosition = newPosition;

   MatrixF xform(true);
   xform.setColumn(3, mCurrPosition);
   setTransform(xform);

   if (bool(mSourceObject))
      mSourceObject->enableCollision();
}

//--------------------------------------------------------------------------
void RTSProjectile::onCollision(const Point3F& hitPosition,
                                const Point3F& hitNormal,
                                SceneObject*   hitObject)
{
   if (hitObject != NULL) 
   {
      char *posArg = Con::getArgBuffer(64);
      char *normalArg = Con::getArgBuffer(64);

      dSprintf(posArg, 64, "%f %f %f", hitPosition.x, hitPosition.y, hitPosition.z);
      dSprintf(normalArg, 64, "%f %f %f", hitNormal.x, hitNormal.y, hitNormal.z);

      Con::executef(mDataBlock, 6, "onClientCollision",
         scriptThis(),
         Con::getIntArg(hitObject->getId()),
         Con::getFloatArg(mFadeValue),
         posArg,
         normalArg);
   }
}
