#pragma once

#include <unordered_map>
#include <vector>

namespace BasicECS{

    using TypeID = std::size_t;
    using EntityID = std::size_t;

    class ECS{
    public:
        using ParseFunc = void (*)(ECS &ecs, EntityID entity, std::string line);
        using InitialiseFunc = void (*)(ECS &ecs, EntityID entity);
        using CleanUpFunc = void (*)(ECS &ecs, EntityID entity);
    public:
        ECS();

        void terminate();

        template <typename T> void addComponentType(InitialiseFunc initialiseFunc, CleanUpFunc cleanUpFunc, ParseFunc parseFunc);
        template <typename T> void removeComponentType();

        ECS& addEntity(EntityID &entityID);
        ECS& removeEntity(EntityID entityID);

        template <typename T> ECS& addComponent(EntityID entityID, T t);
        template <typename T> ECS& addComponent(EntityID entityID, EntityID parentEntityID);
        template <typename T> ECS& removeComponent(EntityID entityID);

        template <typename T> T& getComponent(EntityID entityID);

        template <typename T> void forEach(void (*routine)(T &t));
        template <typename T1, typename T2> void forEach(void (*routine)(T1 &t1, T2 &t2));

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
            std::vector<std::size_t> tombstoneComponents;

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
            std::vector<Entity> entities;
            std::vector<EntityID> tombstoneEntities;
            std::unordered_map<std::string, EntityID> entityNamesToEntityIds;
        };

    private:
        ComponentType* getComponentType(TypeID typeId);
        Component* getComponent(Entity *entity, TypeID typeId);
        Entity* getEntity(EntityID entityID);

        void removeComponent(EntityID entityID, TypeID typeId);

        void runAllComponentCleanUps(ComponentType *componentType, TypeID typeId);

        template <typename T> TypeID getTypeId();

    private:
        EntityManager entityManager;
        ComponentManager componentManager;
    };
}

#include "ecs.tpp"