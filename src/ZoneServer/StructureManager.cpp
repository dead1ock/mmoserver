 /*
---------------------------------------------------------------------------------------
This source file is part of swgANH (Star Wars Galaxies - A New Hope - Server Emulator)
For more information, see http://www.swganh.org


Copyright (c) 2006 - 2010 The swgANH Team

---------------------------------------------------------------------------------------
*/
#include "WorldConfig.h"
#include "StructureManager.h"
#include "nonPersistantObjectFactory.h"
#include "HarvesterObject.h"
#include "FactoryObject.h"
#include "Inventory.h"
#include "DataPad.h"
#include "ResourceContainer.h"
#include "ResourceType.h"
#include "ObjectFactory.h"
#include "PlayerObject.h"
#include "PlayerStructure.h"
#include "QuadTree.h"
#include "WorldManager.h"
#include "ZoneTree.h"
#include "MessageLib/MessageLib.h"

#include "LogManager/LogManager.h"
#include "DatabaseManager/Database.h"



bool						StructureManager::mInsFlag    = false;
StructureManager*			StructureManager::mSingleton  = NULL;


//======================================================================================================================

StructureManager::StructureManager(Database* database,MessageDispatch* dispatch)
{
	mBuildingFenceInterval = gWorldConfig->getConfiguration("Zone_BuildingFenceInterval",(uint16)10000);
	//uint32 structureCheckIntervall = gWorldConfig->getConfiguration("Zone_structureCheckIntervall",(uint32)3600);
	uint32 structureCheckIntervall = gWorldConfig->getConfiguration("Zone_structureCheckIntervall",(uint32)30);

	mDatabase = database;
	mMessageDispatch = dispatch;
	StructureManagerAsyncContainer* asyncContainer;

	// load our structure data
	//todo load buildings from building table and use appropriate stfs there
	//are harvesters on there too
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_LoadDeedData, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sdd.id, sdd.DeedType, sdd.SkillRequirement, s_td.object_string, s_td.lots_used, s_td.stf_name, s_td.stf_file, s_td.healing_modifier, s_td.repair_cost from swganh.structure_deed_data sdd INNER JOIN structure_type_data s_td ON sdd.id = s_td.type");

	//items
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_LoadstructureItem, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sit.structure_id, sit.cell, sit.item_type , sit.relX, sit.relY, sit.relZ, sit.dirX, sit.dirY, sit.dirZ, sit.dirW, sit.tan_type, "
													"it.object_string, it.stf_name, it.stf_file from swganh.structure_item_template sit INNER JOIN item_types it ON (it.id = sit.item_type) WHERE sit.tan_type = %u",TanGroup_Item);

	//statics
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_LoadstructureItem, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sit.structure_id, sit.cell, sit.item_type , sit.relX, sit.relY, sit.relZ, sit.dirX, sit.dirY, sit.dirZ, sit.dirW, sit.tan_type,  "
													"st.object_string, st.name, st.file from swganh.structure_item_template sit INNER JOIN static_types st ON (st.id = sit.item_type) WHERE sit.tan_type = %u",TanGroup_Static);


	//terminals
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_LoadstructureItem, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sit.structure_id, sit.cell, sit.item_type , sit.relX, sit.relY, sit.relZ, sit.dirX, sit.dirY, sit.dirZ, sit.dirW, sit.tan_type,  "
													"tt.object_string, tt.name, tt.file from swganh.structure_item_template sit INNER JOIN terminal_types tt ON (tt.id = sit.item_type) WHERE sit.tan_type = %u",TanGroup_Terminal);


	//=========================
	//check regularly the harvesters - they might have been turned off by the db, harvesters without condition might need to be deleted
	//do so every hour if no other timeframe is set
	gWorldManager->getPlayerScheduler()->addTask(fastdelegate::MakeDelegate(this,&StructureManager::_handleStructureDBCheck),7,structureCheckIntervall*1000,NULL);
}


//======================================================================================================================
StructureManager::~StructureManager()
{
	mInsFlag = false;
	delete(mSingleton);

}
//======================================================================================================================
StructureManager*	StructureManager::Init(Database* database, MessageDispatch* dispatch)
{
	if(!mInsFlag)
	{
		mSingleton = new StructureManager(database,dispatch);
		mInsFlag = true;
		return mSingleton;
	}
	else
		return mSingleton;

}

//======================================================================================================================

void StructureManager::Shutdown()
{

}




