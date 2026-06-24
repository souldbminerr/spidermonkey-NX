#pragma once
#include <switch.h>
#include <jsapi.h>

extern volatile u64 g_buttonsDown;
extern volatile u64 g_buttonsHeld;

void installConsole(JSContext *cx, JS::HandleObject global);
void installNx(JSContext *cx, JS::HandleObject global);
void installGfx(JSContext *cx, JS::HandleObject nx);
bool gfxIsInited();  // true once nx.gfx.init() has taken over the window
