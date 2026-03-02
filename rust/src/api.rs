#![allow(clippy::too_many_arguments)]
use language_tokenizer::{
    Algorithm, MatchMode, MatchResult, find_all_matches as find_all_matches_,
    tokenize,
};
use languagetool_rust::api::{
    check::Request as LTRequest, server::ServerClient,
};
use llm_connector::{
    LlmClient, LlmConnectorError,
    types::{ChatRequest, Message, Role},
};
use log::{debug, info};
use regex::Regex;
use rpgmad_lib::{Decrypter, ExtractError};
use rvpacker_txt_rs_lib::{
    BaseFlags, DuplicateMode, EngineType, FileFlags, GameType, PurgerBuilder,
    ReadMode, ReaderBuilder, WriterBuilder, constants::NEW_LINE, get_ini_title,
    get_system_title,
};
use serde::{Deserialize, Serialize};
use serde_json::{Value, from_str, to_string};
use std::{
    collections::HashMap,
    fs::{self, create_dir_all, read_to_string},
    io::{self},
    path::{Path, PathBuf},
    str::FromStr,
    sync::{Arc, LazyLock, Mutex},
    time::Instant,
};
use thiserror::Error;
use tiktoken_rs::o200k_base;
use whatlang::detect;

pub fn to_bcp47(algorithm: Algorithm) -> &'static str {
    match algorithm {
        Algorithm::None => "",

        Algorithm::Arabic => "ar",
        Algorithm::Armenian => "hy",
        Algorithm::Basque => "eu",
        Algorithm::Catalan => "ca",
        Algorithm::Danish => "da",
        Algorithm::Dutch | Algorithm::DutchPorter => "nl",
        Algorithm::English | Algorithm::Lovins | Algorithm::Porter => "en",
        Algorithm::Esperanto => "eo",
        Algorithm::Estonian => "et",
        Algorithm::Finnish => "fi",
        Algorithm::French => "fr",
        Algorithm::German => "de",
        Algorithm::Greek => "el",
        Algorithm::Hindi => "hi",
        Algorithm::Hungarian => "hu",
        Algorithm::Indonesian => "id",
        Algorithm::Irish => "ga",
        Algorithm::Italian => "it",
        Algorithm::Lithuanian => "lt",
        Algorithm::Nepali => "ne",
        Algorithm::Norwegian => "nb",
        Algorithm::Portuguese => "pt",
        Algorithm::Romanian => "ro",
        Algorithm::Russian => "ru",
        Algorithm::Serbian => "sr",
        Algorithm::Spanish => "es",
        Algorithm::Swedish => "sv",
        Algorithm::Tamil => "ta",
        Algorithm::Turkish => "tr",
        Algorithm::Yiddish => "yi",

        Algorithm::Japanese => "ja",
        Algorithm::Chinese => "zh",
        Algorithm::Korean => "ko",

        Algorithm::Thai => "th",
        Algorithm::Burmese => "my",
        Algorithm::Lao => "lo",
        Algorithm::Khmer => "km",
    }
}

static CHECKED_LANGS: LazyLock<Arc<Mutex<HashMap<&'static str, f64>>>> =
    LazyLock::new(|| Arc::new(Mutex::new(HashMap::new())));

fn get_game_type(
    game_title: &str,
    disable_custom_processing: bool,
) -> GameType {
    if disable_custom_processing {
        GameType::None
    } else {
        let lowercased = game_title.to_lowercase();

        if unsafe { Regex::new(r"\btermina\b").unwrap_unchecked() }
            .is_match(&lowercased)
        {
            GameType::Termina
        } else if unsafe { Regex::new(r"\blisa\b").unwrap_unchecked() }
            .is_match(&lowercased)
        {
            GameType::LisaRPG
        } else {
            GameType::None
        }
    }
}

#[derive(Clone, Copy)]
pub(crate) struct MatchModeInfo {
    pub(crate) mode: MatchMode,
    pub(crate) case_sensitive: bool,
    pub(crate) permissive: bool,
}

