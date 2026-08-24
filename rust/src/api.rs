#![allow(clippy::too_many_arguments)]
use gxhash::{HashMap, HashMapExt};
use language_tokenizer::{Algorithm, MatchMode, MatchResult, find_all_matches as find_all_matches_, tokenize};
#[cfg(feature = "languagetool")]
use languagetool_rust::api::{
    check::{Data, Level, Match, Request as LTRequest},
    server::ServerClient,
};
#[cfg(feature = "llm-connector")]
use llm_connector::{
    LlmClient, LlmConnectorError,
    types::{ChatRequest, Message, ReasoningEffort, Role},
};
use log::{debug, info};
use marshal_rs::Get;
use rpgmad_lib::{Decrypter, ExtractError};
use rvpacker_txt_rs_lib::{
    BaseFlags, DuplicateMode, EngineType, FileFlags, Mode, Processor, constants::DEFAULT_LINE_BREAK,
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, from_str, to_string};
#[cfg(feature = "languagetool")]
use std::time::Duration;
use std::{
    fs::{self, create_dir_all},
    io::{self},
    path::{Path, PathBuf},
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

/// Builds the `LlmClient` for any endpoint reachable through `llm-connector`.
/// Never call with `Google`/`Yandex`/`DeepL` - those aren't `llm-connector` providers.
#[cfg(feature = "llm-connector")]
fn build_llm_client(
    endpoint: TranslationEndpoint,
    api_key: &str,
    base_url: &str,
) -> Result<LlmClient, LlmConnectorError> {
    Ok(match endpoint {
        TranslationEndpoint::Google | TranslationEndpoint::Yandex | TranslationEndpoint::DeepL => unreachable!(),
        TranslationEndpoint::OpenAI
        | TranslationEndpoint::Longcat
        | TranslationEndpoint::Moonshot
        | TranslationEndpoint::Mistral
        | TranslationEndpoint::DeepSeek
        | TranslationEndpoint::Koboldcpp
        | TranslationEndpoint::OpenAICompatible
        | TranslationEndpoint::Xiaomi => {
            let provider_hint = match endpoint {
                TranslationEndpoint::OpenAI => "openai",
                TranslationEndpoint::Longcat => "longcat",
                TranslationEndpoint::DeepSeek => "deepseek",
                TranslationEndpoint::Moonshot => "moonshot",
                TranslationEndpoint::Mistral => "mistral",
                TranslationEndpoint::Koboldcpp => "koboldcpp",
                TranslationEndpoint::OpenAICompatible => "",
                TranslationEndpoint::Xiaomi => "xiaomi",
                _ => unreachable!(),
            };

            LlmClient::openai_compatible(api_key, base_url, provider_hint)?
        }
        TranslationEndpoint::Anthropic => LlmClient::anthropic(api_key, base_url)?,
        TranslationEndpoint::Gemini => LlmClient::google(api_key, base_url)?,
        TranslationEndpoint::Ollama => LlmClient::ollama(base_url)?,
        TranslationEndpoint::Aliyun => LlmClient::aliyun(api_key, base_url)?,
        TranslationEndpoint::Volcengine => LlmClient::volcengine(api_key, base_url)?,
        TranslationEndpoint::Xinference => LlmClient::xinference(base_url)?,
        TranslationEndpoint::Zhipu => LlmClient::zhipu(api_key, base_url)?,
    })
}

#[derive(Clone, Copy)]
#[repr(C)]
pub struct MatchModeInfo {
    pub mode: MatchMode,
    pub case_sensitive: bool,
    pub permissive: bool,
}

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
    #[cfg(feature = "google-translate")]
    #[error(transparent)]
    Translators(#[from] translators::Error),
    #[cfg(feature = "llm-connector")]
    #[error(transparent)]
    LLMConnectorError(#[from] LlmConnectorError),
    #[cfg(feature = "yandex-translate")]
    #[error(transparent)]
    Yandex(#[from] yandex_translate_v2::Error),
    #[cfg(feature = "yandex-translate")]
    #[error("Yandex folder ID is not specified. Input it in settings.")]
    YandexFolderNotSpecified,
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
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TranslationEndpoint {
    Google,
    Yandex,
    DeepL,

    Aliyun,
    Anthropic,
    DeepSeek,
    Gemini,
    Koboldcpp,
    Longcat,
    Moonshot,
    Mistral,
    Ollama,
    OpenAI,
    OpenAICompatible,
    Volcengine,
    Xiaomi,
    Xinference,
    Zhipu,
}

impl Default for TranslationEndpoint {
    fn default() -> Self {
        TranslationEndpoint::Google
    }
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub(crate) struct GlossaryEntry<'a> {
    term: &'a str,
    translation: &'a str,
    note: &'a str,
}

#[derive(Clone, Debug, Serialize)]
pub(crate) struct Request<'a> {
    source_language: &'a str,
    translation_language: &'a str,
    project_context: &'a str,
    local_context: &'a str,
    glossary: &'a [GlossaryEntry<'a>],
    files: &'a HashMap<&'a str, Vec<String>>,
}

#[derive(Clone, Debug, Serialize)]
pub(crate) struct SingleRequest<'a> {
    source_language: &'a str,
    translation_language: &'a str,
    project_context: &'a str,
    local_context: &'a str,
    glossary: &'a [GlossaryEntry<'a>],
    text: &'a str,
}