//=======================================================================================================================
//checks for a name on a permission list
//=======================================================================================================================
void StructureManager::checkNameOnPermissionList(uint64 structureId, uint64 playerId, string name, string list, StructureAsyncCommand command)
{

	StructureManagerAsyncContainer* asyncContainer;

	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_Check_Permission, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"select sf_CheckPermissionList(%I64u,'%s','%s')",structureId,name.getAnsi(),list.getAnsi());
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;
	asyncContainer->command = command;
	sprintf(asyncContainer->name,"%s",name.getAnsi());


	// 0 is Name on list
	// 1 name doesnt exist
	// 2 name not on list
	// 3 Owner 
}


//=======================================================================================================================
//removes a name from a permission list
//=======================================================================================================================
void StructureManager::removeNamefromPermissionList(uint64 structureId, uint64 playerId, string name, string list)
{

	StructureManagerAsyncContainer* asyncContainer;

	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_Remove_Permission, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"select sf_RemovePermissionList(%I64u,'%s','%s')",structureId,name.getAnsi(),list.getAnsi());
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;
	sprintf(asyncContainer->name,"%s",name.getAnsi());


	// 0 is sucess
	// 1 name doesnt exist
	// 2 name not on list
	// 3 Owner cannot be removed
}



//=======================================================================================================================
//adds a name to a permission list
//=======================================================================================================================
void StructureManager::addNametoPermissionList(uint64 structureId, uint64 playerId, string name, string list)
{
	// load our structures maintenance data
	// that means the maintenance attribute and the energy attribute
	//

	StructureManagerAsyncContainer* asyncContainer;

	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_Add_Permission, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"select sf_AddPermissionList(%"PRIu64",'%s','%s')",structureId,name.getAnsi(),list.getAnsi());
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;
	sprintf(asyncContainer->name,"%s",name.getAnsi());


	// 0 is sucess
	// 1 name doesnt exist
	// 2 name already on list

//mDatabase->ExecuteSqlAsync(0,0,"UPDATE item_attributes SET value='%.2f' WHERE item_id=%"PRIu64" AND attribute_id=%u",attValue,mItem->getId(),att->getAttributeId());
}



//=======================================================================================================================
//handles callbacks of db creation of items
//=======================================================================================================================
void StructureManager::getDeleteStructureMaintenanceData(uint64 structureId, uint64 playerId)
{
	// load our structures maintenance data
	// that means the maintenance attribute and the energy attribute

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateAttributes, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,
		"		(SELECT \'power\'		, sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 384)"
		"UNION	(SELECT \'maintenance\'	, sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 382)"
														 " ",structureId, structureId);

	
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;
	asyncContainer->command.Command = Structure_Command_Destroy;

	//382 = examine_maintenance
	//384 = examine_power
}


//======================================================================================================================
//looks up the data for a specific deed
//======================================================================================================================

StructureDeedLink* StructureManager::getDeedData(uint32 type)
{
	DeedLinkList::iterator it = mDeedLinkList.begin();
	//bool found = false;
	while(it != mDeedLinkList.end())
	{
		if ((*it)->item_type == type )
		{
			return (*it);
		}
		it++;
	}

	return NULL;
}

//======================================================================================================================
//returns true when we are NOT within 25m of a camp
//======================================================================================================================

bool StructureManager::checkCampRadius(PlayerObject* player)
{
	QTRegion*			mQTRegion = NULL;
	uint32				subZoneId = player->getSubZoneId();
	float				width  = 25.0;
	float				height = 25.0;

	Anh_Math::Rectangle mQueryRect;
	if(!subZoneId)
	{
		mQTRegion	= gWorldManager->getSI()->getQTRegion(player->mPosition.mX,player->mPosition.mZ);
		subZoneId	= (uint32)mQTRegion->getId();
		mQueryRect	= Anh_Math::Rectangle(player->mPosition.mX - width,player->mPosition.mZ - height,width * 2,height * 2);
	}

	RegionObject*	object;
	ObjectSet		objList;

	gWorldManager->getSI()->getObjectsInRange(player,&objList,ObjType_Region,width*2);

	if(mQTRegion)
	{
		mQTRegion->mTree->getObjectsInRange(player,&objList,ObjType_Region,&mQueryRect);
	}

	ObjectSet::iterator objIt = objList.begin();

	while(objIt != objList.end())
	{
		object = (RegionObject*)(*objIt);

		if(object->getRegionType() == Region_Camp)
		{
			return false;
		}

		++objIt;
	}

	return true;

}

//======================================================================================================================
//returns true when we are NOT within 5m of a city
//======================================================================================================================

