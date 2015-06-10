//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "game/RTS/RTSUnit.h"
#include "game/RTS/RTSCamera.h"
#include "console/consoleInternal.h"
#include "core/realComp.h"
#include "math/mMatrix.h"
#include "game/moveManager.h"
#include "platform/profiler.h"
#include "core/bitStream.h"
#include "ts/tsShapeInstance.h"
#include "game/gameConnection.h"
#include "terrain/terrData.h"
#include "sceneGraph/sceneGraph.h"
#include "audio/audioDataBlock.h"
#include "game/projectile.h"
#include "game/RTS/RTSConnection.h"

#include "sceneGraph/detailManager.h"
#include "dgl/dgl.h"
#include "game/RTS/visManager.h"

static const F32 sMountPendingTickWait = (13 * 32);

// Client prediction
static F32 sMinWarpTicks = 0.5;        // Fraction of tick at which instant warp occures
static S32 sMaxWarpTicks = 3;          // Max warp duration in ticks
static S32 sMaxPredictionTicks = 30;   // Number of ticks to predict

IMPLEMENT_CO_NETOBJECT_V1(RTSUnit);
IMPLEMENT_CONOBJECT(RTSUnitModifier);
IMPLEMENT_CO_DATABLOCK_V1(RTSUnitData);
IMPLEMENT_CO_DATABLOCK_V1(RTSUnitModifierData);

void RTSUnitModifier::initPersistFields()
{
   Parent::initPersistFields();

   addField("damage",      TypeF32, Offset(mDamage,       RTSUnitModifier));
   addField("attackDelay", TypeF32, Offset(mAttackDelay,  RTSUnitModifier));
   addField("armor",       TypeF32, Offset(mArmor,        RTSUnitModifier));
   addField("moveSpeed",   TypeF32, Offset(mMoveSpeed,    RTSUnitModifier));
   addField("range",       TypeF32, Offset(mRange,        RTSUnitModifier));
   addField("vision",      TypeF32, Offset(mVision,       RTSUnitModifier));
}

void RTSUnitModifierData::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->write(mModifier.mDamage);
   stream->write(mModifier.mAttackDelay);
   stream->write(mModifier.mArmor);
   stream->write(mModifier.mMoveSpeed);
   stream->write(mModifier.mRange);
   stream->write(mModifier.mVision);
}

void RTSUnitModifierData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   stream->read(&mModifier.mDamage);
   stream->read(&mModifier.mAttackDelay);
   stream->read(&mModifier.mArmor);
   stream->read(&mModifier.mMoveSpeed);
   stream->read(&mModifier.mRange);
   stream->read(&mModifier.mVision);
}

void RTSUnitModifierData::initPersistFields()
{
   Parent::initPersistFields();

   addField("damage",      TypeF32, Offset(mModifier.mDamage,      RTSUnitModifierData));
   addField("attackDelay", TypeF32, Offset(mModifier.mAttackDelay, RTSUnitModifierData));
   addField("armor",       TypeF32, Offset(mModifier.mArmor,       RTSUnitModifierData));
   addField("moveSpeed",   TypeF32, Offset(mModifier.mMoveSpeed,   RTSUnitModifierData));
   addField("range",       TypeF32, Offset(mModifier.mRange,       RTSUnitModifierData));
   addField("vision",      TypeF32, Offset(mModifier.mVision,      RTSUnitModifierData));
}


RTSUnitData::RTSUnitData()
{
   mBaseDamage = 10;
   mAttackDelay = 320;
   mDamagePlus = 3;
   mArmor = 1;
   mMoveSpeed = 10;
   mRange = 2;
   mVision = 20;
   mDoWaterInteraction = false;
   mDoShadow = false;
   mDoLookAnimation = false;
}

void RTSUnitData::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->write(mBaseDamage);
   stream->write(mAttackDelay);
   stream->write(mDamagePlus);
   stream->write(mArmor);
   stream->write(mMoveSpeed);
   stream->write(mRange);
   stream->write(mVision);
   stream->writeFlag(mDoWaterInteraction);
   stream->writeFlag(mDoShadow);
   stream->writeFlag(mDoLookAnimation);

   stream->writeString(this->className);
}

void RTSUnitData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   stream->read(&mBaseDamage);
   stream->read(&mAttackDelay);
   stream->read(&mDamagePlus);
   stream->read(&mArmor);
   stream->read(&mMoveSpeed);
   stream->read(&mRange);
   stream->read(&mVision);
   mDoWaterInteraction = stream->readFlag();
   mDoShadow           = stream->readFlag();
   mDoLookAnimation    = stream->readFlag();

   className = stream->readSTString();
}

void RTSUnitData::initPersistFields()
{
   Parent::initPersistFields();

   addField("baseDamage",           TypeS32,    Offset(mBaseDamage,         RTSUnitData));
   addField("attackDelay",          TypeS32,    Offset(mAttackDelay,        RTSUnitData));
   addField("damagePlus",           TypeS32,    Offset(mDamagePlus,         RTSUnitData));
   addField("armor",                TypeS32,    Offset(mArmor,              RTSUnitData));
   addField("moveSpeed",            TypeF32,    Offset(mMoveSpeed,          RTSUnitData));
   addField("range",                TypeF32,    Offset(mRange,              RTSUnitData));
   addField("doWaterInteraction",   TypeBool,   Offset(mDoWaterInteraction, RTSUnitData));
   addField("doShadow",             TypeBool,   Offset(mDoShadow,           RTSUnitData));
   addField("doLookAnimation",      TypeBool,   Offset(mDoLookAnimation,    RTSUnitData));
}

//----------------------------------------------------------------------
RTSUnit::RTSUnit()
{
   mNetFlags.set(Ghostable);
   mMoveDestination.set( 0.0f, 0.0f, 0.0f );
   mMoveTolerance  = 0.25f;

   mAimObjectSet   = false;
   mAimObject      = 0;
   mAimLocationSet = false;
   mTargetInLOS    = false;

   mAttackDelay = 32000;

   mMoveState = ModeStop;

   mMoveVelocity = 5;

   mTeam = -1;
   mCurrentProjectile = NULL;

   mPositionDirty = true;
   mControllingConnection = NULL;
}

RTSUnit::~RTSUnit()
{
}

bool RTSUnit::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mNetModifier.registerObject();
   mModifierList.registerObject();
   if (isServerObject())
      gServerVisManager->addObject(this);
   else
      gClientVisManager->addObject(this);

   return true;
}

void RTSUnit::onRemove()
{
   Parent::onRemove();
   mNetModifier.unregisterObject();
   mModifierList.unregisterObject();
   if (isServerObject())
      gServerVisManager->removeObject(this);
   else
      gClientVisManager->removeObject(this);
}

void RTSUnit::stopMove()
{
   if (mMoveState == ModeStop)
      return;

   mMoveState = ModeStop;
   setMaskBits(AIMask | MoveMask);
}

void RTSUnit::setMoveTolerance( const F32 tolerance )
{
   mMoveTolerance = getMax( 0.1f, tolerance );
   setMaskBits(AIMask);
}