pub(crate) fn write(
    source_path: &Path,
    translation_path: &Path,
    output_path: &Path,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    flags: BaseFlags,
    skip_files: FileFlags,
) -> Result<f32, Error> {
    let start_time = Instant::now();

    let mut processor = Processor {
        mode: Mode::Write,
        file_flags: FileFlags::all() & !skip_files,
        flags,
        duplicate_mode,
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
) -> Result<HashMap<String, u64>, Error> {
    let mut processor = Processor {
        mode: read_mode,
        file_flags: FileFlags::all().difference(skip_files),
        flags,
        duplicate_mode,
        game_title: ini_title.to_owned(),
        hashes,
        map_events,
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
) -> Result<(), Error> {
    let mut processor = Processor {
        mode: Mode::Purge,
        file_flags: FileFlags::all() & !skip_files,
        flags,
        duplicate_mode,
        ..Default::default()
    };

    processor.process(engine_type, source_path, translation_path, None)?;

    Ok(())
}

pub(crate) async fn get_models(
    endpoint: TranslationEndpoint,
    api_key: &str,
    base_url: &str,
) -> Result<Vec<String>, Error> {
    #[cfg(feature = "llm-connector")]
    {
        Ok(build_llm_client(endpoint, api_key, base_url)?.models().await?)
    }

    #[cfg(not(feature = "llm-connector"))]
    {
        let _ = (endpoint, api_key, base_url);
        Err(Error::BackendNotCompiled("LLM connector"))
    }
}

pub(crate) async fn translate_single<'a>(
    endpoint_settings: &str,
    source_language: &str,
    translation_language: &str,
    project_context: &str,
    local_context: &str,
    text: &str,
    glossary: Vec<GlossaryEntry<'a>>,
) -> Result<String, Error> {
    let source_language = source_language;
    let translation_language = translation_language;

    let endpoint_settings: Value = unsafe { serde_json::from_str(endpoint_settings).unwrap_unchecked() };
    let endpoint = unsafe { std::mem::transmute(endpoint_settings["type"].as_u64().unwrap_unchecked() as u8) };

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

        _ => {
            let api_key = unsafe { endpoint_settings["apiKey"].as_str().unwrap_unchecked() };

            match endpoint {
                #[cfg(feature = "yandex-translate")]
                TranslationEndpoint::Yandex => {
                    use yandex_translate_v2::{TranslateRequest, YandexTranslateClient};

                    let yandex_folder_id = unsafe { endpoint_settings["folderId"].as_str().unwrap_unchecked() };

                    let client = YandexTranslateClient::with_api_key(api_key)?;
                    let response = client
                        .translate(&TranslateRequest {
                            folder_id: yandex_folder_id,
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
                        .unwrap_or_default()
                }
                #[cfg(not(feature = "yandex-translate"))]
                TranslationEndpoint::Yandex => return Err(Error::BackendNotCompiled("Yandex Translate")),

                #[cfg(feature = "deepl")]
                TranslationEndpoint::DeepL => {
                    use deepl::*;

                    let client = DeepLApi::with(api_key).new();
                    client
                        .translate_text(text, Lang::from_str(translation_language)?)
                        .await?
                        .to_string()
                }
                #[cfg(not(feature = "deepl"))]
                TranslationEndpoint::DeepL => return Err(Error::BackendNotCompiled("DeepL")),

                #[cfg(feature = "llm-connector")]
                _ => {
                    let base_url = unsafe { endpoint_settings["baseUrl"].as_str().unwrap_unchecked() };
                    let client = build_llm_client(endpoint, api_key, base_url)?;

                    let prompt = SingleRequest {
                        source_language,
                        translation_language,
                        project_context,
                        local_context,
                        glossary: &glossary,
                        text,
                    };

                    let model = unsafe { endpoint_settings["model"].as_str().unwrap_unchecked() };
                    let thinking = unsafe { endpoint_settings["thinking"].as_bool().unwrap_unchecked() };
                    let temperature = unsafe { endpoint_settings["temperature"].as_f64().unwrap_unchecked() };

                    let request = ChatRequest {
                        model: model.to_string(),
                        messages: vec![
                            Message::text(Role::System, unsafe {
                                endpoint_settings["singleTranslationSystemPrompt"]
                                    .as_str()
                                    .unwrap_unchecked()
                            }),
                            Message::text(Role::User, to_string(&prompt)?),
                        ],
                        enable_thinking: Some(thinking),
                        temperature: Some(temperature as f32),
                        ..Default::default()
                    };

                    let response = client.chat(&request).await?;
                    response.content
                }
                #[cfg(not(feature = "llm-connector"))]
                _ => return Err(Error::BackendNotCompiled("LLM connector")),
            }
        }
    };

    Ok(translated)
}

pub(crate) async fn translate<'a>(
    endpoint_settings: &str,
    source_language: &str,
    translation_language: &str,
    project_context: &str,
    local_context: &str,
    files: HashMap<&str, Vec<String>>,
    glossary: Vec<GlossaryEntry<'a>>,
    is_interrupted: extern "C" fn(*mut std::ffi::c_void) -> bool,
    is_interrupted_ctx: *mut std::ffi::c_void,
) -> Result<HashMap<String, Vec<String>>, Error> {
    let mut response: HashMap<String, Vec<String>> = HashMap::with_capacity(files.len());

    let endpoint_settings: Value = unsafe { serde_json::from_str(endpoint_settings).unwrap_unchecked() };
    let endpoint = unsafe { std::mem::transmute(endpoint_settings["type"].as_u64().unwrap_unchecked() as u8) };

    let result: HashMap<String, Vec<String>> = match endpoint {
        #[cfg(feature = "google-translate")]
        TranslationEndpoint::Google => {
            use translators::{GoogleTranslator, Translator};
            let translator = GoogleTranslator::default();

            'files: for (&file, strings) in &files {
                if is_interrupted(is_interrupted_ctx) {
                    break;
                }

                response.insert(file.to_string(), Vec::with_capacity(strings.len()));
                let response_file = response.get_mut(file).unwrap();

                for string in strings {
                    if is_interrupted(is_interrupted_ctx) {
                        break 'files;
                    }

                    let translated = translator
                        .translate_async(string, source_language, translation_language)
                        .await?;
                    response_file.push(translated.replace('\n', DEFAULT_LINE_BREAK));
                }
            }

            response
        }
        #[cfg(not(feature = "google-translate"))]
        TranslationEndpoint::Google => return Err(Error::BackendNotCompiled("Google Translate")),

        _ => {
            let api_key = unsafe { endpoint_settings["apiKey"].as_str().unwrap_unchecked() };

            match endpoint {
                #[cfg(feature = "yandex-translate")]
                TranslationEndpoint::Yandex => {
                    use yandex_translate_v2::{TranslateRequest, YandexTranslateClient};

                    let yandex_folder_id = unsafe { endpoint_settings["folderId"].as_str().unwrap_unchecked() };

                    if yandex_folder_id.is_empty() {
                        return Err(Error::YandexFolderNotSpecified);
                    }

                    for (&file, strings) in &files {
                        if is_interrupted(is_interrupted_ctx) {
                            break;
                        }

                        response.insert(file.to_string(), Vec::with_capacity(strings.len()));
                        let response_file = response.get_mut(file).unwrap();

                        let client = YandexTranslateClient::with_api_key(api_key)?;
                        let response = client
                            .translate(&TranslateRequest {
                                folder_id: yandex_folder_id,
                                texts: &strings.iter().map(|s| s.as_str()).collect::<Vec<_>>(),
                                target_language_code: translation_language,
                                source_language_code: Some(source_language),
                            })
                            .await?;
                        let strings = response
                            .translations
                            .into_iter()
                            .map(|x| x.text.replace('\n', DEFAULT_LINE_BREAK))
                            .collect();

                        *response_file = strings;
                    }

                    response
                }
                #[cfg(not(feature = "yandex-translate"))]
                TranslationEndpoint::Yandex => return Err(Error::BackendNotCompiled("Yandex Translate")),

                #[cfg(feature = "deepl")]
                TranslationEndpoint::DeepL => {
                    use deepl::{glossary::*, *};

                    let client = DeepLApi::with(api_key).new();
                    let _ = client
                        .create_glossary("glossary")
                        .entries(glossary.iter().map(|e| (e.term, e.translation)))
                        .source_lang(GlossaryLanguage::from_str(source_language)?)
                        .target_lang(GlossaryLanguage::from_str(translation_language)?)
                        .send()
                        .await?;

                    'files: for (&file, strings) in &files {
                        if is_interrupted(is_interrupted_ctx) {
                            break;
                        }

                        response.insert(file.to_string(), Vec::with_capacity(strings.len()));
                        let response_file = response.get_mut(file).unwrap();

                        let mut new_strings = Vec::with_capacity(files.len());

                        for string in strings {
                            if is_interrupted(is_interrupted_ctx) {
                                *response_file = new_strings;
                                break 'files;
                            }

                            let translated = client
                                .translate_text(string.as_str(), Lang::from_str(translation_language)?)
                                .await?
                                .to_string();

                            new_strings.push(translated.replace('\n', DEFAULT_LINE_BREAK));
                        }

                        *response_file = new_strings;
                    }

                    response
                }
                #[cfg(not(feature = "deepl"))]
                TranslationEndpoint::DeepL => return Err(Error::BackendNotCompiled("DeepL")),

                #[cfg(feature = "llm-connector")]
                _ => {
                    let base_url = unsafe { endpoint_settings["baseUrl"].as_str().unwrap_unchecked() };
                    let client = build_llm_client(endpoint, api_key, base_url)?;

                    let token_limit = unsafe { endpoint_settings["tokenLimit"].as_u64().unwrap_unchecked() } as u32;
                    let output_token_limit =
                        unsafe { endpoint_settings["tokenLimit"].as_u64().unwrap_unchecked() } as u32;
                    let reasoning_effort =
                        unsafe { endpoint_settings["reasoningEffort"].as_u64().unwrap_unchecked() } as u8;
                    let reasoning_effort: ReasoningEffort = unsafe { std::mem::transmute(reasoning_effort) };
                    let model = unsafe { endpoint_settings["model"].as_str().unwrap_unchecked() };
                    let system_prompt = unsafe { endpoint_settings["systemPrompt"].as_str().unwrap_unchecked() };
                    let thinking = unsafe { endpoint_settings["thinking"].as_bool().unwrap_unchecked() };
                    let temperature = endpoint_settings["temperature"].as_f64().map(|t| t as f32);

                    let mut limited_files: Vec<HashMap<&str, Vec<String>>> = vec![HashMap::default()];
                    let mut entry: &mut HashMap<&str, Vec<String>> = limited_files.first_mut().unwrap();
                    let mut limit = 0;

                    for (file, strings) in files {
                        for string in &strings {
                            let tokens = string.len() / 4;
                            limit += tokens;
                        }

                        if limit < token_limit as usize {
                            entry.insert(file, strings);
                        } else {
                            limit = 0;
                            limited_files.push(HashMap::default());
                            entry = limited_files.last_mut().unwrap();
                        }
                    }

                    let mut result: HashMap<String, Vec<String>> = HashMap::default();

                    let mut request = ChatRequest {
                        model: model.to_string(),
                        messages: vec![Message::text(Role::System, system_prompt), Message::default()],
                        enable_thinking: Some(thinking),
                        temperature: temperature,
                        max_tokens: Some(output_token_limit),
                        reasoning_effort: Some(reasoning_effort),
                        ..Default::default()
                    };

                    for files in limited_files {
                        if is_interrupted(is_interrupted_ctx) {
                            break;
                        }

                        let prompt = Request {
                            source_language,
                            translation_language,
                            project_context,
                            local_context,
                            glossary: &glossary,
                            files: &files,
                        };

                        request.messages[1] = Message::text(Role::User, to_string(&prompt)?);

                        let response = client.chat(&request).await?;

                        let response = from_str::<HashMap<String, Vec<String>>>(&response.content)?;

                        result.extend(response);
                    }

                    for strings in result.values_mut() {
                        for string in strings.iter_mut() {
                            *string = string.replace('\n', DEFAULT_LINE_BREAK);
                        }
                    }

                    result
                }
                #[cfg(not(feature = "llm-connector"))]
                _ => return Err(Error::BackendNotCompiled("LLM connector")),
            }
        }
    };

    Ok(result)
}

