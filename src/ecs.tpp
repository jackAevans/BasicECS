#pragma once 

#include "ecs.hpp"
#ifdef __GNUG__
    #include <cxxabi.h>
#endif
#include <iostream>

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