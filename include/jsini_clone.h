#ifndef JSINI_CLONE_H
#define JSINI_CLONE_H

#include "jsini.h"

#ifdef __cplusplus
extern "C" {
#endif

jsini_value_t* jsini_clone(const jsini_value_t* value);

#ifdef __cplusplus
}
#endif

#endif // JSINI_CLONE_H
