/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef TELEPORTGROUPCOMMAND_H_
#define TELEPORTGROUPCOMMAND_H_

#include "server/zone/managers/player/PlayerManager.h"

class TeleportGroupCommand : public QueueCommand {
public:

	TeleportGroupCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		String targetName;

		try {
			UnicodeTokenizer args(arguments);
			args.getStringToken(targetName);

		} catch (Exception& e) {
			creature->sendSystemMessage("SYNTAX: /teleportTo <targetName>");
			return INVALIDPARAMETERS;
		}

		ManagedReference<PlayerManager*> playerManager = server->getPlayerManager();
		ManagedReference<CreatureObject*> targetCreature = playerManager->getPlayer(targetName);

		if (targetCreature == NULL) {
			creature->sendSystemMessage("The specified player does not exist.");
			return INVALIDTARGET;
		}

		if (targetCreature->getZone() == NULL) {
			creature->sendSystemMessage("The specified player is not in a zone that is currently loaded.");
			return INVALIDTARGET;
		}


		String zoneName = creature->getZone()->getZoneName();
		float x = creature->getPositionX();
		float y = creature->getPositionY();
		float z = creature->getPositionZ();
		uint64 parentid = creature->getParentID();

		if (targetCreature != creature) //no point in teleporting to ourselves
		targetCreature->switchZone(zoneName, x, z, y, parentid);

		ManagedReference<GroupObject*> group = targetCreature->getGroup();
		
		if (group != NULL) {
		Reference<CreatureObject*> leader = group->getLeader();

			for (int i = 0; i < group->getGroupSize(); ++i) {
				Reference<CreatureObject*> groupMember = group->getGroupMember(i);

				if (groupMember == NULL || groupMember->getObjectID() == targetCreature->getObjectID())
					continue;
					
				groupMember->switchZone(zoneName, x, z, y, parentid);
				
			}
		}
		

		return SUCCESS;
	}

};

#endif //TELEPORTGROUPCOMMAND_H_
