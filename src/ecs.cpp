#include "ecs.hpp"
#include <random>
#include <iostream>
#include <fstream>

namespace BasicECS{

    ECS::~ECS(){
        terminate();
    }

    void ECS::terminate(){
        for (auto& componentType : componentManager.componentTypes) {
            componentType.second.removeComponentTypeFunc(*this);
        }
    }

    void ECS::clear(){
        for (auto& componentType : componentManager.componentTypes) {
            runAllComponentDeinitializes(&componentType.second, componentType.first);
        }
        for (auto& componentType : componentManager.componentTypes) {
            componentType.second.clearComponentListFunc(*this);
        }

        entityManager.entities.clear();
        entityManager.resourceToID.clear();
        entityManager.tombstoneEntities.clear();
    }

    void ECS::forEachEntity(std::function<void(EntityID &entity)> routine){
        std::size_t currentNextTombstoneIndex = 0;
        std::size_t currentNextTombstone = 0;
        if(!entityManager.tombstoneEntities.empty()){
            currentNextTombstone = entityManager.tombstoneEntities.at(currentNextTombstoneIndex);
        }

        for(std::size_t i = 0; i < entityManager.entities.size(); i++){
            if(i == currentNextTombstone && currentNextTombstoneIndex < entityManager.tombstoneEntities.size()){
                currentNextTombstoneIndex ++;
                if(currentNextTombstoneIndex < entityManager.tombstoneEntities.size()){
                    currentNextTombstone = entityManager.tombstoneEntities.at(currentNextTombstoneIndex);
                }
            }else{
                routine(i);
            }
        }
    }
    void ECS::forEachComponent(EntityID entityID ,std::function<void(TypeID componentTypeID)> routine){
        getEntity(entityID)->components.forEach([&routine](TypeID componentTypeID, Component value){
            routine(componentTypeID);
        });
    }

    ECS& ECS::addEntity(EntityID &entityID){

        ResourceID resourceID = 0;
        addEntity(entityID, resourceID);

        return *this;
    }
    ECS& ECS::addEntity(){
        EntityID entityID;
        return addEntity(entityID);
    }

    void ECS::addEntity(EntityID &entityID, ResourceID resourceID){
        Entity entity{.resourceID = resourceID};

        if(!entityManager.tombstoneEntities.empty()){
            entityID = entityManager.tombstoneEntities.back();
            entityManager.tombstoneEntities.pop_back();
            entityManager.entities[entityID] = entity;
        }else{
            entityManager.entities.push_back(entity);
            entityID = entityManager.entities.size() - 1;
        }

        if(resourceID != 0){
            if(entityManager.resourceToID.find(resourceID) != entityManager.resourceToID.end()){
                std::cerr << "ERROR: resourceID  '" << resourceID << "' already exists\n";
                throw std::exception();
            }
            entityManager.resourceToID[resourceID] = entityID;
        }

        entityManager.cachedEntity = entityID;

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

        for(std::size_t i = 0; i < entity->childEntities.size(); i++){
            removeEntity(entity->childEntities.at(i));
        }

        if(entity->parentEntity != RootEntityID){
            Entity *parentEntity = getEntity(entity->parentEntity);

            std::vector<EntityID> vec = parentEntity->childEntities;
            vec.erase(std::remove(vec.begin(), vec.end(), entityID), vec.end());
        }

        entity->isTombstone = true;

        pruneEntities();
        
        return *this;
    }

    ECS& ECS::appendChild(EntityID entityID, EntityID childEntityID){
        Entity *entity = getEntity(entityID);
        Entity *childEntity = getEntity(childEntityID);

        entity->childEntities.push_back(childEntityID);
        childEntity->parentEntity = entityID;

        return *this;
    }
    EntityID ECS::getParentEntityID(EntityID entityID){
        Entity *entity = getEntity(entityID);
        return entity->parentEntity;
    }
    std::vector<EntityID> ECS::getChildEntityIDs(EntityID entityID){
        Entity *entity = getEntity(entityID);
        return entity->childEntities;
    }

