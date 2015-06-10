//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "sim/sceneObject.h"
#include "platform/event.h"
#include "gui/guiCanvas.h"
#include "game/game.h"
#include "game/gameConnection.h"
#include "core/memstream.h"
#include "collision/clippedPolyList.h"
#include "game/shapeBase.h"
#include "console/consoleInternal.h"
#include "game/sphere.h"
#include "console/simBase.h"
#include "game/RTS/guiRTSTSCtrl.h"
#include "game/RTS/RTSUnit.h"
#include "ts/tsShapeInstance.h"
#include "game/RTS/RTSConnection.h"
#include "game/RTS/RTSCamera.h"
IMPLEMENT_CONOBJECT(GuiRTSTSCtrl);

//------------------------------------------------------------------------------

bool GuiRTSTSCtrl::collide(const Gui3DMouseEvent & event, CollisionInfo & info)
{
   // turn off the collision with the control object
   SceneObject * controlObj = NULL; //getControlObject(); // Get this somehow
   if(controlObj)
      controlObj->disableCollision();

   //
   Point3F startPnt = event.pos;
   Point3F endPnt   = event.pos + event.vec * 500.f; // bjgtodo - figure out better value for this

   //
   RayInfo ri;
   bool hit;
   hit = gClientContainer.castRay(startPnt, endPnt, TerrainObjectType | PlayerObjectType | StaticShapeObjectType, &ri);
   if(controlObj)
      controlObj->enableCollision();

   //
   if(hit)
   {
      RTSConnection* conn = RTSConnection::getServerConnection();
      RTSUnit* unit = dynamic_cast<RTSUnit*>(ri.object);
      if (!unit || conn->isUnitVisible(unit))
      {
         info.pos    = ri.point;
         info.obj    = ri.object;
         info.normal = ri.normal;
         //info.start  = event.pos;
         AssertFatal(info.obj, "GuiSquadTSCtrl::collide - client container returned non SceneObject");
         AssertFatal(info.obj->isClientObject(), "Non server object!");
      }
      else
         hit = false;
   }

   return(hit);
}

//------------------------------------------------------------------------------

GuiRTSTSCtrl::GuiRTSTSCtrl()
{
   mPrevEdge = EdgeNone;
   mRightMousePassThru = false;
   mRenderMissionArea = false;

   mDragRectColor.set(0,255,0);
   mDragSelect = false;

   mDamageFillColor.set( 0, 1, 0, 1 );
   mDamageFrameColor.set( 0, 0, 0, 1 );
   mDamageRectSize.set(50, 4);

   mHitInfo.obj = 0;
   mHitObject = mHitInfo.obj;

   mLockSelection = false;

   mPlacingBuilding = false;
   mNewBuilding = NULL;

   mSelectionIncludesTeam = false;
   mDragSelectionIncludesTeam = false;
}

GuiRTSTSCtrl::~GuiRTSTSCtrl()
{

}

//------------------------------------------------------------------------------
bool GuiRTSTSCtrl::onWake()
{
   if (!Parent::onWake())
      return false;

   Platform::setWindowLocked(true);
   return true;
}

void GuiRTSTSCtrl::onSleep()
{
   Parent::onSleep();
   Platform::setWindowLocked(false);
}

//------------------------------------------------------------------------------

