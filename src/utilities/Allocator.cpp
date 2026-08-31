#include <cstddef>
#include <cstring>
#include <new>

// The Rust static library's #[global_allocator] forwards here (see rust/src/alloc.rs) so that
// Rust allocations go through whichever allocator this executable was built with - mimalloc when
// ENABLE_MIMALLOC overrides ::operator new/delete, or the platform default otherwise - instead of
// linking a second, separate copy of mimalloc into the Rust side.
extern "C" {
void* rpgmtranslate_cpp_alloc(std::size_t size, std::size_t align) noexcept {
    return ::operator new(size, std::align_val_t{ align }, std::nothrow);
}

void rpgmtranslate_cpp_dealloc(void* ptr, std::size_t size, std::size_t align) noexcept {
    ::operator delete(ptr, size, std::align_val_t{ align });
}

void* rpgmtranslate_cpp_realloc(void* ptr, std::size_t oldSize, std::size_t align, std::size_t newSize) noexcept {
    void* newPtr = ::operator new(newSize, std::align_val_t{ align }, std::nothrow);
    if (newPtr != nullptr) {
        std::memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
        ::operator delete(ptr, oldSize, std::align_val_t{ align });
    }
    return newPtr;
}
}
