//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/profiler.h"
#include "dgl/dgl.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "core/dnet.h"
#include "game/RTS/RTSConnection.h"
#include "sceneGraph/sceneGraph.h"
#include "math/mathIO.h"
#include "game/RTS/RTSCamera.h"
#include "terrain/sky.h"
#include "game/RTS/guiRTSTSCtrl.h"
#include "game/RTS/RTSUnit.h"
#include "core/realComp.h"
//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(RTSCameraData);

const F32 gDegsPerTick = mDegToRad(2.0f);

RTSCameraData::RTSCameraData()
{
   mMovementSpeed = 40;
   mPitchAngle    = 90;

   mMaxOrbitHeight = 1000;
   mMinOrbitHeight = 5;
   mOrbitStep = 10;
   mMaxAngle = 90;
   mMinAngle = 20;
   mAngleStep = 5;
}

void RTSCameraData::packData(BitStream *stream)
{
   Parent::packData(stream);

   stream->write(mMovementSpeed);
   stream->write(mPitchAngle);

   stream->write(mMaxOrbitHeight);
   stream->write(mMinOrbitHeight);

   stream->write(mOrbitStep);
}

void RTSCameraData::unpackData(BitStream *stream)
{
   Parent::unpackData(stream);

   stream->read(&mMovementSpeed);
   stream->read(&mPitchAngle);

   stream->read(&mMaxOrbitHeight);
   stream->read(&mMinOrbitHeight);

   stream->read(&mOrbitStep);
}

void RTSCameraData::initPersistFields()
{
   Parent::initPersistFields();

   addField("movementSpeed", TypeF32, Offset(mMovementSpeed, RTSCameraData));
   addField("pitchAngle",    TypeF32, Offset(mPitchAngle,    RTSCameraData));

   addField("maxHeight",     TypeF32, Offset(mMaxOrbitHeight, RTSCameraData));
   addField("minHeight",     TypeF32, Offset(mMinOrbitHeight, RTSCameraData));
   addField("orbitStep",     TypeF32, Offset(mOrbitStep,     RTSCameraData));
   addField("angleStep",     TypeF32, Offset(mAngleStep,     RTSCameraData));

   addField("maxAngle",      TypeF32, Offset(mMaxAngle,      RTSCameraData));
   addField("minAngle",      TypeF32, Offset(mMinAngle,      RTSCameraData));
}

//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(RTSCamera);

RTSCamera::RTSCamera()
{
   mNetFlags.set(Ghostable);
   mTypeMask |= CameraObjectType;

   mCurrAngle = mTargetAngle = mPrevAngle          = mDegToRad(90.0f);
   mCurrYawAngle = mTargetYawAngle = mPrevYawAngle = mDegToRad(0.0f);

   mTargetHeight = mPrevHeight = mCurrHeight = mRenderHeight = 100.0f;
   mTerrain = NULL;
}

bool RTSCamera::onAdd()
{
   if(!Parent::onAdd())
      return false;

   mObjBox.max = mObjScale;
   mObjBox.min = mObjScale;
   mObjBox.min.neg();
   resetWorldBox();

   if(isClientObject())
   {
      gClientContainer.addObject(this);
      mTerrain = gClientSceneGraph->getCurrentTerrain();
   }
   else
   {
      gServerContainer.addObject(this);
      mTerrain = gServerSceneGraph->getCurrentTerrain();
   }
   mTargetPos.x = getPosition().x;
   mTargetPos.y = getPosition().y;

   return true;
}

void RTSCamera::onRemove()
{
   if (getContainer())
      getContainer()->removeObject(this);

   Parent::onRemove();
}

bool RTSCamera::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<RTSCameraData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   // set the view angle
   setPitchAngle(mDataBlock->mPitchAngle, false, false);

   return true;
}

