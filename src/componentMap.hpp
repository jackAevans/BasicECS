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

#include "componentMap.tpp"