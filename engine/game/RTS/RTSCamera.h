#ifndef _RTSCAMERA_H_
#define _RTSCAMERA_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#include "game/gameConnection.h"

//----------------------------------------------------------------------------
struct RTSCameraData: public ShapeBaseData
{
   typedef ShapeBaseData Parent;

public:
   RTSCameraData();

   /// Pan speed of the camera. How quickly does it translate in the world?
   F32 mMovementSpeed;

   /// Starting pitch of the camera.
   F32 mPitchAngle;

   /// Maximum height of the camera.
   F32 mMaxOrbitHeight;

   /// Minimum height of the camera.
   F32 mMinOrbitHeight;

   /// Size of steps for height/zoom.
   F32 mOrbitStep;

   /// Size of steps in angular changes, in degrees.
   F32 mAngleStep;

   /// Minimum view angle, in degrees.
   F32 mMinAngle;

   /// Maximum view angle in degrees.
   F32 mMaxAngle;

   void packData(BitStream* stream);
   void unpackData(BitStream* stream);

   //
   static void initPersistFields();
   DECLARE_CONOBJECT(RTSCameraData);
};

//----------------------------------------------------------------------------

class RTSCameraUpdate;

/// Implements a basic camera object.
class RTSCamera: public ShapeBase
{
   friend class RTSCameraUpdate;
   typedef ShapeBase Parent;
   RTSCameraData* mDataBlock;

   // camera height
   F32 mRenderHeight;
   F32 mPrevHeight;
   F32 mCurrHeight;
   F32 mTargetHeight;

   Point2F mTargetPos;
   Point2F mPrevPos;

   // Deal with pitch
   F32 mTargetAngle;
   F32 mCurrAngle;
   F32 mPrevAngle;

   // Deal with yaw
   F32 mTargetYawAngle;
   F32 mCurrYawAngle;
   F32 mPrevYawAngle;

   // terrain block
   TerrainBlock *mTerrain;

public:
   DECLARE_CONOBJECT(RTSCamera);

   RTSCamera();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr);

   void processTick(const Move* move);
   void interpolateTick(F32 delta);

   void setCameraPosition(Point2F pos);

   F32     getCurrHeight() { return mCurrHeight; }
   RectF   getViewableRect();
   Point3F getLookPosition();
   void getCameraTransform(F32* pos,MatrixF* mat);
   F32 getTerrHeight(Point2F lamePos);

   F32  getPitchAngle() { return mRadToDeg(mTargetAngle); }
   void setPitchAngle(F32 angle, bool updateNet, bool toInterpolate = true);
   void increasePitchAngle() { setPitchAngle(getPitchAngle() + mDataBlock->mAngleStep, true); }
   void decreasePitchAngle() { setPitchAngle(getPitchAngle() - mDataBlock->mAngleStep, true); }

   F32  getOrbitDistance() { return mTargetHeight; }
   void setOrbitDistance(F32 dist, bool updateNet);
   void increaseOrbitDistance() { setOrbitDistance(getOrbitDistance() + mDataBlock->mOrbitStep, true); }
   void decreaseOrbitDistance() { setOrbitDistance(getOrbitDistance() - mDataBlock->mOrbitStep, true); }

   F32  getYawAngle() { return mRadToDeg(mTargetYawAngle); };
   void setYawAngle(F32 angle, bool updateNet, bool toInterpolate = true);
   void increaseYawAngle() { setYawAngle(getYawAngle() + mDataBlock->mAngleStep, true); }
   void decreaseYawAngle() { setYawAngle(getYawAngle() - mDataBlock->mAngleStep, true); }

   // RTSCamera Networking
   void writePacketData(GameConnection *conn, BitStream *stream);
   void readPacketData(GameConnection *conn, BitStream *stream);
   U32  packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn, BitStream *stream);
   void onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query);

};

class RTSCameraUpdate : public NetEvent
{
   typedef NetEvent Parent;

public:

   SimObjectPtr<RTSCamera> mCam;
   S32                     mGhostID;
   MatrixF                 mPos;
   bool                    mOnServer;

   bool mIsOrbitDist;
   F32  mOrbitDist;

   bool mIsPitchAngle;
   F32  mPitchAngle;

   bool mIsYawAngle;
   F32  mYawAngle;

   RTSCameraUpdate();
   RTSCameraUpdate(RTSCamera *camera, bool onServer);

   void setOrbitDistance(F32 dist);
   void setPitchAngle(F32 angle);
   void setYawAngle(F32 angle);

   void pack   (NetConnection *conn, BitStream *stream);
   void write  (NetConnection *conn, BitStream *stream);
   void unpack (NetConnection *conn, BitStream *stream);
   void process(NetConnection *conn);

   DECLARE_CONOBJECT(RTSCameraUpdate);
};

#endif
