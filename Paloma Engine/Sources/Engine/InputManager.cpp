//
//  InputManager.cpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//


#include "InputManager.hpp"

InputManager& InputManager::shared() {
    static InputManager instance;
    return instance;
}

bool InputManager::isKeyPressed(uint16_t keyCode) const {
    return _pressedKeys.find(keyCode) != _pressedKeys.end();
}

simd_float2 InputManager::getMouseDelta() const {
    return _mouseDelta;
}

void InputManager::resetMouseDelta() {
    _mouseDelta = {0, 0};
}

void InputManager::onKeyDown(uint16_t keyCode) {
    _pressedKeys.insert(keyCode);
}

void InputManager::onKeyUp(uint16_t keyCode) {
    _pressedKeys.erase(keyCode);
}

void InputManager::onMouseMoved(float deltaX, float deltaY) {
    _mouseDelta.x += deltaX;
    _mouseDelta.y += deltaY;
}
