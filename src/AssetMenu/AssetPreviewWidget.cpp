#include "AssetPreviewWidget.hpp"

#include "Constants.hpp"
#include "GraphicsAssetViewer.hpp"
#include "Notice.hpp"
#include "Utils.hpp"
#include "rpgmtranslate_rs.h"

#include <QDesktopServices>
#include <QDir>
#include <QFontDatabase>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>

namespace {
constexpr i32 MIN_SCALE = 1;
constexpr i32 MAX_SCALE = 1000;

constexpr i32 FONT_SAMPLE_SIZE_MIN = 6;
constexpr i32 FONT_SAMPLE_SIZE_MAX = 288;
constexpr i32 FONT_SAMPLE_SIZE_DEFAULT = 24;
constexpr i32 MONO_FONT_POINT_SIZE = 10;
constexpr i32 MAIN_LAYOUT_SPACING = 8;
}  // namespace

AssetPreviewWidget::AssetPreviewWidget(QWidget* const parent) :
    QWidget(parent),
#ifdef ENABLE_ASSET_PLAYBACK
    mediaPlayer(new MediaPlayer(this)),
#endif

    codeViewer(new CodeViewer(this)),
    stack(new QStackedWidget(this)),

    errorLabel(new QLabel(stack)),
    supportedWritingSystemsLabel(new QLabel(this)),

    toolbar(new QWidget(this)),
    locateButton(new QPushButton(tr("Locate file"), toolbar)),
    openButton(new QPushButton(tr("Open in default app"), toolbar)),

    // JSON
    beautifyButton(new QPushButton(tr("Beautify"), toolbar)),

    // Code
    searchContainer(new QWidget(toolbar)),
    searchContainerLayout(new QHBoxLayout(searchContainer)),
    searchInput(new QLineEdit(searchContainer)),
    regularExpressionButton(new QPushButton(QIcon(u":/icons/regular_expression.svg"_s), QString(), searchContainer)),
    matchWholeButton(new QPushButton(QIcon(u":/icons/match_word.svg"_s), QString(), searchContainer)),
    matchCaseButton(new QPushButton(QIcon(u":/icons/match_case.svg"_s), QString(), searchContainer)),
    searchButton(new QPushButton(QIcon(u":/icons/search.svg"_s), QString(), searchContainer)),
    searchResultsLabel(new QLabel(searchContainer)),
    searchPrevButton(new QPushButton(QIcon(u":/icons/arrow_upward.svg"_s), QString(), searchContainer)),
    searchNextButton(new QPushButton(QIcon(u":/icons/arrow_downward.svg"_s), QString(), searchContainer)),

    // Images
    scaleContainer(new QWidget(toolbar)),
    scaleContainerLayout(new QHBoxLayout(scaleContainer)),
    scaleSlider(new QSlider(Qt::Horizontal, scaleContainer)),
    scaleLabel(new QLabel(scaleContainer)),

    // Fonts
    fontSampleContainer(new QWidget(toolbar)),
    fontSampleContainerLayout(new QHBoxLayout(fontSampleContainer)),
    fontSampleInput(new QLineEdit(fontSampleContainer)),
    fontSizeInput(new QSpinBox(fontSampleContainer)),

    toolbarLayout(new QHBoxLayout(toolbar)),
    mainLayout(new QVBoxLayout(this)),

    graphicsScene(new QGraphicsScene(this)),
    graphicsViewer(new GraphicsAssetViewer(graphicsScene, stack)) {
    errorLabel->setAlignment(Qt::AlignCenter);
    graphicsViewer->setAlignment(Qt::AlignCenter);

    // Min scale: 1%, max scale: 1000%
    scaleSlider->setRange(MIN_SCALE, MAX_SCALE);
    scaleSlider->setSingleStep(1);

    stack->addWidget(errorLabel);
    stack->addWidget(graphicsViewer);

#ifdef ENABLE_ASSET_PLAYBACK
    stack->addWidget(mediaPlayer);
#else
    auto* const mediaPlaceholder = new QLabel(tr("Media playback not available"), stack);
    mediaPlaceholder->setAlignment(Qt::AlignCenter);
    stack->addWidget(mediaPlaceholder);
#endif

    stack->addWidget(codeViewer);

    matchCaseButton->setCheckable(true);
    matchWholeButton->setCheckable(true);
    regularExpressionButton->setCheckable(true);

    fontSampleInput->setPlaceholderText(tr("The quick brown fox jumps over the lazy dog"));
    fontSizeInput->setRange(FONT_SAMPLE_SIZE_MIN, FONT_SAMPLE_SIZE_MAX);
    fontSizeInput->setValue(FONT_SAMPLE_SIZE_DEFAULT);
    fontSizeInput->setSuffix(tr(" pt"));

    fontSampleContainerLayout->addWidget(fontSampleInput);
    fontSampleContainerLayout->addWidget(fontSizeInput);

    searchContainerLayout->addWidget(searchInput);
    searchContainerLayout->addWidget(matchCaseButton);
    searchContainerLayout->addWidget(matchWholeButton);
    searchContainerLayout->addWidget(regularExpressionButton);
    searchContainerLayout->addWidget(searchButton);
    searchContainerLayout->addWidget(searchResultsLabel);
    searchContainerLayout->addWidget(searchPrevButton);
    searchContainerLayout->addWidget(searchNextButton);

    scaleContainerLayout->addWidget(scaleSlider);
    scaleContainerLayout->addWidget(scaleLabel);

    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);
    toolbarLayout->addWidget(locateButton);
    toolbarLayout->addWidget(openButton);
    toolbarLayout->addWidget(beautifyButton);
    toolbarLayout->addWidget(searchContainer);
    toolbarLayout->addWidget(scaleContainer);
    toolbarLayout->addWidget(fontSampleContainer);

    graphicsViewer->setScene(graphicsScene);
    graphicsViewer->setDragMode(QGraphicsView::ScrollHandDrag);
    graphicsViewer->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsViewer->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsViewer->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    codeViewer->setReadOnly(true);

    auto monoFont = QFont();
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(MONO_FONT_POINT_SIZE);
    codeViewer->setFont(monoFont);

    supportedWritingSystemsLabel->setWordWrap(true);

    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(MAIN_LAYOUT_SPACING);
    mainLayout->addWidget(stack);
    mainLayout->addWidget(supportedWritingSystemsLabel);
    mainLayout->addWidget(toolbar);

    const QRect screenGeometry = screen()->geometry();
    resize(screenGeometry.width() / 2, screenGeometry.height() / 2);

    connect(locateButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(currentPath).absolutePath()));
        }
    });

    connect(openButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(currentPath));
        }
    });

    connect(searchInput, &QLineEdit::returnPressed, this, [this] -> void { performSearch(); });

    connect(searchButton, &QPushButton::clicked, this, [this] -> void { performSearch(); });

    connect(searchPrevButton, &QPushButton::clicked, this, [this] -> void { navigateSearch(-1); });

    connect(searchNextButton, &QPushButton::clicked, this, [this] -> void { navigateSearch(+1); });

    connect(scaleSlider, &QSlider::valueChanged, this, [this](const i32 scale) -> void {
        graphicsViewer->setScaleFactor(scast<f32>(scale) / 100.0F);

        auto scaleString = itos(scale);
        scaleString[scaleString.len++] = u'%';
        scaleLabel->setText(QString(scaleString.qsv()));
    });

    connect(graphicsViewer, &GraphicsAssetViewer::rescaled, this, [this] -> void {
        const i32 scale = scast<i32>(roundf(graphicsViewer->currentScale() * 100));
        scaleSlider->setValue(scale);

        auto scaleString = itos(scale);
        scaleString[scaleString.len++] = u'%';
        scaleLabel->setText(QString(scaleString.qsv()));
    });

    connect(fontSampleInput, &QLineEdit::textChanged, this, [this] -> void { updateFontPreview(); });

    connect(fontSizeInput, &QSpinBox::valueChanged, this, [this] -> void { updateFontPreview(); });

    connect(beautifyButton, &QPushButton::clicked, this, [this] -> void {
        FFIString formatted;
        bool success = false;

        switch (formatLanguage) {
            case HighlightLanguage::JSON:
                rpgm_beautify_json(strtoffi(formatSource), &formatted);
                success = true;
                break;
            case HighlightLanguage::JS:
#ifdef ENABLE_JS_FORMATTING
                success = rpgm_format_src(strtoffi(formatSource), HighlightLanguage::JS, &formatted);
#endif
                break;
            case HighlightLanguage::Ruby:
#ifdef ENABLE_RUBY_FORMATTING
                success = rpgm_format_src(strtoffi(formatSource), HighlightLanguage::Ruby, &formatted);
#endif
                break;
        }

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            present(this, NOTICE("Failed to format: %1", Warning, Status, svtostr(error)));
            return;
        }

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || defined(ENABLE_RUBY_HIGHLIGHTING)
        FFIString highlightedHtml;

        if (rpgm_highlight_code(formatted, formatLanguage, &highlightedHtml)) {
            codeViewer->setHighlightedHtml(QString::fromUtf8(highlightedHtml.ptr, scast<isize>(highlightedHtml.len))
            );
            rpgm_string_free(highlightedHtml);
        } else {
            codeViewer->setPlainText(QString::fromUtf8(formatted.ptr, formatted.len));
        }
#else
        codeViewer->setPlainText(QString::fromUtf8(formatted.ptr, formatted.len));
#endif

        formatSource = QByteArray(formatted.ptr, scast<isize>(formatted.len));
        rpgm_string_free(formatted);
    });
}

