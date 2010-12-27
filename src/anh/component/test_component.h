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

#ifndef LIBANH_COMPONENT_TEST_COMPONENT_INTERFACE_H_
#define LIBANH_COMPONENT_TEST_COMPONENT_INTERFACE_H_

#include <anh/component/base_component.h>

using namespace anh::component;

class NullTestComponent;

class TestComponentInterface : public BaseComponent
{
public:
	TestComponentInterface(const ObjectId& id)
		: BaseComponent(id) { }

	~TestComponentInterface() { }

	virtual bool test_value() = 0;

	static std::shared_ptr<NullTestComponent> NullComponent;
};

class NullTestComponent : public TestComponentInterface
{
public:
	NullTestComponent()
		: TestComponentInterface(0) { }

	~NullTestComponent() { }

	bool Init(void) { return true; }
	void Deinit(void) { }
	void Update(const float timeout) { }
	const ComponentInfo& component_info(void) { return s_component_info_; }

	bool test_value() { return false; }

private:
	static ComponentInfo s_component_info_;
};

std::shared_ptr<NullTestComponent> TestComponentInterface::NullComponent = std::shared_ptr<NullTestComponent>(new NullTestComponent());
ComponentInfo NullTestComponent::s_component_info_ = ComponentInfo(ComponentType("NullTestComponent"), true);

//==========================================================================================

class ConcreteTestComponent : public TestComponentInterface
{
public:
	ConcreteTestComponent(const ObjectId& id, bool test_value)
		: TestComponentInterface(id)
		, test_value_(test_value) 
	{ 
		RegisterMessageHandler(MessageType("TestMessage"), std::bind(&ConcreteTestComponent::handleTestMessage, this, std::placeholders::_1));
	}

	~ConcreteTestComponent() 
	{
		UnregisterMessageHandler(MessageType("TestMessage"));
	}

	bool Init(void) { return true; }
	void Deinit(void) { }

	void Update(const float timeout) { test_value_ = !test_value_; }
	const ComponentInfo& component_info(void) { return s_component_info_; }

	bool test_value() { return test_value_; }

private:
	bool handleTestMessage(MessagePtr message) {
		test_value_ = true;
		return true;
	}

	bool test_value_;
	static ComponentInfo s_component_info_;
};

ComponentInfo ConcreteTestComponent::s_component_info_ = ComponentInfo(ComponentType("ConcreteTestComponent"), true);

//==========================================================================================

class TestMessage : public SimpleMessage
{
public:
	TestMessage()
		: SimpleMessage(MessageType("TestMessage")) { }

	~TestMessage() { }
};

#endif // LIBANH_COMPONENT_TEST_COMPONENT_INTERFACE_H_