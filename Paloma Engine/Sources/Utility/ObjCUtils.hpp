
#pragma once
#include <objc/message.h>
#include <objc/runtime.h>
#include <type_traits>

template <typename T> bool is_kind_of(void *obj, Class cls) {
  if (!obj)
    return false;
  SEL isKindOfClassSel = sel_registerName("isKindOfClass:");
  return ((BOOL(*)(void *, SEL, Class))objc_msgSend)(obj, isKindOfClassSel,
                                                     cls);
}
