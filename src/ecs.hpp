#pragma once

#include <unordered_map>
#include <vector>
#include <componentMap.hpp>
#include <functional>

namespace BasicECS{

    using TypeID = std::size_t;
    using EntityID = std::size_t;
    using ReferenceID = std::size_t;

    template<typename T>
    struct Reference{
        TypeID typeId;
        ReferenceID referenceId;
    };

    class ECS{
    public:
        using ParseFunc = void (*)(ECS &ecs, EntityID entity, const std::vector<uint8_t> data);
        using SerializeFunc = std::vector<uint8_t> (*)(ECS &ecs, EntityID entity);
        using InitialiseFunc = void (*)(ECS &ecs, EntityID entity);
        using CleanUpFunc = void (*)(ECS &ecs, EntityID entity);

        using RemoveComponentTypeFunc = void (*)(ECS &ecs);
        using PruneComponentTypeFunc = void (*)(ECS &ecs);
    public:
        ECS();

        void terminate();
        void saveState(const char* filePath);
        void loadState(const char* filePath);

        template <typename T> void addComponentType(InitialiseFunc initialiseFunc, CleanUpFunc cleanUpFunc);
        template <typename T> void removeComponentType();
        template <typename T> void pruneComponentType();

        ECS& addEntity(EntityID &entityID);
        ECS& removeEntity(EntityID entityID);

        template <typename T> Reference<T> createReference(EntityID entityId);
        ReferenceID getReference(EntityID entityId);

        template <typename T> ECS& addComponent(EntityID entityID, T t);
        template <typename T> ECS& addComponent(EntityID entityID, ReferenceID parentReferenceID);
        template <typename T> ECS& removeComponent(EntityID entityID);

        template <typename T> T& getComponent(EntityID entityID);
        template <typename T> T& getReferenceComponent(Reference<T> reference);

        template <typename T> void forEach(std::function<void(T &t)> routine);
        template <typename T1, typename T2> void forEach(std::function<void(T1 &t1, T2 &t2)> routine);

        void displayECS();

    private:

        struct Component {
            std::size_t componentIndex;
            EntityID parent;
        };
        struct Entity{
            ComponentMap<Component> components;
            ReferenceID referenceId;
        };
        struct ComponentType {
            void* arrayLocation;
            std::vector<EntityID> entitiesUsingThis;
            std::vector<std::size_t> tombstoneComponents;

            InitialiseFunc initialiseFunc;
            CleanUpFunc cleanUpFunc;
            ParseFunc parseFunc;
            SerializeFunc serializeFunc;
            
            RemoveComponentTypeFunc removeComponentTypeFunc;
            PruneComponentTypeFunc pruneComponentTypeFunc;

            std::string name;
        };
        struct ComponentManager{
            std::unordered_map<TypeID, ComponentType> componentTypes;
            std::unordered_map<std::string, TypeID> typeNamesToTypeIds;
        };
        struct EntityManager{
            std::vector<Entity> entities;
            std::vector<EntityID> tombstoneEntities;
            std::unordered_map<ReferenceID, EntityID> referenceToID;
        };

    private:
        ComponentType* getComponentType(TypeID typeId);
        Component* getComponent(Entity *entity, TypeID typeId);
        Entity* getEntity(EntityID entityID);

        void addEntity(EntityID &entityID, ReferenceID &referenceID);

        void removeComponent(EntityID entityID, TypeID typeId);

        void addComponent(EntityID entityID, EntityID parentEntityID, TypeID typeId);

        void runAllComponentCleanUps(ComponentType *componentType, TypeID typeId);

        void pruneEntities();

        template <typename T> TypeID getTypeId();

        void forEachEntity(std::function<void(ECS &ecs, EntityID &entity)> routine);

    private:
        EntityManager entityManager;
        ComponentManager componentManager;
    };
}

#include "ecs.tpp"