void RTSUnit::setMoveDestination( const Point3F &location )
{
   // Check if we're still alive.
   if(getDamageState() == Enabled)
   {
      mMoveDestination = location;
      mMoveState = ModeMove;
      setActionThread(PlayerData::RootAnim,true,false,false, true, true);
   } else 
   {
      stopMove();
      setActionThread("die",false,false,false);      
   }
}

void RTSUnit::setAimObject( GameBase *targetObject )
{
   mAimObject = targetObject;
   mAimObjectSet = true;
   mTargetInLOS = false;
   setMaskBits(AIMask);
}

void RTSUnit::setAimLocation( const Point3F &location )
{
   mAimObject = 0;
   mAimLocationSet = true;
   mAimLocation = location;
   setMaskBits(AIMask);
}

void RTSUnit::clearAim()
{
   mAimObject = 0;
   mAimLocationSet = false;
   mAimObjectSet = false;
   setMaskBits(AIMask);
}

//-----------------------------------------------------------------------------

void RTSUnit::addModifier(RTSUnitModifierData *mod)
{
   if (!mod)
      return;

   U32 size = mModifierList.size();
   mModifierList.addObject(mod);
   if (mModifierList.size() > size)
   {
      mNetModifier += mod->mModifier;
      if (mod->mModifier.mVision != 0)
         setDirty();
   }

   setMaskBits(ModifierMask);
}

void RTSUnit::removeModifier(RTSUnitModifierData *mod)
{
   if (!mod)
      return;

   U32 size = mModifierList.size();
   mModifierList.removeObject(mod);
   if (mModifierList.size() < size)
      mNetModifier -= mod->mModifier;

   setMaskBits(ModifierMask);
}

void RTSUnit::processTick(const Move *move)
{
   // In fact, RTSUnits don't do moves at all. ;)
   resetWorldBox();

   // Pass control up over Player, as we're replacing its functionality.
   ShapeBase::processTick(move);

   if (RTSUnitData* db = dynamic_cast<RTSUnitData*>(mDataBlock))
   {
      if (db->mDoLookAnimation)
      {
         updateLookAnimation();
      }
   }
   updateDeathOffsets();

   if (!isGhost())
      updateAnimation(TickSec);

   PROFILE_START(RTSUnit_Physics);

   getTransform().getColumn(3,&delta.posVec);
   Point3F location = getPosition();
   Point3F rotation = getRotation();

   // Check if we're alive before processing looks and moves
   if (getDamageState() == Enabled) 
   {
      if (!isGhost()) 
      {
         // Orient towards the aim point, aim object, or towards
         // our destination.
         if (mAimObjectSet || mAimLocationSet || mMoveState == ModeMove)
         {
            PROFILE_START(RTSUnit_Look);
            // Update the aim position if we're aiming for an object
            if(mAimObjectSet)
            {
               if(mAimObject!=0)
                  mAimLocation = mAimObject->getBoxCenter();
               else
                  mAimObjectSet = false;
            }
            else if(!mAimLocationSet)
               mAimLocation = mMoveDestination;

            F32 xDiff = mAimLocation.x - location.x;
            F32 yDiff = mAimLocation.y - location.y;
            if (!isZero(xDiff) || !isZero(yDiff))
            {
               // First do Yaw
               // use the cur yaw between -Pi and Pi
               F32 curYaw = rotation.z;
               while (curYaw > M_2PI)
                  curYaw -= M_2PI;
               while (curYaw < -M_2PI)
                  curYaw += M_2PI;

               // find the yaw offset
               F32 newYaw = mAtan(xDiff, yDiff);
               F32 yawDiff = newYaw - curYaw;

               // make it between 0 and 2PI
               if(yawDiff < 0.0f)
                  yawDiff +=M_2PI;
               else if(yawDiff >= M_2PI)
                  yawDiff -= M_2PI;

               // now make sure we take the short way around the circle
               if(yawDiff > M_PI)
                  yawDiff -= M_2PI;
               else if(yawDiff <-M_PI)
                  yawDiff += M_2PI;

               mRot.z += yawDiff;
            }
            PROFILE_END();
         }

      }
      PROFILE_START(RTSUnit_Move);
      updateState();

      // Now move us towards our goal... somehow...

      // Check if goal is to attack a target
      ShapeBase* target = dynamic_cast<ShapeBase*>(&(*mAimObject));
      if(mAimObjectSet  && (!target || target->getDamageState() == ShapeBase::Enabled))
      {
         // Check that our aim object is or is derived from RTSUnit
         if(RTSUnit *target = dynamic_cast<RTSUnit*>((GameBase*)mAimObject))
         {
            Point3F posXY(location.x, location.y, 0);
            Point3F goalXY(target->getPosition().x,target->getPosition().y, 0);

            Point3F goalDelta = goalXY - posXY;

            // Check that we have a valid DataBlock
            if(RTSUnitData *data = dynamic_cast<RTSUnitData*>(mDataBlock))
            {
               // Check if we're within range to fire
               if(goalDelta.len() <= data->mRange)
               {
                  if(mMoveState == ModeMove)                  
                     stopMove();
                     
                  mVelocity.set(0,0,0);

                  if(mAttackDelay >= data->mAttackDelay)
                  {
                     mAttackDelay = 0;

                     ShapeBase* target = dynamic_cast<ShapeBase*>(&(*mAimObject));
                     if (!target || target->getDamageState() == ShapeBase::Enabled)
                     {
                        if(isServerObject())
                        {
                           // Do a proper method callback...
                           Con::executef(mDataBlock, 3, "onAttack", scriptThis(), mAimObject->getIdString());
                        }
                        else
                        {
                           // Call a goofy global function to do the client side animation.
                           Con::executef(3, "doClientAttackAnimation", scriptThis(), mAimObject->getIdString());
                        }
                     }
                  }
                  else
                     mAttackDelay++;
               }
               else
               {
                  setActionThread("run",true,false,false);
                  // Cap speed...
                  F32 v = 0;
                  if(RTSUnitData *data = dynamic_cast<RTSUnitData*>(mDataBlock))
                     v = data->mMoveSpeed * mNetModifier.mMoveSpeed * TickSec;
                  else
                     v = mMoveVelocity * TickSec;

                  if(goalDelta.len() > v)
                  {
                     goalDelta.normalize();
                     goalDelta *= v;
                  }

                  // Set velocity to make the animation system play stuff
                  mVelocity = goalDelta / TickSec;

                  location += goalDelta;
               }

               setMaskBits(MoveMask);
            }
         }
      }
      else if(mMoveState == ModeMove)
      {
         Point3F posXY(location.x, location.y, 0);
         Point3F goalXY(mMoveDestination.x, mMoveDestination.y, 0);

         Point3F goalDelta = goalXY - posXY;

         if(goalDelta.len() < 0.1)
         {
            stopMove();
            mVelocity.set(0,0,0);
            setActionThread(PlayerData::RootAnim,true,false,false);

            Con::executef(this, 1, "onReachDestination");

            //sync up the position
            if (isServerObject())
               setMaskBits(MoveMask);
         }
         else
         {
            setActionThread("run",true,false,false);

            // Cap speed...
            F32 v = 0;
            if(RTSUnitData *data = dynamic_cast<RTSUnitData*>(mDataBlock))
               v = data->mMoveSpeed * mNetModifier.mMoveSpeed * TickSec;
            else
               v = mMoveVelocity * TickSec;

            if(goalDelta.len() > v)
            {
               goalDelta.normalize();
               goalDelta *= v;
            }

            // Set velocity to make the animation system play stuff
            mVelocity = goalDelta / TickSec;

            location += goalDelta;
         }
         setMaskBits(MoveMask);
      }
      else
      {
         // We're not moving, so set animation to root and velocity to zero.
         setActionThread(PlayerData::RootAnim,true,false,false);
         mVelocity.set(0,0,0);
      }
      PROFILE_END();
   }

   PROFILE_START(RTSUnit_Z);

   // Move us to sit on the ground...
   TerrainBlock* block = NULL;
   if (isClientObject() && gClientSceneGraph)
      block = gClientSceneGraph->getCurrentTerrain();
   else if (isServerObject() && gServerSceneGraph)
      block = gServerSceneGraph->getCurrentTerrain();

   if(block)
   {
      Point3F terrPos = location;
      block->getWorldTransform().mulP(terrPos);
      terrPos.convolveInverse(block->getScale());

      F32 height;
      bool res = block->getHeight(Point2F(terrPos.x, terrPos.y), &height);
      if(res)
      {
         terrPos.z = height;
         terrPos.convolve(block->getScale());
         block->getTransform().mulP(terrPos);
      }

      if (mFabs(location.z - height) > 0.01)
      {
         location.z = height;
         setMaskBits(MoveMask);
      }
   }
   PROFILE_END();


   // Warp to catch up to server
   if (delta.warpTicks > 0)
   {
      PROFILE_START(RTSUnit_Warp);
      delta.warpTicks--;

      // Set new pos.
      getTransform().getColumn(3,&delta.pos);
      delta.pos += delta.warpOffset;
      delta.rot += delta.rotOffset;
      setPosition(delta.pos,delta.rot);
      setRenderPosition(delta.pos,delta.rot);
      updateDeathOffsets();
      updateLookAnimation();

      // Backstepping
      delta.posVec.x = -delta.warpOffset.x;
      delta.posVec.y = -delta.warpOffset.y;
      delta.posVec.z = -delta.warpOffset.z;
      delta.rotVec.x = -delta.rotOffset.x;
      delta.rotVec.y = -delta.rotOffset.y;
      delta.rotVec.z = -delta.rotOffset.z;
      PROFILE_END();
   }

   // Set new position
   // If on the client, calc delta for backstepping
   if (isClientObject())
   {
      delta.pos = location;
      delta.rot = mRot;
      delta.posVec.set(0,0,0);
      delta.rotVec.set(0,0,0);
      delta.warpTicks = 0;
      delta.dt = 0;
   }

   PROFILE_START(RTSUnit_UpdatePos);
   if(mLastPos != location)
   {
      setPosition(location,mRot);
   }
   PROFILE_END();

   if (!isGhost())
   {
      PROFILE_START(RTSUnit_Misc);
      // Collisions are only queued on the server and can be
      // generated by either updateMove or updatePos
      notifyCollision();

      // Do mission area callbacks on the server as well
      checkMissionArea();

      // Animations are advanced based on frame rate on the
      // client and must be ticked on the server.
      updateActionThread();
      updateAnimationTree(true);

      if(mFading)
      {
         F32 dt = TickMs / 1000.0;
         F32 newFadeET = mFadeElapsedTime + dt;
         if(mFadeElapsedTime < mFadeDelay && newFadeET >= mFadeDelay)
            setMaskBits(CloakMask);
         mFadeElapsedTime = newFadeET;
         if(mFadeElapsedTime > mFadeTime + mFadeDelay)
         {
            mFadeVal = F32(!mFadeOut);
            mFading = false;
         }
      }
      PROFILE_END();
   }

   PROFILE_END();
}