void GuiRTSTSCtrl::on3DMouseMove(const Gui3DMouseEvent & event)
{
   // Update the mouse over information
   mHitInfo.obj = 0;

   CollisionInfo info;
   if(collide(event, info))
   {
      if(info.obj)
      {
         RTSUnit *unit;
         if(unit = dynamic_cast<RTSUnit*>(info.obj))
         {
            mHitInfo = info;
            Con::executef(this, 2, "onMouseOverUnit", info.obj->getIdString());
         }
         else
            Con::executef(this, 2, "onMouseOverTerrain");
      }
   }

   mHitObject = mHitInfo.obj;

   // See if we are on any of the edges or corners of the screen
   Point2I localPoint = globalToLocalCoord(event.mousePoint);
   U32 edgeFlags = EdgeNone;
   if(localPoint.x <= mBounds.point.x + 5)
      edgeFlags |= EdgeLeft;
   if(localPoint.y <= mBounds.point.y + 5)
      edgeFlags |= EdgeTop;
   if(localPoint.x >= mBounds.extent.x - 5)
      edgeFlags |= EdgeRight;
   if(localPoint.y >= mBounds.extent.y - 5)
      edgeFlags |= EdgeBottom;

   // If the mouse is on an edge, send a command to scroll the display.
   if(edgeFlags & ~(EdgeNone))
   {
      Con::executef(this, 3, "onMousePanDisplay", "1", Con::getIntArg(edgeFlags & ~(EdgeNone)));
      mPrevEdge |= edgeFlags;
   }
   else if(mPrevEdge & ~(EdgeNone))
   {
      Con::executef(this, 3, "onMousePanDisplay", "0", Con::getIntArg(mPrevEdge & ~(EdgeNone)));
      mPrevEdge = EdgeNone;
   }

   if (mPlacingBuilding)
   {
      // This may seem totally random, it is in fact a bad hack to push the building
      // up by 1.5 units, which is half of the z component of it's bounding box
      ((ShapeBase*)mNewBuilding)->setPosition(info.pos + Point3F( 0.f, 0.f, 1.5f ) );
   }
}

void GuiRTSTSCtrl::on3DMouseDown(const Gui3DMouseEvent & event)
{
   // Lock the mouse so drag selects will go through,
   // even if we release over other gui elements.
   mouseLock();

   // Check to see if the mouse collided with something.  If so send
   // onMouseDown event with the object and coordinates if there are any.
   // That way script will have the information it needs to deal with any
   // situation.
   CollisionInfo info;
   if(collide(event, info))
      if(info.obj)
         mHitInfo = info;

   Con::executef(this,
      6,
      "onMouseDown",
      Con::getIntArg((mHitInfo.obj) ? mHitInfo.obj->getId() : -1),
      Con::getIntArg(info.pos.x),
      Con::getIntArg(info.pos.y),
      Con::getIntArg(info.pos.z));

   // Mouse is down, so assume that it is the start of a drag select.
   mDragSelect = false;
   mDragSelected.clear();
   mDragRect.set(Point2I(event.mousePoint), Point2I(0,0));
   mDragStart = event.mousePoint;

   // Update the hitObject so we know what the mouse is over.
   mHitObject = mHitInfo.obj;

   // If we are placing buildings, this is where it happens.
   if (mPlacingBuilding)
      placeBuilding();
}

void GuiRTSTSCtrl::on3DMouseUp(const Gui3DMouseEvent & event)
{
   // Mouse was locked to ensure proper drag select functionality.
   // Since mouse up ends a drag select we should now unlock the mouse.
   mouseUnlock();

   if (mHitInfo.obj)
      mHitInfo.obj->inspectPostApply();
   mHitInfo.obj = 0;


   // Check to see if the mouse collided with something.  If so send
   // onMouseUp event with the object and coordinates if there are any.
   // That way script will have the information it needs to deal with any
   // situation.
   CollisionInfo info;
   if(collide(event, info))
      if(info.obj)
         mHitInfo = info;

   Con::executef(this,
      7,
      "OnMouseUp",
      Con::getIntArg((mDragSelect) ? 1 : 0),
      Con::getIntArg((mHitInfo.obj) ? mHitInfo.obj->getId() : -1),
      Con::getIntArg(info.pos.x),
      Con::getIntArg(info.pos.y),
      Con::getIntArg(info.pos.z));

   // Mouse up, so our drag select is over now.
   mDragSelect = false;

   // Update the mouse over object.
   mHitObject = mHitInfo.obj;
}

#define DRAG_PIXEL_TOLERANCE 4

