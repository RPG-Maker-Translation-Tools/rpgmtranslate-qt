use std::alloc::{GlobalAlloc, Layout};

unsafe extern "C" {
    fn rpgmtranslate_cpp_alloc(size: usize, align: usize) -> *mut u8;
    fn rpgmtranslate_cpp_dealloc(ptr: *mut u8, size: usize, align: usize);
    fn rpgmtranslate_cpp_realloc(ptr: *mut u8, old_size: usize, align: usize, new_size: usize) -> *mut u8;
}

struct CppAllocator;

unsafe impl GlobalAlloc for CppAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { rpgmtranslate_cpp_alloc(layout.size(), layout.align()) }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        unsafe { rpgmtranslate_cpp_dealloc(ptr, layout.size(), layout.align()) }
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        unsafe { rpgmtranslate_cpp_realloc(ptr, layout.size(), layout.align(), new_size) }
    }
}

#[global_allocator]
static GLOBAL: CppAllocator = CppAllocator;
