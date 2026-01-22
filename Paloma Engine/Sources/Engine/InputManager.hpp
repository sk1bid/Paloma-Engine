//
//  InputManager.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//
#include "simd/simd.h"
#include "unordered_set"

class InputManager {
public:
    static InputManager& shared();
    
    // Keyboard state (updated per frame)
    bool isKeyPressed(uint16_t keyCode) const;
    
    // Mouse delta (reset each frame)
    simd_float2 getMouseDelta() const;
    void resetMouseDelta();
    
    // Called from NSEvent handlers
    void onKeyDown(uint16_t keyCode);
    void onKeyUp(uint16_t keyCode);
    void onMouseMoved(float deltaX, float deltaY);
    
private:
    std::unordered_set<uint16_t> _pressedKeys;
    simd_float2 _mouseDelta = {0, 0};
};
