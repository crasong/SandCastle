#pragma once

#include <Components.h>
#include <format>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

class Entity {
public:
    Entity() : mID(GetNextID()), mName(std::format("entity%i", mID)) {}
    Entity(const std::string& name) : mID(GetNextID()), mName(name) {}
    ~Entity() = default;
    // allow copying
    Entity(const Entity& other) {
        mID = other.mID;
        mName = other.mName;
        // iterate through the other entity's components and copy them to this entity
        for (const auto& [typeIndex, component] : other.mComponents) {
            mComponents[typeIndex] = std::make_unique<Component>(*component);
        }
    }
    Entity(Entity&& other ) {
        mID = other.mID;
        mName = other.mName;
        // iterate through the other entity's components and move them to this entity
        for (auto& [typeIndex, component] : other.mComponents) {
            mComponents[typeIndex] = std::move(component);
        }
        // clear the other entity's components
        other.mComponents.clear();
    }

    void PostRegistration();

    template<typename T, typename... Args>
    void AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        std::type_index typeIndex = std::type_index(typeid(T));
        mComponents[typeIndex] = std::make_unique<T>(std::forward<Args>(args)...);
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
    bool HasComponent() const {
        std::type_index typeIndex = std::type_index(typeid(T));
        return mComponents.find(typeIndex) != mComponents.end();
    }

    void GetComponents(std::vector<IUIViewable*>& uiViewables, bool bVisibleOnly = false) const {
        for (const auto& [typeIndex, component] : mComponents) {
            if (!bVisibleOnly || component->mIsVisible) {
                uiViewables.push_back(static_cast<IUIViewable*>(component.get()));
            }
        }
    }

    const char* GetName() const {
        return mName.c_str();
    }
protected:
    uint32_t mID = 0; // Unique ID for the entity
    std::string mName = ""; // Name of the entity
    std::unordered_map<std::type_index, std::unique_ptr<Component>> mComponents;

    static uint32_t s_NextID; // Static variable to keep track of the next ID to assign
    static uint32_t GetNextID() {
        return s_NextID++;
    }
};