AssetPreviewWidget::~AssetPreviewWidget() {
    if (!lastTempFile.isEmpty()) {
        QFile::remove(lastTempFile);
    }
}

void AssetPreviewWidget::clear() {
    currentPath.clear();
    graphicsScene->clear();
    codeViewer->clear();
    currentPath.clear();
    currentFontID = -1;
    graphicsScene->clear();
    codeViewer->clear();
}

void AssetPreviewWidget::showAsset(const QString& path) {
    currentPath = path;

    supportedWritingSystemsLabel->hide();
    beautifyButton->hide();
    scaleContainer->hide();
    searchContainer->hide();
    fontSampleContainer->hide();

    if (!lastTempFile.isEmpty()) {
        QFile::remove(lastTempFile);
        lastTempFile = QString();
    }

    const QString extension = path.sliced(path.lastIndexOf(u'.') + 1).toLower();

    if (extension == u"png_"_qsv || extension == u"rpgmvp"_qsv || extension == u"png"_qsv || extension == u"jpg"_qsv) {
        scaleContainer->show();
        loadGraphicsAsset(path, extension);
    } else if (extension == u"ttf"_qsv || extension == u"otf"_qsv) {
        fontSampleContainer->show();
        searchContainer->show();
        loadFontAsset(path);
    } else if (
        extension == u"ogg_"_qsv || extension == u"m4a_"_qsv || extension == u"rpgmvo"_qsv ||
        extension == u"rpgmvm"_qsv || extension == u"ogg"_qsv || extension == u"m4a"_qsv
    ) {
        loadAudioAsset(path, extension);
    } else if (extension == u"webm"_qsv || extension == u"mp4"_qsv) {
        loadVideoAsset(path);
    } else if (
        extension == u"js"_qsv || extension == u"json"_qsv || extension == u"rxdata"_qsv ||
        extension == u"rvdata"_qsv || extension == u"rvdata2"_qsv
    ) {
        searchContainer->show();
        loadTextAsset(path, extension);
    } else {
        showPage(Page::Error, NOTICE("Extension is unsupported.", Warning, Inline).translated());
    }

    setWindowTitle(lastPathComponent(path).toString());
    show();
}

