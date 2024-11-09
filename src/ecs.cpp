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

        uint64_t entityCount = entityManager.entities.size() - entityManager.tombstoneEntities.size();
        outFile.write(reinterpret_cast<const char*>(&entityCount), sizeof(uint64_t));

        forEachEntity([&outFile](ECS &ecs, EntityID entityId){
            uint64_t entityIdData = (uint64_t)entityId;
            outFile.write(reinterpret_cast<const char*>(&entityIdData), sizeof(uint64_t));

            Entity *entity = ecs.getEntity(entityId);

            uint64_t referenceIdData = (uint64_t)entity->referenceId;
            outFile.write(reinterpret_cast<const char*>(&referenceIdData), sizeof(uint64_t));

            uint64_t componentCount = (uint64_t)entity->components.size;
            outFile.write(reinterpret_cast<const char*>(&componentCount), sizeof(uint64_t));

            entity->components.forEach([&outFile, entityId, &ecs](std::size_t typeId, Component component){
                uint64_t typeIdData = (uint64_t)typeId;
                outFile.write(reinterpret_cast<const char*>(&typeIdData), sizeof(uint64_t)); //8

                ComponentType componentType = ecs.componentManager.componentTypes.at(typeId);

                if(component.parent == entityId){
                    uint8_t isParent = 1;
                    outFile.write(reinterpret_cast<const char*>(&isParent), sizeof(uint8_t)); //1

                    std::vector<uint8_t> componentData = componentType.serializeFunc(ecs, entityId);
                    uint64_t dataLength = componentData.size();
                    outFile.write(reinterpret_cast<const char*>(&dataLength), sizeof(uint64_t)); //8
                    outFile.write(reinterpret_cast<const char*>(componentData.data()), componentData.size()); //12
                }else{
                    uint8_t isParent = 0;
                    outFile.write(reinterpret_cast<const char*>(&isParent), sizeof(uint8_t)); //1

                    uint64_t parentData = component.parent;
                    outFile.write(reinterpret_cast<const char*>(&parentData), sizeof(uint64_t)); //8
                }

            });
        });

        outFile.close();
    }

    void ECS::loadState(const char* filePath){
        std::ifstream inFile(filePath, std::ios::binary);

        if (!inFile) {
            std::cerr << "Error opening file for reading!" << std::endl;
            throw std::exception();
        }

        std::unordered_map<uint64_t, EntityID> entityIdResolution;
        std::unordered_map<TypeID, std::pair<uint64_t, uint64_t>> parentReferences;

        uint64_t entityCount = 0;
        inFile.read(reinterpret_cast<char*>(&entityCount), sizeof(uint64_t));

        for(int i = 0; i < entityCount; i++){
            uint64_t entityIdData = 0;
            inFile.read(reinterpret_cast<char*>(&entityIdData), sizeof(uint64_t));

            uint64_t referenceIdData = 0;
            inFile.read(reinterpret_cast<char*>(&referenceIdData), sizeof(uint64_t));

            EntityID entityId;
            ReferenceID referenceId = (ReferenceID)referenceIdData;
            addEntity(entityId, referenceId);
            entityIdResolution[entityIdData] = entityId;

            uint64_t componentCount = 0;
            inFile.read(reinterpret_cast<char*>(&componentCount), sizeof(uint64_t));

            for(std::size_t i = 0; i < (std::size_t)componentCount; i++){
                uint64_t typeId = 0;
                inFile.read(reinterpret_cast<char*>(&typeId), sizeof(uint64_t));

                uint8_t isParent = 0;
                inFile.read(reinterpret_cast<char*>(&isParent), sizeof(uint8_t));

                if(isParent == 0){
                    uint64_t parent = 0;
                    inFile.read(reinterpret_cast<char*>(&parent), sizeof(uint64_t));
                    parentReferences[typeId] = std::make_pair(entityIdData, parent);
                }else{
                    uint64_t dataLength = 0;
                    inFile.read(reinterpret_cast<char*>(&dataLength), sizeof(uint64_t));

                    std::vector<uint8_t> componentData(dataLength);
                    inFile.read(reinterpret_cast<char*>(componentData.data()), componentData.size());

                    ComponentType *componentType = getComponentType(typeId);
                    componentType->parseFunc(*this, entityId, componentData);
                }
            }
        }
        for (const auto& parentReference : parentReferences) {
            EntityID entityId = entityIdResolution.at(parentReference.second.first);
            EntityID parentEntityId = entityIdResolution.at(parentReference.second.second);
            addComponent(entityId, parentEntityId, parentReference.first);
        }
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

        ReferenceID referenceID = rand() % 4294967295;
        while(entityManager.referenceToID.find(referenceID) != entityManager.referenceToID.end()){
            referenceID ++;
        }
        addEntity(entityID, referenceID);

        return *this;
    }

    void ECS::addEntity(EntityID &entityID, ReferenceID &referenceID){
        while(entityManager.referenceToID.find(referenceID) != entityManager.referenceToID.end()){
            referenceID ++;
        }
        Entity entity{.referenceId = referenceID};

        if(!entityManager.tombstoneEntities.empty()){
            entityID = entityManager.tombstoneEntities.back();
            entityManager.tombstoneEntities.pop_back();
            entityManager.entities[entityID] = entity;
        }else{
            entityManager.entities.push_back(entity);
            entityID = entityManager.entities.size() - 1;
        }

        entityManager.referenceToID[referenceID] = entityID;
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

    ReferenceID ECS::getReference(EntityID entityId){
        return getEntity(entityId)->referenceId;
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

        componentType->pruneComponentTypeFunc(*this);
    }

    void ECS::addComponent(EntityID entityID, EntityID parentEntityID, TypeID typeId){
        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Entity *parentEntity = getEntity(parentEntityID);

        Component component = (Component)*getComponent(parentEntity, typeId);

        Component *component_it = entity->components.get(typeId);
        if(component_it != nullptr){
            removeComponent(entityID, typeId);
        }

        entity->components.insert(typeId, component);

        componentType->entitiesUsingThis.push_back(entityID);
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

        forEachEntity([](ECS &ecs, EntityID &entityId){ 
            Entity entity = ecs.entityManager.entities.at(entityId);
            std::cout << "id: " << entityId << " refId: " << entity.referenceId << "\n";
            entity.components.forEach([&ecs](std::size_t key, Component value){
                std::cout << "  " << ecs.componentManager.componentTypes.at(key).name 
                            << ", index: " << value.componentIndex 
                            << ", parent: " << value.parent << "\n";
            });
            std::cout <<  "\n";
        });
    }
}