bool StructureManager::checkCityRadius(PlayerObject* player)
{
	QTRegion*			mQTRegion = NULL;
	uint32				subZoneId = player->getSubZoneId();
	float				width  = 5.0;
	float				height = 5.0;

	Anh_Math::Rectangle mQueryRect;
	if(!subZoneId)
	{
		mQTRegion	= gWorldManager->getSI()->getQTRegion(player->mPosition.mX,player->mPosition.mZ);
		subZoneId	= (uint32)mQTRegion->getId();
		mQueryRect	= Anh_Math::Rectangle(player->mPosition.mX - width,player->mPosition.mZ - height,width * 2,height * 2);
	}

	RegionObject*	object;
	ObjectSet		objList;

	gWorldManager->getSI()->getObjectsInRangeIntersection(player,&objList,ObjType_Region,width*2);

	if(mQTRegion)
	{
		mQTRegion->mTree->getObjectsInRange(player,&objList,ObjType_Region,&mQueryRect);
	}

	ObjectSet::iterator objIt = objList.begin();

	while(objIt != objList.end())
	{
		object = (RegionObject*)(*objIt);

		if(object->getRegionType() == Region_City)
		{
			return false;
		}

		++objIt;
	}

	return true;

}

//======================================================================================================================
//returns true when we are within 1m of a camp
//======================================================================================================================

bool StructureManager::checkinCamp(PlayerObject* player)
{
	QTRegion*			mQTRegion = NULL;
	uint32				subZoneId = player->getSubZoneId();
	float				width  = 1.0;
	float				height = 1.0;

	Anh_Math::Rectangle mQueryRect;
	if(!subZoneId)
	{
		mQTRegion	= gWorldManager->getSI()->getQTRegion(player->mPosition.mX,player->mPosition.mZ);
		subZoneId	= (uint32)mQTRegion->getId();
		mQueryRect	= Anh_Math::Rectangle(player->mPosition.mX - width,player->mPosition.mZ - height,width * 2,height * 2);
	}

	RegionObject*	object;
	ObjectSet		objList;

	gWorldManager->getSI()->getObjectsInRange(player,&objList,ObjType_Region,width*2);

	if(mQTRegion)
	{
		mQTRegion->mTree->getObjectsInRange(player,&objList,ObjType_Region,&mQueryRect);
	}

	ObjectSet::iterator objIt = objList.begin();

	while(objIt != objList.end())
	{
		object = (RegionObject*)(*objIt);

		if(object->getRegionType() == Region_Camp)
		{
			return true;
		}

		++objIt;
	}

	return false;

}


//=========================================================================================0
// gets the code to confirm structure destruction
//
string StructureManager::getCode()
{
	int8	serial[12],chance[9];
	bool	found = false;
	uint8	u;

	for(uint8 i = 0; i < 6; i++)
	{
		while(!found)
		{
			found = true;
			u = static_cast<uint8>(static_cast<double>(gRandom->getRand()) / (RAND_MAX + 1.0f) * (122.0f - 48.0f) + 48.0f);

			//only 1 to 9 or a to z
			if(u >57)
				found = false;

			if(u < 48)
				found = false;

		}
		chance[i] = u;
		found = false;
	}
	chance[6] = 0;

	sprintf(serial,"%s",chance);

	return(BString(serial));
}

//======================================================================================================================
//
// Handle deletion of destroyed Structures / building fences and other stuff
//