void AssetPreviewWidget::showPage(const Page page, const QString& error) {
    if (!error.isEmpty()) {
        errorLabel->setText(error);
    }

    stack->setCurrentIndex(scast<i32>(page));
}

void AssetPreviewWidget::loadGraphicsAsset(const QString& path, const QString& extension) {
    const QByteArray utf8Path = path.toUtf8();
    QPixmap pixmap;
    bool loaded;

    if (extension == u"png"_qsv || extension == u"jpg"_qsv) {
        auto imageFile = QFile(path);

        if (!imageFile.open(QFile::ReadOnly)) {
            showPage(
                Page::Error,
                NOTICE("Failed to load asset %1: %2", Critical, Inline, path, imageFile.errorString()).translated()
            );
            return;
        }

        const auto imageData = imageFile.readAll();
        loaded = pixmap.loadFromData(imageData);
    } else {
        ByteBuffer imageData;
        const bool success = rpgm_decrypt_asset(strtoffi(utf8Path), &imageData);

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());

            showPage(
                Page::Error,
                NOTICE("Failed to decrypt asset %1: %2", Critical, Inline, path, svtostr(error)).translated()
            );
            return;
        }

        loaded = pixmap.loadFromData(imageData.ptr + HEADER_LENGTH, imageData.len - HEADER_LENGTH);
        rpgm_buffer_free(imageData);
    }

    if (!loaded) {
        showPage(Page::Error, NOTICE("Failed to load pixmap from %1", Critical, Inline, path).translated());
        return;
    }

    graphicsScene->clear();
    graphicsScene->addPixmap(pixmap);
    showPage(Page::Graphics);

    QTimer::singleShot(0, this, [this] -> void {
        graphicsViewer->fitInView(graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);

        const i32 scale = scast<i32>(roundf(graphicsViewer->currentScale() * 100));
        scaleSlider->setValue(scale);

        auto scaleString = itos(scale);
        scaleString[scaleString.len++] = u'%';
        scaleLabel->setText(QString(scaleString.qsv()));
    });
}

