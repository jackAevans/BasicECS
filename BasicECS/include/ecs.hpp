#pragma once

#include "componentMap.hpp"

#include <unordered_map>
#include <vector>
#include <functional>

#ifdef __GNUG__
    #include <cxxabi.h>
#endif
#include <iostream>

namespace BasicECS{

    class ECS;

    using TypeID = std::size_t;
    using EntityID = std::size_t;
    using EntityGUID = uint64_t;

    constexpr std::size_t RootEntityID = -1;

    template<typename T>
    struct Reference{
        TypeID typeId;
        EntityGUID entityGUID;
    };

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

namespace BasicECS{
    template <typename T> static std::string getTypeName() {
        std::string typeName;

        #ifdef __GNUG__  // GCC/Clang demangling
            int status;
            char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
            typeName = (status == 0) ? demangled : typeid(T).name();
            free(demangled);
        #else  // MSVC or other compilers
            typeName = typeid(T).name();
        #endif

        return typeName;
    }

    template <typename T>  TypeID ECS::getTypeID(){
        return typeid(T).hash_code();
    }

    template <typename T> static std::vector<uint8_t> serializeTrivialComponent(ECS &ecs, EntityID entity){
        std::vector<uint8_t> serializedData;

        T& component = ecs.getComponent<T>(entity);
        uint8_t* componentData = reinterpret_cast<uint8_t*>(&component);
        serializedData.insert(serializedData.end(), componentData, componentData + sizeof(T));

        return serializedData;
    }

    template <typename T> static void deserializeTrivialComponent(ECS &ecs, EntityID entity, const std::vector<uint8_t> data){
        T component;

        // Check if data size matches the expected size for the component type
        if (data.size() != sizeof(T)) {
            throw std::runtime_error("Invalid data size for component deserialization.");
        }

        // Copy the data into the component's memory
        std::memcpy(&component, data.data(), sizeof(T));

        // Add the component to the entity in ECS
        ecs.addComponent(entity, component);
    }

    template <typename T> void removeComponentType_(ECS &ecs){
        ecs.removeComponentType<T>();
    }

    template <typename T> void pruneComponentList_(ECS &ecs){
        ecs.pruneComponentList<T>();
    }

    template <typename T> void clearComponentList_(ECS &ecs){
        ecs.clearComponentList<T>();
    }

    template <typename T> Reference<T> ECS::createReference(EntityID entityId){
        return {getTypeID<T>(), getEntity(entityId)->entityGUID};
    }

    template <typename T> void ECS::addComponentType(ComponentFunctions componentFunctions){
        TypeID typeID = getTypeID<T>();
        std::string name = getTypeName<T>();

        ComponentType componentType = {
            .arrayLocation = new std::vector<T>(),
            .entitiesUsingThis = {},
            .tombstoneComponents = {},
            .initialiseFunc = componentFunctions.initialiseFunc,
            .deinitializeFunc = componentFunctions.deinitializeFunc,
            .removeComponentTypeFunc = removeComponentType_<T>,
            .pruneComponentListFunc = pruneComponentList_<T>,
            .clearComponentListFunc = clearComponentList_<T>,
            .name = name
        };

        bool isTrivial = std::is_trivial<T>();

        if(componentFunctions.serializeFunc != nullptr){
            componentType.serializeFunc = componentFunctions.serializeFunc;
        }else{
            if(isTrivial){
                componentType.serializeFunc = serializeTrivialComponent<T>;
            }else{
                std::cout << "WARNING: Not trivial component '" << name << "' doesn't have serialize function\n";
                componentType.serializeFunc = nullptr;
            }
        }

        if(componentFunctions.deserializeFunc != nullptr){
            componentType.deserializeFunc = componentFunctions.deserializeFunc;
        }else{
            if(isTrivial){
                componentType.deserializeFunc = deserializeTrivialComponent<T>;
            }else{
                std::cout << "WARNING: Not trivial component '" << name << "' doesn't have deserialize function\n";
                componentType.deserializeFunc = nullptr;
            }
        }
        
        componentManager.componentTypes[typeID] = componentType;
        componentManager.typeNamesToTypeIds[name] = typeID;
    }

    template <typename T> void ECS::removeComponentType(){
        TypeID typeId = getTypeID<T>();
        auto it = componentManager.componentTypes.find(typeId);
        if(it != componentManager.componentTypes.end()){
            runAllComponentDeinitializes(&it->second, typeId);
            delete static_cast<std::vector<T>*>(it->second.arrayLocation);
        }
    }