void GuiRTSTSCtrl::on3DMouseDragged(const Gui3DMouseEvent & event)
{
   if( !mDragSelect && ( mDragStart.x + DRAG_PIXEL_TOLERANCE < event.mousePoint.x ||
                         mDragStart.y + DRAG_PIXEL_TOLERANCE < event.mousePoint.y ||
                         mDragStart.x < event.mousePoint.x + DRAG_PIXEL_TOLERANCE ||
                         mDragStart.y < event.mousePoint.y + DRAG_PIXEL_TOLERANCE ) )
   {
      // The mouse has moved more than DRAG_PIXEL_TOLERANCE pixels since mouse down
      mDragSelect = true;
   }

   // based on drag selection scheme in the world editor
   //   see engine/editor/worldEditor.cc line 1946
   // Are we drag selecting?
   if(mDragSelect)
   {
      // Check to see if selection modifier is being pressed, if so clear selection.
      Con::executef(this, 1, "checkDragSelectionModifier");

      // Drag selection is built in onRender, make sure here that there are no
      // negative extents.
      mDragRect.point.x  = (event.mousePoint.x < mDragStart.x) ? event.mousePoint.x : mDragStart.x;
      mDragRect.extent.x = (event.mousePoint.x > mDragStart.x) ? event.mousePoint.x - mDragStart.x : mDragStart.x - event.mousePoint.x;
      mDragRect.point.y  = (event.mousePoint.y < mDragStart.y) ? event.mousePoint.y : mDragStart.y;
      mDragRect.extent.y = (event.mousePoint.y > mDragStart.y) ? event.mousePoint.y - mDragStart.y : mDragStart.y - event.mousePoint.y;
      return;
   }
}

void GuiRTSTSCtrl::on3DMouseEnter(const Gui3DMouseEvent &)
{
}

void GuiRTSTSCtrl::on3DMouseLeave(const Gui3DMouseEvent &)
{
}

void GuiRTSTSCtrl::on3DRightMouseDown(const Gui3DMouseEvent & event)
{
   // Check to see if the mouse collided with something.  If so send
   // onRightMouseDown event with the object and coordinates if there are any.
   // That way script will have the information it needs to deal with any
   // situation.
   CollisionInfo info;
   if(collide(event, info))
      if(info.obj)
         mHitInfo = info;

   Con::executef(this,
      6,
      "onRightMouseDown",
      Con::getIntArg((mHitInfo.obj) ? mHitInfo.obj->getId() : -1),
      Con::getIntArg(info.pos.x),
      Con::getIntArg(info.pos.y),
      Con::getIntArg(info.pos.z));

   // Right mouse down, so cancel active drag select.
   mDragSelect = false;

   // Update the mouse over object.
   mHitObject = mHitInfo.obj;
}

void GuiRTSTSCtrl::on3DRightMouseUp(const Gui3DMouseEvent & event)
{
}

//------------------------------------------------------------------------------

void GuiRTSTSCtrl::updateGuiInfo()
{
}

//------------------------------------------------------------------------------
static void findObjectsCallback(SceneObject* obj, void * val)
{
   // Queries the current scene to return a list of objects.
   // This is used to update the drag selection.
   Vector<SceneObject*> * list = (Vector<SceneObject*>*)val;
   list->push_back(obj);
}

void GuiRTSTSCtrl::renderScene(const RectI & updateRect)
{
   // Show damage/whiteout
   CameraQuery camQ;
   if(GameProcessCameraQuery(&camQ))
      GameRenderFilters(camQ);
}

void GuiRTSTSCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   Parent::onRender(offset, updateRect);

   RTSConnection* conn = dynamic_cast<RTSConnection*>(NetConnection::getServerConnection());

   if(!conn)
      return;

   // setup GL for selection info rendering
   dglSetClipRect(updateRect);
   glDisable     (GL_CULL_FACE);
   glEnable      (GL_BLEND);
   glBlendFunc   (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glLineWidth(4.f);

   // highlight selected objects
   for(S32 i=0; i<getSelectionSize(); i++)
   {
      if(mDragSelected.objInSet(mSelected[i]) || (const SceneObject *)mHitObject == mSelected[i])
         continue;

      drawDamage(mSelected[i]);
   }

   if(bool(mHitObject))
   {
      drawDamage(mHitObject);
   }

   // change gl states back
   glLineWidth(1.f);
   glDisable( GL_BLEND );
   glEnable(GL_CULL_FACE);

   // drag selection update based on worldEditor
   //   defined in engine/editor/worldEditor line 2379
   if(mDragSelect)
   {
      // Draw drag selection box
      dglDrawRect(mDragRect, mDragRectColor);

      Con::executef(this, 1, "dragClearSelection");

      Vector<SceneObject *> objects;
      gClientContainer.findObjects(0xFFFFFFFF, findObjectsCallback, &objects);
      for(S32 i = 0; i < objects.size(); i++)
      {
         SceneObject * obj = objects[i];

         // -- PROBLEM IS RIGHT HERE -pw

         Box3F box = obj->getObjBox();
         RTSConnection* conn = (RTSConnection*)NetConnection::getServerConnection();
         RTSCamera* camera = dynamic_cast<RTSCamera*>( conn->getControlObject() );
         F32 dist = camera->getCurrHeight();
         F32 radius = getMax( box.max.x - box.min.x, box.max.y - box.min.y );
         F32 renderRadius = dglProjectRadius( dist, radius ) / 3.f;
         RectI modifiedRect( mDragRect.point.x - renderRadius, mDragRect.point.y - renderRadius,
                             mDragRect.extent.x + renderRadius * 2, mDragRect.extent.y + renderRadius * 2 );

         //dglDrawRect( modifiedRect, ColorI( 255, 0, 0 ) );

         Point3F wPos;
         obj->getTransform().getColumn(3, &wPos);

         dglProjectRadius( dist, radius );

         Point3F sPos;
         if(project(wPos, &sPos))
            if(mDragSelect && modifiedRect.pointInRect(Point2I((S32)sPos.x, (S32)sPos.y)))
                  Con::executef(this,
                     2,
                     "updateDragSelect",
                     obj->getIdString());
      }

      // Clean up from rubber band box.
      glDisable(GL_BLEND);
      glEnable(GL_TEXTURE_2D);
   }


   renderChildControls(offset, updateRect);
}

void GuiRTSTSCtrl::drawDamage(SceneObject *obj)
{
   Point3F res;
   const SphereF wSph = obj->getWorldSphere();
   Point3F objPos = obj->getPosition();
   objPos.z += obj->getObjBox().max.z - obj->getObjBox().min.z;

   if(project(objPos, &res))
   {
      // Get distance to object from camera
      Point3F camPos, objPos;

      objPos = wSph.center;
      unproject( Point3F( 0, 0, 0), &camPos);

      F32 dist   = (camPos - objPos).len();
      F32 width = dglProjectRadius(dist, wSph.radius) + 5;
      Point2I offset(res.x - width / 2,(S32)(res.y - (2*dglProjectRadius(dist, wSph.radius))));

      // Damage should be 0->1 (0 being no damage,or healthy), but
      // we'll just make sure here as we flip it.
      if(ShapeBase *object = dynamic_cast<ShapeBase*>(obj))
      {
         //don't draw if dead
         if (object->getDamageState() != Player::Enabled)
            return;

         F32 damage = mClampF(1 - object->getDamageValue(), 0, 1);

         // Center the bar
         RectI rect(offset, Point2I(width, mDamageRectSize.y));
         //rect.point.x -= mDamageRectSize.x / 2;

         // Draw the border
         dglDrawRect(rect, mDamageFrameColor);

         // Draw the damage % fill
         rect.point += Point2I(1, 1);
         rect.extent -= Point2I(1, 1);
         rect.extent.x = (S32)(rect.extent.x * damage);
         if (rect.extent.x == 1)
            rect.extent.x = 2;
         if (rect.extent.x > 0)
            dglDrawRectFill(rect, mDamageFillColor);
      }
   }
}

void GuiRTSTSCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("SelectionInfo");
   addField("selectionLocked", TypeBool, Offset(mLockSelection, GuiRTSTSCtrl));
   endGroup("SelectionInfo");

   addGroup("SelectionTeamInfo");
   addField("selectionIncludesTeam",  TypeBool, Offset(mSelectionIncludesTeam, GuiRTSTSCtrl));
   addField("dragSelectionIncludesTeam", TypeBool, Offset(mDragSelectionIncludesTeam, GuiRTSTSCtrl));
   endGroup("SelectionTeamInfo");
}

