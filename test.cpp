#include <iostream>
#include <cassert>
#include <ecs.hpp> // Assume your ECS header file is named "ECS.hpp"

// Define some mock components and functions for testing
static int allocation_count = 0;

struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };

void initialisePosition(BasicECS::ECS &ecs, BasicECS::EntityID entity) { allocation_count ++; }
void cleanUpPosition(BasicECS::ECS &ecs, BasicECS::EntityID entity) { allocation_count --; }

void initialiseVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) { ecs.getComponent<Velocity>(entity).dx = 12; }
void cleanUpVelocity(BasicECS::ECS &ecs, BasicECS::EntityID entity) { /* No-op for now */ }

void testECS() {
    // Step 1: Create an ECS instance
    BasicECS::ECS ecs;

    // Step 2: Register component types
    ecs.addComponentType<Position>(initialisePosition, cleanUpPosition);
    ecs.addComponentType<Velocity>(initialiseVelocity, cleanUpVelocity);

    // Step 3: Add and retrieve entities
    BasicECS::EntityID entity1, entity2;
    ecs.addEntity(entity1);
    ecs.addEntity(entity2);
    assert(entity1 != entity2);

    // Step 4: Add components to entities
    Position pos1 = {10.0f, 20.0f, 30.0f};
    ecs.addComponent(entity1, pos1);

    Velocity vel1 = {1.0f, 0.5f, -1.0f};
    ecs.addComponent(entity1, vel1);

    // Adding only Position component to entity2
    Position pos2 = {15.0f, 25.0f, 35.0f};
    ecs.addComponent(entity2, pos2);

    // Step 5: Test getComponent functionality
    Position& retrievedPos1 = ecs.getComponent<Position>(entity1);
    assert(retrievedPos1.x == 10.0f && retrievedPos1.y == 20.0f && retrievedPos1.z == 30.0f);

    Velocity& retrievedVel1 = ecs.getComponent<Velocity>(entity1);
    assert(retrievedVel1.dx == 12.0f && retrievedVel1.dy == 0.5f && retrievedVel1.dz == -1.0f);

    Position& retrievedPos2 = ecs.getComponent<Position>(entity2);
    assert(retrievedPos2.x == 15.0f && retrievedPos2.y == 25.0f && retrievedPos2.z == 35.0f);

    // Step 6: Use forEach to verify iteration over components
    ecs.forEach<Position>([](Position& pos) {
        pos.x += 1.0f;  // Modify x component for all Position components
    });

    // Check that positions have been updated
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

    ecs.addComponent<Position>(entity1, entity2);

    // Final check: display ECS state if displayECS method is available
    ecs.displayECS();
    ecs.saveState("newState");

    // Step 9: Terminate and cleanup ECS
    ecs.terminate();

    // Step 10: Count allocations 
    assert(allocation_count == 0);
}

int main() {
    testECS();
    std::cout << "All ECS tests passed successfully." << std::endl;
    return 0;
}


double timeSinceEpochMillisec();

void ecsSpeedTest(int amount){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>(nullptr, nullptr);
    ecs.addComponentType<Velocity>(nullptr, nullptr);

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

    std::cout << "ECS time: " << timeSinceEpochMillisec() - startTime << "ms\n";
    
    ecs.terminate();
}

// void controlSpeedTest(int amount){
//     std::vector<Player> players;

//     for(int i = 0; i < amount; i++){
//         players.push_back({0, 1, 20});
//     }

//     uint64_t startTime = timeSinceEpochMillisec();

//     for(int i = 0; i < amount; i++){
//         players.at(i).health += 9;
//     }

//     std::cout << "Control time: "<< timeSinceEpochMillisec() - startTime << "ms\n";
// }

double timeSinceEpochMillisec() {
    using namespace std::chrono;
    uint64_t nano = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    return (double)nano/(double)1000000;
}