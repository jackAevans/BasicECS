#include <iostream>
#include <ecs.hpp>
#include <entt.hpp>
#include <sstream>

#include "test.hpp"

using namespace BasicECS;

int main() {
    LOG_TEST_RESULT(createEntitiesTest);
    LOG_TEST_RESULT(addingComponentTest);
    LOG_TEST_RESULT(removingComponentTest);
    LOG_TEST_RESULT(initializeComponentTest);
    LOG_TEST_RESULT(gettingComponentNameTest);
    LOG_TEST_RESULT(entityGUIDTest);
    LOG_TEST_RESULT(referencesTest);
    LOG_TEST_RESULT(forEachTest);
    LOG_TEST_RESULT(serializationTest);
    LOG_TEST_RESULT(parentingTest);
    LOG_TEST_RESULT(clearingTest);
    LOG_TEST_RESULT(removingEntityTest);

    basicEcsSpeedTest(1000000);

    return 0;
}

struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };

void initialiseVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) { ecs.getComponent<Position>(entity).x = 10;}
void deinitializeVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) { ecs.getComponent<Position>(entity).x = -5;}

std::vector<uint8_t> serializePosition(BasicECS::ECS &ecs, BasicECS::EntityID entity){
    std::vector<uint8_t> serializedData;

    Position component = ecs.getComponent<Position>(entity);
    component.x *= 2;
    uint8_t* componentData = reinterpret_cast<uint8_t*>(&component);
    serializedData.insert(serializedData.end(), componentData, componentData + sizeof(Position));

    return serializedData;
}

void deserializePosition(BasicECS::ECS &ecs, BasicECS::EntityID entity, std::vector<uint8_t> data){
    Position component;

    if (data.size() != sizeof(Position)) {
        throw std::runtime_error("Invalid data size for component deserialization.");
    }

    std::memcpy(&component, data.data(), sizeof(Position));

    ecs.addComponent(entity, component);
}

bool createEntitiesTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1, entity2;
    ecs.addEntity(entity1);
    ecs.addEntity(entity2);

    TEST_ASSERT(entity1 != entity2);

    int iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 2);

    return true;
}

bool addingComponentTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    Velocity &vel = ecs.getComponent<Velocity>(entity1);

    TEST_ASSERT(vel.dx == 2 && vel.dy == 2 && vel.dz == 3);

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Position{1,3,2.5});

    Position pos = ecs.getComponent<Position>(entity2);

    TEST_ASSERT(pos.x == 1 && pos.y == 3 && pos.z == 2.5);


    ecs.addComponent<Velocity>(entity2, entity1);

    Velocity &vel_2 = ecs.getComponent<Velocity>(entity2);

    vel.dx = 4;

    TEST_ASSERT(vel_2.dx == vel.dx && vel_2.dy == vel.dy && vel_2.dz == vel.dz);


    ecs.removeComponent<Position>(entity2);

    std::streambuf* originalBuffer = std::cout.rdbuf();
    std::ostringstream nullStream;
    std::cerr.rdbuf(nullStream.rdbuf());

    try {
        ecs.getComponent<Position>(entity2); 
        TEST_ASSERT(false); 
    } catch (...) {
        // Expected to fail, Position component does not exist
    }

    std::cerr.rdbuf(originalBuffer);

    return true;
}

bool removingComponentTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    ecs.removeComponent<Velocity>(entity1);

    std::streambuf* originalBuffer = std::cout.rdbuf();
    std::ostringstream nullStream;
    std::cerr.rdbuf(nullStream.rdbuf());

    try {

        ecs.getComponent<Velocity>(entity1); 
        TEST_ASSERT(false); 
    } catch (...) {
        // Expected to fail, Position component does not exist
    }

    std::cerr.rdbuf(originalBuffer);

    int iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    return true;
}

bool initializeComponentTest(){
    BasicECS::ECS ecs;

    ecs.addComponentType<Velocity>({initialiseVelocity, deinitializeVelocity});

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Position{0,0,0})
        .addComponent(Velocity{2,2,3});

    Position &pos = ecs.getComponent<Position>(entity1);

    TEST_ASSERT(pos.x == 10);

    ecs.removeComponent<Velocity>(entity1);

    TEST_ASSERT(pos.x == -5);

    return true;
}

bool gettingComponentNameTest(){
    BasicECS::ECS ecs;

    ecs.addComponentType<Velocity>({});

    TEST_ASSERT(ecs.getTypeID("Velocity") == BasicECS::ECS::getTypeID<Velocity>());

    return true;
}

bool entityGUIDTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1, entity2;
    ecs.addEntity(entity1);
    ecs.addEntity(entity2);

    BasicECS::EntityGUID entity1GUID = ecs.getEntityGUID(entity1);
    BasicECS::EntityGUID entity2GUID = ecs.getEntityGUID(entity2);

    TEST_ASSERT(entity1GUID != entity2GUID);

    TEST_ASSERT(ecs.getEntityID(entity1GUID) == entity1);
    TEST_ASSERT(ecs.getEntityID(entity2GUID) == entity2);

    return true;
}

