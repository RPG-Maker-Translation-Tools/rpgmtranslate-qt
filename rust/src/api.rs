#![allow(clippy::too_many_arguments)]
use language_tokenizer::{Algorithm, MatchMode, MatchResult, find_all_matches as find_all_matches_, tokenize};
#[cfg(feature = "languagetool")]
use languagetool_rust::api::{
    check::{Data, Level, Match, Request as LTRequest},
    server::ServerClient,
};
use log::{debug, info, warn};
use rpgmad_lib::{ExtractError, decrypt};
use rvpacker_txt_rs_lib::{BaseFlags, DuplicateMode, EngineType, FileFlags, HashMap, Mode, Processor};
use serde::Deserialize;
#[cfg(feature = "languagetool")]
use std::time::Duration;
use std::{
    fs::{self, create_dir_all},
    io::{self},
    path::{Component, Path, PathBuf},
    str::FromStr,
    time::Instant,
};
use thiserror::Error;

const KEYRING_SERVICE: &str = "rpgmtranslate";

// Tests set this to keep API keys out of the real OS credential store: entries then live in keyring-core's in-memory
// store, shared by every entry with the same service and account, until the process ends.
const MOCK_KEYRING_ENV: &str = "RPGMTRANSLATE_MOCK_KEYRING";

pub(crate) fn credential_entry(account: &str) -> Result<keyring_core::Entry, keyring::Error> {
    if std::env::var_os(MOCK_KEYRING_ENV).is_none() {
        // Sets the platform store on the first call
        return Ok(keyring::Entry::new(KEYRING_SERVICE, account)?.inner);
    }

    static MOCK_STORE: std::sync::Once = std::sync::Once::new();
    let mut store_error = None;

    MOCK_STORE.call_once(|| match keyring_core::mock::Store::new() {
        Ok(store) => keyring_core::set_default_store(store),
        Err(err) => store_error = Some(err),
    });

    match store_error {
        Some(err) => Err(err),
        None => keyring_core::Entry::new(KEYRING_SERVICE, account),
    }
}

pub(crate) fn store_credential(account: &str, secret: &str) -> Result<(), Error> {
    Ok(credential_entry(account)?.set_password(secret)?)
}

pub(crate) fn get_credential(account: &str) -> Result<String, Error> {
    Ok(credential_entry(account)?.get_password()?)
}

pub(crate) fn delete_credential(account: &str) -> Result<(), Error> {
    Ok(credential_entry(account)?.delete_credential()?)
}

#[derive(Clone, Copy)]
#[repr(C)]
pub struct MatchModeInfo {
    pub mode: MatchMode,
    pub case_sensitive: bool,
    pub permissive: bool,
}

