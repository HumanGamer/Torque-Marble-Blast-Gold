//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RTSPROJECTILE_H_
#define _RTSPROJECTILE_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _PLATFORMAUDIO_H_
#include "platform/platformAudio.h" 
#endif

#include "game/projectile.h"
#include "game/fx/particleEmitter.h"

class ExplosionData;
class ShapeBase;
class TSShapeInstance;
class TSThread;

//--------------------------------------------------------------------------
/// Base class for RTSProjectiles.
class RTSProjectile : public Projectile
{
   typedef GameBase Parent;

   Point3F mTargetPos;
   F32 mSpeed;

protected:

   bool onAdd();

   void processTick(const Move *move);

   /// What to do once this projectile collides with something
   virtual void onCollision(const Point3F& p, const Point3F& n, SceneObject*);

   /// What to do when this projectile explodes
   virtual void explode(const Point3F& p, const Point3F& n, const U32 collideType );

   // Rendering
   void prepModelView    ( SceneState *state);
 
   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream)  { return 0; };
   void unpackUpdate(NetConnection *conn,           BitStream *stream) { return; };

public:
   RTSProjectile();
   ~RTSProjectile();

   DECLARE_CONOBJECT(RTSProjectile);
   static void initPersistFields();

   virtual bool calculateImpact(float    simTime,
                                Point3F& pointOfImpact,
                                float&   impactTime);

};

#endif // _H_RTSPROJECTILE