#[derive(Debug, Error)]
pub(crate) enum Error {
    #[error("{0}: IO error occurred: {1}")]
    Io(PathBuf, io::Error),
    #[error(transparent)]
    Rvpacker(#[from] rvpacker_txt_rs_lib::Error),
    #[error(transparent)]
    Extract(#[from] ExtractError),
    #[error(transparent)]
    Translators(#[from] translators::Error),
    #[error(transparent)]
    LLMConnectorError(#[from] LlmConnectorError),
    #[error(transparent)]
    Yandex(#[from] yandex_translate_v2::Error),
    #[error("Yandex folder ID is not specified. Input it in settings.")]
    YandexFolderNotSpecified,
    #[error(transparent)]
    JSON(#[from] serde_json::Error),
    #[error(transparent)]
    DeepL(#[from] deepl::Error),
    #[error(transparent)]
    DeepLLangConvert(#[from] deepl::LangConvertError),
    #[error(transparent)]
    LanguageTokenizer(#[from] language_tokenizer::Error),
    #[error(transparent)]
    LanguageTool(#[from] languagetool_rust::error::Error),
}

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TranslationEndpoint {
    Google,
    Yandex,
    DeepL,
    OpenAI,
    Anthropic,
    DeepSeek,
    Gemini,
    OpenAICompatible,
    Ollama,
}

impl Default for TranslationEndpoint {
    fn default() -> Self {
        TranslationEndpoint::Google
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub(crate) struct GlossaryEntry<'a> {
    term: &'a str,
    translation: &'a str,
    note: &'a str,
}

#[derive(Debug, Clone, Serialize)]
pub(crate) struct Request<'a> {
    source_language: &'a str,
    translation_language: &'a str,
    project_context: &'a str,
    local_context: &'a str,
    glossary: &'a [GlossaryEntry<'a>],
    files: &'a HashMap<&'a str, Vec<String>>,
}

#[derive(Debug, Clone, Serialize)]
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
    game_title: &str,
    flags: BaseFlags,
    skip_files: FileFlags,
) -> Result<f32, Error> {
    let start_time = Instant::now();
    let game_type = get_game_type(
        game_title,
        flags.contains(BaseFlags::DisableCustomProcessing),
    );

    let mut writer = WriterBuilder::new()
        .with_files(FileFlags::all() & !skip_files)
        .with_flags(flags)
        .game_type(game_type)
        .duplicate_mode(duplicate_mode)
        .build();

    writer.write(source_path, translation_path, output_path, engine_type)?;

    Ok(start_time.elapsed().as_secs_f32())
}

pub(crate) fn read(
    project_path: &Path,
    source_path: &Path,
    translation_path: &Path,
    read_mode: ReadMode,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    skip_files: FileFlags,
    flags: BaseFlags,
    map_events: bool,
    hashes: Vec<u128>,
) -> Result<Vec<u128>, Error> {
    let game_title: String = if engine_type.is_new() {
        let system_file_path = source_path.join("System.json");
        let system_file_content = read_to_string(&system_file_path)
            .map_err(|err| Error::Io(system_file_path, err))?;
        get_system_title(&system_file_content)?
    } else {
        let ini_file_path = Path::new(project_path).join("Game.ini");
        let ini_file_content = fs::read(&ini_file_path)
            .map_err(|err| Error::Io(ini_file_path, err))?;
        let title = get_ini_title(&ini_file_content)?;
        String::from_utf8_lossy(&title).into_owned()
    };

    let game_type = get_game_type(
        &game_title,
        flags.contains(BaseFlags::DisableCustomProcessing),
    );

    let mut reader = ReaderBuilder::new()
        .with_files(FileFlags::all().difference(skip_files))
        .with_flags(flags)
        .game_type(game_type)
        .read_mode(read_mode)
        .duplicate_mode(duplicate_mode)
        .map_events(map_events)
        .hashes(hashes)
        .build();

    reader.read(source_path, translation_path, engine_type)?;

    Ok(reader.hashes())
}

pub(crate) fn purge(
    source_path: &Path,
    translation_path: &Path,
    engine_type: EngineType,
    duplicate_mode: DuplicateMode,
    game_title: &str,
    flags: BaseFlags,
    skip_files: FileFlags,
) -> Result<(), Error> {
    let game_type = get_game_type(
        game_title,
        flags.contains(BaseFlags::DisableCustomProcessing),
    );

    PurgerBuilder::new()
        .with_files(FileFlags::all() & !skip_files)
        .with_flags(flags)
        .game_type(game_type)
        .duplicate_mode(duplicate_mode)
        .build()
        .purge(source_path, translation_path, engine_type)?;

    Ok(())
}

pub(crate) async fn get_models(
    endpoint: TranslationEndpoint,
    api_key: &str,
    base_url: &str,
) -> Result<Vec<String>, Error> {
    Ok(match endpoint {
        TranslationEndpoint::Google
        | TranslationEndpoint::Yandex
        | TranslationEndpoint::DeepL => Vec::new(),
        TranslationEndpoint::DeepSeek => {
            LlmClient::deepseek(api_key)?.models().await?
        }
        TranslationEndpoint::OpenAI => {
            LlmClient::openai(api_key)?.models().await?
        }
        TranslationEndpoint::Anthropic => {
            LlmClient::openai(api_key)?.models().await?
        }
        TranslationEndpoint::Gemini => {
            LlmClient::google(api_key)?.models().await?
        }
        TranslationEndpoint::OpenAICompatible => {
            LlmClient::openai_with_base_url(api_key, base_url)?
                .models()
                .await?
        }
        TranslationEndpoint::Ollama => {
            LlmClient::ollama_with_base_url(base_url)?.models().await?
        }
    })
}

pub(crate) async fn translate_single<'a>(
    endpoint: TranslationEndpoint,
    endpoint_settings: &str,
    source_language: Algorithm,
    translation_language: Algorithm,
    project_context: &str,
    local_context: &str,
    text: &str,
    glossary: Vec<GlossaryEntry<'a>>,
) -> Result<String, Error> {
    let source_language = to_bcp47(source_language);
    let translation_language = to_bcp47(translation_language);

    let translated = match endpoint {
        TranslationEndpoint::Google => {
            use translators::{GoogleTranslator, Translator};

            GoogleTranslator::default()
                .translate_async(text, source_language, translation_language)
                .await?
        }

        _ => {
            let endpoint_settings: Value = unsafe {
                serde_json::from_str(endpoint_settings).unwrap_unchecked()
            };

            let api_key = unsafe {
                endpoint_settings["apiKey"].as_str().unwrap_unchecked()
            };

            match endpoint {
                TranslationEndpoint::Yandex => {
                    use yandex_translate_v2::{
                        TranslateRequest, YandexTranslateClient,
                    };

                    let yandex_folder_id = unsafe {
                        endpoint_settings["folderId"]
                            .as_str()
                            .unwrap_unchecked()
                    };

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

                TranslationEndpoint::DeepL => {
                    use deepl::*;

                    let client = DeepLApi::with(api_key).new();
                    client
                        .translate_text(
                            text,
                            Lang::from_str(translation_language)?,
                        )
                        .await?
                        .to_string()
                }

                _ => {
                    let client = match endpoint {
                        TranslationEndpoint::OpenAI => {
                            LlmClient::openai(api_key)?
                        }
                        TranslationEndpoint::Anthropic => {
                            LlmClient::anthropic(api_key)?
                        }
                        TranslationEndpoint::DeepSeek => {
                            LlmClient::deepseek(api_key)?
                        }
                        TranslationEndpoint::Gemini => {
                            LlmClient::google(api_key)?
                        }
                        TranslationEndpoint::OpenAICompatible => {
                            let base_url = unsafe {
                                endpoint_settings["baseUrl"]
                                    .as_str()
                                    .unwrap_unchecked()
                            };
                            LlmClient::openai_with_base_url(api_key, base_url)?
                        }
                        TranslationEndpoint::Ollama => {
                            let base_url = unsafe {
                                endpoint_settings["baseUrl"]
                                    .as_str()
                                    .unwrap_unchecked()
                            };
                            LlmClient::ollama_with_base_url(base_url)?
                        }
                        _ => unreachable!(),
                    };

                    let prompt = SingleRequest {
                        source_language,
                        translation_language,
                        project_context,
                        local_context,
                        glossary: &glossary,
                        text,
                    };

                    let model = unsafe {
                        endpoint_settings["model"].as_str().unwrap_unchecked()
                    };
                    let thinking = unsafe {
                        endpoint_settings["thinking"]
                            .as_bool()
                            .unwrap_unchecked()
                    };
                    let temperature = unsafe {
                        endpoint_settings["temperature"]
                            .as_f64()
                            .unwrap_unchecked()
                    };

                    let request = ChatRequest {
                        model: model.to_string(),
                        messages: vec![
                            Message::text(Role::System, unsafe {
                                endpoint_settings["singleTranslationSystemPrompt"].as_str().unwrap_unchecked()
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
            }
        }
    };

    Ok(translated)
}

pub(crate) async fn translate<'a>(
    endpoint: TranslationEndpoint,
    endpoint_settings: &str,
    source_language: Algorithm,
    translation_language: Algorithm,
    project_context: &str,
    local_context: &str,
    files: HashMap<&str, Vec<String>>,
    glossary: Vec<GlossaryEntry<'a>>,
) -> Result<HashMap<String, Vec<String>>, Error> {
    let source_language = to_bcp47(source_language);
    let translation_language = to_bcp47(translation_language);

    let mut response: HashMap<String, Vec<String>> =
        HashMap::with_capacity(files.len());

    let result: HashMap<String, Vec<String>> = match endpoint {
        TranslationEndpoint::Google => {
            use translators::{GoogleTranslator, Translator};
            let translator = GoogleTranslator::default();

            for (&file, strings) in &files {
                response.insert(
                    file.to_string(),
                    Vec::with_capacity(strings.len()),
                );
                let response_file = response.get_mut(file).unwrap();

                for string in strings {
                    let translated = translator
                        .translate_async(
                            string,
                            source_language,
                            translation_language,
                        )
                        .await?;
                    response_file.push(translated.replace('\n', NEW_LINE));
                }
            }

            response
        }

        _ => {
            let endpoint_settings: Value = unsafe {
                serde_json::from_str(endpoint_settings).unwrap_unchecked()
            };

            let api_key = unsafe {
                endpoint_settings["apiKey"].as_str().unwrap_unchecked()
            };

            match endpoint {
                TranslationEndpoint::Yandex => {
                    use yandex_translate_v2::{
                        TranslateRequest, YandexTranslateClient,
                    };

                    let yandex_folder_id = unsafe {
                        endpoint_settings["folderId"]
                            .as_str()
                            .unwrap_unchecked()
                    };

                    if yandex_folder_id.is_empty() {
                        return Err(Error::YandexFolderNotSpecified);
                    }

                    for (&file, strings) in &files {
                        response.insert(
                            file.to_string(),
                            Vec::with_capacity(strings.len()),
                        );
                        let response_file = response.get_mut(file).unwrap();

                        let client =
                            YandexTranslateClient::with_api_key(api_key)?;
                        let response = client
                            .translate(&TranslateRequest {
                                folder_id: yandex_folder_id,
                                texts: &strings
                                    .iter()
                                    .map(|s| s.as_str())
                                    .collect::<Vec<_>>(),
                                target_language_code: translation_language,
                                source_language_code: Some(source_language),
                            })
                            .await?;
                        let strings = response
                            .translations
                            .into_iter()
                            .map(|x| x.text.replace('\n', NEW_LINE))
                            .collect();

                        *response_file = strings;
                    }

                    response
                }

                TranslationEndpoint::DeepL => {
                    use deepl::{glossary::*, *};

                    let client = DeepLApi::with(api_key).new();
                    let _ = client
                        .create_glossary("glossary")
                        .entries(
                            glossary.iter().map(|e| (e.term, e.translation)),
                        )
                        .source_lang(GlossaryLanguage::from_str(
                            source_language,
                        )?)
                        .target_lang(GlossaryLanguage::from_str(
                            translation_language,
                        )?)
                        .send()
                        .await?;

                    for (&file, strings) in &files {
                        response.insert(
                            file.to_string(),
                            Vec::with_capacity(strings.len()),
                        );
                        let response_file = response.get_mut(file).unwrap();

                        let mut new_strings = Vec::with_capacity(files.len());

                        for string in strings {
                            let translated = client
                                .translate_text(
                                    string.as_str(),
                                    Lang::from_str(translation_language)?,
                                )
                                .await?
                                .to_string();

                            new_strings
                                .push(translated.replace('\n', NEW_LINE));
                        }

                        *response_file = new_strings;
                    }

                    response
                }

                _ => {
                    let client = match endpoint {
                        TranslationEndpoint::OpenAI => {
                            LlmClient::openai(api_key)?
                        }
                        TranslationEndpoint::Anthropic => {
                            LlmClient::anthropic(api_key)?
                        }
                        TranslationEndpoint::DeepSeek => {
                            LlmClient::deepseek(api_key)?
                        }
                        TranslationEndpoint::Gemini => {
                            LlmClient::google(api_key)?
                        }
                        TranslationEndpoint::OpenAICompatible => {
                            let base_url = unsafe {
                                endpoint_settings["baseUrl"]
                                    .as_str()
                                    .unwrap_unchecked()
                            };
                            LlmClient::openai_with_base_url(api_key, base_url)?
                        }
                        TranslationEndpoint::Ollama => {
                            let base_url = unsafe {
                                endpoint_settings["baseUrl"]
                                    .as_str()
                                    .unwrap_unchecked()
                            };
                            LlmClient::ollama_with_base_url(base_url)?
                        }
                        _ => unreachable!(),
                    };

                    let token_limit = unsafe {
                        endpoint_settings["tokenLimit"]
                            .as_u64()
                            .unwrap_unchecked()
                    } as u32;
                    let output_token_limit = unsafe {
                        endpoint_settings["tokenLimit"]
                            .as_u64()
                            .unwrap_unchecked()
                    } as u32;
                    let model = unsafe {
                        endpoint_settings["model"].as_str().unwrap_unchecked()
                    };
                    let system_prompt = unsafe {
                        endpoint_settings["systemPrompt"]
                            .as_str()
                            .unwrap_unchecked()
                    };
                    let thinking = unsafe {
                        endpoint_settings["thinking"]
                            .as_bool()
                            .unwrap_unchecked()
                    };
                    let temperature = unsafe {
                        endpoint_settings["temperature"]
                            .as_f64()
                            .unwrap_unchecked()
                    } as f32;

                    let mut limited_files: Vec<HashMap<&str, Vec<String>>> =
                        vec![HashMap::new()];
                    let mut entry: &mut HashMap<&str, Vec<String>> =
                        limited_files.first_mut().unwrap();
                    let mut limit = 0;

                    let tokenizer = o200k_base().unwrap();

                    for (file, strings) in files {
                        for string in &strings {
                            let tokens =
                                tokenizer.encode_with_special_tokens(string);
                            limit += tokens.len();
                        }

                        if limit < token_limit as usize {
                            entry.insert(file, strings);
                        } else {
                            limit = 0;
                            limited_files.push(HashMap::new());
                            entry = limited_files.last_mut().unwrap();
                        }
                    }

                    let mut result: HashMap<String, Vec<String>> =
                        HashMap::new();

                    let mut request = ChatRequest {
                        model: model.to_string(),
                        messages: vec![
                            Message::text(Role::System, system_prompt),
                            Message::default(),
                        ],
                        enable_thinking: Some(thinking),
                        temperature: Some(temperature),
                        max_tokens: Some(output_token_limit),
                        ..Default::default()
                    };

                    for files in limited_files {
                        let prompt = Request {
                            source_language,
                            translation_language,
                            project_context,
                            local_context,
                            glossary: &glossary,
                            files: &files,
                        };

                        request.messages[1] =
                            Message::text(Role::User, to_string(&prompt)?);

                        let response = client.chat(&request).await?;

                        let response = from_str::<HashMap<String, Vec<String>>>(
                            &response.content,
                        )?;

                        result.extend(response);
                    }

                    for strings in result.values_mut() {
                        for string in strings.iter_mut() {
                            *string = string.replace('\n', NEW_LINE);
                        }
                    }

                    result
                }
            }
        }
    };

    Ok(result)
}

pub(crate) fn extract_archive(
    input_path: &Path,
    output_path: &Path,
) -> Result<(), Error> {
    debug!("Decrypting archive: {}", input_path.display());

    let bytes = fs::read(input_path)
        .map_err(|err| Error::Io(input_path.to_path_buf(), err))?;

    let decrypted_entries = Decrypter::new().decrypt(&bytes)?;

    for file in decrypted_entries {
        let path = String::from_utf8_lossy(&file.path);
        debug!("Decrypting archive entry: {}", path);

        let output_file_path = output_path.join(path.as_ref());

        if let Some(parent) = output_file_path.parent() {
            create_dir_all(parent)
                .map_err(|err| Error::Io(parent.to_path_buf(), err))?;
        }

        fs::write(&output_file_path, file.data)
            .map_err(|err| Error::Io(output_file_path, err))?;

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
) -> Result<
    Option<(Vec<MatchResult>, Vec<MatchResult>)>,
    language_tokenizer::Error,
> {
    let source_haystack = tokenize(
        source_haystack,
        source_algorithm,
        source_mode.case_sensitive,
    )?;
    let source_needle =
        tokenize(source_needle, source_algorithm, source_mode.case_sensitive)?;
    let tr_haystack =
        tokenize(tr_haystack, tr_algorithm, tr_mode.case_sensitive)?;
    let tr_needle = tokenize(tr_needle, tr_algorithm, tr_mode.case_sensitive)?;

    let source_matches = find_all_matches_(
        &source_haystack,
        &source_needle,
        source_mode.mode,
        source_mode.permissive,
    );

    let source_has_needle = !source_matches.is_empty();

    Ok(if source_has_needle {
        let translation_matches = find_all_matches_(
            &tr_haystack,
            &tr_needle,
            tr_mode.mode,
            tr_mode.permissive,
        );

        Some((source_matches, translation_matches))
    } else {
        None
    })
}

pub(crate) fn detect_lang(str: &str) {
    let lang = detect(str);

    if let Some(lang) = lang {
        let mut lock = CHECKED_LANGS.lock().unwrap();
        let entry = lock.entry(lang.lang().eng_name()).or_default();
        *entry += lang.confidence();
    }
}

pub(crate) fn get_lang() -> Option<&'static str> {
    CHECKED_LANGS
        .lock()
        .unwrap()
        .drain()
        .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap())
        .and_then(|x| Some(x.0))
}

pub(crate) fn count_words(
    text: &str,
    algorithm: Algorithm,
) -> Result<u32, Error> {
    let tokens = tokenize(text, algorithm, false)?;
    Ok(tokens.len() as u32)
}

// TODO
pub(crate) async fn language_tool_lint(
    text: &str,
    base_url: &str,
    api_key: &str,
    algorithm: Algorithm,
) -> Result<(), Error> {
    let client = ServerClient::new(base_url, "");
    let mut request = LTRequest::default()
        .with_language(to_bcp47(algorithm).to_string())
        .with_text(text);

    if !api_key.is_empty() {
        request.api_key = Some(api_key.to_string());
    }

    let response = client.check(&request).await?;

    Ok(())
}