void RTSUnit::interpolateTick(F32 dt)
{
   if (mControlObject)
      mControlObject->interpolateTick(dt);

   // Client side interpolation
   ShapeBase::interpolateTick(dt);
   if(dt != 0.0f)
   {
      Point3F pos = delta.pos + delta.posVec * dt;
      Point3F rot = delta.rot + delta.rotVec * dt;
      
      mHead = delta.head + delta.headVec * dt;
      setRenderPosition(pos,rot,dt);

      // apply camera effects - is this the best place? - bramage
      GameConnection* connection = GameConnection::getServerConnection();
      if( connection->isFirstPerson() )
      {
         ShapeBase *obj = connection->getControlObject();
         if( obj == this )
         {
            MatrixF curTrans = getRenderTransform();
            ShapeBase::setRenderTransform( curTrans );
         }
      }
   }
   else
   {
      mHead = delta.head;
      setRenderPosition(delta.pos, delta.rot, 0);
   }
   updateLookAnimation();
   delta.dt = dt;
}

void RTSUnit::advanceTime(F32 dt)
{
   // Client side animations
   ShapeBase::advanceTime(dt);
   updateActionThread();
   updateAnimation(dt);
   if (RTSUnitData* db = dynamic_cast<RTSUnitData*>(mDataBlock))
   {
      if (db->mDoWaterInteraction)
      {
         updateSplash();
         updateFroth(dt);
         updateWaterSounds(dt);
      }
   }

   mLastPos = getPosition();

   if (mImpactSound)
      playImpactSound();
}

//-----------------------------------------------------------------------------

