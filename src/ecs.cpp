#include "ecs.hpp"
#include <random>
#include <iostream>
#include <fstream>

namespace BasicECS{

    ECS::ECS(){}

    void ECS::terminate(){
        for (auto& componentType : componentManager.componentTypes) {
            runAllComponentCleanUps(&componentType.second, componentType.first);
            componentType.second.removeComponentTypeFunc(*this);
        }
    }

    void ECS::saveState(const char* filePath){
        std::ofstream outFile(filePath, std::ios::binary);

        if (!outFile) {
            std::cerr << "Error opening file for writing!" << std::endl;
            throw std::exception();
        }

        forEachEntity([&outFile](ECS &ecs, EntityID entityId){
            outFile.write(reinterpret_cast<const char*>(&entityId), sizeof(uint64_t));

            Entity *entity = ecs.getEntity(entityId);
            outFile.write(reinterpret_cast<const char*>(&entity->components.size), sizeof(uint64_t));

            entity->components.forEach([&outFile, entityId, &ecs](std::size_t typeId, Component component){
                outFile.write(reinterpret_cast<const char*>(&typeId), sizeof(uint64_t)); //8

                ComponentType componentType = ecs.componentManager.componentTypes.at(typeId);

                if(component.parent == entityId){
                    uint8_t isParent = 1;
                    outFile.write(reinterpret_cast<const char*>(&isParent), sizeof(uint8_t)); //1

                    std::vector<uint8_t> componentData = componentType.serializeFunc(ecs, entityId);
                    outFile.write(reinterpret_cast<const char*>(componentData.data()), componentData.size()); //12
                }else{
                    uint8_t isParent = 0;
                    outFile.write(reinterpret_cast<const char*>(&isParent), sizeof(uint8_t)); //1

                    outFile.write(reinterpret_cast<const char*>(&component.parent), sizeof(uint64_t)); //8
                }

            });
        });

        outFile.close();
    }

    void ECS::forEachEntity(std::function<void(ECS &ecs, EntityID &entity)> routine){
        std::size_t currentNextTombstoneIndex = 0;
        std::size_t currentNextTombstone = 0;
        if(!entityManager.tombstoneEntities.empty()){
            currentNextTombstone = entityManager.tombstoneEntities.at(currentNextTombstoneIndex);
        }

        for(std::size_t i = 0; i < entityManager.entities.size(); i++){
            if(i == currentNextTombstone && currentNextTombstoneIndex < entityManager.tombstoneEntities.size()){
                currentNextTombstone = entityManager.tombstoneEntities.at(currentNextTombstoneIndex);
                currentNextTombstoneIndex ++;
            }else{
                routine(*this, i);
            }
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

        entity->components.forEach([&componentsToDelete](std::size_t key, Component value){
            componentsToDelete.push_back(key);
        });

        for(std::size_t i = 0; i < componentsToDelete.size(); i++){
            removeComponent(entityID, componentsToDelete.at(i));
        }

        auto pos = std::lower_bound(entityManager.tombstoneEntities.begin(), entityManager.tombstoneEntities.end(), entityID);
        entityManager.tombstoneEntities.insert(pos, entityID);

        pruneEntities();
        
        return *this;
    }

    void ECS::removeComponent(EntityID entityID, TypeID typeId){

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = getComponent(entity, typeId);

        if(component->parent == entityID){
            std::size_t index = (std::size_t)component->componentIndex;
            
            auto pos = std::lower_bound(componentType->tombstoneComponents.begin(), componentType->tombstoneComponents.end(), index);
            componentType->tombstoneComponents.insert(pos, index);

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
            std::cerr << "ERROR: component type with id '" << typeId << "' is unknown\n";
            throw std::exception();
        }
        return &componentType_it->second;
    }

    ECS::Component* ECS::getComponent(Entity *entity, TypeID typeId){
        Component *component = entity->components.get(typeId);
        if(component == nullptr){
            ComponentType *componentType = getComponentType(typeId);
            std::cerr << "ERROR: Entity doesn't contain component of type '" << componentType->name << "'\n";
            throw std::exception();
        }
        return component;
    }

    ECS::Entity* ECS::getEntity(EntityID entityID){
        if(entityID >= entityManager.entities.size()){
            std::cerr << "ERROR: No entity with id '" << entityID << "'\n";
            throw std::exception();
        }
        return &entityManager.entities.at(entityID);
    }

    void ECS::runAllComponentCleanUps(ComponentType *componentType, TypeID typeId){
        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            EntityID entityID = componentType->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            if(entity->components.get(typeId)->parent == entityID){
                if(componentType->cleanUpFunc != nullptr){
                    componentType->cleanUpFunc(*this, entityID);
                }
            }
        }
    }

    void ECS::pruneEntities(){
        if(entityManager.tombstoneEntities.empty()){
            return;
        }
        std::size_t i = entityManager.tombstoneEntities.size() - 1;
        std::size_t expected = entityManager.entities.size() - 1;

        while(entityManager.tombstoneEntities.at(i) == expected){
            entityManager.entities.pop_back();
            entityManager.tombstoneEntities.pop_back();
            expected --;
            if(i <= 0){
                return;
            }
            i --;
        }
    }

    void ECS::displayECS(){
        std::cout << "Component Types" << "\n" << "===============" << "\n";

        for (const auto& entry : componentManager.componentTypes) {
            std::string name = entry.second.name;
            int count = entry.second.entitiesUsingThis.size();
            int tombStoneCount = entry.second.tombstoneComponents.size();

            std::cout <<  name << "\n   count: " << count << "\n   tomb stone count: " << tombStoneCount << "\n   tomb stone indexes: ";
            for(int i = 0; i < entry.second.tombstoneComponents.size(); i++){
                std::cout << entry.second.tombstoneComponents.at(i) << " ";
            }
            std::cout << "\n";

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
            entity.components.forEach([*this](std::size_t key, Component value){
                std::cout << "  " << componentManager.componentTypes.at(key).name 
                            << ", index: " << value.componentIndex 
                            << ", parent: " << value.parent << "\n";
            });
        }
    }
}