void AssetPreviewWidget::loadFontAsset(const QString& path) {
    const i32 fontID = QFontDatabase::addApplicationFont(path);

    if (fontID == -1) {
        showPage(Page::Error, NOTICE("Failed to load font %1", Critical, Inline, path).translated());
        return;
    }

    const QString family = QFontDatabase::applicationFontFamilies(fontID).value(0);
    currentFontID = fontID;
    fontSizeInput->setValue(FONT_SAMPLE_SIZE_DEFAULT);
    updateFontPreview();

    const auto writingSystems = QFontDatabase::writingSystems(QFont(family, FONT_SAMPLE_SIZE_DEFAULT).family());
    QStringList stringWritingSystems;
    stringWritingSystems.reserve(writingSystems.size());

    for (const auto writingSystem : writingSystems) {
        switch (writingSystem) {
            case QFontDatabase::Any:
                stringWritingSystems.append(tr("Any"));
                break;
            case QFontDatabase::Latin:
                stringWritingSystems.append(tr("Latin"));
                break;
            case QFontDatabase::Greek:
                stringWritingSystems.append(tr("Greek"));
                break;
            case QFontDatabase::Cyrillic:
                stringWritingSystems.append(tr("Cyrillic"));
                break;
            case QFontDatabase::Armenian:
                stringWritingSystems.append(tr("Armenian"));
                break;
            case QFontDatabase::Hebrew:
                stringWritingSystems.append(tr("Hebrew"));
                break;
            case QFontDatabase::Arabic:
                stringWritingSystems.append(tr("Arabic"));
                break;
            case QFontDatabase::Syriac:
                stringWritingSystems.append(tr("Syriac"));
                break;
            case QFontDatabase::Thaana:
                stringWritingSystems.append(tr("Thaana"));
                break;
            case QFontDatabase::Devanagari:
                stringWritingSystems.append(tr("Devanagari"));
                break;
            case QFontDatabase::Bengali:
                stringWritingSystems.append(tr("Bengali"));
                break;
            case QFontDatabase::Gurmukhi:
                stringWritingSystems.append(tr("Gurmukhi"));
                break;
            case QFontDatabase::Gujarati:
                stringWritingSystems.append(tr("Gujarati"));
                break;
            case QFontDatabase::Oriya:
                stringWritingSystems.append(tr("Oriya"));
                break;
            case QFontDatabase::Tamil:
                stringWritingSystems.append(tr("Tamil"));
                break;
            case QFontDatabase::Telugu:
                stringWritingSystems.append(tr("Telugu"));
                break;
            case QFontDatabase::Kannada:
                stringWritingSystems.append(tr("Kannada"));
                break;
            case QFontDatabase::Malayalam:
                stringWritingSystems.append(tr("Malayalam"));
                break;
            case QFontDatabase::Sinhala:
                stringWritingSystems.append(tr("Sinhala"));
                break;
            case QFontDatabase::Thai:
                stringWritingSystems.append(tr("Thai"));
                break;
            case QFontDatabase::Lao:
                stringWritingSystems.append(tr("Lao"));
                break;
            case QFontDatabase::Tibetan:
                stringWritingSystems.append(tr("Tibetan"));
                break;
            case QFontDatabase::Myanmar:
                stringWritingSystems.append(tr("Myanmar"));
                break;
            case QFontDatabase::Georgian:
                stringWritingSystems.append(tr("Georgian"));
                break;
            case QFontDatabase::Khmer:
                stringWritingSystems.append(tr("Khmer"));
                break;
            case QFontDatabase::SimplifiedChinese:
                stringWritingSystems.append(tr("Simplified Chinese"));
                break;
            case QFontDatabase::TraditionalChinese:
                stringWritingSystems.append(tr("Traditional Chinese"));
                break;
            case QFontDatabase::Japanese:
                stringWritingSystems.append(tr("Japanese"));
                break;
            case QFontDatabase::Korean:
                stringWritingSystems.append(tr("Korean"));
                break;
            case QFontDatabase::Vietnamese:
                stringWritingSystems.append(tr("Vietnamese"));
                break;
            case QFontDatabase::Symbol:
                stringWritingSystems.append(tr("Symbol"));
                break;
            case QFontDatabase::Ogham:
                stringWritingSystems.append(tr("Ogham"));
                break;
            case QFontDatabase::Runic:
                stringWritingSystems.append(tr("Runic"));
                break;
            case QFontDatabase::Nko:
                stringWritingSystems.append(tr("Nko"));
                break;
            case QFontDatabase::WritingSystemsCount:
                std::unreachable();
        }
    }

    supportedWritingSystemsLabel->setText(tr("Supported writing systems: ") + stringWritingSystems.join(u", "));

    showPage(Page::Graphics);

    const i32 scale = scast<i32>(roundf(graphicsViewer->currentScale() * 100));
    scaleSlider->setValue(scale);

    auto scaleString = itos(scale);
    scaleString[scaleString.len++] = u'%';
    scaleLabel->setText(QString(scaleString.qsv()));

    supportedWritingSystemsLabel->show();
}

