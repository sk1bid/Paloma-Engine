//
//  FlyCamera.cpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#include "FlyCamera.hpp"
#include <Carbon/Carbon.h>
#include <algorithm>

void FlyCamera::update(float deltaTime, const InputManager &input) {
  auto delta = input.getMouseDelta();
  yaw -= delta.x * lookSpeed;
  pitch -= delta.y * lookSpeed;
  pitch = std::clamp(pitch, -1.5f, 1.5f);

  simd_float3 fwd = forward();
  simd_float3 rgt = right();

  float speed = moveSpeed * deltaTime;

  if (input.isKeyPressed(kVK_Shift))
    speed *= 3.0f;

  if (input.isKeyPressed(kVK_ANSI_W))
    position += fwd * speed;
  if (input.isKeyPressed(kVK_ANSI_S))
    position -= fwd * speed;
  if (input.isKeyPressed(kVK_ANSI_A))
    position -= rgt * speed;
  if (input.isKeyPressed(kVK_ANSI_D))
    position += rgt * speed;
  if (input.isKeyPressed(kVK_ANSI_E))
    position.y += speed;
  if (input.isKeyPressed(kVK_ANSI_Q))
    position.y -= speed;
}

void FlyCamera::lookAt(simd_float3 target) {
  simd_float3 dir = simd_normalize(target - position);
  pitch = asinf(dir.y);
  yaw = atan2f(dir.x, dir.z);
}

simd_float3 FlyCamera::forward() const {
  return simd_make_float3(cosf(pitch) * sinf(yaw), sinf(pitch),
                          cosf(pitch) * cosf(yaw));
}

simd_float3 FlyCamera::right() const {
  simd_float3 up = simd_make_float3(0, 1, 0);
  return simd_normalize(simd_cross(forward(), up));
}

matrix_float4x4 FlyCamera::viewMatrix() const {
  simd_float3 target = position + forward();
  simd_float3 up = simd_make_float3(0, 1, 0);
  return matrix_look_at_right_hand(position, target, up);
}
