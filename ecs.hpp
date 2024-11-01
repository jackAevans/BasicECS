#pragma once

#include <unordered_map>
#include <vector>

using TypeID = std::size_t;
using EntityID = uint32_t;

namespace BasicECS{

    class ECS{
    private:
        using ParseFunc = void (*)(ECS &ecs, EntityID entity, std::string line);
        using InitialiseFunc = void (*)(ECS &ecs, EntityID entity);
        using CleanUpFunc = void (*)(ECS &ecs, EntityID entity);
    public:

        template <typename T> void addComponentType(InitialiseFunc initialiseFunc, CleanUpFunc cleanUpFunc, ParseFunc parseFunc);
        template <typename T> void removeComponentType();

        ECS& addEntity(EntityID &entityID);
        ECS& removeEntity(EntityID entityID);

        template <typename T> ECS& addComponent(EntityID entityID, T t);
        template <typename T> ECS& addComponent(EntityID entityID, EntityID parentEntityID);
        template <typename T> ECS& removeComponent(EntityID entityID);

        template <typename T> T& getComponent(EntityID entityID);

        void displayECS();

    private:

        struct Component {
            std::size_t componentIndex;
            EntityID parent;
        };
        struct Entity{
            std::unordered_map<TypeID, Component> components;
            std::string name;
        };
        struct ComponentType {
            void* arrayLocation;
            std::vector<EntityID> entitiesUsingThis;
            std::vector<std::size_t> removedComponentIndices;

            InitialiseFunc initialiseFunc;
            CleanUpFunc cleanUpFunc;
            ParseFunc parseFunc;

            std::string name;
        };
        struct ComponentManager{
            std::unordered_map<TypeID, ComponentType> componentTypes;
            std::unordered_map<std::string, TypeID> typeNamesToTypeIds;
        };
        struct EntityManager{
            std::unordered_map<EntityID, Entity> entities;
            std::unordered_map<std::string, EntityID> entityNamesToEntityIds;
        };

    private:
        template <typename T> ComponentType* getComponentType(TypeID typeId);
        template <typename T> Component* getComponent(Entity *entity, TypeID typeId);
        Entity* getEntity(EntityID entityID);

        template <typename T> TypeID getTypeId();
    private:
        EntityManager entityManager;
        ComponentManager componentManager;
    };
}

#include "ecs.tpp"