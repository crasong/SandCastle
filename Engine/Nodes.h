#pragma once

#include <Components.h>

// Nodes are used to group components together for processing in systems
// Because the ownership of the components belongs to the entity, the nodes are not responsible for destroying them
class Node{
public:
    Node() = default;
    ~Node() = default;
};

class MoveNode : public Node{
public:
    TransformComponent* mTransform = nullptr;
    VelocityComponent* mVelocity = nullptr;
};

class RenderNode : public Node{
public:
    TransformComponent* mTransform = nullptr;
    DisplayComponent* mDisplay = nullptr;
};

class UINode : public Node{
public:
    UIComponent* mUI = nullptr;
};