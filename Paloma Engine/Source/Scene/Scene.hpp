//
//  Scene.hpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#pragma once

#include "Types.hpp"
#include <simd/simd.h>
#include "Camera.hpp"

namespace Paloma {

class Renderer;


// -- Base Scene Class --
class Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;
    
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    
    // -- lifecycle --
    virtual void setup(Renderer* renderer) = 0;
    
    virtual void cleanup() {}
    
    // -- Frame --
    virtual void update(float dt) = 0;
    
    /// Render Scene
    virtual void render(Renderer* render) = 0;
    
    // --Camera--
    const Camera& camera() const { return _camera; }
    Camera& camera() { return _camera; }

protected:
    Camera _camera;
};
}
