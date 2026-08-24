#![allow(unsafe_op_in_unsafe_fn)]
#![allow(static_mut_refs)]
use crate::api::*;
use gxhash::{HashMap, HashMapExt, gxhash64};
use language_tokenizer::{Algorithm, MatchResult};
#[cfg(feature = "languagetool")]
use languagetool_rust::api::check::{Data, DataAnnotation};
use log::{Log, Metadata, Record};
use marshal_rs::Get;
use rvpacker_txt_rs_lib::{BaseFlags, DuplicateMode, EngineType, FileFlags, RPGMFileType, core::parse_rpgm_file};
use std::{
    ffi::{CStr, c_char, c_void},
    fs::{self, read_to_string},
    io::{Cursor, Read},
    mem,
    ops::Deref,
    path::{Path, PathBuf},
    ptr, slice,
    sync::{LazyLock, OnceLock},
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

impl Bitset2048 {
    #[inline]
    pub fn is_set(&self, bit: usize) -> bool {
        let byte = unsafe { *self.data.get_unchecked(bit >> 3) };
        (byte >> (bit & 7)) & 1 == 1
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Selected {
    map_indices: Bitset2048,
    valid_indices: Bitset2048,
    map_count: u16,
    file_flags: FileFlags,
    padding: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
/// Byte buffer with FFI-compatible layout.
///
/// This struct shouldn't be used for UTF-8 string. Instead, [`FFIString`] should be used.
///
/// If `cap == 0`, then this is just a view.
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
#[derive(Clone, Copy)]
/// UTF-8 string with FFI-compatible layout.
///
/// Use [`str_to_ffi`] and [`ffi_to_str`] for conversions.
///
/// Use [`FFIString::null`] to default-initialize to null.
///
/// If `cap == 0`, then this is just a view.
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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_free_translated_files(translated_files: ByteBuffer, translated_files_ffi: ByteBuffer) {
    let rebuilt_files_ffi: Vec<ByteBuffer> = Vec::from_raw_parts(
        translated_files_ffi.ptr.cast_mut().cast::<ByteBuffer>(),
        translated_files_ffi.len as usize,
        translated_files_ffi.cap as usize,
    );

    for buffer in &rebuilt_files_ffi {
        if buffer.ptr.is_null() || buffer.len == 0 {
            continue;
        }

        let _: Vec<FFIString> = Vec::from_raw_parts(
            buffer.ptr.cast_mut().cast::<FFIString>(),
            buffer.len as usize,
            buffer.cap as usize,
        );
    }

    let _: Vec<Vec<String>> = Vec::from_raw_parts(
        translated_files.ptr.cast_mut().cast::<Vec<String>>(),
        translated_files.len as usize,
        translated_files.cap as usize,
    );
}

static mut ERROR: String = String::new();

/// Borrows the static error buffer - `cap` is deliberately 0 so that passing
/// the result to [`rpgm_string_free`] is a no-op instead of freeing a static.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_error() -> FFIString {
    FFIString {
        ptr: ERROR.as_ptr().cast::<c_char>(),
        len: ERROR.len() as u32,
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
    out_hashes: *mut ByteBuffer,
) -> bool {
    let result = (|| -> Result<_, Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);
        let ini_title = ffi_to_str(ini_title);
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

        let cap = hashes.cap;
        let mut hashes_map: HashMap<String, u64> = HashMap::with_capacity(cap as usize);

        let hashes = slice::from_raw_parts(hashes.ptr, hashes.len as usize);
        let mut cursor = Cursor::new(hashes);

        let mut buf = [0u8; 16];
        let mut hash_buf = [0u8; 8];

        while let Ok(r) = cursor.read(&mut buf)
            && r != 0
        {
            let _ = cursor.read_exact(&mut hash_buf);
            hashes_map.insert(
                CStr::from_bytes_until_nul(&buf)
                    .unwrap_unchecked()
                    .to_string_lossy()
                    .into_owned(),
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
        )?;

        Ok(out)
    })();

    match result {
        Ok(hashes) => {
            dbg!(&hashes);

            let mut buf = Vec::with_capacity(4 + (hashes.len() * 16) + (hashes.len() * size_of::<u64>()));

            buf.extend((hashes.len() as u32).to_le_bytes());

            for (key, value) in hashes {
                buf.extend(key.as_bytes());

                for _ in 0..16 - key.len() {
                    buf.push(0);
                }

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
            ERROR = err.to_string();
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
        )?;

        Ok(elapsed)
    })();

    match result {
        Ok(elapsed) => {
            *elapsed_out = elapsed;
            true
        }
        Err(err) => {
            ERROR = err.to_string();
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
        )?;

        Ok(())
    })();

    match result {
        Ok(_) => true,
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_extract_archive(input_path: FFIString, output_path: FFIString) -> bool {
    let result = (|| -> Result<(), Error> {
        let input_path = ffi_to_str(input_path);
        let output_path = ffi_to_str(output_path);
        extract_archive(Path::new(&input_path), Path::new(&output_path))?;
        Ok(())
    })();

    match result {
        Ok(()) => true,
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_get_models(
    endpoint: TranslationEndpoint,
    api_key: FFIString,
    base_url: FFIString,
    out: *mut ByteBuffer,
) -> bool {
    let result = (|| -> Result<_, Error> {
        let api_key = ffi_to_str(api_key);
        let base_url = ffi_to_str(base_url);
        let models = TOKIO_RT.block_on(async move { get_models(endpoint, &api_key, &base_url).await })?;
        Ok(models)
    })();

    match result {
        Ok(models) => {
            let mut buffer: Vec<u8> = Vec::new();
            buffer.extend_from_slice(&(models.len() as u32).to_le_bytes());

            for string in models {
                buffer.extend_from_slice(&(string.len() as u32).to_le_bytes());
                buffer.extend_from_slice(string.as_bytes());
            }

            *out = ByteBuffer {
                ptr: buffer.as_mut_ptr(),
                len: buffer.len() as u32,
                cap: buffer.capacity() as u32,
            };

            mem::forget(buffer);
            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_translate_single(
    endpoint_settings: FFIString,
    project_context: FFIString,
    local_context: FFIString,
    source_language: FFIString,
    translation_language: FFIString,
    text: FFIString,
    glossary: FFIString,
    out_string: *mut FFIString,
) -> bool {
    let project_context = ffi_to_str(project_context);
    let local_context = ffi_to_str(local_context);
    let text = ffi_to_str(text);
    let glossary = ffi_to_str(glossary);
    let endpoint_settings = ffi_to_str(endpoint_settings);

    let glossary: Vec<GlossaryEntry> = unsafe { serde_json::from_str(glossary).unwrap_unchecked() };

    let result = (|| -> Result<_, Error> {
        let results = TOKIO_RT.block_on(async move {
            translate_single(
                endpoint_settings,
                ffi_to_str(source_language),
                ffi_to_str(translation_language),
                &project_context,
                &local_context,
                &text,
                glossary,
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
            ERROR = err.to_string();
            false
        }
    }
}

unsafe fn take<'a>(buf: &'a [u8], pos: &mut usize, n: usize) -> &'a [u8] {
    let end = pos.checked_add(n).unwrap_unchecked();
    let out = &buf[*pos..end];
    *pos = end;
    out
}

unsafe fn read_u32_le(buf: &[u8], pos: &mut usize) -> u32 {
    let b = take(buf, pos, 4);
    u32::from_le_bytes(*b.as_ptr().cast::<[u8; 4]>())
}

pub unsafe fn parse_strings<'a>(buf: &'a [u8]) -> Vec<&'a str> {
    let mut pos = 0;

    let count = read_u32_le(buf, &mut pos) as usize;
    let mut out = Vec::with_capacity(count);

    for _ in 0..count {
        let n = read_u32_le(buf, &mut pos) as usize;
        let bytes = take(buf, &mut pos, n);
        let s = str::from_utf8_unchecked(bytes);
        out.push(s);
    }

    out
}

pub fn split_into_sections(input: &str) -> Vec<&str> {
    const MARKER: &str = "<#>ID";

    let mut starts: Vec<usize> = Vec::from([0]);

    let bytes = input.as_bytes();
    let mut i = 0;

    while i < bytes.len() {
        if bytes[i] == b'\n' {
            let line_start = i + 1;

            if line_start + MARKER.len() <= bytes.len() && input[line_start..].starts_with(MARKER) {
                starts.push(line_start);
            }
        }

        i += 1;
    }

    let mut out = Vec::with_capacity(starts.len());
    for w in 0..starts.len() {
        let s = starts[w];

        let e = if w + 1 < starts.len() {
            starts[w + 1]
        } else {
            input.len()
        };

        out.push(&input[s..e]);
    }

    out
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_translate<'a>(
    endpoint_settings: FFIString,
    project_context: FFIString,
    local_context: FFIString,
    translation_path: FFIString,
    source_language: FFIString,
    translation_language: FFIString,
    filenames: ByteBuffer,
    glossary: FFIString,
    out_translated: *mut ByteBuffer,
    out_translated_ffi: *mut ByteBuffer,
    is_interrupted: extern "C" fn(*mut c_void) -> bool,
    is_interrupted_ctx: *mut c_void,
) -> bool {
    let project_context = ffi_to_str(project_context);
    let local_context = ffi_to_str(local_context);
    let glossary = ffi_to_str(glossary);
    let glossary: Vec<GlossaryEntry> = unsafe { serde_json::from_str(glossary).unwrap_unchecked() };

    let translation_path = ffi_to_str(translation_path);
    let endpoint_settings = ffi_to_str(endpoint_settings);

    let map_content = String::new();
    let mut sections: Vec<&str> = Vec::new();

    let result = (|| -> Result<_, Error> {
        let filenames = slice::from_raw_parts::<[u8; 16]>(filenames.ptr.cast::<[u8; 16]>(), filenames.len as usize);

        let mut files: HashMap<&str, Vec<String>> = HashMap::with_capacity(filenames.len());
        let mut ordered_filenames: Vec<&str> = Vec::with_capacity(filenames.len());

        for filename in filenames {
            let filename = str::from_utf8_unchecked(filename);
            let filename = &filename[..=filename.rfind(|chr| chr != '\0').unwrap_unchecked()];
            ordered_filenames.push(filename);

            if filename.starts_with("map") {
                if map_content.is_empty() {
                    let path = Path::new(translation_path).join("maps.txt");

                    #[allow(invalid_reference_casting)]
                    unsafe {
                        *(&mut *(&map_content as *const String as *mut String)) =
                            read_to_string(&path).map_err(|err| Error::Io(path, err))?;
                    }

                    sections = split_into_sections(&map_content);
                }

                files.insert(filename, Vec::new());
                let entry = unsafe { files.get_mut(filename).unwrap_unchecked() };
                let id = &filename[3..];

                for &section in &sections {
                    let id_line = &section[..section.find('\n').unwrap_or(section.len())];
                    let id_part = &id_line[id_line.find("<#>").unwrap_or(id_line.len()) + 3..];

                    if id_part != id {
                        continue;
                    }

                    for (i, line) in section.split('\n').enumerate() {
                        if line.is_empty() {
                            continue;
                        }

                        if i == 0 {
                            entry.push(line.to_string());
                            continue;
                        }

                        let not_name = !line.starts_with("<!>NAME");
                        let not_in_game_name = !line.starts_with("<!>IN-GAME");
                        let not_map_name = !line.starts_with("<!>MAP NAME");

                        if line.starts_with("<!>") && not_name && not_in_game_name && not_map_name {
                            continue;
                        }

                        if not_name && not_in_game_name && not_map_name {
                            if let Some(separator_pos) = line.find("<#>") {
                                entry.push(line[..separator_pos].to_string());
                            } else {
                                log::error!("Failed to split line {i} in file {filename}");
                            }
                        } else {
                            entry.push(line.to_string());
                        }
                    }
                }
            } else {
                let path = Path::new(&translation_path).join(filename).with_extension("txt");
                let content = read_to_string(&path).map_err(|err| Error::Io(path, err))?;
                let lines = content.split('\n');

                files.insert(filename, Vec::new());

                for (idx, line) in lines.enumerate() {
                    if line.is_empty()
                        || line.starts_with("<!>") && !line.starts_with("<!>ID") && !line.starts_with("<!>NAME")
                    {
                        continue;
                    }

                    let entry = files.get_mut(filename).unwrap_unchecked();

                    if !line.starts_with("<!>ID") && !line.starts_with("<!>NAME") {
                        if let Some(separator_pos) = line.rfind("<#>") {
                            entry.push(line[0..separator_pos].to_string());
                        } else {
                            log::error!("Failed to split line {idx} in file {filename}");
                        }
                    } else {
                        entry.push(line.to_string());
                    }
                }
            }
        }

        let mut results = TOKIO_RT.block_on(async move {
            translate(
                endpoint_settings,
                ffi_to_str(source_language),
                ffi_to_str(translation_language),
                &project_context,
                &local_context,
                files,
                glossary,
                is_interrupted,
                is_interrupted_ctx,
            )
            .await
        })?;

        let mut translations: Vec<Vec<String>> = Vec::with_capacity(ordered_filenames.len());

        for filename in &ordered_filenames {
            translations.push(results.remove(*filename).unwrap_or_default());
        }

        Ok(translations)
    })();

    match result {
        Ok(translated_files) => {
            let mut translated_files_ffi = Vec::with_capacity(translated_files.len());

            for translated_strings in translated_files.iter() {
                let mut strings_ffi: Vec<FFIString> = Vec::with_capacity(translated_strings.len());

                if translated_strings.is_empty() {
                    translated_files_ffi.push(ByteBuffer {
                        ptr: ptr::null(),
                        len: 0,
                        cap: 0,
                    });
                    continue;
                }

                for string in translated_strings {
                    strings_ffi.push(str_to_ffi(string));
                }

                translated_files_ffi.push(ByteBuffer {
                    ptr: strings_ffi.as_ptr().cast::<u8>(),
                    len: strings_ffi.len() as u32,
                    cap: strings_ffi.capacity() as u32,
                });

                mem::forget(strings_ffi);
            }

            *out_translated_ffi = ByteBuffer {
                ptr: translated_files_ffi.as_ptr().cast::<u8>(),
                len: translated_files_ffi.len() as u32,
                cap: translated_files_ffi.capacity() as u32,
            };

            *out_translated = ByteBuffer {
                ptr: translated_files.as_ptr().cast::<u8>(),
                len: translated_files.len() as u32,
                cap: translated_files.capacity() as u32,
            };

            mem::forget(translated_files);
            mem::forget(translated_files_ffi);

            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
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

                vec.extend((src.len() as u32).to_le_bytes());

                for m in src {
                    match m {
                        MatchResult::Exact { offset, len } => {
                            vec.extend((offset as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend(0f32.to_le_bytes());
                        }
                        MatchResult::Fuzzy { offset, len, score } => {
                            vec.extend((offset as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend((score as f32).to_le_bytes());
                        }
                    }
                }

                vec.extend((tr.len() as u32).to_le_bytes());

                for m in tr {
                    match m {
                        MatchResult::Exact { offset, len } => {
                            vec.extend((offset as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend(0f32.to_le_bytes());
                        }
                        MatchResult::Fuzzy { offset, len, score } => {
                            vec.extend((offset as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend((score as f32).to_le_bytes());
                        }
                    }
                }

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
            ERROR = err.to_string();
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

        cb(record.level() as u8, str_to_ffi(&text));
        mem::forget(text);
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

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_count_words(text: FFIString, algorithm: FFIString, out: *mut u32) -> bool {
    let result = (|| -> Result<_, Error> {
        let count = count_words(&ffi_to_str(text), from_bcp47(ffi_to_str(algorithm)))?;
        Ok(count)
    })();

    match result {
        Ok(count) => {
            *out = count;
            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
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
    let segments = unsafe { parse_strings(slice::from_raw_parts(segments.ptr, segments.len as usize)) };

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
            if mother_tongue.len == 0 {
                None
            } else {
                Some(ffi_to_str(mother_tongue).to_string())
            },
            ffi_opt_list(preferred_variants),
            ffi_opt_string(username),
        )
        .await?;
        Ok(decrypted)
    });

    match result {
        Ok(matches) => {
            let json = unsafe { serde_json::to_string(&matches).unwrap_unchecked() };

            *out_json = FFIString {
                ptr: json.as_ptr().cast::<i8>(),
                len: json.len() as u32,
                cap: json.capacity() as u32,
            };

            mem::forget(json);
            true
        }
        Err(err) => {
            ERROR = err.to_string();
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
            ERROR = err.to_string();
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
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
#[must_use]
pub unsafe extern "C" fn rpgm_generate_json(content: FFIString, filename: FFIString, json_out: *mut FFIString) -> bool {
    let result = (|| -> Result<_, Error> {
        let json = generate_json(ffi_to_str(content).as_bytes(), ffi_to_str(filename))?;
        Ok(json)
    })();

    match result {
        Ok(json) => {
            *json_out = str_to_ffi(&json);
            mem::forget(json);

            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_serde_export(content: FFIString, format: SerdeFormat, out: *mut ByteBuffer) -> bool {
    match serde_export(ffi_to_str(content), format) {
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
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_serde_import(bytes: ByteBuffer, format: SerdeFormat, out: *mut FFIString) -> bool {
    let slice = slice::from_raw_parts(bytes.ptr, bytes.len as usize);

    match serde_import(slice, format) {
        Ok(text) => {
            *out = str_to_ffi(&text);
            mem::forget(text);

            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_store_credential(account: FFIString, secret: FFIString) -> bool {
    match store_credential(ffi_to_str(account), ffi_to_str(secret)) {
        Ok(()) => true,
        Err(err) => {
            ERROR = err.to_string();
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
pub unsafe extern "C" fn rpgm_delete_credential(account: FFIString) {
    let _ = delete_credential(ffi_to_str(account));
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_ini_title(project_path: FFIString, out: *mut ByteBuffer) -> bool {
    let project_path = ffi_to_str(project_path);

    let result = (|| -> Result<_, Error> {
        let title = get_ini_title(project_path)?;
        Ok(title)
    })();

    match result {
        Ok(title) => {
            *out = ByteBuffer {
                ptr: title.as_ptr(),
                len: title.len() as u32,
                cap: title.capacity() as u32,
            };

            mem::forget(title);

            true
        }
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_hash_file(content: ByteBuffer, duplicate_mode: DuplicateMode, out: *mut u64) {
    let content = slice::from_raw_parts(content.ptr, content.len as usize);
    *out = gxhash64(content, duplicate_mode as i64);
}

enum DataPath<'a> {
    Keys(std::str::Split<'a, char>),
    Key(&'a str),
    Index(usize),
}

enum DataPathItem<'a> {
    Key(&'a str),
    Index(usize),
}

enum DataPathIter<'a> {
    Keys(std::str::Split<'a, char>),
    Key(std::option::IntoIter<&'a str>),
    Index(std::option::IntoIter<usize>),
}

impl<'a> Iterator for DataPathIter<'a> {
    type Item = DataPathItem<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        match self {
            DataPathIter::Keys(iter) => iter.next().map(DataPathItem::Key),
            DataPathIter::Key(iter) => iter.next().map(DataPathItem::Key),
            DataPathIter::Index(iter) => iter.next().map(DataPathItem::Index),
        }
    }
}

impl<'a> DataPath<'a> {
    fn iter(&'a self) -> DataPathIter<'a> {
        match self {
            DataPath::Keys(v) => DataPathIter::Keys(v.clone()),
            DataPath::Key(s) => DataPathIter::Key(Some(*s).into_iter()),
            DataPath::Index(n) => DataPathIter::Index(Some(*n).into_iter()),
        }
    }
}

fn parse_next_path(paths: &str) -> Option<DataPath<'_>> {
    let bytes = paths.as_bytes();

    let typ = bytes[0];
    let content_start = 1;

    let content = &paths[content_start..];

    let path = match typ {
        1 => {
            if content.contains('\x02') {
                DataPath::Keys(content.split('\x02'))
            } else {
                DataPath::Key(content)
            }
        }
        3 => DataPath::Index(content.parse().unwrap_or(usize::MAX)),
        _ => unreachable!(),
    };

    Some(path)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_entity_data(
    path: FFIString,
    engine_type: EngineType,
    file_type: RPGMFileType,
    paths: FFIString,
    out: *mut FFIString,
) -> bool {
    let path = ffi_to_str(path);
    let paths = ffi_to_str(paths);

    let result = (|| -> Result<_, Error> {
        let content = fs::read(path).map_err(|err| Error::Io(Path::new(path).to_path_buf(), err))?;

        let value = parse_rpgm_file(&content, engine_type, file_type)?;

        let mut entries = paths.split('\x04').peekable();
        let mut cur = &value;

        while let Some(entry) = entries.next() {
            if entries.peek().is_none() {
                let mut out_idx = 0usize;

                if let Some(segment) = parse_next_path(entry) {
                    for segment in segment.iter() {
                        let resolved = match segment {
                            DataPathItem::Key(key) => cur
                                .get(key)
                                .ok_or_else(|| Error::InvalidKey(key.to_string(), path.to_string()))?,
                            DataPathItem::Index(idx) => cur
                                .get_index(idx)
                                .ok_or_else(|| Error::InvalidKey(idx.to_string(), path.to_string()))?,
                        };

                        let string;

                        match resolved.deref() {
                            marshal_rs::ValueType::Bool(bool) => {
                                if *bool {
                                    string = String::from("true")
                                } else {
                                    string = String::from("false")
                                }
                            }
                            marshal_rs::ValueType::Integer(int) => string = int.to_string(),
                            marshal_rs::ValueType::String(str) => string = str.clone(),
                            _ => {
                                unreachable!();
                            }
                        }

                        *out.add(out_idx) = FFIString {
                            ptr: string.as_ptr().cast::<i8>(),
                            len: string.len() as u32,
                            cap: string.capacity() as u32,
                        };

                        mem::forget(string);
                        out_idx += 1;
                    }
                }

                break;
            }

            let Some(segment) = parse_next_path(entry) else {
                return Err(Error::InvalidKey(String::from("0"), path.to_string()));
            };

            let next = match segment {
                DataPath::Keys(_) => unreachable!(),
                DataPath::Key(key) => cur
                    .get(key)
                    .ok_or_else(|| Error::InvalidKey(key.to_string(), path.to_string()))?,
                DataPath::Index(idx) => cur
                    .get_index(idx)
                    .ok_or_else(|| Error::InvalidKey(idx.to_string(), path.to_string()))?,
            };

            cur = next;
        }

        Ok(())
    })();

    match result {
        Ok(()) => true,
        Err(err) => {
            ERROR = err.to_string();
            false
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn gxhash(ptr: *const std::ffi::c_void, len: usize) -> u64 {
    gxhash::gxhash64(slice::from_raw_parts(ptr.cast::<u8>(), len), 0)
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
            ERROR = err.to_string();
            false
        }
    }
}