//------------------------------------------------------------------------------
// Class guiRTSTSCtrl::Selection
// Based upon WorldEditor::Selection
//   defined in engine/editor/worldEditor.cc line 190
//------------------------------------------------------------------------------

GuiRTSTSCtrl::Selection::~Selection()
{
}

bool GuiRTSTSCtrl::Selection::objInSet(SceneObject * obj)
{
   return (mObjectList.find(mObjectList.begin(),mObjectList.end(),obj) != mObjectList.end());
}

bool GuiRTSTSCtrl::Selection::addObject(SceneObject * obj)
{
   if(objInSet(obj))
      return(false);

   mObjectList.addObject(obj);
   deleteNotify(obj);

   return(true);
}

bool GuiRTSTSCtrl::Selection::removeObject(SceneObject * obj)
{
   if(!objInSet(obj))
      return(false);

   mObjectList.removeObject(obj);
   clearNotify(obj);

   return(true);
}

void GuiRTSTSCtrl::Selection::clear()
{
   while(mObjectList.size())
      removeObject((SceneObject*)mObjectList[0]);
}

void GuiRTSTSCtrl::Selection::onDeleteNotify(SimObject * obj)
{
   removeObject((SceneObject*)obj);
}

void GuiRTSTSCtrl::Selection::enableCollision()
{
   for(U32 i = 0; i < mObjectList.size(); i++)
      ((SceneObject*)mObjectList[i])->enableCollision();
}

void GuiRTSTSCtrl::Selection::disableCollision()
{
   for(U32 i = 0; i < mObjectList.size(); i++)
      ((SceneObject*)mObjectList[i])->disableCollision();
}

void GuiRTSTSCtrl::startBuildingPlacement(RTSBuildingMarker* building)
{
   //can only build one building at a time...
   if (mPlacingBuilding)
      return;

   mPlacingBuilding = true;
   mNewBuilding = building;
   mNewBuilding->setTransVal(0.5f);
}

void GuiRTSTSCtrl::placeBuilding()
{
   if (!mPlacingBuilding || !mNewBuilding)
      return;

   Con::executef(this, 1, "placeBuilding");

   mNewBuilding = NULL;
   mPlacingBuilding = false;
}

//-----------------------------------------------------------------------------
// Interface functions for selection/drag-selection lists
//-----------------------------------------------------------------------------
U32 GuiRTSTSCtrl::getSelectionSize()
{
   return mSelected.size();
}

U32 GuiRTSTSCtrl::getDragSize()
{
   return mDragSelected.size();
}

void GuiRTSTSCtrl::addToSelection(SceneObject* obj)
{
   mSelected.addObject(obj);
}

void GuiRTSTSCtrl::addToDrag(SceneObject* obj)
{
   mDragSelected.addObject(obj);
}

void GuiRTSTSCtrl::removeFromSelection(SceneObject* obj)
{
   mSelected.removeObject(obj);
}

void GuiRTSTSCtrl::removeFromDrag(SceneObject* obj)
{
   mDragSelected.removeObject(obj);
}

SceneObject* GuiRTSTSCtrl::getSelectedObject(U32 index)
{
   return mSelected[index];
}

SceneObject* GuiRTSTSCtrl::getDragObject(U32 index)
{
   return mDragSelected[index];
}

void GuiRTSTSCtrl::clearSelection()
{
   mSelected.clear();
   Con::executef(this, 1, "onClearSelection" );
}

void GuiRTSTSCtrl::clearDrag()
{
   mDragSelected.clear();
}

bool GuiRTSTSCtrl::isObjectInSelection(SceneObject* obj)
{
   return mSelected.objInSet(obj);
}

bool GuiRTSTSCtrl::isObjectInDrag(SceneObject* obj)
{
   return mDragSelected.objInSet(obj);
}


//-----------------------------------------------------------------------------

ColorF GuiRTSTSCtrl::getSelectedColor()
{
   return ColorF(0.1, 0.8, 0.1);
}

