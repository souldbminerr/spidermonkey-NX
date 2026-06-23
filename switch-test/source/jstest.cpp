#include "bindings.h"
#include <cstdio>
#include <cstring>
#include <pthread.h>

#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/Initialization.h>
#include <js/SourceText.h>

static JSClass gGlobalClass = {"global", JSCLASS_GLOBAL_FLAGS,
                               &JS::DefaultGlobalClassOps};

static void eval(JSContext *cx, JS::HandleObject global, const char *code) {
  JSAutoRealm ar(cx, global);
  JS::CompileOptions opts(cx);
  opts.setFileAndLine("launcher", 1);
  JS::SourceText<mozilla::Utf8Unit> src;
  if (!src.init(cx, code, strlen(code), JS::SourceOwnership::Borrowed))
    return;
  JS::RootedValue rval(cx);
  if (!JS::Evaluate(cx, opts, src, &rval)) {
    JS_ClearPendingException(cx);
    return;
  }
  if (rval.isUndefined())
    return;
  JS::RootedString s(cx, JS::ToString(cx, rval));
  if (!s)
    return;
  if (JS::UniqueChars out = JS_EncodeStringToUTF8(cx, s))
    printf("%s = %s\n", code, out.get());
  consoleUpdate(nullptr);
}

static const char *kLauncher =
    "const DIRS = ['romfs:/scripts', 'sdmc:/switch/spidermonkey-nx'];\n"
    "function scan() {\n"
    "  const out = [];\n"
    "  for (const d of DIRS) {\n"
    "    let names = [];\n"
    "    try { names = nx.listFiles(d); } catch (e) {}\n"
    "    for (const n of names)\n"
    "      if (n.endsWith('.js')) out.push({ label: n, path: d + '/' + n });\n"
    "  }\n"
    "  out.sort((a, b) => a.label < b.label ? -1 : 1);\n"
    "  return out;\n"
    "}\n"
    "function waitRelease(mask) { while (nx.buttonsHeld() & mask) nx.sleep(16); }\n"
    "let scripts = scan();\n"
    "let sel = 0;\n"
    "let prev = nx.buttonsHeld();\n"
    "for (;;) {\n"
    "  const held = nx.buttonsHeld();\n"
    "  const hit = held & ~prev;\n"
    "  prev = held;\n"
    "  if (hit & nx.Plus) break;\n"
    "  if (hit & nx.X) { scripts = scan(); sel = 0; }\n"
    "  if (scripts.length) {\n"
    "    if (hit & nx.Down) sel = (sel + 1) % scripts.length;\n"
    "    if (hit & nx.Up) sel = (sel + scripts.length - 1) % scripts.length;\n"
    "    if (hit & nx.A) {\n"
    "      waitRelease(nx.A);\n"
    "      nx.clear();\n"
    "      try { nx.run(scripts[sel].path); }\n"
    "      catch (e) {\n"
    "        nx.clear();\n"
    "        nx.drawAt(2, 2, 'Error in ' + scripts[sel].label + ':');\n"
    "        nx.drawAt(2, 4, '' + e);\n"
    "        nx.drawAt(2, 6, 'Press A to return');\n"
    "        nx.present();\n"
    "        do { nx.sleep(16); } while (!(nx.buttonsHeld() & nx.A));\n"
    "      }\n"
    "      prev = nx.buttonsHeld();\n"
    "      if (sel >= scripts.length) sel = 0;\n"
    "    }\n"
    "  }\n"
    "  nx.clear();\n"
    "  nx.drawAt(2, 1, 'SpiderMonkey-NX');\n"
    "  nx.drawAt(2, 2, 'Up/Down: select   A: run   X: rescan   +: exit');\n"
    "  if (!scripts.length) {\n"
    "    nx.drawAt(4, 4, 'No .js files in romfs:/scripts or');\n"
    "    nx.drawAt(4, 5, 'sdmc:/switch/spidermonkey-nx');\n"
    "  } else {\n"
    "    for (let i = 0; i < scripts.length; i++)\n"
    "      nx.drawAt(4, 4 + i, (i === sel ? '> ' : '  ') + scripts[i].label);\n"
    "  }\n"
    "  nx.present();\n"
    "  nx.sleep(16);\n"
    "}\n";

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
        installNx(cx, global);
        eval(cx, global, kLauncher);
      }
      JS_DestroyContext(cx);
    }
    JS_ShutDown();
  }
  gDone = true;
  return nullptr;
}

int main(int, char **) {
  romfsInit();
  consoleInit(nullptr);
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  PadState pad;
  padInitializeDefault(&pad);

  consoleClear();
  printf("\x1b[2J\x1b[HSpiderMonkey-NX  loading...");
  consoleUpdate(nullptr);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 16 * 1024 * 1024);
  pthread_t t;
  pthread_create(&t, &attr, jsThread, nullptr);

  while (appletMainLoop() && !gDone) {
    padUpdate(&pad);
    g_buttonsDown = padGetButtonsDown(&pad);
    g_buttonsHeld = padGetButtons(&pad);
    svcSleepThread(8000000ULL);
  }

  pthread_join(t, nullptr);
  consoleExit(nullptr);
  return 0;
}
