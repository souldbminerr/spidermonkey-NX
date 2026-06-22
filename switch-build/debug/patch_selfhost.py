import re, sys
p = "/root/sm/mozjs-128.13.0/js/src/vm/SelfHosting.cpp"
s = open(p).read()
if "g_sh_fail" in s:
    print("already patched"); sys.exit(0)

glob = ('extern "C" {\n'
        'volatile int g_sh_fail = 0;\n'
        'volatile unsigned long g_sh_xdrlen = 0, g_sh_srclen = 0, g_sh_complen = 0;\n'
        'volatile int g_sh_decodeok = -1;\n'
        '}\n\n')
anchor = "bool JSRuntime::initSelfHostingStencil(JSContext* cx,"
i = s.index(anchor)
s = s[:i] + glob + s[i:]

start = s.index(anchor)
b = s.index("{", start)
depth = 0; end = b
for j in range(b, len(s)):
    if s[j] == '{': depth += 1
    elif s[j] == '}':
        depth -= 1
        if depth == 0: end = j; break
body = s[b:end+1]

body = body.replace("if (xdrCache.Length() > 0) {",
                    "g_sh_xdrlen = (unsigned long)xdrCache.Length();\n  if (xdrCache.Length() > 0) {", 1)
body = body.replace("uint32_t compressedLen = GetCompressedSize();",
                    "uint32_t compressedLen = GetCompressedSize();\n  g_sh_srclen = srcLen; g_sh_complen = compressedLen;", 1)
body = body.replace("if (decodeOk) {",
                    "g_sh_decodeok = decodeOk ? 1 : 0;\n    if (decodeOk) {", 1)

n = [0]
def repl(m):
    n[0] += 1
    return "{ g_sh_fail = %d; return false; }" % n[0]
body = re.sub(r"return false;", repl, body)

s = s[:b] + body + s[end+1:]
open(p, "w").write(s)
print("patched; return-false sites:", n[0])