void RTSCamera::setCameraPosition(Point2F pos)
{
   F32 realHeight = getTerrHeight(pos) + mCurrHeight;

   Point3F dir;
   Point3F startPos = Point3F(pos.x, pos.y, realHeight);
   Point3F endPos;

   mObjToWorld.getColumn(1, &dir);
   endPos = startPos - dir * mTargetHeight;
   endPos.z = getTerrHeight(Point2F(endPos.x, endPos.y)) + mCurrHeight;
   mTargetPos = mPrevPos = Point2F(endPos.x, endPos.y);

   setPosition(endPos);

   // tell the server what we are doing
   if(isClientObject())
   {
      GameConnection *conn = GameConnection::getServerConnection();
      if(conn)
         conn->postNetEvent(new RTSCameraUpdate(this, false));
   }
}

void RTSCamera::setPitchAngle(F32 angle, bool updateNet, bool toInterpolate)
{
   RTSCameraData* data = (RTSCameraData*)mDataBlock;
   mTargetAngle = mDegToRad(mClampF(angle, data->mMinAngle, data->mMaxAngle));
   if (!toInterpolate)
   {
      mCurrAngle = mTargetAngle;
      MatrixF xRot, zRot;
      xRot.set(EulerF(mCurrAngle, 0, 0));
      zRot.set(EulerF(0, 0, mCurrYawAngle));
      MatrixF temp;
      temp.mul(zRot, xRot);
      setTransform(temp);
   }

   if(updateNet)
   {
      // Send a net update
      GameConnection *conn = getControllingClient();
      if(!conn)
      {
         Con::warnf("This camera's not in control of anything!");
         return;
      }

      RTSCameraUpdate *rtsCamUpdate = new RTSCameraUpdate(this, !isClientObject());
      rtsCamUpdate->setPitchAngle(angle);

      conn->postNetEvent(rtsCamUpdate);
   }
}

void RTSCamera::setYawAngle(F32 angle, bool updateNet, bool toInterpolate)
{
   RTSCameraData* data = (RTSCameraData*)mDataBlock;
   mTargetYawAngle = mDegToRad(angle);

   // Make sure we're not getting cranzy angles!
   while(mTargetYawAngle > M_2PI_F)
   {
      mTargetYawAngle -= M_2PI_F;
      mCurrYawAngle   -= M_2PI_F;
      mPrevYawAngle   -= M_2PI_F;
   }
   while(mTargetYawAngle < -M_2PI_F)
   {
      mTargetYawAngle += M_2PI_F;
      mCurrYawAngle   += M_2PI_F;
      mPrevYawAngle   += M_2PI_F;
   }

   if (!toInterpolate)
   {
      mCurrYawAngle = mTargetYawAngle;
      MatrixF xRot, zRot;
      xRot.set(EulerF(mCurrAngle, 0, 0));
      zRot.set(EulerF(0, 0, mCurrYawAngle));
      MatrixF temp;
      temp.mul(zRot, xRot);
      setTransform(temp);
   }

   if(updateNet)
   {
      // Send a net update
      GameConnection *conn = getControllingClient();
      if(!conn)
      {
         Con::warnf("This camera´s not in control of anything!");
         return;
      }

      RTSCameraUpdate *rtsCamUpdate = new RTSCameraUpdate(this, !isClientObject());
      rtsCamUpdate->setPitchAngle(angle);

      conn->postNetEvent(rtsCamUpdate);
   }
}

Point3F RTSCamera::getLookPosition()
{
   Point3F dir;
   Point3F pos = getPosition();
   mObjToWorld.getColumn(1, &dir);

   F32 div = mSin(mCurrAngle);
   if (isZero(div))
      div = 0.001;
   return pos + dir * mCurrHeight / div;
}

void RTSCamera::getCameraTransform(F32* pos, MatrixF* mat)
{
   getEyeTransform(mat);
}

RectF RTSCamera::getViewableRect()
{
   Point3F center = getLookPosition();

   F32 fov = getCameraFov();
   F32 pct = mSin(fov);
   F32 height = mCurrHeight;

   F32 div = mSin(mCurrAngle);
   if (isZero(div))
      div = 0.001;

   F32 w = pct * height;
   F32 h = pct * height * 0.75 / div;
   return RectF(center.x - w, center.y - h, w*2, h*2);
}

