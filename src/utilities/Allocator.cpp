#include <cstddef>
#include <cstring>
#include <new>

#ifdef ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

// The Rust `#[global_allocator]` (rust/src/alloc.rs) forwards here, so Rust uses this executable's allocator
// (mimalloc with ENABLE_MIMALLOC, otherwise the platform default) and no second copy is linked.
extern "C" {
auto rpgmtranslate_cpp_alloc(std::size_t size, std::size_t align) noexcept -> void* {
    return ::operator new(size, std::align_val_t{ align }, std::nothrow);
}

void rpgmtranslate_cpp_dealloc(void* ptr, std::size_t align) noexcept {
    ::operator delete(ptr, std::align_val_t{ align });
}

auto rpgmtranslate_cpp_realloc(void* ptr, std::size_t oldSize, std::size_t align, std::size_t newSize) noexcept
    -> void* {
#ifdef ENABLE_MIMALLOC
    // mimalloc can grow in place, but `mi_realloc` keeps only the default alignment: larger ones copy below.
    if (align <= alignof(std::max_align_t)) {
        return mi_realloc(ptr, newSize);
    }
#endif
    void* newPtr = ::operator new(newSize, std::align_val_t{ align }, std::nothrow);
    if (newPtr != nullptr) {
        std::memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
        ::operator delete(ptr, std::align_val_t{ align });
    }
    return newPtr;
}
}
