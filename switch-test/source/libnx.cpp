#include "bindings.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <js/Array.h>
#include <js/CallArgs.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/Object.h>
#include <js/PropertyAndElement.h>
#include <js/SourceText.h>

volatile u64 g_buttonsDown = 0;
volatile u64 g_buttonsHeld = 0;
volatile s32 g_lstickX = 0;
volatile s32 g_lstickY = 0;

static JS::UniqueChars argStr(JSContext *cx, JS::HandleValue v) {
  JS::RootedString s(cx, JS::ToString(cx, v));
  return s ? JS_EncodeStringToUTF8(cx, s) : nullptr;
}

static char *slurp(const char *path, size_t *outLen) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return nullptr;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) {
    fclose(f);
    return nullptr;
  }
  char *buf = (char *)malloc((size_t)n + 1);
  if (!buf) {
    fclose(f);
    return nullptr;
  }
  size_t rd = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[rd] = 0;
  if (outLen)
    *outLen = rd;
  return buf;
}

static bool nx_sleep(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double ms = 0;
  if (args.length() > 0 && !JS::ToNumber(cx, args[0], &ms))
    return false;
  svcSleepThread((s64)(ms * 1000000.0));
  args.rval().setUndefined();
  return true;
}

static bool nx_readFile(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (args.length() < 1) {
    JS_ReportErrorASCII(cx, "readFile(path)");
    return false;
  }
  JS::UniqueChars path = argStr(cx, args[0]);
  if (!path)
    return false;
  size_t len = 0;
  char *buf = slurp(path.get(), &len);
  if (!buf) {
    args.rval().setNull();
    return true;
  }
  JS::RootedString s(cx, JS_NewStringCopyN(cx, buf, len));
  free(buf);
  if (!s)
    return false;
  args.rval().setString(s);
  return true;
}

static bool nx_listFiles(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::UniqueChars dir = args.length() >= 1 ? argStr(cx, args[0]) : nullptr;
  if (!dir) {
    JS_ReportErrorASCII(cx, "listFiles(dir)");
    return false;
  }
  JS::RootedObject arr(cx, JS::NewArrayObject(cx, 0));
  if (!arr)
    return false;
  DIR *d = opendir(dir.get());
  if (d) {
    uint32_t idx = 0;
    struct dirent *e;
    while ((e = readdir(d)) != nullptr) {
      if (e->d_name[0] == '.')
        continue;
      JS::RootedString name(cx, JS_NewStringCopyZ(cx, e->d_name));
      if (!name) {
        closedir(d);
        return false;
      }
      JS::RootedValue v(cx, JS::StringValue(name));
      if (!JS_SetElement(cx, arr, idx++, v)) {
        closedir(d);
        return false;
      }
    }
    closedir(d);
  }
  args.rval().setObject(*arr);
  return true;
}

static bool nx_run(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::UniqueChars path = args.length() >= 1 ? argStr(cx, args[0]) : nullptr;
  if (!path) {
    JS_ReportErrorASCII(cx, "run(path)");
    return false;
  }
  size_t len = 0;
  char *src = slurp(path.get(), &len);
  if (!src) {
    JS_ReportErrorASCII(cx, "run: cannot open %s", path.get());
    return false;
  }
  const char pfx[] = "(()=>{";
  const char sfx[] = "\n})();";
  size_t wlen = sizeof(pfx) - 1 + len + sizeof(sfx) - 1;
  char *w = (char *)malloc(wlen + 1);
  if (!w) {
    free(src);
    JS_ReportOutOfMemory(cx);
    return false;
  }
  memcpy(w, pfx, sizeof(pfx) - 1);
  memcpy(w + sizeof(pfx) - 1, src, len);
  memcpy(w + sizeof(pfx) - 1 + len, sfx, sizeof(sfx) - 1);
  w[wlen] = 0;
  free(src);

  JS::CompileOptions opts(cx);
  opts.setFileAndLine(path.get(), 1);
  JS::SourceText<mozilla::Utf8Unit> source;
  bool ok = source.init(cx, w, wlen, JS::SourceOwnership::Borrowed);
  JS::RootedValue rval(cx);
  if (ok)
    ok = JS::Evaluate(cx, opts, source, &rval);
  free(w);
  if (!ok)
    return false;
  args.rval().set(rval);
  return true;
}

static bool nx_writeFile(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (args.length() < 2) {
    JS_ReportErrorASCII(cx, "writeFile(path, data)");
    return false;
  }
  JS::UniqueChars path = argStr(cx, args[0]);
  JS::UniqueChars data = argStr(cx, args[1]);
  if (!path || !data)
    return false;
  FILE *f = fopen(path.get(), "wb");
  bool ok = false;
  if (f) {
    ok = fwrite(data.get(), 1, strlen(data.get()), f) > 0 || strlen(data.get()) == 0;
    fclose(f);
  }
  args.rval().setBoolean(ok);
  return true;
}

static bool nx_buttonsDown(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNumber((double)g_buttonsDown);
  return true;
}

