/*
---------------------------------------------------------------------------------------
This source file is part of SWG:ANH (Star Wars Galaxies - A New Hope - Server Emulator)

For more information, visit http://www.swganh.com

Copyright (c) 2006 - 2010 The SWG:ANH Team
---------------------------------------------------------------------------------------
Use of this source code is governed by the GPL v3 license that can be found
in the COPYING file or at http://www.gnu.org/licenses/gpl-3.0.html

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
---------------------------------------------------------------------------------------
*/

#include "CharacterAdminHandler.h"
#include "ChatOpcodes.h"

#include "Common/LogManager.h"

#include "DatabaseManager/DataBinding.h"
#include "DatabaseManager/Database.h"
#include "DatabaseManager/DatabaseResult.h"

#include "NetworkManager/DispatchClient.h"
#include "NetworkManager/Message.h"
#include "NetworkManager/MessageDispatch.h"
#include "NetworkManager/MessageFactory.h"

#include "Utils/utils.h"

#include <boost/lexical_cast.hpp>

#include <string>
#include <cassert>
#include <cstdlib>
#include <cstring>

//======================================================================================================================
CharacterAdminHandler::CharacterAdminHandler(Database* database, MessageDispatch* dispatch)
{
  // Store our members
  mDatabase = database;
  mMessageDispatch = dispatch;

  // Register our opcodes
  mMessageDispatch->RegisterMessageCallback(opClientCreateCharacter,std::bind(&CharacterAdminHandler::_processCreateCharacter, this, std::placeholders::_1, std::placeholders::_2));
  //mMessageDispatch->RegisterMessageCallback(opLagRequest,this);
  mMessageDispatch->RegisterMessageCallback(opClientRandomNameRequest,std::bind(&CharacterAdminHandler::_processRandomNameRequest, this, std::placeholders::_1, std::placeholders::_2));

  // Load anything we need from the database
}


//======================================================================================================================
CharacterAdminHandler::~CharacterAdminHandler(void)
{
  // Unregister our callbacks
  mMessageDispatch->UnregisterMessageCallback(opClientCreateCharacter);
  mMessageDispatch->UnregisterMessageCallback(opLagRequest);
  mMessageDispatch->UnregisterMessageCallback(opClientRandomNameRequest);
}

//======================================================================================================================
void CharacterAdminHandler::Process(void)
{

}

//======================================================================================================================
void CharacterAdminHandler::_processRandomNameRequest(Message* message, DispatchClient* client)
{
    if(!client)
    {
        return;
    }

  Message* newMessage;

  // If it's not, validate it to the client.
  gMessageFactory->StartMessage();
  gMessageFactory->addUint32(opHeartBeat);
  newMessage = gMessageFactory->EndMessage();

  // Send our message to the client.
  client->SendChannelAUnreliable(newMessage, client->getAccountId(), CR_Client, 1);

  // set our object type string.
  BString objectType;
  message->getStringAnsi(objectType);

  CAAsyncContainer* asyncContainer = new CAAsyncContainer(CAQuery_RequestName,client);
  asyncContainer->mObjBaseType = objectType.getAnsi();
  mDatabase->ExecuteSqlAsync(this,asyncContainer,"SELECT sf_CharacterNameCreate(\'%s\')",objectType.getAnsi());
}


