#include "game/RTS/guiMapHud.h"
#include "sceneGraph/sceneGraph.h"
#include "interior/interiorInstance.h"
#include "game/missionArea.h"

static Point2F sCurrRenderPos;
static F32 sHalfRenderWidth;

//rebuild map using the backbuffer for a framebuffer
bool GuiMapHud::rebuildMapOld()
{
   GLint drawBuffer = 0, readBuffer = 0;
   glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
   glGetIntegerv(GL_READ_BUFFER, &readBuffer);
   glDrawBuffer(GL_BACK);
   glReadBuffer(GL_BACK);

   RectI oldBounds = mBounds;

   //make sure there is enough screen space
   U32 maxSize = Platform::getWindowSize().y;
   if (!isPow2(maxSize))
      maxSize = getNextPow2(maxSize) / 2;
   //need to do at least 4 pieces
   if (maxSize > mTextureSize / 2)
      maxSize = mTextureSize / 2;
   U32 subDivs = mTextureSize / maxSize;

   const RectI missionArea = MissionArea::smMissionArea;
   S32 biggerExtent = getMax(missionArea.extent.x, missionArea.extent.y);

   //setup some constants
   S32 texelSquareSize = maxSize;
   S32 renderSquareSize = biggerExtent / subDivs;
   sHalfRenderWidth = renderSquareSize / 2;

   Point2I center = (missionArea.extent / 2) + missionArea.point;
   Point2I start;
   if (missionArea.extent.x < missionArea.extent.y)
   {
      start.y = missionArea.point.y + sHalfRenderWidth;
      start.x = center.x + (missionArea.point.y - center.y) + sHalfRenderWidth;
   }
   else if (missionArea.extent.y < missionArea.extent.x)
   {
      start.x = missionArea.point.x + sHalfRenderWidth;
      start.y = center.y + (missionArea.point.x - center.x) + sHalfRenderWidth;
   }
   else
   {
      start = missionArea.point + Point2I(sHalfRenderWidth, sHalfRenderWidth);
   }

   mBounds.extent = Point2I(texelSquareSize, texelSquareSize);
   mBounds.point = Point2I(0,Platform::getWindowSize().y - texelSquareSize);

   //ok, we're good now...
   glColor3f(1.0,1.0,1.0);
   Con::printf("Map generation beginning...");
   U32 startTime = Platform::getRealMilliseconds();
   for (S32 v = 0; v < subDivs; v++)
   {
      sCurrRenderPos.y = start.y + v * renderSquareSize;
      for (S32 u = 0; u < subDivs; u++)
      {
         sCurrRenderPos.x = start.x + u * renderSquareSize;

         U32 start = Platform::getRealMilliseconds();
         glClear(GL_DEPTH_BUFFER_BIT);
         Parent::onRender(mBounds.point, mBounds);
         Con::printf("Section rendered in %.3f seconds", (Platform::getRealMilliseconds() - start) / 1000.0);

         start = Platform::getRealMilliseconds();
         glFinish();
         glBindTexture(GL_TEXTURE_2D, mRenderTexture.getGLName());
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
            texelSquareSize*u, texelSquareSize*v,
            0, 0,
            texelSquareSize, texelSquareSize);
         Con::printf("Section captured in %.3f seconds", (Platform::getRealMilliseconds() - start) / 1000.0);
      }
   }

   //restore data
   mBounds = oldBounds;
   glDrawBuffer(drawBuffer);
   glReadBuffer(readBuffer);

   Con::printf("Map generation complete in %.3f seconds", (Platform::getRealMilliseconds() - startTime) / 1000.0);

   return true;
}

