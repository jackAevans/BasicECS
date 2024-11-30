#pragma once

#include <componentMap.hpp>

#include <unordered_map>
#include <vector>
#include <functional>

namespace BasicECS{

    class ECS;

    using TypeID = std::size_t;
    using EntityID = std::size_t;
    using EntityGUID = uint64_t;

    constexpr std::size_t RootEntityID = -1;

    using InitialiseFunc = void (*)(ECS &ecs, EntityID entity);
    using DeinitializeFunc = void (*)(ECS &ecs, EntityID entity);
    using SerializeFunc = std::vector<uint8_t> (*)(ECS &ecs, EntityID entity);
    using DeserializeFunc = void (*)(ECS &ecs, EntityID entity, const std::vector<uint8_t> data);

    struct ComponentFunctions{
        InitialiseFunc initialiseFunc = nullptr;
        DeinitializeFunc deinitializeFunc = nullptr;
        SerializeFunc serializeFunc = nullptr;
        DeserializeFunc deserializeFunc = nullptr;
    };

    template<typename T>
    struct Reference{
        TypeID typeId;
        EntityGUID entityGUID;
    };

    class ECS{
    public:
        /**
         * @brief Terminates the ecs 
         */
        ~ECS();

        /**
         * @brief Clears the ecs of all entities and components 
         */
        void clear();

        /**
         * @brief Adds a new component type to the ecs 
         * @tparam T Component type to add
         * @param initialiseFunc The fuction that gets called when this component is added  
         * @param deinitializeFunc The fuction that gets called when this component is removed  
         */
        template <typename T> void addComponentType(ComponentFunctions componentFunctions);
        /**
         * @brief Removes a component type from the ecs 
         * @tparam T Component type to remove
         */
        template <typename T> void removeComponentType();

        /**
         * @brief Adds a new entity to the ecs (caches entity)
         * @param entityID A reference to the new entityId 
         * @return A reference to the ecs
         */
        ECS& addEntity(EntityID &entityID);
        /**
         * @brief Adds a new entity to the ecs (caches entity)
         * @param entityID A reference to the new entityId 
         * @param entityGUID A globally unique ID 
         * @return A reference to the ecs
         */
        ECS& addEntity(EntityID &entityID, EntityGUID entityGUID);
        /**
         * @brief Adds a new entity to the ecs (caches entity)
         * @return A reference to the ecs
         */
        ECS& addEntity();
        /**
         * @brief Removes an entity from the ecs 
         * @param entityID The id of the entity to remove
         * @return A reference to the ecs
         */
        ECS& removeEntity(EntityID entityID);

        /**
         * @brief Append a child entity to an entity
         * @param entityID The entity to append the child entity to 
         * @param childEntityID The child entity to append to the entity
         * @return A reference to the ecs
         */
        ECS& appendChild(EntityID entityID, EntityID childEntityID);
        /**
         * @brief Gets the parent entity ID from an entity
         * @param entityID The entity to get the parent entity ID from
         * @return The entity ID of the parent
         */
        EntityID getParentEntityID(EntityID entityID);
        /**
         * @brief Gets the child entity IDs from an entity
         * @param entityID The entity to get the child entity IDs from
         * @return A list of the child entity IDs
         */
        std::vector<EntityID> getChildEntityIDs(EntityID entityID);
        
        /**
         * @brief Creates a reference of a component 
         * @tparam T Component type 
         * @param entityID The entity with the component to reference 
         * @return A reference of the type T
         */
        template <typename T> Reference<T> createReference(EntityID entityID);

        /**
         * @brief Get the globally unique ID of an entity
         * @param entityID The ID of the entity to get the GUID from 
         * @return The GUID of the entity
         */
        EntityGUID getEntityGUID(EntityID entityID);
        /**
         * @brief Get the ID of an entity
         * @param entityGUID The globally unique ID to get the entity ID from
         * @return The ID of the entity
         */
        EntityID getEntityID(EntityGUID entityGUID);

        /**
         * @brief Add a component to an entity (caches entity)
         * @param entityID The ID of the entity to add the component
         * @param component The component
         * @return A reference to the ecs
         */
        template <typename T> ECS& addComponent(EntityID entityID, T component);
        /**
         * @brief Add a component to the cached entity
         * @param component The component
         * @return A reference to the ecs
         */
        template <typename T> ECS& addComponent(T component);
        /**
         * @brief Add a shared component to an entity that is already a child of another component (caches entity)
         * @tparam T Component type to share
         * @param entityID The ID of the entity to add the shared component
         * @param parentEntityID The ID of the parent entity to shares its component
         * @return A reference to the ecs
         */
        template <typename T> ECS& addComponent(EntityID entityID, EntityID parentEntityID);
        /**
         * @brief Add a shared component to the cached entity that is already a child of another component
         * @tparam T Component type to share
         * @param parentEntityID The ID of the parent entity to shares its component
         * @return A reference to the ecs
         */
        template <typename T> ECS& addComponent(EntityID parentEntityID);
        /**
         * @brief Removes a component from an entity
         * @tparam T Component type to remove
         * @param entityID The ID of the entity to remove this component
         * @return A reference to the ecs
         */
        template <typename T> ECS& removeComponent(EntityID entityID);

        /**
         * @brief Gets a component from an entity
         * @tparam T Component type to get 
         * @param entityID The ID of the entity to get the component from 
         * @return A reference to the requested component 
         */
        template <typename T> T& getComponent(EntityID entityID);
        /**
         * @brief Gets the only instance of this component
         * @tparam T Component type to get 
         * @return A reference to the requested component 
         */
        template <typename T> T& getComponent();
        /**
         * @brief Checks if there is only one instance of the component
         * @tparam T Component type to check 
         * @return If component type is singular
         */
        template <typename T> bool isSingular();