//======================================================================================================================
void CharacterAdminHandler::_processCreateCharacter(Message* message, DispatchClient* client)
{
  CharacterCreateInfo characterInfo;
  memset(&characterInfo, 0, sizeof(characterInfo));

  // Using a string as a buffer object for the appearance data.  The string class can parse the message format
  _parseAppearanceData(message, &characterInfo);

  BString characterName;
  characterName.setType(BSTRType_Unicode16);

  message->getStringUnicode16(characterName);

  uint16* space = reinterpret_cast<uint16*>(wcschr(reinterpret_cast<wchar_t*>(characterName.getRawData()),' '));

  // If there is no last name, there is no space.
  if (space)
  {
    // Get our first name
    *space = 0;
    uint16 len = (uint16)wcslen(reinterpret_cast<wchar_t*>(characterName.getRawData()));
    characterInfo.mFirstName.setType(BSTRType_Unicode16);
    characterInfo.mFirstName.setLength(len);
    wcscpy(reinterpret_cast<wchar_t*>(characterInfo.mFirstName.getRawData()), reinterpret_cast<wchar_t*>(characterName.getRawData()));

    // Get our last name
    *space = ' ';
    len = (uint16)wcslen(reinterpret_cast<wchar_t*>(space) + 1);
    characterInfo.mLastName.setType(BSTRType_Unicode16);
    characterInfo.mLastName.setLength(len);
    wcscpy(reinterpret_cast<wchar_t*>(characterInfo.mLastName.getRawData()), reinterpret_cast<wchar_t*>(space) + 1);
  }
  else
  {
    // We only have one name
    characterInfo.mFirstName = characterName;
    characterInfo.mLastName.setType(BSTRType_ANSI);
  }

  // we must have a firstname
  if(characterInfo.mFirstName.getLength() < 3)
  {
      // characterInfo.mFirstName.convert(BSTRType_ANSI);
    _sendCreateCharacterFailed(1,client);
    return;
  }
  // we dont want large names
  else if(characterInfo.mFirstName.getLength() > 16)
  {
      // characterInfo.mFirstName.convert(BSTRType_ANSI);
      _sendCreateCharacterFailed(11,client);
      return;
  }

  // check the name further, allow no numbers,only 3 special signs
  // other verifications are done via the database
  uint8 specialCount = 0;
  BString checkName;
  bool needsEscape = false;
  checkName.setType(BSTRType_Unicode16);

  uint16 len = (uint16)wcslen(reinterpret_cast<wchar_t*>(characterName.getRawData()));
  characterInfo.mFirstName.setType(BSTRType_Unicode16);
  characterInfo.mFirstName.setLength(len);
  wcscpy(reinterpret_cast<wchar_t*>(checkName.getRawData()),reinterpret_cast<wchar_t*>(characterName.getRawData()));
  checkName.convert(BSTRType_ANSI);
  int8* check = checkName.getAnsi();

  while(*check)
  {
    if(!((*check >= 'A' && *check <= 'Z') || (*check >= 'a' && *check <= 'z')))
    {
        if(*check == ' ')
        {
            check++;
            continue;
        }
        else if(*check == '\'' || *check == '-')
        {
            needsEscape = true;

            if(++specialCount > 2)
            {
                gLogger->log(LogManager::DEBUG,"specialCount > 2 in name");
                _sendCreateCharacterFailed(10,client);
                return;
            }
            else
            {
                check++;
                continue;
            }

        }
        else
        {
            gLogger->log(LogManager::DEBUG,"Invalid chars in name");
            _sendCreateCharacterFailed(10,client);
            return;
        }
    }
    check++;
  }

  // Base character model
  message->getStringAnsi(characterInfo.mBaseModel);
  //uint32 crc = characterInfo.mBaseModel.getCrc();

  // Starting city
  message->getStringAnsi(characterInfo.mStartCity);

  // Hair model
  message->getStringAnsi(characterInfo.mHairModel);

  // hair customization
    _parseHairData(message, &characterInfo);

  // Character profession
  message->getStringAnsi(characterInfo.mProfession);

  // Unknown attribute
  /* uint8 unkown = */message->getUint8();

  // Character height
  characterInfo.mScale = message->getFloat();

  // Character biography (Optional);
  uint32 bioSize = message->peekUint32();
  if(bioSize)
  {
    message->getStringUnicode16(characterInfo.mBiography);
  }
  else
  {
      message->getUint32();
      characterInfo.mBiography.setType(BSTRType_ANSI);
  }

  uint8 tutorialFlag = message->getUint8();

  if(tutorialFlag)
      characterInfo.mStartCity = "tutorial";
  else
      characterInfo.mStartCity = "default_location";

  // Setup our statement
  int8 sql[4096];

  // Convert our name strings to UTF8 so they can be put in the DB.
  characterInfo.mFirstName.convert(BSTRType_ANSI);
  characterInfo.mBiography.convert(BSTRType_ANSI);

  if(needsEscape)
      characterInfo.mFirstName = strRep(std::string(characterInfo.mFirstName.getAnsi()),"'","''").c_str();

  if(characterInfo.mLastName.getType() != BSTRType_ANSI && characterInfo.mLastName.getLength())
  {
    characterInfo.mLastName.convert(BSTRType_ANSI);

    if(needsEscape)
        characterInfo.mLastName = strRep(std::string(characterInfo.mLastName.getAnsi()),"'","''").c_str();

    // Build our procedure call
    sprintf(sql, "CALL sp_CharacterCreate(%"PRIu32", 2,'%s','%s', '%s', '%s', %f",
        client->getAccountId(),
        characterInfo.mFirstName.getAnsi(),
        characterInfo.mLastName.getAnsi(),
        characterInfo.mProfession.getAnsi(),
        characterInfo.mStartCity.getAnsi(),
        characterInfo.mScale);
  }
  else
  {
      sprintf(sql, "CALL sp_CharacterCreate(%"PRIu32", 2, '%s',NULL , '%s', '%s', %f",
          client->getAccountId(),
          characterInfo.mFirstName.getAnsi(),
          characterInfo.mProfession.getAnsi(),
          characterInfo.mStartCity.getAnsi(),
          characterInfo.mScale);
  }

  int8 sql2[4096];

  if(bioSize)
  {
      characterInfo.mBiography = strRep(std::string(characterInfo.mBiography.getAnsi()),"'","''").c_str();
      characterInfo.mBiography = strRep(std::string(characterInfo.mBiography.getAnsi()),"\"","\"\"").c_str();
      characterInfo.mBiography = strRep(std::string(characterInfo.mBiography.getAnsi()),"\\","\\\\").c_str();

      sprintf(sql2,",'%s'",characterInfo.mBiography.getAnsi());
      strcat(sql,sql2);
  }
  else
      strcat(sql,",NULL");

  // Iterate our appearance and append as necessary.
  for (uint32 i = 0; i < 0x71; i++)
  {
    strcat(sql, ", ");
        strcat(sql, boost::lexical_cast<std::string>(characterInfo.mAppearance[i]).c_str());
  }
  strcat(sql, ", ");
    strcat(sql, boost::lexical_cast<std::string>(characterInfo.mAppearance[171]).c_str());
  strcat(sql, ", ");
    strcat(sql, boost::lexical_cast<std::string>(characterInfo.mAppearance[172]).c_str());

  if(characterInfo.mHairModel.getLength())
  {
      sprintf(sql2,",'%s',%u,%u", characterInfo.mHairModel.getAnsi(),characterInfo.mHairCustomization[1],characterInfo.mHairCustomization[2]);
  }

  else
  {
    sprintf(sql2,",NULL,0,0");

  }

  int8 sql3[128];

  sprintf(sql3,",'%s');",characterInfo.mBaseModel.getAnsi());


  strcat(sql2,sql3);
  strcat(sql, sql2);

  //Logging the character create sql for debugging purposes,beware this contains binary data
  gLogger->log(LogManager::DEBUG,"SQL :: %s", sql);

  CAAsyncContainer* asyncContainer = new CAAsyncContainer(CAQuery_CreateCharacter,client);
  mDatabase->ExecuteProcedureAsync(this, asyncContainer, sql);
}

