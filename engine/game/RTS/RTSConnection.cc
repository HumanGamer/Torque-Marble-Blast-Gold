//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "core/bitStream.h"
#include "sim/pathManager.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneLighting.h"
#include "audio/audioDataBlock.h"
#include "game/game.h"
#include "game/shapeBase.h"
#include "game/gameConnection.h"
#include "game/gameConnectionEvents.h"
#include "game/auth.h"
#include "game/RTS/RTSUnit.h"
#include "game/RTS/RTSConnection.h"
#include "game/RTS/visManager.h"

IMPLEMENT_CONOBJECT(RTSConnection);

class SetTeamEvent : public NetEvent
{
   S32 mTeam;
public:
   SetTeamEvent(U32 team = 0)
   {
      mTeam = team;
   }
   void pack(NetConnection *, BitStream *bstream)
   {
      bstream->writeInt(mTeam, 5);
   }
   void write(NetConnection *con, BitStream *bstream)
   {
      pack(con, bstream);
   }
   void unpack(NetConnection *, BitStream *bstream)
   {
      mTeam = bstream->readInt(5);
   }
   void process(NetConnection *con)
   {
      if(con->isServerConnection())
      {
         RTSConnection *rtscon = dynamic_cast<RTSConnection*>(con);
         if(rtscon)
            rtscon->setTeam(mTeam);
      }
   }
   DECLARE_CONOBJECT(SetTeamEvent);
};

class RTSUnitMoveEvent : public NetEvent
{
public:
   SimSet* selection;
   SimSet objects;
   Point2F pos;

   RTSUnitMoveEvent()
   {
      selection = NULL;
      objects.registerObject();
   }

   ~RTSUnitMoveEvent()
   {
      objects.unregisterObject();
   }

   void pack(NetConnection *con, BitStream *bstream)
   {
      bstream->write(pos.x);
      bstream->write(pos.y);

      S32 numUnits = 0;
      for (S32 k = 0; k < selection->size(); k++)
      {
         AssertFatal(dynamic_cast<NetObject*>((*selection)[k]), "Non-NetObject object in move event!");
         NetObject* obj = (NetObject*)((*selection)[k]);
         if (con->getGhostIndex(obj) != -1)
            numUnits++;
      }
      bstream->write(numUnits);

      for (S32 k = 0; k < selection->size(); k++)
      {
         NetObject* obj = (NetObject*)((*selection)[k]);
         S32 id = con->getGhostIndex(obj);
         if (id != -1)
         {
            // Sanity checking...
            numUnits--;
            bstream->writeInt(id, NetConnection::GhostIdBitSize);
         }
      }

      AssertFatal(numUnits == 0, "RTSUnitMoveEvent - Wrote a different number of selections than expected!");
   }

   void write(NetConnection *con, BitStream *bstream)
   {
      pack(con, bstream);
   }

   void unpack(NetConnection *con, BitStream *bstream)
   {
      bstream->read(&pos.x);
      bstream->read(&pos.y);

      S32 numUnits;
      bstream->read(&numUnits);

      for (S32 k = 0; k < numUnits; k++)
      {
         S32 id = bstream->readInt(NetConnection::GhostIdBitSize);
         AssertFatal(id != -1, "RTSUnitMoveEvent::unpack - Unexpected invalid ghost ID got written!");

         NetObject* obj = con->resolveGhost(id);
         if(dynamic_cast<RTSUnit*>(obj))
            objects.addObject(obj);
      }
   }
   void process(NetConnection *con)
   {
      S32 realSize = 0;
      Point3F center(0,0,0);
      for (S32 k = 0; k < objects.size(); k++)
      {
         if (RTSUnit* unit = dynamic_cast<RTSUnit*>(objects[k]))
         {
            center += unit->getPosition();
            realSize++;
         }
      }
      center *= 1.0f / realSize;

      for (S32 k = 0; k < objects.size(); k++)
      {
         RTSUnit* unit = dynamic_cast<RTSUnit*>(objects[k]);

         if(!unit)
            continue;

         Point3F upos = unit->getPosition();
         VectorF offset = upos - center;
         unit->clearAim();
         unit->setMoveDestination(Point3F(pos.x + offset.x, pos.y + offset.y, 0));
      }

      //really really lame, but this keeps us from crashing on disconnect
      while (objects.size())
         objects.removeObject(objects[0]);
   }
   DECLARE_CONOBJECT(RTSUnitMoveEvent);
};

class RTSUnitAttackEvent : public NetEvent
{
public:
   SimSet* selection;
   SimSet objects;
   NetObject* victim;

   RTSUnitAttackEvent()
   {
      selection = NULL;
      objects.registerObject();
   }

   ~RTSUnitAttackEvent()
   {
      objects.unregisterObject();
   }

