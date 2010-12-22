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

#ifndef LIBANH_COMPONENT_BASE_COMPONENT_H_
#define LIBANH_COMPONENT_BASE_COMPONENT_H_

#include <anh/component/icomponent.h>

namespace anh {
namespace component {

class BaseComponent : public IComponent
{
public:
	/**
	 * \brief Default constructor.
	 */
	BaseComponent();
	
	/**
	 * \brief Default deconstructor.
	 */
	~BaseComponent();

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
	 * \brief
	 */
	virtual MessageResult PostMessage(const MessageType& type, Message& message) = 0;

	/**
	 * \brief
	 */
	ObjectId GetObjectId(void);

protected:
private:
	ObjectId	object_id_;

	/**
	 * \brief
	 */
	void SetObjectId_(const ObjectId& id);

	friend ObjectManager;
};

} // namespace anh
} // namespace component

#endif // LIBANH_COMPONENT_BASE_COMPONENT_H_