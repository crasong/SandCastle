#pragma once
#include "GameObject.h"
#include <iostream>

class Player : public GameObject {
public:
    void Update() override;
    void Render() override;
};