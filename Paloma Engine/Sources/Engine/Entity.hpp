//
//  Entity.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <simd/simd.h>

// Forward declaration
class Entity;
class Mesh;

class Entity : public std::enable_shared_from_this<Entity> {
public:
    std::string name = "UnnamedEntity";
    
    matrix_float4x4 transform;
    
    std::vector<std::shared_ptr<Entity>> children;
    std::weak_ptr<Entity> parent;

    Entity() {
        transform = matrix_identity_float4x4;
    }
    
    virtual ~Entity() = default;

    matrix_float4x4 worldTransform() {
        if (auto p = parent.lock()) {
            return matrix_multiply(p->worldTransform(), transform);
        }
        return transform;
    }

    void removeChild(std::shared_ptr<Entity> child) {
        auto it = std::remove_if(children.begin(), children.end(),
            [&](const std::shared_ptr<Entity>& e) { return e == child; });
        
        if (it != children.end()) {
            children.erase(it, children.end());
            child->parent.reset();
        }
    }

    void removeFromParent() {
        if (auto p = parent.lock()) {
            p->removeChild(shared_from_this());
        }
    }

    void addChild(std::shared_ptr<Entity> child) {
        child->removeFromParent();
        children.push_back(child);
        child->parent = weak_from_this();
    }

    void visitHierarchy(std::function<void(Entity*)> visitor) {
        visitor(this);
        for (auto& child : children) {
            child->visitHierarchy(visitor);
        }
    }

    std::shared_ptr<Entity> childNamed(const std::string& searchName, bool recursively = true) {
        for (auto& child : children) {
            if (child->name == searchName) {
                return child;
            } else if (recursively) {
                if (auto found = child->childNamed(searchName, recursively)) {
                    return found;
                }
            }
        }
        return nullptr;
    }
};

class ModelEntity : public Entity {
public:
    std::shared_ptr<Mesh> mesh;
};