void RTSCamera::setOrbitDistance(F32 dist, bool updateNet)
{
   mTargetHeight = mClampF(dist, mDataBlock->mMinOrbitHeight, mDataBlock->mMaxOrbitHeight);

   if(updateNet)
   {
      // Send a net update
      GameConnection *conn = getControllingClient();
      if(!conn)
      {
         Con::warnf("This camera's not in control of anything!");
         return;
      }

      RTSCameraUpdate *rtsCamUpdate = new RTSCameraUpdate(this, !isClientObject());
      rtsCamUpdate->setOrbitDistance(mTargetHeight);

      conn->postNetEvent(rtsCamUpdate);
   }
}

void RTSCamera::processTick(const Move* move)
{
   Parent::processTick(move);

   mPrevPos      = mTargetPos;
   mPrevYawAngle = mCurrYawAngle;
   mPrevHeight   = mCurrHeight;
   mPrevAngle    = mCurrAngle;

   if (mTargetHeight != mCurrHeight)
   {
      if (mCurrHeight < mTargetHeight)
      {
         F32 step = mDataBlock->mMovementSpeed * TickSec;
         if (mCurrHeight + step > mTargetHeight)
            mCurrHeight = mTargetHeight;
         else
            mCurrHeight += step;
      }
      else
      {
         F32 step = -mDataBlock->mMovementSpeed * TickSec;
         if (mCurrHeight + step < mTargetHeight)
            mCurrHeight = mTargetHeight;
         else
            mCurrHeight += step;
      }
   }

   if (mTargetAngle != mCurrAngle)
   {
      if (mCurrAngle < mTargetAngle)
      {
         F32 step = gDegsPerTick;
         if (mCurrAngle + step > mTargetAngle)
            mCurrAngle = mTargetAngle;
         else
            mCurrAngle += step;
      }
      else
      {
         F32 step = -gDegsPerTick;
         if (mCurrAngle + step < mTargetAngle)
            mCurrAngle = mTargetAngle;
         else
            mCurrAngle += step;
      }
   }

   if (mTargetYawAngle != mCurrYawAngle)
   {
      if (mCurrYawAngle < mTargetYawAngle)
      {
         F32 step = gDegsPerTick;
         if (mCurrYawAngle + step > mTargetYawAngle)
            mCurrYawAngle = mTargetYawAngle;
         else
            mCurrYawAngle += step;
      }
      else
      {
         F32 step = -gDegsPerTick;
         if (mCurrYawAngle + step < mTargetYawAngle)
            mCurrYawAngle = mTargetYawAngle;
         else
            mCurrYawAngle += step;
      }
   }

   MatrixF xRot, zRot;
   xRot.set(EulerF(mPrevAngle, 0, 0));
   zRot.set(EulerF(0, 0, mPrevYawAngle));
   MatrixF temp;
   temp.mul(zRot, xRot);

   // Update pos
   Point3F pos;
   pos.x = mTargetPos.x;
   pos.y = mTargetPos.y;
   pos.z = getTerrHeight(mTargetPos) + mCurrHeight;

   //setPosition(pos);
   temp.setPosition(pos);
   setTransform(temp);

   if (move)
   {
      // Move appropriately based on our direction.
      F32 tempX, tempY;
      tempX = move->x * TickSec * mDataBlock->mMovementSpeed;
      tempY = move->y * TickSec * mDataBlock->mMovementSpeed;
      mTargetPos.x += ((tempX * mCos(-1 * mCurrYawAngle)) - (tempY * mSin(-1 * mCurrYawAngle)));
      mTargetPos.y += ((tempY * mCos(-1 * mCurrYawAngle)) + (tempX * mSin(-1 * mCurrYawAngle)));
         
      // tell the server what we are doing
      if(isClientObject())
      {
         GameConnection *conn = GameConnection::getServerConnection();
         if(conn)
            conn->postNetEvent(new RTSCameraUpdate(this, false));
      }
   }

   if(getControllingClient() && mContainer)
      updateContainer();
}

