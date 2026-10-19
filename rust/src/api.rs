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

pub(crate) fn store_credential(account: &str, secret: &str) -> Result<(), Error> {
    Ok(keyring::Entry::new(KEYRING_SERVICE, account)?.set_password(secret)?)
}

pub(crate) fn get_credential(account: &str) -> Result<String, Error> {
    Ok(keyring::Entry::new(KEYRING_SERVICE, account)?.get_password()?)
}

pub(crate) fn delete_credential(account: &str) -> Result<(), Error> {
    Ok(keyring::Entry::new(KEYRING_SERVICE, account)?.delete_credential()?)
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

            let client = YandexTranslateClient::with_api_key(endpoint_settings.api_key)?;
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

pub(crate) fn serde_import(bytes: &[u8], format: SerdeFormat) -> Result<String, Error> {
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

            let result = format_text(FormatTextOptions {
                path: Path::new(""),
                extension: None,
                text: src.into(),
                config: &config,
                external_formatter: None,
            })?;

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
