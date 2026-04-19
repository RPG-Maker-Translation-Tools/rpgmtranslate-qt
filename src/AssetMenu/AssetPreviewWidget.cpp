#include "AssetPreviewWidget.hpp"

#include "GraphicsAssetViewer.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QDesktopServices>
#include <QDir>
#include <QFontDatabase>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryFile>
#include <QUrl>

AssetPreviewWidget::AssetPreviewWidget(QWidget* const parent) :
    QWidget(parent),
#ifdef ENABLE_ASSET_PLAYBACK
    mediaPlayer(new MediaPlayer(this)),
#endif

    codeViewer(new CodeViewer(this)),

    graphicsScene(new QGraphicsScene(this)),
    graphicsViewer(new GraphicsAssetViewer(graphicsScene, stack)),

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
    regularExpressionButton(new QPushButton(
        QIcon(u":/icons/regular_expression.svg"_s),
        QString(),
        searchContainer
    )),
    matchWholeButton(new QPushButton(
        QIcon(u":/icons/match_word.svg"_s),
        QString(),
        searchContainer
    )),
    matchCaseButton(new QPushButton(
        QIcon(u":/icons/match_case.svg"_s),
        QString(),
        searchContainer
    )),
    searchButton(new QPushButton(
        QIcon(u":/icons/search.svg"_s),
        QString(),
        searchContainer
    )),
    searchResultsLabel(new QLabel(searchContainer)),
    searchPrevButton(new QPushButton(
        QIcon(u":/icons/arrow_upward.svg"_s),
        QString(),
        searchContainer
    )),
    searchNextButton(new QPushButton(
        QIcon(u":/icons/arrow_downward.svg"_s),
        QString(),
        searchContainer
    )),

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
    mainLayout(new QVBoxLayout(this)) {
    errorLabel->setAlignment(Qt::AlignCenter);
    graphicsViewer->setAlignment(Qt::AlignCenter);

    // Min scale: 1%, max scale: 1000%
    scaleSlider->setRange(1, 1000);
    scaleSlider->setSingleStep(1);

    stack->addWidget(errorLabel);
    stack->addWidget(graphicsViewer);

#ifdef ENABLE_ASSET_PLAYBACK
    stack->addWidget(mediaPlayer);
#else
    auto* const mediaPlaceholder =
        new QLabel(tr("Media playback not available"), &stack);
    mediaPlaceholder->setAlignment(Qt::AlignCenter);
    stack->addWidget(mediaPlaceholder);
#endif

    stack->addWidget(codeViewer);

    matchCaseButton->setCheckable(true);
    matchWholeButton->setCheckable(true);
    regularExpressionButton->setCheckable(true);

    fontSampleInput->setPlaceholderText(
        tr("The quick brown fox jumps over the lazy dog")
    );
    fontSizeInput->setRange(6, 288);
    fontSizeInput->setValue(24);
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
    graphicsViewer->setRenderHints(
        QPainter::Antialiasing | QPainter::SmoothPixmapTransform
    );

    codeViewer->setReadOnly(true);

    auto monoFont = QFont(u"Monospace"_s);
    monoFont.setStyleHint(QFont::TypeWriter);
    monoFont.setPointSize(10);
    codeViewer->setFont(monoFont);

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
    highlighter = new TreeSitterHighlighter(codeViewer->document());
#endif

    supportedWritingSystemsLabel->setWordWrap(true);

    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(stack);
    mainLayout->addWidget(supportedWritingSystemsLabel);
    mainLayout->addWidget(toolbar);

    const QRect screenGeometry = screen()->geometry();
    resize(screenGeometry.width() / 2, screenGeometry.height() / 2);

    connect(locateButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(currentPath).absolutePath())
            );
        }
    });

    connect(openButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(currentPath));
        }
    });

    connect(searchInput, &QLineEdit::returnPressed, this, [this] -> void {
        performSearch();
    });

    connect(searchButton, &QPushButton::clicked, this, [this] -> void {
        performSearch();
    });

    connect(searchPrevButton, &QPushButton::clicked, this, [this] -> void {
        navigateSearch(-1);
    });

    connect(searchNextButton, &QPushButton::clicked, this, [this] -> void {
        navigateSearch(+1);
    });

    connect(
        scaleSlider,
        &QSlider::valueChanged,
        this,
        [this](const i32 scale) -> void {
        graphicsViewer->setScaleFactor(f32(scale) / 100.0F);

        array<char, 16> scaleString = itos(scale);
        scaleString[strlen(scaleString.data())] = '%';
        scaleLabel->setText(QL1SV(scaleString.data()));
    }
    );

    connect(
        graphicsViewer,
        &GraphicsAssetViewer::rescaled,
        this,
        [this] -> void {
        const i32 scale = i32(roundf(graphicsViewer->currentScale() * 100));
        scaleSlider->setValue(scale);

        array<char, 16> scaleString = itos(scale);
        scaleString[strlen(scaleString.data())] = '%';
        scaleLabel->setText(QL1SV(scaleString.data()));
    }
    );

    connect(fontSampleInput, &QLineEdit::textChanged, this, [this] -> void {
        updateFontPreview();
    });

    connect(fontSizeInput, &QSpinBox::valueChanged, this, [this] -> void {
        updateFontPreview();
    });

    connect(beautifyButton, &QPushButton::clicked, this, [this] -> void {
        FFIString beautified;
        rpgm_beautify_json(strtoffi(codeUtf8), &beautified);
        const QByteArray jsonCode = codeViewer->toPlainText().toUtf8();

        codeViewer->setPlainText(
            QString::fromUtf8(beautified.ptr, beautified.len)
        );

#ifdef ENABLE_JSON_HIGHLIGHTING
        ByteBuffer highlights;
        rpgm_highlight_code(beautified, HighlightLanguage::JSON, &highlights);

        highlighter->setHighlights(
            span<const HighlightToken>(
                ras<const HighlightToken*>(highlights.ptr),
                highlights.len
            )
        );
#else
        QMessageBox::warning(
            this,
            tr("Highlighting is disabled"),
            tr("JSON highlighting was disabled during compilation.")
        );
#endif

        rpgm_string_free(beautified);
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

    if (extension == "png_"_L1 || extension == "rpgmvp"_L1 ||
        extension == "png"_L1 || extension == "jpg"_L1) {
        scaleContainer->show();
        loadGraphicsAsset(path, extension);
    } else if (extension == "ttf"_L1 || extension == "otf"_L1) {
        fontSampleContainer->show();
        searchContainer->show();
        loadFontAsset(path);
    } else if (
        extension == "ogg_"_L1 || extension == "m4a_"_L1 ||
        extension == "rpgmvo"_L1 || extension == "rpgmvm"_L1 ||
        extension == "ogg"_L1 || extension == "m4a"_L1
    ) {
        loadAudioAsset(path, extension);
    } else if (extension == "webm"_L1 || extension == "mp4"_L1) {
        loadVideoAsset(path);
    } else if (
        extension == "js"_L1 || extension == "json"_L1 ||
        extension == "rxdata"_L1 || extension == "rvdata"_L1 ||
        extension == "rvdata2"_L1
    ) {
        searchContainer->show();
        loadTextAsset(path, extension);
    } else {
        showPage(Page::Error, tr("Extension is unsupported."));
    }

    setWindowTitle(lastPathComponent(path).toString());
    show();
}

void AssetPreviewWidget::showPage(const Page page, const QString& error) {
    if (!error.isEmpty()) {
        errorLabel->setText(error);
    }

    stack->setCurrentIndex(u8(page));
}

void AssetPreviewWidget::loadGraphicsAsset(
    const QString& path,
    const QString& extension
) {
    const QByteArray utf8Path = path.toUtf8();
    QPixmap pixmap;
    bool loaded;

    if (extension == "png"_L1 || extension == "jpg"_L1) {
        auto imageFile = QFile(path);

        if (!imageFile.open(QFile::ReadOnly)) {
            qCritical() << "Failed to load asset %1: %2"_L1.arg(
                path,
                imageFile.errorString()
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

            qCritical() << "Failed to decrypt asset %1: %2"_L1.arg(
                path,
                error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                    .toString()
#endif
            );
            showPage(
                Page::Error,
                tr("Failed to decrypt asset %1: %2")
                    .arg(
                        path,
                        error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                            .toString()
#endif
                    )
            );
            return;
        }

        loaded = pixmap.loadFromData(
            imageData.ptr + HEADER_LENGTH,
            imageData.len - HEADER_LENGTH
        );
        rpgm_buffer_free(imageData);
    }

    if (!loaded) {
        qCritical() << "Failed to load pixmap from %1"_L1.arg(path);
        showPage(Page::Error, tr("Failed to load pixmap from %1").arg(path));
        return;
    }

    graphicsScene->clear();
    const auto* const item = graphicsScene->addPixmap(pixmap);
    showPage(Page::Graphics);
    graphicsViewer->fitInView(item, Qt::KeepAspectRatio);

    const i32 scale = i32(roundf(graphicsViewer->currentScale() * 100));
    scaleSlider->setValue(scale);

    array<char, 16> scaleString = itos(scale);
    scaleString[strlen(scaleString.data())] = '%';
    scaleLabel->setText(QL1SV(scaleString.data()));
}

void AssetPreviewWidget::loadFontAsset(const QString& path) {
    const i32 fontID = QFontDatabase::addApplicationFont(path);

    if (fontID == -1) {
        qCritical() << "Failed to load font %1"_L1.arg(path);
        showPage(Page::Error, tr("Failed to load font %1").arg(path));
        return;
    }

    const QString family =
        QFontDatabase::applicationFontFamilies(fontID).value(0);
    currentFontID = fontID;
    fontSizeInput->setValue(24);
    updateFontPreview();

    const auto writingSystems =
        QFontDatabase::writingSystems(QFont(family, 24).family());
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

    supportedWritingSystemsLabel->setText(
        tr("Supported writing systems: ") + stringWritingSystems.join(", "_L1)
    );

    showPage(Page::Graphics);

    const i32 scale = i32(roundf(graphicsViewer->currentScale() * 100));
    scaleSlider->setValue(scale);

    array<char, 16> scaleString = itos(scale);
    scaleString[strlen(scaleString.data())] = '%';
    scaleLabel->setText(QL1SV(scaleString.data()));

    supportedWritingSystemsLabel->show();
}

void AssetPreviewWidget::loadAudioAsset(
    const QString& path,
    const QString& extension
) {
    if (extension != "ogg"_L1 && extension != "m4a"_L1) {
        const QByteArray utf8Path = path.toUtf8();

        ByteBuffer decryptedData;
        const bool success =
            rpgm_decrypt_asset(strtoffi(utf8Path), &decryptedData);

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            qCritical() << "Failed to decrypt asset %1: %2"_L1.arg(
                path,
                error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                    .toString()
#endif
            );
            showPage(
                Page::Error,
                tr("Failed to decrypt asset %1: %2")
                    .arg(
                        path,
                        error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                            .toString()
#endif
                    )
            );
            return;
        }

        auto tempFile = QTemporaryFile(
            QDir::temp().filePath(u"rpgmtranslate-qt-XXXXXX.ogg"_s)
        );

        if (!tempFile.open()) {
            qCritical() << "Failed to open temporary file %1: %2"_L1.arg(
                tempFile.fileName(),
                tempFile.errorString()
            );
            rpgm_buffer_free(decryptedData);
            showPage(
                Page::Error,
                tr("Failed to open temporary file %1: %2")
                    .arg(tempFile.fileName(), tempFile.errorString())
            );
            return;
        }

        tempFile.write(
            ras<cstr>(decryptedData.ptr + HEADER_LENGTH),
            i64(decryptedData.len - HEADER_LENGTH)
        );
        tempFile.setAutoRemove(false);
        tempFile.close();
        rpgm_buffer_free(decryptedData);

        lastTempFile = tempFile.fileName();
    }

#ifdef ENABLE_ASSET_PLAYBACK
    if (const auto result = mediaPlayer->open(
            extension == "ogg"_L1 || extension == "m4a"_L1 ? path : lastTempFile
        )) {
        mediaPlayer->play();
        showPage(Page::Media);
    } else {
        qCritical() << result.error();
        showPage(Page::Error, result.error());
    }
#else
    showPage(
        Page::Error,
        tr("Asset playback is disabled. You can open asset in the default app.")
    );
#endif
}

void AssetPreviewWidget::loadVideoAsset(const QString& path) {
#ifdef ENABLE_ASSET_PLAYBACK
    if (const auto result = mediaPlayer->open(path)) {
        mediaPlayer->play();
        showPage(Page::Media);
    } else {
        qCritical() << result.error();
        showPage(Page::Error, result.error());
    }
#else
    showPage(
        Page::Error,
        tr("Asset playback is disabled. You can open asset in the default app.")
    );
#endif
}

void AssetPreviewWidget::loadTextAsset(
    const QString& path,
    const QString& extension
) {
    auto file = QFile(path);

    if (!file.open(QFile::ReadOnly)) {
        qCritical() << "Failed to open file %1: %2"_L1.arg(
            path,
            file.errorString()
        );
        showPage(
            Page::Error,
            tr("Failed to open file %1: %2").arg(path, file.errorString())
        );
        return;
    }

    codeUtf8 = file.readAll();

    QString code;

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
    ByteBuffer highlights;
#endif

    if (extension == "rxdata"_L1 || extension == "rvdata"_L1 ||
        extension == "rvdata2"_L1) {
        FFIString json;
        const QByteArray filename = lastPathComponent(path).toUtf8();

        const bool success =
            rpgm_generate_json(strtoffi(codeUtf8), strtoffi(filename), &json);

        if (!success) {
            const QUtf8SV error = ffitostr(rpgm_error());
            qCritical() << "Failed to generate JSON for file %1: %2"_L1.arg(
                path,
                error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                    .toString()
#endif
            );
            QMessageBox::critical(
                this,
                tr("Failed to generate JSON"),
                tr("Failed to generate JSON for file %1: %2")
                    .arg(
                        path,
                        error
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
                            .toString()
#endif
                    )
            );
            return;
        }

        if (lastPathComponent(path).contains("Scripts"_L1)) {
#ifdef ENABLE_RUBY_HIGHLIGHTING
            rpgm_highlight_code(json, HighlightLanguage::Ruby, &highlights);
#endif
        } else {
#ifdef ENABLE_JSON_HIGHLIGHTING
            rpgm_highlight_code(json, HighlightLanguage::JSON, &highlights);
#endif

            beautifyButton->show();
        }

        code = QString::fromUtf8(json.ptr, isize(json.len));

        rpgm_string_free(json);
    } else {
        QByteArrayView codeView;

        // BOM check
        if (codeUtf8.startsWith("\xEF\xBB\xBF"_ba)) {
            codeView = codeUtf8.slice(3);
        } else {
            codeView = codeUtf8;
        }

        if (extension == "json"_L1) {
#ifdef ENABLE_JSON_HIGHLIGHTING
            rpgm_highlight_code(
                strtoffi(codeView),
                HighlightLanguage::JSON,
                &highlights
            );
#endif

            beautifyButton->show();
        } else {
#ifdef ENABLE_JS_HIGHLIGHTING
            rpgm_highlight_code(
                strtoffi(codeView),
                HighlightLanguage::JS,
                &highlights
            );
#endif
        }

        code = QString::fromUtf8(codeView);
    }

    codeViewer->setPlainText(code);

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
    highlighter->setHighlights(
        span<const HighlightToken>(
            ras<const HighlightToken*>(highlights.ptr),
            highlights.len
        )
    );
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

    QTextDocument* const doc = codeViewer->document();

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

void AssetPreviewWidget::navigateSearch(const int delta) {
    if (searchResults.isEmpty()) {
        return;
    }

    currentSearchIndex = (currentSearchIndex + delta + searchResults.size()) %
                         searchResults.size();

    searchResultsLabel->setText("%1/%2"_L1.arg(
        QL1SV(itos(currentSearchIndex + 1).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
            .toString()
#endif
            ,
        QL1SV(itos(searchResults.size()).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
            .toString()
#endif
    ));

    const QColor allMatchColor = QColor(255, 220, 0, 80);
    const QColor currentMatchColor = QColor(255, 160, 0, 180);

    QList<QTextEdit::ExtraSelection> highlights;
    highlights.reserve(searchResults.size());

    for (const auto i : range(0, searchResults.size())) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = searchResults[i];
        sel.format.setBackground(
            i == currentSearchIndex ? currentMatchColor : allMatchColor
        );
        highlights.append(sel);
    }

    codeViewer->setExtraSelections(highlights);

    QTextCursor cursor = searchResults[currentSearchIndex];
    codeViewer->setTextCursor(cursor);
    codeViewer->ensureCursorVisible();
}

void AssetPreviewWidget::updateFontPreview() {
    if (currentFontID == -1) {
        return;
    }

    const QString family =
        QFontDatabase::applicationFontFamilies(currentFontID).value(0);

    const QString sampleText =
        fontSampleInput->text().isEmpty()
            ? u"The quick brown fox jumps over the lazy dog\n1234567890"_s
            : fontSampleInput->text();

    graphicsScene->clear();
    auto* const textItem = graphicsScene->addText(sampleText);
    textItem->setFont(QFont(family, fontSizeInput->value()));

    showPage(Page::Graphics);
    graphicsViewer->fitInView(textItem, Qt::KeepAspectRatio);

    const i32 scale = i32(roundf(graphicsViewer->currentScale() * 100));
    scaleSlider->setValue(scale);

    array<char, 16> scaleString = itos(scale);
    scaleString[strlen(scaleString.data())] = '%';
    scaleLabel->setText(QL1SV(scaleString.data()));
}