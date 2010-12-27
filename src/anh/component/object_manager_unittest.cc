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

#include <gtest/gtest.h>

#include <anh/component/object_manager.h>
#include <anh/component/test_component.h>

using namespace anh::component;

#define TEST_OBJECT_ID 0xDEADBABE

/// Attach a component and see if the object now has the interface in its components list.
TEST(ObjectManagerTest, AttachComponent) {
	ObjectManager object_manager;

	object_manager.AttachComponent(TEST_OBJECT_ID, std::shared_ptr<ComponentInterface>(new ConcreteTestComponent(TEST_OBJECT_ID, false)));
	EXPECT_TRUE(object_manager.HasInterface(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent")));
	object_manager.DetachComponent(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent"));
}

/// Attach a component and then detach it to make sure it no longer has the interface in its component list.
TEST(ObjectManagerTest, DetachComponent) {
	ObjectManager object_manager;

	object_manager.AttachComponent(TEST_OBJECT_ID, std::shared_ptr<ComponentInterface>(new ConcreteTestComponent(TEST_OBJECT_ID, false)));
	object_manager.DetachComponent(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent"));
	EXPECT_FALSE(object_manager.HasInterface(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent")));
}

/// Broadcast a message to the object's components, and check for a changed value within the test component
TEST(ObjectManagerTest, BroadcastMessage) {
	ObjectManager object_manager;

	std::shared_ptr<ConcreteTestComponent> component( new ConcreteTestComponent(TEST_OBJECT_ID, false));

	object_manager.AttachComponent(TEST_OBJECT_ID, component);

	// The value is initialized to 'false' in the constructor.
	EXPECT_FALSE(component->test_value());

	object_manager.BroadcastMessage(TEST_OBJECT_ID, Message(new TestMessage()));

	// After the test message has been broadcasted, the value should be flipped to 'false'.
	EXPECT_TRUE(component->test_value());

	object_manager.DetachComponent(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent"));
}

/// Tick the ObjectManager and make sure components added to the update list are ticked as well.
TEST(ObjectManagerTest, Tick) {
	ObjectManager object_manager;

	std::shared_ptr<ConcreteTestComponent> component( new ConcreteTestComponent(TEST_OBJECT_ID, false));
	object_manager.AttachComponent(TEST_OBJECT_ID, component);

	// The value is initalized to 'false' in the constructor.
	EXPECT_FALSE(component->test_value());
	object_manager.Tick(0);

	// The value should have been toggled in the Update function.
	EXPECT_TRUE(component->test_value());
}

/// Attach a component and query for the interface.
TEST(ObjectManagerTest, QueryInterface) {
	ObjectManager object_manager;

	object_manager.AttachComponent(TEST_OBJECT_ID, std::shared_ptr<ComponentInterface>(new ConcreteTestComponent(TEST_OBJECT_ID, true)));

	std::shared_ptr<TestComponentInterface> component = object_manager.QueryInterface<TestComponentInterface>(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent"));
	EXPECT_EQ(ComponentType("ConcreteTestComponent").ident(), component->component_info().type.ident());
}

/// When a component interface is asked for that doesnt exist, a NullComponent should be returned.
TEST(ObjectManagerTest, QueryInterfaceNullComponentReturn) {
	ObjectManager object_manager;

	std::shared_ptr<TestComponentInterface> component = object_manager.QueryInterface<TestComponentInterface>(TEST_OBJECT_ID, ComponentType("ConcreteTestComponent"));
	EXPECT_EQ(ComponentType("NullTestComponent").ident(), component->component_info().type.ident());
	EXPECT_FALSE(component->test_value());
}