void RTSUnit::updateActionThread()
{
   PROFILE_START(UpdateActionThread);

   // Select an action animation sequence, this assumes that
   // this function is called once per tick.

   if(mActionAnimation.action != PlayerData::NullAnimation)
      if (mActionAnimation.forward)
         mActionAnimation.atEnd = mShapeInstance->getPos(mActionAnimation.thread) == 1;
      else
         mActionAnimation.atEnd = mShapeInstance->getPos(mActionAnimation.thread) == 0;

   // Only need to deal with triggers on the client
   // This is disabled as it is a bit of a performance hog -- BJG
   //if (isGhost())  {
   //   bool triggeredLeft = false;
   //   bool triggeredRight = false;
   //   F32 offset = 0.0f;
   //   if(mShapeInstance->getTriggerState(1)) {
   //      triggeredLeft = true;
   //      offset = -mDataBlock->decalOffset;
   //   }
   //   else if(mShapeInstance->getTriggerState(2)) {
   //      triggeredRight = true;
   //      offset = mDataBlock->decalOffset;
   //   }

   //   if (triggeredLeft || triggeredRight)
   //   {
   //      Point3F rot, pos;
   //      RayInfo rInfo;
   //      MatrixF mat = getRenderTransform();
   //      mat.getColumn(1, &rot);
   //      mat.mulP(Point3F(offset,0.0f,0.0f), &pos);
   //      if (gClientContainer.castRay(Point3F(pos.x, pos.y, pos.z + 0.01f),
   //         Point3F(pos.x, pos.y, pos.z - 2.0f ),
   //         TerrainObjectType | InteriorObjectType | VehicleObjectType, &rInfo))
   //      {
   //         S32 sound = -1;
   //         // Only put footpuffs and prints on the terrain
   //         if( rInfo.object->getTypeMask() & TerrainObjectType)
   //         {
   //            TerrainBlock* tBlock = static_cast<TerrainBlock*>(rInfo.object);

   //            // Footpuffs, if we can get the material color...
   //            S32 mapIndex = tBlock->mMPMIndex[0];
   //            if (mapIndex != -1) {
   //               MaterialPropertyMap* pMatMap = static_cast<MaterialPropertyMap*>(Sim::findObject("MaterialPropertyMap"));
   //               const MaterialPropertyMap::MapEntry* pEntry = pMatMap->getMapEntryFromIndex(mapIndex);
   //               if(pEntry)
   //               {
   //                  sound = pEntry->sound;
   //                  if( rInfo.t <= 0.5 && mWaterCoverage == 0.0f)
   //                  {
   //                     // New emitter every time for visibility reasons
   //                     ParticleEmitter * emitter = new ParticleEmitter;
   //                     emitter->onNewDataBlock( mDataBlock->footPuffEmitter );

   //                     S32 x;
   //                     ColorF colorList[ParticleEngine::PC_COLOR_KEYS];
   //
   //                     for(x = 0; x < 2; ++x)
   //                        colorList[x].set( pEntry->puffColor[x].red, pEntry->puffColor[x].green, pEntry->puffColor[x].blue, pEntry->puffColor[x].alpha );
   //                     for(x = 2; x < ParticleEngine::PC_COLOR_KEYS; ++x)
   //                        colorList[x].set( 1.0, 1.0, 1.0, 0.0 );
   //
   //                     emitter->setColors( colorList );
   //                     if( !emitter->registerObject() )
   //                     {
   //                        Con::warnf( ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName() );
   //                        delete emitter;
   //                        emitter = NULL;
   //                     }
   //                     else
   //                     {
   //                        emitter->emitParticles( pos, Point3F( 0.0, 0.0, 1.0 ), mDataBlock->footPuffRadius,
   //                                                Point3F(0, 0, 0), mDataBlock->footPuffNumParts );
   //                        emitter->deleteWhenEmpty();
   //                     }
   //                  }
   //               }
   //            }

   //            // Footprint...
   //            if (mDataBlock->decalData != NULL)
   //               mSceneManager->getCurrentDecalManager()->addDecal(rInfo.point, rot,
   //                  Point3F(rInfo.normal), getScale(), mDataBlock->decalData);
   //         }
   //         else
   //            if ( rInfo.object->getTypeMask() & VehicleObjectType)
   //               sound = 2; // Play metal sound

   //         // Play footstep sounds
   //         playFootstepSound(triggeredLeft, sound);
   //      }
   //   }
   //}

   // Mount pending variable puts a hold on the delayTicks below so players don't
   // inadvertently stand up because their mount has not come over yet.
   //if (mMountPending)
   //   mMountPending = (isMounted() ? 0 : (mMountPending - 1));

   if (mActionAnimation.action == PlayerData::NullAnimation ||
      ((!mActionAnimation.waitForEnd || mActionAnimation.atEnd)) &&
      !mActionAnimation.holdAtEnd && (mActionAnimation.delayTicks -= !mMountPending) <= 0)
   {
      //The scripting language will get a call back when a script animation has finished...
      //  example: When the chat menu animations are done playing...
      if ( isServerObject() && mActionAnimation.action >= PlayerData::NumTableActionAnims )
         Con::executef(mDataBlock,3,"animationDone",scriptThis());
      pickActionAnimation();
   }

   if ( (mActionAnimation.action != PlayerData::LandAnim) &&
      (mActionAnimation.action != PlayerData::NullAnimation) )
   {
      // Update action animation time scale to match ground velocity
      PlayerData::ActionAnimation &anim = mDataBlock->actionList[mActionAnimation.action];
      F32 scale = 1;
      if (anim.velocityScale && anim.speed)
      {
         VectorF vel;
         mWorldToObj.mulV(mVelocity,&vel);
         scale = mFabs(mDot(vel, anim.dir) / anim.speed);

         if (scale > mDataBlock->maxTimeScale)
            scale = mDataBlock->maxTimeScale;
      }

      mShapeInstance->setTimeScale(mActionAnimation.thread,
         mActionAnimation.forward? scale: -scale);
   }
   PROFILE_END();
}

//-----------------------------------------------------------------------------
bool RTSUnit::prepRenderImage(SceneState* state, const U32 stateKey,
                              const U32 startZone, const bool modifyBaseState)
{
   RTSConnection* conn = RTSConnection::getServerConnection();
   if (!conn->isUnitVisible(this))
      return false;

   return ShapeBase::prepRenderImage(state, stateKey, startZone, modifyBaseState);
}

