#include <iostream>
#include <cassert>
#include <ecs.hpp>
#include <entt.hpp>
#include <sstream>

#include "test.hpp"

// Define some mock components and functions for testing
static int allocation_count = 0;

struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };
struct Body { BasicECS::Reference<Velocity> velocityRef;};

void initialisePosition(BasicECS::ECS &ecs, BasicECS::EntityID entity) { allocation_count ++; std::cout << "init pos\n";}
void deinitializePosition(BasicECS::ECS &ecs, BasicECS::EntityID entity) { allocation_count --; std::cout << "clean pos\n";}

void initialiseVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) { allocation_count ++; ecs.getComponent<Velocity>(entity).dx = 12;}
void deinitializeVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) {allocation_count --;}

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

void testECS() {
    // Create an ECS instance
    BasicECS::ECS ecs;

    // Register component types
    ecs.addComponentType<Position>({initialisePosition, deinitializePosition});
    ecs.addComponentType<Velocity>({initialiseVelocity, deinitializeVelocity});
    ecs.addComponentType<Body>({});

    // Add and retrieve entities
    BasicECS::EntityID entity1, entity2;
    ecs.addEntity(entity1);
    ecs.addEntity(entity2);
    assert(entity1 != entity2);

    // Add components to entities
    Position pos1 = {10.0f, 20.0f, 30.0f};
    ecs.addComponent(entity1, pos1);

    Velocity vel1 = {1.0f, 0.5f, -1.0f};
    ecs.addComponent(vel1);

    // Adding only Position component to entity2
    Position pos2 = {15.0f, 25.0f, 35.0f};
    ecs.addComponent(entity2, pos2);

    // create a reference to the first entities velocity component
    BasicECS::Reference<Velocity> ref = ecs.createReference<Velocity>(entity1);
    assert(ecs.getComponent<Velocity>(entity1).dx == ecs.getComponent(ref).dx);

    // create a entity with a body component that uses the reference
    BasicECS::EntityID entity3;
    ecs.addEntity(entity3);
    Body body = {ref};
    ecs.addComponent<Body>(entity3, body);
    assert(entity1 != entity3);

    // Test cached entity 
    ecs.addEntity()
        .addComponent(Velocity{0,2,0})
        .addComponent<Body>(entity3);

    // Test getComponent functionality
    Position& retrievedPos1 = ecs.getComponent<Position>(entity1);
    assert(retrievedPos1.x == 10.0f && retrievedPos1.y == 20.0f && retrievedPos1.z == 30.0f);

    Velocity& retrievedVel1 = ecs.getComponent<Velocity>(entity1);
    assert(retrievedVel1.dx == 12.0f && retrievedVel1.dy == 0.5f && retrievedVel1.dz == -1.0f);

    Position& retrievedPos2 = ecs.getComponent<Position>(entity2);
    assert(retrievedPos2.x == 15.0f && retrievedPos2.y == 25.0f && retrievedPos2.z == 35.0f);

    // Use forEach to verify iteration over components
    ecs.forEach<Position>([](Position& pos) {
        pos.x += 1.0f;  // Modify x component for all Position components
    });

    // Check that positions have been updated
    std::cout << ecs.getComponent<Position>(entity1).x << std::endl;
    assert(ecs.getComponent<Position>(entity1).x == 11.0f);
    assert(ecs.getComponent<Position>(entity2).x == 16.0f);

    // Step 7: Use forEach for multiple component types
    ecs.forEach<Position, Velocity>([](Position& pos, Velocity& vel) {
        pos.y += vel.dy;
        pos.z += vel.dz;
    });

    // Check if values have been updated accordingly
    assert(ecs.getComponent<Position>(entity1).y == 20.5f);
    assert(ecs.getComponent<Position>(entity1).z == 29.0f);

    // Step 8: Test removing a component
    ecs.removeComponent<Position>(entity1);
    try {
        ecs.getComponent<Position>(entity1); // Should throw or cause an error if Position was removed
        assert(false); // If we reach here, the test has failed
    } catch (...) {
        // Expected to fail, Position component does not exist
    }

    // Check that the reference from body works
    Velocity& retrievedVel2 = ecs.getComponent(ecs.getComponent<Body>(entity3).velocityRef);
    assert(retrievedVel2.dx == 12.0f && retrievedVel2.dy == 0.5f && retrievedVel2.dz == -1.0f);

    // Add a position component to entity1 that references entity2
    ecs.addComponent<Position>(entity1, entity2);
    Position& ent1Pos = ecs.getComponent<Position>(entity1);
    Position& ent2Pos = ecs.getComponent<Position>(entity2);
    assert(ent1Pos.x == ent2Pos.x && ent1Pos.y == ent2Pos.y && ent1Pos.z == ent2Pos.z);

    // Test serialization 
    std::vector<uint8_t> data = ecs.serializeComponent(BasicECS::ECS::getTypeId<Velocity>(), entity1);
    ecs.deserializeComponent(BasicECS::ECS::getTypeId<Velocity>(), entity3, data);
    
    Velocity &vel3_1 = ecs.getComponent<Velocity>(entity1);
    Velocity &vel3_3 = ecs.getComponent<Velocity>(entity3);

    assert(vel3_1.dx == vel3_3.dx && vel3_1.dy == vel3_3.dy && vel3_1.dz == vel3_3.dz);

    // Test parent 
    ecs.appendChild(entity1, entity2);
    assert(ecs.getParentEntityID(entity2) == entity1);
    assert(ecs.getChildEntityIDs(entity1).at(0) == entity2);

    assert(ecs.getParentEntityID(entity3) == BasicECS::RootEntityID);

    ecs.removeEntity(entity1);

    ecs.forEachEntity([](BasicECS::EntityID entityID){
        std::cout << entityID << "\n";
    });

    int iterations = 0;

    ecs.forEach<Body>([&iterations](Body pos, BasicECS::EntityID ent){
        iterations++;
    });

    std::cout << "iterations: " << iterations << "\n";

    ecs.displayECS();

    // Test if all components are cleared 
    ecs.clear();

    int positionCount = 0;
    int velocityCount = 0;
    int bodyCount = 0;

    ecs.forEach<Position>([&positionCount](Position& pos) {
        positionCount++;
    });
    ecs.forEach<Velocity>([&velocityCount](Velocity& vel) {
        velocityCount++;
    });
    ecs.forEach<Body>([&bodyCount](Body& body) {
        bodyCount++;
    });

    assert(positionCount == 0 && velocityCount == 0 && bodyCount == 0);

    std::cout << "state after clearing\n";
    ecs.displayECS();

    // Count allocations 
    assert(allocation_count == 0);

    std::cout << "All ECS tests passed successfully." << std::endl;
}


