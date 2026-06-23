#pragma once
#include <switch.h>
#include <jsapi.h>

extern volatile u64 g_buttonsDown;
extern volatile u64 g_buttonsHeld;

void installConsole(JSContext *cx, JS::HandleObject global);
void installNx(JSContext *cx, JS::HandleObject global);
