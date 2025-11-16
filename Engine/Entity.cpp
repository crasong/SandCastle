#include "Entity.h"

// Define the static member variable
std::atomic<uint32_t> Entity::s_NextID{0};

// We have to do this to ensure that the pointer is up to date.
void Entity::PostRegistration() {
    for (auto& [typeIndex, component] : mComponents) {
        if (component) {
            component->mEntity = this;
        }
    }
}