bool GuiMapHud::rebuildMap(bool force)
{
   // get game server connection
   mConnection = GameConnection::getServerConnection();
   if(!mConnection)
      return false;

   Con::printf("Proceeding with mini-map generation...");

   TerrainBlock* newTerrain = gClientSceneGraph->getCurrentTerrain();
   AssertWarn(newTerrain, "No terrain block!");
   if (!newTerrain)
      return false;

   //ok, we've got everything we need - now do some calculations
   mTerrainBlock = newTerrain;
   mTerrainSize = TerrainBlock::BlockSize * newTerrain->getSquareSize();

   //make sure we're just above the highest object
   mMaxHeight = 0;
   mMinHeight = 1000;
   for (U32 v = 0; v < TerrainBlock::BlockSize; v++)
   {
      for (U32 u = 0; u < TerrainBlock::BlockSize; u++)
      {
         GridSquare* gs = newTerrain->findSquare(TerrainBlock::BlockShift, u, v);
         if (gs->maxHeight * 0.03125f > mMaxHeight)
            mMaxHeight = gs->maxHeight * 0.03125f;
         if (gs->minHeight * 0.03125f < mMinHeight)
            mMinHeight = gs->minHeight * 0.03125f;
      }
   }

   mMaxHeight += 1;

   //allocate bitmap, and backup old data
   //DON'T NEED TO DELETE BITMAP! TEXTURE MANAGER DOES THIS
   mRenderBitmap = new GBitmap(mTextureSize, mTextureSize);
   mRenderTexture = TextureHandle(NULL, mRenderBitmap);

   rebuildMapOld();
   return true;
}

//set up a section to be rendered
void GuiMapHud::setupRender()
{
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   glClear(GL_DEPTH_BUFFER_BIT); //don't clear the depth buffer
   glDisable(GL_CULL_FACE);
   glMatrixMode(GL_MODELVIEW);
   dglSetCanonicalState();
   //TerrainRender::mRenderingCommander = true;

   // set new and old visibility data
   mGameVisDistance = gClientSceneGraph->getVisibleDistance();
   mGameFogDistance = gClientSceneGraph->getFogDistance();
   mVisDistanceMod = gClientSceneGraph->getVisibleDistanceMod();
   F32 x = pow(mMaxHeight - mMinHeight + 2, 2);
   F32 y = pow(sHalfRenderWidth * mSqrt(2.0), 2);
   F32 dist = mSqrt(x + y) + 200;
   gClientSceneGraph->setVisibleDistance(dist);
   gClientSceneGraph->setFogDistance(dist + 50);
   gClientSceneGraph->smVisibleDistanceMod = 1.0;
}

//render a section
void GuiMapHud::renderWorld(const RectI &updateRect)
{
   setupRender();

   gClientSceneGraph->renderScene(TerrainObjectType | WaterObjectType);

   cleanupRender();
}

//cleanup that render
void GuiMapHud::cleanupRender()
{
   //TerrainRender::mRenderingCommander = false;
   gClientSceneGraph->setVisibleDistance(mGameVisDistance);
   gClientSceneGraph->setFogDistance(mGameFogDistance);
   gClientSceneGraph->smVisibleDistanceMod = mVisDistanceMod;
}

//GuiTSControl::onRender calls this for camera info before rendering
bool GuiMapHud::processCameraQuery(CameraQuery * query)
{
   // Get our camera matrix
   if(!mConnection->getControlCameraTransform(0.0f, &query->cameraMatrix))
      return false;

   // Set the rotation angle
   Point3F rotation(mDegToRad(90.0f), 0.0f, 0.0f);

   // Set near/far planes
   query->nearPlane = 0.1;
   query->farPlane = mMaxHeight - mMinHeight + 2;

   query->leftRight = sHalfRenderWidth;
   query->topBottom = sHalfRenderWidth;
   query->ortho = true;

   if(!query->cameraMatrix.isIdentity())
      query->cameraMatrix.identity();

   Point3F pos(sCurrRenderPos.x, sCurrRenderPos.y, mMaxHeight);
   query->cameraMatrix.setPosition(pos);
   query->cameraMatrix.mul(MatrixF(rotation));

   // Return OK.
   return(true);
}