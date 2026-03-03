#![allow(unsafe_op_in_unsafe_fn)]
use crate::api::{self, *};
use language_tokenizer::{Algorithm, MatchMode, MatchResult};
use log::{Log, Metadata, Record};
use rvpacker_txt_rs_lib::{BaseFlags, DuplicateMode, EngineType, FileFlags};
use std::{
    collections::HashMap,
    ffi::c_char,
    fs::read_to_string,
    path::Path,
    sync::{LazyLock, OnceLock},
};
use tokio::runtime::{Builder, Runtime};

#[repr(u8)]
#[derive(Clone, Copy, Debug)]
pub enum ReadMode {
    Default,
    DefaultForce,
    AppendDefault,
    AppendForce,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
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
pub struct ByteBuffer {
    pub ptr: *const u8,
    pub len: usize,
}

impl ByteBuffer {
    pub const fn null() -> Self {
        Self {
            ptr: std::ptr::null(),
            len: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MatchModeInfo {
    fuzzy_threshold: f32,
    mode: u8,
    case_sensitive: bool,
    permissive: bool,
}

static TOKIO_RT: LazyLock<Runtime> =
    LazyLock::new(|| Builder::new_multi_thread().enable_all().build().unwrap());

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FFIString {
    pub ptr: *const c_char,
    pub len: usize,
}

impl FFIString {
    pub const fn null() -> Self {
        Self {
            ptr: std::ptr::null(),
            len: 0,
        }
    }
}

#[inline]
unsafe fn ffi_to_str<'a>(string: FFIString) -> &'a str {
    let slice = std::slice::from_raw_parts(string.ptr.cast::<u8>(), string.len);
    str::from_utf8_unchecked(slice)
}

#[inline]
fn str_to_ffi(str: &str) -> FFIString {
    let vec = str.to_owned().into_bytes();
    let len = vec.len();
    let ptr = vec.as_ptr().cast::<c_char>();
    std::mem::forget(vec);
    FFIString { ptr, len }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_string_free(ffi_string: FFIString) {
    let _ = Vec::from_raw_parts(
        ffi_string.ptr.cast_mut(),
        ffi_string.len,
        ffi_string.len,
    );
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_buffer_free(buffer: ByteBuffer) {
    let _ = Vec::from_raw_parts(buffer.ptr.cast_mut(), buffer.len, buffer.len);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_free_translated_files(
    translated_files: ByteBuffer,
    translated_files_ffi: ByteBuffer,
) {
    let rebuilt_files_ffi: Vec<ByteBuffer> = Vec::from_raw_parts(
        translated_files_ffi.ptr.cast_mut().cast::<ByteBuffer>(),
        translated_files_ffi.len,
        translated_files_ffi.len,
    );

    for buffer in &rebuilt_files_ffi {
        if buffer.ptr.is_null() || buffer.len == 0 {
            continue;
        }

        let _: Vec<FFIString> = Vec::from_raw_parts(
            buffer.ptr.cast_mut().cast::<FFIString>(),
            buffer.len,
            buffer.len,
        );
    }

    let _: Vec<Vec<String>> = Vec::from_raw_parts(
        translated_files.ptr.cast_mut().cast::<Vec<String>>(),
        translated_files.len,
        translated_files.len,
    );
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_read(
    project_path: FFIString,
    source_path: FFIString,
    translation_path: FFIString,
    read_mode: ReadMode,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    selected: Selected,
    flags: BaseFlags,
    map_events: bool,
    hashes: ByteBuffer,
    out_hashes: *mut ByteBuffer,
) -> FFIString {
    let result = (|| -> Result<_, Error> {
        let project_path = ffi_to_str(project_path);
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);

        let hashes = if hashes.ptr.is_null() {
            &[]
        } else {
            std::slice::from_raw_parts(hashes.ptr.cast::<u128>(), hashes.len)
        };

        let read_mode = match read_mode {
            ReadMode::Default => rvpacker_txt_rs_lib::ReadMode::Default(false),
            ReadMode::DefaultForce => {
                rvpacker_txt_rs_lib::ReadMode::Default(true)
            }
            ReadMode::AppendDefault => {
                rvpacker_txt_rs_lib::ReadMode::Append(false)
            }
            ReadMode::AppendForce => {
                rvpacker_txt_rs_lib::ReadMode::Append(true)
            }
        };

        let out = read(
            Path::new(&project_path),
            Path::new(&source_path),
            Path::new(&translation_path),
            read_mode,
            engine_type,
            duplicate_mode,
            selected.file_flags,
            flags,
            map_events,
            hashes.to_vec(),
        )?;

        Ok(out)
    })();

    match result {
        Ok(mut serialized) => {
            *out_hashes = ByteBuffer {
                ptr: serialized.as_mut_ptr().cast::<u8>(),
                len: serialized.len() * size_of::<u128>(),
            };

            std::mem::forget(serialized);

            FFIString::null()
        }
        Err(err) => {
            let msg = err.to_string();
            str_to_ffi(&msg)
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_write(
    source_path: FFIString,
    translation_path: FFIString,
    output_path: FFIString,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    game_title: FFIString,
    flags: BaseFlags,
    selected: Selected,
    elapsed_out: *mut f32,
) -> FFIString {
    let result = (|| -> Result<f32, Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);
        let output_path = ffi_to_str(output_path);
        let game_title = ffi_to_str(game_title);

        let elapsed = write(
            Path::new(&source_path),
            Path::new(&translation_path),
            Path::new(&output_path),
            engine_type,
            duplicate_mode,
            &game_title,
            flags,
            selected.file_flags,
        )?;

        Ok(elapsed)
    })();

    match result {
        Ok(elapsed) => {
            *elapsed_out = elapsed;
            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_purge(
    source_path: FFIString,
    translation_path: FFIString,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    game_title: FFIString,
    flags: BaseFlags,
    selected: Selected,
) -> FFIString {
    let result = (|| -> Result<(), Error> {
        let source_path = ffi_to_str(source_path);
        let translation_path = ffi_to_str(translation_path);
        let game_title = ffi_to_str(game_title);

        purge(
            Path::new(&source_path),
            Path::new(&translation_path),
            engine_type,
            duplicate_mode,
            &game_title,
            flags,
            selected.file_flags,
        )?;

        Ok(())
    })();

    match result {
        Ok(_) => FFIString::null(),
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_extract_archive(
    input_path: FFIString,
    output_path: FFIString,
) -> FFIString {
    let result = (|| -> Result<(), Error> {
        let input_path = ffi_to_str(input_path);
        let output_path = ffi_to_str(output_path);
        extract_archive(Path::new(&input_path), Path::new(&output_path))?;
        Ok(())
    })();

    match result {
        Ok(()) => FFIString::null(),
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_get_models(
    endpoint: TranslationEndpoint,
    api_key: FFIString,
    base_url: FFIString,
    out: *mut ByteBuffer,
) -> FFIString {
    let result = (|| -> Result<_, Error> {
        let api_key = ffi_to_str(api_key);
        let base_url = ffi_to_str(base_url);
        let models = TOKIO_RT.block_on(async move {
            get_models(endpoint, &api_key, &base_url).await
        })?;
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
                len: buffer.len(),
            };

            std::mem::forget(buffer);
            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_translate_single(
    endpoint: TranslationEndpoint,
    endpoint_settings: FFIString,
    project_context: FFIString,
    local_context: FFIString,
    source_language: Algorithm,
    translation_language: Algorithm,
    text: FFIString,
    glossary: ByteBuffer,
    out_string: *mut FFIString,
) -> FFIString {
    let project_context = ffi_to_str(project_context);
    let local_context = ffi_to_str(local_context);
    let text = ffi_to_str(text);
    let glossary = str::from_utf8_unchecked(std::slice::from_raw_parts(
        glossary.ptr,
        glossary.len,
    ));
    let endpoint_settings = ffi_to_str(endpoint_settings);

    let glossary: Vec<GlossaryEntry> =
        unsafe { serde_json::from_str(glossary).unwrap_unchecked() };

    let result = (|| -> Result<_, Error> {
        let results = TOKIO_RT.block_on(async move {
            translate_single(
                endpoint,
                endpoint_settings,
                source_language,
                translation_language,
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
            *out_string = str;
            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
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
    const MARKER: &str = "<!-- ID -->";

    let mut starts: Vec<usize> = Vec::from([0]);

    let bytes = input.as_bytes();
    let mut i = 0;

    while i < bytes.len() {
        if bytes[i] == b'\n' {
            let line_start = i + 1;

            if line_start + MARKER.len() <= bytes.len()
                && input[line_start..].starts_with(MARKER)
            {
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
pub unsafe extern "C" fn rpgm_translate<'a>(
    endpoint: TranslationEndpoint,
    endpoint_settings: FFIString,
    project_context: FFIString,
    local_context: FFIString,
    translation_path: FFIString,
    source_language: Algorithm,
    translation_language: Algorithm,
    filenames: ByteBuffer,
    glossary: FFIString,
    out_translated: *mut ByteBuffer,
    out_translated_ffi: *mut ByteBuffer,
) -> FFIString {
    let project_context = ffi_to_str(project_context);
    let local_context = ffi_to_str(local_context);
    let glossary = ffi_to_str(glossary);
    let glossary: Vec<GlossaryEntry> =
        unsafe { serde_json::from_str(glossary).unwrap_unchecked() };

    let translation_path = ffi_to_str(translation_path);
    let endpoint_settings = ffi_to_str(endpoint_settings);

    let map_content = String::new();
    let mut sections: Vec<&str> = Vec::new();

    let result = (|| -> Result<_, Error> {
        let filenames = std::slice::from_raw_parts::<[u8; 13]>(
            filenames.ptr.cast::<[u8; 13]>(),
            filenames.len,
        );

        let mut files: HashMap<&str, Vec<String>> =
            HashMap::with_capacity(filenames.len());

        for filename in filenames {
            let filename = str::from_utf8_unchecked(filename);
            let filename = &filename[..filename.find('\0').unwrap_unchecked()];

            if filename.starts_with("map") {
                if map_content.is_empty() {
                    let path = Path::new(translation_path).join("maps.txt");
                    #[allow(invalid_reference_casting)]
                    unsafe {
                        *(&mut *(&map_content as *const String
                            as *mut String)) = read_to_string(&path)
                            .map_err(|err| Error::Io(path, err))?;
                    }
                    sections = split_into_sections(&map_content);
                }

                files.insert(filename, Vec::new());
                let entry =
                    unsafe { files.get_mut(filename).unwrap_unchecked() };
                let id = &filename[3..];

                for &section in &sections {
                    if !section[..section.find('\n').unwrap_or(section.len())]
                        .ends_with(id)
                    {
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

                        let not_name = !line.starts_with("<!-- NAME");
                        let not_in_game_name =
                            !line.starts_with("<!-- IN-GAME");
                        let not_map_name = !line.starts_with("<!-- MAP NAME");

                        if line.starts_with("<!--")
                            && not_name
                            && not_in_game_name
                            && not_map_name
                        {
                            continue;
                        }

                        if not_name && not_in_game_name && not_map_name {
                            entry.push(
                                line[..line.find("<#>").unwrap()].to_string(),
                            );
                        } else {
                            entry.push(line.to_string());
                        }
                    }
                }
            } else {
                let path = Path::new(&translation_path)
                    .join(filename)
                    .with_extension("txt");
                let content = read_to_string(&path)
                    .map_err(|err| Error::Io(path, err))?;
                let lines = content.split('\n');

                files.insert(filename, Vec::new());

                for line in lines {
                    if line.starts_with("<!--")
                        && !line.starts_with("<!-- ID")
                        && !line.starts_with("<!-- NAME")
                    {
                        continue;
                    }

                    let entry = files.get_mut(filename).unwrap_unchecked();

                    if !line.starts_with("<!-- ID")
                        && !line.starts_with("<!-- NAME")
                    {
                        entry.push(
                            line[0..line.rfind("<#>").unwrap()].to_string(),
                        );
                    } else {
                        entry.push(line.to_string());
                    }
                }
            }
        }

        let results = TOKIO_RT.block_on(async move {
            translate(
                endpoint,
                endpoint_settings,
                source_language,
                translation_language,
                &project_context,
                &local_context,
                files,
                glossary,
            )
            .await
        })?;

        let mut translations: Vec<Vec<String>> =
            Vec::with_capacity(results.len());

        for translated in results.into_values() {
            translations.push(translated);
        }

        Ok(translations)
    })();

    match result {
        Ok(translated_files) => {
            let mut translated_files_ffi =
                Vec::with_capacity(translated_files.len());

            for translated_strings in translated_files.iter() {
                let mut strings_ffi: Vec<FFIString> =
                    Vec::with_capacity(translated_strings.len());

                if translated_strings.is_empty() {
                    translated_files_ffi.push(ByteBuffer {
                        ptr: std::ptr::null(),
                        len: 0,
                    });
                }

                for string in translated_strings {
                    strings_ffi.push(FFIString {
                        ptr: string.as_ptr().cast::<c_char>(),
                        len: string.len(),
                    });
                }

                translated_files_ffi.push(ByteBuffer {
                    ptr: strings_ffi.as_ptr().cast::<u8>(),
                    len: strings_ffi.len(),
                });

                std::mem::forget(strings_ffi);
            }

            *out_translated_ffi = ByteBuffer {
                ptr: translated_files_ffi.as_ptr().cast::<u8>(),
                len: translated_files_ffi.len(),
            };

            *out_translated = ByteBuffer {
                ptr: translated_files.as_ptr().cast::<u8>(),
                len: translated_files.len(),
            };

            std::mem::forget(translated_files);
            std::mem::forget(translated_files_ffi);

            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_find_all_matches(
    source_haystack: FFIString,
    source_needle: FFIString,
    source_mode: MatchModeInfo,
    tr_haystack: FFIString,
    tr_needle: FFIString,
    tr_mode: MatchModeInfo,
    source_algorithm: Algorithm,
    tr_algorithm: Algorithm,
    out: *mut ByteBuffer,
) -> FFIString {
    let result = (|| -> Result<_, Error> {
        let source_haystack = ffi_to_str(source_haystack);
        let source_needle = ffi_to_str(source_needle);
        let tr_haystack = ffi_to_str(tr_haystack);
        let tr_needle = ffi_to_str(tr_needle);

        let source_mode = api::MatchModeInfo {
            mode: match source_mode.mode {
                0 => MatchMode::Exact,
                1 => MatchMode::Fuzzy(source_mode.fuzzy_threshold as f64),
                2 => MatchMode::Both(source_mode.fuzzy_threshold as f64),
                _ => unreachable!(),
            },
            case_sensitive: source_mode.case_sensitive,
            permissive: source_mode.permissive,
        };

        let tr_mode = api::MatchModeInfo {
            mode: match tr_mode.mode {
                0 => MatchMode::Exact,
                1 => MatchMode::Fuzzy(tr_mode.fuzzy_threshold as f64),
                2 => MatchMode::Both(tr_mode.fuzzy_threshold as f64),
                _ => unreachable!(),
            },
            case_sensitive: tr_mode.case_sensitive,
            permissive: tr_mode.permissive,
        };

        let m = find_all_matches(
            &source_haystack,
            &source_needle,
            source_mode,
            &tr_haystack,
            &tr_needle,
            tr_mode,
            source_algorithm,
            tr_algorithm,
        )?;

        let bytes = match m {
            None => Vec::new(),
            Some((src, tr)) => {
                let mut vec = Vec::with_capacity(
                    4 + (src.len() * (size_of::<MatchResult>() / 2))
                        + (tr.len() * (size_of::<MatchResult>() / 2)),
                );

                vec.extend((src.len() as u32).to_le_bytes());

                for m in src {
                    match m {
                        MatchResult::Exact((start, len)) => {
                            vec.extend((start as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend(0f32.to_le_bytes());
                        }
                        MatchResult::Fuzzy((start, len), score) => {
                            vec.extend((start as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend((score as f32).to_le_bytes());
                        }
                    }
                }

                vec.extend((tr.len() as u32).to_le_bytes());

                for m in tr {
                    match m {
                        MatchResult::Exact((start, len)) => {
                            vec.extend((start as u32).to_le_bytes());
                            vec.extend((len as u32).to_le_bytes());
                            vec.extend(0f32.to_le_bytes());
                        }
                        MatchResult::Fuzzy((start, len), score) => {
                            vec.extend((start as u32).to_le_bytes());
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
                len: serialized.len(),
            };

            std::mem::forget(serialized);

            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_add_detect_lang(string: FFIString) {
    let s = ffi_to_str(string);
    detect_lang(&s);
}

#[unsafe(no_mangle)]
pub extern "C" fn rpgm_get_detected_lang(out_string: *mut FFIString) {
    let lang = get_lang().unwrap_or("");

    unsafe {
        *out_string = str_to_ffi(lang);
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

        cb(
            record.level() as u8,
            FFIString {
                ptr: text.as_ptr().cast::<std::ffi::c_char>(),
                len: text.len(),
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

    log::set_logger(Box::leak(Box::new(FFILogger)))
        .expect("logger already set");
    log::set_max_level(log::LevelFilter::Trace);

    log::info!("Rust FFI initialized");
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_count_words(
    text: FFIString,
    algorithm: Algorithm,
    out: *mut u32,
) -> FFIString {
    let result = (|| -> Result<_, Error> {
        let count = count_words(&ffi_to_str(text), algorithm)?;
        Ok(count)
    })();

    match result {
        Ok(count) => {
            *out = count;
            FFIString::null()
        }
        Err(error) => str_to_ffi(&error.to_string()),
    }
}

#[cfg(feature = "languagetool")]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rpgm_language_tool_lint(
    text: FFIString,
    base_url: FFIString,
    api_key: FFIString,
    algorithm: Algorithm,
) -> FFIString {
    // TODO

    FFIString::null()
}
