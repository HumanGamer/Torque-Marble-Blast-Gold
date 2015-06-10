#include "game/RTS/visManager.h"
#include "game/RTS/RTSUnit.h"
#include "game/RTS/RTSConnection.h"
#include "platform/profiler.h"

VisManager* gServerVisManager = new VisManager;
VisManager* gClientVisManager = new VisManager;

//TODO: get client-side render check working with TeamsShareVision = false
bool gTeamsShareVision = true;
bool gFogOfWarIsOn     = true;

void VisManager::addObject(RTSUnit* unit)
{
   mObjectList.addObject(unit);
}

void VisManager::removeObject(RTSUnit* unit)
{
   mObjectList.removeObject(unit);
}

void VisManager::onConnectionSet(RTSUnit* unit, RTSConnection* conn)
{
   if (gFogOfWarIsOn)
   {
      if (gTeamsShareVision)
      {
         //fog of war on, teams share vision, so only set visible to friendlies
         for (U32 k = 0; k < Sim::getClientGroup()->size(); k++)
         {
            RTSConnection* thisConn = (RTSConnection*)((*Sim::getClientGroup())[k]);
            if (thisConn->getTeam() == conn->getTeam())
               thisConn->setUnitVisible(unit);
            else
               thisConn->setUnitInvisible(unit);
         }
      }
      else
      {
         //fog of war on, but can only see your own units...
         for (U32 k = 0; k < Sim::getClientGroup()->size(); k++)
         {
            RTSConnection* thisConn = (RTSConnection*)((*Sim::getClientGroup())[k]);
            if (thisConn == conn)
               thisConn->setUnitVisible(unit);
            else
               thisConn->setUnitInvisible(unit);
         }
      }
   }
   else
   {
      //no fog of war
      for (U32 k = 0; k < Sim::getClientGroup()->size(); k++)
      {
         RTSConnection* thisConn = (RTSConnection*)((*Sim::getClientGroup())[k]);
         thisConn->setUnitVisible(unit);
      }
   }
}

void VisManager::processServer()
{
   if (!mDirty || !gFogOfWarIsOn)
      return; //nothing has changed...

   PROFILE_START(VisManager_Process_Server);
   //U32 numDirty = 0;
   //U32 numSkipped = 0;
   //U32 numProcessedUnits = 0;
   for (U32 i = 0; i < mObjectList.size(); i++)
   {
      RTSUnit* unit = (RTSUnit*)mObjectList[i];

      //make sure something changed somehow
      if (!unit->isDirty())
      {
         //numSkipped++;
         continue;
      }

      //numDirty++;

      RTSConnection* client = unit->getControllingConnection();
      for (U32 k = 0; k < mObjectList.size(); k++)
      {
         RTSUnit* target = (RTSUnit*)mObjectList[k];
         RTSConnection* targetClient = target->getControllingConnection();
         if (!targetClient)
            continue;

         if (targetClient == client)
            continue;
         if (gTeamsShareVision && targetClient->getTeam() == client->getTeam())
            continue;

         VectorF vec = unit->getPosition() - target->getPosition();
         F32 lenSqr = vec.lenSquared();
         F32 unitVision = ((RTSUnitData*)(unit->getDataBlock()))->mVision + unit->mNetModifier.mVision;
         F32 targetVision = ((RTSUnitData*)(target->getDataBlock()))->mVision + target->mNetModifier.mVision;

         if (client)
         {
            if (unitVision * unitVision > lenSqr)
               client->setUnitVisible(target);
            else
               client->setUnitInvisible(target);
         }

         if (targetClient)
         {
            if (targetVision * targetVision > lenSqr)
               targetClient->setUnitVisible(unit);
            else
               targetClient->setUnitInvisible(unit);
         }

         //numProcessedUnits++;
      }

      unit->setClean();
   }

   /*
#ifdef TORQUE_DEBUG
   Con::printf("Num DIRTY: %i num SKIPPED: %i", numDirty, numSkipped);

   if (numProcessedUnits)
      Con::printf("Had to process %i units.", numProcessedUnits);
#endif
      */

   PROFILE_END();

   mDirty = false;
}

void VisManager::onSetTeam(RTSUnit* unit)
{
   RTSConnection* conn = RTSConnection::getServerConnection();
   if (unit->getTeam() == conn->getTeam())
      conn->setUnitVisible(unit);
}

//objects that won't be visible will be deleted next time they get an update
void VisManager::processClient()
{
   if (!mDirty || !gFogOfWarIsOn)
      return;
   PROFILE_START(VisManager_Process_Client);

   RTSConnection* conn = RTSConnection::getServerConnection();
   for (U32 i = 0; i < mObjectList.size(); i++)
   {
      RTSUnit* unit = (RTSUnit*)mObjectList[i];

      if (unit->getTeam() == conn->getTeam())
         continue;

      //need to see if any of the "targets" can see this "unit"
      conn->setUnitInvisible(unit);

      for (U32 k = 0; k < mObjectList.size(); k++)
      {
         RTSUnit* target = (RTSUnit*)mObjectList[k];

         if (target->getTeam() != conn->getTeam())
            continue;

         VectorF vec = unit->getPosition() - target->getPosition();
         F32 lenSqr = vec.lenSquared();
         F32 targetVision = ((RTSUnitData*)(target->getDataBlock()))->mVision + target->mNetModifier.mVision;

         if (targetVision * targetVision > lenSqr)
         {
            conn->setUnitVisible(unit);
            break;
         }
      }

      unit->setClean();
   }

   PROFILE_END();
   mDirty = false;
}

void VisManager::handleConnectionDrop(RTSConnection *conn)
{
   for (SimSetIterator itr(&mObjectList); *itr; ++itr)
   {
      RTSUnit *unit = static_cast<RTSUnit *>(*itr);

      if( unit->getControllingConnection() == conn )
      {
         unit->clearControllingConnection();
      }
   }
}
