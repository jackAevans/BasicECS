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
            .removedComponentIndices = {},
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

        ComponentType *componentType = getComponentType<T>(typeId);

        Entity *entity = getEntity(entityID);

        auto component_it = entity->components.find(typeId);
        if(component_it != entity->components.end()){
            removeComponent<T>(entityID);
        }

        void* componentArrLocation = componentType->arrayLocation;
        std::vector<T>* componentArr = static_cast<std::vector<T>*>(componentArrLocation);

        if(!componentType->removedComponentIndices.empty()){
            std::size_t index = componentType->removedComponentIndices.back();
            componentArr->at(index) = t;
            componentType->removedComponentIndices.pop_back();
    
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

        ComponentType *componentType = getComponentType<T>(typeId);

        Entity *entity = getEntity(entityID);

        Entity *parentEntity = getEntity(parentEntityID);

        Component *component = getComponent<T>(parentEntity, typeId);

        auto component_it = entity->components.find(typeId);
        if(component_it != entity->components.end()){
            removeComponent<T>(entityID);
        }

        entity->components[typeId] = *component;

        componentType->entitiesUsingThis.push_back(entityID);

        return *this;
    }
    template <typename T> ECS& ECS::removeComponent(EntityID entityID){
        TypeID typeId = getTypeId<T>();

        ComponentType *componentType = getComponentType<T>(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = getComponent<T>(entity, typeId);

        if(component->parent == entityID){
            componentType->removedComponentIndices.push_back(component->componentIndex);
                if(componentType->cleanUpFunc != nullptr){
                componentType->cleanUpFunc(*this, entityID);
            }
        }

        entity->components.erase(typeId);

        for(int i = 0; i < (int)componentType->entitiesUsingThis.size(); i++){
            if(componentType->entitiesUsingThis.at(i) == entityID){
                componentType->entitiesUsingThis.erase(componentType->entitiesUsingThis.begin() + i);
            }
        }

        return *this;
    }

    template <typename T> ECS::ComponentType* ECS::getComponentType(TypeID typeId){
        auto componentType_it = componentManager.componentTypes.find(typeId);
        if(componentType_it == componentManager.componentTypes.end()){
            std::cerr << "ERROR: No component of type '" << getTypeName<T>() << "'";
            exit(1);
        }
        return &componentType_it->second;
    }

    template <typename T> ECS::Component* ECS::getComponent(Entity *entity, TypeID typeId){
        auto component_it = entity->components.find(typeId);
        if(component_it == entity->components.end()){
            ComponentType *componentType = getComponentType<T>(typeId);
            std::cerr << "ERROR: Entity doesn't contain component of type '" << componentType->name << "'";
            exit(1);
        }
        return &component_it->second;
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