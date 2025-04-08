#include "Scene.h"

void Scene::AddObject(std::unique_ptr<GameObject> obj) {
    objects.push_back(std::move(obj));
}

void Scene::Update() {
    for (auto& obj : objects) {
        obj->Update();
    }
}

void Scene::Render() {
    for (auto& obj : objects) {
        obj->Render();
    }
}