    void ECS::removeComponent(EntityID entityID, TypeID typeId){

        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Component *component = getComponent(entity, typeId);

        if(component->parent == entityID){
            std::size_t index = (std::size_t)component->componentIndex;
            
            auto pos = std::lower_bound(componentType->tombstoneComponents.begin(), componentType->tombstoneComponents.end(), index);
            componentType->tombstoneComponents.insert(pos, index);

            if(componentType->deinitializeFunc != nullptr){
                componentType->deinitializeFunc(*this, entityID);
            }
        }

        entity->components.erase(typeId);

        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            if(componentType->entitiesUsingThis.at(i) == entityID){
                componentType->entitiesUsingThis.erase(componentType->entitiesUsingThis.begin() + i);
            }
        }

        componentType->pruneComponentListFunc(*this);
    }

    void ECS::addComponent(EntityID entityID, EntityID parentEntityID, TypeID typeId){
        ComponentType *componentType = getComponentType(typeId);

        Entity *entity = getEntity(entityID);

        Entity *parentEntity = getEntity(parentEntityID);

        Component component = (Component)*getComponent(parentEntity, typeId);

        entityManager.cachedEntity = entityID;

        Component *component_it = entity->components.get(typeId);
        if(component_it != nullptr){
            removeComponent(entityID, typeId);
        }

        entity->components.insert(typeId, component);

        componentType->entitiesUsingThis.push_back(entityID);

        ResourceID newResourceID = (std::size_t)rand();
        while(entityManager.resourceToID.find(newResourceID) != entityManager.resourceToID.end() || newResourceID == 0){
            newResourceID ++;
        }
        getEntity(parentEntityID)->resourceID = newResourceID;
        entityManager.resourceToID[newResourceID] = parentEntityID;
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
        if(entityID < entityManager.entities.size()){
            if(entityManager.entities.at(entityID).isTombstone == false){
                return &entityManager.entities.at(entityID);
            }
        }

        std::cerr << "ERROR: No entity with id '" << entityID << "'\n";
        throw std::exception();
    }

    std::vector<uint8_t> ECS::serializeComponent(TypeID componentTypeID, EntityID entityID){
        ComponentType *componentType = getComponentType(componentTypeID);
        if(componentType->serializeFunc == nullptr){
            return {};
        }
        return componentType->serializeFunc(*this, entityID);
    }
    void ECS::deserializeComponent(TypeID componentTypeID, EntityID entityID, const std::vector<uint8_t> componentData){
        ComponentType *componentType = getComponentType(componentTypeID);
        if(componentType->deserializeFunc == nullptr){
            return;
        }
        componentType->deserializeFunc(*this, entityID, componentData);
    }

    void ECS::runAllComponentDeinitializes(ComponentType *componentType, TypeID typeId){
        for(std::size_t i = 0; i < componentType->entitiesUsingThis.size(); i++){
            EntityID entityID = componentType->entitiesUsingThis.at(i);
            Entity *entity = &entityManager.entities.at(entityID);

            if(entity->components.get(typeId)->parent == entityID){
                if(componentType->deinitializeFunc != nullptr){
                    componentType->deinitializeFunc(*this, entityID);
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

        std::cout << "tombstone entities: ";
        for(int i = 0; i < entityManager.tombstoneEntities.size(); i++){
            std::cout << entityManager.tombstoneEntities.at(i) << ", ";
        }
        std::cout <<  "\n";

        forEachEntity([this](EntityID &entityId){ 
            Entity entity = this->entityManager.entities.at(entityId);
            std::cout << "id: " << entityId;
            if(entity.resourceID > 0){std::cout << "  resourceId: " << entity.resourceID;}
            if(entity.parentEntity < (std::size_t)-1){std::cout << " parentEntity: " << entity.parentEntity;}
            std::cout <<  "\n";
            entity.components.forEach([this, entityId](std::size_t key, Component value){
                std::cout << "  " << this->componentManager.componentTypes.at(key).name 
                            << ", index: " << value.componentIndex;
                if(value.parent != entityId){std::cout << ", parent: " << value.parent;}
                std::cout <<  "\n";
            });
            std::cout <<  "\n";
        });
    }
}