void RTSCamera::interpolateTick(F32 dt)
{
   // We are occasionally requested to interp to 0 dt, and we're lazy...
   // so we don't. If you notice visual shizzle happening, you might want to
   // make this more robust -- BJG
   if (dt == 0.0)
      return;

   F32 interpHeight = mCurrHeight;
   bool toUpdate = false;
   Point3F pos = getPosition();

   if ((mTargetPos - mPrevPos).lenSquared() > 0.01)
   {
      Point2F tickStep = mTargetPos - mPrevPos;

      pos.x = mPrevPos.x + tickStep.x * (1-dt);
      pos.y = mPrevPos.y + tickStep.y * (1-dt);

      toUpdate = true;
   }

   if (mPrevHeight != mCurrHeight)
   {
      F32 tickStep = mDataBlock->mMovementSpeed * TickSec * (mPrevHeight < mCurrHeight ? 1 : -1);
      interpHeight = mPrevHeight + tickStep * (1-dt);
      if (tickStep < 0)
      {
         if (interpHeight < mCurrHeight)
            mPrevHeight = interpHeight = mCurrHeight;
      }
      else
      {
         if (interpHeight > mCurrHeight)
            mPrevHeight = interpHeight = mCurrHeight;
      }

      MatrixF xRot, zRot;
      xRot.set(EulerF(mPrevAngle, 0, 0));
      zRot.set(EulerF(0, 0, mPrevYawAngle));
      MatrixF temp;
      temp.mul(zRot, xRot);

      toUpdate = true;
   }

   if (mPrevAngle != mCurrAngle)
   {
      F32 tickStep = gDegsPerTick * (mPrevAngle < mCurrAngle ? 1 : -1);
      F32 interpRot = mPrevAngle + tickStep * (1-dt);
      if (tickStep < 0)
      {
         if (interpRot < mCurrAngle)
            mPrevAngle = interpRot = mCurrAngle;
      }
      else
      {
         if (interpRot > mCurrAngle)
            mPrevAngle = interpRot = mCurrAngle;
      }

      MatrixF xRot, zRot;
      xRot.set(EulerF(interpRot, 0, 0));
      zRot.set(EulerF(0, 0, mPrevYawAngle));
      MatrixF temp;
      temp.mul(zRot, xRot);
      setTransform(temp);

      toUpdate = true;
   }

   if (mPrevYawAngle != mCurrYawAngle)
   {
      F32 tickStep = gDegsPerTick * (mPrevYawAngle < mCurrYawAngle ? 1 : -1);
      F32 interpRot = mPrevYawAngle + tickStep * (1-dt);
      if (tickStep < 0)
      {
         if (interpRot < mCurrYawAngle)
            mPrevYawAngle = interpRot = mCurrYawAngle;
      }
      else
      {
         if (interpRot > mCurrYawAngle)
            mPrevYawAngle = interpRot = mCurrYawAngle;
      }

      MatrixF xRot, zRot;
      xRot.set(EulerF(mPrevAngle, 0, 0));
      zRot.set(EulerF(0, 0, interpRot));
      MatrixF temp;
      temp.mul(zRot, xRot);
      setTransform(temp);

      toUpdate = true;
   }

   if (toUpdate)
   {
      pos.z = getTerrHeight(Point2F(pos.x, pos.y)) + interpHeight;
      setPosition(pos);
   }

}

// returns the height of the terrain
F32 RTSCamera::getTerrHeight(Point2F lamePos)
{
   Point3F pos(lamePos.x, lamePos.y, 0);

   if(mTerrain)
   {
      Point3F terrPos = pos;
      mTerrain->getWorldTransform().mulP(terrPos);
      terrPos.convolveInverse(mTerrain->getScale());

      F32 height;
      bool res = mTerrain->getHeight(Point2F(terrPos.x, terrPos.y), &height);
      if(res)
      {
         terrPos.z = height;
         terrPos.convolve(mTerrain->getScale());
         mTerrain->getTransform().mulP(terrPos);
      }

      return height;
   }
   else
      return 0;
}

void RTSCamera::onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query)
{
   PROFILE_START(RTS_CAMERA_SCOPE_QUERY);

   AssertFatal(dynamic_cast<RTSConnection*>(cr), "Not an RTS connection!");
   RTSConnection* rc = (RTSConnection*)cr;

   for (SimSetIterator itr(Sim::getRootGroup()); *itr; ++itr)
   {
      if (RTSUnit* unit1 = dynamic_cast<RTSUnit*>(*itr))
      {
         if (!unit1->isScopeable())
            continue;

         if (rc->isUnitVisible(unit1))
            cr->objectInScope(unit1);
      }
   }
   PROFILE_END();
}