bool StructureManager::_handleStructureObjectTimers(uint64 callTime, void* ref)
{
	//iterate through all harvesters to delete
	ObjectIDList* objectList = gStructureManager->getStrucureDeleteList();
	ObjectIDList::iterator it = objectList->begin();

	while(it != objectList->end())
	{
		PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById((*it)));

		if(!structure)
		{
			gLogger->logMsg("StructureManager::_handleStructureObjectTimers: No structure");
			it = objectList->erase(it);
			continue;
		}

		if(structure->getTTS()->todo == ttE_UpdateHopper)
		{

			if(Anh_Utils::Clock::getSingleton()->getLocalTime() < structure->getTTS()->projectedTime)
			{
				gLogger->logMsg("StructureManager::_handleStructureObjectTimers: intervall to short - delayed");
				break;
			}

			PlayerObject* player = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById( structure->getTTS()->playerId ));
			if(!player)
			{
				it = objectList->erase(it);
				continue;
			}

			HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(structure);

			if(!harvester)
			{
				gLogger->logMsg("StructureManager::_handleStructureObjectTimers: No structure");
				it = objectList->erase(it);
				continue;
			}

			// TODO
			// read the current resource hopper contents
			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_HopperUpdate,player->getClient());

			asyncContainer->mStructureId	= structure->getId();
			asyncContainer->mPlayerId		= player->getId();
			
			int8 sql[250];
			sprintf(sql,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",asyncContainer->mStructureId);
			
			mDatabase->ExecuteSqlAsync(harvester,asyncContainer,sql);	

			//is the structure in Range??? - otherwise stop updating
			float fTransferDistance = gWorldConfig->getConfiguration("Player_Structure_Operate_Distance",(float)20.0);
			if(player->mPosition.inRange2D(structure->mPosition,fTransferDistance))
			{
				structure->getTTS()->projectedTime = Anh_Utils::Clock::getSingleton()->getLocalTime() + 5000;
				addStructureforHopperUpdate(structure->getId());
			}
		

		}

		if(structure->getTTS()->todo == ttE_Delete)
		{
			PlayerObject* player = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById( structure->getTTS()->playerId ));
			if(structure->canRedeed())
			{	
				Inventory* inventory	= dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory));
				if(!inventory->checkSlots(1))
				{
					gMessageLib->sendSystemMessage(player,L"","player_structure","inventory_full");
					continue;
				}

				gMessageLib->sendSystemMessage(player,L"","player_structure","deed_reclaimed");

				//update the deeds attributes and set the new owner id (owners inventory = characterid +1)
				StructureManagerAsyncContainer* asyncContainer;
				asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateStructureDeed, 0);
				asyncContainer->mPlayerId		= structure->getOwner();
				asyncContainer->mStructureId	= structure->getId();
				int8 sql[150];
				sprintf(sql,"select sf_DefaultHarvesterUpdateDeed(%"PRIu64",%"PRIu64")", structure->getId(),structure->getOwner()+1);
				mDatabase->ExecuteSqlAsync(this,asyncContainer,sql);

			}
			else
			//delete the deed
			{
				gMessageLib->sendSystemMessage(player,L"","player_structure","structure_destroyed");
				int8 sql[200];
				sprintf(sql,"DELETE FROM items WHERE parent_id = %"PRIu64" AND item_family = 15",structure->getId());
				mDatabase->ExecuteSqlAsync(NULL,NULL,sql);
				gObjectFactory->deleteObjectFromDB(structure);
				gMessageLib->sendDestroyObject_InRangeofObject(structure);
				gWorldManager->destroyObject(structure);
				UpdateCharacterLots(structure->getOwner());
			}

			

		}

		if(structure->getTTS()->todo == ttE_BuildingFence)
		{
			if(Anh_Utils::Clock::getSingleton()->getLocalTime() < structure->getTTS()->projectedTime)
			{
				gLogger->logMsg("StructureManager::_handleStructureObjectTimers: intervall to short - delayed");
				break;
			}

			//gLogger->logMsg("StructureManager::_handleStructureObjectTimers: building fence");

			PlayerObject* player = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById( structure->getTTS()->playerId ));
			PlayerStructure* fence = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(structure->getTTS()->buildingFence));

			if(!player)
			{
				gLogger->logMsg("StructureManager::_handleStructureObjectTimers: No Player");
				gMessageLib->sendDestroyObject_InRangeofObject(fence);
				gWorldManager->destroyObject(fence);
				gWorldManager->handleObjectReady(structure,player->getClient());
				it = objectList->erase(it);
				continue;

				return false;
			}

			if(!fence)
			{
				gLogger->logMsg("StructureManager::_handleStructureObjectTimers: No fence");
				it = objectList->erase(it);
				continue;
				return false;
			}

			//delete the fence
			gMessageLib->sendDestroyObject_InRangeofObject(fence);
			gWorldManager->destroyObject(fence);

			PlayerObjectSet*			inRangePlayers	= player->getKnownPlayers();
			PlayerObjectSet::iterator	it				= inRangePlayers->begin();
			while(it != inRangePlayers->end())
			{
				PlayerObject* targetObject = (*it);
				gMessageLib->sendCreateStructure(structure,targetObject);
				targetObject->addKnownObjectSafe(structure);
				structure->addKnownObjectSafe(targetObject);
				++it;
			}

			gMessageLib->sendCreateStructure(structure,player);
			player->addKnownObjectSafe(structure);
			structure->addKnownObjectSafe(player);
			gMessageLib->sendDataTransform(structure);

			gMessageLib->sendConstructionComplete(player,structure);
			

		}


		it = objectList->erase(it);
	
	}

	return (false);
}


//=======================================================================================================================
//handles callback of altering the hopper list
//

void StructureManager::OpenStructureHopperList(uint64 structureId, uint64 playerId)
{
	// load our structures Admin data
	//
	HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(gWorldManager->getObjectById(structureId));
	if(!harvester)
	{
		gLogger->logMsgF("OpenStructureHopperList : No harvester :(",MSG_HIGH);
		return;
	}

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_Hopper_Data, 0);
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;

	mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT c.firstname FROM structure_admin_data sad  INNER JOIN characters c ON (sad.PlayerID = c.ID)where sad.StructureID = %I64u AND sad.AdminType like 'HOPPER'",structureId);

}

