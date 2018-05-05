/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef APPLYPOISONDARTCOMMAND_H_
#define APPLYPOISONDARTCOMMAND_H_

#include "DotPackCommand.h"

class ApplyPoisonDartCommand : public DotPackCommand {
public:

	ApplyPoisonDartCommand(const String& name, ZoneProcessServer* server)
		: DotPackCommand(name, server) {
		skillName = "applypoisondart";
		effectName = "clienteffect/throw_trap_drowsy_dart.cef";  //find a better effect
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!creature->hasSkill("combat_bountyhunter_novice")){
			creature->sendSystemMessage("You are not trained in the application of the Hssiss Venom");
			return GENERALERROR;
		}

		int cost = hasCost(creature);

		if (cost < 0)
			return INSUFFICIENTHAM;

		ManagedReference<SceneObject*> object = server->getZoneServer()->getObject(target);
		if (object == NULL || !object->isCreatureObject() || creature == object)
			return INVALIDTARGET;

		uint64 objectId = 0;

		parseModifier(arguments.toString(), objectId);
		ManagedReference<DotPack*> dotPack = NULL;

		SceneObject* inventory = creature->getSlottedObject("inventory");

		if (inventory != NULL) {
			dotPack = inventory->getContainerObject(objectId).castTo<DotPack*>();
		}

		if (dotPack == NULL)
			return GENERALERROR;

		PlayerManager* playerManager = server->getPlayerManager();
		CombatManager* combatManager = CombatManager::instance();

		CreatureObject* creatureTarget = cast<CreatureObject*>(object.get());

		if (creature != creatureTarget && !CollisionManager::checkLineOfSight(creature, creatureTarget)) {
			creature->sendSystemMessage("@healing:no_line_of_sight"); // You cannot see your target.
			return GENERALERROR;
		}
		
		if (!creatureTarget->checkCooldownRecovery("venomdart")) {
            creature->sendSystemMessage("You cannot use this ability again on that target so soon.");
            return GENERALERROR;
        }

		int	range = int(dotPack->getRange() + creature->getSkillMod("healing_range") / 100 * 14);

		if(!checkDistance(creature, creatureTarget, range))
					return TOOFAR;

			int delay = 1;
			creature->addCooldown(skillName, delay * 1000);
		

		Locker clocker(creatureTarget, creature);

		if (!combatManager->startCombat(creature, creatureTarget))
			return INVALIDTARGET;

		applyCost(creature, cost);

		int dotPower = dotPack->calculatePower(creature);
		int dotDMG = 0;

		if (dotPack->isPoisonDeliveryUnit()) {
			if (!creatureTarget->hasDotImmunity(dotPack->getDotType())) {
				StringIdChatParameter stringId("healing", "apply_poison_self");
				stringId.setTT(creatureTarget->getObjectID());

				creature->sendSystemMessage(stringId);

				StringIdChatParameter stringId2("healing", "apply_poison_other");
				stringId2.setTU(creature->getObjectID());

				creatureTarget->sendSystemMessage(stringId2);
				//put check in here so only hssiss venom works (crc lookup)
				dotDMG = creatureTarget->addDotState(creature, CreatureState::POISONED, dotPack->getServerObjectCRC(), dotPower, dotPack->getPool(), dotPack->getDuration(), dotPack->getPotency(), creatureTarget->getSkillMod("resistance_poison") + creatureTarget->getSkillMod("poison_disease_resist"));
			}

		} else {
			if (!creatureTarget->hasDotImmunity(dotPack->getDotType())) {
				StringIdChatParameter stringId("healing", "apply_disease_self");
				stringId.setTT(creatureTarget->getObjectID());

				creature->sendSystemMessage(stringId);

				StringIdChatParameter stringId2("healing", "apply_disease_other");
				stringId2.setTU(creature->getObjectID());

				creatureTarget->sendSystemMessage(stringId2);

				dotDMG = creatureTarget->addDotState(creature, CreatureState::DISEASED, dotPack->getServerObjectCRC(), dotPower, dotPack->getPool(), dotPack->getDuration(), dotPack->getPotency(), creatureTarget->getSkillMod("resistance_disease") + creatureTarget->getSkillMod("poison_disease_resist"));
			}
		}

		if (dotDMG) {
			awardXp(creature, "medical", dotDMG); //No experience for healing yourself.
			creatureTarget->getThreatMap()->addDamage(creature, dotDMG, "");
		} else {
			StringIdChatParameter stringId("dot_message", "dot_resisted");
			stringId.setTT(creatureTarget->getObjectID());

			creature->sendSystemMessage(stringId);

			StringIdChatParameter stringId2("healing", "dot_resist_other");

			creatureTarget->sendSystemMessage(stringId2);
		}

		checkForTef(creature, creatureTarget);

		if (creatureTarget->isPlayerCreature()) {
			ManagedReference<PlayerObject*> ghost = creatureTarget->getPlayerObject();

			if (ghost != NULL) {
				bool removed = false;
				
				if (creatureTarget == NULL)
					return GENERALERROR;
				
				if (creatureTarget->hasBuff(BuffCRC::JEDI_FORCE_RUN_2)) {
					creatureTarget->removeBuff(BuffCRC::JEDI_FORCE_RUN_2);
					removed = true;
				}
				if (creatureTarget->hasBuff(BuffCRC::JEDI_FORCE_RUN_1)) {
					creatureTarget->removeBuff(BuffCRC::JEDI_FORCE_RUN_1);
					removed = true;
				}
				if (removed == true){
					creatureTarget->updateCooldownTimer("venomdart", 15000);
					creature->sendSystemMessage("Your venom dart causes the target to lose focus on their seed.");
					creatureTarget->sendSystemMessage("A strange venom spreads through you like fire, momentarily interupting your connection with the force.");
				}
				
			}

		}

		if (dotPack != NULL) {
			if (creatureTarget != creature)
				clocker.release();

			Locker dlocker(dotPack, creature);
			dotPack->decreaseUseCount();
		}

		doAnimationsRange(creature, creatureTarget, dotPack->getObjectID(), creature->getWorldPosition().distanceTo(creatureTarget->getWorldPosition()), dotPack->isArea());

		creature->notifyObservers(ObserverEventType::MEDPACKUSED);

		return SUCCESS;
	}


};

#endif //APPLYPOISONDARTCOMMAND_H_