void RTSUnit::renderImage(SceneState* state, SceneRenderImage* image)
{
   glMatrixMode(GL_MODELVIEW);

   // Base shape
   F32 fogAmount = 0;
   F32 dist = 0;
   PROFILE_START(PlayerRenderPrimary);

   GameConnection *con = GameConnection::getServerConnection();

   if (mShapeInstance && DetailManager::selectCurrentDetail(mShapeInstance))
   {
      glPushMatrix();
      dglMultMatrix(&getRenderTransform());
      glScalef(mObjScale.x,mObjScale.y,mObjScale.z);

      if (mCloakLevel != 0.0)
      {
         glMatrixMode(GL_TEXTURE);
         glPushMatrix();

         static U32 shiftX = 0;
         static U32 shiftY = 0;

         shiftX = (shiftX + 1) % 128;
         shiftY = (shiftY + 1) % 127;
         glTranslatef(F32(shiftX) / 127.0, F32(shiftY)/126.0, 0);
         glMatrixMode(GL_MODELVIEW);

         mShapeInstance->smRenderData.renderDecals = false;
         mShapeInstance->setAlphaAlways(0.04 + (1 - mCloakLevel) * 0.96);
         mShapeInstance->setOverrideTexture(mCloakTexture);
      }
      else
      {
         mShapeInstance->setAlphaAlways(1.0);
      }

      if (mCloakLevel == 0.0 && (mDataBlock->emap && gRenderEnvMaps) && state->getEnvironmentMap().getGLName() != 0)
      {
         mShapeInstance->setEnvironmentMap(state->getEnvironmentMap());
         mShapeInstance->setEnvironmentMapOn(true, 1.0);
      }
      else
      {
         mShapeInstance->setEnvironmentMapOn(false, 1.0);
      }

      Point3F cameraOffset;
      getRenderTransform().getColumn(3,&cameraOffset);
      cameraOffset -= state->getCameraPosition();
      dist = cameraOffset.len();

      bool fogExemption = false;
      GameConnection *con = GameConnection::getServerConnection();
      if(con && con->getControlObject() == this && con->isFirstPerson() == true)
         fogExemption = true;
      fogAmount = fogExemption ? 0.0 : state->getHazeAndFog(dist,cameraOffset.z);

      if( mCloakLevel > 0.0 )
      {
         fogAmount = 0.0;
      }

      TSMesh::setOverrideFade( mFadeVal );

      mShapeInstance->setupFog(fogAmount,state->getFogColor());
      mShapeInstance->animate();
      mShapeInstance->render();

      TSMesh::setOverrideFade( 1.0 );

      mShapeInstance->setEnvironmentMapOn(false, 1.0);

      if (mCloakLevel != 0.0)
      {
         glMatrixMode(GL_TEXTURE);
         glPopMatrix();

         mShapeInstance->clearOverrideTexture();
         mShapeInstance->smRenderData.renderDecals = true;
      }

      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
   }
   PROFILE_END();

   // draw the shadow...
   // only render shadow if 1) we have one, 2) we are close enough to be visible
   // 3) we are not the very last visible detail level (stop rendering shadow a little before shape)

   if (RTSUnitData* db = dynamic_cast<RTSUnitData*>(mDataBlock))
   {
      if (db->mDoShadow)
      {
         PROFILE_START(PlayerRenderShadow);
         TSMesh::setOverrideFade( mFadeVal );

         if (mShapeInstance && mCloakLevel == 0.0 &&
            mMount.object == NULL && image->isTranslucent == true)
         {
            renderShadow(dist,fogAmount);
         }
         TSMesh::setOverrideFade( 1.0 );
         PROFILE_END();
      }
   }

   dglSetCanonicalState();

   // Debugging Bounding Box
   if (!mShapeInstance || gShowBoundingBox)
   {
      Point3F box;
      glPushMatrix();
      box = (mWorkingQueryBox.min + mWorkingQueryBox.max) * 0.5;
      glTranslatef(box.x,box.y,box.z);
      box = (mWorkingQueryBox.max - mWorkingQueryBox.min) * 0.5;
      glScalef(box.x,box.y,box.z);
      glColor3f(1, 1, 0);
      wireCube(Point3F(1,1,1),Point3F(0,0,0));
      glPopMatrix();

      Box3F convexBox = mConvex.getBoundingBox(getRenderTransform(), getScale());
      glPushMatrix();
      box = (convexBox.min + convexBox.max) * 0.5;
      glTranslatef(box.x,box.y,box.z);
      box = (convexBox.max - convexBox.min) * 0.5;
      glScalef(box.x,box.y,box.z);
      glColor3f(1, 1, 1);
      wireCube(Point3F(1,1,1),Point3F(0,0,0));
      glPopMatrix();

      glEnable(GL_DEPTH_TEST);
   }
}

//-----------------------------------------------------------------------------

U32 RTSUnit::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = GameBase::packUpdate(con, mask, stream);

   if (mask & InitialUpdateMask)
   {
      // mask off sounds that aren't playing
      S32 i;
      for (i = 0; i < MaxSoundThreads; i++)
         if (!mSoundThread[i].play)
            mask &= ~(SoundMaskN << i);

      // mask off threads that aren't running
      for (i = 0; i < MaxScriptThreads; i++)
         if (mScriptThread[i].sequence == -1)
            mask &= ~(ThreadMaskN << i);

      // mask off images that aren't updated
      for(i = 0; i < MaxMountedImages; i++)
         if(!mMountedImageList[i].dataBlock)
            mask &= ~(ImageMaskN << i);
   }

   if(!stream->writeFlag(mask & (DamageMask | SoundMask | ThreadMask | SkinMask |
      ActionMask | AIMask | InitialUpdateMask | CloakMask | ImpactMask | 
      MoveMask | ModifierMask | ProjectileMask)))
      return retMask;

   // ShapeBase stuff
   if (stream->writeFlag((mask & ThreadMask) && (mDamageState != Destroyed)))
   {
      for (int i = 0; i < MaxScriptThreads; i++)
      {
         Thread& st = mScriptThread[i];
         if (stream->writeFlag(st.sequence != -1 && (mask & (ThreadMaskN << i))))
         {
            stream->writeInt(st.sequence,ThreadSequenceBits);
            stream->writeInt(st.state,2);
            stream->writeFlag(st.forward);
            stream->writeFlag(st.atEnd);
         }
      }
   }

   if (stream->writeFlag(mask & DamageMask))
   {
      stream->writeFloat(mClampF(mDamage / mDataBlock->maxDamage, 0.f, 1.f), DamageLevelBits);
      stream->writeInt(mDamageState,NumDamageStateBits);

      // Irrelevant for an RTS -- BJG
      /*stream->writeNormalVector( damageDir, 8 ); */
   }

   if (stream->writeFlag(mask & SoundMask))
   {
      // Better to fake it with play3daudio. -- BJG
      /*
      for (int i = 0; i < MaxSoundThreads; i++)
      {
      Sound& st = mSoundThread[i];
      if (stream->writeFlag(mask & (SoundMaskN << i)))
      if (stream->writeFlag(st.play))
      stream->writeRangedU32(st.profile->getId(),DataBlockObjectIdFirst,
      DataBlockObjectIdLast);
      }*/
   }

   if(stream->writeFlag(mask & SkinMask))
      con->packStringHandleU(stream,mSkinNameHandle);

   // Player stuff
   if (stream->writeFlag((mask & ImpactMask) && !(mask & InitialUpdateMask)))
      stream->writeInt(mImpactSound, PlayerData::ImpactBits);

   if (stream->writeFlag(mask & ActionMask &&
      mActionAnimation.action != PlayerData::NullAnimation &&
      mActionAnimation.action >= PlayerData::NumTableActionAnims))
   {
      stream->writeInt(mActionAnimation.action,PlayerData::ActionAnimBits);
      stream->writeFlag(mActionAnimation.holdAtEnd);
      stream->writeFlag(mActionAnimation.atEnd);
      stream->writeFlag(mActionAnimation.firstPerson);
      if (!mActionAnimation.atEnd)
      {
         // If somewhere in middle on initial update, must send position-
         F32 where = mShapeInstance->getPos(mActionAnimation.thread);
         if (stream->writeFlag((mask & InitialUpdateMask) != 0 && where > 0))
            stream->writeSignedFloat(where, 5);
      }
   }

   if (stream->writeFlag(mask & ActionMask &&
      mArmAnimation.action != PlayerData::NullAnimation &&
      (!(mask & InitialUpdateMask) ||
      mArmAnimation.action != mDataBlock->lookAction)))
   {
      stream->writeInt(mArmAnimation.action,PlayerData::ActionAnimBits);
   }

   if (stream->writeFlag(mask & MoveMask))
   {
      stream->writeFlag(mFalling);

      stream->writeInt(mState,NumStateBits);

      Point3F pos;
      getTransform().getColumn(3,&pos);
      stream->writeCompressedPoint(pos, 0.1f);
      setDirty();


      F32 len = mVelocity.len();
      if(stream->writeFlag(len > 0.02))
      {
         Point3F outVel = mVelocity;
         outVel *= 1/len;
         stream->writeNormalVector(outVel, 8, 2);
         len *= 32.0;  // 5 bits of fraction
         if(len > 8191)
            len = 8191;
         stream->writeInt((S32)len, 13);
      }
      stream->writeFloat(mRot.z / M_2PI, 6);
      delta.move.pack(stream);
      stream->writeFlag(!(mask & NoWarpMask));
   }

   // Ghost needs energy to predict reliably (but not in RTS -- BJG)
   //   stream->writeFloat(getEnergyLevel() / mDataBlock->maxEnergy,EnergyLevelBits);

   // Transmit AI state.
   if (stream->writeFlag(mask & AIMask))
   {
      stream->writeInt(mMoveState, 2);

      if(mMoveState != ModeStop)
      {
         // If we didn't previously write a position, send it over.
         stream->writeCompressedPoint(mMoveDestination, 0.1f);

         AssertISV( 0.f <= mMoveTolerance && mMoveTolerance <= 4.f, "Invalid movetolerance (0..4)!");
         stream->writeFloat(mMoveTolerance / 4.f, 5);
      }

      if (mAimObjectSet)
      {
         S32 ghostId = con->getGhostIndex(mAimObject);
         if (stream->writeFlag(ghostId != -1))
            stream->writeInt(ghostId, NetConnection::GhostIdBitSize);
      }
      else
         stream->writeFlag(false);

      if(stream->writeFlag(mAimLocationSet))
      {
         stream->writeCompressedPoint(mAimLocation, 0.1f);
      }

      AssertISV( 0.f <= mMoveVelocity && mMoveVelocity <= 32.f, "Invalid moveVelocity (0..32)!");
      stream->writeFloat(mMoveVelocity / 32.f, 8);
   }

   if (stream->writeFlag(mask & ModifierMask))
   {
      AssertISV(mModifierList.size() < 32, "Max number of active modifiers on a unit is 16!");
      stream->writeInt(mModifierList.size(), 5);
      for (U32 k = 0; k < mModifierList.size(); k++)
      {
         stream->write(mModifierList[k]->getId());
      }
   }

   // Team and class
   if(stream->writeFlag(mask & InitialUpdateMask))
   {
      AssertISV(getTeam() < 64, "Can only pack 64 unique teams!");
      stream->writeInt(getTeam(), 5);
   }

   // Projectile datablock
   if(stream->writeFlag((mask & ProjectileMask) && (mCurrentProjectile != NULL)))
   {
      stream->writeRangedU32(mCurrentProjectile->getId(),
         DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
   }

   // Fade out on death
   if (stream->writeFlag(mask & CloakMask)) 
   {
      // cloaking
      stream->writeFlag( mCloaked );

      // fading
      if(stream->writeFlag(mFading && mFadeElapsedTime >= mFadeDelay)) {
         stream->writeFlag(mFadeOut);
         stream->write(mFadeTime);
      }
      else
         stream->writeFlag(mFadeVal == 1.0f);
   }

   return retMask;
}

