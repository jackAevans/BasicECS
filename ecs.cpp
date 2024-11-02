#include "ecs.hpp"
#include <random>
#include <iostream>

namespace BasicECS{

    ECS::ECS(){}

    void ECS::terminate(){
        for (auto& componentType : componentManager.componentTypes) {
            runAllComponentCleanUps(&componentType.second, componentType.first);
        }
    }

    ECS& ECS::addEntity(EntityID &entityID){

        if(!entityManager.tombstoneEntities.empty()){
            entityID = entityManager.tombstoneEntities.back();
            entityManager.tombstoneEntities.pop_back();
            entityManager.entities[entityID] = Entity{};
        }else{
            entityManager.entities.push_back(Entity{});
            entityID = entityManager.entities.size() - 1;
        }

        return *this;
    }

    ECS& ECS::removeEntity(EntityID entityID){
        Entity *entity = getEntity(entityID);

        std::vector<TypeID> componentsToDelete;

        for (const auto& component : entity->components) {
            componentsToDelete.push_back(component.first);
        }

        for(std::size_t i = 0; i < componentsToDelete.size(); i++){
            removeComponent(entityID, componentsToDelete.at(i));
        }

        entityManager.tombstoneEntities.push_back(entityID);

        return *this;
    }

    void ECS::removeComponent(EntityID entityID, TypeID typeId){

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = getComponent(entity, typeId);

        if(component->parent == entityID){
            componentType->tombstoneComponents.push_back(component->componentIndex);
                if(componentType->cleanUpFunc != nullptr){
                    componentType->cleanUpFunc(*this, entityID);
                }
        }

        entity->components.erase(typeId);

        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            if(componentType->entitiesUsingThis.at(i) == entityID){
                componentType->entitiesUsingThis.erase(componentType->entitiesUsingThis.begin() + i);
            }
        }
    }

    ECS::ComponentType* ECS::getComponentType(TypeID typeId){
        auto componentType_it = componentManager.componentTypes.find(typeId);
        if(componentType_it == componentManager.componentTypes.end()){
            std::cerr << "ERROR: component type with id '" << typeId << "' is unknown";
            exit(1);
        }
        return &componentType_it->second;
    }

    ECS::Component* ECS::getComponent(Entity *entity, TypeID typeId){
        auto component_it = entity->components.find(typeId);
        if(component_it == entity->components.end()){
            ComponentType *componentType = getComponentType(typeId);
            std::cerr << "ERROR: Entity doesn't contain component of type '" << componentType->name << "'";
            exit(1);
        }
        return &component_it->second;
    }

    ECS::Entity* ECS::getEntity(EntityID entityID){
        if(entityID >= entityManager.entities.size()){
            std::cerr << "ERROR: No entity with id '" << entityID << "'";
            exit(1);
        }
        return &entityManager.entities.at(entityID);
    }

    void ECS::runAllComponentCleanUps(ComponentType *componentType, TypeID typeId){
        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            EntityID entityID = componentType->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            if(entity->components.at(typeId).parent == entityID){
                if(componentType->cleanUpFunc != nullptr){
                    componentType->cleanUpFunc(*this, entityID);
                }
            }
        }
    }

    void ECS::displayECS(){
        std::cout << "Component Types" << "\n" << "===============" << "\n";

        for (const auto& entry : componentManager.componentTypes) {
            std::string name = entry.second.name;
            int count = entry.second.entitiesUsingThis.size();
            int tombStoneCount = entry.second.tombstoneComponents.size();

            std::cout <<  name << ", count: " << count << ", tomb stone count: " << tombStoneCount << "\n";
        }
        std::cout <<  "\n";

        std::cout << "Entities" << "\n" << "========" << "\n";

        for (std::size_t i = 0; i < entityManager.entities.size(); i++) {
            Entity entity = entityManager.entities.at(i);
            if(entity.name.empty()){
                std::cout << i << "\n";
            }else{
                std::cout << entity.name << "\n";
            }
            for (const auto& component : entity.components) {
                std::cout << "  " << componentManager.componentTypes.at(component.first).name 
                            << ", index: " << component.second.componentIndex 
                            << ", parent: " << component.second.parent << "\n";
            }
        }
    }
}