//=======================================================================================================================
//handles callback of altering the admin list
//

void StructureManager::OpenStructureAdminList(uint64 structureId, uint64 playerId)
{
	// load our structures Admin data
	//

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_Query_Admin_Data, 0);
	asyncContainer->mStructureId = structureId;
	asyncContainer->mPlayerId = playerId;

	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT c.firstname FROM structure_admin_data sad  INNER JOIN characters c ON (sad.PlayerID = c.ID)where sad.StructureID = %I64u AND sad.AdminType like 'ADMIN'",structureId);


}

//=======================================================================================================================
//processes a succesfull PermissionList verification
//=======================================================================================================================

void StructureManager::processVerification(StructureAsyncCommand command, bool owner)
{

	PlayerObject* player = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(command.PlayerId));

	if(!player)
	{
		gLogger->logMsg("StructureManager::processVerification : No Player");
		return;
	}

	switch(command.Command)
	{
		case Structure_Command_AccessOutHopper:
		{
		 	FactoryObject* factory = dynamic_cast<FactoryObject*>(gWorldManager->getObjectById(command.StructureId));
			if(!factory)
			{
				gLogger->logMsg("StructureManager::processVerification : No Factory (Structure_Command_AccessInHopper) ");
				return;
			}

			//send the hopper as tangible if we havnt done that already ...
			//add each other to the known objects list
			Item* outHopper = dynamic_cast<Item*>(gWorldManager->getObjectById(factory->getOutputHopper()));
			if(!outHopper)
			{
				gLogger->logMsg("StructureManager::processVerification : No outHopper (Structure_Command_AccessInHopper) ");
				return;
			}
			//are we already known ???
			if(outHopper->checkKnownObjects(player))
			{
				gLogger->logMsg("OutHopper already known ");
				gMessageLib->sendOpenedContainer(outHopper->getId(),player);
				return;
			}

			//no create
			gMessageLib->sendCreateObject(outHopper,player,false);
			outHopper->addKnownObjectSafe(player);
			player->addKnownObjectSafe(outHopper);
			//now add the hoppers content - update the objects location to the factory location so the destroy works


			gMessageLib->sendOpenedContainer(outHopper->getId(),player);

		}
		break;

		case Structure_Command_AccessInHopper:
		{
			FactoryObject* factory = dynamic_cast<FactoryObject*>(gWorldManager->getObjectById(command.StructureId));
			if(!factory)
			{
				gLogger->logMsg("StructureManager::processVerification : No Factory (Structure_Command_AccessInHopper) ");
				return;
			}

			//send the hopper as tangible if we havnt done that already ...
			//add each other to the known objects list
			Item* inHopper = dynamic_cast<Item*>(gWorldManager->getObjectById(factory->getIngredientHopper()));
			if(!inHopper)
			{
				gLogger->logMsg("StructureManager::processVerification : No inHopper (Structure_Command_AccessInHopper) ");
				return;
			}
			//are we already known ???
			if(inHopper->checkKnownObjects(player))
			{
				gLogger->logMsg("InHopper already known ");
				gMessageLib->sendOpenedContainer(inHopper->getId(),player);
				return;
			}

			//no create
			gMessageLib->sendCreateObject(inHopper,player,false);
			inHopper->addKnownObjectSafe(player);
			player->addKnownObjectSafe(inHopper);
			//now add the hoppers content - update the objects location to the factory location so the destroy works


			gMessageLib->sendOpenedContainer(inHopper->getId(),player);

		}
		break;

		case Structure_Command_RemoveSchem:
		{
			FactoryObject* factory = dynamic_cast<FactoryObject*>(gWorldManager->getObjectById(command.StructureId));
			if(!factory)
			{
				gLogger->logMsg("StructureManager::processVerification : No Factory (Structure_Command_AddSchem) ");
				return;
			}

			//do we have a schematic that needs to be put back into the inventory???

			if(!factory->getManSchemID())
			{
				//nothing to do for us
				return;
			}

			//return the old schematic to the Datapad
			Datapad*	datapad		= dynamic_cast<Datapad*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Datapad));
			//Inventory*	inventory	= dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory));
			
			if(!datapad->getCapacity())
			{
				gMessageLib->sendSystemMessage(player,L"","manf_station","schematic_not_removed");
				return;
			}
			
			//change the ManSchems Owner ID and load it into the datapad
			gObjectFactory->requestTanoNewParent(datapad,factory->getManSchemID() ,datapad->getId(),TanGroup_ManufacturingSchematic);
			mDatabase->ExecuteSqlAsync(0,0,"UPDATE factories SET ManSchematicID = 0 WHERE ID = %I64u",command.StructureId);

			//finally reset the schem ID in the factory
			factory->setManSchemID(0);
			gMessageLib->sendSystemMessage(player,L"","manf_station","schematic_removed");
			
		}
		break;

		case Structure_Command_AddSchem:
		{
			FactoryObject* factory = dynamic_cast<FactoryObject*>(gWorldManager->getObjectById(command.StructureId));
			if(!factory)
			{
				gMessageLib->sendSystemMessage(player,L"","manf_station","schematic_not_added");
				gLogger->logMsg("StructureManager::processVerification : No Factory (Structure_Command_AddSchem) ");
				return;
			}

			//do we have a schematic that needs to be put back into the inventory???

			if(factory->getManSchemID())
			{
				if(factory->getManSchemID() == command.SchematicId)
				{
					//nothing to do for us
					return;
				}

				//first return the old schematic to the Datapad
				Datapad*	datapad		= dynamic_cast<Datapad*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Datapad));
				//Inventory*	inventory	= dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory));
				
				//change the ManSchems Owner ID and load it into the datapad
				gObjectFactory->requestTanoNewParent(datapad,factory->getManSchemID() ,datapad->getId(),TanGroup_ManufacturingSchematic);

				//get the new Manufacturing schematic
				datapad->removeManufacturingSchematic(command.SchematicId);
		
			}

			factory->setManSchemID(command.SchematicId);
			
			//link the schematic to the factory in the db
			mDatabase->ExecuteSqlAsync(0,0,"UPDATE factories SET ManSchematicID = %I64u WHERE ID = %I64u",command.SchematicId,command.StructureId);
			mDatabase->ExecuteSqlAsync(0,0,"UPDATE items SET parent_id = %I64u WHERE ID = %I64u",command.StructureId,command.SchematicId);
			
			//remove the schematic from the player
			PlayerObject* player	= dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(command.PlayerId));
			Datapad* datapad		= dynamic_cast<Datapad*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Datapad));
			datapad->removeManufacturingSchematic(command.SchematicId);
			gMessageLib->sendDestroyObject(command.SchematicId,player);

			gMessageLib->sendSystemMessage(player,L"","manf_station","schematic_added");

		}
		break;

		case Structure_Command_AccessSchem:
		{
			PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateAttributes,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command			= command;
			//mDatabase->ExecuteSqlAsync(structure,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());
			mDatabase->ExecuteSqlAsync(this,asyncContainer,
				"		(SELECT \'schematicCustom\', i.customName FROM factories f INNER JOIN items i ON (i.id = f.ManSchematicID) WHERE f.ID = %I64u)"
				 "UNION (SELECT \'schematicName\', it.stf_name FROM factories f INNER JOIN items i ON (i.id = f.ManSchematicID) INNER JOIN item_types it ON (i.item_type = it.id) WHERE f.ID = %I64u)"
				 "UNION (SELECT \'schematicFile\', it.stf_file FROM factories f INNER JOIN items i ON (i.id = f.ManSchematicID) INNER JOIN item_types it ON (i.item_type = it.id) WHERE f.ID = %I64u)"
				,command.StructureId);
			
		}
		break;

		case Structure_Command_ViewStatus:
		{
			PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));
			
			//the structure might have been deleted between the last and the current refresh
			if(!structure)
			{
				gMessageLib->sendSystemMessage(player,L"","player_structure","no_valid_structurestatus");
				return;
			}
			if(player->getTargetId() != structure->getId())
			{
				gMessageLib->sendSystemMessage(player,L"","player_structure","changed_structurestatus");
				return;
			}

			//read the relevant attributes in then display the status page
			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateAttributes,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command			= command;
			
			mDatabase->ExecuteSqlAsync(this,asyncContainer,
				"(SELECT \'name\', c.firstname  FROM characters c WHERE c.id = %"PRIu64")"
				"UNION (SELECT \'power\', sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 384)"
				"UNION (SELECT \'maintenance\', sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 382)"
														 " ",structure->getOwner(),command.StructureId,command.StructureId);


			
		}
		break;

		case Structure_Command_DepositPower:
		{
			PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateAttributes,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command			= command;
			//mDatabase->ExecuteSqlAsync(structure,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());
			mDatabase->ExecuteSqlAsync(this,asyncContainer,
				"(SELECT \'power\', sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 384)"
				,structure->getId());
		}
		break;

		case Structure_Command_PayMaintenance:
		{
			PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateAttributes,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command			= command;
			//mDatabase->ExecuteSqlAsync(structure,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());
			mDatabase->ExecuteSqlAsync(this,asyncContainer,"(SELECT \'maintenance\', sa.value FROM structure_attributes sa WHERE sa.structure_id = %"PRIu64" AND sa.attribute_id = 382)"
														 " UNION (SELECT \'condition\', s.condition FROM structures s WHERE s.id = %"PRIu64")",structure->getId(),structure->getId());

		}
		break;

		// callback for retrieving a variable amount of the selected resource
		case Structure_Command_RetrieveResource:
		{
			HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(gWorldManager->getObjectById(command.StructureId));

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_ResourceRetrieve,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command 		= command;
	
			//mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());
			mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT sf_DiscardResource(%"PRIu64",%"PRIu64",%u) ",harvester->getId(),command.ResourceId,command.Amount);

		}
		break;

		// callback for discarding a variable amount of the selected resource
		case Structure_Command_DiscardResource:
		{
			HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(gWorldManager->getObjectById(command.StructureId));

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_ResourceDiscard,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			asyncContainer->command 		= command;
			
			//mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());
			mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT sf_DiscardResource(%"PRIu64",%"PRIu64",%u) ",harvester->getId(),command.ResourceId,command.Amount);

		}
		break;

		case Structure_Command_GetResourceData:
		{
			HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(gWorldManager->getObjectById(command.StructureId));

			if(!harvester)
				return;

			StructureManagerAsyncContainer* asyncContainer = new StructureManagerAsyncContainer(Structure_GetResourceData,player->getClient());
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"SELECT hr.resourceID, hr.quantity FROM harvester_resources hr WHERE hr.ID = '%"PRIu64"' ",harvester->getId());

		}
		break;

		case Structure_Command_DiscardHopper:
		{
			//send the db update
			StructureManagerAsyncContainer* asyncContainer;

			HarvesterObject* harvester = dynamic_cast<HarvesterObject*>(gWorldManager->getObjectById(command.StructureId));

			asyncContainer = new StructureManagerAsyncContainer(Structure_HopperDiscard, 0);
			asyncContainer->mStructureId	= command.StructureId;
			asyncContainer->mPlayerId		= command.PlayerId;
			mDatabase->ExecuteSqlAsync(harvester,asyncContainer,"select sf_DiscardHopper(%I64u)",command.StructureId);

		}
		break;	 

		case Structure_Command_OperateHarvester:
		{
			PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));
			gMessageLib->sendOperateHarvester(structure,player);
		}
		return;

		case Structure_Command_RenameStructure:
		{
			if(owner)
			{
				PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));
				createRenameStructureBox(player, structure);
			}
			else
				gMessageLib->sendSystemMessage(player,L"","player_structure","rename_must_be_owner ");

			
		}
		return;

		case Structure_Command_TransferStructure:
		{
			if(owner)
				gStructureManager->TransferStructureOwnership(command);
			else
				gMessageLib->sendSystemMessage(player,L"You cannot transfer ownership of this structure");
			
		}
		return;

		case Structure_Command_Destroy: 
		{		
			if(owner)
				gStructureManager->getDeleteStructureMaintenanceData(command.StructureId, command.PlayerId);
			else
				gMessageLib->sendSystemMessage(player,L"","player_structure","destroy_must_be_owner");
			
			
		}
		break;

		case Structure_Command_PermissionAdmin:
		{
			player->setStructurePermissionId(command.StructureId);
			OpenStructureAdminList(command.StructureId, command.PlayerId);

		}
		break;

		case Structure_Command_PermissionHopper:
		{
			player->setStructurePermissionId(command.StructureId);
			OpenStructureHopperList(command.StructureId, command.PlayerId);

		}
		break;

		case Structure_Command_AddPermission:
		{
		
			addNametoPermissionList(command.StructureId, command.PlayerId, command.PlayerStr, command.List);

		}
		break;

		case Structure_Command_RemovePermission:
		{

			removeNamefromPermissionList(command.StructureId, command.PlayerId, command.PlayerStr, command.List);
	
		}
	
	}
}