    template <typename T> ECS& ECS::addComponent(EntityID entityID, T t){
        TypeID typeId = getTypeID<T>();

        auto componentType_it = componentManager.componentTypes.find(typeId);
        if(componentType_it == componentManager.componentTypes.end()){
            addComponentType<T>({});
        }
        ComponentType *componentType = &componentManager.componentTypes.at(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = entity->components.get(typeId);

        entityManager.cachedEntity = entityID;
        
        if(component != nullptr){
            removeComponent<T>(entityID);
        }

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        if(!componentType->tombstoneComponents.empty()){
            std::size_t index = componentType->tombstoneComponents.back();
            componentArr->at(index) = t;
            componentType->tombstoneComponents.pop_back();
    
            entity->components.insert(typeId, {.componentIndex = index, .parent = entityID});
        }else{
            componentArr->push_back(t);

            entity->components.insert(typeId, {.componentIndex = componentArr->size() - 1, .parent = entityID});
        }

        componentType->entitiesUsingThis.push_back(entityID);

        if(componentType->initialiseFunc != nullptr){
            componentType->initialiseFunc(*this, entityID);
        }

        return *this;
    }
    template <typename T> ECS& ECS::addComponent(T component){
        return addComponent(entityManager.cachedEntity, component);
    }
    template <typename T> ECS& ECS::addComponent(EntityID entityID, EntityID parentID){
        TypeID typeId = getTypeID<T>();

        addComponent(entityID, parentID, typeId);

        return *this;
    }
    template <typename T> ECS& ECS::addComponent(EntityID parentEntityID){
        return addComponent<T>(entityManager.cachedEntity, parentEntityID);
    }
    template <typename T> ECS& ECS::removeComponent(EntityID entityID){
        removeComponent(entityID, getTypeID<T>());
        return *this;
    }

    template <typename T> T& ECS::getComponent(EntityID entityID){
        TypeID typeId = getTypeID<T>();

        ComponentType *componentType = getComponentType(typeId);

        Component *component = getComponent(getEntity(entityID), typeId);

        if(entityID >= entityManager.entities.size()){
            std::cerr << "ERROR: component parent does not exist for component '" <<  componentType->name << "'\n";
        }

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        return componentArr->at(component->componentIndex);
    }

    template <typename T> T& ECS::getComponent(Reference<T> reference){
        return getComponent<T>(entityManager.entityGUIDToEntityID.at(reference.entityGUID));
    }

    template <typename T> void ECS::forEach(std::function<void(T &t)> routine){
        TypeID typeId = getTypeID<T>();

        ComponentType *componentType = getComponentType(typeId);

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        std::size_t currentNextTombstoneIndex = 0;
        std::size_t currentNextTombstone = 0;
        if(!componentType->tombstoneComponents.empty()){
            currentNextTombstone = componentType->tombstoneComponents.at(currentNextTombstoneIndex);
        }


        for(std::size_t i = 0; i < componentArr->size(); i++){
            if(i == currentNextTombstone && currentNextTombstoneIndex < componentType->tombstoneComponents.size()){
                currentNextTombstoneIndex ++;
                if(currentNextTombstoneIndex < componentType->tombstoneComponents.size()){
                    currentNextTombstone = componentType->tombstoneComponents.at(currentNextTombstoneIndex);
                }
            }else{
                routine(componentArr->at(i));
            }
        }
    }
    template <typename T> void ECS::forEach(std::function<void(T &t, EntityID entityID)> routine){
        TypeID typeId = getTypeID<T>();
        ComponentType *componentType = getComponentType(typeId);
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentType->arrayLocation);

        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            EntityID entityID = componentType->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            Component *component = entity->components.get(typeId);
            routine(componentArr->at(component->componentIndex), entityID);
        }
    }

    template <typename T1, typename T2> void ECS::forEach(std::function<void(T1 &t1, T2 &t2)> routine){
        TypeID typeId1 = getTypeID<T1>();
        ComponentType *componentType1 = getComponentType(typeId1);
        std::vector<T1>* component1Arr = static_cast<std::vector<T1>*>(componentType1->arrayLocation);

        TypeID typeId2 = getTypeID<T2>();
        ComponentType *componentType2 = getComponentType(typeId2);
        std::vector<T2>* component2Arr = static_cast<std::vector<T2>*>(componentType2->arrayLocation);

        for(std::size_t i = 0; i < componentType1->entitiesUsingThis.size(); i++){

            EntityID entityID = componentType1->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            Component *component2 = entity->components.get(typeId2);

            if(component2 != nullptr){
                Component *component1 = entity->components.get(typeId1);

                routine(component1Arr->at(component1->componentIndex), component2Arr->at(component2->componentIndex));
            }
        }
    }

    template <typename T1, typename T2> void ECS::forEach(std::function<void(T1 &t1, T2 &t2, EntityID entityID)> routine){
        TypeID typeId1 = getTypeID<T1>();
        ComponentType *componentType1 = getComponentType(typeId1);
        std::vector<T1>* component1Arr = static_cast<std::vector<T1>*>(componentType1->arrayLocation);

        TypeID typeId2 = getTypeID<T2>();
        ComponentType *componentType2 = getComponentType(typeId2);
        std::vector<T2>* component2Arr = static_cast<std::vector<T2>*>(componentType2->arrayLocation);

        for(std::size_t i = 0; i < componentType1->entitiesUsingThis.size(); i++){

            EntityID entityID = componentType1->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            Component *component2 = entity->components.get(typeId2);

            if(component2 != nullptr){
                Component *component1 = entity->components.get(typeId1);

                routine(component1Arr->at(component1->componentIndex), component2Arr->at(component2->componentIndex), entityID);
            }
        }
    }

    template <typename T> void ECS::pruneComponentList(){
        TypeID typeId = getTypeID<T>();
        ComponentType *componentType = getComponentType(typeId);
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentType->arrayLocation);

        if(componentType->tombstoneComponents.empty()){
            return;
        }

        std::size_t i = componentType->tombstoneComponents.size() - 1;
        std::size_t expected = componentArr->size() - 1;

        while(componentType->tombstoneComponents.at(i) == expected){
            componentArr->pop_back();
            componentType->tombstoneComponents.pop_back();
            expected --;
            if(i <= 0){
                return;
            }
            i --;
        }
    }

    template <typename T> void ECS::clearComponentList(){
        TypeID typeId = getTypeID<T>();
        ComponentType *componentType = getComponentType(typeId);
        componentType->entitiesUsingThis.clear();
        componentType->tombstoneComponents.clear();
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentType->arrayLocation);
        componentArr->clear();
    }
}