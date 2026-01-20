/*
 * MTKDefines.hpp
 */

#pragma once

#include <Foundation/Foundation.hpp>

#ifdef __cplusplus
#define MTK_EXTERN extern "C"
#else
#define MTK_EXTERN extern
#endif

#define MTK_EXPORT __attribute__((visibility("default")))