bool referencesTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    Velocity vel = ecs.getComponent<Velocity>(entity1);


    BasicECS::Reference<Velocity> vel_ref = ecs.createReference<Velocity>(entity1);
    Velocity vel2 = ecs.getComponent(vel_ref);

    TEST_ASSERT(vel.dx == vel2.dx && vel.dy == vel2.dy && vel.dz == vel2.dz);

    return true;
}

bool forEachTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Velocity{0,-4,2})
        .addComponent(Position{1,3,2});

    ecs.forEach<Velocity>([](Velocity &vel){
        vel.dx ++;
        vel.dy --;
    });

    Velocity &vel_1 = ecs.getComponent<Velocity>(entity1);
    Velocity &vel_2 = ecs.getComponent<Velocity>(entity2);

    TEST_ASSERT(vel_1.dx == 3 && vel_1.dy == 1);
    TEST_ASSERT(vel_2.dx == 1 && vel_2.dy == -5);

    ecs.forEach<Position, Velocity>([](Position &pos, Velocity &vel){
        vel.dz ++;
        pos.z ++;
    });

    TEST_ASSERT(vel_1.dz == 3);
    TEST_ASSERT(vel_2.dz == 3);

    Position &pos = ecs.getComponent<Position>(entity2);
    TEST_ASSERT(pos.z == 3);

    return true;
}

bool serializationTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    std::size_t velocity_typeID = BasicECS::ECS::getTypeID<Velocity>();

    std::vector<uint8_t> serialized_vel = ecs.serializeComponent(velocity_typeID, entity1);

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2);

    ecs.deserializeComponent(velocity_typeID, entity2, serialized_vel);

    Velocity vel = ecs.getComponent<Velocity>(entity2);

    TEST_ASSERT(vel.dx == 2 && vel.dy == 2 && vel.dz == 3);

    ecs.addComponentType<Position>({ .serializeFunc = serializePosition, .deserializeFunc = deserializePosition});

    BasicECS::EntityID entity3;
    ecs.addEntity(entity3)
        .addComponent(Position{2,2,3});

    std::size_t position_typeID = BasicECS::ECS::getTypeID<Position>();

    std::vector<uint8_t> serialized_pos = ecs.serializeComponent(position_typeID, entity3);

    ecs.deserializeComponent(position_typeID, entity2, serialized_pos);

    Position pos = ecs.getComponent<Position>(entity2);

    TEST_ASSERT(pos.x == 4 && pos.y == 2 && pos.z == 3);

    return true;
}

bool parentingTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Velocity{0,-4,2})
        .addComponent(Position{1,3,2});

    BasicECS::EntityID entity3;
    ecs.addEntity(entity3)
        .addComponent(Position{1,3,2});

    ecs.appendChild(entity1, entity2);
    ecs.appendChild(entity1, entity3);

    TEST_ASSERT(ecs.getParentEntityID(entity1) == BasicECS::RootEntityID);
    TEST_ASSERT(ecs.getParentEntityID(entity2) == entity1);
    TEST_ASSERT(ecs.getChildEntityIDs(entity1).at(0) == entity2);
    TEST_ASSERT(ecs.getChildEntityIDs(entity1).at(1) == entity3);

    return true;
}

bool clearingTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Velocity{0,-4,2})
        .addComponent(Position{1,3,2});

    BasicECS::EntityID entity3;
    ecs.addEntity(entity3)
        .addComponent(Position{1,3,2});

    ecs.clear();

    int iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    return true;
}

bool removingEntityTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Velocity{0,-4,2})
        .addComponent(Position{1,3,2});

    BasicECS::EntityID entity3;
    ecs.addEntity(entity3)
        .addComponent(Position{1,3,2});

    ecs.removeEntity(entity2);

    int iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 2);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    TEST_ASSERT(iterations == 1);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 1);

    ecs.appendChild(entity1, entity3);

    ecs.removeEntity(entity1);

    iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    TEST_ASSERT(iterations == 0);

    return true;
}

double timeSinceEpochMillisec() {
    using namespace std::chrono;
    uint64_t nano = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    return (double)nano/(double)1000000;
}

void enttSpeedTest(int amount){
    entt::registry registry;

    for(int i = 0; i < amount; i++){
        entt::entity entity = registry.create();
        registry.emplace<Position>(entity, Position{0, 1, 20});
        registry.emplace<Velocity>(entity, Velocity{0,0,0});
    }

    double startTime = timeSinceEpochMillisec();

    auto view = registry.view<Velocity>();
    for(auto entity : view){
        Velocity& velocity = view.get<Velocity>(entity);
        velocity.dx += 9;
    }

    std::cout << "Entt time: " << timeSinceEpochMillisec() - startTime << "ms\n";
}

void basicEcsSpeedTest(int amount){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>({});
    ecs.addComponentType<Velocity>({});

    for(int i = 0; i < amount; i++){
        BasicECS::EntityID testEnt;
        ecs.addEntity(testEnt)
            .addComponent<Position>(testEnt, {0, 1, 20})
            .addComponent<Velocity>(testEnt, {0,0,0});
    }

    double startTime = timeSinceEpochMillisec();

    ecs.forEach<Velocity>([](Velocity &velocity){
        velocity.dx += 9;
    });

    std::cout << "BasicECS time: " << timeSinceEpochMillisec() - startTime << "ms\n";

}