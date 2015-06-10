#ifndef _VISMANAGER_H_
#define _VISMANAGER_H_

#include "console/simBase.h"

class RTSUnit;
class RTSConnection;

class VisManager
{
   SimSet mObjectList;

   bool mDirty;
   
public:
   VisManager() { mDirty = false; }
   void addObject(RTSUnit* unit);
   void removeObject(RTSUnit* unit);

   void processServer();
   void processClient();
   void setDirty() { mDirty = true; }
   void onConnectionSet(RTSUnit* unit, RTSConnection* conn);
   void onSetTeam(RTSUnit* unit);
   void handleConnectionDrop(RTSConnection *conn);
};


extern VisManager* gServerVisManager;
extern VisManager* gClientVisManager;

#endif

