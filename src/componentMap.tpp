// #pragma once

// #include "componentMap.hpp"
// #include <iostream>

// template<typename ValueType>
// void ComponentMap<ValueType>::insert(std::size_t key, ValueType type){
//     map.insert({key, type});
// }

// template<typename ValueType>
// void ComponentMap<ValueType>::erase(std::size_t key){
//     map.erase(key);
// }

// template<typename ValueType>
// ValueType* ComponentMap<ValueType>::get(std::size_t key){
//     auto it = map.find(key);
//     if(it == map.end()){
//         return nullptr;
//     }
//     return &it->second;
// }

// template<typename ValueType>
// void ComponentMap<ValueType>::forEach(std::function<void(std::size_t key, ValueType value)> routine){
//     for (const auto& pair : map) {
//         routine(pair.first, pair.second);
//     }
// }

// template<typename ValueType>
// void ComponentMap<ValueType>::resize(std::size_t newCapacity){
//     capacity = newCapacity;
//     std::vector<std::tuple<std::size_t, ValueType, std::uint16_t>> tableCopy = table;
//     table.clear();
//     table.resize(capacity);
//     size = 0;
//     for(int i = 0; i < tableCopy.size(); i++){
//         std::tuple<std::size_t, ValueType, std::uint16_t> row = tableCopy.at(i);
//         if(std::get<2>(row) == 2){
//             insert(std::get<0>(row), std::get<1>(row));
//         }
//     }
//     std::cout << "resize" << std::endl;
// }

// template<typename ValueType>
// std::size_t ComponentMap<ValueType>::hash(std::size_t key){
//     return key; 
// }

#pragma once

#include "componentMap.hpp"
#include <iostream>

template<typename ValueType>
void ComponentMap<ValueType>::insert(std::size_t key, ValueType type){
    if(size >= capacity*loadFactor){
        resize(capacity * 2);
    }

    std::size_t index = hash(key) % capacity;
    std::tuple<std::size_t, ValueType, std::uint16_t> row = table.at(index);

    while(std::get<2>(row) == 2){
        if(std::get<0>(row) == key){
            table.at(index) = {key, type, 2};
            return;
        }

        index++;

        if (index >= capacity){
            index = 0;
        }

        row = table.at(index);
    }
    table[index] = std::make_tuple(key, type, 2);
    size ++;
}

template<typename ValueType>
void ComponentMap<ValueType>::erase(std::size_t key){
    if(size < capacity/2){
        resize(capacity/(2*loadFactor));
    }

    std::size_t index = hash(key) % capacity;
    std::size_t firstIndex = index;
    bool first = false;
    std::tuple<std::size_t, ValueType, std::uint16_t> row = table.at(index);

    while(std::get<0>(row) != key){
        if(std::get<2>(row) == 0 || (index == firstIndex && first)){
            return;
        }
        first = true;

        index++;
        if (index >= capacity){
            index = 0;
        }
        row = table.at(index);
    }

    size --;
    std::get<2>(table.at(index)) = 1;
}

template<typename ValueType>
ValueType* ComponentMap<ValueType>::get(std::size_t key){
    std::size_t index = hash(key) % capacity;
    std::size_t firstIndex = index;
    bool first = true;
    std::tuple<std::size_t, ValueType, std::uint16_t> row = table.at(index);

    while(true){
        if(std::get<2>(row) == 0 || (index == firstIndex && !first)){
            return nullptr;
        }
        if(std::get<2>(row) == 1 && std::get<0>(row) == key){
            return nullptr;
        }
        first = false;

        if(std::get<0>(row) == key){
            break;
        }

        index++;
        if (index >= capacity){
            index = 0;
        }
        row = table.at(index);
    }

    return &(std::get<1>(row));
}

template<typename ValueType>
void ComponentMap<ValueType>::resize(std::size_t newCapacity){
    capacity = newCapacity;
    std::vector<std::tuple<std::size_t, ValueType, std::uint16_t>> tableCopy = table;
    table.clear();
    table.resize(capacity);
    size = 0;
    for(int i = 0; i < tableCopy.size(); i++){
        std::tuple<std::size_t, ValueType, std::uint16_t> row = tableCopy.at(i);
        if(std::get<2>(row) == 2){
            insert(std::get<0>(row), std::get<1>(row));
        }
    }
}

template<typename ValueType>
std::size_t ComponentMap<ValueType>::hash(std::size_t key){
    return key; 
}

template<typename ValueType>
void ComponentMap<ValueType>::forEach(std::function<void(std::size_t key, ValueType value)> routine){
    for(int i = 0; i < table.size(); i++){
        std::tuple<std::size_t, ValueType, std::uint16_t> row = table.at(i);
        if(std::get<2>(row) == 2){
            routine(std::get<0>(row), std::get<1>(row));
        }
    }
}