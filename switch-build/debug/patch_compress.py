import sys
p = "/root/sm/mozjs-128.13.0/js/src/vm/Compression.cpp"
s = open(p).read()
if "g_zi_init" in s:
    print("already patched"); sys.exit(0)

anchor = "bool js::DecompressString(const unsigned char* inp, size_t inplen,"
i = s.index(anchor)
glob = ('extern "C" {\n'
        'volatile int g_zi_init = 99, g_zi_inflate = 99;\n'
        'volatile unsigned long g_zi_in0 = 0, g_zi_out0 = 0, g_zi_inplen = 0, g_zi_outlen = 0;\n'
        '}\n\n')
s = s[:i] + glob + s[i:]

s = s.replace(
    "  int ret = inflateInit(&zs);\n  if (ret != Z_OK) {",
    ("  g_zi_inplen = (unsigned long)inplen; g_zi_outlen = (unsigned long)outlen;\n"
     "  g_zi_in0 = inplen>=4 ? (unsigned long)(inp[0]|(inp[1]<<8)|(inp[2]<<16)|((unsigned long)inp[3]<<24)) : 0xDEAD;\n"
     "  int ret = inflateInit(&zs);\n"
     "  g_zi_init = ret;\n"
     "  if (ret != Z_OK) {"), 1)
s = s.replace(
    "  ret = inflate(&zs, Z_FINISH);\n  MOZ_ASSERT(ret == Z_STREAM_END);",
    ("  ret = inflate(&zs, Z_FINISH);\n  g_zi_inflate = ret;\n"
     "  g_zi_out0 = outlen>=4 ? (unsigned long)((unsigned char)out[0]|((unsigned char)out[1]<<8)|((unsigned char)out[2]<<16)|((unsigned long)(unsigned char)out[3]<<24)) : 0;\n"
     "  MOZ_ASSERT(ret == Z_STREAM_END);"), 1)

open(p, "w").write(s)
print("patched Compression.cpp")
