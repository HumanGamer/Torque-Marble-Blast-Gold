#include "game/RTS/guiMapHud.h"
#include "console/consoleTypes.h"
#include "game/missionArea.h"
#include "sceneGraph/sceneGraph.h"
#include "game/RTS/RTSCamera.h"

IMPLEMENT_CONOBJECT(GuiMapHud);

//------------------------------------------------------------------------------
GuiMapHud::GuiMapHud() : GuiTSCtrl()
{
   // Initialize everything
   mTerrainBlock = NULL;

   mRenderBitmap = NULL;

   mGameVisDistance = 0.0f;
   mGameFogDistance = 0;

   mTextureSize = 200;
   mLeftMouseDown = false;
   mPingHead = NULL;
   mLastRenderTime = 0;

   mPingTexture = TextureHandle();
   mPingTextureName = StringTable->insert("");
};

GuiMapHud::~GuiMapHud()
{
   mTerrainBlock = NULL;
   //DON'T NEED TO DELETE BITMAP! TEXTURE MANAGER DOES THIS
}

bool GuiMapHud::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void GuiMapHud::onRemove()
{
   Parent::onRemove();
}

//------------------------------------------------------------------------------
bool GuiMapHud::onWake()
{
   if(!Parent::onWake())
   {
      return false;
   }
   setActive(true);

   mConnection = GameConnection::getServerConnection();
   mPingTexture = TextureHandle(mPingTextureName, BitmapTexture);

   // return ok
   if( mConnection == NULL )
      return true;
   else
      return rebuildMap();
}

void GuiMapHud::onSleep()
{
   mConnection = 0; //make sure there's a connection when we wake
   Parent::onSleep();
}

void GuiMapHud::inspectPostApply()
{
   Parent::inspectPostApply();

   if (mTextureSize < 32)
   {
      Con::errorf("Map texture size cannot be less than 32.");
      mTextureSize = 32;
   }
   if (mTextureSize > 2048)
   {
      Con::errorf("Map texture size cannot be greater than 2048.");
      mTextureSize = 2048;
   }
   mBounds.extent = Point2I(mTextureSize, mTextureSize);

   //make sure we have a map to display
   rebuildMap();
}

ConsoleMethod(GuiMapHud, rebuildMap, bool, 2, 3, "(bool force)")
{
   if (argc == 2)
      return object->rebuildMap();
   else
      return object->rebuildMap(dAtob(argv[2]));
}