int main() {
    // testECS();
    // createEntitiesTest();
    // addingComponentTest();
    // removingComponentTest();
    // referencesTest();
    // forEachTest();
    // serializationTest();
    // parentingTest();
    // clearingTest();
    // removingEntityTest();

    basicEcsSpeedTest(1000000);

    return 0;
}

bool createEntitiesTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1, entity2;
    ecs.addEntity(entity1);
    ecs.addEntity(entity2);

    assert(entity1 != entity2);

    int iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    assert(iterations == 2);

    return true;
}

bool addingComponentTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    Velocity &vel = ecs.getComponent<Velocity>(entity1);

    assert(vel.dx == 2 && vel.dy == 2 && vel.dz == 3);


    BasicECS::EntityID entity2;
    ecs.addEntity(entity2)
        .addComponent(Position{1,3,2.5});

    Position pos = ecs.getComponent<Position>(entity2);

    assert(pos.x == 1 && pos.y == 3 && pos.z == 2.5);


    ecs.addComponent<Velocity>(entity2, entity1);

    Velocity &vel_2 = ecs.getComponent<Velocity>(entity2);

    vel.dx = 4;

    assert(vel_2.dx == vel.dx && vel_2.dy == vel.dy && vel_2.dz == vel.dz);


    ecs.removeComponent<Position>(entity2);

    std::streambuf* originalBuffer = std::cout.rdbuf();
    std::ostringstream nullStream;
    std::cerr.rdbuf(nullStream.rdbuf());

    try {
        ecs.getComponent<Position>(entity2); 
        assert(false); 
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
        assert(false); 
    } catch (...) {
        // Expected to fail, Position component does not exist
    }

    std::cerr.rdbuf(originalBuffer);

    int iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    assert(iterations == 0);

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

    assert(vel.dx == vel2.dx && vel.dy == vel2.dy && vel.dz == vel2.dz);

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

    assert(vel_1.dx == 3 && vel_1.dy == 1);
    assert(vel_2.dx == 1 && vel_2.dy == -5);

    ecs.forEach<Position, Velocity>([](Position &pos, Velocity &vel){
        vel.dz ++;
        pos.z ++;
    });

    assert(vel_1.dz == 3);
    assert(vel_2.dz == 3);

    Position &pos = ecs.getComponent<Position>(entity2);
    assert(pos.z == 3);

    return true;
}