static bool nx_buttonsHeld(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNumber((double)g_buttonsHeld);
  return true;
}

static bool nx_time(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNumber((double)time(nullptr));
  return true;
}

static bool nx_ticks(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  args.rval().setNumber((double)armGetSystemTick());
  return true;
}

static bool nx_clear(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  consoleClear();
  args.rval().setUndefined();
  return true;
}

static bool nx_drawAt(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  double col = 1, row = 1;
  if (args.length() < 3 || !JS::ToNumber(cx, args[0], &col) ||
      !JS::ToNumber(cx, args[1], &row)) {
    JS_ReportErrorASCII(cx, "drawAt(x, y, text)");
    return false;
  }
  JS::UniqueChars text = argStr(cx, args[2]);
  if (!text)
    return false;
  printf("\x1b[%d;%dH%s", (int)row, (int)col, text.get());
  args.rval().setUndefined();
  return true;
}

static bool nx_present(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  consoleUpdate(nullptr);
  args.rval().setUndefined();
  return true;
}

static bool nx_print(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (gfxIsInited()) {
    args.rval().setUndefined();
    return true;
  }
  JS::UniqueChars s = args.length() >= 1 ? argStr(cx, args[0]) : nullptr;
  if (s) {
    printf("%s\n", s.get());
    FILE *f = fopen("sdmc:/switch/sandboxels/log.txt", "a");
    if (f) { fprintf(f, "%s\n", s.get()); fclose(f); }
  }
  args.rval().setUndefined();
  return true;
}

static bool nx_stick(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedObject o(cx, JS_NewPlainObject(cx));
  JS::RootedValue xv(cx, JS::NumberValue((double)g_lstickX / 32767.0));
  JS::RootedValue yv(cx, JS::NumberValue((double)g_lstickY / 32767.0));
  JS_SetProperty(cx, o, "x", xv);
  JS_SetProperty(cx, o, "y", yv);
  args.rval().setObject(*o);
  return true;
}

static bool nx_touch(JSContext *cx, unsigned argc, JS::Value *vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  HidTouchScreenState st = {0};
  int count = 0, x = 0, y = 0;
  if (hidGetTouchScreenStates(&st, 1) && st.count > 0) {
    count = st.count;
    x = st.touches[0].x;
    y = st.touches[0].y;
  }
  JS::RootedObject o(cx, JS_NewPlainObject(cx));
  JS::RootedValue cv(cx, JS::NumberValue(count));
  JS::RootedValue xv(cx, JS::NumberValue(x));
  JS::RootedValue yv(cx, JS::NumberValue(y));
  JS_SetProperty(cx, o, "count", cv);
  JS_SetProperty(cx, o, "x", xv);
  JS_SetProperty(cx, o, "y", yv);
  args.rval().setObject(*o);
  return true;
}

static const JSFunctionSpec nxFunctions[] = {
    JS_FN("touch", nx_touch, 0, 0),
    JS_FN("stick", nx_stick, 0, 0),
    JS_FN("print", nx_print, 1, 0),
    JS_FN("sleep", nx_sleep, 1, 0),
    JS_FN("readFile", nx_readFile, 1, 0),
    JS_FN("writeFile", nx_writeFile, 2, 0),
    JS_FN("buttonsDown", nx_buttonsDown, 0, 0),
    JS_FN("buttonsHeld", nx_buttonsHeld, 0, 0),
    JS_FN("time", nx_time, 0, 0),
    JS_FN("ticks", nx_ticks, 0, 0),
    JS_FN("clear", nx_clear, 0, 0),
    JS_FN("drawAt", nx_drawAt, 3, 0),
    JS_FN("present", nx_present, 0, 0),
    JS_FN("listFiles", nx_listFiles, 1, 0),
    JS_FN("run", nx_run, 1, 0),
    JS_FS_END};

void installNx(JSContext *cx, JS::HandleObject global) {
  JSAutoRealm ar(cx, global);
  JS::RootedObject nx(cx, JS_NewPlainObject(cx));
  JS_DefineFunctions(cx, nx, nxFunctions);

  static const struct {
    const char *name;
    u64 bit;
  } buttons[] = {
      {"A", HidNpadButton_A},       {"B", HidNpadButton_B},
      {"X", HidNpadButton_X},       {"Y", HidNpadButton_Y},
      {"L", HidNpadButton_L},       {"R", HidNpadButton_R},
      {"ZL", HidNpadButton_ZL},     {"ZR", HidNpadButton_ZR},
      {"Plus", HidNpadButton_Plus}, {"Minus", HidNpadButton_Minus},
      {"Up", HidNpadButton_Up},     {"Down", HidNpadButton_Down},
      {"Left", HidNpadButton_Left}, {"Right", HidNpadButton_Right},
  };
  for (auto &b : buttons) {
    JS::RootedValue v(cx, JS::NumberValue((double)b.bit));
    JS_SetProperty(cx, nx, b.name, v);
  }

  installGfx(cx, nx);

  JS::RootedValue v(cx, JS::ObjectValue(*nx));
  JS_SetProperty(cx, global, "nx", v);
}
