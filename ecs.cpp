#include "ecs.hpp"
#include <random>
#include <iostream>

namespace BasicECS{

    ECS::ECS(){}

    EntityID getRandomEntityID() {
        // Static random number engine to avoid re-seeding on every call
        static std::mt19937 rng(std::random_device{}());
        // Distribution range [0, 2^32 - 1]
        static std::uniform_int_distribution<std::uint32_t> dist(0, UINT32_MAX);
        return dist(rng);
    }

    ECS& ECS::addEntity(EntityID &entityID){
        entityID = getRandomEntityID();

        while(entityManager.entities.find(entityID) != entityManager.entities.end()){
            entityID++;
        }
        
        entityManager.entities[entityID] = Entity{};

        return *this;
    }

    ECS& ECS::removeEntity(EntityID entityID){
        Entity *entity = getEntity(entityID);

        for (const auto& component : entity->components) {
            removeComponent(entityID, component.first);
        }
        
        entityManager.entities.erase(entityID);
        return *this;
    }

    void ECS::removeComponent(EntityID entityID, TypeID typeId){

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = getComponent(entity, typeId);

        if(component->parent == entityID){
            componentType->removedComponentIndices.push_back(component->componentIndex);
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
        auto entity_it = entityManager.entities.find(entityID);
        if(entity_it == entityManager.entities.end()){
            std::cerr << "ERROR: No entity with id '" << entityID << "'";
            exit(1);
        }
        return &entity_it->second;
    }

    void ECS::displayECS(){
        std::cout << "Component Types" << "\n" << "===============" << "\n";

        for (const auto& entry : componentManager.componentTypes) {
            std::string name = entry.second.name;
            int count = entry.second.entitiesUsingThis.size();
            int tombStoneCount = entry.second.removedComponentIndices.size();

            std::cout <<  name << ", count: " << count << ", tomb stone count: " << tombStoneCount << "\n";
        }
        std::cout <<  "\n";

        std::cout << "Entities" << "\n" << "========" << "\n";

        for (const auto& entity : entityManager.entities) {
            if(entity.second.name.empty()){
                std::cout << entity.first << "\n";
            }else{
                std::cout << entity.second.name << "\n";
            }
            for (const auto& component : entity.second.components) {
                std::cout << "  " << componentManager.componentTypes.at(component.first).name 
                            << ", index: " << component.second.componentIndex 
                            << ", parent: " << component.second.parent << "\n";
            }
        }
    }
}