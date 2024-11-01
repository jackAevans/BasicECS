#include "ecs.hpp"
#include <iostream>
#include <chrono>
#include <vector>

uint64_t timeSinceEpochMillisec();

void testECS(int amount);
void testControl(int amount);

struct Player { // 124 bytes
    uint64_t pos[15];
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
    testECS(1);
    // testControl(1000000);

    return 0;
}

void testECS(int amount){
    BasicECS::ECS ecs;

    ecs.addComponentType<Position>(nullptr, nullptr, nullptr);
    ecs.addComponentType<Health>(nullptr, nullptr, nullptr);

    uint32_t testEnt;
    ecs.addEntity(testEnt)
        .addComponent<Position>(testEnt, {1,1,1});

    uint32_t testEnt2;
    ecs.addEntity(testEnt2)
        .addComponent<Position>(testEnt2, testEnt)
        .addComponent<Health>(testEnt2, Health{100});

    ecs.displayECS();

    ecs.removeComponentType<Position>();
    ecs.removeComponentType<Health>();
}

void testControl(int amount){
    std::vector<Player> players;

    for(int i = 0; i < amount; i++){
        players.push_back({});
    }

    uint64_t startTime = timeSinceEpochMillisec();

    for(int i = 0; i < amount; i++){
        players.at(i).health ++;
    }

    std::cout << timeSinceEpochMillisec() - startTime << "\n";
}

uint64_t timeSinceEpochMillisec() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}