void RTSUnit::unpackUpdate(NetConnection *con, BitStream *stream)
{
   GameBase::unpackUpdate(con,stream);

   mLastRenderFrame = sLastRenderFrame; // make sure we get a process after the event...

   if(!stream->readFlag())
      return;

   // ShapeBase stuff
   if (stream->readFlag())
   {
      for (S32 i = 0; i < MaxScriptThreads; i++)
      {
         if (stream->readFlag())
         {
            Thread& st = mScriptThread[i];
            U32 seq = stream->readInt(ThreadSequenceBits);
            st.state = stream->readInt(2);
            st.forward = stream->readFlag();
            st.atEnd = stream->readFlag();
            if (st.sequence != seq)
               setThreadSequence(i,seq,false);
            else
               updateThread(st);
         }
      }
   }

   if (stream->readFlag())
   {
      mDamage = mClampF(stream->readFloat(DamageLevelBits) * mDataBlock->maxDamage, 0.f, mDataBlock->maxDamage);
      DamageState prevState = mDamageState;
      mDamageState = DamageState(stream->readInt(NumDamageStateBits));

      // Don't need dir for RTS.
      //stream->readNormalVector( &damageDir, 8 );

      if (prevState != Destroyed && mDamageState == Destroyed && isProperlyAdded())
         blowUp();

      updateDamageLevel();
      updateDamageState();
   }

   if (stream->readFlag())
   {
      // Don't need this for an RTS -- BJG
      /*
      for (S32 i = 0; i < MaxSoundThreads; i++)
      {
      if (stream->readFlag())
      {
      Sound& st = mSoundThread[i];
      if ((st.play = stream->readFlag()) == true)
      st.profile = (AudioProfile*) stream->readRangedU32(DataBlockObjectIdFirst,
      DataBlockObjectIdLast);
      if (isProperlyAdded())
      updateAudioState(st);
      }
      }*/
   }

   if (stream->readFlag())  // SkinMask
   {
      StringHandle skinDesiredNameHandle = con->unpackStringHandleU(stream);
      if (mSkinNameHandle != skinDesiredNameHandle)
      {
         mSkinNameHandle = skinDesiredNameHandle;
         if (mShapeInstance)
         {
            mShapeInstance->reSkin(mSkinNameHandle);
            if (mSkinNameHandle.isValidString())
               mSkinHash = _StringTable::hashString(mSkinNameHandle.getString());
         }
      }
   }

   // Player stuff
   if (stream->readFlag())
      mImpactSound = stream->readInt(PlayerData::ImpactBits);

   // Server specified action animation
   if (stream->readFlag())
   {
      U32 action = stream->readInt(PlayerData::ActionAnimBits);
      bool hold  = stream->readFlag();
      bool atEnd = stream->readFlag();
      bool fsp   = stream->readFlag();

      F32   animPos = -1.0;
      if (!atEnd && stream->readFlag())
         animPos = stream->readSignedFloat(5);

      if (isProperlyAdded())
      {
         setActionThread(action,true,hold,true,fsp);
         bool  inDeath = inDeathAnim();
         if (atEnd)
         {
            mShapeInstance->clearTransition(mActionAnimation.thread);
            mShapeInstance->setPos(mActionAnimation.thread,
               mActionAnimation.forward ? 1 : 0);
            if (inDeath)
               mDeath.lastPos = 1.0;
         }
         else if (animPos > 0)
         {
            mShapeInstance->setPos(mActionAnimation.thread, animPos);
            if (inDeath)
               mDeath.lastPos = animPos;
         }

         // mMountPending suppresses tickDelay countdown so players will sit until
         // their mount, or another animation, comes through (or 13 seconds elapses).
         mMountPending = (S32) (inSittingAnim() ? sMountPendingTickWait : 0);
      }
      else
      {
         mActionAnimation.action      = action;
         mActionAnimation.holdAtEnd   = hold;
         mActionAnimation.atEnd       = atEnd;
         mActionAnimation.firstPerson = fsp;
      }
   }

   // Server specified arm animation
   if (stream->readFlag())
   {
      U32 action = stream->readInt(PlayerData::ActionAnimBits);
      if (isProperlyAdded())
         setArmThread(action);
      else
         mArmAnimation.action = action;
   }

   if (stream->readFlag())
   {
      mPredictionCount = sMaxPredictionTicks;
      mFalling = stream->readFlag();

      ActionState actionState = (ActionState)stream->readInt(NumStateBits);
      setState(actionState);

      Point3F pos,rot;
      stream->readCompressedPoint(&pos, 0.1f);

      F32 speed = mVelocity.len();
      if(stream->readFlag())
      {
         stream->readNormalVector(&mVelocity, 8, 2);
         mVelocity *= stream->readInt(13) / 32.0f;
      }
      else
         mVelocity.set(0,0,0);


      rot.y = rot.x = 0;
      rot.z = stream->readFloat(6) * M_2PI;
      delta.move.unpack(stream);

      delta.head = mHead;
      delta.headVec.set(0,0,0);

      if (stream->readFlag() && isProperlyAdded())
      {
         // Determine number of ticks to warp based on the average
         // of the client and server velocities.
         delta.warpOffset = pos - delta.pos;
         F32 as = (speed + mVelocity.len()) * 0.5 * TickSec;
         F32 dt = (as > 0.00001f) ? delta.warpOffset.len() / as: sMaxWarpTicks;
         delta.warpTicks = (S32)((dt > sMinWarpTicks) ? getMax(mFloor(dt + 0.5), 1.0f) : 0.0f);

         if (delta.warpTicks)
         {
            // Setup the warp to start on the next tick.
            if (delta.warpTicks > sMaxWarpTicks)
               delta.warpTicks = sMaxWarpTicks;
            delta.warpOffset /= delta.warpTicks;

            delta.rotOffset = rot - delta.rot;
            if(delta.rotOffset.z < - M_PI)
               delta.rotOffset.z += M_2PI;
            else if(delta.rotOffset.z > M_PI)
               delta.rotOffset.z -= M_2PI;
            delta.rotOffset /= delta.warpTicks;
         }
         else
         {
            // Going to skip the warp, server and client are real close.
            // Adjust the frame interpolation to move smoothly to the
            // new position within the current tick.
            Point3F cp = delta.pos + delta.posVec * delta.dt;
            if (delta.dt == 0)
            {
               delta.posVec.set(0,0,0);
               delta.rotVec.set(0,0,0);
            }
            else
            {
               F32 dti = 1 / delta.dt;
               delta.posVec = (cp - pos) * dti;
               delta.rotVec.z = mRot.z - rot.z;

               if(delta.rotVec.z > M_PI)
                  delta.rotVec.z -= M_2PI;
               else if(delta.rotVec.z < -M_PI)
                  delta.rotVec.z += M_2PI;

               delta.rotVec.z *= dti;
            }
            delta.pos = pos;
            delta.rot = rot;
            setPosition(pos,rot);
         }
      }
      else
      {
         // Set the player to the server position
         delta.pos = pos;
         delta.rot = rot;
         delta.posVec.set(0,0,0);
         delta.rotVec.set(0,0,0);
         delta.warpTicks = 0;
         delta.dt = 0;
         setPosition(pos,rot);
      }
      gClientVisManager->setDirty();
   }

   // Ghost needs energy to predict reliably (but not in RTS -- BJG)
   //F32 energy = stream->readFloat(EnergyLevelBits) * mDataBlock->maxEnergy;
   //setEnergyLevel(energy);

   // Transmit AI state.
   if (stream->readFlag())
   {
      mMoveState = (RTSUnit::MoveState)stream->readInt(2);

      if(mMoveState != ModeStop)
      {
         stream->readCompressedPoint(&mMoveDestination, 0.1f);
         mMoveTolerance = stream->readFloat(5) * 4.f;
      }

      if(mAimObjectSet = stream->readFlag())
      {
         S32 aimObjId = stream->readInt(NetConnection::GhostIdBitSize);
         mAimObject = (GameBase*)con->resolveGhost(aimObjId);
      }

      if(mAimLocationSet = stream->readFlag())
      {
         stream->readCompressedPoint(&mAimLocation, 0.1f);
      }

      mMoveVelocity = stream->readFloat(8) * 32.f;
   }

   if (stream->readFlag())
   {
      // Clear the modifier list...
      U32 size = mModifierList.size();
      for (U32 k = 0; k < size; k++)
         removeModifier((RTSUnitModifierData*)mModifierList[size-1-k]);

      AssertFatal(mModifierList.empty(), "Modifier list not empty!!");

      size = stream->readInt(5);
      for (U32 k = 0; k < size; k++)
      {
         U32 id;
         stream->read(&id);
         if (RTSUnitModifierData* mod = dynamic_cast<RTSUnitModifierData*>(Sim::findObject(id)))
            addModifier(mod);
         else
            AssertFatal(false, "BAD MODIFIER ON CLIENT!!!");
      }
   }

   // Team
   if(stream->readFlag())
   {
      setTeam(stream->readInt(5));
   }

   // Projectile datablock
   if(stream->readFlag())
   {
      ProjectileData* dptr = NULL;
      SimObjectId id = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);

      if (!Sim::findObject(id,dptr))
         con->setLastError("Failed to load projectile datablock.");

      mCurrentProjectile = dptr;
   }

   // Fade status...
   if(stream->readFlag())
   {
      setCloakedState(stream->readFlag());

      if (( mFading = stream->readFlag()) == true) {
         mFadeOut = stream->readFlag();
         if(mFadeOut)
            mFadeVal = 1.0f;
         else
            mFadeVal = 0;
         stream->read(&mFadeTime);
         mFadeDelay = 0;
         mFadeElapsedTime = 0;
      }
      else
         mFadeVal = F32(stream->readFlag());


   }
}

