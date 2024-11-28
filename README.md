# <span style="color: #FF5733;"> BasicECS</span>

`BasicECS` is an ECS designed for fast component iteration that deals with many entities written in **modern c++**. The library is light weight, easy to use and easy to intergrate into your own project. The library is data oriented and cache freindly. 

## Features
- Adding and removing entities
- Adding and removing components 
- Iterating over entities with specific component archetypes   
- Resource managment when components are added/removed
- Shared components between entities 
- Entity hierarchy system 
- Component references 
- Globally unique IDs for entities
- Component serialization/deserialization 
- Automatically named component types

## Installation

Use `CMake` or your preferred build system:
```bash
git clone https://github.com/jackAevans/BasicECS.git
cd BasicECS
mkdir build && cd build
cmake ..
make
```

## Example

Adding entities with components and iteration over the entities:

```C++ 
struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };

int main(){
    BasicECS::ECS ecs;

    ecs.addEntity()
        .addComponent(Velocity{2, 2, 3});

    BasicECS::EntityID entityID;
    ecs.addEntity(entityID)
        .addComponent(Velocity{8, 1, 6})
        .addComponent(Position{0, 1, 3.5});

    std::cout << "Getting a single component:\n";

    Position &pos = getComponent<Position>(entityID);
    std::cout << "Position component: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";

    ecs.forEach<Velocity>([](Velocity &vel){
        vel.dx ++;
    });

    std::cout << "Iterating over entities with both Velocity and Position components:\n";

    ecs.forEach<Position, Velocity>([](Position pos, Velocity &vel){
        std::cout << "Position component: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
        std::cout << "Velocity component: " << vel.dx << ", " << vel.dy << ", " << vel.dz << "\n";
    });

}
```

Expected output:

``` bash
Getting a single component:
Position component: 0, 1, 3.5
Iterating over entities with both Velocity and Position components:
Position component: 0, 1, 3.5
Velocity component: 9, 1, 6
```