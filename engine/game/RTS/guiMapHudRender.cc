#include "game/RTS/guiMapHud.h"
#include "game/RTS/RTSUnit.h"
#include "game/missionArea.h"
#include "game/RTS/RTSCamera.h"
#include "game/RTS/RTSConnection.h"

//invoked to render the actual map with everything on it
void GuiMapHud::onRender(Point2I offset, const RectI &updateRect)
{
   RTSConnection* conn = (RTSConnection*)NetConnection::getServerConnection();
   //need a net connection
   if (!mConnection || !mTerrainBlock)
      return;

   //update the pings
   U32 currTime = Platform::getVirtualMilliseconds();
   if (mLastRenderTime)
      updatePings(currTime - mLastRenderTime);
   mLastRenderTime = currTime;

   //render the map
   dglSetBitmapModulation(ColorF(1.0, 1.0, 1.0));
   dglDrawBitmap(mRenderTexture, offset, GFlip_Y);

   const RectI missionArea = MissionArea::smMissionArea;


   //render the units
   glPointSize(2.0f);
   glColor3f(0.0, 0.0, 1.0);
   glBegin(GL_POINTS);
   static char buff[18];
   dSprintf(buff, 18, "$Server::TeamInfo0");
   for (SimSetIterator itr(mConnection); *itr; ++itr)
   {
      if ((*itr)->getType() & PlayerObjectType)
      {
         if (RTSUnit* unit = dynamic_cast<RTSUnit*>(*itr))
         {
            if (!conn->isUnitVisible(unit))
               continue;

            Point2F screenPos;
            if (projectPoint(unit->getPosition(), &screenPos))
            {
               S32 team = unit->getTeam();
               buff[17] = '0' + team;
               const char* pref = Con::getVariable(buff);
               ColorF teamColor;
               dSscanf(pref, "%f %f %f", &teamColor.red, &teamColor.green, &teamColor.blue);
               glColor3f(teamColor.red, teamColor.green, teamColor.blue);
               glVertex2f(offset.x + screenPos.x, offset.y + screenPos.y);
            }
         }
      }
   }
   glEnd();
   glPointSize(1.0f);


   //render the camera box
   if (RTSCamera* camera = dynamic_cast<RTSCamera*>(mConnection->getControlObject()))
   {
      RectF cameraRect = camera->getViewableRect();
      Point2F center = cameraRect.point + cameraRect.extent * 0.5;

      Point2F screenCenter;
      projectPoint(center, &screenCenter);
      Point2F squareOffset;
      projectPoint(Point3F(center.x + cameraRect.extent.x / 2, center.y + cameraRect.extent.y / 2, 0), &squareOffset);
      squareOffset -= screenCenter;
      screenCenter.x += offset.x;
      screenCenter.y += offset.y;

      glColor3f(0.0, 1.0, 0.0);
      glBegin(GL_LINE_LOOP);
      glVertex2f(screenCenter.x - squareOffset.x, screenCenter.y - squareOffset.y);
      glVertex2f(screenCenter.x + squareOffset.x, screenCenter.y - squareOffset.y);
      glVertex2f(screenCenter.x + squareOffset.x, screenCenter.y + squareOffset.y);
      glVertex2f(screenCenter.x - squareOffset.x, screenCenter.y + squareOffset.y);
      glEnd();
   }


   //render gutters to hide what's outside the mission area
   if (missionArea.extent.x < missionArea.extent.y)
   {
      S32 rectWidth = (1 - ((F32)missionArea.extent.x / missionArea.extent.y)) / 2 * mBounds.extent.x;
      RectI rect(offset.x, offset.y, rectWidth, mBounds.extent.y);
      dglDrawRectFill(rect, ColorI(0,0,0));
      rect.point.x = offset.x + mBounds.extent.x - rectWidth;
      dglDrawRectFill(rect, ColorI(0,0,0));
   }
   else if (missionArea.extent.y < missionArea.extent.x)
   {
      S32 rectHeight = (1 - ((F32)missionArea.extent.y / missionArea.extent.x)) / 2 * mBounds.extent.y;
      RectI rect(offset.x, offset.y, mBounds.extent.x, rectHeight);
      dglDrawRectFill(rect, ColorI(0,0,0));
      rect.point.y = offset.y + mBounds.extent.y - rectHeight;
      dglDrawRectFill(rect, ColorI(0,0,0));
   }


   //render pings
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, mPingTexture.getGLName());
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBegin(GL_QUADS);
   PingLocationEvent* event = mPingHead;
   while (event)
   {
      glColor3f(event->color.red, event->color.green, event->color.blue);
      Point2F center = Point2F(offset.x + event->screenPos.x, offset.y + event->screenPos.y);
      F32 magnitude = mFabs(pingMagnitude * mSin(event->t / (F32)pingCycleMS * M_PI));
      magnitude += pingRadius;
      glTexCoord2f(0.0, 1.0);
      glVertex2f(center.x - magnitude, center.y - magnitude);
      glTexCoord2f(1.0, 1.0);
      glVertex2f(center.x + magnitude, center.y - magnitude);
      glTexCoord2f(1.0, 0.0);
      glVertex2f(center.x + magnitude, center.y + magnitude);
      glTexCoord2f(0.0, 0.0);
      glVertex2f(center.x - magnitude, center.y + magnitude);
      event = event->next;
   }
   glEnd();
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
}