//----------------------------------------------------------------------------
// RTSCamera Networking
//----------------------------------------------------------------------------
void RTSCamera::writePacketData(GameConnection *connection, BitStream *bstream)
{
/*   // Update client regardless of status flags.
   Parent::writePacketData(connection, bstream);

   Point3F pos = getPosition();
   bstream->setCompressionPoint(pos);

   mathWrite(*bstream, pos); */
}

void RTSCamera::readPacketData(GameConnection *connection, BitStream *bstream)
{
/*   Parent::readPacketData(connection, bstream);

   Point3F pos, rot;
   mathRead(*bstream, &pos);

   bstream->setCompressionPoint(pos);

   setPosition(pos); */
}

U32 RTSCamera::packUpdate(NetConnection *con, U32 mask, BitStream *bstream)
{
   // we dont do anything here, players dont need to know
   // where other players are looking
   return GameBase::packUpdate(con, mask, bstream);
}

void RTSCamera::unpackUpdate(NetConnection *con, BitStream *bstream)
{
   // we dont do anything here, players dont need to know
   // where other players are looking
   GameBase::unpackUpdate(con, bstream);
}

IMPLEMENT_CO_NETEVENT_V1(RTSCameraUpdate);

RTSCameraUpdate::RTSCameraUpdate()
{
   mCam = NULL;
}

RTSCameraUpdate::RTSCameraUpdate(RTSCamera *camera, bool onServer)
{
   mCam      = camera;
   mPos      = camera->getTransform();
   mGhostID  = -1;
   mOnServer = onServer;

   mIsOrbitDist  = false;
   mIsPitchAngle = false;
   mIsYawAngle   = false;
}

void RTSCameraUpdate::setOrbitDistance(F32 dist)
{
   mIsOrbitDist = true;
   mOrbitDist = dist;
}

void RTSCameraUpdate::setPitchAngle(F32 angle)
{
   mIsPitchAngle = true;
   mPitchAngle = angle;
}

void RTSCameraUpdate::setYawAngle(F32 angle)
{
   mIsYawAngle = true;
   mYawAngle = angle;
}

void RTSCameraUpdate::pack(NetConnection *conn, BitStream * stream)
{
   AssertFatal(mCam, "RTSCameraUpdate::pack - someone deleted the RTSCamera out from underneath us!");

   // Write the ghost id
   if(mOnServer)
      mGhostID = conn->getGhostIndex(mCam);
   else
      mGhostID = mCam->getNetIndex();

//   AssertFatal(mGhostID > -1, "RTSCameraUpdate::pack - couldn't get ghost index for our RTSCamera.");
   if(stream->writeFlag(mGhostID > -1))
   {
      stream->writeRangedU32(mGhostID, 0, NetConnection::MaxGhostCount);

      // Write positional data
      stream->writeAffineTransform(mPos);
      stream->writeFlag(mOnServer);

      if(stream->writeFlag(mIsOrbitDist))
         stream->write(mOrbitDist);

      if(stream->writeFlag(mIsPitchAngle))
         stream->write(mPitchAngle);

      if(stream->writeFlag(mIsYawAngle))
         stream->write(mYawAngle);
   }
}

void RTSCameraUpdate::write(NetConnection *conn, BitStream * stream)
{
   pack(conn, stream);
}

void RTSCameraUpdate::unpack(NetConnection *conn, BitStream *stream)
{
   // Read the camera we're talking about...
   if(stream->readFlag())
   {
       mGhostID = stream->readRangedU32(0, NetConnection::MaxGhostCount);

      // Read the positional data
      stream->readAffineTransform(&mPos);
      mOnServer = stream->readFlag();

      if(mIsOrbitDist = stream->readFlag())
         stream->read(&mOrbitDist);

      if(mIsPitchAngle = stream->readFlag())
         stream->read(&mPitchAngle);

      if(mIsYawAngle = stream->readFlag())
         stream->read(&mYawAngle);
   }
}

