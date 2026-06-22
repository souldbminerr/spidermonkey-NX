#include <switch.h>
#include <cstdio>
#include <cstring>
#include <pthread.h>

#include <jsapi.h>
#include <js/Initialization.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/Conversions.h>
#include <js/PropertyAndElement.h>

static JSClass gGlobalClass = {"global", JSCLASS_GLOBAL_FLAGS,
                               &JS::DefaultGlobalClassOps};

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

static void installConsole(JSContext *cx, JS::HandleObject global) {
  JSAutoRealm ar(cx, global);
  JS::RootedObject console(cx, JS_NewPlainObject(cx));
  JS_DefineFunction(cx, console, "log", consoleLog, 1, 0);
  JS::RootedValue consoleVal(cx, JS::ObjectValue(*console));
  JS_SetProperty(cx, global, "console", consoleVal);
}

static void eval(JSContext *cx, JS::HandleObject global, const char *code) {
  JSAutoRealm ar(cx, global);
  JS::CompileOptions opts(cx);
  opts.setFileAndLine("jstest", 1);
  JS::SourceText<mozilla::Utf8Unit> src;
  if (!src.init(cx, code, strlen(code), JS::SourceOwnership::Borrowed))
    return;
  JS::RootedValue rval(cx);
  if (!JS::Evaluate(cx, opts, src, &rval)) {
    JS_ClearPendingException(cx);
    return;
  }
  JS::RootedString s(cx, JS::ToString(cx, rval));
  if (!s)
    return;
  if (JS::UniqueChars out = JS_EncodeStringToUTF8(cx, s))
    printf("%s = %s\n", code, out.get());
  consoleUpdate(nullptr);
}

static volatile bool gDone = false;
static void *jsThread(void *) {
  if (JS_Init()) {
    JSContext *cx = JS_NewContext(32 * 1024 * 1024);
    if (cx && JS::InitSelfHostedCode(cx)) {
      JS::RealmOptions options;
      JS::RootedObject global(
          cx, JS_NewGlobalObject(cx, &gGlobalClass, nullptr,
                                 JS::FireOnNewGlobalHook, options));
      if (global) {
        installConsole(cx, global);
        eval(cx, global, "20 + 22");
        eval(cx, global, "let s = 0; for (let i = 1; i <= 100; i++) s += i; s");
        eval(cx, global, "Math.sqrt(144)");
        eval(cx, global, "[1, 2, 3, 4, 5].reduce((a, b) => a * b, 1)");
        eval(cx, global, "function fact(n) { return n <= 1 ? 1 : n * fact(n - 1); } fact(10)");
        eval(cx, global, "console.log('console.log from', 'SpiderMonkey', 8 * 8)");
      }
      JS_DestroyContext(cx);
    }
    JS_ShutDown();
  }
  gDone = true;
  return nullptr;
}

int main(int, char **) {
  consoleInit(nullptr);
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);

  printf("SpiderMonkey-NX\n\n");
  consoleUpdate(nullptr);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 16 * 1024 * 1024);
  pthread_t t;
  if (pthread_create(&t, &attr, jsThread, nullptr) == 0)
    while (!gDone)
      svcSleepThread(50000000ULL);

  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      break;
    consoleUpdate(nullptr);
  }
  consoleExit(nullptr);
  return 0;
}
