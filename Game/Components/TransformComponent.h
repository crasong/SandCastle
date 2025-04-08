#include "BaseComponent.h"
#include <glm/vec3.hpp>

class TransformComponent: public BaseComponent {
public:
    TransformComponent(GameObject* owner);
    virtual void Init() override;
protected:
    glm::vec3 mPosition;
};