void RTSCameraUpdate::process(NetConnection * conn)
{
   // Resolve the ghost id and set the positional data
   if(mGhostID > -1)
   {
      if(!mOnServer)
         mCam = dynamic_cast<RTSCamera*>(conn->resolveObjectFromGhostIndex(mGhostID));
      else
         mCam = dynamic_cast<RTSCamera*>(conn->resolveGhost(mGhostID));

      if(mCam)
      {
         mCam->setTransform(mPos);

         if(mIsOrbitDist)
            mCam->setOrbitDistance(mOrbitDist, false);

         if(mIsPitchAngle)
            mCam->setPitchAngle(mPitchAngle, false);
         
         if(mIsYawAngle)
            mCam->setYawAngle(mYawAngle, false);
      }
      else
         Con::errorf("Received a RTSCameraUpdate for a RTSCamera that isn't ghosted!");
   }
}

//-----------------------------------------------------------------------------

ConsoleMethod(RTSCamera, getYawAngle, F32, 2, 2, "RTSCamera.getYawAngle()")
{
   return object->getYawAngle();
}

ConsoleMethod(RTSCamera, increaseYawAngle, void, 2, 2, "RTSCamera.increaseYawAngle()")
{
   object->increaseYawAngle();
}

ConsoleMethod(RTSCamera, decreaseYawAngle, void, 2, 2, "RTSCamera.decreaseYawAngle()")
{
   object->decreaseYawAngle();
}

ConsoleMethod(RTSCamera, setYawAngle, void, 3, 3, "RTSCamera.setYawAngle(degrees)" )
{
   object->setYawAngle( dAtof( argv[2] ), true );
}

ConsoleMethod(RTSCamera, getPitchAngle, F32, 2, 2, "RTSCamera.getPitchAngle()")
{
   return object->getPitchAngle();
}

ConsoleMethod(RTSCamera, increasePitchAngle, void, 2, 2, "RTSCamera.increasePitchAngle()")
{
   object->increasePitchAngle();
}

ConsoleMethod(RTSCamera, decreasePitchAngle, void, 2, 2, "RTSCamera.decreasePitchAngle()")
{
   object->decreasePitchAngle();
}

ConsoleMethod(RTSCamera, setPitchAngle, void, 3, 3, "RTSCamera.setPitchAngle(degrees)" )
{
   object->setPitchAngle( dAtof( argv[2] ), true );
}

ConsoleMethod(RTSCamera, setCameraPosition, void, 4, 4, "RTSCamera.setCameraPosition(posx, posy)")
{
   object->setCameraPosition( Point2F(dAtoi(argv[2]), dAtoi(argv[3])) );
}

ConsoleMethod(RTSCamera, getCameraPosition, const char *, 2, 2, "RTSCamera.getCameraPosition()" )
{
   char *retbuffer = Con::getReturnBuffer(256);
   Point3F pos = object->getRenderPosition();
   dSprintf( retbuffer, 256, "%f %f", pos.x, pos.y );

   return retbuffer;
}

ConsoleMethod(RTSCamera, isInCameraView, bool, 3, 3, "RTSCamera.isInCameraView(%location)")
{
   Point3F pos;
   dSscanf(argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z);

   RectF rect = object->getViewableRect();

   return (pos.x >= rect.point.x && pos.x < rect.point.x + rect.extent.x) &&
          (pos.y >= rect.point.y && pos.y < rect.point.y + rect.extent.y);
}

ConsoleMethod(RTSCamera, getOrbitDistance, F32, 2, 2, "RTSCamera.getOrbitDistance()")
{
   return object->getOrbitDistance();
}

ConsoleMethod(RTSCamera, increaseOrbitDistance, void, 2, 2, "RTSCamera.increaseOrbitDistance()")
{
   object->increaseOrbitDistance();
}

ConsoleMethod(RTSCamera, decreaseOrbitDistance, void, 2, 2, "RTSCamera.decreaseOrbitDistance()")
{
   object->decreaseOrbitDistance();
}

ConsoleMethod(RTSCamera, setOrbitDistance, void, 3, 3, "RTSCamera.setOrbitDistance(height)")
{
   object->setOrbitDistance(dAtof(argv[2]), true);
}