//======================================================================================================================

void CharacterAdminHandler::handleDatabaseJobComplete(void* ref,DatabaseResult* result)
{
    CAAsyncContainer* asyncContainer = reinterpret_cast<CAAsyncContainer*>(ref);
    switch(asyncContainer->mQueryType)
    {
        case CAQuery_CreateCharacter:
        {
            DataBinding* binding = mDatabase->CreateDataBinding(1);
            binding->addField(DFT_uint64,0,8);

            uint64 queryResult;
            result->GetNextRow(binding,&queryResult);

            if(queryResult >= 0x0000000200000000ULL)
            {
                _sendCreateCharacterSuccess(queryResult,asyncContainer->mClient);
            }
            else
            {
                _sendCreateCharacterFailed(static_cast<uint32>(queryResult),asyncContainer->mClient);
            }

            mDatabase->DestroyDataBinding(binding);
        }
        break;

        case CAQuery_RequestName:
        {
            Message* newMessage;

            BString randomName,ui,state;
            ui = "ui";
            state = "name_approved";

            DataBinding* binding = mDatabase->CreateDataBinding(1);
            binding->addField(DFT_bstring,0,64);
            result->GetNextRow(binding,&randomName);
            randomName.convert(BSTRType_Unicode16);

            gMessageFactory->StartMessage();
            gMessageFactory->addUint32(opClientRandomNameResponse);
            gMessageFactory->addString(asyncContainer->mObjBaseType);
            gMessageFactory->addString(randomName);
            gMessageFactory->addString(ui);
            gMessageFactory->addUint32(0);
            gMessageFactory->addString(state);
            newMessage = gMessageFactory->EndMessage();

            asyncContainer->mClient->SendChannelA(newMessage, asyncContainer->mClient->getAccountId(), CR_Client, 4);

            mDatabase->DestroyDataBinding(binding);
        }
        break;

        default:break;
    }
    SAFE_DELETE(asyncContainer);
}

