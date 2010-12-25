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
#include <anh/component/component_interface.h>
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
	void AttachComponent(const ObjectId& id, std::shared_ptr<ComponentInterface> component);
	
	/**
	 * \brief Detaches a component from an object by id.
	 *
	 * \param id The id of the object to detach the component from.
	 * \param type The type of component we want to detach.
	 */
	void DetachComponent(const ObjectId& id, const ComponentType& type);
	
	/**
	 * \brief Fetches an component that is attached to a object.
	 *
	 * \param id The id of the object we want to fetch the component from.
	 * \param type The type of component we want to fetch.
	 * \returns T The interface of the component we fetched.
	 */
	template<class T> std::shared_ptr<T> QueryInterface(const ObjectId& id, const ComponentType& type);

	/**
	 * \brief Looks up whether a specific component interface has been attached to an object id.
	 *
	 * \param id Id of the object to search for the attached component on.
	 * \param type The type of component we are looking for.
	 * \returns bool True if the component exists, false if the component does not exist.
	 */
	bool HasInterface(const ObjectId& id, const ComponentType& type);

	/**
	 * \brief Sends a message to each component that is attached to a specific object. When a result other than
	 * MR_IGNORE is returned by a component, the message passing loop breaks and returns the result. This means
	 * if two components handle the same message, after the first component has handled its message and returns
	 * a result other then MR_IGNORE, the second component will not get the passed message.
	 *
	 * If you want to send a message to each component without being consered about a result, see BroadcastMessage.
	 *
	 * \param object_id Id of the object to post the message to.
	 * \param type The type of message to post to each component.
	 * \param message The message to pass to the component.
	 * \returns MessageResult The result of the post operation.
	 * \see MessageResult
	 */
	MessageResult PostMessage(const ObjectId& object_id, const MessageType& type, const Message message);

	/**
	 * \brief Sends a message to each component that is attached to a specific object. This function will not break
	 * from the message passing loop if a return result other than MR_IGNORE is returned by a component. This allows
	 * you to pass messages that potentually multiple components need to handle or in which you do not care to retrieve
	 * a result.
	 *
	 * If you want a success result, see PostMessage.
	 *
	 * \param object_id Id of the object to broadcast the message to.
	 * \param type The type of message to post to each component.
	 * \param message The message to pass the component.
	 */
	void BroadcastMessage(const ObjectId& object_id, const MessageType& type, const Message message);

	/**
	 * \brief Updates components added to the per-frame update list.
	 *
	 * \param timeout The number of milliseconds between the last update.
	 */
	void Tick(const float timeout);


protected:
private:
	typedef std::map<ObjectId, std::list<std::shared_ptr<ComponentInterface>>>				ObjectComponentMap;
	typedef std::map<ObjectId, std::list<std::shared_ptr<ComponentInterface>>>::iterator	ObjectComponentMapIterator;
	typedef std::pair<ObjectId, std::list<std::shared_ptr<ComponentInterface>>>				ObjectComponentMapPair;

	typedef std::list<std::shared_ptr<ComponentInterface>>									ComponentList;
	typedef std::list<std::shared_ptr<ComponentInterface>>::iterator						ComponentListIterator;

	ObjectComponentMap	object_components_map_;
	ComponentList		update_components_;		// Components being updated every frame.

};

} // namespace component
} // namespace anh

#endif // LIBANH_COMPONENT_OBJECT_MANAGER_H_