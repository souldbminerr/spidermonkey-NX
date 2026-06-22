import sys

path, addalloc = sys.argv[1], sys.argv[2] == "1"
lines = open(path).read().split("\n")
if any(l.strip() == "#![no_std]" for l in lines[:5]):
    print("already no_std:", path); sys.exit(0)
lines = [l.replace("::std::", "::core::") for l in lines]
i = 0
while i < len(lines):
    s = lines[i].strip()
    if s == "" or s.startswith("//") or s.startswith("#![") or s.startswith("/*") \
       or s.startswith("*") or s.endswith("*/"):
        i += 1; continue
    break
inject = []
if addalloc:
    inject = ["extern crate alloc;",
              "use alloc::boxed::Box;",
              "use alloc::vec::Vec;",
              "use alloc::string::String;"]
out = ["#![no_std]"] + lines[:i] + inject + lines[i:]
open(path, "w").write("\n".join(out))
print("patched", path, "(addalloc=%s) header-end=%d" % (addalloc, i))