F32 RTSUnit::getUpdatePriority(CameraScopeQuery *focusObject, U32 updateMask, S32 updateSkips)
{
   return Parent::getUpdatePriority(focusObject, updateMask, updateSkips);

   /*  //this is what we want to do...
   //
   // SCREEN TEST consists of high priority for on screen, middle priority for
   //             almost on screen, and low priority for not close to on screen
   // FOG OF WAR TEST consist of checking to see that the unit is visible to
   //                 any other friendly units
   //
   // Note that the fog of war test could include a team test
   //
   // Test team (See if the unit is the same team as the client)
   //  true
   //    SCREEN TEST
   //  false
   //    FOG OF WAR TEST
   //     true
   //       SCREEN TEST
   //     false
   //       set 0 priority
   //
   // Basically, if the unit is visible, we want to use a screen test to see
   // where the unit is in relation to the viewable space.  If the unit is not
   // visible, then set priority to 0.

   Point3F pos = getPosition();

   //TODO: test fog of war in here somewhere...
   F32 fogOfWarInterest;
   if (true)
   {
   //able to see it through fog of war
   fogOfWarInterest = 0.1;
   }

   F32 onScreenInterest = 0.0f;
   if (RTSCamera* camera = dynamic_cast<RTSCamera*>(focusObject->camera))
   {
   RectF viewableRect = camera->getViewableRect();
   if (pos.x > viewableRect.point.x &&
   pos.x < viewableRect.point.x + viewableRect.extent.x &&
   pos.y > viewableRect.point.y &&
   pos.y < viewableRect.point.y + viewableRect.extent.y)
   {
   //on screen
   onScreenInterest = 1.0f;
   }
   else
   {
   //see if it's bordering the camera
   viewableRect.point.x -= 20.0f;
   viewableRect.point.y -= 20.0f;
   viewableRect.extent.x += 40.0f;
   viewableRect.extent.y += 40.0f;

   if (pos.x > viewableRect.point.x &&
   pos.x < viewableRect.point.x + viewableRect.extent.x &&
   pos.y > viewableRect.point.y &&
   pos.y < viewableRect.point.y + viewableRect.extent.y)
   {
   //just off screen
   onScreenInterest = 0.5f;
   }
   else
   onScreenInterest = 0.2f;
   }
   }

   F32 totalInterest = onScreenInterest + fogOfWarInterest;
   return totalInterest; */
}