ColorF GuiRTSTSCtrl::getToBeSelectedColor()
{
   return ColorF(0.3, 1.0, 0.3);
}

ColorF GuiRTSTSCtrl::getToBeDeselectedColor()
{
   return ColorF(1.0, 0.2, 0.2);
}


//------------------------------------------------------------------------------
// Script Interface
//------------------------------------------------------------------------------
ConsoleMethod( GuiRTSTSCtrl, selectGroup, void, 3, 3, "(Simset group) Add group to selection.")
{
   SimSet *group;
   RTSUnit *unit;

   if(!Sim::findObject(argv[2], group))
      return;

   for(S32 i=0; i<group->size(); i++)
   {
      if(unit = dynamic_cast<RTSUnit*>((*group)[i]))
         Con::executef(object, 2, "selectObject", unit->getIdString());
   }
}

ConsoleMethod( GuiRTSTSCtrl, unselectGroup, void, 3, 3, "(Simset group) Removes group from selection.")
{
   SimSet *group;

   if(!Sim::findObject(argv[2], group))
      return;

   for(S32 i=0; i<group->size(); i++)
      Con::executef(object, 2, "unselectObject", (SceneObject*)(*group)[i]->getIdString());
}

ConsoleMethod( GuiRTSTSCtrl, startBuildingPlacement, void, 3, 3, "(objectId)"
              "Sets mode to placing a building")
{
   SimObject* obj = Sim::findObject(dAtoi(argv[2]));

   if (RTSBuildingMarker* building = dynamic_cast<RTSBuildingMarker*>(Sim::findObject(dAtoi(argv[2]))))
      object->startBuildingPlacement(building);
   else
      Con::errorf("Invalid object id for new building: %i", dAtoi(argv[2]));
}

ConsoleMethod( GuiRTSTSCtrl, getSelectionSize, S32, 2, 2, "()"
              "Gets the current selection size")
{
   return object->getSelectionSize();
}

ConsoleMethod( GuiRTSTSCtrl, getDragSize, S32, 2, 2, "()"
              "Gets the current drag-selection size")
{
   return object->getDragSize();
}

ConsoleMethod( GuiRTSTSCtrl, addToSelection, void, 3, 3, "(object)"
              "Adds object to the selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   object->addToSelection(obj);
}

ConsoleMethod( GuiRTSTSCtrl, addToDrag, void, 3, 3, "(object)"
              "Adds object to the selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   object->addToDrag(obj);
}

ConsoleMethod( GuiRTSTSCtrl, removeFromSelection, void, 3, 3, "(object)"
              "Removes object from the selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   object->removeFromSelection(obj);
}

ConsoleMethod( GuiRTSTSCtrl, removeFromDrag, void, 3, 3, "(object)"
              "Removes object from the selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   object->removeFromDrag(obj);
}

ConsoleMethod( GuiRTSTSCtrl, getSelectedObject, S32, 3, 3, "(index)"
              "Gets the selected object at index")
{
   return object->getSelectedObject(dAtoi(argv[2]))->getId();
}

ConsoleMethod( GuiRTSTSCtrl, getDragObject, S32, 3, 3, "(index)"
              "Gets the drag-selected object at index")
{
   return object->getDragObject(dAtoi(argv[2]))->getId();
}

ConsoleMethod( GuiRTSTSCtrl, clearSelection, void, 2, 2, "()"
              "Clears the selection")
{
   object->clearSelection();
}

ConsoleMethod( GuiRTSTSCtrl, clearDrag, void, 2, 2, "()"
              "Clears the selection")
{
   object->clearDrag();
}

ConsoleMethod( GuiRTSTSCtrl, isObjectInSelection, bool, 3, 3, "(object)"
              "Returns true if the given object is in the selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   return object->isObjectInSelection(obj);
}

ConsoleMethod( GuiRTSTSCtrl, isObjectInDrag, bool, 3, 3, "(object)"
              "Returns true if the given object is in the drag-selection")
{
   SceneObject* obj = (SceneObject*)Sim::findObject(dAtoi(argv[2]));
   return object->isObjectInDrag(obj);
}