//=======================================================================================================================
//processes a structure transfer
//=======================================================================================================================

void StructureManager::TransferStructureOwnership(StructureAsyncCommand command)
{
	//at this point we have already made sure that the command is issued by the owner
	PlayerStructure* structure = dynamic_cast<PlayerStructure*>(gWorldManager->getObjectById(command.StructureId));

	//if we have no structure that way, see whether we have a structure were we just used the adminlist
	if(!structure)
	{
		gLogger->logMsgF("StructureManager::TransferStructureOwnership structure not found :(",MSG_HIGH);	
		return;
	}
	
	//step 1 make sure the recipient has enough free lots!

	//step 1a -> get the recipients ID

	//the recipient MUST be online !!!!! ???????

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_StructureTransfer_Lots_Recipient, 0);
	asyncContainer->mStructureId = command.StructureId;
	asyncContainer->mPlayerId = command.PlayerId;
	asyncContainer->mTargetId = command.RecipientId;

	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sf_getLotCount(%I64u)",command.PlayerId);
}

uint32 StructureManager::getCurrentPower(PlayerObject* player)
{
	ObjectList*	invObjects = dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory))->getObjects();
	ObjectList::iterator listIt = invObjects->begin();

	uint32 power = 0;

	while(listIt != invObjects->end())
	{
		// we are looking for resource containers
		if(ResourceContainer* resCont = dynamic_cast<ResourceContainer*>(*listIt))
		{
			uint16 category = resCont->getResource()->getType()->getCategoryId();
			
			gLogger->logMsgF("StructureManager::getCurrentPower() category : %u",MSG_NORMAL, category);
			if(category == 475 || category == 476||category == 477||((category >= 618)&&category <=651 )||category ==903||category == 904 )
			{
				float pe = resCont->getResource()->getAttribute(ResAttr_PE);//7
				
				// thats actually not the classic way in precu energy was received on a
				// 1::1 basis if pe was < 500
				uint32 containerPower = (uint32)(resCont->getAmount()* (pe/500));
				power += containerPower;
			}

			
		}

		++listIt;
	}

	return power;
}