//======================================================================================================================

void CharacterAdminHandler::_parseHairData(Message* message, CharacterCreateInfo* info)
{
    info->mHairCustomization[1]= 0;
    info->mHairCustomization[2]= 0;

      // Get the size of the data block
      uint16 dataSize = message->getUint16();
      gLogger->log(LogManager::DEBUG,"datasize : %u ", dataSize);

      uint8 startindex = 0;
      uint8 endindex = 0;

      if(dataSize!= 0)
      {
          uint16  dataIndex = 0;
          if(dataSize!= 5)
          {
              //cave the trandoshan - there we have no index!!!

              startindex = message->getUint8();
              endindex = message->getUint8();
              gLogger->log(LogManager::DEBUG,"StartIndex : %u   : EndIndex %u", startindex, endindex);
              dataIndex = 2;
          }

          uint8 index =0;


          uint8   attributeIndex = 0, valueLowByte = 0, valueHighByte = 0;

          while (dataIndex != (dataSize -2))
          {
              index++;
            // reset our locals
            attributeIndex = 0;
            valueLowByte = 0;
            valueHighByte = 0;

            // get our attribute index
            attributeIndex = message->getUint8();
            dataIndex++;

            // get our first byte of the value
            valueLowByte = message->getUint8();
            dataIndex++;

            // If it's an 0xff, get the next byte.
            if (valueLowByte == 0xFF)
            {
              valueHighByte = message->getUint8();
              dataIndex++;
            }

            // Set our attribute value
            info->mHairCustomization[attributeIndex] = ((uint16)valueHighByte << 8) | valueLowByte;
            gLogger->log(LogManager::DEBUG,"Hair Customization Index : %u   : data %u", attributeIndex,info->mHairCustomization[attributeIndex]);
          }

          /* uint16 end2  = */message->getUint16();
      }
}

//======================================================================================================================

void CharacterAdminHandler::_parseAppearanceData(Message* message, CharacterCreateInfo* info)
{
  // Get the size of the data block
  uint16 dataSize = message->getUint16();

  // Grab our first two bytes which are unknown at this point
  //byte 1 = start index; byte 2 = end index
  /*uint8		startindex  = */message->getUint8();
  /*uint8		endindex	= */message->getUint8();
  uint8		boobJob		= message->peekUint8();
  uint16	dataIndex	= 2;
  uint8		index		= 0;
  uint8		attributeIndex = 0, valueLowByte = 0, valueHighByte = 0;

  if(boobJob==171)
  {
    valueLowByte = 0;
    valueHighByte = 0;
    //field > 73 means 2 attributes after index in this case breasts
    boobJob	= message->getUint8();
    dataIndex++;

    valueLowByte = message->getUint8();
    dataIndex++;

    // If it's an 0xff, get the next byte.
    if (valueLowByte == 0xFF)
    {
      valueHighByte = message->getUint8();
      dataIndex++;
    }

    // Set our attribute value
    info->mAppearance[171] = ((uint16)valueHighByte << 8) | valueLowByte;

    valueLowByte = 0;
    valueHighByte = 0;

    valueLowByte = message->getUint8();
    dataIndex++;

    // If it's an 0xff, get the next byte.
    if (valueLowByte == 0xFF)
    {
      valueHighByte = message->getUint8();
      dataIndex++;
    }

    // Set our attribute value
    info->mAppearance[172] = ((uint16)valueHighByte << 8) | valueLowByte;

  }

  //dont interpret end header as data
  while (dataIndex != (dataSize-2))
  {
      index++;
    // reset our locals
    attributeIndex = 0;
    valueLowByte = 0;
    valueHighByte = 0;

    // get our attribute index
    attributeIndex = message->getUint8();
    dataIndex++;

    // get our first byte of the value
    valueLowByte = message->getUint8();
    dataIndex++;

    // If it's an 0xff, get the next byte.
    if (valueLowByte == 0xFF)
    {
      valueHighByte = message->getUint8();
      dataIndex++;
    }

    // Set our attribute value
    info->mAppearance[attributeIndex] = ((uint16)valueHighByte << 8) | valueLowByte;
  }

  //end is ff 03 - no data
  /* uint16 end  = */message->getUint16();
}

