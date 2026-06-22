/* no_std port of mozglue-static for the Nintendo Switch (horizon) target.
 * Original: Mozilla, MPL-2.0. Provides the global allocator + panic handler. */
#![no_std]
extern crate alloc;

use arrayvec::ArrayString;
use core::alloc::{GlobalAlloc, Layout};
use core::cmp;
use core::ffi::{c_char, c_int, c_void};
use core::ops::Deref;

#[link(name = "wrappers")]
extern "C" {
    fn RustMozCrash(filename: *const c_char, line: c_int, reason: *const c_char) -> !;
}

fn str_truncate_valid(s: &str, mut mid: usize) -> &str {
    loop {
        if let Some(res) = s.get(..mid) {
            return res;
        }
        mid -= 1;
    }
}

#[derive(Debug, PartialEq)]
struct ArrayCString<const CAP: usize> {
    inner: ArrayString<CAP>,
}

impl<S: AsRef<str>, const CAP: usize> From<S> for ArrayCString<CAP> {
    fn from(s: S) -> Self {
        let s = s.as_ref();
        let len = cmp::min(s.len(), CAP - 1);
        let mut result = Self {
            inner: ArrayString::from(str_truncate_valid(s, len)).unwrap(),
        };
        result.inner.push('\0');
        result
    }
}

impl<const CAP: usize> Deref for ArrayCString<CAP> {
    type Target = str;
    fn deref(&self) -> &str {
        self.inner.as_str()
    }
}

#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
    let (filename, line) = if let Some(loc) = info.location() {
        (loc.file(), loc.line())
    } else {
        ("unknown.rs", 0)
    };
    let reason = ArrayCString::<512>::from("rust panic");
    let filename = ArrayCString::<512>::from(filename);
    unsafe {
        RustMozCrash(
            filename.as_ptr() as *const c_char,
            line as c_int,
            reason.as_ptr() as *const c_char,
        )
    }
}

/// Was a std panic-hook installer; now a no-op (panic handling is global).
#[no_mangle]
pub extern "C" fn install_rust_hooks() {}

extern "C" {
    fn malloc(size: usize) -> *mut c_void;
    fn free(ptr: *mut c_void);
    fn calloc(nmemb: usize, size: usize) -> *mut c_void;
    fn realloc(ptr: *mut c_void, size: usize) -> *mut c_void;
    fn memalign(align: usize, size: usize) -> *mut c_void;
}

pub struct GeckoAlloc;

#[inline(always)]
fn need_memalign(layout: Layout) -> bool {
    layout.align() > layout.size() || layout.align() > 16
}

unsafe impl GlobalAlloc for GeckoAlloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if need_memalign(layout) {
            memalign(layout.align(), layout.size()) as *mut u8
        } else {
            malloc(layout.size()) as *mut u8
        }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        free(ptr as *mut c_void)
    }
    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        if need_memalign(layout) {
            let ptr = self.alloc(layout);
            if !ptr.is_null() {
                core::ptr::write_bytes(ptr, 0, layout.size());
            }
            ptr
        } else {
            calloc(1, layout.size()) as *mut u8
        }
    }
    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        let new_layout = Layout::from_size_align_unchecked(new_size, layout.align());
        if need_memalign(new_layout) {
            let new_ptr = self.alloc(new_layout);
            if !new_ptr.is_null() {
                let size = cmp::min(layout.size(), new_size);
                core::ptr::copy_nonoverlapping(ptr, new_ptr, size);
                self.dealloc(ptr, layout);
            }
            new_ptr
        } else {
            realloc(ptr as *mut c_void, new_size) as *mut u8
        }
    }
}

#[global_allocator]
static A: GeckoAlloc = GeckoAlloc;
