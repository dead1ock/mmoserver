/*
---------------------------------------------------------------------------------------
This source file is part of swgANH (Star Wars Galaxies - A New Hope - Server Emulator)
For more information, see http://www.swganh.org


Copyright (c) 2006 - 2008 The swgANH Team

---------------------------------------------------------------------------------------
*/

//common includes
#include "Deed.h"
#include "HarvesterFactory.h"
#include "Heightmap.h"
#include "HarvesterObject.h"
#include "PlayerStructure.h"
#include "PlayerObject.h"
#include "Inventory.h"
#include "Item.h"
#include "ObjectController.h"
#include "ObjectControllerCommandMap.h"
#include "ObjectControllerOpcodes.h"
#include "ObjectFactory.h"
#include "PlayerObject.h"
#include "UIManager.h"
#include "StructureManager.h"
#include "WorldConfig.h"
#include "WorldManager.h"

#include "MessageLib/MessageLib.h"
#include "LogManager/LogManager.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DatabaseResult.h"
#include "DatabaseManager/DataBinding.h"
#include "Common/Message.h"
#include "Common/MessageFactory.h"


//specific includes

//======================================================================================================================
//
// Modifies the Admin List
//
void	ObjectController::_handleModifyPermissionList(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)

{
	PlayerObject*	player	= dynamic_cast<PlayerObject*>(mObject);

	if(!player)
	{
		//gMessageLib->sendSystemMessage(entertainer,L"","performance","flourish_not_performing");
		return;
	}
	
	//find out where our structure is
	string dataStr;
	gLogger->hexDump(message->getData(), message->getSize());
	message->getStringUnicode16(dataStr);
	
	string playerStr,list,action;
	
	dataStr.convert(BSTRType_ANSI);

	sscanf(dataStr.getAnsi(),"%s %s %s",playerStr.getAnsi(), list.getAnsi(), action.getAnsi());

	if(playerStr.getLength() > 40)
	{
		gMessageLib->sendSystemMessage(player,L"","player_structure","permission_40_char ");
		return;
	}

	gLogger->logMsgF(" %s %s %s",MSG_HIGH, playerStr.getAnsi(), list.getAnsi(), action.getAnsi());

	//TODO is target a structure?? used when using the commandline option
	uint64 id = player->getTargetId();
	Object* object = gWorldManager->getObjectById(id);
	PlayerStructure* structure = dynamic_cast<PlayerStructure*>(object);

	//if we have no structure that way, see whether we have a structure were we just used the adminlist
	if(!structure)
	{
		id = player->getStructurePermissionId();
		Object* object = gWorldManager->getObjectById(id);
		structure = dynamic_cast<PlayerStructure*>(object);
	}
	
	if(!structure)
	{
		gLogger->logMsgF("ObjectController::_handleModifyPermissionList No structure found :(",MSG_HIGH);
		return;
	}

	//is the structure in Range???
	float fAdminListDistance = gWorldConfig->getConfiguration("Player_Admin_List_Distance",(float)32.0);
	
	if(!player->mPosition.inRange2D(structure->mPosition,fAdminListDistance))
	{
		gMessageLib->sendSystemMessage(player,L"","player_structure","command_no_building");
		return;
	}

	player->setStructurePermissionId(0);
	

	if(action == "add")
		gStructureManager->addNametoPermissionList(structure->getId(), player->getId(), playerStr, list);

	if(action == "remove")
		gStructureManager->addNametoPermissionList(structure->getId(), player->getId(), playerStr, list);
}
//======================================================================================================================
//
// Places a structure in the game world
//

void	ObjectController::_handleStructurePlacement(uint64 targetId,Message* message,ObjectControllerCmdProperties* cmdProperties)
{
	PlayerObject*	player	= dynamic_cast<PlayerObject*>(mObject);

	// TODO - are we in structure placement mode ???
	if(!player)
	{
		//gMessageLib->sendSystemMessage(entertainer,L"","performance","flourish_not_performing");
		return;
	}
	
	//find out where our structure is
	string dataStr;
	message->getStringUnicode16(dataStr);
	
	float x,y,z,dir;
	uint64 deedId;

	swscanf(dataStr.getUnicode16(),L"%I64u %f %f %f",&deedId, &x, &z, &dir);

	gLogger->logMsgF(" ID %I64u x %f y %f dir %f",MSG_HIGH, deedId, x, z, dir);
	
	//slow query - use for building placement only
	y = Heightmap::Instance()->getHeight(x,z);
	
	//now get our deed
	Inventory* inventory = dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory));

	Deed* deed = dynamic_cast<Deed*>(inventory->getObjectById(deedId));

	//TODO
	//still have to check the region whether were allowed to build
	//still have to remove the deed out of the inventory (assign to harvester)

	switch(deed->getItemType())
	{
		case	ItemType_generator_fusion_personal:
		case	ItemType_generator_solar_personal:
		case	ItemType_generator_wind_personal:

		case	ItemType_harvester_flora_personal:
		case	ItemType_harvester_flora_heavy:
		case	ItemType_harvester_flora_medium:
		case	ItemType_harvester_gas_personal:
		case	ItemType_harvester_gas_heavy:
		case	ItemType_harvester_gas_medium:
		case	ItemType_harvester_liquid_personal:
		case	ItemType_harvester_liquid_heavy:
		case	ItemType_harvester_liquid_medium:

		case	ItemType_harvester_moisture_personal:
		case	ItemType_harvester_moisture_heavy:
		case	ItemType_harvester_moisture_medium:

		case	ItemType_harvester_ore_personal:
		case	ItemType_harvester_ore_heavy:
		case	ItemType_harvester_ore_medium:
		{
			gObjectFactory->requestnewHarvesterbyDeed(gStructureManager,deed,player->getClient(),x,y,z,dir,"",player);
		}
		break;
	}
		
	
}