void GuiMapHud::initPersistFields()
{
   Parent::initPersistFields();

   addField("textureSize", TypeS32,      Offset(mTextureSize, GuiMapHud));
   addField("pingTexture", TypeFilename, Offset(mPingTextureName, GuiMapHud));
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Projection stuff
//------------------------------------------------------------------------------
bool GuiMapHud::projectPoint(Point2F pt, Point2F* screenPos)
{
   const RectI missionArea = MissionArea::smMissionArea;
   Point2I point(pt.x, pt.y);

   pt.x -= missionArea.point.x;
   pt.y -= missionArea.point.y;
   Point2F pct(pt.x / missionArea.extent.x, pt.y / missionArea.extent.y);

   Point2I gutter(0,0);
   if (missionArea.extent.x < missionArea.extent.y)
      gutter.x = (1 - ((F32)missionArea.extent.x / missionArea.extent.y)) / 2 * mBounds.extent.x;
   else if (missionArea.extent.y < missionArea.extent.x)
      gutter.y = (1 - ((F32)missionArea.extent.y / missionArea.extent.x)) / 2 * mBounds.extent.y;

   screenPos->x = pct.x * (mBounds.extent.x - 2*gutter.x) + gutter.x;
   screenPos->y = (1-pct.y) * (mBounds.extent.y - 2*gutter.y) + gutter.y;

   if (!MissionArea::smMissionArea.pointInRect(point))
      return false;
   else
      return true;
}

bool GuiMapHud::projectPoint(Point3F pt, Point2F* screenPos)
{
   return projectPoint(Point2F(pt.x, pt.y), screenPos);
}

bool GuiMapHud::unprojectPoint(Point2I pt, Point3F* worldPos, bool needZ)
{
   const RectI missionArea = MissionArea::smMissionArea;
   //account for the gutters
   Point2I gutter(0,0);
   if (missionArea.extent.x < missionArea.extent.y)
   {
      gutter.x = (1 - ((F32)missionArea.extent.x / missionArea.extent.y)) / 2 * mBounds.extent.x;
      pt.x -= gutter.x;
      if (pt.x < 0 || pt.x >= mBounds.extent.x - 2*gutter.x)
         return false;
   }
   else if (missionArea.extent.y < missionArea.extent.x)
   {
      gutter.y = (1 - ((F32)missionArea.extent.y / missionArea.extent.x)) / 2 * mBounds.extent.y;
      pt.y -= gutter.y;
      if (pt.y < 0 || pt.y >= mBounds.extent.y - 2*gutter.y)
         return false;
   }

   Point2F pct((F32)pt.x / (mBounds.extent.x - 2*gutter.x), 1 - (F32)pt.y / (mBounds.extent.y - 2*gutter.y));
   pct.x *= missionArea.extent.x;
   pct.y *= missionArea.extent.y;
   worldPos->x = pct.x + missionArea.point.x;
   worldPos->y = pct.y + missionArea.point.y;

   //UNTESTED
   if (needZ)
   {
      TerrainBlock* block = gClientSceneGraph->getCurrentTerrain();
      Point3F terrPos = *worldPos;
      block->getWorldTransform().mulP(terrPos);
      terrPos.convolveInverse(block->getScale());

      F32 height;
      bool res = block->getHeight(Point2F(terrPos.x, terrPos.y), &height);
      if(res)
      {
         terrPos.z = height;
         terrPos.convolve(block->getScale());
         block->getTransform().mulP(terrPos);
      }
      worldPos->z = terrPos.z;
   }

   return true;
}


//------------------------------------------------------------------------------
// GUI stuff
//------------------------------------------------------------------------------
void GuiMapHud::onMouseDown(const GuiEvent &event)
{
   Point2I screenPos = event.mousePoint - mBounds.point;
   GuiControl* parent = getParent();
   while (parent)
   {
      screenPos -= parent->getPosition();
      parent     = parent->getParent();
   }

   Point3F worldPos(0,0,0);
   if (unprojectPoint(screenPos, &worldPos))
   {
      char* worldPosBuff = Con::getReturnBuffer(128);
      dSprintf(worldPosBuff, 128, "%f %f %f", worldPos.x, worldPos.y, worldPos.z);
      Con::executef(this, 2, "onMouseDown", worldPosBuff);
      Con::printf("Clicked at: %f %f", worldPos.x, worldPos.y);
   }
   //else
   //   Con::printf("Missed the map!");

   mLeftMouseDown = true;
}

void GuiMapHud::onMouseUp(const GuiEvent &event)
{
   mLeftMouseDown = false;
   Parent::onMouseUp(event);
}

void GuiMapHud::onMouseDragged(const GuiEvent &event)
{
   if (!mLeftMouseDown)
      return;

   Point2I screenPos = event.mousePoint - mBounds.point;
   GuiControl* parent = getParent();
   while (parent)
   {
      screenPos -= parent->getPosition();
      parent     = parent->getParent();
   }

   Point3F worldPos(0,0,0);
   if (unprojectPoint(screenPos, &worldPos))
   {
      char* worldPosBuff = Con::getReturnBuffer(128);
      dSprintf(worldPosBuff, 128, "%f %f %f", worldPos.x, worldPos.y, worldPos.z);
      Con::executef(this, 2, "onMouseDragged", worldPosBuff);
      Con::printf("Clicked at: %f %f", worldPos.x, worldPos.y);
   }
//   else
//      Con::printf("Missed the map!");
}

void GuiMapHud::onRightMouseDown(const GuiEvent &event)
{
   Point2I screenPos = event.mousePoint - mBounds.point;
   GuiControl* parent = getParent();
   while (parent)
   {
      screenPos -= parent->getPosition();
      parent = parent->getParent();
   }

   Point3F worldPos(0,0,0);
   if (unprojectPoint(screenPos, &worldPos, true))
   {
      char* worldPosBuff = Con::getReturnBuffer(128);
      dSprintf(worldPosBuff, 128, "%f %f %f", worldPos.x, worldPos.y, worldPos.z);
      Con::executef(this, 2, "onRightMouseDown", worldPosBuff);
   }
}

void GuiMapHud::createPingEvent(Point2F pos, ColorF color)
{
   Point2F screenPos;
   if (!projectPoint(pos, &screenPos))
      return;

   PingLocationEvent* event = new PingLocationEvent(screenPos, color);
   if (!mPingHead)
   {
      mPingHead = event;
      return;
   }

   event->next = mPingHead;
   mPingHead->prev = event;
   mPingHead = event;
}

void GuiMapHud::updatePings(U32 ms)
{
   if (!mPingHead)
      return;

   U32 count = 0;
   PingLocationEvent* event = mPingHead;
   while (event)
   {
      count++;
      PingLocationEvent* next = event->next;

      event->t += ms;
      if (event->t > pingLifetimeMS)
      {
         //remove it
         if (event->prev)                   //update prev's next pointer
            event->prev->next = event->next;
         if (event->next)                   //update next's prev pointer
            event->next->prev = event->prev;

         if (mPingHead == event)            //is this the first one?  If so, gotta change head
         {
            if (event->next)                //ok, there's still more in the list
               mPingHead = event->next;
            else                            //last event - kill the list
               mPingHead = NULL;
         }
         delete event;
      }

      event = next;
   }

   //Con::printf("Updated %i pings", count);
}


ConsoleMethod(GuiMapHud, createPingEvent, void, 4, 4, "hud.createPingEvent(worldPos, color)")
{
   Point2F pt;
   dSscanf(argv[2], "%f %f", &pt.x, &pt.y);
   ColorF col;
   dSscanf(argv[3], "%f %f %f", &col.red, &col.green, &col.blue);

   object->createPingEvent(pt, col);
}