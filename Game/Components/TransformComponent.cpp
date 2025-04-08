#include "TransformComponent.h"
#include "../GameObject.h"

TransformComponent::TransformComponent(GameObject* owner): BaseComponent(owner) {
    mPosition = glm::vec3(50);
}