void AssetPreviewWidget::loadAudioAsset(const QString& path, const QString& extension) {
    if (extension != u"ogg"_qsv && extension != u"m4a"_qsv) {
        const QByteArray utf8Path = path.toUtf8();

        ByteBuffer decryptedData;
        const bool success = rpgm_decrypt_asset(strtoffi(utf8Path), &decryptedData);

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            showPage(
                Page::Error,
                NOTICE("Failed to decrypt asset %1: %2", Critical, Inline, path, svtostr(error)).translated()
            );
            return;
        }

        auto tempFile = QTemporaryFile(QDir::temp().filePath(u"rpgmtranslate-qt-XXXXXX.ogg"_s));

        if (!tempFile.open()) {
            rpgm_buffer_free(decryptedData);
            showPage(
                Page::Error,
                NOTICE(
                    "Failed to open temporary file %1: %2",
                    Critical,
                    Inline,
                    tempFile.fileName(),
                    tempFile.errorString()
                )
                    .translated()
            );
            return;
        }

        tempFile.write(
            rcast<const char*>(decryptedData.ptr + HEADER_LENGTH),
            scast<i64>(decryptedData.len - HEADER_LENGTH)
        );
        tempFile.setAutoRemove(false);
        tempFile.close();
        rpgm_buffer_free(decryptedData);

        lastTempFile = tempFile.fileName();
    }

