#include "ecs.hpp"
#include <iostream>
#include <chrono>
#include <vector>

double timeSinceEpochMillisec();

void ecsSpeedTest(int amount);
void controlSpeedTest(int amount);
void testEcs();

struct Player { // 124 bytes
    int x;
    int y;
    int z;
    float health;
};

struct Position {
    int x;
    int y;
    int z;
};

struct Health {
    float health;
};

int main() {
    // ecsSpeedTest(1000000);
    // controlSpeedTest(1000000);
    testEcs();

    return 0;
}

void initPosition(BasicECS::ECS &ecs, BasicECS::EntityID entID){
    ecs.getComponent<Position>(entID).x = 20;
    std::cout << "init pos\n"; 
}

void cleanPosition(BasicECS::ECS &ecs, BasicECS::EntityID entID){
    ecs.getComponent<Position>(entID).x = 0;
    std::cout << "clean pos " << entID << "\n"; 
}

void testEcs(){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>(initPosition, cleanPosition, nullptr);
    ecs.addComponentType<Health>(nullptr, nullptr, nullptr);

    BasicECS::EntityID testEnt;
    ecs.addEntity(testEnt)
        .addComponent<Position>(testEnt, {0, 1, 20})
        .addComponent<Health>(testEnt, Health{0});

    BasicECS::EntityID testEnt2;
    ecs.addEntity(testEnt2)
        .addComponent<Position>(testEnt2, {5, 1, 8})
        .addComponent<Health>(testEnt2, Health{100});

    ecs.forEach<Position, Health>([](Position &pos, Health &health){
        pos.x += 4;
        health.health += 9;
    });

    ecs.removeEntity(testEnt);

    BasicECS::EntityID testEnt3;
    ecs.addEntity(testEnt3)
        .addComponent<Health>(testEnt3, Health{50});

    ecs.displayECS();

    ecs.terminate();

    ecs.removeComponentType<Position>();
    ecs.removeComponentType<Health>();
}

void ecsSpeedTest(int amount){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>(nullptr, nullptr, nullptr);
    ecs.addComponentType<Health>(nullptr, nullptr, nullptr);

    for(int i = 0; i < amount; i++){
        BasicECS::EntityID testEnt;
        ecs.addEntity(testEnt)
            .addComponent<Position>(testEnt, {0, 1, 20});

        if(i%1 == 0){
            ecs.addComponent<Health>(testEnt, Health{0});
        }
    }

    double startTime = timeSinceEpochMillisec();

    ecs.forEach<Health, Position>([]( Health &health, Position &pos){
        pos.x += 4;
        health.health += 9;
    });

    std::cout << "ECS time: " << timeSinceEpochMillisec() - startTime << "ms\n";

    ecs.removeComponentType<Position>();
    ecs.removeComponentType<Health>();
}

void controlSpeedTest(int amount){
    std::vector<Player> players;

    for(int i = 0; i < amount; i++){
        players.push_back({0, 1, 20});
    }

    uint64_t startTime = timeSinceEpochMillisec();

    for(int i = 0; i < amount; i++){
        players.at(i).x += 4;
        players.at(i).health += 9;
    }

    std::cout << "Control time: "<< timeSinceEpochMillisec() - startTime << "ms\n";
}

double timeSinceEpochMillisec() {
    using namespace std::chrono;
    uint64_t nano = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    return (double)nano/(double)1000000;
}