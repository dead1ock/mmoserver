/*
 This file is part of MMOServer. For more information, visit http://swganh.com
 
 Copyright (c) 2006 - 2010 The SWG:ANH Team

 MMOServer is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MMOServer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MMOServer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIBANH_COMPONENT_COMPONENT_INTERFACE_H_
#define LIBANH_COMPONENT_COMPONENT_INTERFACE_H_

#include <anh/byte_buffer.h>
#include <anh/component/component_type.h>

namespace anh {
namespace component {

typedef	unsigned long long	ObjectId;
typedef anh::HashString		MessageType;
typedef anh::ByteBuffer		Message;

/**
 * Possible result types of a message handling operation.
 */
enum MessageResult
{
	MR_FALSE,
	MR_TRUE,
	MR_IGNORED,
	MR_ERROR
};

/**
 * \brief
 */
class ComponentInterface
{
public:
	/**
	 * \brief Initializes the component.
	 */
	virtual bool Init(void) = 0;

	/**
	 * \brief Releases any resources the component holds.
	 */
	virtual void Deinit(void) = 0;

	/**
	 * \brief Called every tick if the option is enabled in the ComponentType.
	 * \see ComponentType
	 */
	virtual void Update(const float timeout) = 0;

	/**
	 * \breif Handles a message that is passed to the component by the ObjectManager and then
	 * returns the result of the operation. If no operation was performed MR_IGNORED should be returned.
	 * 
	 * \param type The type of message that needs to be processed.
	 * \param message The data for the message.
	 * \returns The result of the operation, if no operation was performed MR_IGNORED should be returned.
	 */
	virtual MessageResult HandleMessage(const MessageType& type, Message message) = 0;

	/**
	 * \returns The type of component this is in the form of a hashed string.
	 * \see HashString
	 */
	virtual const ComponentType& component_type(void) = 0;

	/**
	 * \returns The Object Id of the entity that owns this component.
	 */
	virtual const ObjectId& object_id(void) = 0;

};

}  // namespace component
}  // namespace anh

#endif  // LIBANH_COMPONENT_COMPONENT_INTERFACE_H_
