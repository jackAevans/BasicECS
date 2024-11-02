#include "ecs.hpp"
#include <iostream>
#include <chrono>
#include <vector>

uint64_t timeSinceEpochMillisec();

void testECS(int amount);
void testControl(int amount);

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
    testECS(1000000);
    testControl(1000000);

    return 0;
}

void testECS(int amount){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>(nullptr, nullptr, nullptr);
    ecs.addComponentType<Health>(nullptr, nullptr, nullptr);

    for(int i = 0; i < amount; i++){
        EntityID testEnt;
        ecs.addEntity(testEnt)
            .addComponent<Position>(testEnt, {0, 1, 20})
            .addComponent<Health>(testEnt, Health{0});
    }

    uint64_t startTime = timeSinceEpochMillisec();

    ecs.forEach<Position, Health>([](Position &pos, Health &health){
        pos.x += 4;
        health.health += 9;
    });

    std::cout << "ECS time: " << timeSinceEpochMillisec() - startTime << "\n";

    ecs.removeComponentType<Position>();
    ecs.removeComponentType<Health>();
}

void testControl(int amount){
    std::vector<Player> players;

    for(int i = 0; i < amount; i++){
        players.push_back({0, 1, 20});
    }

    uint64_t startTime = timeSinceEpochMillisec();

    for(int i = 0; i < amount; i++){
        players.at(i).x += 4;
        players.at(i).health += 9;
    }

    std::cout << "Control time: "<< timeSinceEpochMillisec() - startTime << "\n";
}

uint64_t timeSinceEpochMillisec() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}