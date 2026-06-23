#include "bindings.h"
#include <cstdio>
#include <js/CallArgs.h>
#include <js/Conversions.h>
#include <js/Object.h>
#include <js/PropertyAndElement.h>

static bool consoleLog(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  for (unsigned i = 0; i < args.length(); i++) {
    JS::RootedString s(cx, JS::ToString(cx, args[i]));
    if (!s)
      return false;
    JS::UniqueChars str = JS_EncodeStringToUTF8(cx, s);
    if (!str)
      return false;
    printf("%s%s", i ? " " : "", str.get());
  }
  printf("\n");
  consoleUpdate(nullptr);
  args.rval().setUndefined();
  return true;
}

static const JSFunctionSpec consoleFunctions[] = {
    JS_FN("log", consoleLog, 1, 0),
    JS_FN("info", consoleLog, 1, 0),
    JS_FN("warn", consoleLog, 1, 0),
    JS_FN("error", consoleLog, 1, 0),
    JS_FS_END};

void installConsole(JSContext *cx, JS::HandleObject global) {
  JSAutoRealm ar(cx, global);
  JS::RootedObject console(cx, JS_NewPlainObject(cx));
  JS_DefineFunctions(cx, console, consoleFunctions);
  JS::RootedValue v(cx, JS::ObjectValue(*console));
  JS_SetProperty(cx, global, "console", v);
}