        /**
         * @brief Gets a component from an reference
         * @tparam T Component type to get
         * @param reference The reference to get the component from 
         * @return A reference to the requested component 
         */
        template <typename T> T& getComponent(Reference<T> reference);
        /**
         * @brief Serializes a component 
         * @param componentTypeID The TypeID of the component
         * @param entityID The entity of the component to serialize
         * @return A vector of the serialized components' bytes 
         */

        std::vector<uint8_t> serializeComponent(TypeID componentTypeID, EntityID entityID);
        /**
         * @brief Deserializes a component 
         * @param componentTypeID The TypeID of the component
         * @param entityID The entity to add the deserialized component
         * @param componentData The serialized component data to deserialize
         */
        void deserializeComponent(TypeID componentTypeID, EntityID entityID, const std::vector<uint8_t> componentData);

        /**
         * @brief Iterates over all the entities 
         * @param routine The function for each iteration (function parameters: ECS &ecs, EntityID &entityID)
         */
        void forEachEntity(std::function<void(EntityID &entityID)> routine);
        /**
         * @brief Iterates over all the components in an entity 
         * @param entityID the id of the entity to iterate through 
         * @param routine The function for each iteration (function parameters: ECS &ecs, EntityID &entityID)
         */
        void forEachComponent(EntityID entityID ,std::function<void(TypeID componentTypeID)> routine);
        /**
         * @brief Iterates over all the entities with the specified component 
         * @tparam T Component type to iterate over
         * @param routine The function for each iteration (function parameters: T &component)
         */
        template <typename T> void forEach(std::function<void(T &component)> routine);
        /**
         * @brief Iterates over all the entities with the specified component
         * @tparam T Component type to iterate over
         * @param routine The function for each iteration (function parameters: T &component, EntityID entityID)
         */
        template <typename T> void forEach(std::function<void(T &component, EntityID entityID)> routine);
        /**
         * @brief Iterates over all the entities with the specified components
         * @tparam T1 Component type to iterate over
         * @tparam T2 Component type to iterate over
         * @param routine The function for each iteration (function parameters: T1 &component1, T2 &component2)
         */
        template <typename T1, typename T2> void forEach(std::function<void(T1 &component1, T2 &component2)> routine);
        /**
         * @brief Iterates over all the entities with the specified components
         * @tparam T1 Component type to iterate over
         * @tparam T2 Component type to iterate over
         * @param routine The function for each iteration (function parameters: T1 &component1, T2 &component2, EntityID entityID)
         */
        template <typename T1, typename T2> void forEach(std::function<void(T1 &component1, T2 &component2, EntityID entityID)> routine);

        /**
         * @brief Display the component types, entities and components
         */
        void displayECS();

        /**
         * @brief Gets the type ID of a component from the name 
         * @param typeName The name of the component to get the ID from
         * @return the ID of the component 
         */
        TypeID getTypeID(std::string typeName);
        /**
         * @brief Gets the ID of a component type 
         * @tparam T The component type to get the ID of 
         * @param routine The function for each iteration (function parameters: T1 &component1, T2 &component2, EntityID entityID)
         */
        template <typename T>  static TypeID getTypeID();

    private:
        struct Component {
            std::size_t componentIndex;
            EntityID parent;
        };
        struct Entity{
            ComponentMap<Component> components;
            EntityGUID entityGUID = 0;
            bool isTombstone = false;

            std::vector<EntityID> childEntities;
            EntityID parentEntity = RootEntityID;
        };

        using RemoveComponentTypeFunc = void (*)(ECS &ecs);
        using PruneComponentListFunc = void (*)(ECS &ecs);
        using ClearComponentListFunc = void (*)(ECS &ecs);

        struct ComponentType {
            void* arrayLocation;
            std::vector<EntityID> entitiesUsingThis;
            std::vector<std::size_t> tombstoneComponents;

            InitialiseFunc initialiseFunc;
            DeinitializeFunc deinitializeFunc;
            SerializeFunc serializeFunc;
            DeserializeFunc deserializeFunc;
            
            RemoveComponentTypeFunc removeComponentTypeFunc;
            PruneComponentListFunc pruneComponentListFunc;
            ClearComponentListFunc clearComponentListFunc;

            std::string name;
        };
        struct ComponentManager{
            std::unordered_map<TypeID, ComponentType> componentTypes;
            std::unordered_map<std::string, TypeID> typeNamesToTypeIds;
        };
        struct EntityManager{
            std::vector<Entity> entities;
            std::vector<EntityID> tombstoneEntities;
            std::unordered_map<EntityGUID, EntityID> entityGUIDToEntityID;

            EntityID cachedEntity;
        };

    private:
        void terminate();
        
        ComponentType* getComponentType(TypeID typeId);
        Component* getComponent(Entity *entity, TypeID typeId);
        Entity* getEntity(EntityID entityID);

        void removeComponent(EntityID entityID, TypeID typeId);

        void addComponent(EntityID entityID, EntityID parentEntityID, TypeID typeId);

        bool componentTypeExists(TypeID typeId);

        void runAllComponentDeinitializes(ComponentType *componentType, TypeID typeId);

        void pruneEntities();

        template <typename T> void pruneComponentList();
        template <typename T> friend void pruneComponentList_(ECS &ecs);

        template <typename T> friend void removeComponentType_(ECS &ecs);

        template <typename T> void clearComponentList();
        template <typename T> friend void clearComponentList_(ECS &ecs);

    private:
        EntityManager entityManager;
        ComponentManager componentManager;
    };
}

#include "ecs.tpp"