   void pack(NetConnection *con, BitStream *bstream)
   {
      bstream->writeInt(con->getGhostIndex(victim),NetConnection::GhostIdBitSize);

      S32 numUnits = 0;
      for (S32 k = 0; k < selection->size(); k++)
      {
         AssertFatal(dynamic_cast<NetObject*>((*selection)[k]), "Non-NetObject object in move event!");
         NetObject* obj = (NetObject*)((*selection)[k]);
         if (con->getGhostIndex(obj) != -1)
            numUnits++;
      }

      bstream->write(numUnits);

      for (S32 k = 0; k < selection->size(); k++)
      {
         NetObject* obj = (NetObject*)((*selection)[k]);
         S32 id = con->getGhostIndex(obj);
         if (id != -1)
            bstream->writeInt(id, NetConnection::GhostIdBitSize);
      }
   }
   void write(NetConnection *con, BitStream *bstream)
   {
      pack(con, bstream);
   }
   void unpack(NetConnection *con, BitStream *bstream)
   {
      S32 victimIdx = bstream->readInt(NetConnection::GhostIdBitSize);

      victim = con->resolveGhost(victimIdx);

      if(!victim)
         return;

      S32 numUnits;
      bstream->read(&numUnits);

      for (S32 k = 0; k < numUnits; k++)
      {
         S32 id = bstream->readInt(NetConnection::GhostIdBitSize);

         AssertFatal(id != -1, "RTSUnitAttackEvent::unpack - unexpected ghost ID!");

         NetObject* obj = con->resolveGhost(id);
         objects.addObject(obj);
      }
   }

   void process(NetConnection *con)
   {
      GameBase *rv = dynamic_cast<GameBase*>(victim);

      for (S32 k = 0; k < objects.size(); k++)
      {
         RTSUnit *unit = dynamic_cast<RTSUnit*>(objects[k]);
         unit->setAimObject(rv);
      }

      //really really lame, but this keeps us from crashing on disconnect
      while (objects.size())
         objects.removeObject(objects[0]);
   }
   DECLARE_CONOBJECT(RTSUnitAttackEvent);
};

IMPLEMENT_CO_CLIENTEVENT_V1(SetTeamEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(RTSUnitMoveEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(RTSUnitAttackEvent);

void RTSConnection::onDisconnect(const char *reason)
{
   if( !isServerConnection() )
   {
      gServerVisManager->handleConnectionDrop(this);
   }

   Parent::onDisconnect(reason);
}

void RTSConnection::setTeam(S32 team)
{
   if (mTeam == team)
      return;

   mTeam = team;

   if (!isServerConnection())
      postNetEvent(new SetTeamEvent(team));
}

void RTSConnection::sendMoveEvent(SimSet* moveSet, Point2F pos)
{
   if (!isServerConnection())
   {
      RTSUnitMoveEvent* event = new RTSUnitMoveEvent;
      event->pos = pos;
      event->selection = moveSet;
      postNetEvent(event);
   }
}

void RTSConnection::sendAttackEvent(SimSet* attackSet, GameBase* victim)
{
   if (!isServerConnection())
   {
      if (getGhostIndex(victim) == -1)
         return;

      RTSUnitAttackEvent* event = new RTSUnitAttackEvent;
      event->selection = attackSet;
      event->victim = victim;
      postNetEvent(event);
   }
}

void RTSConnection::setUnitVisible(RTSUnit* unit)
{
   if(!mVisibleUnits.isNull())
      mVisibleUnits->addObject(unit);
}

void RTSConnection::setUnitInvisible(RTSUnit* unit)
{
   if(!mVisibleUnits.isNull())
      mVisibleUnits->removeObject(unit);
}

bool RTSConnection::isUnitVisible(RTSUnit* unit)
{
   //search!
   if(mVisibleUnits.isNull())
      return false;

   SimSet *s = mVisibleUnits;
   for (U32 k = 0; k < s->size(); k++)
   {
      if ((*s)[k] == unit)
         return true;
   }
   return false;
}

ConsoleMethod(RTSConnection,setTeam,void,3,3,"conn.setTeam(team)"
              "Sets the team associated with this net connection")
{
   argc;
   argv;
   if(!object->isServerConnection())
      object->setTeam(dAtoi(argv[2]));
}

ConsoleMethod(RTSConnection, getTeam, S32, 2, 2, "conn.getTeam()"
              "Gets the team associated with this net connection")
{
   argc;
   argv;
   return object->getTeam();
}

ConsoleMethod( RTSConnection, sendMoveEvent, void, 4, 4, "(clientSrc, position)"
              "Sends an update to the clientDest that clientSrc has moved their units to a position")
{
   SimSet *set;

   if (!Sim::findObject(dAtoi(argv[2]), set))
   {
      Con::errorf("RTSConnection::sendMoveEvent - expected SimSet as first argument.");
      return;
   }

   Point2F pos;
   dSscanf(argv[3], "%f %f", &pos.x, &pos.y);
   object->sendMoveEvent(set, pos);
}

ConsoleMethod( RTSConnection, sendAttackEvent, void, 4, 4, "(clientSrc, victim)"
              "Sends an update that clientSrc has attacked the victim unit")
{
   SimSet *set;
   GameBase *victim;
   if (!Sim::findObject(dAtoi(argv[2]), set) || !Sim::findObject(dAtoi(argv[3]), victim))
   {
      Con::errorf("RTSConnection::sendAttackEvent - failed to find clientSrc or victim");
      return;
   }

   object->sendAttackEvent((SimSet*)set, (GameBase*)victim);
}
