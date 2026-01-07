//
//  Types.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

// Basic Types Paloma Engine

#pragma once

#include <simd/simd.h>
#include <cstdint>

namespace Paloma {

// SIMD types Optimized for Metal Devices
using float2 = simd::float2;
using float3 = simd::float3;
using float4 = simd::float4;
using float4x4 = simd::float4x4;
using quatf = simd::quatf;

// Tripple Buffering
constexpr uint32_t MaxFramesInFlight = 3;
}
