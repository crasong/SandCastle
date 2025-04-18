#pragma once

#include <Components.h>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <memory>

class Entity {
public:
    template<typename T, typename... Args>
    void AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        std::type_index typeIndex = std::type_index(typeid(T));
        mComponents[typeIndex] = std::unique_ptr<Component>(new T(std::forward<Args>(args)...));
    }

    // Can return nullptr if the component is not found
    template<typename T>
    T* GetComponent() const {
        std::type_index typeIndex = std::type_index(typeid(T));
        auto it = mComponents.find(typeIndex);
        if (it != mComponents.end()) {
            return static_cast<T*>(it->second.get());
        }
        else {
            return nullptr;
        }
    }

    template<typename T>
    bool HasComponent() {
        std::type_index typeIndex = std::type_index(typeid(T));
        return mComponents.find(typeIndex) != mComponents.end();
    }
private:
    std::unordered_map<std::type_index, std::unique_ptr<Component>> mComponents;
};