#![allow(unsafe_op_in_unsafe_fn)]
use crate::api::*;
use crate::count::{TextCounts, count_text};
use language_tokenizer::{Algorithm, MatchResult};
#[cfg(feature = "languagetool")]
use languagetool_rust::api::check::{Data, DataAnnotation};
use log::{Log, Metadata, Record};
use rvpacker_txt_rs_lib::{
    BaseFlags, DuplicateMode, HashMap, HashMapExt, content_hash, EngineType, FileFlags, RPGMFileType,
    core::{PathSegment, Rm2kEntity, get_entity_values, get_rm2k_entity_name, get_rm2k_system_graphic_name},
    save::get_save_summary,
};
use std::{
    ffi::c_char,
    fs::{self},
    io::{Cursor, Read},
    mem,
    path::{Path, PathBuf},
    ptr, slice,
    sync::{Arc, LazyLock, Mutex, OnceLock, PoisonError},
    time::SystemTime,
};
use tokio::runtime::{Builder, Runtime};

pub fn from_bcp47(tag: &str) -> Algorithm {
    let primary = tag.split(['-', '_']).next().unwrap_or("");
    match primary.to_ascii_lowercase().as_str() {
        "ar" => Algorithm::Arabic,
        "hy" => Algorithm::Armenian,
        "eu" => Algorithm::Basque,
        "ca" => Algorithm::Catalan,
        "da" => Algorithm::Danish,
        "nl" => Algorithm::Dutch,
        "en" => Algorithm::English,
        "eo" => Algorithm::Esperanto,
        "et" => Algorithm::Estonian,
        "fi" => Algorithm::Finnish,
        "fr" => Algorithm::French,
        "de" => Algorithm::German,
        "el" => Algorithm::Greek,
        "hi" => Algorithm::Hindi,
        "hu" => Algorithm::Hungarian,
        "id" => Algorithm::Indonesian,
        "ga" => Algorithm::Irish,
        "it" => Algorithm::Italian,
        "lt" => Algorithm::Lithuanian,
        "ne" => Algorithm::Nepali,
        "no" | "nb" | "nn" => Algorithm::Norwegian,
        "pt" => Algorithm::Portuguese,
        "ro" => Algorithm::Romanian,
        "ru" => Algorithm::Russian,
        "sr" => Algorithm::Serbian,
        "es" => Algorithm::Spanish,
        "sv" => Algorithm::Swedish,
        "ta" => Algorithm::Tamil,
        "tr" => Algorithm::Turkish,
        "yi" => Algorithm::Yiddish,
        "ja" => Algorithm::Japanese,
        "zh" => Algorithm::Chinese,
        "ko" => Algorithm::Korean,
        "th" => Algorithm::Thai,
        "my" => Algorithm::Burmese,
        "lo" => Algorithm::Lao,
        "km" => Algorithm::Khmer,
        _ => Algorithm::None,
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Bitset2048 {
    data: [u8; 256],
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Selected {
    map_indices: Bitset2048,
    valid_indices: Bitset2048,
    map_count: u16,
    file_flags: FileFlags,
    padding: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
/// Byte buffer with FFI-compatible layout. Not for UTF-8 strings: use [`FFIString`].
///
/// If `cap == 0`, it is just a view.
pub struct ByteBuffer {
    pub ptr: *const u8,
    pub len: u32,
    pub cap: u32,
}

impl ByteBuffer {
    pub const fn null() -> Self {
        Self {
            ptr: ptr::null(),
            len: 0,
            cap: 0,
        }
    }
}

static TOKIO_RT: LazyLock<Runtime> = LazyLock::new(|| Builder::new_multi_thread().enable_all().build().unwrap());

#[repr(C)]
#[derive(Clone, Copy, Debug)]
/// UTF-8 string with FFI-compatible layout. Convert with [`str_to_ffi`] / [`ffi_to_str`]; [`FFIString::null`] is the
/// default. If `cap == 0`, it is just a view.
pub struct FFIString {
    pub ptr: *const c_char,
    pub len: u32,
    pub cap: u32,
}

impl FFIString {
    pub const fn null() -> Self {
        Self {
            ptr: ptr::null(),
            len: 0,
            cap: 0,
        }
    }
}

#[derive(Clone, Copy, Debug)]
#[repr(u8)]
pub enum ReadMode {
    Default,
    DefaultForce,
    AppendDefault,
    AppendForce,
}

#[inline]
unsafe fn ffi_to_str<'a>(string: FFIString) -> &'a str {
    if string.len == 0 {
        return "";
    }

    let slice = slice::from_raw_parts(string.ptr.cast::<u8>(), string.len as usize);

    // Invariant: C++ sends `QByteArray::toUtf8()` output, which is always valid UTF-8
    str::from_utf8_unchecked(slice)
}

#[inline]
fn str_to_ffi(str: &String) -> FFIString {
    let len = str.len();
    let ptr = str.as_ptr().cast::<c_char>();
    FFIString {
        ptr,
        len: len as u32,
        cap: str.capacity() as u32,
    }
}

#[inline]
unsafe fn ffi_opt_string(string: FFIString) -> Option<String> {
    if string.len == 0 {
        None
    } else {
        Some(ffi_to_str(string).to_string())
    }
}

#[inline]
unsafe fn ffi_opt_list(string: FFIString) -> Option<Vec<String>> {
    if string.len == 0 {
        None
    } else {
        Some(ffi_to_str(string).split(',').map(str::to_owned).collect())
    }
}

/// Resolves a WHATWG label to an [`encoding_rs::Encoding`] for the processor's read/write encodings. `""` means
/// UTF-8 on both the read and the write side.
#[inline]
unsafe fn ffi_encoding(string: FFIString) -> Option<&'static encoding_rs::Encoding> {
    if string.len == 0 {
        None
    } else {
        encoding_rs::Encoding::for_label(ffi_to_str(string).as_bytes())
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_string_free(ffi_string: FFIString) {
    if ffi_string.cap == 0 {
        return;
    }

    let _ = Vec::from_raw_parts(
        ffi_string.ptr.cast_mut(),
        ffi_string.len as usize,
        ffi_string.cap as usize,
    );
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_buffer_free(buffer: ByteBuffer) {
    if buffer.cap == 0 {
        return;
    }

    let _ = Vec::from_raw_parts(buffer.ptr.cast_mut(), buffer.len as usize, buffer.cap as usize);
}

// The last error message. `rpgm_error` is read from another thread than the one that failed, so each message is a
// leaked, immutable allocation: the published pointer stays valid, and the `Mutex` guards only the current one. A
// repeated message does not leak again.
static ERROR: Mutex<&'static str> = Mutex::new("");

fn set_error(message: String) {
    let mut current = ERROR.lock().unwrap_or_else(PoisonError::into_inner);

    if *current != message {
        *current = Box::leak(message.into_boxed_str());
    }
}

/// Borrows the current error message - `cap` is deliberately 0 so that passing
/// the result to [`rpgm_string_free`] is a no-op instead of freeing a leaked string.
#[unsafe(no_mangle)]
pub extern "C" fn rpgm_error() -> FFIString {
    let message = *ERROR.lock().unwrap_or_else(PoisonError::into_inner);

    FFIString {
        ptr: message.as_ptr().cast::<c_char>(),
        len: message.len() as u32,
        cap: 0,
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_read(
    source_path: FFIString,
    translation_path: FFIString,
    read_mode: ReadMode,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    selected: Selected,
    flags: BaseFlags,
    map_events: bool,
    hashes: ByteBuffer,
    ini_title: FFIString,
    read_encoding: FFIString,
    out_hashes: *mut ByteBuffer,
) -> bool {
    let result = (|| -> Result<_, Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);
        let ini_title = ffi_to_str(ini_title);
        let read_encoding = ffi_encoding(read_encoding);
        let read_mode = match read_mode {
            ReadMode::Default => rvpacker_txt_rs_lib::Mode::Read {
                append: false,
                force: false,
            },
            ReadMode::DefaultForce => rvpacker_txt_rs_lib::Mode::Read {
                append: false,
                force: true,
            },
            ReadMode::AppendDefault => rvpacker_txt_rs_lib::Mode::Read {
                append: true,
                force: false,
            },
            ReadMode::AppendForce => rvpacker_txt_rs_lib::Mode::Read {
                append: true,
                force: true,
            },
        };

        // One record is a 16-byte name and an 8-byte hash
        let mut hashes_map: HashMap<String, u64> = HashMap::with_capacity(hashes.len as usize / 24);

        // An empty buffer may have a null pointer, which `from_raw_parts` does not take even for length 0
        let hashes: &[u8] = if hashes.len == 0 {
            &[]
        } else {
            slice::from_raw_parts(hashes.ptr, hashes.len as usize)
        };
        let mut cursor = Cursor::new(hashes);

        let mut buf = [0u8; 16];
        let mut hash_buf = [0u8; 8];

        // Records of a 16-byte name, padded with NULs, and a hash. A name of all 16 bytes has no NUL.
        while cursor.read_exact(&mut buf).is_ok() && cursor.read_exact(&mut hash_buf).is_ok() {
            let name_len = buf.iter().position(|&byte| byte == 0).unwrap_or(buf.len());

            hashes_map.insert(
                String::from_utf8_lossy(&buf[..name_len]).into_owned(),
                u64::from_le_bytes(hash_buf),
            );
        }

        let out = read(
            Path::new(&source_path),
            Path::new(&translation_path),
            read_mode,
            engine_type,
            duplicate_mode,
            selected.file_flags,
            flags,
            map_events,
            hashes_map,
            ini_title,
            read_encoding,
        )?;

        Ok(out)
    })();

    match result {
        Ok(hashes) => {
            // A 16-byte name leaves no NUL in its record, and C++ reads names as C strings. Such a file keeps no
            // hash, so it is read again next time.
            let records: Vec<(&String, &u64)> = hashes.iter().filter(|(name, _)| name.len() < 16).collect();
            let mut buf = Vec::with_capacity(4 + (records.len() * 16) + (records.len() * size_of::<u64>()));

            buf.extend((records.len() as u32).to_le_bytes());

            for (key, value) in records {
                // The records have a name of 16 bytes, NUL padded
                let name = key.as_bytes();
                buf.extend(name);
                buf.resize(buf.len() + (16 - name.len()), 0);

                buf.extend(value.to_le_bytes());
            }

            *out_hashes = ByteBuffer {
                ptr: buf.as_ptr(),
                len: buf.len() as u32,
                cap: buf.capacity() as u32,
            };

            mem::forget(buf);
            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_write(
    source_path: FFIString,
    translation_path: FFIString,
    output_path: FFIString,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    flags: BaseFlags,
    selected: Selected,
    read_encoding: FFIString,
    write_encoding: FFIString,
    elapsed_out: *mut f32,
) -> bool {
    let result = (|| -> Result<f32, Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);
        let output_path = ffi_to_str(output_path);

        let elapsed = write(
            Path::new(&source_path),
            Path::new(&translation_path),
            Path::new(&output_path),
            engine_type,
            duplicate_mode,
            flags,
            selected.file_flags,
            ffi_encoding(read_encoding),
            ffi_encoding(write_encoding),
        )?;

        Ok(elapsed)
    })();

    match result {
        Ok(elapsed) => {
            *elapsed_out = elapsed;
            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_purge(
    source_path: FFIString,
    translation_path: FFIString,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    flags: BaseFlags,
    selected: Selected,
    read_encoding: FFIString,
    write_encoding: FFIString,
) -> bool {
    let result = (|| -> Result<(), Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);

        purge(
            Path::new(&source_path),
            Path::new(&translation_path),
            engine_type,
            duplicate_mode,
            flags,
            selected.file_flags,
            ffi_encoding(read_encoding),
            ffi_encoding(write_encoding),
        )?;

        Ok(())
    })();

    match result {
        Ok(_) => true,
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_extract_archive(
    input_path: FFIString,
    output_path: FFIString,
    read_encoding: FFIString,
) -> bool {
    let result = (|| -> Result<(), Error> {
        let input_path = ffi_to_str(input_path);
        let output_path = ffi_to_str(output_path);
        extract_archive(
            Path::new(&input_path),
            Path::new(&output_path),
            ffi_encoding(read_encoding),
        )?;
        Ok(())
    })();

    match result {
        Ok(()) => true,
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_translate_single(
    endpoint_settings: FFIString,
    source_language: FFIString,
    translation_language: FFIString,
    text: FFIString,
    out_string: *mut FFIString,
) -> bool {
    let text = ffi_to_str(text);
    let endpoint_settings = ffi_to_str(endpoint_settings);

    let result = (|| -> Result<_, Error> {
        let results = TOKIO_RT.block_on(async move {
            translate_single(
                endpoint_settings,
                ffi_to_str(source_language),
                ffi_to_str(translation_language),
                &text,
            )
            .await
        })?;

        Ok(results)
    })();

    match result {
        Ok(results) => {
            let str = str_to_ffi(&results);
            mem::forget(results);
            *out_string = str;
            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

fn take<'a>(buf: &'a [u8], pos: &mut usize, n: usize) -> &'a [u8] {
    let end = pos.checked_add(n).expect("a length in the buffer cannot overflow");
    let out = &buf[*pos..end];
    *pos = end;
    out
}

fn read_u32_le(buf: &[u8], pos: &mut usize) -> u32 {
    let bytes: [u8; 4] = take(buf, pos, 4).try_into().expect("take returns the bytes asked for");
    u32::from_le_bytes(bytes)
}

pub unsafe fn parse_strings<'a>(buf: &'a [u8]) -> Vec<&'a str> {
    let mut pos = 0;

    let count = read_u32_le(buf, &mut pos) as usize;
    let mut out = Vec::with_capacity(count);

    for _ in 0..count {
        let n = read_u32_le(buf, &mut pos) as usize;
        let bytes = take(buf, &mut pos, n);
        // Invariant: C++ builds these strings from `toUtf8()` output, which is always valid UTF-8
        let s = str::from_utf8_unchecked(bytes);
        out.push(s);
    }

    out
}

/// Appends a `u32` count, then one 12-byte record per match: offset, length and score (0 for exact matches).
fn push_matches(out: &mut Vec<u8>, matches: Vec<MatchResult>) {
    out.extend((matches.len() as u32).to_le_bytes());

    for m in matches {
        match m {
            MatchResult::Exact { offset, len } => {
                out.extend((offset as u32).to_le_bytes());
                out.extend((len as u32).to_le_bytes());
                out.extend(0f32.to_le_bytes());
            }
            MatchResult::Fuzzy { offset, len, score } => {
                out.extend((offset as u32).to_le_bytes());
                out.extend((len as u32).to_le_bytes());
                out.extend((score as f32).to_le_bytes());
            }
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_find_all_matches(
    source_haystack: FFIString,
    source_needle: FFIString,
    source_mode: MatchModeInfo,
    tr_haystack: FFIString,
    tr_needle: FFIString,
    tr_mode: MatchModeInfo,
    source_algorithm: FFIString,
    tr_algorithm: FFIString,
    out: *mut ByteBuffer,
) -> bool {
    let result = (|| -> Result<_, Error> {
        let source_haystack = ffi_to_str(source_haystack);
        let source_needle = ffi_to_str(source_needle);
        let tr_haystack = ffi_to_str(tr_haystack);
        let tr_needle = ffi_to_str(tr_needle);

        let m = find_all_matches(
            &source_haystack,
            &source_needle,
            source_mode,
            &tr_haystack,
            &tr_needle,
            tr_mode,
            from_bcp47(ffi_to_str(source_algorithm)),
            from_bcp47(ffi_to_str(tr_algorithm)),
        )?;

        let bytes = match m {
            None => Vec::new(),
            Some((src, tr)) => {
                let mut vec = Vec::with_capacity(
                    4 + (src.len() * (size_of::<MatchResult>() / 2)) + (tr.len() * (size_of::<MatchResult>() / 2)),
                );

                push_matches(&mut vec, src);
                push_matches(&mut vec, tr);

                vec
            }
        };

        Ok(bytes)
    })();

    match result {
        Ok(mut serialized) => {
            *out = ByteBuffer {
                ptr: serialized.as_mut_ptr(),
                len: serialized.len() as u32,
                cap: serialized.capacity() as u32,
            };

            mem::forget(serialized);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

type LogCallback = extern "C" fn(level: u8, str: FFIString);

static LOG_CALLBACK: OnceLock<LogCallback> = OnceLock::new();

struct FFILogger;

impl Log for FFILogger {
    fn enabled(&self, _: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        let Some(cb) = LOG_CALLBACK.get() else { return };

        let text = format!("[{}] {}", record.target(), record.args());

        // `log::Level` counts from 1 (Error), the C++ side from 0 (Error). The callback copies the message before it
        // returns, so it is a view (`cap == 0`) and the string is dropped here instead of leaking with every record.
        cb(
            record.level() as u8 - 1,
            FFIString {
                ptr: text.as_ptr().cast::<c_char>(),
                len: text.len() as u32,
                cap: 0,
            },
        );
    }

    fn flush(&self) {}
}

#[unsafe(no_mangle)]
pub extern "C" fn init_rust_logger(callback: LogCallback) {
    if LOG_CALLBACK.set(callback).is_err() {
        return;
    }

    log::set_logger(Box::leak(Box::new(FFILogger))).expect("logger already set");
    log::set_max_level(log::LevelFilter::Trace);

    log::info!("Rust FFI initialized");
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn init_lindera_dictionaries(data_root: FFIString) {
    let data_root = unsafe { ffi_to_str(data_root) };
    let base = Path::new(data_root).join("lindera-dictionaries");

    let existing_dir = |language: &str| -> Option<PathBuf> {
        let path = base.join(language);
        path.is_dir().then_some(path)
    };

    #[cfg(feature = "japanese-lindera")]
    language_tokenizer::set_dictionary_path(Algorithm::Japanese, existing_dir("japanese"));
    #[cfg(feature = "chinese-lindera")]
    language_tokenizer::set_dictionary_path(Algorithm::Chinese, existing_dir("chinese"));
    #[cfg(feature = "korean-lindera")]
    language_tokenizer::set_dictionary_path(Algorithm::Korean, existing_dir("korean"));

    #[cfg(not(any(
        feature = "japanese-lindera",
        feature = "chinese-lindera",
        feature = "korean-lindera"
    )))]
    let _ = existing_dir;
}

/// Counts the words and characters a player will read in `text`; RPG Maker codes are not counted.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_count_text(text: FFIString) -> TextCounts {
    count_text(ffi_to_str(text))
}

#[cfg(feature = "languagetool")]
#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_language_tool_lint(
    segments: ByteBuffer,
    base_url: FFIString,
    api_key: FFIString,
    language: FFIString,
    dicts: FFIString,
    disabled_categories: FFIString,
    disabled_rules: FFIString,
    enabled_categories: FFIString,
    enabled_rules: FFIString,
    picky: bool,
    mother_tongue: FFIString,
    preferred_variants: FFIString,
    username: FFIString,
    out_json: *mut FFIString,
) -> bool {
    let segment_bytes: &[u8] = if segments.len == 0 {
        &[]
    } else {
        slice::from_raw_parts(segments.ptr, segments.len as usize)
    };
    let segments = unsafe { parse_strings(segment_bytes) };

    let data: Data = segments
        .into_iter()
        .enumerate()
        .filter(|(_, seg)| !seg.is_empty())
        .map(|(i, seg)| {
            if i % 2 == 0 {
                DataAnnotation::new_text(seg)
            } else {
                DataAnnotation::new_markup(seg)
            }
        })
        .collect();

    let result: Result<Vec<_>, Error> = TOKIO_RT.block_on(async move {
        let decrypted = language_tool_lint(
            data,
            ffi_to_str(base_url),
            ffi_opt_string(api_key),
            ffi_to_str(language),
            ffi_opt_list(dicts),
            ffi_opt_list(disabled_categories),
            ffi_opt_list(disabled_rules),
            ffi_opt_list(enabled_categories),
            ffi_opt_list(enabled_rules),
            picky,
            ffi_opt_string(mother_tongue),
            ffi_opt_list(preferred_variants),
            ffi_opt_string(username),
        )
        .await?;
        Ok(decrypted)
    });

    match result {
        Ok(matches) => {
            let json = serde_json::to_string(&matches).expect("LanguageTool matches always serialize");

            *out_json = FFIString {
                ptr: json.as_ptr().cast::<i8>(),
                len: json.len() as u32,
                cap: json.capacity() as u32,
            };

            mem::forget(json);
            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_decrypt_asset(path: FFIString, out: *mut ByteBuffer) -> bool {
    let result = (|| -> Result<_, Error> {
        let decrypted = decrypt_asset(Path::new(&ffi_to_str(path)))?;
        Ok(decrypted)
    })();

    match result {
        Ok(decrypted) => {
            *out = ByteBuffer {
                ptr: decrypted.as_ptr(),
                len: decrypted.len() as u32,
                cap: decrypted.capacity() as u32,
            };

            mem::forget(decrypted);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[cfg(any(
    feature = "json-highlighting",
    feature = "js-highlighting",
    feature = "ruby-highlighting"
))]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_highlight_code(input: FFIString, lang: HighlightLanguage, out: *mut FFIString) -> bool {
    match highlight_code(ffi_to_str(input), lang) {
        Ok(html) => {
            *out = str_to_ffi(&html);
            mem::forget(html);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_generate_json(content: FFIString, filename: FFIString, json_out: *mut FFIString) -> bool {
    let result = (|| -> Result<_, Error> {
        // The content is a game file, which is binary for RPG Maker 2000/2003 and Ruby's Marshal: not a `&str`
        let bytes = if content.len == 0 {
            &[]
        } else {
            slice::from_raw_parts(content.ptr.cast::<u8>(), content.len as usize)
        };

        let json = generate_json(bytes, ffi_to_str(filename))?;
        Ok(json)
    })();

    match result {
        Ok(json) => {
            *json_out = str_to_ffi(&json);
            mem::forget(json);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_serde_export(content: FFIString, format: SerdeFormat, out: *mut ByteBuffer) -> bool {
    // A translation file edited outside the application is not guaranteed to be UTF-8
    let content = if content.len == 0 {
        Ok("")
    } else {
        str::from_utf8(slice::from_raw_parts(content.ptr.cast::<u8>(), content.len as usize))
    };

    let result = match content {
        Ok(content) => serde_export(content, format),
        Err(err) => Err(Error::Serde(err.to_string())),
    };

    match result {
        Ok(bytes) => {
            *out = ByteBuffer {
                ptr: bytes.as_ptr(),
                len: bytes.len() as u32,
                cap: bytes.capacity() as u32,
            };
            mem::forget(bytes);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_serde_import(bytes: ByteBuffer, format: SerdeFormat, out: *mut FFIString) -> bool {
    let slice: &[u8] = if bytes.len == 0 {
        &[]
    } else {
        slice::from_raw_parts(bytes.ptr, bytes.len as usize)
    };

    match serde_import(slice, format) {
        Ok(text) => {
            *out = str_to_ffi(&text);
            mem::forget(text);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_store_credential(account: FFIString, secret: FFIString) -> bool {
    match store_credential(ffi_to_str(account), ffi_to_str(secret)) {
        Ok(()) => true,
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_credential(account: FFIString, out: *mut FFIString) -> bool {
    match get_credential(ffi_to_str(account)) {
        Ok(secret) => {
            *out = str_to_ffi(&secret);
            mem::forget(secret);
            true
        }
        Err(_) => {
            *out = FFIString::null();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_delete_credential(account: FFIString) -> bool {
    match delete_credential(ffi_to_str(account)) {
        Ok(()) => true,
        // Nothing is stored under the account, so there is nothing to delete
        Err(Error::Keyring(keyring::Error::NoEntry)) => true,
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_ini_title(
    project_path: FFIString,
    out: *mut ByteBuffer,
    out_is_rm2k: *mut bool,
) -> bool {
    let project_path = ffi_to_str(project_path);

    let result = (|| -> Result<_, Error> {
        let title = get_ini_title(project_path)?;
        Ok(title)
    })();

    match result {
        Ok((title, is_rm2k)) => {
            *out = ByteBuffer {
                ptr: title.as_ptr(),
                len: title.len() as u32,
                cap: title.capacity() as u32,
            };
            *out_is_rm2k = is_rm2k;

            mem::forget(title);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_hash_file(content: ByteBuffer, duplicate_mode: DuplicateMode, out: *mut u64) {
    let content: &[u8] = if content.len == 0 {
        &[]
    } else {
        slice::from_raw_parts(content.ptr, content.len as usize)
    };
    *out = content_hash(duplicate_mode, content);
}

enum DataPath<'a> {
    Keys(std::str::Split<'a, char>),
    Key(&'a str),
    Index(usize),
}

fn parse_next_path(paths: &str) -> Option<DataPath<'_>> {
    let typ = *paths.as_bytes().first()?;

    // The type is a byte below 0x80, so the content starts on a character boundary
    let content = paths.get(1..)?;

    let path = match typ {
        1 => {
            if content.contains('\x02') {
                DataPath::Keys(content.split('\x02'))
            } else {
                DataPath::Key(content)
            }
        }
        3 => DataPath::Index(content.parse().unwrap_or(usize::MAX)),
        _ => return None,
    };

    Some(path)
}

/// The database file the last tooltip read, with its modification time. A tooltip looks up several entities of one
/// file, one after another.
static ENTITY_FILE_CACHE: Mutex<Option<(PathBuf, SystemTime, Arc<Vec<u8>>)>> = Mutex::new(None);

fn read_entity_file(path: &str) -> Result<Arc<Vec<u8>>, Error> {
    let io_error = |err| Error::Io(PathBuf::from(path), err);
    let modified = fs::metadata(path).and_then(|meta| meta.modified()).map_err(io_error)?;

    let mut cache = ENTITY_FILE_CACHE.lock().unwrap_or_else(PoisonError::into_inner);

    if let Some((cached_path, cached_modified, content)) = &*cache
        && cached_path == Path::new(path)
        && *cached_modified == modified
    {
        return Ok(Arc::clone(content));
    }

    let content = Arc::new(fs::read(path).map_err(io_error)?);
    *cache = Some((PathBuf::from(path), modified, Arc::clone(&content)));
    Ok(content)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_entity_data(
    path: FFIString,
    engine_type: EngineType,
    _file_type: RPGMFileType,
    paths: FFIString,
    read_encoding: FFIString,
    out: *mut FFIString,
    out_capacity: u32,
) -> bool {
    let path = ffi_to_str(path);
    let paths = ffi_to_str(paths);

    let result = (|| -> Result<_, Error> {
        let content = read_entity_file(path)?;

        let mut entries = paths.split('\x04').peekable();
        let mut steps: Vec<PathSegment<'_>> = Vec::new();
        let mut leaves: Vec<PathSegment<'_>> = Vec::new();

        while let Some(entry) = entries.next() {
            let Some(segment) = parse_next_path(entry) else {
                return Err(Error::InvalidKey(entry.to_string(), path.to_string()));
            };

            if entries.peek().is_none() {
                leaves = match segment {
                    DataPath::Keys(iter) => iter.map(PathSegment::Key).collect(),
                    DataPath::Key(key) => vec![PathSegment::Key(key)],
                    DataPath::Index(idx) => vec![PathSegment::Index(idx)],
                };
                break;
            }

            match segment {
                // Only the last segment can name several keys
                DataPath::Keys(_) => return Err(Error::InvalidKey(String::from(entry), path.to_string())),
                DataPath::Key(key) => steps.push(PathSegment::Key(key)),
                DataPath::Index(idx) => steps.push(PathSegment::Index(idx)),
            }
        }

        let values = get_entity_values(&content, engine_type, ffi_encoding(read_encoding), &steps, &leaves)?;

        if values.len() > out_capacity as usize {
            return Err(Error::TooManyValues(values.len(), out_capacity as usize));
        }

        for (out_idx, string) in values.into_iter().enumerate() {
            *out.add(out_idx) = FFIString {
                ptr: string.as_ptr().cast::<i8>(),
                len: string.len() as u32,
                cap: string.capacity() as u32,
            };

            mem::forget(string);
        }

        Ok(())
    })();

    match result {
        Ok(()) => true,
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

/// Hands `result` to the caller: the string goes to `out` (freed with `rpgm_string_free`), an error to `rpgm_error`.
unsafe fn write_string_result(result: Result<String, Error>, out: *mut FFIString) -> bool {
    match result {
        Ok(string) => {
            let mut string = string.into_bytes();

            *out = FFIString {
                ptr: string.as_mut_ptr().cast::<i8>(),
                len: string.len() as u32,
                cap: string.capacity() as u32,
            };

            mem::forget(string);

            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

/// Writes the name of entry `id` of `entity` in the RPG Maker 2000/2003 database at `path` to `out` (freed with
/// `rpgm_string_free`). `read_encoding` is the project's read encoding, empty for UTF-8.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_rm2k_entity_name(
    path: FFIString,
    entity: Rm2kEntity,
    id: i32,
    read_encoding: FFIString,
    out: *mut FFIString,
) -> bool {
    let path = ffi_to_str(path);

    let result = (|| -> Result<_, Error> {
        let content = read_entity_file(path)?;
        Ok(get_rm2k_entity_name(&content, entity, id, ffi_encoding(read_encoding))?)
    })();

    write_string_result(result, out)
}

/// Writes the name of the system graphic the RPG Maker 2000/2003 database at `path` selects to `out` (freed with
/// `rpgm_string_free`). `read_encoding` is as for `rpgm_get_rm2k_entity_name`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_rm2k_system_graphic_name(
    path: FFIString,
    read_encoding: FFIString,
    out: *mut FFIString,
) -> bool {
    let path = ffi_to_str(path);

    let result = (|| -> Result<_, Error> {
        let content = read_entity_file(path)?;
        Ok(get_rm2k_system_graphic_name(&content, ffi_encoding(read_encoding))?)
    })();

    write_string_result(result, out)
}

/// Writes the JSON of a `SaveSummary` of the save file at `path` to `out` (freed with `rpgm_string_free`).
/// `read_encoding` is as for `rpgm_get_rm2k_entity_name`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_save_summary(
    path: FFIString,
    engine_type: EngineType,
    read_encoding: FFIString,
    out: *mut FFIString,
) -> bool {
    let path = ffi_to_str(path);

    let result = (|| -> Result<_, Error> {
        let content = fs::read(path).map_err(|err| Error::Io(Path::new(path).to_path_buf(), err))?;
        let summary = get_save_summary(&content, engine_type, ffi_encoding(read_encoding))?;
        Ok(serde_json::to_string(&summary)?)
    })();

    write_string_result(result, out)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn gxhash(ptr: *const std::ffi::c_void, len: usize) -> u64 {
    let bytes: &[u8] = if len == 0 {
        &[]
    } else {
        slice::from_raw_parts(ptr.cast::<u8>(), len)
    };

    #[cfg(feature = "gxhash")]
    {
        gxhash::gxhash64(bytes, 0)
    }
    #[cfg(not(feature = "gxhash"))]
    {
        rapidhash::v3::rapidhash_v3(bytes)
    }
}

#[cfg(any(feature = "js-formatting", feature = "ruby-formatting"))]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_format_src(src: FFIString, lang: HighlightLanguage, out: *mut FFIString) -> bool {
    let src = ffi_to_str(src);

    let result = (|| -> Result<String, Error> {
        let result = format_src(src, lang)?;
        Ok(result)
    })();

    match result {
        Ok(formatted) => {
            *out = FFIString {
                ptr: formatted.as_ptr().cast::<i8>(),
                len: formatted.len() as u32,
                cap: formatted.capacity() as u32,
            };

            mem::forget(formatted);
            true
        }
        Err(err) => {
            set_error(err.to_string());
            false
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(feature = "snowball")]
    use language_tokenizer::MatchMode;

    /// A view over `text`, as C++ passes it: `cap == 0`, so Rust never frees it.
    fn view(text: &str) -> FFIString {
        FFIString {
            ptr: text.as_ptr().cast::<c_char>(),
            len: text.len() as u32,
            cap: 0,
        }
    }

    /// Builds the buffer `parse_strings` reads: a `u32` count, then each string as a `u32` length and its bytes.
    fn pack(strings: &[&str]) -> Vec<u8> {
        let mut out = (strings.len() as u32).to_le_bytes().to_vec();

        for string in strings {
            out.extend((string.len() as u32).to_le_bytes());
            out.extend(string.as_bytes());
        }

        out
    }

    // Searching needs a tokenizer, which only a crate feature provides: these tests run with `snowball`, the one with
    // no heavy dependencies.
    #[cfg(feature = "snowball")]
    fn exact_mode() -> MatchModeInfo {
        MatchModeInfo {
            mode: MatchMode::Exact,
            case_sensitive: true,
            permissive: false,
        }
    }

    #[test]
    fn parse_strings_reads_what_cpp_packs() {
        let strings = ["", "abc", "日本語", "😀 emoji", "line\nbreak"];
        let buf = pack(&strings);

        assert_eq!(unsafe { parse_strings(&buf) }, strings);
    }

    #[test]
    fn parse_strings_reads_an_empty_list() {
        let buf = pack(&[]);

        assert!(unsafe { parse_strings(&buf) }.is_empty());
    }

    #[test]
    fn push_matches_layout_is_count_then_twelve_byte_records() {
        let mut out = Vec::new();
        push_matches(
            &mut out,
            vec![
                MatchResult::Exact { offset: 3, len: 4 },
                MatchResult::Fuzzy {
                    offset: 1,
                    len: 2,
                    score: 0.5,
                },
            ],
        );

        let mut expected = 2u32.to_le_bytes().to_vec();
        expected.extend(3u32.to_le_bytes());
        expected.extend(4u32.to_le_bytes());
        expected.extend(0f32.to_le_bytes());
        expected.extend(1u32.to_le_bytes());
        expected.extend(2u32.to_le_bytes());
        expected.extend(0.5f32.to_le_bytes());

        assert_eq!(out, expected);
        assert_eq!(out.len(), 4 + 2 * 12);
    }

    #[test]
    fn push_matches_writes_only_the_count_for_no_matches() {
        let mut out = Vec::new();
        push_matches(&mut out, Vec::new());

        assert_eq!(out, 0u32.to_le_bytes());
    }

    #[test]
    #[cfg(feature = "snowball")]
    fn find_all_matches_serializes_source_then_translation() {
        let (haystack, needle) = ("hello world hello", "hello");
        let (tr_haystack, tr_needle) = ("hola mundo hola", "hola");
        let mut out = ByteBuffer::null();

        let ok = unsafe {
            rpgm_find_all_matches(
                view(haystack),
                view(needle),
                exact_mode(),
                view(tr_haystack),
                view(tr_needle),
                exact_mode(),
                view("en"),
                view("es"),
                &mut out,
            )
        };
        assert!(ok, "{}", ERROR.lock().unwrap());

        let bytes = unsafe { slice::from_raw_parts(out.ptr, out.len as usize) }.to_vec();
        let word = |at: usize| u32::from_le_bytes(bytes[at..at + 4].try_into().unwrap());

        // Source: two matches, at characters 0 and 12, each 5 long and exact (score 0).
        assert_eq!((word(0), word(4), word(8), word(12)), (2, 0, 5, 0));
        assert_eq!((word(16), word(20), word(24)), (12, 5, 0));
        // Translation follows at once: two matches, at 0 and 11, each 4 long.
        assert_eq!((word(28), word(32), word(36), word(40)), (2, 0, 4, 0));
        assert_eq!((word(44), word(48)), (11, 4));
        assert_eq!(bytes.len(), 28 + 4 + 2 * 12);

        unsafe { rpgm_buffer_free(out) };
    }

    #[test]
    #[cfg(feature = "snowball")]
    fn find_all_matches_counts_characters_not_bytes_or_utf16_units() {
        // The emoji is 4 UTF-8 bytes and 2 UTF-16 units but one character: C++ converts from characters.
        let mut out = ByteBuffer::null();

        let ok = unsafe {
            rpgm_find_all_matches(
                view("😀 abc"),
                view("abc"),
                exact_mode(),
                view("x"),
                view("y"),
                exact_mode(),
                view("en"),
                view("en"),
                &mut out,
            )
        };
        assert!(ok);

        let bytes = unsafe { slice::from_raw_parts(out.ptr, out.len as usize) }.to_vec();
        let word = |at: usize| u32::from_le_bytes(bytes[at..at + 4].try_into().unwrap());

        assert_eq!((word(0), word(4), word(8)), (1, 2, 3));

        unsafe { rpgm_buffer_free(out) };
    }

    #[test]
    #[cfg(feature = "snowball")]
    fn find_all_matches_gives_an_empty_buffer_when_the_source_has_no_match() {
        let mut out = ByteBuffer::null();

        let ok = unsafe {
            rpgm_find_all_matches(
                view("hello world"),
                view("absent"),
                exact_mode(),
                view("hola"),
                view("hola"),
                exact_mode(),
                view("en"),
                view("es"),
                &mut out,
            )
        };

        assert!(ok);
        assert_eq!(out.len, 0);

        unsafe { rpgm_buffer_free(out) };
    }

    #[test]
    fn free_functions_accept_owned_and_view_values() {
        // Ownership passes to `rpgm_string_free`, as it does when Rust hands a string to C++ and C++ gives it back.
        let owned = mem::ManuallyDrop::new(String::from("owned"));
        let owned_ffi = str_to_ffi(&owned);
        unsafe {
            rpgm_string_free(owned_ffi);
            // Views (`cap == 0`) and the null value are no-ops.
            rpgm_string_free(view("view"));
            rpgm_string_free(FFIString::null());
        }

        let mut bytes = mem::ManuallyDrop::new(vec![1u8, 2, 3]);
        let buffer = ByteBuffer {
            ptr: bytes.as_mut_ptr(),
            len: bytes.len() as u32,
            cap: bytes.capacity() as u32,
        };
        unsafe {
            rpgm_buffer_free(buffer);
            rpgm_buffer_free(ByteBuffer::null());
        }
    }

    #[test]
    fn hash_file_is_stable_and_depends_on_mode_and_content() {
        let hash = |content: &[u8], mode| {
            let mut out = 0u64;
            let buffer = if content.is_empty() {
                ByteBuffer::null()
            } else {
                ByteBuffer {
                    ptr: content.as_ptr(),
                    len: content.len() as u32,
                    cap: 0,
                }
            };
            unsafe { rpgm_hash_file(buffer, mode, &mut out) };
            out
        };

        assert_eq!(
            hash(b"line<#>translation", DuplicateMode::Allow),
            content_hash(DuplicateMode::Allow, b"line<#>translation")
        );
        // Pinned: the hashes are saved in the settings of a project, so a change of the algorithm or of what goes into it
        // marks every file of every project as changed
        #[cfg(feature = "gxhash")]
        {
            assert_eq!(hash(b"line<#>translation", DuplicateMode::Allow), 10140132349371025813);
            assert_eq!(hash(b"line<#>translation", DuplicateMode::Remove), 1746919030700552829);
        }
        assert_eq!(hash(b"same", DuplicateMode::Allow), hash(b"same", DuplicateMode::Allow));
        assert_ne!(hash(b"same", DuplicateMode::Allow), hash(b"same", DuplicateMode::Remove));
        assert_ne!(hash(b"one", DuplicateMode::Allow), hash(b"two", DuplicateMode::Allow));
        // An empty file has a null pointer from C++: it must hash as empty content, not dereference it.
        assert_eq!(hash(b"", DuplicateMode::Allow), content_hash(DuplicateMode::Allow, b""));
    }

    #[test]
    fn from_bcp47_uses_the_primary_subtag_case_insensitively() {
        assert!(matches!(from_bcp47("en"), Algorithm::English));
        assert!(matches!(from_bcp47("en-US"), Algorithm::English));
        assert!(matches!(from_bcp47("pt_BR"), Algorithm::Portuguese));
        assert!(matches!(from_bcp47("RU"), Algorithm::Russian));
        assert!(matches!(from_bcp47("nb"), Algorithm::Norwegian));
        assert!(matches!(from_bcp47("nn"), Algorithm::Norwegian));
        assert!(matches!(from_bcp47("xx"), Algorithm::None));
        assert!(matches!(from_bcp47(""), Algorithm::None));
    }

    #[test]
    fn parse_next_path_reads_the_type_byte() {
        assert!(matches!(parse_next_path("\x01Name"), Some(DataPath::Key("Name"))));
        assert!(matches!(parse_next_path("\x01"), Some(DataPath::Key(""))));

        match parse_next_path("\x01a\x02b\x02c") {
            Some(DataPath::Keys(keys)) => assert_eq!(keys.collect::<Vec<_>>(), ["a", "b", "c"]),
            _ => panic!("a path with separators is a key list"),
        }

        assert!(matches!(parse_next_path("\x037"), Some(DataPath::Index(7))));
        // An index that is not a number can never match an element.
        assert!(matches!(parse_next_path("\x03abc"), Some(DataPath::Index(usize::MAX))));
        assert!(matches!(parse_next_path("\x03-1"), Some(DataPath::Index(usize::MAX))));

        assert!(parse_next_path("").is_none());
        assert!(parse_next_path("\x09x").is_none());
    }

    /// A directory of its own under the system one, removed with its content when dropped.
    struct TempDir(PathBuf);

    impl TempDir {
        fn new(name: &str) -> Self {
            let path = std::env::temp_dir().join(format!("rpgmtranslate-test-{name}-{}", std::process::id()));
            let _ = fs::remove_dir_all(&path);
            fs::create_dir_all(&path).unwrap();
            Self(path)
        }
    }

    impl Drop for TempDir {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.0);
        }
    }

    // The error message is process-wide, so everything that sets or reads it is one test: the others would race it.
    #[test]
    fn failures_are_reported_through_rpgm_error() {
        let _guard = error_guard();
        let dir = TempDir::new("error");
        let project = dir.0.to_str().unwrap();
        let mut out = ByteBuffer::null();
        let mut is_rm2k = false;

        // Neither `Game.ini` nor `RPG_RT.ini` exists
        let ok = unsafe { rpgm_get_ini_title(view(project), &mut out, &mut is_rm2k) };
        assert!(!ok);

        let error = rpgm_error();
        let message = unsafe { ffi_to_str(error) };
        assert_eq!(error.cap, 0, "the message is borrowed, never freed by C++");
        assert!(message.contains("RPG_RT.ini"), "{message}");

        // Freeing the borrowed message is a no-op
        unsafe { rpgm_string_free(error) };

        // The same message again keeps the allocation: it is published to other threads, and must not leak anew
        set_error(message.to_string());
        assert_eq!(rpgm_error().ptr, error.ptr);

        set_error(String::from("another message"));
        assert_eq!(unsafe { ffi_to_str(rpgm_error()) }, "another message");
    }

    #[test]
    fn entity_file_cache_follows_the_path_and_the_modification_time() {
        // The cache holds one file for the whole process, so the tests that read entities cannot overlap
        let _guard = error_guard();
        let dir = TempDir::new("entity");
        let first = dir.0.join("Actors.json");
        let second = dir.0.join("Items.json");
        let first_path = first.to_str().unwrap();

        fs::write(&first, b"one").unwrap();
        let modified = fs::metadata(&first).unwrap().modified().unwrap();

        let read = read_entity_file(first_path).unwrap();
        assert_eq!(read.as_slice(), b"one");

        // Unchanged file: the cached content is shared, not read again
        assert!(Arc::ptr_eq(&read, &read_entity_file(first_path).unwrap()));

        // A changed file is read again. The time is set by hand, as a fast rewrite may keep the coarse one.
        fs::write(&first, b"two!").unwrap();
        fs::OpenOptions::new()
            .write(true)
            .open(&first)
            .unwrap()
            .set_modified(modified + std::time::Duration::from_secs(10))
            .unwrap();

        let reread = read_entity_file(first_path).unwrap();
        assert_eq!(reread.as_slice(), b"two!");
        assert!(!Arc::ptr_eq(&read, &reread));

        // Another path is another file, whatever the cache holds
        fs::write(&second, b"items").unwrap();
        assert_eq!(read_entity_file(second.to_str().unwrap()).unwrap().as_slice(), b"items");
        assert_eq!(read_entity_file(first_path).unwrap().as_slice(), b"two!");

        assert!(read_entity_file(dir.0.join("Missing.json").to_str().unwrap()).is_err());
    }

    // Helpers for calling the exported functions as C++ does

    /// The error message is process-wide: whoever asserts on it holds this for the call and the read.
    fn error_guard() -> std::sync::MutexGuard<'static, ()> {
        static GUARD: Mutex<()> = Mutex::new(());
        GUARD.lock().unwrap_or_else(PoisonError::into_inner)
    }

    fn last_error() -> String {
        unsafe { ffi_to_str(rpgm_error()) }.to_owned()
    }

    fn view_bytes(bytes: &[u8]) -> ByteBuffer {
        if bytes.is_empty() {
            return ByteBuffer::null();
        }

        ByteBuffer {
            ptr: bytes.as_ptr(),
            len: bytes.len() as u32,
            cap: 0,
        }
    }

    /// Copies a string Rust handed out and gives it back, as C++ does.
    fn take_string(string: FFIString) -> String {
        let copy = unsafe { ffi_to_str(string) }.to_owned();
        unsafe { rpgm_string_free(string) };
        copy
    }

    fn take_bytes(buffer: ByteBuffer) -> Vec<u8> {
        let copy = if buffer.len == 0 {
            Vec::new()
        } else {
            unsafe { slice::from_raw_parts(buffer.ptr, buffer.len as usize) }.to_vec()
        };
        unsafe { rpgm_buffer_free(buffer) };
        copy
    }

    /// The files to leave out, which is what read, write and purge take.
    fn leaving_out(flags: FileFlags) -> Selected {
        Selected {
            map_indices: Bitset2048 { data: [0; 256] },
            valid_indices: Bitset2048 { data: [0; 256] },
            map_count: 0,
            file_flags: flags,
            padding: 0,
        }
    }

    fn all_but_actors() -> Selected {
        leaving_out(FileFlags::all().difference(FileFlags::Actors))
    }

    /// A game with one actor; returns its data folder.
    fn mv_game(dir: &TempDir) -> PathBuf {
        let source = dir.0.join("data");
        fs::create_dir_all(&source).unwrap();
        fs::write(
            source.join("Actors.json"),
            r#"[null,{"id":1,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Harold","nickname":"The Brave","note":"","profile":"A hero"}]"#,
        )
        .unwrap();
        source
    }

    fn read_game(source: &Path, translation: &Path, mode: ReadMode, hashes: ByteBuffer) -> (bool, ByteBuffer) {
        let (source, translation) = (source.to_str().unwrap(), translation.to_str().unwrap());
        let mut out = ByteBuffer::null();

        let ok = unsafe {
            rpgm_read(
                view(source),
                view(translation),
                mode,
                EngineType::MVMZ,
                DuplicateMode::Allow,
                all_but_actors(),
                BaseFlags::empty(),
                false,
                hashes,
                view("Test Game"),
                FFIString::null(),
                &mut out,
            )
        };

        (ok, out)
    }

    #[test]
    fn text_arriving_from_cpp_is_read_as_it_is_sent() {
        let empty = FFIString { ptr: ptr::null(), len: 0, cap: 0 };

        unsafe {
            assert_eq!(ffi_to_str(empty), "");
            assert_eq!(ffi_to_str(view("日本語")), "日本語");

            assert_eq!(ffi_opt_string(empty), None);
            assert_eq!(ffi_opt_string(view("key")), Some("key".to_owned()));

            assert_eq!(ffi_opt_list(empty), None);
            assert_eq!(ffi_opt_list(view("a,b,,c")), Some(vec!["a".into(), "b".into(), "".into(), "c".into()]));
            assert_eq!(ffi_opt_list(view("single")), Some(vec!["single".to_owned()]));

            assert!(ffi_encoding(empty).is_none());
            assert_eq!(ffi_encoding(view("shift_jis")), Some(encoding_rs::SHIFT_JIS));
            assert_eq!(ffi_encoding(view("windows-1252")), Some(encoding_rs::WINDOWS_1252));
            // A label that no encoding has reads as UTF-8, like no label at all
            assert!(ffi_encoding(view("not-an-encoding")).is_none());
        }
    }

    #[test]
    fn rust_strings_go_to_cpp_with_their_capacity() {
        let owned = String::from("handed over");
        let ffi = str_to_ffi(&owned);

        assert_eq!(ffi.len as usize, owned.len());
        assert_eq!(ffi.cap as usize, owned.capacity());
        assert_eq!(ffi.ptr.cast::<u8>(), owned.as_ptr());

        // Ownership goes with the value: it is freed by whoever holds it last
        mem::forget(owned);
        assert_eq!(take_string(ffi), "handed over");
    }

    #[test]
    fn the_four_read_modes_differ_in_what_they_keep() {
        let dir = TempDir::new("read-modes");
        let source = mv_game(&dir);
        let translation = dir.0.join("translation");
        let file = translation.join("actors.txt");

        let (ok, hashes) = read_game(&source, &translation, ReadMode::Default, ByteBuffer::null());
        assert!(ok);
        let hashes = take_bytes(hashes);
        // A count, then a record of a 16-byte NUL-padded name and an 8-byte hash
        assert_eq!(hashes.len(), 4 + 16 + 8);
        assert_eq!(u32::from_le_bytes(hashes[..4].try_into().unwrap()), 1);
        assert_eq!(&hashes[4..10], b"actors");
        assert!(hashes[10..20].iter().all(|&byte| byte == 0));
        assert_eq!(fs::read_to_string(&file).unwrap(), "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>");

        let mine = "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>";

        for (mode, kept) in [
            (ReadMode::Default, true),
            (ReadMode::DefaultForce, false),
            (ReadMode::AppendDefault, true),
            (ReadMode::AppendForce, true),
        ] {
            fs::write(&file, mine).unwrap();

            let (ok, out) = read_game(&source, &translation, mode, ByteBuffer::null());
            take_bytes(out);

            assert!(ok, "{mode:?}");
            assert_eq!(fs::read_to_string(&file).unwrap().contains("Harold<#>Гарольд"), kept, "{mode:?}");
        }
    }

    #[test]
    fn read_hashes_that_come_back_make_an_unchanged_file_be_skipped() {
        let dir = TempDir::new("read-hashes");
        let source = mv_game(&dir);
        let translation = dir.0.join("translation");

        let (ok, out) = read_game(&source, &translation, ReadMode::Default, ByteBuffer::null());
        assert!(ok);
        // What comes back is a count and then the records; what goes in is the records alone
        let hashes = take_bytes(out);

        // The game gets a second actor: with the old hashes the file is read again
        let first = fs::read_to_string(source.join("Actors.json")).unwrap();
        let actor = r#"{"id":2,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Alice","nickname":"","note":"","profile":""}"#;
        fs::write(source.join("Actors.json"), first.replace("}]", &format!("}},{actor}]"))).unwrap();

        let (ok, out) = read_game(&source, &translation, ReadMode::AppendDefault, view_bytes(&hashes[4..]));
        assert!(ok);
        let new_hashes = take_bytes(out);

        assert!(fs::read_to_string(translation.join("actors.txt")).unwrap().contains("Alice<#>"));
        assert_ne!(hashes[20..28], new_hashes[20..28]);

        // With the hashes it just returned, the file is unchanged since: nothing is read
        fs::write(translation.join("actors.txt"), "kept as it is").unwrap();
        let (ok, out) = read_game(&source, &translation, ReadMode::AppendDefault, view_bytes(&new_hashes[4..]));
        assert!(ok);
        assert_eq!(take_bytes(out), new_hashes);
        assert_eq!(fs::read_to_string(translation.join("actors.txt")).unwrap(), "kept as it is");
    }

    #[test]
    fn write_and_purge_work_through_the_ffi() {
        let dir = TempDir::new("write-purge");
        let source = mv_game(&dir);
        let translation = dir.0.join("translation");
        let output = dir.0.join("output");

        let (ok, out) = read_game(&source, &translation, ReadMode::Default, ByteBuffer::null());
        assert!(ok);
        take_bytes(out);

        fs::write(
            translation.join("actors.txt"),
            "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>",
        )
        .unwrap();

        let mut elapsed = -1.0f32;
        let ok = unsafe {
            rpgm_write(
                view(source.to_str().unwrap()),
                view(translation.to_str().unwrap()),
                view(output.to_str().unwrap()),
                EngineType::MVMZ,
                DuplicateMode::Allow,
                BaseFlags::empty(),
                all_but_actors(),
                FFIString::null(),
                view("utf-8"),
                &mut elapsed,
            )
        };

        assert!(ok);
        assert!(elapsed >= 0.0);
        assert!(fs::read_to_string(output.join("data/Actors.json")).unwrap().contains("Гарольд"));

        let ok = unsafe {
            rpgm_purge(
                view(source.to_str().unwrap()),
                view(translation.to_str().unwrap()),
                EngineType::MVMZ,
                DuplicateMode::Allow,
                BaseFlags::empty(),
                all_but_actors(),
                FFIString::null(),
                FFIString::null(),
            )
        };

        assert!(ok);
        let purged = fs::read_to_string(translation.join("actors.txt")).unwrap();
        assert!(purged.contains("Harold<#>Гарольд") && !purged.contains("The Brave"), "{purged}");
    }

    #[test]
    fn read_write_and_purge_report_what_went_wrong() {
        let _guard = error_guard();
        let dir = TempDir::new("failures");
        let missing = dir.0.join("data");
        let translation = dir.0.join("translation");

        let (ok, _) = read_game(&missing, &translation, ReadMode::Default, ByteBuffer::null());
        assert!(!ok);
        assert!(last_error().contains("data"), "{}", last_error());

        let mut elapsed = 0.0f32;
        let ok = unsafe {
            rpgm_write(
                view(missing.to_str().unwrap()),
                view(translation.to_str().unwrap()),
                view(dir.0.join("output").to_str().unwrap()),
                EngineType::MVMZ,
                DuplicateMode::Allow,
                BaseFlags::empty(),
                all_but_actors(),
                FFIString::null(),
                FFIString::null(),
                &mut elapsed,
            )
        };
        assert!(!ok);
        assert!(!last_error().is_empty());

        let ok = unsafe {
            rpgm_purge(
                view(missing.to_str().unwrap()),
                view(translation.to_str().unwrap()),
                EngineType::MVMZ,
                DuplicateMode::Allow,
                BaseFlags::empty(),
                all_but_actors(),
                FFIString::null(),
                FFIString::null(),
            )
        };
        assert!(!ok);
        assert!(last_error().contains("data"), "{}", last_error());
    }

    #[test]
    fn archives_are_extracted_through_the_ffi() {
        let _guard = error_guard();
        let dir = TempDir::new("archive");

        let entries = [rpgmad_lib::ArchiveEntry { path: b"Data\\Actors.rxdata", data: b"actors" }];
        let mut bytes = vec![0; rpgmad_lib::encrypted_buffer_size(&entries, rpgmad_lib::Engine::Older)];
        rpgmad_lib::encrypt(&entries, rpgmad_lib::Engine::Older, &mut bytes).unwrap();

        let archive = dir.0.join("Game.rgssad");
        fs::write(&archive, &bytes).unwrap();
        let out = dir.0.join("out");

        let ok = unsafe { rpgm_extract_archive(view(archive.to_str().unwrap()), view(out.to_str().unwrap()), view("shift_jis")) };
        assert!(ok);
        assert_eq!(fs::read(out.join("Data/Actors.rxdata")).unwrap(), b"actors");

        let garbage = dir.0.join("garbage.rgssad");
        fs::write(&garbage, b"nothing like an archive").unwrap();
        let ok = unsafe { rpgm_extract_archive(view(garbage.to_str().unwrap()), view(out.to_str().unwrap()), FFIString::null()) };
        assert!(!ok);
        assert!(!last_error().is_empty());
    }

    #[test]
    fn translating_is_refused_for_script_endpoints() {
        let _guard = error_guard();
        let mut out = FFIString::null();

        let ok = unsafe {
            rpgm_translate_single(
                view(r#"{"apiKey":"","yandexFolderId":"","endpoint":3}"#),
                view("ja"),
                view("en"),
                view("text"),
                &mut out,
            )
        };

        assert!(!ok);
        assert!(last_error().contains("Script endpoints"), "{}", last_error());
        assert!(out.ptr.is_null());

        let ok = unsafe { rpgm_translate_single(view("{"), view("ja"), view("en"), view("text"), &mut out) };
        assert!(!ok);
    }

    #[test]
    fn assets_are_decrypted_through_the_ffi() {
        let _guard = error_guard();
        let dir = TempDir::new("asset");

        let mut png = vec![0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, b'I', b'H', b'D', b'R'];
        png.extend([0, 0, 0, 4, 0, 0, 0, 4, 8, 6, 0, 0, 0, 9, 9, 9, 9]);
        let encrypted = rpgm_asset_decrypter_lib::encrypt(&png, rpgm_asset_decrypter_lib::FileType::Png, "d41d8cd98f00b204e9800998ecf8427e").unwrap();

        let path = dir.0.join("picture.rpgmvp");
        fs::write(&path, encrypted).unwrap();

        let mut out = ByteBuffer::null();
        assert!(unsafe { rpgm_decrypt_asset(view(path.to_str().unwrap()), &mut out) });

        // The fake header stays in front, and C++ skips it
        let decrypted = take_bytes(out);
        assert_eq!(&decrypted[16..], png);

        assert!(!unsafe { rpgm_decrypt_asset(view(dir.0.join("missing.rpgmvp").to_str().unwrap()), &mut out) });
        assert!(!last_error().is_empty());
    }

    #[test]
    fn game_files_are_shown_as_json_through_the_ffi() {
        let _guard = error_guard();
        let save = rvpacker_txt_rs_lib::save::dump(r#"{"gold":5}"#);
        let mut out = FFIString::null();

        assert!(unsafe { rpgm_generate_json(view(&save), view("file1.rpgsave"), &mut out) });
        let json: serde_json::Value = serde_json::from_str(&take_string(out)).unwrap();
        assert_eq!(json["gold"], 5);

        // Empty content has a null pointer from C++
        let mut out = FFIString::null();
        assert!(!unsafe { rpgm_generate_json(FFIString::null(), view("file1.rpgsave"), &mut out) });
        assert!(!unsafe { rpgm_generate_json(view("garbage"), view("Map0001.lmu"), &mut out) });
        assert!(!last_error().is_empty());
    }

    #[test]
    fn translation_files_are_exported_and_imported_through_the_ffi() {
        let _guard = error_guard();
        let text = "<!>ID<#>1\nHarold<#>Гарольд\nBob<#>";

        let mut exported = ByteBuffer::null();
        assert!(unsafe { rpgm_serde_export(view(text), SerdeFormat::Json, &mut exported) });
        let bytes = take_bytes(exported);
        assert!(String::from_utf8(bytes.clone()).unwrap().contains("Гарольд"));

        let mut imported = FFIString::null();
        assert!(unsafe { rpgm_serde_import(view_bytes(&bytes), SerdeFormat::Json, &mut imported) });
        assert_eq!(take_string(imported), text);

        // An empty file has a null pointer from C++: exported as no entries
        let mut empty = ByteBuffer::null();
        assert!(unsafe { rpgm_serde_export(FFIString::null(), SerdeFormat::Json, &mut empty) });
        assert_eq!(take_bytes(empty), b"[]");

        // Text that is not UTF-8 is refused, and so is an import with no entries
        let bad = [0xFFu8, 0xFE];
        let mut out = ByteBuffer::null();
        let invalid = FFIString { ptr: bad.as_ptr().cast::<c_char>(), len: 2, cap: 0 };
        assert!(!unsafe { rpgm_serde_export(invalid, SerdeFormat::Json, &mut out) });
        assert!(!last_error().is_empty());

        let mut out = FFIString::null();
        assert!(!unsafe { rpgm_serde_import(view_bytes(b"[]"), SerdeFormat::Json, &mut out) });
        assert!(last_error().contains("no entries"), "{}", last_error());
    }

    #[test]
    fn the_title_of_a_game_comes_from_its_ini() {
        let _guard = error_guard();
        let dir = TempDir::new("ini");
        let project = dir.0.to_str().unwrap();

        fs::write(dir.0.join("Game.ini"), "[Game]\nTitle=My Game\n").unwrap();
        let mut out = ByteBuffer::null();
        let mut is_rm2k = true;
        assert!(unsafe { rpgm_get_ini_title(view(project), &mut out, &mut is_rm2k) });
        assert_eq!(take_bytes(out), b"My Game");
        assert!(!is_rm2k);

        fs::remove_file(dir.0.join("Game.ini")).unwrap();
        fs::write(dir.0.join("RPG_RT.ini"), "[RPG_RT]\nGameTitle=Old Game\n").unwrap();
        let mut out = ByteBuffer::null();
        assert!(unsafe { rpgm_get_ini_title(view(project), &mut out, &mut is_rm2k) });
        assert_eq!(take_bytes(out), b"Old Game");
        assert!(is_rm2k);
    }

    #[test]
    fn entity_data_is_read_by_path() {
        let _guard = error_guard();
        let dir = TempDir::new("entity-data");
        let source = mv_game(&dir);
        let path = source.join("Actors.json");
        let path = path.to_str().unwrap();

        let read = |paths: &str, capacity: u32| -> (bool, Vec<String>) {
            let mut out = [FFIString::null(); 4];
            let ok = unsafe {
                rpgm_get_entity_data(
                    view(path),
                    EngineType::MVMZ,
                    RPGMFileType::Actors,
                    view(paths),
                    FFIString::null(),
                    out.as_mut_ptr(),
                    capacity,
                )
            };

            let values = out.into_iter().filter(|value| !value.ptr.is_null()).map(take_string).collect();
            (ok, values)
        };

        // The entry at index 1, then its name
        assert_eq!(read("\x031\x04\x01name", 4), (true, vec!["Harold".to_owned()]));
        // Several keys of the last segment come out in order
        assert_eq!(
            read("\x031\x04\x01name\x02nickname", 4),
            (true, vec!["Harold".to_owned(), "The Brave".to_owned()])
        );

        // More values than the caller made room for
        let (ok, values) = read("\x031\x04\x01name\x02nickname", 1);
        assert!(!ok && values.is_empty());
        assert!(last_error().contains("2 values"), "{}", last_error());

        // A segment of a type that does not exist, and a list of keys before the last segment
        let (ok, _) = read("\x09junk", 4);
        assert!(!ok);
        assert!(last_error().contains("Actors.json"), "{}", last_error());
        let (ok, _) = read("\x01name\x02nickname\x04\x01name", 4);
        assert!(!ok);

        // The file is gone
        fs::remove_file(path).unwrap();
        let (ok, _) = read("\x031\x04\x01name", 4);
        assert!(!ok);
    }

    #[test]
    fn entity_data_follows_keys_and_indices_down_to_a_leaf() {
        let _guard = error_guard();
        let dir = TempDir::new("entity-path");
        let path = dir.0.join("Items.json");
        fs::write(&path, r#"{"outer":{"inner":["zero","one"],"name":"leaf"}}"#).unwrap();
        let path = path.to_str().unwrap();

        let read = |paths: &str| -> (bool, Vec<String>) {
            let mut out = [FFIString::null(); 2];
            let ok = unsafe {
                rpgm_get_entity_data(
                    view(path),
                    EngineType::MVMZ,
                    RPGMFileType::Actors,
                    view(paths),
                    FFIString::null(),
                    out.as_mut_ptr(),
                    2,
                )
            };

            (ok, out.into_iter().filter(|value| !value.ptr.is_null()).map(take_string).collect())
        };

        // A key as a step, then a key as the leaf
        assert_eq!(read("\x01outer\x04\x01name"), (true, vec!["leaf".to_owned()]));
        // A key as a step, then an index as the leaf
        assert_eq!(read("\x01outer\x04\x01inner\x04\x031"), (true, vec!["one".to_owned()]));
    }

    #[cfg(all(feature = "snowball", not(any(feature = "japanese-icu", feature = "japanese-lindera"))))]
    #[test]
    fn matching_reports_a_tokenizer_that_is_not_compiled_in() {
        let _guard = error_guard();
        let mut out = ByteBuffer::null();

        // Japanese needs a feature the unit tests are not built with
        let ok = unsafe {
            rpgm_find_all_matches(
                view("text"),
                view("text"),
                exact_mode(),
                view("text"),
                view("text"),
                exact_mode(),
                view("ja"),
                view("ja"),
                &mut out,
            )
        };

        assert!(!ok);
        assert!(!last_error().is_empty());
    }

    #[test]
    fn rm2k_databases_that_are_not_there_or_not_databases_are_errors() {
        let _guard = error_guard();
        let dir = TempDir::new("rm2k");
        let path = dir.0.join("RPG_RT.ldb");
        let mut out = FFIString::null();

        assert!(!unsafe { rpgm_get_rm2k_entity_name(view(path.to_str().unwrap()), Rm2kEntity::Actors, 1, FFIString::null(), &mut out) });
        assert!(!unsafe { rpgm_get_rm2k_system_graphic_name(view(path.to_str().unwrap()), FFIString::null(), &mut out) });

        fs::write(&path, b"not a database").unwrap();
        assert!(!unsafe { rpgm_get_rm2k_entity_name(view(path.to_str().unwrap()), Rm2kEntity::Actors, 1, FFIString::null(), &mut out) });
        assert!(!unsafe { rpgm_get_rm2k_system_graphic_name(view(path.to_str().unwrap()), view("shift_jis"), &mut out) });
        assert!(!last_error().is_empty());
    }

    /// A server on the loopback that answers one request per entry of `replies` (status line, body) and hands back
    /// every request it got (head and body) when it has answered them all.
    #[cfg(any(feature = "languagetool", feature = "yandex-translate"))]
    fn stub_server(replies: Vec<(&'static str, String)>) -> (String, std::thread::JoinHandle<Vec<String>>) {
        use std::io::{Read, Write};

        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let url = format!("http://{}", listener.local_addr().unwrap());

        let handle = std::thread::spawn(move || {
            let mut requests = Vec::new();

            for (status, body) in replies {
                let (mut stream, _) = listener.accept().unwrap();
                let mut request = Vec::new();
                let mut chunk = [0u8; 4096];

                // The head, then as much body as it says there is
                loop {
                    let read = stream.read(&mut chunk).unwrap();
                    request.extend_from_slice(&chunk[..read]);

                    let text = String::from_utf8_lossy(&request).into_owned();

                    if let Some(split) = text.find("\r\n\r\n") {
                        let length = text[..split]
                            .lines()
                            .find_map(|line| line.to_ascii_lowercase().strip_prefix("content-length:").map(str::to_owned))
                            .and_then(|value| value.trim().parse::<usize>().ok())
                            .unwrap_or(0);

                        if request.len() >= split + 4 + length {
                            break;
                        }
                    }

                    if read == 0 {
                        break;
                    }
                }

                requests.push(String::from_utf8_lossy(&request).into_owned());

                let reply = format!(
                    "HTTP/1.1 {status}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
                    body.len()
                );
                stream.write_all(reply.as_bytes()).unwrap();
            }

            requests
        });

        (url, handle)
    }

    #[cfg(feature = "languagetool")]
    const LANGUAGETOOL_REPLY: &str = r#"{"software":{"name":"LanguageTool","version":"6.0","buildDate":"2024","apiVersion":1,"premium":false,"status":""},"warnings":{"incompleteResults":false},"language":{"name":"English (US)","code":"en-US","detectedLanguage":{"name":"English (US)","code":"en-US","confidence":0.9}},"matches":[{"message":"Possible spelling mistake found.","shortMessage":"Spelling mistake","replacements":[{"value":"hello"}],"offset":0,"length":4,"context":{"text":"helo world","offset":0,"length":4},"sentence":"helo world","type":{"typeName":"Other"},"rule":{"id":"MORFOLOGIK_RULE_EN_US","description":"Possible spelling mistake","issueType":"misspelling","category":{"id":"TYPOS","name":"Possible Typo"}},"ignoreForIncompleteSentence":false,"contextForSureMatch":0}]}"#;

    #[cfg(feature = "languagetool")]
    fn lint(base_url: &str, segments: &[&str]) -> Option<String> {
        let packed = pack(segments);
        let mut out = FFIString::null();

        unsafe {
            rpgm_language_tool_lint(
                view_bytes(&packed),
                view(base_url),
                FFIString::null(),
                view("en-US"),
                FFIString::null(),
                FFIString::null(),
                FFIString::null(),
                FFIString::null(),
                FFIString::null(),
                false,
                FFIString::null(),
                FFIString::null(),
                FFIString::null(),
                &mut out,
            )
        }
        .then(|| take_string(out))
    }

    #[cfg(feature = "languagetool")]
    #[test]
    fn language_tool_matches_come_back_as_json() {
        let _guard = error_guard();
        let (url, server) = stub_server(vec![("200 OK", LANGUAGETOOL_REPLY.to_owned())]);

        let json = lint(&url, &["helo world"]).expect("the server answered");

        let requests = server.join().unwrap();
        assert!(requests[0].starts_with("POST /v2/check"));
        // The text goes in the form, with the language it is checked as
        assert!(requests[0].contains("en-US"));
        assert!(requests[0].contains("helo"));

        assert!(json.contains("MORFOLOGIK_RULE_EN_US"));
        assert!(json.contains("Possible spelling mistake found."));
    }

    #[cfg(feature = "languagetool")]
    #[test]
    fn language_tool_marks_the_markup_segments_as_markup() {
        let _guard = error_guard();
        let (url, server) = stub_server(vec![("200 OK", LANGUAGETOOL_REPLY.to_owned())]);

        // Segments alternate between text and markup (the codes between the lines of a message)
        assert!(lint(&url, &["helo ", "\\C[2]", " world"]).is_some());

        let requests = server.join().unwrap();
        let request = &requests[0];
        assert!(request.contains("markup"), "{request}");
        assert!(request.contains("text"), "{request}");
    }

    #[cfg(feature = "languagetool")]
    #[test]
    fn language_tool_failures_are_errors_not_matches() {
        let _guard = error_guard();

        // The server answers, but with an error
        let (url, server) = stub_server(vec![("500 Internal Server Error", "{}".to_owned())]);
        assert!(lint(&url, &["helo world"]).is_none());
        assert!(!last_error().is_empty());
        server.join().unwrap();

        // The server answers, but not with what LanguageTool answers
        let (url, server) = stub_server(vec![("200 OK", "[1, 2, 3]".to_owned())]);
        assert!(lint(&url, &["helo world"]).is_none());
        server.join().unwrap();

        // Nobody listens: the port of a listener that is gone
        let gone = {
            let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
            format!("http://{}", listener.local_addr().unwrap())
        };
        assert!(lint(&gone, &["helo world"]).is_none());
    }

    #[cfg(feature = "yandex-translate")]
    fn yandex_translate(settings: &str) -> Option<String> {
        let mut out = FFIString::null();

        unsafe { rpgm_translate_single(view(settings), view("ja"), view("en"), view("konnichiwa"), &mut out) }
            .then(|| take_string(out))
    }

    #[cfg(feature = "yandex-translate")]
    #[test]
    fn yandex_translations_come_from_the_server_that_is_configured() {
        let _guard = error_guard();
        let (url, server) = stub_server(vec![("200 OK", r#"{"translations":[{"text":"hello"}]}"#.to_owned())]);

        let settings = format!(r#"{{"apiKey":"secret","yandexFolderId":"folder","yandexBaseUrl":"{url}","endpoint":1}}"#);
        assert_eq!(yandex_translate(&settings).as_deref(), Some("hello"));

        let requests = server.join().unwrap();
        assert!(requests[0].starts_with("POST /translate"), "{}", requests[0]);
        assert!(requests[0].contains("Api-Key secret"), "{}", requests[0]);
        // The request names the folder, the languages and the text
        assert!(requests[0].contains("folder"), "{}", requests[0]);
        assert!(requests[0].contains("konnichiwa"), "{}", requests[0]);
    }

    #[cfg(feature = "yandex-translate")]
    #[test]
    fn yandex_failures_are_errors_not_translations() {
        let _guard = error_guard();
        let settings = |url: &str| {
            format!(r#"{{"apiKey":"secret","yandexFolderId":"folder","yandexBaseUrl":"{url}","endpoint":1}}"#)
        };

        // The server refuses
        let (url, server) = stub_server(vec![("403 Forbidden", r#"{"message":"bad key"}"#.to_owned())]);
        assert_eq!(yandex_translate(&settings(&url)), None);
        assert!(last_error().contains("403"), "{}", last_error());
        server.join().unwrap();

        // The server answers without any translation
        let (url, server) = stub_server(vec![("200 OK", r#"{"translations":[]}"#.to_owned())]);
        assert_eq!(yandex_translate(&settings(&url)), None);
        server.join().unwrap();

        // The server answers with something else
        let (url, server) = stub_server(vec![("200 OK", "not json".to_owned())]);
        assert_eq!(yandex_translate(&settings(&url)), None);
        server.join().unwrap();

        // Nobody listens
        let gone = {
            let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
            format!("http://{}", listener.local_addr().unwrap())
        };
        assert_eq!(yandex_translate(&settings(&gone)), None);
    }

    /// A chunk of an LCF stream; every length here is below 128, so each is one byte.
    fn lcf_chunk(id: u8, data: &[u8]) -> Vec<u8> {
        assert!(data.len() < 0x80);
        let mut out = vec![id, data.len() as u8];
        out.extend_from_slice(data);
        out
    }

    /// `RPG_RT.ldb` with two actors, one variable and a system graphic.
    fn rm2k_database() -> Vec<u8> {
        let entry = |id: u8, name: &[u8]| -> Vec<u8> {
            let mut out = vec![id];
            out.extend(lcf_chunk(0x01, name));
            out.push(0);
            out
        };

        let mut actors = vec![2];
        actors.extend(entry(1, b"Harold"));
        actors.extend(entry(3, b"Alice"));

        let mut variables = vec![1];
        variables.extend(entry(3, b"Gold"));

        let mut database = vec![11];
        database.extend_from_slice(b"LcfDataBase");
        database.extend(lcf_chunk(0x0B, &actors));
        database.extend(lcf_chunk(0x16, &lcf_chunk(0x13, b"System2")));
        database.extend(lcf_chunk(0x18, &variables));
        database
    }

    #[test]
    fn rm2k_names_are_read_from_the_database() {
        let _guard = error_guard();
        let dir = TempDir::new("rm2k_names");
        let path = dir.0.join("RPG_RT.ldb");
        fs::write(&path, rm2k_database()).unwrap();
        let path = path.to_str().unwrap();

        let name = |entity: Rm2kEntity, id: i32| -> Option<String> {
            let mut out = FFIString::null();
            unsafe { rpgm_get_rm2k_entity_name(view(path), entity, id, FFIString::null(), &mut out) }
                .then(|| take_string(out))
        };

        assert_eq!(name(Rm2kEntity::Actors, 1).as_deref(), Some("Harold"));
        assert_eq!(name(Rm2kEntity::Actors, 3).as_deref(), Some("Alice"));
        assert_eq!(name(Rm2kEntity::Variables, 3).as_deref(), Some("Gold"));

        // An id the list does not have names the list in the error
        assert_eq!(name(Rm2kEntity::Actors, 2), None);
        assert!(last_error().contains("actors"));

        let mut graphic = FFIString::null();
        assert!(unsafe { rpgm_get_rm2k_system_graphic_name(view(path), FFIString::null(), &mut graphic) });
        assert_eq!(take_string(graphic), "System2");
    }

    #[test]
    fn rm2k_names_are_decoded_with_the_read_encoding() {
        let _guard = error_guard();
        let dir = TempDir::new("rm2k_encoding");
        let path = dir.0.join("RPG_RT.ldb");

        // "ハロルド" in Shift JIS
        let sjis = [0x83u8, 0x6E, 0x83, 0x8D, 0x83, 0x8B, 0x83, 0x68];
        let mut actors = vec![1, 1, 1, sjis.len() as u8];
        actors.extend_from_slice(&sjis);
        actors.push(0);

        let mut database = vec![11];
        database.extend_from_slice(b"LcfDataBase");
        database.extend(lcf_chunk(0x0B, &actors));
        fs::write(&path, database).unwrap();

        let mut out = FFIString::null();
        assert!(unsafe {
            rpgm_get_rm2k_entity_name(view(path.to_str().unwrap()), Rm2kEntity::Actors, 1, view("shift_jis"), &mut out)
        });
        assert_eq!(take_string(out), "ハロルド");
    }

    #[test]
    fn save_summaries_are_given_as_json() {
        let _guard = error_guard();
        let dir = TempDir::new("save");
        let path = dir.0.join("file1.rpgsave");

        let save = rvpacker_txt_rs_lib::save::dump(
            r#"{"system":{"_framesOnSave":7200},"map":{"_mapId":3},"party":{"_gold":250,"_actors":[1],"_items":{"2":5}},"actors":{"_data":[null,{"_actorId":1,"_name":"Harold","_nickname":"The Brave","_classId":2,"_level":4,"_hp":30,"_mp":8,"_tp":1}]},"switches":{"_data":[null,true,false]},"variables":{"_data":[null,0,42]}}"#,
        );
        fs::write(&path, save).unwrap();

        let mut out = FFIString::null();
        assert!(unsafe { rpgm_get_save_summary(view(path.to_str().unwrap()), EngineType::MVMZ, FFIString::null(), &mut out) });

        let summary: serde_json::Value = serde_json::from_str(&take_string(out)).unwrap();
        assert_eq!(summary["frames"], 7200);
        assert_eq!(summary["mapId"], 3);
        assert_eq!(summary["gold"], 250);
        assert_eq!(summary["party"], serde_json::json!([1]));
        assert_eq!(summary["actors"][0]["name"], "Harold");
        assert_eq!(summary["actors"][0]["level"], 4);
        assert_eq!(summary["items"][0], serde_json::json!({"id": 2, "count": 5}));
        assert_eq!(summary["switches"], serde_json::json!([false, true, false]));
        assert_eq!(summary["variables"], serde_json::json!([0, 0, 42]));

        fs::write(&path, b"not a save").unwrap();
        let mut out = FFIString::null();
        assert!(!unsafe { rpgm_get_save_summary(view(path.to_str().unwrap()), EngineType::MVMZ, FFIString::null(), &mut out) });
        assert!(!unsafe { rpgm_get_save_summary(view(dir.0.join("missing").to_str().unwrap()), EngineType::VXAce, FFIString::null(), &mut out) });
        assert!(!last_error().is_empty());
    }

    #[test]
    fn credentials_that_were_never_stored_are_not_found() {
        let account = format!("rpgmtranslate-test-never-stored-{}", std::process::id());
        let mut out = FFIString { ptr: ptr::NonNull::dangling().as_ptr(), len: 1, cap: 0 };

        // Neither a missing entry nor a machine without a keyring is a secret
        assert!(!unsafe { rpgm_get_credential(view(&account), &mut out) });
        assert!(out.ptr.is_null());
    }

    #[test]
    fn credential_store_failures_are_reported_and_leave_the_secret_alone() {
        use keyring_core::{Error as KeyError, mock::Cred};

        static MOCK: std::sync::Once = std::sync::Once::new();
        MOCK.call_once(|| unsafe { std::env::set_var("RPGMTRANSLATE_MOCK_KEYRING", "1") });

        let _guard = error_guard();
        let account = format!("rpgmtranslate-test-failures-{}", std::process::id());
        let entry = credential_entry(&account).unwrap();
        let fail_next = |err: KeyError| entry.as_any().downcast_ref::<Cred>().unwrap().set_error(err);
        let refusal = || KeyError::Invalid("mock".to_owned(), "refused".to_owned());

        assert!(unsafe { rpgm_store_credential(view(&account), view("kept")) });

        // A store that refuses the write says so, and the old secret stays
        fail_next(refusal());
        assert!(!unsafe { rpgm_store_credential(view(&account), view("lost")) });
        assert!(last_error().contains("refused"), "{}", last_error());
        assert_eq!(entry.get_password().unwrap(), "kept");

        // A refused read gives no secret and a null string
        let mut out = FFIString::null();
        fail_next(refusal());
        assert!(!unsafe { rpgm_get_credential(view(&account), &mut out) });
        assert!(out.ptr.is_null());

        // A refused delete is an error, and the secret is still there
        fail_next(refusal());
        assert!(!unsafe { rpgm_delete_credential(view(&account)) });
        assert!(last_error().contains("refused"), "{}", last_error());
        assert_eq!(entry.get_password().unwrap(), "kept");

        // Nothing stored under the account is not a failure of a delete
        fail_next(KeyError::NoEntry);
        assert!(unsafe { rpgm_delete_credential(view(&account)) });
        assert!(unsafe { rpgm_delete_credential(view(&account)) });
    }

    #[test]
    fn credentials_are_stored_read_replaced_and_deleted() {
        // The mock keeps the secrets out of the credential store of the machine the tests run on
        static MOCK: std::sync::Once = std::sync::Once::new();
        MOCK.call_once(|| unsafe { std::env::set_var("RPGMTRANSLATE_MOCK_KEYRING", "1") });

        let account = format!("rpgmtranslate-test-round-trip-{}", std::process::id());
        let read = || {
            let mut out = FFIString::null();
            let found = unsafe { rpgm_get_credential(view(&account), &mut out) };
            found.then(|| take_string(out))
        };

        assert_eq!(read(), None);
        assert!(unsafe { rpgm_store_credential(view(&account), view("first")) });
        assert_eq!(read().as_deref(), Some("first"));

        // A second key replaces the first instead of adding to it
        assert!(unsafe { rpgm_store_credential(view(&account), view("sécret ключ")) });
        assert_eq!(read().as_deref(), Some("sécret ключ"));

        assert!(unsafe { rpgm_delete_credential(view(&account)) });
        assert_eq!(read(), None);
        // Deleting what is not there is not a failure
        assert!(unsafe { rpgm_delete_credential(view(&account)) });
    }

    #[test]
    fn hashing_bytes_is_stable_and_accepts_nothing() {
        let hash = |bytes: &[u8]| unsafe { gxhash(if bytes.is_empty() { ptr::null() } else { bytes.as_ptr().cast() }, bytes.len()) };

        assert_eq!(hash(b"same"), hash(b"same"));
        assert_ne!(hash(b"same"), hash(b"other"));
        // Empty input has a null pointer from C++
        assert_eq!(hash(b""), hash(b""));

        #[cfg(feature = "gxhash")]
        assert_eq!(hash(b"abc"), gxhash::gxhash64(b"abc", 0));
    }

    #[test]
    fn text_is_counted_through_the_ffi() {
        let counts = unsafe { rpgm_count_text(view("Hello world")) };
        assert_eq!(counts.words, 2);

        let none = unsafe { rpgm_count_text(FFIString::null()) };
        assert_eq!((none.words, none.characters), (0, 0));
    }

    #[test]
    fn dictionaries_that_are_not_installed_are_not_an_error() {
        let dir = TempDir::new("lindera");

        // Nothing to find under the root: nothing is set, nothing fails
        unsafe { init_lindera_dictionaries(view(dir.0.to_str().unwrap())) };
        unsafe { init_lindera_dictionaries(FFIString::null()) };
    }

    static LOGGED: Mutex<Vec<(u8, String)>> = Mutex::new(Vec::new());

    extern "C" fn record(level: u8, text: FFIString) {
        LOGGED
            .lock()
            .unwrap_or_else(PoisonError::into_inner)
            .push((level, unsafe { ffi_to_str(text) }.to_owned()));
    }

    extern "C" fn record_elsewhere(_: u8, _: FFIString) {
        panic!("a second callback must not replace the first");
    }

    #[test]
    fn log_records_reach_the_callback_with_the_cpp_levels() {
        init_rust_logger(record);
        // Only the first callback counts
        init_rust_logger(record_elsewhere);

        log::error!(target: "ffi-test", "e");
        log::warn!(target: "ffi-test", "w");
        log::info!(target: "ffi-test", "i");
        log::debug!(target: "ffi-test", "d");
        log::trace!(target: "ffi-test", "t");

        let logged = LOGGED.lock().unwrap_or_else(PoisonError::into_inner);
        let ours: Vec<_> = logged.iter().filter(|(_, text)| text.starts_with("[ffi-test] ")).cloned().collect();

        // The C++ levels count from 0 (Error), the `log` ones from 1
        let mut expected = vec![
            (0, "[ffi-test] e".to_owned()),
            (1, "[ffi-test] w".to_owned()),
            (2, "[ffi-test] i".to_owned()),
            (3, "[ffi-test] d".to_owned()),
        ];

        // Some dependencies cap the level at compile time, which removes the trace calls of this crate too
        if log::STATIC_MAX_LEVEL >= log::LevelFilter::Trace {
            expected.push((4, "[ffi-test] t".to_owned()));
        }

        assert_eq!(ours, expected);
    }

    #[cfg(feature = "json-highlighting")]
    #[test]
    fn code_is_highlighted_through_the_ffi() {
        let mut out = FFIString::null();

        assert!(unsafe { rpgm_highlight_code(view(r#"{"a": 1}"#), HighlightLanguage::JSON, &mut out) });
        // What the highlighting makes of it is checked in `api`; here only that the string arrives
        assert!(!take_string(out).is_empty());
    }

    #[cfg(feature = "js-formatting")]
    #[test]
    fn source_is_formatted_through_the_ffi() {
        let _guard = error_guard();
        let mut out = FFIString::null();

        assert!(unsafe { rpgm_format_src(view("var a=1"), HighlightLanguage::JS, &mut out) });
        // What the formatter makes of it is checked in `api`; here only that the string arrives
        assert!(!take_string(out).is_empty());

        assert!(!unsafe { rpgm_format_src(view("var = ;"), HighlightLanguage::JS, &mut out) });
        assert!(!last_error().is_empty());
    }
}
