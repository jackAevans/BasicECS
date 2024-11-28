#pragma once 

#include <iostream>
#include <vector>
#include <functional>

template<typename ValueType>
class ComponentMap { 
public:
    ComponentMap() : capacity(5), size(0), loadFactor(0.75){
        table.resize(capacity);
    }

    void insert(std::size_t key, ValueType type);

    void erase(std::size_t key);

    ValueType* get(std::size_t key);

    void forEach(std::function<void(std::size_t key, ValueType value)> routine);

public: 
    void resize(std::size_t newCapacity);
    std::size_t hash(std::size_t key);

    std::size_t capacity;
    std::size_t size;
    float loadFactor;
    std::vector<std::tuple<std::size_t, ValueType, std::uint16_t>> table;

    std::unordered_map<std::size_t, ValueType> map;
};

template<typename ValueType>
void ComponentMap<ValueType>::insert(std::size_t key, ValueType type){
    map.insert({key, type});
}

template<typename ValueType>
void ComponentMap<ValueType>::erase(std::size_t key){
    map.erase(key);
}

template<typename ValueType>
ValueType* ComponentMap<ValueType>::get(std::size_t key){
    auto it = map.find(key);
    if(it == map.end()){
        return nullptr;
    }
    return &it->second;
}

template<typename ValueType>
void ComponentMap<ValueType>::forEach(std::function<void(std::size_t key, ValueType value)> routine){
    for (const auto& pair : map) {
        routine(pair.first, pair.second);
    }
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