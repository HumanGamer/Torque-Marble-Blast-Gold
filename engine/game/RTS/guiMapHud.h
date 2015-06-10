#ifndef _GUIMAPHUD_H_
#define _GUIMAPHUD_H_

#include "dgl/dgl.h"
#include "game/game.h"
#include "terrain/terrRender.h"
#include "game/gameConnection.h"
#include "game/missionArea.h"
#include "gui/guiTSControl.h"

/// This is a somewhat-simple minimap.  It renders an ortho-projection image of the
/// relevant mission area, and copies it into a texture.  Note that this method requires
/// the area of the window that it will render into not to be covered up while it's doing
/// this.  An alternate method of render-to-texture would be advisable, but is beyond
/// the scope of this pack.
///
/// In addition to rendering units, the viewable area, and terrain, the map can also
/// render "pings".  Pings are alerts that appear on the map that can signify different
/// things.  They might be used so that teammates can easily point out important areas
/// of the map, or to signify that something is under attack and needs your attention.
class GuiMapHud : public GuiTSCtrl
{
   private:
      typedef GuiTSCtrl Parent;

      /// Some constants that control the rendering of map pings
      enum
      {
         pingCycleMS = 500,
         pingLifetimeMS = 2000,
         pingMagnitude = 10,
         pingRadius = 5
      };

      /// Stores all the information relating to a Ping on the minimap.
      struct PingLocationEvent
      {
         Point2F screenPos;
         ColorF color;
         U32 t;
         PingLocationEvent* next;
         PingLocationEvent* prev;

         PingLocationEvent()
         {
            t = 0;
            next = NULL;
            prev = NULL;
         }
         PingLocationEvent(Point2F in_screenPos, ColorF in_color)
         {
            t = 0;
            next = NULL;
            prev = NULL;
            screenPos = in_screenPos;
            color = in_color;
         }
      };

      /// Keep a linked list of the active pings.
      PingLocationEvent* mPingHead;
      U32 mLastRenderTime;

      bool mLeftMouseDown;

      U32 mTerrainSize;
      U32 mTextureSize;

      SimObjectPtr<GameConnection> mConnection;

      TerrainBlock* mTerrainBlock;
      GBitmap* mRenderBitmap;
      TextureHandle mRenderTexture;

      // Visibility variables
      F32 mGameVisDistance;
      F32 mGameFogDistance;
      F32 mVisDistanceMod;

      F32 mMinHeight;
      F32 mMaxHeight;

      StringTableEntry mPingTextureName;
      TextureHandle mPingTexture;

      //Takes a point in world space and turns it into screen space
      bool projectPoint(Point2F pt, Point2F* screenPos);
      bool projectPoint(Point3F pt, Point2F* screenPos);
      //Takes a point in screen space and turns it into world space
      bool unprojectPoint(Point2I pt, Point3F* worldPos, bool needZ = false);

      void setupRender();
      void cleanupRender();

      bool processCameraQuery(CameraQuery * query);

      void updatePings(U32 ms);

   public:
      // Constructor and Destructor
      GuiMapHud();
      ~GuiMapHud();

      void createPingEvent(Point2F pt, ColorF color);

      void renderWorld(const RectI &updateRect);
      bool rebuildMap(bool force = false);
      bool rebuildMapOld();

      // Public GUI Methods
      bool onWake();
      void onSleep();
      bool onAdd();
      void onRemove();
      void inspectPostApply();
      static void initPersistFields();

      void onMouseDown(const GuiEvent &event);
      void onMouseUp(const GuiEvent &event);
      void onRightMouseDown(const GuiEvent &event);
      void onMouseDragged(const GuiEvent &event);

      // Rendering Methods
      void onRender(Point2I offset, const RectI &updateRect);

      DECLARE_CONOBJECT(GuiMapHud);
};

#endif