uint32 StructureManager::deductPower(PlayerObject* player, uint32 amount)
{
	ObjectList*	invObjects = dynamic_cast<Inventory*>(player->getEquipManager()->getEquippedObject(CreatureEquipSlot_Inventory))->getObjects();
	ObjectList::iterator listIt = invObjects->begin();

	uint32 power = 0;

	while(listIt != invObjects->end())
	{
		// we are looking for resource containers
		if(ResourceContainer* resCont = dynamic_cast<ResourceContainer*>(*listIt))
		{
			uint16 category = resCont->getResource()->getType()->getCategoryId();
			
			gLogger->logMsgF("StructureManager::getCurrentPower() category : %u",MSG_NORMAL, category);
			if(category == 475 || category == 476||category == 477||((category >= 618)&&category <=651 )||category ==903||category == 904 )
			{
				float pe = resCont->getResource()->getAttribute(ResAttr_PE);//7
				
				// thats actually not the classic way in precu energy was received on a
				// 1::1 basis if pe was < 500
				uint32 containerPower = (uint32)(resCont->getAmount()* (pe/500));
				
				uint32 tdAmount = amount;
				if(tdAmount >containerPower)
					tdAmount = containerPower;

				
				uint32 todelete = (uint32)(tdAmount /(pe/500));
				uint32 newAmount = resCont->getAmount()-todelete;
				if(newAmount <0)
				{
					assert(false);
				}
				
				resCont->setAmount(newAmount);
				gMessageLib->sendResourceContainerUpdateAmount(resCont,player);
				mDatabase->ExecuteSqlAsync(NULL,NULL,"UPDATE resource_containers SET amount=%u WHERE id=%"PRIu64"",newAmount,resCont->getId());				

				
				amount -= tdAmount;
			}

			
		}

		++listIt;
	}

	if(amount>0)
	{
		gLogger->logMsgF("StructureManager::deductPower() couldnt deduct the entire amount !!!!!",MSG_NORMAL);
	}
	return (!amount);
}


