//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIRTSTSCTRL_H_
#define _GUIRTSTSCTRL_H_

#ifndef _EDITTSCTRL_H_
#include "editor/editTSCtrl.h"
#endif
#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif

#include "game/RTS/RTSBuilding.h"

class SceneObject;
class GuiRTSTSCtrl : public EditTSCtrl
{
private:
   typedef EditTSCtrl Parent;

public:
   struct CollisionInfo
   {
      SceneObject *     obj;
      Point3F           pos;
      VectorF           normal;
      Point3F           start;
   };

   enum SelectionModifier
   {
      Shift,
      Control,
      Alt
   };

   enum PanEdge {
      EdgeNone   = bit(0),
      EdgeLeft   = bit(1),
      EdgeRight  = bit(2),
      EdgeTop    = bit(3),
      EdgeBottom = bit(4)
   };
   U32 mPrevEdge;

   SceneObject * getControlObject();
   bool collide(const Gui3DMouseEvent & event, CollisionInfo & info);

   GuiRTSTSCtrl();
   ~GuiRTSTSCtrl();

   // SimObject
   bool onWake();
   void onSleep();

   // EditTSCtrl
   void on3DMouseMove(const Gui3DMouseEvent & event);
   void on3DMouseDown(const Gui3DMouseEvent & event);
   void on3DMouseUp(const Gui3DMouseEvent & event);
   void on3DMouseDragged(const Gui3DMouseEvent & event);
   void on3DMouseEnter(const Gui3DMouseEvent & event);
   void on3DMouseLeave(const Gui3DMouseEvent & event);
   void on3DRightMouseDown(const Gui3DMouseEvent & event);
   void on3DRightMouseUp(const Gui3DMouseEvent & event);

   void updateGuiInfo();

   //
   void onRender(Point2I offset, const RectI &updateRect);
   void renderScene(const RectI & updateRect);
   void drawDamage(SceneObject *obj);

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiRTSTSCtrl);

   //Start creation of a building.  This will make a building marker
   //attatch to the cursor so it can be place properly
   void startBuildingPlacement(RTSBuildingMarker* building);
   //This tells the script to place the current building marker and
   //create an actual building.
   void placeBuilding();

   U32 getSelectionSize();
   U32 getDragSize();
   void addToSelection(SceneObject* obj);
   void addToDrag(SceneObject* obj);
   void removeFromSelection(SceneObject* obj);
   void removeFromDrag(SceneObject* obj);
   SceneObject* getSelectedObject(U32 index);
   SceneObject* getDragObject(U32 index);
   void clearSelection();
   void clearDrag();
   bool isObjectInSelection(SceneObject* obj);
   bool isObjectInDrag(SceneObject* obj);

   class Selection : public SimObject
   {
      typedef SimObject    Parent;

      private:

         SimSet mObjectList;

      public:

         ~Selection();

         //
         U32 size()  { return(mObjectList.size()); }
         SceneObject * operator[] (S32 index) { return((SceneObject*)mObjectList[index]); }

         bool objInSet(SceneObject *);

         bool addObject(SceneObject *);
         bool removeObject(SceneObject *);
         void clear();

         void onDeleteNotify(SimObject *);

         void enableCollision();
         void disableCollision();

   };

   Selection& getSelection() { return mSelected; }
   Selection& getDragSelection() { return mDragSelected; }
   SceneObject* getHitObject() { return mHitObject; }
   ColorF getSelectedColor();
   ColorF getToBeSelectedColor();
   ColorF getToBeDeselectedColor();

   private:
      // Selection
      Selection   mSelected;
      Selection   mDragSelected;
      bool        mDragSelect;
      RectI       mDragRect;
      Point2I     mDragStart;

      ColorF   mDamageFillColor;
      ColorF   mDamageFrameColor;
      Point2I  mDamageRectSize;

      CollisionInfo              mHitInfo;
      Point3F                    mHitOffset;
      SimObjectPtr<SceneObject>  mHitObject;

      bool mPlacingBuilding;
      RTSBuildingMarker* mNewBuilding;

   public:
      ColorI    mDragRectColor;

      bool      mLockSelection;

      bool      mSelectionIncludesTeam;
      bool      mDragSelectionIncludesTeam;
      

};

#endif