//======================================================================================================================

void CharacterAdminHandler::_sendCreateCharacterSuccess(uint64 characterId,DispatchClient* client)
{
    if(!client)
    {
        return;
    }

    gMessageFactory->StartMessage();
    gMessageFactory->addUint32(opHeartBeat);
    Message* newMessage = gMessageFactory->EndMessage();

    client->SendChannelAUnreliable(newMessage, client->getAccountId(), CR_Client, 1);

    gMessageFactory->StartMessage();
    gMessageFactory->addUint32(opClientCreateCharacterSuccess);
    gMessageFactory->addUint64(characterId);
    newMessage = gMessageFactory->EndMessage();

    client->SendChannelA(newMessage, client->getAccountId(), CR_Client, 2);
}

//======================================================================================================================

void CharacterAdminHandler::_sendCreateCharacterFailed(uint32 errorCode,DispatchClient* client)
{
	if(!client)
	{
		return;
	}

	BString unknown = L"o_O";
	BString stfFile = "ui";
	BString errorString;

	switch(errorCode)
	{
	case 0:
		errorString = "name_declined_developer";
		break;

	case 1:
		errorString = "name_declined_empty";
		break;

	case 2:
		errorString = "name_declined_fictionally_reserved";
		break;

	case 3:
		errorString = "name_declined_in_use";
		break;

	case 4:
		errorString = "name_declined_internal_error";
		break;

	case 5:
		errorString = "name_declined_no_name_generator";
		break;

	case 6:
		errorString = "name_declined_no_template";
		break;

	case 7:
		errorString = "name_declined_not_authorized_for_species";
		break;

	case 8:
		errorString = "name_declined_not_creature_template";
		break;

	case 9:
		errorString = "name_declined_not_authorized_for_species";
		break;

	case 10:
		errorString = "name_declined_number";
		break;

	case 11:
		errorString = "name_declined_profane";
		break;

	case 12:
		errorString = "name_declined_racially_inappropriate";
		break;

	case 13:
		errorString = "name_declined_reserved";
		break;

	case 14:
		errorString = "name_declined_retry";
		break;

	case 15:
		errorString = "name_declined_syntax";
		break;

	case 16:
		errorString = "name_declined_too_fast";
		break;

	case 17:
		errorString = "name_declined_cant_create_avatar";
		break;

	default:
		errorString = "name_declined_internal_error";
		gLogger->log(LogManager::DEBUG,"CharacterAdminHandler::_sendCreateCharacterFailed Unknown Errorcode in CharacterCreation: %u", errorCode);
		break;
	}

	gMessageFactory->StartMessage();
	gMessageFactory->addUint32(opHeartBeat);
	Message* newMessage = gMessageFactory->EndMessage();

	client->SendChannelAUnreliable(newMessage, client->getAccountId(), CR_Client, 1);

	gMessageFactory->StartMessage();
	gMessageFactory->addUint32(opClientCreateCharacterFailed);
	gMessageFactory->addString(unknown);
	gMessageFactory->addString(stfFile);
	gMessageFactory->addUint32(0);            // unknown
	gMessageFactory->addString(errorString);
	newMessage = gMessageFactory->EndMessage();

	client->SendChannelA(newMessage, client->getAccountId(), CR_Client, 3);
}

//======================================================================================================================

