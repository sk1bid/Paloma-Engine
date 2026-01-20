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
class Mesh; // forward decl для ModelEntity

class Entity : public std::enable_shared_from_this<Entity> {
public:
    std::string name = "UnnamedEntity";
    
    // Transform: в Swift AffineTransform3D, в Metal обычно matrix_float4x4
    matrix_float4x4 transform;
    
    std::vector<std::shared_ptr<Entity>> children;
    std::weak_ptr<Entity> parent; // weak, чтобы не было retain cycle

    Entity() {
        transform = matrix_identity_float4x4;
    }
    
    virtual ~Entity() = default; // Виртуальный деструктор для полиморфизма

    // Computed property: worldTransform
    matrix_float4x4 worldTransform() {
        if (auto p = parent.lock()) {
            return matrix_multiply(p->worldTransform(), transform);
        }
        return transform;
    }

    // private func removeChild(_ child: Entity)
    // Делаем public или protected, так как C++ требует доступа
    void removeChild(std::shared_ptr<Entity> child) {
        // Удаляем ребенка из вектора children
        // Swift: children.removeAll { $0 === child }
        auto it = std::remove_if(children.begin(), children.end(),
            [&](const std::shared_ptr<Entity>& e) { return e == child; });
        
        if (it != children.end()) {
            children.erase(it, children.end());
            child->parent.reset(); // child.parent = nil
        }
    }

    // func removeFromParent()
    void removeFromParent() {
        if (auto p = parent.lock()) {
            p->removeChild(shared_from_this());
        }
    }

    // func addChild(_ child: Entity)
    void addChild(std::shared_ptr<Entity> child) {
        child->removeFromParent();
        children.push_back(child);
        child->parent = weak_from_this(); // child.parent = self
    }

    // func visitHierarchy(_ visitor: (Entity) -> Void)
    void visitHierarchy(std::function<void(Entity*)> visitor) {
        visitor(this);
        for (auto& child : children) {
            child->visitHierarchy(visitor);
        }
    }

    // func childNamed(_ name: String, recursively: Bool = true) -> Entity?
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

// class ModelEntity : Entity
// Требует #include "Mesh.hpp" в реализации или .cpp файле
class ModelEntity : public Entity {
public:
    std::shared_ptr<Mesh> mesh; // var mesh: Mesh?
};
