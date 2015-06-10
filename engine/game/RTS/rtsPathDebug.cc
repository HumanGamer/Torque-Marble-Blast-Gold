//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/gameBase.h"
#include "math/mBox.h"
#include "collision/earlyOutPolyList.h"
#include "sceneGraph/sceneState.h"
#include "dgl/dgl.h"
#include "dgl/gTexManager.h"
#include "console/consoleTypes.h"
#include "collision/boxConvex.h"
#include "core/bitStream.h"
#include "math/mathIO.h"
#include "sceneGraph/sceneGraph.h"

class RTSPathDebug : public SceneObject
{
   typedef SceneObject Parent;

   struct RTSPathDebugNode
   {
      RTSPathDebugNode(Point3F p, ColorF c) : mPos(p), mColor(c) { };

      Point3F  mPos;
      ColorF   mColor;
   };

   Vector<RTSPathDebugNode>   mNodes;

protected:
   bool onAdd();
   void onRemove();

   // Rendering
protected:
   bool prepRenderImage  ( SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState=false);
   void renderObject     ( SceneState *state, SceneRenderImage *image);

public:
   RTSPathDebug()
   {
   }

   ~RTSPathDebug()
   {
   }

   void addNode(Point3F pos, ColorF color);
   void clearPath();

   DECLARE_CONOBJECT(RTSPathDebug);
   static void initPersistFields() { };
};

IMPLEMENT_CONOBJECT(RTSPathDebug);

bool RTSPathDebug::onAdd()
{
   if(!Parent::onAdd())
      return false;

   // Add us to the client scenegraph!
   gClientSceneGraph->addObjectToScene(this);

   // Was it that easy? WAS IT?!??!?!!?!?
   return true;
}

void RTSPathDebug::onRemove()
{
   Parent::onRemove();

   // Clean up!
   gClientSceneGraph->removeObjectFromScene(this);
}

bool RTSPathDebug::prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState/* =false */)
{
   // Set up some sort of CRAZY RENDER STATE SHIZZLE!!!!

   if (isLastState(state, stateKey))
      return false;

   setLastState(state, stateKey);

   // Just always render. Just... Just do it.
   SceneRenderImage* image = new SceneRenderImage;
   image->obj = this;
   state->insertRenderImage(image);

   return true;
}

void RTSPathDebug::renderObject(SceneState *state, SceneRenderImage *image)
{
   glBegin(GL_LINE);

   // DRAW US A PATH OMG!!!!!!!!
   for(S32 i=0; i<mNodes.size(); i++)
   {
      glColor3fv(mNodes[i].mColor);
      glVertex3fv(mNodes[i].mPos);
   }

   glEnd();
}

void RTSPathDebug::addNode(Point3F pos, ColorF color)
{
   mNodes.push_back(RTSPathDebugNode(pos, color));
}

void RTSPathDebug::clearPath()
{
   mNodes.clear();
}

ConsoleMethod(RTSPathDebug, addNode, void, 4, 4, "(Point3F pos, ColorF color) - Add a node to the path.")
{
   // PARSE SHIZZLE
   ColorF  thaC;
   Point3F thaFreakingP;

   dSscanf(argv[2], "%f %f %f", &thaC.red, &thaC.green, &thaC.blue);
   dSscanf(argv[3], "%f %f %f", &thaFreakingP.x, &thaFreakingP.y, &thaFreakingP.z);

   // ADD SHIZZLE
   object->addNode(thaFreakingP, thaC);
}

ConsoleMethod(RTSPathDebug, clearPath, void, 3, 3, "Clear the path.")
{
   object->clearPath();
}