#ifdef ENABLE_ASSET_PLAYBACK
    if (const auto result =
            mediaPlayer->open(extension == u"ogg"_qsv || extension == u"m4a"_qsv ? path : lastTempFile)) {
        mediaPlayer->play();
        showPage(Page::Media);
    } else {
        showPage(Page::Error, result.error().translated());
    }
#else
    showPage(
        Page::Error,
        NOTICE("Asset playback is disabled. You can open asset in the default app.", Warning, Inline).translated()
    );
#endif
}

void AssetPreviewWidget::loadVideoAsset(const QString& path) {
#ifdef ENABLE_ASSET_PLAYBACK
    if (const auto result = mediaPlayer->open(path)) {
        mediaPlayer->play();
        showPage(Page::Media);
    } else {
        showPage(Page::Error, result.error().translated());
    }
#else
    showPage(
        Page::Error,
        NOTICE("Asset playback is disabled. You can open asset in the default app.", Warning, Inline).translated()
    );
#endif
}

void AssetPreviewWidget::loadTextAsset(const QString& path, const QString& extension) {
    auto file = QFile(path);

    if (!file.open(QFile::ReadOnly)) {
        showPage(
            Page::Error,
            NOTICE("Failed to open file %1: %2", Critical, Inline, path, file.errorString()).translated()
        );
        return;
    }

    codeUtf8 = file.readAll();

    QString code;

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || defined(ENABLE_RUBY_HIGHLIGHTING)
    bool highlighted = false;
    FFIString highlightedHtml;
#endif

    if (extension == u"rxdata"_qsv || extension == u"rvdata"_qsv || extension == u"rvdata2"_qsv) {
        FFIString json;
        const QByteArray filename = lastPathComponent(path).toUtf8();

        const bool success = rpgm_generate_json(strtoffi(codeUtf8), strtoffi(filename), &json);

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            present(this, NOTICE("Failed to generate JSON for file %1: %2", Critical, Modal, path, svtostr(error)));
            return;
        }

        if (lastPathComponent(path).contains(u"Scripts"_qsv)) {
#ifdef ENABLE_RUBY_HIGHLIGHTING
            highlighted = rpgm_highlight_code(json, HighlightLanguage::Ruby, &highlightedHtml);
#endif

#ifdef ENABLE_RUBY_FORMATTING
            formatLanguage = HighlightLanguage::Ruby;
            beautifyButton->show();
#endif
        } else {
#ifdef ENABLE_JSON_HIGHLIGHTING
            highlighted = rpgm_highlight_code(json, HighlightLanguage::JSON, &highlightedHtml);
#endif

            formatLanguage = HighlightLanguage::JSON;
            beautifyButton->show();
        }

        code = QString::fromUtf8(json.ptr, scast<isize>(json.len));
        formatSource = QByteArray(json.ptr, scast<isize>(json.len));

        rpgm_string_free(json);
    } else {
        QByteArrayView codeView;

        // BOM check
        if (codeUtf8.startsWith("\xEF\xBB\xBF"_ba)) {
            codeView = codeUtf8.slice(3);
        } else {
            codeView = codeUtf8;
        }

        if (extension == u"json"_qsv) {
#ifdef ENABLE_JSON_HIGHLIGHTING
            highlighted = rpgm_highlight_code(strtoffi(codeView), HighlightLanguage::JSON, &highlightedHtml);
#endif

            formatLanguage = HighlightLanguage::JSON;
            beautifyButton->show();
        } else {
#ifdef ENABLE_JS_HIGHLIGHTING
            highlighted = rpgm_highlight_code(strtoffi(codeView), HighlightLanguage::JS, &highlightedHtml);
#endif

#ifdef ENABLE_JS_FORMATTING
            formatLanguage = HighlightLanguage::JS;
            beautifyButton->show();
#endif
        }

        code = QString::fromUtf8(codeView);
        formatSource = codeView.toByteArray();
    }

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || defined(ENABLE_RUBY_HIGHLIGHTING)
    if (highlighted) {
        codeViewer->setHighlightedHtml(QString::fromUtf8(highlightedHtml.ptr, scast<isize>(highlightedHtml.len)));
        rpgm_string_free(highlightedHtml);
    } else {
        codeViewer->setPlainText(code);
    }