bool serializationTest(){
    BasicECS::ECS ecs;

    BasicECS::EntityID entity1;
    ecs.addEntity(entity1)
        .addComponent(Velocity{2,2,3});

    std::size_t velocity_typeID = BasicECS::ECS::getTypeId<Velocity>();

    std::vector<uint8_t> serialized_vel = ecs.serializeComponent(velocity_typeID, entity1);

    BasicECS::EntityID entity2;
    ecs.addEntity(entity2);

    ecs.deserializeComponent(velocity_typeID, entity2, serialized_vel);

    Velocity vel = ecs.getComponent<Velocity>(entity2);

    assert(vel.dx == 2 && vel.dy == 2 && vel.dz == 3);

    ecs.addComponentType<Position>({ .serializeFunc = serializePosition, .deserializeFunc = deserializePosition});

    BasicECS::EntityID entity3;
    ecs.addEntity(entity3)
        .addComponent(Position{2,2,3});

    std::size_t position_typeID = BasicECS::ECS::getTypeId<Position>();

    std::vector<uint8_t> serialized_pos = ecs.serializeComponent(position_typeID, entity3);

    ecs.deserializeComponent(position_typeID, entity2, serialized_pos);

    Position pos = ecs.getComponent<Position>(entity2);

    assert(pos.x == 4 && pos.y == 2 && pos.z == 3);

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

    assert(ecs.getParentEntityID(entity1) == BasicECS::RootEntityID);
    assert(ecs.getParentEntityID(entity2) == entity1);
    assert(ecs.getChildEntityIDs(entity1).at(0) == entity2);
    assert(ecs.getChildEntityIDs(entity1).at(1) == entity3);

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

    assert(iterations == 0);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    assert(iterations == 0);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    assert(iterations == 0);

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

    assert(iterations == 2);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    assert(iterations == 1);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    assert(iterations == 1);

    ecs.appendChild(entity1, entity3);

    ecs.removeEntity(entity1);

    iterations = 0;

    ecs.forEachEntity([&iterations](BasicECS::EntityID entityID){
        iterations ++;
    });

    assert(iterations == 0);

    iterations = 0;

    ecs.forEach<Velocity>([&iterations](Velocity vel){
        iterations ++;
    });

    assert(iterations == 0);

    iterations = 0;

    ecs.forEach<Position>([&iterations](Position vel, BasicECS::EntityID entityID){
        iterations ++;
    });

    assert(iterations == 0);

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

    ecs.forEach<Velocity>([](Velocity &velocity, BasicECS::EntityID entityID){
        velocity.dx += 9;
    });

    std::cout << "BasicECS time: " << timeSinceEpochMillisec() - startTime << "ms\n";

}