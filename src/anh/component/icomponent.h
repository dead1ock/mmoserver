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

#ifndef LIBANH_COMPONENT_ICOMPONENT_H_
#define LIBANH_COMPONENT_ICOMPONENT_H_

#include <anh/byte_buffer.h>
#include <anh/hash_string.h>

namespace anh {
namespace component {

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
class IComponent
{
public:
	typedef	unsigned long long	ObjectId;
	typedef anh::HashString		ComponentType;
	typedef anh::HashString		MessageType;
	typedef anh::ByteBuffer		Message;

	/**
	 * \breif Default constructor.
	 */
	IComponent() { }
	
	/**
	 * \breif Default destructor.
	 */
	~IComponent() { }

	/**
	 * \brief 
	 */
	virtual bool Init(void) = 0;

	/**
	 * \brief
	 */
	virtual void Deinit(void) = 0;

	/**
	 * \brief
	 */
	virtual const ComponentType& GetComponentType(void) = 0;

	/**
	 * \breif
	 */
	virtual MessageResult PostMessage(const MessageType& type, Message& message) = 0;

	/**
	 * \returns The Object Id of the entity that owns this component.
	 */
	virtual ObjectId GetObjectId(void) = 0;

};

}  // namespace component
}  // namespace anh

#endif  // LIBANH_COMPONENT_ICOMPONENT_H_
