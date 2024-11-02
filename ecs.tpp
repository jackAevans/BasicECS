#pragma once 

#include "ecs.hpp"
#ifdef __GNUG__
    #include <cxxabi.h>
#endif
#include <iostream>

namespace BasicECS{
    template <typename T> std::string getTypeName();

    template <typename T> void ECS::addComponentType(InitialiseFunc initialiseFunc, CleanUpFunc cleanUpFunc, ParseFunc parseFunc){
        TypeID typeID = getTypeId<T>();
        std::string name = getTypeName<T>();
        
        componentManager.componentTypes[typeID] = {
            .arrayLocation = new std::vector<T>(),
            .entitiesUsingThis = {},
            .tombstoneComponents = {},
            .initialiseFunc = initialiseFunc,
            .cleanUpFunc = cleanUpFunc,
            .parseFunc = parseFunc,
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

        auto component_it = entity->components.find(typeId);
        if(component_it != entity->components.end()){
            removeComponent<T>(entityID);
        }

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        if(!componentType->tombstoneComponents.empty()){
            std::size_t index = componentType->tombstoneComponents.back();
            componentArr->at(index) = t;
            componentType->tombstoneComponents.pop_back();
    
            entity->components[typeId] = {.componentIndex = index, .parent = entityID};
        }else{
            componentArr->push_back(t);

            entity->components[typeId] = {.componentIndex = componentArr->size() - 1, .parent = entityID};
        }

        componentType->entitiesUsingThis.push_back(entityID);

        if(componentType->initialiseFunc != nullptr){
            componentType->initialiseFunc(*this, entityID);
        }

        return *this;
    }
    template <typename T> ECS& ECS::addComponent(EntityID entityID, EntityID parentEntityID){
        TypeID typeId = getTypeId<T>();

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Entity *parentEntity = getEntity(parentEntityID);

        Component *component = getComponent(parentEntity, typeId);

        auto component_it = entity->components.find(typeId);
        if(component_it != entity->components.end()){
            removeComponent<T>(entityID);
        }

        entity->components[typeId] = *component;

        componentType->entitiesUsingThis.push_back(entityID);

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

    template <typename T> void ECS::forEach(void (*routine)(T &t)){
        TypeID typeId = getTypeId<T>();

        ComponentType *componentType = getComponentType(typeId);

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        for(std::size_t i = 0; i < componentArr->size(); i++){
            routine(componentArr->at(i));
        }
    }

    template <typename T1, typename T2> void ECS::forEach(void (*routine)(T1 &t1, T2 &t2)){
        TypeID typeId1 = getTypeId<T1>();
        ComponentType *componentType1 = getComponentType(typeId1);
        std::vector<T1>* component1Arr = static_cast<std::vector<T1>*>(componentType1->arrayLocation);

        TypeID typeId2 = getTypeId<T2>();
        ComponentType *componentType2 = getComponentType(typeId2);
        std::vector<T2>* component2Arr = static_cast<std::vector<T2>*>(componentType2->arrayLocation);

        for(std::size_t i = 0; i < componentType1->entitiesUsingThis.size(); i++){

            EntityID entityID = componentType1->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            auto typeId2_it = entity->components.find(typeId2);

            if(typeId2_it != entity->components.end()){
                Component component1 = entity->components.at(typeId1);
                Component component2 = typeId2_it->second;

                routine(component1Arr->at(component1.componentIndex), component2Arr->at(component2.componentIndex));
            }
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