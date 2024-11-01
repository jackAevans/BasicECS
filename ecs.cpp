#include "ecs.hpp"
#include <random>
#include <iostream>

namespace BasicECS{

    EntityID getRandomEntityID() {
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);

        return distribution(generator);
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
        entityManager.entities.erase(entityID);
        return *this;
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