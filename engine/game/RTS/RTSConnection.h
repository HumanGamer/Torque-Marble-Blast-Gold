#include "game/gameConnection.h"

class RTSUnit;
class RTSConnection : public GameConnection
{
   typedef GameConnection Parent;

   S32 mTeam;

   SimObjectPtr<SimSet> mVisibleUnits;

public:

   RTSConnection()
   {
      mTeam = -1;
   }

   bool onAdd()
   {
      if(!Parent::onAdd())
         return false;

      SimSet *s = new SimSet();
      s->registerObject();

      mVisibleUnits = s;

      return true;
   }

   void onRemove()
   {
      Parent::onRemove();

      if(!mVisibleUnits.isNull())
         mVisibleUnits->deleteObject();
   }


   virtual void onDisconnect(const char *reason);

   void setTeam(S32 team);
   S32 getTeam() const { return mTeam; }

   void sendMoveEvent(SimSet* moveSet, Point2F pos);
   void sendAttackEvent(SimSet* attackSet, GameBase* victim);

   void setUnitVisible(RTSUnit* unit);
   void setUnitInvisible(RTSUnit* unit);

   bool isUnitVisible(RTSUnit* unit);

   static RTSConnection *getServerConnection() { return dynamic_cast<RTSConnection*>((NetConnection *) mServerConnection); }
   static RTSConnection *getLocalClientConnection() { return dynamic_cast<RTSConnection*>((NetConnection *) mLocalClientConnection); }

   DECLARE_CONOBJECT(RTSConnection);
};

