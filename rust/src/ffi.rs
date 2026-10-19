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