void RTSUnit::setTeam(S32 team)
{
   mTeam = team;
   setDirty();
   if (isClientObject())
      gClientVisManager->onSetTeam(this);
}

void RTSUnit::setControllingConnection(RTSConnection* conn)
{
   if (!conn)
      return;

   if (mControllingConnection)
      mControllingConnection->setUnitInvisible(this);

   gServerVisManager->onConnectionSet(this, conn);

   mControllingConnection = conn;
   setDirty();
}

void RTSUnit::clearControllingConnection()
{
   if( mControllingConnection )
      mControllingConnection->setUnitInvisible(this);

   mControllingConnection = NULL;
}

void RTSUnit::setDirty()
{
   mPositionDirty = true;
   if (isServerObject())
      gServerVisManager->setDirty();
   else
      gClientVisManager->setDirty();
}

// --------------------------------------------------------------------------------------------
// Console Functions
// --------------------------------------------------------------------------------------------

ConsoleMethod( RTSUnit, stop, void, 2, 2, "()"
              "Stop moving.")
{
   object->stopMove();
}

ConsoleMethod( RTSUnit, clearAim, void, 2, 2, "()"
              "Stop aiming at anything.")
{
   object->clearAim();
}

ConsoleMethod( RTSUnit, setMoveDestination, void, 3, 3, "(Point3F goal)"
              "Tells the AI to move to the location provided.")
{
   Point3F v( 0.0f, 0.0f, 0.0f );
   dSscanf( argv[2], "%f %f %f", &v.x, &v.y, &v.z );
   object->setMoveDestination( v);
}

ConsoleMethod( RTSUnit, getMoveDestination, const char *, 2, 2, "()"
              "Returns the point the AI is set to move to.")
{
   Point3F movePoint = object->getMoveDestination();

   char *returnBuffer = Con::getReturnBuffer( 256 );
   dSprintf( returnBuffer, 256, "%f %f %f", movePoint.x, movePoint.y, movePoint.z );

   return returnBuffer;
}

ConsoleMethod( RTSUnit, setAimLocation, void, 3, 3, "( Point3F target )"
              "Tells the AI to aim at the location provided.")
{
   Point3F v( 0.0f,0.0f,0.0f );
   dSscanf( argv[2], "%f %f %f", &v.x, &v.y, &v.z );

   object->setAimLocation( v );
}

ConsoleMethod( RTSUnit, getAimLocation, const char *, 2, 2, "()"
              "Returns the point the AI is aiming at.")
{
   Point3F aimPoint = object->getAimLocation();

   char *returnBuffer = Con::getReturnBuffer( 256 );
   dSprintf( returnBuffer, 256, "%f %f %f", aimPoint.x, aimPoint.y, aimPoint.z );

   return returnBuffer;
}

ConsoleMethod( RTSUnit, setAimObject, void, 3, 3, "( GameBase obj )"
              "Sets the bot's target object.")
{
   // Find the target
   GameBase *targetObject;

   if( Sim::findObject( argv[2], targetObject ) )
   {
      object->setAimObject( targetObject );
   }
   else
   {
      Con::warnf("RTSUnit::setAimObject - No object found!");
      object->setAimObject( NULL );
   }
}

ConsoleMethod( RTSUnit, getAimObject, S32, 2, 2, "()"
              "Gets the object the AI is targeting.")
{
   GameBase* obj = object->getAimObject();
   return obj? obj->getId(): -1;
}

ConsoleMethod( RTSUnit, addModifier, void, 3, 3, "( RTSUnitModifierData modifier )"
              "Adds a modifier to this unit")
{
   RTSUnitModifierData* data = dynamic_cast<RTSUnitModifierData*>(Sim::findObject(dAtoi(argv[2])));
   if (data)
      object->addModifier(data);
}

ConsoleMethod( RTSUnit, removeModifier, void, 3, 3, "( RTSUnitModifierData modifier )"
              "Removes a modifier from this unit")
{
   RTSUnitModifierData* data = dynamic_cast<RTSUnitModifierData*>(Sim::findObject(dAtoi(argv[2])));
   if (data)
      object->removeModifier(data);
}

ConsoleMethod( RTSUnit, getNetModifier, S32, 2, 2, "()"
              "Gets a modifier object which is the sum of all modifiers applied to this object")
{
   return object->mNetModifier.getId();
}

ConsoleMethod( RTSUnit, getModifierList, S32, 2, 2, "()"
              "Gets a simset with all modifiers applied to this object")
{
   return object->mModifierList.getId();
}

ConsoleMethod( RTSUnit, setTeam, void, 3, 3, "( S32 teamID )"
              "Sets the units team.")
{
   object->setTeam(dAtoi(argv[2]));
}

ConsoleMethod( RTSUnit, getTeam, S32, 2, 2, "()"
              "Returns the units teamID.")
{
   return object->getTeam();
}

ConsoleMethod( RTSUnit, setProjectileDatablock, void, 3, 3, "(ProjectileData block)")
{
   if(!Sim::findObject(argv[2], object->mCurrentProjectile))
   {
      Con::errorf("RTSUnit::setProjectileDatablock - failed to find ProjectileData '%s'", argv[2]);
      return;
   }

   object->setMaskBits(RTSUnit::ProjectileMask);
}

ConsoleMethod( RTSUnit, getProjectileDatablock, S32, 2, 2, "()" )
{
   if(object->mCurrentProjectile)
      return object->mCurrentProjectile->getId();
   else
      return -1;
}

ConsoleMethod( RTSUnit, setControllingConnection, void, 3, 3, "(RTSConnection conn)"
              "Sets the connection which owns this object.")
{
   RTSConnection* conn;

   if (!Sim::findObject(dAtoi(argv[2]), conn))
      return;

   object->setControllingConnection(conn);
}