#else
    codeViewer->setPlainText(code);
#endif

    showPage(Page::Text);
}

void AssetPreviewWidget::performSearch() {
    searchResults.clear();
    currentSearchIndex = -1;
    searchResultsLabel->clear();

    const QString query = searchInput->text();
    if (query.isEmpty()) {
        codeViewer->setExtraSelections({});
        return;
    }

    const QTextDocument* const doc = codeViewer->document();

    QTextDocument::FindFlags flags;

    if (matchCaseButton->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    if (matchWholeButton->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }

    if (regularExpressionButton->isChecked()) {
        auto regexp = QRegularExpression(query);

        if (!regexp.isValid()) {
            searchResultsLabel->setText(tr("Invalid regex"));
            return;
        }

        if (!matchCaseButton->isChecked()) {
            regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }

        QTextCursor cursor = doc->find(regexp, 0, flags);

        while (!cursor.isNull()) {
            searchResults.append(cursor);
            cursor = doc->find(regexp, cursor, flags);
        }
    } else {
        QTextCursor cursor = doc->find(query, 0, flags);

        while (!cursor.isNull()) {
            searchResults.append(cursor);
            cursor = doc->find(query, cursor, flags);
        }
    }

    if (searchResults.isEmpty()) {
        searchResultsLabel->setText(tr("No results"));
        codeViewer->setExtraSelections({});
        return;
    }

    const QColor allMatchColor = QColor(255, 220, 0, 80);
    QList<QTextEdit::ExtraSelection> allHighlights;
    allHighlights.reserve(searchResults.size());

    for (const QTextCursor& cursor : searchResults) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format.setBackground(allMatchColor);
        allHighlights.append(sel);
    }

    codeViewer->setExtraSelections(allHighlights);

    currentSearchIndex = 0;
    navigateSearch(0);
}

void AssetPreviewWidget::navigateSearch(const i32 delta) {
    if (searchResults.isEmpty()) {
        return;
    }

    currentSearchIndex = scast<i32>((currentSearchIndex + delta + searchResults.size()) % searchResults.size());

    searchResultsLabel->setText(u"%1/%2"_qsv.arg(itos(currentSearchIndex + 1).qsv(), itos(searchResults.size()).qsv()));

    const QColor allMatchColor = QColor(255, 220, 0, 80);
    const QColor currentMatchColor = QColor(255, 160, 0, 180);

    QList<QTextEdit::ExtraSelection> highlights;
    highlights.reserve(searchResults.size());

    for (const auto idx : range(0, searchResults.size())) {
        QTextEdit::ExtraSelection selection;
        selection.cursor = searchResults[idx];
        selection.format.setBackground(idx == currentSearchIndex ? currentMatchColor : allMatchColor);
        highlights.append(selection);
    }

    codeViewer->setExtraSelections(highlights);

    const QTextCursor cursor = searchResults[currentSearchIndex];
    codeViewer->setTextCursor(cursor);
    codeViewer->ensureCursorVisible();
}

void AssetPreviewWidget::updateFontPreview() {
    if (currentFontID == -1) {
        return;
    }

    const QString family = QFontDatabase::applicationFontFamilies(currentFontID).value(0);

    const QString sampleText = fontSampleInput->text().isEmpty()
                                   ? u"The quick brown fox jumps over the lazy dog\n1234567890"_s
                                   : fontSampleInput->text();

    graphicsScene->clear();
    auto* const textItem = graphicsScene->addText(sampleText);
    textItem->setFont(QFont(family, fontSizeInput->value()));

    showPage(Page::Graphics);

    QTimer::singleShot(0, this, [this] -> void {
        graphicsViewer->fitInView(graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);

        const i32 scale = scast<i32>(roundf(graphicsViewer->currentScale() * 100));
        scaleSlider->setValue(scale);

        auto scaleString = itos(scale);
        scaleString[scaleString.len++] = u'%';
        scaleLabel->setText(QString(scaleString.qsv()));
    });
}