//======================================================================================================================
//
// Handle deletion of destroyed Structures / building fences and other stuff
//

bool StructureManager::_handleStructureDBCheck(uint64 callTime, void* ref)
{
	//iterate through all harvesters which are marked inactive in the db

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_GetInactiveHarvesters, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT h.ID, s.condition FROM harvesters h INNER JOIN structures s ON (h.ID = s.ID) WHERE active = 0 AND s.zone = %u", gWorldManager->getZoneId());

	asyncContainer = new StructureManagerAsyncContainer(Structure_GetDestructionStructures, 0);
	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT h.ID, s.condition FROM harvesters h INNER JOIN structures s ON (h.ID = s.ID) WHERE active = 0 AND( s.condition >= 1000) AND s.zone = %u", gWorldManager->getZoneId());

	return (true);
}


//==========================================================================================0
//asynchronously updates the lot count of a player
void StructureManager::UpdateCharacterLots(uint64 charId)
{
	PlayerObject* player = dynamic_cast<PlayerObject*>(gWorldManager->getObjectById(charId));

	if(!player)
		return;

	StructureManagerAsyncContainer* asyncContainer;
	asyncContainer = new StructureManagerAsyncContainer(Structure_UpdateCharacterLots, 0);
	asyncContainer->mPlayerId = charId;

	mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sf_getLotCount(%I64u)",charId);
}