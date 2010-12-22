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

#ifndef LIBANH_COMPONENT_OBJECT_MANAGER_H_
#define LIBANH_COMPONENT_OBJECT_MANAGER_H_

#include <list>
#include <map>
#include <anh/component/icomponent.h>
#include <Utils/Singleton.h>

namespace anh {
namespace component {

class ObjectManager;
#define gObjectManager utils::Singleton<anh::component::ObjectManager>::Instance()

/**
 * \brief A front-end for handling the attaching, detaching and querying of object components.
 */
class ObjectManager
{
public:
	typedef std::map<IComponent::ObjectId, std::list<std::shared_ptr<IComponent>>>				ObjectComponentMap;
	typedef std::map<IComponent::ObjectId, std::list<std::shared_ptr<IComponent>>>::iterator	ObjectComponentMapIterator;

	/**
	 * \brief Default constructor.
	 */
	ObjectManager();
	
	/**
	 * \brief Default destructor.
	 */
	~ObjectManager();

	/**
	 * \brief Attaches a component to an object by id.
	 *
	 * \param id The id of the object to attach the new component to.
	 * \param component The component to attach.
	 */
	void AttachComponent(const IComponent::ObjectId& id, std::shared_ptr<IComponent> component);
	
	/**
	 * \brief Detaches a component from an object by id.
	 *
	 * \param id The id of the object to detach the component from.
	 * \param type The type of component we want to detach.
	 */
	void DetachComponent(const IComponent::ObjectId& id, const IComponent::ComponentType& type);
	
	/**
	 * \brief Fetches an component that is attached to a object.
	 *
	 * \param id The id of the object we want to fetch the component from.
	 * \param type The type of component we want to fetch.
	 * \returns T The interface of the component we fetched.
	 */
	template<class T> T QueryInterface(const IComponent::ObjectId& id, const IComponent::ComponentType& type);

	/**
	 * \brief Looks up whether a specific component interface has been attached to an object id.
	 *
	 * \param id Id of the object to search for the attached component on.
	 * \param type The type of component we are looking for.
	 * \returns bool True if the component exists, false if the component does not exist.
	 */
	bool HasInterface(const IComponent::ObjectId& id, const IComponent::ComponentType& type);

	/**
	 * \brief Sends a message to each component that is attached to a specific object.
	 *
	 * \param object_id Id of the object to post the message to.
	 * \param type The type of message to post to each component.
	 * \param message The message to pass to the component.
	 * \returns MessageResult The result of the post operation.
	 * \see MessageResult
	 */
	MessageResult PostMessage(const IComponent::ObjectId& object_id, const IComponent::MessageType& type, IComponent::Message message);

protected:
private:
	ObjectComponentMap	object_component_map_;

};

} // namespace component
} // namespace anh

#endif // LIBANH_COMPONENT_OBJECT_MANAGER_H_