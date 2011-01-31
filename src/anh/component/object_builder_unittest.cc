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
#include <anh/component/object_builder.h>
#include <anh/component/test_components_unittest.h>

using namespace anh::component;

#define TEST_OBJECT_ID 0xDEADBABE

/// When a new ObjectBuilder instance is instatiated, no templates should exist.
TEST(ObjectBuilderTest, NoTemplatesByDefault) {
	ObjectBuilder object_builder;
	EXPECT_FALSE(object_builder.TemplateExists("t21"));
}

/// When a new ObjectBuilder is initialized, all the templates in the passed in
/// directory path should be loaded and stored within the ObjectBuilder.
TEST(ObjectBuilderTest, Init) {
	ObjectBuilder object_builder;
	object_builder.Init("F:/develop/SWGANH/mmoserver/build/src/anh/Debug/templates");

	EXPECT_TRUE(object_builder.TemplateExists("t21"));
	EXPECT_TRUE(object_builder.TemplateExists("ion_rifle"));
}

/// A newly registered creator should return true.
TEST(ObjectBuilderTest, CanRegisterAndUnregisterCreator) {
	ObjectBuilder object_builder;

	object_builder.RegisterCreator("TransformComponent", [=](const ObjectId& id){ return std::shared_ptr<ComponentInterface>( new TransformComponent(id) ); });
	EXPECT_TRUE(object_builder.CreatorExists("TransformComponent"));

	object_builder.UnregisterCreator("TransformComponent");
	EXPECT_FALSE(object_builder.CreatorExists("TransformComponent"));
}

///
TEST(ObjectBuilderTest, CanRegisterAndUnregisterLoader) {
	ObjectBuilder object_builder;

	object_builder.RegisterLoader("TransformComponent", std::shared_ptr<ComponentLoaderInterface>( new TransformComponentLoader() ));
	EXPECT_TRUE(object_builder.LoaderExists("TransformComponent"));

	object_builder.UnregisterLoader("TransformComponent");
	EXPECT_FALSE(object_builder.LoaderExists("TransformComponent"));
}

/// We shouldn't be able to register two creators for a single component type.
TEST(ObjectBuilderTest, CannotRegisterTwoCreators) {
	ObjectBuilder object_builder;

	EXPECT_TRUE(object_builder.RegisterCreator("TransformComponent", [=](const ObjectId& id){ return std::shared_ptr<ComponentInterface>( new TransformComponent(id) ); }));
	EXPECT_FALSE(object_builder.RegisterCreator("TransformComponent", [=](const ObjectId& id){ return std::shared_ptr<ComponentInterface>( new TransformComponent(id) ); }));
}

/// We shouldn't be able to register two loaders for a single component type.
TEST(ObjectBuilderTest, CannotRegisterTwoLoaders) {
	ObjectBuilder object_builder;

	std::shared_ptr<ComponentLoaderInterface> loader( new TransformComponentLoader() );
	EXPECT_TRUE(object_builder.RegisterLoader("TransformComponent", loader));
	EXPECT_FALSE(object_builder.RegisterLoader("TransformComponent", loader));
}

/// Test to make sure we are not able to construct an object that doesnt have a
/// template.
TEST(ObjectBuilderTest, CannotBuildObjectWithoutTemplate) {
	ObjectBuilder object_builder;
	EXPECT_FALSE(object_builder.BuildObject(TEST_OBJECT_ID, "t21"));
}

///
TEST(ObjectBuilderTest, BuildWithMissingComponentRegistration) {
	ObjectBuilder object_builder;
	object_builder.Init("F:/develop/SWGANH/mmoserver/build/src/anh/Debug/templates");

	
}

///
TEST(ObjectBuilderTest, BuildSingleComponentObjectNoLoader) {
	ObjectBuilder object_builder;
	object_builder.Init("F:/develop/SWGANH/mmoserver/build/src/anh/Debug/templates");

	object_builder.RegisterCreator("TransformComponent", [=](const ObjectId& id){ return std::shared_ptr<ComponentInterface>( new TransformComponent(id) ); });
	EXPECT_TRUE(object_builder.BuildObject(TEST_OBJECT_ID, "t21"));

	std::shared_ptr<TransformComponentInterface> component = gObjectManager.QueryInterface<TransformComponentInterface>(TEST_OBJECT_ID, "TransformComponent");
	
	EXPECT_EQ(TEST_OBJECT_ID, component->object_id());
	EXPECT_TRUE(component->component_info().type == ComponentType("TransformComponent"));
}

//
TEST(ObjectBuilderTest, BuildMultiComponentObjectNoLoaders) {
}

//
TEST(ObjectBuilderTest, BuildSingleComponentObjectWithLoader) {
}

//
TEST(ObjectBuilderTest, BuildMultiComponentObjectWithLoaders) {
}

//
TEST(ObjectBuilderTest, BuildMultiComponentObjectWithMixedLoaders) {
}