fn fix_path_separators(raw: &mut [u8]) {
    let mut i = 0;

    while i < raw.len() {
        let b = raw[i];

        if b >= 0x80 {
            i += 1;

            if i < raw.len() {
                i += 1;
            }
        } else if b == b'\\' {
            raw[i] = b'/';
            i += 1;
        } else {
            i += 1;
        }
    }
}

pub(crate) fn extract_archive(input_path: &Path, output_path: &Path) -> Result<(), Error> {
    debug!("Decrypting archive: {}", input_path.display());

    let mut bytes = fs::read(input_path).map_err(|err| Error::Io(input_path.to_path_buf(), err))?;

    let mut decrypter = Decrypter::new();
    let decrypted_entries = decrypter.decrypt(&mut bytes)?;

    for file in decrypted_entries {
        fix_path_separators(unsafe { std::slice::from_raw_parts_mut(file.path.as_ptr().cast_mut(), file.path.len()) });

        let path = String::from_utf8_lossy(&file.path);
        debug!("Decrypting archive entry: {}", path);

        let output_file_path = output_path.join(path.as_ref());

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

pub(crate) fn count_words(text: &str, algorithm: Algorithm) -> Result<u32, Error> {
    let tokens = tokenize(text, algorithm, false)?;
    Ok(tokens.len() as u32)
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
    let extension = unsafe { path.extension().unwrap_unchecked() };

    let file_type = if extension == MV_M4A_EXT || extension == MZ_M4A_EXT {
        FileType::M4A
    } else if extension == MV_OGG_EXT || extension == MZ_OGG_EXT {
        FileType::OGG
    } else {
        FileType::PNG
    };

    decrypt_in_place(&mut file_content, file_type)?;

    return Ok(file_content);
}

pub(crate) fn generate_json(content: &[u8], filename: &str) -> Result<String, Error> {
    let json = rvpacker_txt_rs_lib::json::generate_file(content, filename)?;
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

pub(crate) fn get_ini_title(project_path: &str) -> Result<Vec<u8>, Error> {
    let ini_path = Path::new(project_path).join("Game.ini");

    Ok(rvpacker_txt_rs_lib::get_ini_title(
        &fs::read(&ini_path).map_err(|err| Error::Io(ini_path, err))?,
    )?)
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

            Ok(unsafe { result.unwrap_unchecked() })
        }
        #[cfg(not(feature = "js-formatting"))]
        HighlightLanguage::JS => return Err(Error::BackendNotCompiled("JavaScript formatting")),

        #[cfg(feature = "ruby-formatting")]
        HighlightLanguage::Ruby => {
            use rubyfmt::format_buffer;
            let result = unsafe {
                String::from_utf8_unchecked(
                    format_buffer(src.as_bytes()).map_err(|err| Error::Rubyfmt(err.as_exit_code()))?,
                )
            };
            Ok(result)
        }
        #[cfg(not(feature = "ruby-formatting"))]
        HighlightLanguage::Ruby => return Err(Error::BackendNotCompiled("Ruby formatting")),

        _ => unreachable!(),
    }
}
