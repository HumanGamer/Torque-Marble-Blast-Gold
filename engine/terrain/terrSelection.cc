#include "terrain/terrRender.h"
#include "game/RTS/guiRTSTSCtrl.h"
#include "gui/guiCanvas.h"

U32               TerrainRender::mDynamicSelectionCount;
TerrSelectionInfo TerrainRender::mTerrainSelections[MaxTerrainSelections];

SelectionField TerrainRender::TestSquareSelections(GridSquare *sq, S32 level, Point2I sqPos, SelectionField selectionMask)
{
   SelectionField retMask;

   F32 blockX = sqPos.x * mSquareSize + mBlockPos.x;
   F32 blockY = sqPos.y * mSquareSize + mBlockPos.y;
   F32 blockZ = fixedToFloat(sq->minHeight);

   F32 blockSize = mSquareSize * (1 << level);
   F32 blockHeight = fixedToFloat(sq->maxHeight - sq->minHeight);

   Point3F vec;

   S32 lastBit = mDynamicSelectionCount;
   for (lastBit = mDynamicSelectionCount; lastBit > 0; lastBit--)
   {
      if (selectionMask.test(lastBit))
         break;
   }

   for(S32 i = 0; i <= lastBit; i++)
   {
      if(selectionMask.test(i))
      {
         Point3F *pos = &mTerrainSelections[i].pos;
         // test the visibility of this selection to box
         // find closest point on box to selection and test
         
         if(pos->z < blockZ)
            vec.z = blockZ - pos->z;
         else if(pos->z > (blockZ + blockHeight))
            vec.z = pos->z - (blockZ + blockHeight);
         else
            vec.z = 0;
         
         if(pos->x < blockX)
            vec.x = blockX - pos->x;
         else if(pos->x > blockX + blockSize)
            vec.x = pos->x - (blockX + blockSize);
         else
            vec.x = 0;

         if(pos->y < blockY)
            vec.y = blockY - pos->y;
         else if(pos->y > blockY + blockSize)
            vec.y = pos->y - (blockY + blockSize);
         else
            vec.y = 0;
         
         F32 dist = vec.len();

         if(dist < mTerrainSelections[i].radius)
            retMask.set(i);
      }
   }
   return retMask;
}
void TerrainRender::addSelection(U32 index, SceneObject* obj, ColorF color)
{
   // set the 'fo
   TerrSelectionInfo & info = mTerrainSelections[index];

   mCurrentBlock->getWorldTransform().mulP(obj->getPosition(), &info.pos);
   Box3F box = obj->getObjBox();
   info.radius = getMax(box.max.x - box.min.x, box.max.y - box.min.y);
   info.radiusSquared = info.radius * info.radius;
   
   //
   info.r = color.red;
   info.g = color.green;
   info.b = color.blue;

   Point3F dVec = mCamPos - obj->getPosition();
   info.distSquared = mDot(dVec, dVec);
}

void TerrainRender::buildSelectionArray()
{
   GuiRTSTSCtrl* playgui = dynamic_cast<GuiRTSTSCtrl*>(Canvas->getContentControl());
   
   if (!playgui)
      return;

   GuiRTSTSCtrl::Selection& selection = playgui->getSelection();
   GuiRTSTSCtrl::Selection& dragSelection = playgui->getDragSelection();
   SceneObject* hitObject = playgui->getHitObject();

   ColorF selectedColor = playgui->getSelectedColor();
   ColorF selectColor   = playgui->getToBeSelectedColor();
   ColorF deselectColor = playgui->getToBeDeselectedColor();

   // first do our current selection
   U32 curIndex = 0;
   for(U32 i = 0; i < playgui->getSelectionSize() && curIndex < MaxTerrainSelections; i++)
   {
      //don't draw things that are in the drag selection with this color
      if(dragSelection.objInSet(selection[i]) || hitObject == selection[i])
         continue;

      addSelection(curIndex++, selection[i], selectedColor);
   }

   //now do the hit object if it exists and it's not terrain...
   if (hitObject && curIndex < MaxTerrainSelections && !(hitObject->getTypeMask() & TerrainObjectType))
   {
      addSelection(curIndex++, hitObject, selectColor);
   }

   //and finally, drag selected items...
   for(S32 i = 0; i < dragSelection.size() && curIndex < MaxTerrainSelections; i++)
   {
      if(hitObject == dragSelection[i])
         continue;
      if(selection.objInSet(dragSelection[i]))
         addSelection(curIndex++, dragSelection[i], deselectColor);
      else
         addSelection(curIndex++, dragSelection[i], selectColor);
   }


   mDynamicSelectionCount = curIndex;
}

