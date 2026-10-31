#![warn(clippy::all, clippy::pedantic, clippy::nursery)]

// Its allocator calls into the C++ side, which the crate's own test binary doesn't have
#[cfg(not(test))]
mod alloc;
mod api;
mod count;
mod ffi;

pub use ffi::*;
