#pragma once

// Fwd Declares
class GameObject;

class BaseComponent {
public:
    BaseComponent(GameObject* owner);
    virtual void Init() = 0;
protected:
    GameObject* mOwner;
};