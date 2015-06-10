#ifndef _RTSBUILDING_H_
#define _RTSBUILDING_H_

#include "game/staticShape.h"
#include "game/tsStatic.h"
#include "game/RTS/RTSUnit.h"

class RTSBuilding : public RTSUnit
{
   typedef RTSUnit Parent;
public:

   // don't want this doing anything
   DECLARE_CONOBJECT(RTSBuilding);
};

/// A building marker is what attaches to the cursor in order to place
/// a building.  Usually, the marker will be translucent to show that
/// the building isn't actually placed, but what the user is seeing is
/// just a preview of where the building will be placed.
///
/// All the class consists of is basically an object that is completely
/// cutoff from the network (client-side only) and a specialized render
/// function.  All the render function does is allow for easy transparency.
class RTSBuildingMarker : public TSStatic
{
   typedef TSStatic Parent;

   F32              mTransVal;
   StringTableEntry mOverrideFile;
   TextureHandle    mOverrideTex;
   TSThread *mThread;

public:

   RTSBuildingMarker()
   {
      mOverrideFile = StringTable->insert("");
      mTransVal = 1.0f;

      // Magic to make this a client-side only object.
      mNetFlags.clear(Ghostable);  //NOT ghostable - don't want this networked
      mNetFlags.set(IsGhost);      //however, it IS a client-side object
      mThread = NULL;
   }

   bool onAdd();
   void onRemove();

   void setTransVal(F32 val)
   {
      mTransVal = val;
   }

   void setOverrideTexture(StringTableEntry f)
   {
      mOverrideFile = f;
      mOverrideTex.set(mOverrideFile);
   }

   void renderObject(SceneState* state, SceneRenderImage* image);

   DECLARE_CONOBJECT(RTSBuildingMarker);
};

#endif
