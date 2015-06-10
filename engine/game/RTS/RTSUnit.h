//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RTSUnit_H_
#define _RTSUnit_H_

#ifndef _PLAYER_H_
#include "game/player.h"
#endif
#include "sim/netConnection.h"
#include "core/bitStream.h"

class RTSConnection;
class RTSUnitModifier : public SimObject
{
   typedef SimObject Parent;
public:
   F32 mDamage;
   F32 mAttackDelay;
   F32 mArmor;
   F32 mMoveSpeed;
   F32 mRange;
   F32 mVision;

   RTSUnitModifier()
   {
      mDamage = 1.0;
      mAttackDelay = 1.0;
      mArmor = 1.0;
      mMoveSpeed = 1.0;
      mRange = 1.0;
      mVision = 1.0;
   }

   static void initPersistFields();

   RTSUnitModifier& operator+=(RTSUnitModifier& mod)
   {
      mDamage      += mod.mDamage;
      mAttackDelay += mod.mAttackDelay;
      mArmor       += mod.mArmor;
      mMoveSpeed   += mod.mMoveSpeed;
      mRange       += mod.mRange;
      mVision      += mod.mVision;

      return *this;
   }
   RTSUnitModifier& operator-=(RTSUnitModifier& mod)
   {
      mDamage      -= mod.mDamage;
      mAttackDelay -= mod.mAttackDelay;
      mArmor       -= mod.mArmor;
      mMoveSpeed   -= mod.mMoveSpeed;
      mRange       -= mod.mRange;
      mVision      -= mod.mVision;

      return *this;
   }
   DECLARE_CONOBJECT(RTSUnitModifier);
};

class RTSUnitModifierData : public SimDataBlock
{
   typedef SimDataBlock Parent;

public:

   RTSUnitModifier mModifier;

   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
   static void initPersistFields();

   DECLARE_CONOBJECT(RTSUnitModifierData);
};

class RTSUnitData : public PlayerData
{
   typedef PlayerData Parent;

public:
   S32 mBaseDamage;
   S32 mAttackDelay;
   S32 mDamagePlus;

   S32 mArmor;

   F32 mMoveSpeed;
   F32 mRange;
   F32 mVision;

   bool mDoWaterInteraction;
   bool mDoShadow;
   bool mDoLookAnimation;

   RTSUnitData();

   static void initPersistFields();

   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);

   DECLARE_CONOBJECT(RTSUnitData);
};

// RTSUnit provides a lightweight version of the player for use in real
// time simulations. It doesn't do any physics. It does replicate the existing
// animation functionality.

class RTSUnit : public Player
{
   typedef Player Parent;

public:
   // Unit move states
   enum MoveState
   {
      ModeStop,
      ModeMove,
      ModeStuck,
   };

   // Network states
   enum MaskBits
   {
      ActionMask      = ShapeBase::NextFreeMask << 0,
      MoveMask        = ShapeBase::NextFreeMask << 1,
      AIMask          = ShapeBase::NextFreeMask << 2,
      ModifierMask    = ShapeBase::NextFreeMask << 3,

      // We're all crazy!
      ProjectileMask  = ShapeBase::NextFreeMask << 4,
      NextFreeMask    = ShapeBase::NextFreeMask << 5,
   };

   F32 getUpdatePriority(CameraScopeQuery *focusObject, U32 updateMask, S32 updateSkips);

   // Modifier information...
   RTSUnitModifier mNetModifier;
   SimSet mModifierList;

   // What's the active projectile?
   ProjectileData *mCurrentProjectile;

   // Visibility management
   bool mPositionDirty;
   RTSConnection* mControllingConnection;

private:
   // Unit movement
   MoveState mMoveState;
   F32       mMoveVelocity;
   F32       mMoveTolerance;     // Distance from destination before we stop
   Point3F   mMoveDestination;   // Destination for movement
   Point3F   mLastLocation;      // For stuck check

   // Unit targeting and aiming
   bool                   mAimObjectSet;     // Has an aim object set?
   SimObjectPtr<GameBase> mAimObject;        // Object to point at, overrides location
   bool                   mAimLocationSet;   // Has an aim location been set?
   Point3F                mAimLocation;      // Point to look at
   bool                   mTargetInLOS;      // Is target object visible?

   S32 mTeam;
   S32 mAttackDelay;

public:
   RTSUnit();
   ~RTSUnit();

   bool onAdd();
   void onRemove();


   // Unit movement
   void setMoveSpeed( const F32 speed );
   void setMoveTolerance( const F32 tolerance );
   void setMoveDestination( const Point3F &location );
   void stopMove();

   F32     getMoveTolerance() const { return mMoveTolerance; }
   Point3F getMoveDestination() const { return mMoveDestination; }

   // Unit targeting and aiming
   void setAimObject( GameBase *targetObject );
   GameBase* getAimObject() const  { return mAimObject; }
   void setAimLocation( const Point3F &location );
   Point3F getAimLocation() const { return mAimLocation; }
   void clearAim();

   //modifier support
   void addModifier(RTSUnitModifierData *mod);
   void removeModifier(RTSUnitModifierData *mod);

   // visibility manager interface
   bool isDirty() const { return mPositionDirty; }
   void setClean() { mPositionDirty = false; }
   void setDirty();
   void setControllingConnection(RTSConnection* conn);
   RTSConnection* getControllingConnection() const { return mControllingConnection; }
   void clearControllingConnection();

   // Team interface
   void setTeam(S32 team);
   S32 getTeam() const { return mTeam; }

   // NetObject
   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);

   // Simulation update
   void interpolateTick(F32 dt);
   void processTick(const Move *move);
   void advanceTime(F32 dt);

   void updateActionThread();

   // SceneObject
   bool prepRenderImage(SceneState* state, const U32 stateKey,
                                const U32 startZone, const bool modifyBaseState);
   void renderImage(SceneState* state, SceneRenderImage* image);

   DECLARE_CONOBJECT( RTSUnit );
};


//--------------------------------------------------------------------------


#endif