#[allow(dead_code)]
#[derive(Debug, Error)]
pub(crate) enum Error {
    #[error("{0}: IO error occurred: {1}")]
    Io(PathBuf, io::Error),
    #[error(transparent)]
    Rvpacker(#[from] rvpacker_txt_rs_lib::Error),
    #[error(transparent)]
    Extract(#[from] ExtractError),
    #[error(
        "{0} support was not compiled into this build. Rebuild with the corresponding CMake option enabled, or choose \
         a different option."
    )]
    BackendNotCompiled(&'static str),
    #[error("The file is not a valid MV/MZ save file.")]
    InvalidSave,
    #[error("Script endpoints are run by the application and cannot be translated by the core.")]
    ScriptEndpoint,
    #[cfg(feature = "google-translate")]
    #[error(transparent)]
    Translators(#[from] translators::Error),
    #[cfg(feature = "yandex-translate")]
    #[error(transparent)]
    Yandex(#[from] yandex_translate_v2::Error),
    #[cfg(feature = "yandex-translate")]
    #[error("The translation service returned no translation.")]
    NoTranslation,
    #[error("The entry has {0} values, but the caller reserved room for {1}.")]
    TooManyValues(usize, usize),
    #[error(transparent)]
    JSON(#[from] serde_json::Error),
    #[cfg(feature = "deepl")]
    #[error(transparent)]
    DeepL(#[from] deepl::Error),
    #[cfg(feature = "deepl")]
    #[error(transparent)]
    DeepLLangConvert(#[from] deepl::LangConvertError),
    #[error(transparent)]
    LanguageTokenizer(#[from] language_tokenizer::Error),
    #[cfg(feature = "languagetool")]
    #[error(transparent)]
    LanguageTool(#[from] languagetool_rust::error::Error),
    #[cfg(feature = "languagetool")]
    #[error(transparent)]
    Reqwest(#[from] reqwest::Error),
    #[error(transparent)]
    AssetDecrypt(#[from] rpgm_asset_decrypter_lib::Error),
    #[error("Key {0} does not exist in file {1}")]
    InvalidKey(String, String),
    #[error(transparent)]
    Keyring(#[from] keyring::Error),
    #[error("{0}")]
    Serde(String),
    #[cfg(any(
        feature = "json-highlighting",
        feature = "js-highlighting",
        feature = "ruby-highlighting"
    ))]
    #[error(transparent)]
    LumisBuilder(#[from] lumis::formatters::html_inline::HtmlInlineBuilderError),
    #[cfg(feature = "ruby-formatting")]
    #[error("Ruby formatting error occured: {0}")]
    Rubyfmt(i32),
    #[cfg(feature = "js-formatting")]
    #[error(transparent)]
    Dprint(#[from] anyhow::Error),
}

#[repr(u8)]
#[derive(Clone, Copy, Debug, Deserialize, Eq, PartialEq)]
#[serde(from = "u8")]
pub enum TranslationEndpoint {
    Google,
    Yandex,
    DeepL,
    Script,
}

impl Default for TranslationEndpoint {
    fn default() -> Self {
        TranslationEndpoint::Google
    }
}

impl From<u8> for TranslationEndpoint {
    fn from(v: u8) -> TranslationEndpoint {
        // A value that no endpoint has (settings written by another version) is not a valid enum to transmute
        match v {
            1 => TranslationEndpoint::Yandex,
            2 => TranslationEndpoint::DeepL,
            3 => TranslationEndpoint::Script,
            _ => TranslationEndpoint::Google,
        }
    }
}

pub(crate) fn write(
    source_path: &Path,
    translation_path: &Path,
    output_path: &Path,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    flags: BaseFlags,
    skip_files: FileFlags,
    read_encoding: Option<&'static encoding_rs::Encoding>,
    write_encoding: Option<&'static encoding_rs::Encoding>,
) -> Result<f32, Error> {
    let start_time = Instant::now();

    let mut processor = Processor {
        mode: Mode::Write,
        file_flags: FileFlags::all() & !skip_files,
        flags,
        duplicate_mode,
        read_encoding,
        write_encoding,
        ..Default::default()
    };

    processor.process(engine_type, source_path, translation_path, Some(output_path))?;

    Ok(start_time.elapsed().as_secs_f32())
}

pub(crate) fn read(
    source_path: &Path,
    translation_path: &Path,
    read_mode: Mode,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    skip_files: FileFlags,
    flags: BaseFlags,
    map_events: bool,
    hashes: HashMap<String, u64>,
    ini_title: &str,
    read_encoding: Option<&'static encoding_rs::Encoding>,
) -> Result<HashMap<String, u64>, Error> {
    let mut processor = Processor {
        mode: read_mode,
        file_flags: FileFlags::all().difference(skip_files),
        flags,
        duplicate_mode,
        game_title: ini_title.to_owned(),
        hashes,
        map_events,
        read_encoding,
        ..Default::default()
    };

    processor.process(engine_type, source_path, translation_path, None)?;

    Ok(processor.hashes)
}

pub(crate) fn purge(
    source_path: &Path,
    translation_path: &Path,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    flags: BaseFlags,
    skip_files: FileFlags,
    read_encoding: Option<&'static encoding_rs::Encoding>,
    write_encoding: Option<&'static encoding_rs::Encoding>,
) -> Result<(), Error> {
    let mut processor = Processor {
        mode: Mode::Purge,
        file_flags: FileFlags::all() & !skip_files,
        flags,
        duplicate_mode,
        read_encoding,
        write_encoding,
        ..Default::default()
    };

    processor.process(engine_type, source_path, translation_path, None)?;

    Ok(())
}

pub(crate) async fn translate_single(
    endpoint_settings: &str,
    source_language: &str,
    translation_language: &str,
    text: &str,
) -> Result<String, Error> {
    let endpoint_settings: EndpointSettings = serde_json::from_str(endpoint_settings)?;
    let endpoint = endpoint_settings.endpoint;

    let translated = match endpoint {
        #[cfg(feature = "google-translate")]
        TranslationEndpoint::Google => {
            use translators::{GoogleTranslator, Translator};

            GoogleTranslator::default()
                .translate_async(text, source_language, translation_language)
                .await?
        }
        #[cfg(not(feature = "google-translate"))]
        TranslationEndpoint::Google => return Err(Error::BackendNotCompiled("Google Translate")),

        #[cfg(feature = "yandex-translate")]
        TranslationEndpoint::Yandex => {
            use yandex_translate_v2::{TranslateRequest, YandexTranslateClient};

            let mut client = YandexTranslateClient::with_api_key(endpoint_settings.api_key)?;

            // Empty means the address of the API itself. Only the tests set it, to reach a server of their own.
            if let Some(base_url) = endpoint_settings.yandex_base_url.filter(|url| !url.is_empty()) {
                client = client.with_base_url(base_url);
            }

            let response = client
                .translate(&TranslateRequest {
                    folder_id: endpoint_settings.yandex_folder_id,
                    texts: &[text],
                    target_language_code: translation_language,
                    source_language_code: Some(source_language),
                })
                .await?;

            response
                .translations
                .into_iter()
                .next()
                .map(|x| x.text)
                .ok_or(Error::NoTranslation)?
        }
        #[cfg(not(feature = "yandex-translate"))]
        TranslationEndpoint::Yandex => return Err(Error::BackendNotCompiled("Yandex Translate")),

        #[cfg(feature = "deepl")]
        TranslationEndpoint::DeepL => {
            use deepl::*;

            let client = DeepLApi::with(endpoint_settings.api_key).new();
            client
                .translate_text(text, Lang::from_str(translation_language)?)
                .await?
                .to_string()
        }
        #[cfg(not(feature = "deepl"))]
        TranslationEndpoint::DeepL => return Err(Error::BackendNotCompiled("DeepL")),

        TranslationEndpoint::Script => return Err(Error::ScriptEndpoint),
    };

    Ok(translated)
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct EndpointSettings<'a> {
    api_key: &'a str,
    yandex_folder_id: &'a str,
    #[serde(default)]
    yandex_base_url: Option<&'a str>,
    endpoint: TranslationEndpoint,
}

pub(crate) fn extract_archive(
    input_path: &Path,
    output_path: &Path,
    read_encoding: Option<&'static encoding_rs::Encoding>,
) -> Result<(), Error> {
    debug!("Decrypting archive: {}", input_path.display());

    let mut bytes = fs::read(input_path).map_err(|err| Error::Io(input_path.to_path_buf(), err))?;

    let decrypted_entries = decrypt(&mut bytes)?;

    for file in decrypted_entries {
        // Decoded before the separators are changed: in Shift_JIS a 0x5C byte can be the second byte of a character
        let decoded = match read_encoding {
            Some(encoding) => encoding.decode_without_bom_handling(file.path).0,
            None => String::from_utf8_lossy(file.path),
        };
        let path = decoded.replace('\\', "/");
        debug!("Decrypting archive entry: {}", path);

        // The archive comes with a game that is not trusted: an entry named `..\..\x` or `C:\x` would be written
        // outside the folder it is extracted to
        if Path::new(&path)
            .components()
            .any(|component| !matches!(component, Component::Normal(_) | Component::CurDir))
        {
            warn!(
                "Skipped archive entry with a path that leaves the output folder: {}",
                path
            );
            continue;
        }

        let output_file_path = output_path.join(&path);

        if let Some(parent) = output_file_path.parent() {
            create_dir_all(parent).map_err(|err| Error::Io(parent.to_path_buf(), err))?;
        }

        fs::write(&output_file_path, file.data).map_err(|err| Error::Io(output_file_path, err))?;

        info!("Decrypted archive entry: {}", path);
    }

    info!("Decrypted archive: {}", input_path.display());

    Ok(())
}

pub(crate) fn find_all_matches(
    source_haystack: &str,
    source_needle: &str,
    source_mode: MatchModeInfo,
    tr_haystack: &str,
    tr_needle: &str,
    tr_mode: MatchModeInfo,
    source_algorithm: Algorithm,
    tr_algorithm: Algorithm,
) -> Result<Option<(Vec<MatchResult>, Vec<MatchResult>)>, language_tokenizer::Error> {
    // A language without a tokenizer is `Algorithm::None`, which `tokenize` takes for a snowball one and builds an
    // invalid enum from: the process aborts. Refused here instead, as any tokenizer that is missing is.
    for algorithm in [source_algorithm, tr_algorithm] {
        if matches!(algorithm, Algorithm::None) {
            return Err(language_tokenizer::Error::NoTokenizer(algorithm));
        }
    }

    let source_haystack = tokenize(source_haystack, source_algorithm, source_mode.case_sensitive)?;
    let source_needle = tokenize(source_needle, source_algorithm, source_mode.case_sensitive)?;
    let tr_haystack = tokenize(tr_haystack, tr_algorithm, tr_mode.case_sensitive)?;
    let tr_needle = tokenize(tr_needle, tr_algorithm, tr_mode.case_sensitive)?;

    let source_matches = find_all_matches_(
        &source_haystack,
        &source_needle,
        source_mode.mode,
        source_mode.permissive,
    );

    let source_has_needle = !source_matches.is_empty();

    Ok(if source_has_needle {
        let translation_matches = find_all_matches_(&tr_haystack, &tr_needle, tr_mode.mode, tr_mode.permissive);

        Some((source_matches, translation_matches))
    } else {
        None
    })
}

#[cfg(feature = "languagetool")]
const LANGUAGETOOL_TIMEOUT: Duration = Duration::from_secs(20);

#[cfg(feature = "languagetool")]
pub(crate) async fn language_tool_lint<'a>(
    data: Data<'a>,
    base_url: &str,
    api_key: Option<String>,
    algorithm: &str,
    dicts: Option<Vec<String>>,
    disabled_categories: Option<Vec<String>>,
    disabled_rules: Option<Vec<String>>,
    enabled_categories: Option<Vec<String>>,
    enabled_rules: Option<Vec<String>>,
    picky: bool,
    mother_tongue: Option<String>,
    preferred_variants: Option<Vec<String>>,
    username: Option<String>,
) -> Result<Vec<Match>, Error> {
    let mut client = ServerClient::new(base_url, "");
    client.client = reqwest::Client::builder().timeout(LANGUAGETOOL_TIMEOUT).build()?;

    let mut request = LTRequest::default()
        .with_language(algorithm.to_string())
        .with_data(data);

    request.dicts = dicts;
    request.disabled_categories = disabled_categories;
    request.disabled_rules = disabled_rules;
    request.enabled_categories = enabled_categories;
    request.enabled_rules = enabled_rules;
    request.level = if picky { Level::Picky } else { Level::Default };
    request.mother_tongue = mother_tongue;
    request.preferred_variants = preferred_variants;
    request.username = username;
    request.api_key = api_key;

    let response = client.check(&request).await?;

    Ok(response.matches)
}

pub(crate) fn decrypt_asset(path: &Path) -> Result<Vec<u8>, Error> {
    use rpgm_asset_decrypter_lib::{FileType, MV_M4A_EXT, MV_OGG_EXT, MZ_M4A_EXT, MZ_OGG_EXT, decrypt_in_place};

    let mut file_content = fs::read(path).map_err(|err| Error::Io(path.to_path_buf(), err))?;
    let Some(extension) = path.extension() else {
        return Err(Error::Io(
            path.to_path_buf(),
            io::Error::new(
                io::ErrorKind::InvalidInput,
                "the file has no extension to tell its type by",
            ),
        ));
    };

    let file_type = if extension == MV_M4A_EXT || extension == MZ_M4A_EXT {
        FileType::M4a
    } else if extension == MV_OGG_EXT || extension == MZ_OGG_EXT {
        FileType::Ogg
    } else {
        FileType::Png
    };

    decrypt_in_place(&mut file_content, file_type)?;

    Ok(file_content)
}

pub(crate) fn generate_json(content: &[u8], filename: &str) -> Result<String, Error> {
    use rvpacker_txt_rs_lib::json::*;

    let json: String;

    if filename.ends_with(".lmu") {
        json = generate_rm2k_map_file(content)?;
    } else if filename.ends_with(".ldb") {
        json = generate_rm2k_database_file(content)?;
    } else if filename.ends_with(".lmt") {
        json = generate_rm2k_tree_map_file(content)?;
    } else if filename.ends_with(".lsd") {
        json = generate_rm2k_save_file(content)?;
    } else if filename.ends_with(".rpgsave") {
        let compressed = std::str::from_utf8(content).map_err(|_| Error::InvalidSave)?;
        let raw = rvpacker_txt_rs_lib::save::load(compressed).ok_or(Error::InvalidSave)?;
        json = serde_json::to_string_pretty(&serde_json::from_str::<serde_json::Value>(&raw)?)?;
    } else {
        // The preview has no project, so the text is read as UTF-8
        json = generate_file(content, filename, None)?;
    }

    Ok(json)
}

#[repr(u8)]
pub enum SerdeFormat {
    Csv,
    Xlsx,
    Xml,
    Json,
    Yaml,
}

pub(crate) fn serde_export(content: &str, format: SerdeFormat) -> Result<Vec<u8>, Error> {
    #[cfg(feature = "serde-csv")]
    use rvpacker_txt_rs_lib::serde::export_csv;
    use rvpacker_txt_rs_lib::serde::export_json;
    #[cfg(feature = "serde-xlsx")]
    use rvpacker_txt_rs_lib::serde::export_xlsx;
    #[cfg(feature = "serde-xml")]
    use rvpacker_txt_rs_lib::serde::export_xml;
    #[cfg(feature = "serde-yaml")]
    use rvpacker_txt_rs_lib::serde::export_yaml;

    match format {
        #[cfg(feature = "serde-csv")]
        SerdeFormat::Csv => export_csv(content).map(String::into_bytes),
        #[cfg(not(feature = "serde-csv"))]
        SerdeFormat::Csv => return Err(Error::BackendNotCompiled("CSV")),

        #[cfg(feature = "serde-xlsx")]
        SerdeFormat::Xlsx => export_xlsx(content),
        #[cfg(not(feature = "serde-xlsx"))]
        SerdeFormat::Xlsx => return Err(Error::BackendNotCompiled("XLSX")),

        #[cfg(feature = "serde-xml")]
        SerdeFormat::Xml => export_xml(content).map(String::into_bytes),
        #[cfg(not(feature = "serde-xml"))]
        SerdeFormat::Xml => return Err(Error::BackendNotCompiled("XML")),

        SerdeFormat::Json => export_json(content).map(String::into_bytes),

        #[cfg(feature = "serde-yaml")]
        SerdeFormat::Yaml => export_yaml(content).map(String::into_bytes),
        #[cfg(not(feature = "serde-yaml"))]
        SerdeFormat::Yaml => return Err(Error::BackendNotCompiled("YAML")),
    }
    .map_err(|err| Error::Serde(err.to_string()))
}

/// The translation text of an exported file.
///
/// The importers of the library read what they are given as far as they can: a CSV of other columns, or an XML that
/// never closes, gives some text or none. That replaces a translation file, so a file that holds no entries is refused,
/// and so is a CSV that does not start with the header the export writes.
pub(crate) fn serde_import(bytes: &[u8], format: SerdeFormat) -> Result<String, Error> {
    #[cfg(feature = "serde-csv")]
    if let SerdeFormat::Csv = format {
        let text = std::str::from_utf8(bytes).map_err(|err| Error::Serde(err.to_string()))?;

        if !text.trim_start_matches('\u{feff}').starts_with("type,source,translation_count") {
            return Err(Error::Serde(
                "the first row is not the header `type,source,translation_count,...` of an exported file".to_owned(),
            ));
        }
    }

    let imported = import_entries(bytes, format)?;

    if imported.trim().is_empty() {
        return Err(Error::Serde("the file holds no entries".to_owned()));
    }

    Ok(imported)
}

fn import_entries(bytes: &[u8], format: SerdeFormat) -> Result<String, Error> {
    #[cfg(feature = "serde-csv")]
    use rvpacker_txt_rs_lib::serde::import_csv;
    use rvpacker_txt_rs_lib::serde::import_json;
    #[cfg(feature = "serde-xlsx")]
    use rvpacker_txt_rs_lib::serde::import_xlsx;
    #[cfg(feature = "serde-xml")]
    use rvpacker_txt_rs_lib::serde::import_xml;
    #[cfg(feature = "serde-yaml")]
    use rvpacker_txt_rs_lib::serde::import_yaml;

    if let SerdeFormat::Xlsx = format {
        #[cfg(feature = "serde-xlsx")]
        {
            return import_xlsx(bytes).map_err(|err| Error::Serde(err.to_string()));
        }
        #[cfg(not(feature = "serde-xlsx"))]
        {
            return Err(Error::BackendNotCompiled("XLSX"));
        }
    }

    let text = std::str::from_utf8(bytes).map_err(|err| Error::Serde(err.to_string()))?;

    match format {
        #[cfg(feature = "serde-csv")]
        SerdeFormat::Csv => import_csv(text),
        #[cfg(not(feature = "serde-csv"))]
        SerdeFormat::Csv => return Err(Error::BackendNotCompiled("CSV")),

        #[cfg(feature = "serde-xml")]
        SerdeFormat::Xml => import_xml(text),
        #[cfg(not(feature = "serde-xml"))]
        SerdeFormat::Xml => return Err(Error::BackendNotCompiled("XML")),

        SerdeFormat::Json => import_json(text),

        #[cfg(feature = "serde-yaml")]
        SerdeFormat::Yaml => import_yaml(text),
        #[cfg(not(feature = "serde-yaml"))]
        SerdeFormat::Yaml => return Err(Error::BackendNotCompiled("YAML")),

        SerdeFormat::Xlsx => unreachable!(),
    }
    .map_err(|err| Error::Serde(err.to_string()))
}

#[repr(u8)]
pub enum HighlightLanguage {
    JS,
    JSON,
    Ruby,
}

#[cfg(any(
    feature = "json-highlighting",
    feature = "js-highlighting",
    feature = "ruby-highlighting"
))]
const HIGHLIGHT_THEME: &str = "onedark";

#[cfg(any(
    feature = "json-highlighting",
    feature = "js-highlighting",
    feature = "ruby-highlighting"
))]
pub(crate) fn highlight_code(input: &str, lang: HighlightLanguage) -> Result<String, Error> {
    use lumis::{HtmlInlineBuilder, themes, write_highlight};

    let language = match lang {
        HighlightLanguage::JSON => {
            #[cfg(feature = "json-highlighting")]
            {
                lumis::languages::Language::JSON
            }
            #[cfg(not(feature = "json-highlighting"))]
            unreachable!("JSON highlighting was not compiled in")
        }
        HighlightLanguage::JS => {
            #[cfg(feature = "js-highlighting")]
            {
                lumis::languages::Language::JavaScript
            }
            #[cfg(not(feature = "js-highlighting"))]
            unreachable!("JS highlighting was not compiled in")
        }
        HighlightLanguage::Ruby => {
            #[cfg(feature = "ruby-highlighting")]
            {
                lumis::languages::Language::Ruby
            }
            #[cfg(not(feature = "ruby-highlighting"))]
            unreachable!("Ruby highlighting was not compiled in")
        }
    };

    let theme = themes::get(HIGHLIGHT_THEME).expect("bundled lumis theme is always present");

    let mut builder = HtmlInlineBuilder::new();
    builder.language(language).theme(Some(theme));
    let formatter = builder.build()?;

    let mut out = Vec::new();
    // Vec<u8>'s Write impl never errors, so this can't actually fail.
    write_highlight(&mut out, input, formatter).expect("writing to an in-memory Vec<u8> cannot fail");

    Ok(String::from_utf8(out).unwrap_or_default())
}

/// Returns the extracted title alongside whether it was parsed from `RPG_RT.ini` (RM2K/2003)
/// rather than `Game.ini` (XP/VX/VXAce).
pub(crate) fn get_ini_title(project_path: &str) -> Result<(Vec<u8>, bool), Error> {
    let ini_path = Path::new(project_path).join("Game.ini");

    match fs::read(&ini_path) {
        Ok(content) => Ok((rvpacker_txt_rs_lib::get_ini_title(&content)?, false)),
        Err(err) if err.kind() == io::ErrorKind::NotFound => {
            let rm2k_ini_path = Path::new(project_path).join("RPG_RT.ini");
            let content = fs::read(&rm2k_ini_path).map_err(|err| Error::Io(rm2k_ini_path, err))?;
            Ok((rvpacker_txt_rs_lib::get_ini_title_rm2k(&content)?, true))
        }
        Err(err) => Err(Error::Io(ini_path, err)),
    }
}

#[cfg(any(feature = "js-formatting", feature = "ruby-formatting"))]
pub(crate) fn format_src(src: &str, lang: HighlightLanguage) -> Result<String, Error> {
    match lang {
        #[cfg(feature = "js-formatting")]
        HighlightLanguage::JS => {
            use dprint_plugin_typescript::{configuration::*, *};
            let config = ConfigurationBuilder::new().build();

            // The path is how dprint finds the language and the file's specifier: an empty one is refused
            let result = format_text(FormatTextOptions {
                path: Path::new("script.js"),
                extension: None,
                text: src.into(),
                config: &config,
                external_formatter: None,
                arena_pool: None,
            })
            .map_err(|err| Error::Dprint(anyhow::anyhow!("{err}")))?;

            // `None` means the text needs no change (it has dprint's ignore comment)
            Ok(result.unwrap_or_else(|| src.to_owned()))
        }
        #[cfg(not(feature = "js-formatting"))]
        HighlightLanguage::JS => return Err(Error::BackendNotCompiled("JavaScript formatting")),

        #[cfg(feature = "ruby-formatting")]
        HighlightLanguage::Ruby => {
            use rubyfmt::format_buffer;
            let formatted = format_buffer(src.as_bytes()).map_err(|err| Error::Rubyfmt(err.as_exit_code()))?;
            Ok(String::from_utf8_lossy(&formatted).into_owned())
        }
        #[cfg(not(feature = "ruby-formatting"))]
        HighlightLanguage::Ruby => return Err(Error::BackendNotCompiled("Ruby formatting")),

        _ => unreachable!(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn project(name: &str) -> PathBuf {
        let path = std::env::temp_dir().join(format!("rpgmtranslate-test-ini-{name}-{}", std::process::id()));
        let _ = fs::remove_dir_all(&path);
        fs::create_dir_all(&path).unwrap();
        path
    }

    #[test]
    fn ini_title_is_read_from_game_ini() {
        let dir = project("mv");
        fs::write(
            dir.join("Game.ini"),
            "[Game]\r\nTitleFont=Other\r\ntitle = My Game \r\n",
        )
        .unwrap();

        let (title, is_rm2k) = get_ini_title(dir.to_str().unwrap()).unwrap();

        // The whole key must match: `TitleFont` is not the title
        assert_eq!(title, b"My Game");
        assert!(!is_rm2k);

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn ini_title_falls_back_to_rpg_rt_ini() {
        let dir = project("rm2k");
        fs::write(dir.join("RPG_RT.ini"), "[RPG_RT]\nGameTitle=Old Game\n").unwrap();

        let (title, is_rm2k) = get_ini_title(dir.to_str().unwrap()).unwrap();

        assert_eq!(title, b"Old Game");
        assert!(is_rm2k);

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn ini_title_errors_name_the_missing_file_or_the_missing_key() {
        let dir = project("none");

        assert!(matches!(get_ini_title(dir.to_str().unwrap()), Err(Error::Io(path, _)) if path.ends_with("RPG_RT.ini")));

        fs::write(dir.join("Game.ini"), "[Game]\nFont=x\n").unwrap();

        assert!(matches!(get_ini_title(dir.to_str().unwrap()), Err(Error::Rvpacker(_))));

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn serde_json_round_trips_a_translation_file() {
        let text = "<!>ID<#>1\nHarold<#>Гарольд\nLine1\\#Line2<#>a<#>b\nBob<#>";
        let exported = serde_export(text, SerdeFormat::Json).unwrap();

        assert_eq!(serde_import(&exported, SerdeFormat::Json).unwrap(), text);
    }

    #[test]
    fn serde_import_refuses_a_file_without_entries() {
        // It would replace a translation file with nothing
        assert!(serde_import(b"[]", SerdeFormat::Json).is_err());
        assert!(serde_import(b"", SerdeFormat::Json).is_err());
        assert!(serde_import(b"  \n", SerdeFormat::Json).is_err());
    }

    #[test]
    fn serde_import_refuses_text_that_is_not_utf8() {
        assert!(serde_import(&[0xFF, 0xFE, 0x00], SerdeFormat::Json).is_err());
    }

    // The CSV header is read as text before anything parses the file, so only the encoding can be what refuses it
    #[cfg(feature = "serde-csv")]
    #[test]
    fn serde_csv_refuses_text_that_is_not_utf8() {
        let error = serde_import(b"type,source,translation_count\n\xFF\xFE", SerdeFormat::Csv).unwrap_err();

        assert!(matches!(&error, Error::Serde(message) if message.contains("utf-8")), "{error}");
    }

    #[cfg(feature = "serde-csv")]
    #[test]
    fn serde_csv_needs_the_header_of_an_export() {
        let exported = serde_export("A<#>b", SerdeFormat::Csv).unwrap();
        assert_eq!(serde_import(&exported, SerdeFormat::Csv).unwrap(), "A<#>b");

        // A spreadsheet adds a byte order mark when it saves a file as UTF-8
        let mut with_mark = "\u{feff}".as_bytes().to_vec();
        with_mark.extend(&exported);
        assert_eq!(serde_import(&with_mark, SerdeFormat::Csv).unwrap(), "A<#>b");

        assert!(serde_import(b"not,a,translation\n1,2,3\n", SerdeFormat::Csv).is_err());
        assert!(serde_import(b"type,source,translation_count\n", SerdeFormat::Csv).is_err());
    }

    #[cfg(feature = "serde-xml")]
    #[test]
    fn serde_xml_without_entries_is_refused() {
        assert!(serde_import(b"<open><unclosed>", SerdeFormat::Xml).is_err());

        // A well-formed export that holds no entries
        let empty = serde_export("", SerdeFormat::Xml).unwrap();
        assert!(matches!(serde_import(&empty, SerdeFormat::Xml), Err(Error::Serde(_))));
    }

    // XLSX is binary, XML and YAML are text: each has to give back exactly what was exported.
    #[cfg(any(feature = "serde-xlsx", feature = "serde-xml", feature = "serde-yaml"))]
    fn assert_round_trip(format: fn() -> SerdeFormat) {
        let text = "<!>ID<#>1\nHarold<#>Гарольд\nLine1\\#Line2<#>a<#>b\nBob<#>";
        let exported = serde_export(text, format()).unwrap();

        assert!(!exported.is_empty());
        assert_eq!(serde_import(&exported, format()).unwrap(), text);
        assert!(serde_import(b"", format()).is_err());
    }

    #[cfg(feature = "serde-xlsx")]
    #[test]
    fn serde_xlsx_round_trips_a_translation_file() {
        assert_round_trip(|| SerdeFormat::Xlsx);
        // Not a workbook
        assert!(serde_import(b"plain text", SerdeFormat::Xlsx).is_err());
    }

    #[cfg(feature = "serde-xml")]
    #[test]
    fn serde_xml_round_trips_a_translation_file() {
        assert_round_trip(|| SerdeFormat::Xml);
    }

    #[cfg(feature = "serde-yaml")]
    #[test]
    fn serde_yaml_round_trips_a_translation_file() {
        assert_round_trip(|| SerdeFormat::Yaml);
        assert!(serde_import(b"- [unclosed", SerdeFormat::Yaml).is_err());
    }

    #[cfg(feature = "js-highlighting")]
    #[test]
    fn javascript_is_highlighted() {
        let html = highlight_code("const answer = 42; // <b>", HighlightLanguage::JS).unwrap();

        assert!(html.contains("<span"));
        assert!(html.contains("&lt;b&gt;"));
    }

    #[cfg(feature = "ruby-highlighting")]
    #[test]
    fn ruby_is_highlighted() {
        let html = highlight_code("puts 'hi' # <b>", HighlightLanguage::Ruby).unwrap();

        assert!(html.contains("<span"));
        assert!(html.contains("&lt;b&gt;"));
    }

    fn scratch(name: &str) -> PathBuf {
        let path = std::env::temp_dir().join(format!("rpgmtranslate-test-api-{name}-{}", std::process::id()));
        let _ = fs::remove_dir_all(&path);
        fs::create_dir_all(&path).unwrap();
        path
    }

    fn block_on<F: std::future::Future>(future: F) -> F::Output {
        tokio::runtime::Builder::new_current_thread()
            .build()
            .unwrap()
            .block_on(future)
    }

    #[test]
    fn endpoints_are_read_from_their_numbers() {
        assert_eq!(TranslationEndpoint::from(0), TranslationEndpoint::Google);
        assert_eq!(TranslationEndpoint::from(1), TranslationEndpoint::Yandex);
        assert_eq!(TranslationEndpoint::from(2), TranslationEndpoint::DeepL);
        assert_eq!(TranslationEndpoint::from(3), TranslationEndpoint::Script);
        // Settings written by another version may hold a number that no endpoint has
        assert_eq!(TranslationEndpoint::from(200), TranslationEndpoint::Google);
        assert_eq!(TranslationEndpoint::default(), TranslationEndpoint::Google);
        assert_eq!(
            serde_json::from_str::<TranslationEndpoint>("2").unwrap(),
            TranslationEndpoint::DeepL
        );
    }

    #[test]
    fn translating_needs_a_backend_the_core_has() {
        let script = r#"{"apiKey":"","yandexFolderId":"","endpoint":3}"#;
        let result = block_on(translate_single(script, "ja", "en", "text"));
        assert!(matches!(result, Err(Error::ScriptEndpoint)));

        let result = block_on(translate_single("not json", "ja", "en", "text"));
        assert!(matches!(result, Err(Error::JSON(_))));

        // A missing key is a malformed request, not a default
        let result = block_on(translate_single(r#"{"endpoint":0}"#, "ja", "en", "text"));
        assert!(matches!(result, Err(Error::JSON(_))));
    }

    #[cfg(not(feature = "google-translate"))]
    #[test]
    fn a_backend_that_was_not_compiled_is_named() {
        let google = r#"{"apiKey":"","yandexFolderId":"","endpoint":0}"#;

        match block_on(translate_single(google, "ja", "en", "text")) {
            Err(Error::BackendNotCompiled(name)) => assert_eq!(name, "Google Translate"),
            other => panic!("expected a missing backend, got {other:?}"),
        }
    }

    #[test]
    fn errors_tell_what_went_wrong() {
        let missing = Error::Io(PathBuf::from("a/b.txt"), io::Error::new(io::ErrorKind::NotFound, "gone"));
        // The message names the path and what the OS said, whatever the words around them
        let message = missing.to_string();
        assert!(message.contains("a/b.txt") && message.contains("gone"), "{message}");

        assert!(Error::InvalidSave.to_string().contains("save file"));
        assert!(Error::BackendNotCompiled("CSV").to_string().starts_with("CSV support"));
        let message = Error::TooManyValues(5, 3).to_string();
        assert!(message.contains('5') && message.contains('3'), "{message}");
        assert_eq!(Error::Serde("bad".to_owned()).to_string(), "bad");
    }

    // Searching needs a tokenizer, which only a crate feature provides: `snowball` is the one with no heavy dependencies
    #[cfg(feature = "snowball")]
    fn mode(case_sensitive: bool) -> MatchModeInfo {
        MatchModeInfo {
            mode: MatchMode::Exact,
            case_sensitive,
            permissive: false,
        }
    }

    #[cfg(feature = "snowball")]
    #[test]
    fn matches_are_looked_for_in_the_translation_only_if_the_source_has_the_term() {
        let found = find_all_matches(
            "The HP is low",
            "HP",
            mode(true),
            "Здоровье мало: HP",
            "HP",
            mode(true),
            Algorithm::English,
            Algorithm::English,
        )
        .unwrap();

        let (source, translation) = found.unwrap();
        assert_eq!(source.len(), 1);
        assert_eq!(translation.len(), 1);

        // No term in the source: there is nothing to compare, whatever the translation holds
        let none = find_all_matches(
            "Nothing here",
            "HP",
            mode(true),
            "HP",
            "HP",
            mode(true),
            Algorithm::English,
            Algorithm::English,
        )
        .unwrap();
        assert!(none.is_none());

        // The term in the source but not in the translation: the source matches still come back
        let (source, translation) = find_all_matches(
            "HP and HP",
            "HP",
            mode(true),
            "no term",
            "HP",
            mode(true),
            Algorithm::English,
            Algorithm::English,
        )
        .unwrap()
        .unwrap();
        assert_eq!(source.len(), 2);
        assert!(translation.is_empty());
    }

    #[cfg(feature = "snowball")]
    #[test]
    fn matching_honours_case_of_each_side() {
        let sensitive = |source: bool, translation: bool| {
            let (s, t) = find_all_matches(
                "the hp",
                "HP",
                mode(source),
                "the hp",
                "HP",
                mode(translation),
                Algorithm::English,
                Algorithm::English,
            )
            .unwrap()
            .unwrap_or_default();
            (s.len(), t.len())
        };

        assert_eq!(sensitive(false, false), (1, 1));
        assert_eq!(sensitive(false, true), (1, 0));
        // The source side finds nothing, so the translation side is not looked at
        assert_eq!(sensitive(true, false), (0, 0));
    }

    fn archive(entries: &[(&[u8], &[u8])], engine: rpgmad_lib::Engine) -> Vec<u8> {
        let entries: Vec<rpgmad_lib::ArchiveEntry> = entries
            .iter()
            .map(|&(path, data)| rpgmad_lib::ArchiveEntry { path, data })
            .collect();

        let mut buffer = vec![0; rpgmad_lib::encrypted_buffer_size(&entries, engine)];
        rpgmad_lib::encrypt(&entries, engine, &mut buffer).unwrap();
        buffer
    }

    #[test]
    fn archive_entries_keep_their_folders() {
        for (name, engine) in [("older", rpgmad_lib::Engine::Older), ("vxace", rpgmad_lib::Engine::Vxace)] {
            let dir = scratch(&format!("archive-folders-{name}"));
            let bytes = archive(
                &[
                    (b"Data\\Actors.rxdata", b"actors"),
                    (b"Graphics/Pictures/face.png", b"png"),
                    (b"Scripts.rxdata", b""),
                ],
                engine,
            );
            let archive_path = dir.join("Game.rgssad");
            fs::write(&archive_path, bytes).unwrap();

            let out = dir.join("out");
            extract_archive(&archive_path, &out, None).unwrap();

            assert_eq!(fs::read(out.join("Data/Actors.rxdata")).unwrap(), b"actors");
            assert_eq!(fs::read(out.join("Graphics/Pictures/face.png")).unwrap(), b"png");
            assert_eq!(fs::read(out.join("Scripts.rxdata")).unwrap(), b"");

            let _ = fs::remove_dir_all(dir);
        }
    }

    // The archive comes with a game that is not trusted
    #[test]
    fn archive_entries_cannot_leave_the_output_folder() {
        let dir = scratch("archive-traversal");
        let bytes = archive(
            &[
                (b"..\\evil.txt", b"x"),
                (b"a/../../evil2.txt", b"x"),
                (b"/absolute.txt", b"x"),
                (b"safe.txt", b"safe"),
            ],
            rpgmad_lib::Engine::Older,
        );
        let archive_path = dir.join("Game.rgssad");
        fs::write(&archive_path, bytes).unwrap();

        let out = dir.join("out");
        extract_archive(&archive_path, &out, None).unwrap();

        assert_eq!(fs::read(out.join("safe.txt")).unwrap(), b"safe");
        assert!(!dir.join("evil.txt").exists());
        assert!(!dir.join("evil2.txt").exists());
        assert!(!out.join("absolute.txt").exists());
        assert!(!Path::new("/absolute.txt").exists());

        let _ = fs::remove_dir_all(dir);
    }

    // In Shift_JIS the second byte of 表 is 0x5C, a backslash: splitting the raw bytes would cut the name there
    #[test]
    fn archive_names_are_decoded_before_the_separators_are_read() {
        let dir = scratch("archive-sjis");
        let bytes = archive(
            &[(&[0x95, 0x5C, b'.', b't', b'x', b't'], b"one"), (b"Data\\\x95\x5C.bin", b"two")],
            rpgmad_lib::Engine::Older,
        );
        let archive_path = dir.join("Game.rgssad");
        fs::write(&archive_path, bytes).unwrap();

        let out = dir.join("out");
        extract_archive(&archive_path, &out, Some(encoding_rs::SHIFT_JIS)).unwrap();

        assert_eq!(fs::read(out.join("表.txt")).unwrap(), b"one");
        assert_eq!(fs::read(out.join("Data").join("表.bin")).unwrap(), b"two");

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn archive_that_cannot_be_read_is_an_error() {
        let dir = scratch("archive-errors");

        let missing = extract_archive(&dir.join("missing.rgssad"), &dir.join("out"), None);
        assert!(matches!(missing, Err(Error::Io(..))));

        let garbage = dir.join("garbage.rgssad");
        fs::write(&garbage, b"this is not an archive at all").unwrap();
        assert!(matches!(extract_archive(&garbage, &dir.join("out"), None), Err(Error::Extract(_))));

        let truncated = archive(&[(b"a.txt", b"some content")], rpgmad_lib::Engine::Older);
        let cut = dir.join("cut.rgssad");
        fs::write(&cut, &truncated[..truncated.len() - 4]).unwrap();
        assert!(extract_archive(&cut, &dir.join("out"), None).is_err());

        let _ = fs::remove_dir_all(dir);
    }

    fn png() -> Vec<u8> {
        let mut png = vec![0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, b'I', b'H', b'D', b'R'];
        // 4x4, 8 bits, RGBA, no compression, filter or interlace
        png.extend([0, 0, 0, 4, 0, 0, 0, 4, 8, 6, 0, 0, 0]);
        png.extend([1, 2, 3, 4, 5, 6, 7, 8]);
        png
    }

    #[test]
    fn assets_are_decrypted_by_their_extension() {
        let dir = scratch("assets");
        let plain = png();
        let key = "d41d8cd98f00b204e9800998ecf8427e";
        let encrypted = rpgm_asset_decrypter_lib::encrypt(&plain, rpgm_asset_decrypter_lib::FileType::Png, key).unwrap();

        for extension in ["rpgmvp", "png_", "RPGMVP", "unknown"] {
            let path = dir.join(format!("picture.{extension}"));
            fs::write(&path, &encrypted).unwrap();

            // Decrypted in place: the fake header stays in front, and the callers skip it
            let decrypted = decrypt_asset(&path).unwrap();
            assert_eq!(&decrypted[..5], b"RPGMV", "{extension}");
            assert_eq!(&decrypted[16..], plain, "{extension}");
        }

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn assets_that_cannot_be_decrypted_are_errors() {
        let dir = scratch("assets-errors");

        assert!(matches!(decrypt_asset(&dir.join("missing.rpgmvp")), Err(Error::Io(..))));

        // The type is told by the extension, so a file without one cannot be handled
        let bare = dir.join("noextension");
        fs::write(&bare, b"data").unwrap();
        assert!(matches!(decrypt_asset(&bare), Err(Error::Io(..))));

        let garbage = dir.join("garbage.rpgmvp");
        fs::write(&garbage, b"not an encrypted picture").unwrap();
        assert!(matches!(decrypt_asset(&garbage), Err(Error::AssetDecrypt(_))));

        let empty = dir.join("empty.ogg_");
        fs::write(&empty, b"").unwrap();
        assert!(matches!(decrypt_asset(&empty), Err(Error::AssetDecrypt(_))));

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn save_files_are_shown_as_json() {
        let save = rvpacker_txt_rs_lib::save::dump(r#"{"gold":120,"party":["Harold"]}"#);

        let shown = generate_json(save.as_bytes(), "file1.rpgsave").unwrap();
        let value: serde_json::Value = serde_json::from_str(&shown).unwrap();

        assert_eq!(value["gold"], 120);
        assert_eq!(value["party"][0], "Harold");
        // Pretty printed, for a person to read
        assert!(shown.contains('\n'));
    }

    #[test]
    fn broken_save_files_are_refused() {
        assert!(matches!(generate_json(&[0xFF, 0xFE], "file1.rpgsave"), Err(Error::InvalidSave)));
        assert!(matches!(generate_json(b"not compressed json", "file1.rpgsave"), Err(Error::InvalidSave)));

        // Compressed, but not JSON
        let not_json = rvpacker_txt_rs_lib::save::dump("{ not json");
        assert!(matches!(generate_json(not_json.as_bytes(), "file1.rpgsave"), Err(Error::JSON(_))));
    }

    #[test]
    fn a_language_without_a_tokenizer_is_refused_on_either_side() {
        let none = |source, translation| {
            find_all_matches("a", "a", mode(true), "a", "a", mode(true), source, translation)
        };

        assert!(matches!(
            none(Algorithm::None, Algorithm::None),
            Err(language_tokenizer::Error::NoTokenizer(_))
        ));
        assert!(matches!(
            none(Algorithm::English, Algorithm::None),
            Err(language_tokenizer::Error::NoTokenizer(_))
        ));
        assert!(matches!(
            none(Algorithm::None, Algorithm::English),
            Err(language_tokenizer::Error::NoTokenizer(_))
        ));
    }

    #[test]
    fn audio_assets_are_told_by_their_extension_and_refused_when_broken() {
        let dir = scratch("assets-audio");

        for extension in ["rpgmvm", "m4a_"] {
            let path = dir.join(format!("song.{extension}"));
            fs::write(&path, b"not an encrypted song").unwrap();

            assert!(matches!(decrypt_asset(&path), Err(Error::AssetDecrypt(_))), "{extension}");
        }

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn a_file_is_read_without_a_project_as_what_its_extension_says() {
        // The preview has no project to tell the engine, so the content is decoded as that kind of file and refused
        let error = generate_json(b"[null]", "Actors.rvdata2").unwrap_err();
        assert!(matches!(error, Error::Rvpacker(_)));
        assert!(error.to_string().contains("Marshal"), "{error}");
    }

    #[test]
    fn rm2k_files_are_picked_by_their_extension() {
        // The decoder of the RPG Maker 2000/2003 files is the one that refuses them, not the Marshal one of the others
        for name in ["Map0001.lmu", "RPG_RT.ldb", "RPG_RT.lmt", "Save01.lsd"] {
            let error = generate_json(b"definitely not an RPG Maker file", name).unwrap_err().to_string();
            assert!(error.contains("2000/2003"), "{name}: {error}");
        }

        let error = generate_json(b"definitely not an RPG Maker file", "Actors.rvdata2").unwrap_err().to_string();
        assert!(error.contains("Marshal") && !error.contains("2000/2003"), "{error}");
    }

    #[cfg(feature = "json-highlighting")]
    #[test]
    fn highlighted_code_is_html_with_the_markup_escaped() {
        let html = highlight_code(r#"{"text": "<b>bold</b>"}"#, HighlightLanguage::JSON).unwrap();

        assert!(html.contains("<span"));
        assert!(!html.contains("<b>"));
        assert!(html.contains("&lt;b&gt;"));
        assert!(highlight_code("", HighlightLanguage::JSON).unwrap().len() < html.len());
    }

    #[cfg(feature = "js-formatting")]
    #[test]
    fn javascript_is_formatted() {
        assert_eq!(format_src("var a=1", HighlightLanguage::JS).unwrap(), "var a = 1;\n");
        assert!(format_src("var = = ;", HighlightLanguage::JS).is_err());
    }

    #[cfg(feature = "ruby-formatting")]
    #[test]
    fn ruby_is_formatted() {
        let formatted = format_src("def hello;puts 'hi';end", HighlightLanguage::Ruby).unwrap();

        assert!(formatted.starts_with("def hello\n"));
        assert!(format_src("def (", HighlightLanguage::Ruby).is_err());
    }

    // A minimal MV/MZ game with two actors
    fn mv_game(name: &str) -> (PathBuf, PathBuf, PathBuf) {
        let dir = scratch(name);
        let source = dir.join("data");
        fs::create_dir_all(&source).unwrap();

        let actor = |id: u32, name: &str, nickname: &str| {
            format!(
                r#"{{"id":{id},"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"{name}","nickname":"{nickname}","note":"","profile":"A hero"}}"#
            )
        };
        fs::write(source.join("Actors.json"), format!("[null,{}]", actor(1, "Harold", "The Brave"))).unwrap();

        (dir.clone(), source, dir.join("translation"))
    }

    /// Everything but the actors, which is what a read, write or purge of them leaves out
    fn all_but_actors() -> FileFlags {
        FileFlags::all().difference(FileFlags::Actors)
    }

    fn read_actors(source: &Path, translation: &Path, mode: Mode, hashes: HashMap<String, u64>) -> Result<HashMap<String, u64>, Error> {
        read(
            source,
            translation,
            mode,
            EngineType::MVMZ,
            DuplicateMode::Allow,
            all_but_actors(),
            BaseFlags::empty(),
            false,
            hashes,
            "Test Game",
            None,
        )
    }

    const DEFAULT: Mode = Mode::Read { append: false, force: false };
    const FORCE: Mode = Mode::Read { append: false, force: true };
    const APPEND: Mode = Mode::Read { append: true, force: false };

    #[test]
    fn reading_extracts_the_text_and_reports_a_hash_per_file() {
        let (dir, source, translation) = mv_game("read");

        let hashes = read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();

        assert_eq!(hashes.len(), 1);
        assert!(hashes.contains_key("actors"));
        assert_eq!(
            fs::read_to_string(translation.join("actors.txt")).unwrap(),
            "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>\nThe Brave<#>"
        );

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn reading_keeps_a_translation_file_unless_forced() {
        let (dir, source, translation) = mv_game("read-force");
        read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();

        let file = translation.join("actors.txt");
        fs::write(&file, "mine").unwrap();

        read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();
        assert_eq!(fs::read_to_string(&file).unwrap(), "mine");

        read_actors(&source, &translation, FORCE, HashMap::default()).unwrap();
        assert!(fs::read_to_string(&file).unwrap().contains("Harold<#>"));

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn appending_adds_new_text_and_keeps_translations() {
        let (dir, source, translation) = mv_game("read-append");
        let hashes = read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();

        let file = translation.join("actors.txt");
        fs::write(&file, "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>").unwrap();

        // Nothing changed in the game
        read_actors(&source, &translation, APPEND, hashes.clone()).unwrap();
        assert!(fs::read_to_string(&file).unwrap().contains("Harold<#>Гарольд"));

        // The game got a second actor
        let actor = r#"{"id":2,"battlerName":"","characterIndex":0,"characterName":"","classId":1,"equips":[0,0,0,0,0],"faceIndex":0,"faceName":"","traits":[],"initialLevel":1,"maxLevel":99,"name":"Alice","nickname":"","note":"","profile":""}"#;
        let first = fs::read_to_string(source.join("Actors.json")).unwrap();
        fs::write(source.join("Actors.json"), first.replace("}]", &format!("}},{actor}]"))).unwrap();

        let new_hashes = read_actors(&source, &translation, APPEND, hashes.clone()).unwrap();
        let text = fs::read_to_string(&file).unwrap();
        assert!(text.contains("Harold<#>Гарольд"), "{text}");
        assert!(text.contains("Alice<#>"), "{text}");
        // The file changed, so its hash did
        assert_ne!(new_hashes["actors"], hashes["actors"]);

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn reading_a_game_that_is_not_there_is_an_error() {
        let dir = scratch("read-missing");

        let error = read_actors(&dir.join("data"), &dir.join("translation"), DEFAULT, HashMap::default()).unwrap_err();

        assert!(error.to_string().contains("data"), "{error}");

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn writing_puts_translations_into_a_copy_of_the_game() {
        let (dir, source, translation) = mv_game("write");
        read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();
        fs::write(
            translation.join("actors.txt"),
            "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>Храбрый",
        )
        .unwrap();

        let output = dir.join("output");
        let write_actors = |skip: FileFlags| {
            write(
                &source,
                &translation,
                &output,
                EngineType::MVMZ,
                DuplicateMode::Allow,
                BaseFlags::empty(),
                skip,
                None,
                None,
            )
        };

        let elapsed = write_actors(all_but_actors()).unwrap();
        assert!(elapsed >= 0.0);

        let written = fs::read_to_string(output.join("data/Actors.json")).unwrap();
        assert!(written.contains("Гарольд") && written.contains("Храбрый"));
        // What is not translated stays as it was
        assert!(written.contains("A hero"));
        assert!(!written.contains("Harold"));
        // The game itself is not touched
        assert!(fs::read_to_string(source.join("Actors.json")).unwrap().contains("Harold"));

        // The actors are among the files to leave out: nothing is written for them
        fs::remove_dir_all(&output).unwrap();
        write_actors(FileFlags::all()).unwrap();
        assert!(!output.join("data/Actors.json").exists());

        let _ = fs::remove_dir_all(dir);
    }

    #[test]
    fn purging_drops_the_entries_never_filled_in() {
        let (dir, source, translation) = mv_game("purge");
        read_actors(&source, &translation, DEFAULT, HashMap::default()).unwrap();
        fs::write(
            translation.join("actors.txt"),
            "<!>ID<#>1\n<!>NAME<#>Harold\nHarold<#>Гарольд\nThe Brave<#>",
        )
        .unwrap();

        purge(
            &source,
            &translation,
            EngineType::MVMZ,
            DuplicateMode::Allow,
            BaseFlags::empty(),
            all_but_actors(),
            None,
            None,
        )
        .unwrap();

        let text = fs::read_to_string(translation.join("actors.txt")).unwrap();
        assert!(text.contains("Harold<#>Гарольд"), "{text}");
        assert!(!text.contains("The Brave"), "{text}");

        let _ = fs::remove_dir_all(dir);
    }
}
