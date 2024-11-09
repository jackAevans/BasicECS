#pragma once 

#include "ecs.hpp"
#ifdef __GNUG__
    #include <cxxabi.h>
#endif
#include <iostream>

namespace BasicECS{
    template <typename T> std::string getTypeName();

    template <typename T> std::vector<uint8_t> serializeComponent_(ECS &ecs, EntityID entity){
        std::vector<uint8_t> serializedData;

        T& component = ecs.getComponent<T>(entity);
        uint8_t* componentData = reinterpret_cast<uint8_t*>(&component);
        serializedData.insert(serializedData.end(), componentData, componentData + sizeof(T));

        return serializedData;
    }

    template <typename T> void parseComponent_(ECS &ecs, EntityID entity, const std::vector<uint8_t> data){
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

    template <typename T> void pruneComponentType_(ECS &ecs){
        ecs.pruneComponentType<T>();
    }

    template <typename T> Reference<T> ECS::createReference(EntityID entityId){
        return {getTypeId<T>(), getReference(entityId)};
    }

    template <typename T> void ECS::addComponentType(InitialiseFunc initialiseFunc, CleanUpFunc cleanUpFunc){
        TypeID typeID = getTypeId<T>();
        std::string name = getTypeName<T>();
        
        componentManager.componentTypes[typeID] = {
            .arrayLocation = new std::vector<T>(),
            .entitiesUsingThis = {},
            .tombstoneComponents = {},
            .initialiseFunc = initialiseFunc,
            .cleanUpFunc = cleanUpFunc,
            .parseFunc = parseComponent_<T>,
            .serializeFunc = serializeComponent_<T>,
            .removeComponentTypeFunc = removeComponentType_<T>,
            .pruneComponentTypeFunc = pruneComponentType_<T>,
            .name = name
        };

        componentManager.typeNamesToTypeIds[name] = typeID;
    }

    template <typename T> void ECS::removeComponentType(){
        TypeID typeId = getTypeId<T>();
        auto it = componentManager.componentTypes.find(typeId);
        if(it != componentManager.componentTypes.end()){
            delete static_cast<std::vector<T>*>(it->second.arrayLocation);
        }
    }

    template <typename T> ECS& ECS::addComponent(EntityID entityID, T t){
        TypeID typeId = getTypeId<T>();

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = entity->components.get(typeId);
        
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
    template <typename T> ECS& ECS::addComponent(EntityID entityID, ReferenceID parentReferenceID){
        TypeID typeId = getTypeId<T>();
        auto it = entityManager.referenceToID.find(parentReferenceID);

        if(it != entityManager.referenceToID.end()){
            addComponent(entityID, it->second, typeId);
        }else{
            std::cout << "ERROR: component reference doesn't exist '" <<  parentReferenceID << "'\n";
        }

        return *this;
    }
    template <typename T> ECS& ECS::removeComponent(EntityID entityID){
        removeComponent(entityID, getTypeId<T>());
        return *this;
    }

    template <typename T> T& ECS::getComponent(EntityID entityID){
        TypeID typeId = getTypeId<T>();

        ComponentType *componentType = getComponentType(typeId);

        Component *component = getComponent(getEntity(entityID), typeId);

        if(entityID >= entityManager.entities.size()){
            std::cout << "ERROR: component parent does not exist for component '" <<  componentType->name << "'\n";
        }

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        return componentArr->at(component->componentIndex);
    }

    template <typename T> T& ECS::getReferenceComponent(Reference<T> reference){
        return getComponent<T>(entityManager.referenceToID.at(reference.referenceId));
    }

    template <typename T> void ECS::forEach(std::function<void(T &t)> routine){
        TypeID typeId = getTypeId<T>();

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
                currentNextTombstone = componentType->tombstoneComponents.at(currentNextTombstoneIndex);
                currentNextTombstoneIndex ++;
            }else{
                routine(componentArr->at(i));
            }
        }
    }

    template <typename T1, typename T2> void ECS::forEach(std::function<void(T1 &t1, T2 &t2)> routine){
        TypeID typeId1 = getTypeId<T1>();
        ComponentType *componentType1 = getComponentType(typeId1);
        std::vector<T1>* component1Arr = static_cast<std::vector<T1>*>(componentType1->arrayLocation);

        TypeID typeId2 = getTypeId<T2>();
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

    template <typename T> void ECS::pruneComponentType(){
        TypeID typeId = getTypeId<T>();
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

    template <typename T> TypeID ECS::getTypeId(){
        return typeid(T).hash_code();
    }

    template <typename T> std::string getTypeName() {
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
}