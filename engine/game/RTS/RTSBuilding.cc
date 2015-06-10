#include "game/RTS/RTSBuilding.h"
#include "game/shapeBase.h"
#include "dgl/dgl.h"
#include "ts/tsShapeInstance.h"

IMPLEMENT_CO_NETOBJECT_V1(RTSBuilding);
IMPLEMENT_CONOBJECT(RTSBuildingMarker);

bool RTSBuildingMarker::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   // Set this to display the idle animation, note we never update the thread
   // so it's just sitting on the same frame, but whatever
   if( isGhost() )
   {
      mThread = mShapeInstance->addThread();
      S32 seq = mShapeInstance->getShape()->findSequence( "idle" );

      if(!mThread || seq == -1)
      {
         Con::errorf("RTSBuildingMarker::onAdd - Unable to locate sequence 'idle'");
         return false;
      }

      mShapeInstance->setSequence( mThread, seq, 0.f );
   }

   return true;
}

void RTSBuildingMarker::onRemove()
{
   mShapeInstance->destroyThread( mThread );

   Parent::onRemove();
}

void RTSBuildingMarker::renderObject(SceneState* state, SceneRenderImage* image)
{
   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

   RectI viewport;
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   dglGetViewport(&viewport);

   installLights();

   // Uncomment this if this is a "simple" (non-zone managing) object
   state->setupObjectProjection(this);

   // This is something of a hack, but since the 3space objects don't have a
   //  clear conception of texels/meter like the interiors do, we're sorta
   //  stuck.  I can't even claim this is anything more scientific than eyeball
   //  work.  DMM
   F32 axis = (getObjBox().len_x() + getObjBox().len_y() + getObjBox().len_z()) / 3.0;
   F32 dist = (getRenderWorldBox().getClosestPoint(state->getCameraPosition()) - state->getCameraPosition()).len();
   if (dist != 0)
   {
      F32 projected = dglProjectRadius(dist, axis) / 350;
      if (projected < (1.0 / 16.0))
      {
         TextureManager::setSmallTexturesActive(true);
      }
   }
   
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   dglMultMatrix(&mObjToWorld);
   glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

   // RENDER CODE HERE
   mShapeInstance->setEnvironmentMap(state->getEnvironmentMap());
   mShapeInstance->setEnvironmentMapOn(true,1);
   mShapeInstance->setAlphaAlways(1.0);

   Point3F cameraOffset;
   mObjToWorld.getColumn(3,&cameraOffset);
   cameraOffset -= state->getCameraPosition();
   dist = cameraOffset.len();
   F32 fogAmount = state->getHazeAndFog(dist,cameraOffset.z);

   // render shield effect
   if (mTransVal == 1.0f)
   {
      if (image->isTranslucent == true)
      {
         TSShapeInstance::smNoRenderNonTranslucent = true;
         TSShapeInstance::smNoRenderTranslucent    = false;
      }
      else
      {
         TSShapeInstance::smNoRenderNonTranslucent = false;
         TSShapeInstance::smNoRenderTranslucent    = true;
      }
   }
   else
   {
      TSShapeInstance::smNoRenderNonTranslucent = false;
      TSShapeInstance::smNoRenderTranslucent    = false;
   }

   // Make this model be transparent and textured.
   TSMesh::setOverrideFade( mTransVal );
   mShapeInstance->setOverrideTexture( mOverrideTex );

   mShapeInstance->setupFog(fogAmount,state->getFogColor());
   mShapeInstance->animate();
   mShapeInstance->render();

   // Clear the overrides...
   mShapeInstance->clearOverrideTexture();
   TSMesh::setOverrideFade(1.0f);

   renderShadow(dist,fogAmount);

   TSShapeInstance::smNoRenderNonTranslucent = false;
   TSShapeInstance::smNoRenderTranslucent    = false;
   TextureManager::setSmallTexturesActive(false);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   uninstallLights();
   dglSetCanonicalState();


   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

ConsoleMethod(RTSBuildingMarker, setOverrideTexture, void, 3, 3, "(string filename)")
{
   object->setOverrideTexture(StringTable->insert(argv[2]));
}

//-----------------------------------------------------------------------------

ConsoleMethod( RTSBuilding, setTeam, void, 3, 3, "( S32 teamID )"
               "Sets the units team.")
{
   object->setTeam(dAtoi(argv[2]));
}

ConsoleMethod( RTSBuilding, getTeam, S32, 2, 2, "()"
               "Returns the units teamID.")
{
   return object->getTeam();
}