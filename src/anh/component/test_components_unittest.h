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

#ifndef LIBANH_COMPONENT_TEST_COMPONENTS_H_
#define LIBANH_COMPONENT_TEST_COMPONENTS_H_

#include <anh/component/base_component.h>
#include <anh/component/component_loader_interface.h>

using namespace anh::component;

//================================================================
// Transform Component
//================================================================

struct Vector3
{
	Vector3(float _x = 0, float _y = 0, float _z = 0)
		: x(_x)
		, y(_y)
		, z(_z) { }

	float x, y, z;
};

struct Quaternion
{
	Quaternion(float _x = 0, float _y = 0, float _z = 0, float _w = 0)
		: x(_x)
		, y(_y)
		, z(_z)
		, w(_w) { }

	float x, y, z, w;
};

class NullTransformComponent;

class TransformComponentInterface : public BaseComponent
{
public:
	TransformComponentInterface(const ObjectId& id)
		: BaseComponent(id) { }

	virtual Vector3& position() = 0;
	virtual Quaternion& rotation() = 0;

	static std::shared_ptr<NullTransformComponent> NullComponent;
};

class NullTransformComponent : public TransformComponentInterface
{
public:
	NullTransformComponent()
		: TransformComponentInterface(0) { }

	Vector3& position() { return position_; }
	Quaternion& rotation() { return rotation_; }

	const ComponentInfo& component_info() { return component_info_; }

private:
	Vector3 position_;
	Quaternion rotation_;
	static ComponentInfo component_info_;
};

class TransformComponent : public TransformComponentInterface
{
public:
	TransformComponent(const ObjectId& id)
		: TransformComponentInterface(id) { }

	void Init(boost::property_tree::ptree& pt) {
		position_.x = pt.get<float>("position.x", 0.0f);
		position_.y = pt.get<float>("position.y", 0.0f);
		position_.z = pt.get<float>("position.z", 0.0f);
		rotation_.x = pt.get<float>("rotation.x", 0.0f);
		rotation_.y = pt.get<float>("rotation.y", 0.0f);
		rotation_.z = pt.get<float>("rotation.z", 0.0f);
		rotation_.w = pt.get<float>("rotation.w", 1.0f);
	}

	Vector3& position() { return position_; }
	Quaternion& rotation() { return rotation_; }

	const ComponentInfo& component_info() { return component_info_; }

private:
	Vector3 position_;
	Quaternion rotation_;
	static ComponentInfo component_info_;
};

class TransformComponentLoader : public ComponentLoaderInterface
{
public:
	bool Load(std::shared_ptr<ComponentInterface> comp) {
		std::shared_ptr<TransformComponentInterface> component = std::dynamic_pointer_cast<TransformComponentInterface>(comp);
		component->position().x = 100;
		component->position().y = 48;
		component->position().z = 3485;

		return true;
	}

	void Unload(std::shared_ptr<ComponentInterface> comp) {
	}
};

//================================================================
// Radial Component
//================================================================

class RadialComponentInterface : public BaseComponent
{
};

class NullRadialComponent : public RadialComponentInterface
{
};

class RadialComponent : public RadialComponentInterface
{
};

//================================================================
// Appearance Component
//================================================================

class AppearanceComponentInterface : public BaseComponent
{
};

class NullAppearanceComponent : public AppearanceComponentInterface
{
};

class AppearanceComponent : public AppearanceComponentInterface
{
};

//================================================================
// Tickable Component
//================================================================

class NullTickableComponent;

class TickableComponentInterface : public BaseComponent
{
public:
	TickableComponentInterface(const ObjectId& id)
		: BaseComponent(id) { }

	virtual bool ticked() = 0;
	static std::shared_ptr<NullTickableComponent> NullComponent;
};

class NullTickableComponent : public TickableComponentInterface
{
public:
	NullTickableComponent()
		: TickableComponentInterface(0) { }

	bool ticked() { return false; }
	const ComponentInfo& component_info() { return s_component_info_; }

private:
	static ComponentInfo s_component_info_;
};

class TickableComponent : public TickableComponentInterface
{
public:
	TickableComponent(const ObjectId& id)
		: TickableComponentInterface(id)
		, ticked_(false) { }

	bool ticked() { return ticked_; }
	void Update(const float timeout) { ticked_ = true; }
	const ComponentInfo& component_info() { return s_component_info_; }

private:
	bool ticked_;
	static ComponentInfo s_component_info_;
};

//================================================================
// HAM Component
//================================================================

class DoDamageMessage : public SimpleMessage
{
public:
	DoDamageMessage(uint8_t bar, uint32_t amount)
		: SimpleMessage(MessageType("DoDamage"), 0)
		, bar_(bar)
		, amount_(amount) { }

	const uint8_t bar() const { return bar_; }
	const uint32_t amount() const { return amount_; }

private:
	uint8_t bar_;
	uint32_t amount_;
};

struct HAM
{
	unsigned int health;
	unsigned int max_health;
	unsigned int action;
	unsigned int max_action;
	unsigned int mind;
	unsigned int max_mind;
};

class NullHAMComponent;

class HAMComponentInterface : public BaseComponent
{
public:
	HAMComponentInterface(const ObjectId& id)
		: BaseComponent(id) { }

	virtual HAM& ham() = 0;

	static std::shared_ptr<NullHAMComponent> NullComponent;
};

class NullHAMComponent : public HAMComponentInterface
{
public:
	NullHAMComponent()
		: HAMComponentInterface(0) { }

	HAM& ham() { return ham_; }
	const ComponentInfo& component_info() { return component_info_; }

private:
	HAM ham_;
	static ComponentInfo component_info_;
};

class HAMComponent : public HAMComponentInterface
{
public:
	HAMComponent(const ObjectId& id)
		: HAMComponentInterface(id) 
	{ 
		RegisterMessageHandler(MessageType("DoDamage"), std::bind(&HAMComponent::handleDoDamage, this, std::placeholders::_1));
	}

	~HAMComponent()
	{
		UnregisterMessageHandler(MessageType("DoDamage"));
	}

	void Init(boost::property_tree::ptree pt)
	{
		ham_.max_health = pt.get<unsigned int>("health", 1);
		ham_.health = pt.get<unsigned int>("health", 1);
		ham_.max_action = pt.get<unsigned int>("action", 1);
		ham_.action = pt.get<unsigned int>("action", 1);
		ham_.max_mind = pt.get<unsigned int>("mind", 1);
		ham_.mind = pt.get<unsigned int>("mind", 1);
	}

	HAM& ham() { return ham_; }
	const ComponentInfo& component_info() { return component_info_; }

private:
	HAM ham_;
	static ComponentInfo component_info_;

	bool handleDoDamage(const Message m)
	{
		std::shared_ptr<DoDamageMessage> message = std::dynamic_pointer_cast<DoDamageMessage>(m);

		switch(message->bar())
		{
		case 1:
			ham_.health -= message->amount();
			break;

		case 2:
			ham_.action -= message->amount();
			break;

		case 3:
			ham_.mind -= message->amount();
			break;
		}

		return true;
	}
};

#endif // LIBANH_COMPONENT_TEST_COMPONENTS_H_