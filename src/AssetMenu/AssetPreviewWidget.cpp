#include "AssetPreviewWidget.hpp"

#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QDesktopServices>
#include <QDir>
#include <QFontDatabase>
#include <QGraphicsTextItem>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QUrl>

// TODO: Let user type their own sample text for font inspection

AssetPreviewWidget::AssetPreviewWidget(QWidget* const parent) :
    QWidget(parent),
    errorLabel(&stack),
    graphicsViewer(&graphicsScene, &stack) {
    errorLabel.setAlignment(Qt::AlignCenter);
    graphicsViewer.setAlignment(Qt::AlignCenter);

    stack.addWidget(&errorLabel);
    stack.addWidget(&graphicsViewer);

#ifdef ENABLE_ASSET_PLAYBACK
    stack.addWidget(&mediaPlayer);
#else
    auto* const mediaPlaceholder =
        new QLabel(tr("Media playback not available"), &stack);
    mediaPlaceholder->setAlignment(Qt::AlignCenter);
    stack.addWidget(mediaPlaceholder);
#endif

    stack.addWidget(&codeViewer);

    toolbarLayout.setContentsMargins(0, 0, 0, 0);
    toolbarLayout.setSpacing(4);
    toolbarLayout.addWidget(&locateButton);
    toolbarLayout.addWidget(&openButton);
    toolbarLayout.addWidget(&beautifyButton);

    graphicsViewer.setScene(&graphicsScene);
    graphicsViewer.setDragMode(QGraphicsView::ScrollHandDrag);
    graphicsViewer.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsViewer.setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsViewer.setRenderHints(
        QPainter::Antialiasing | QPainter::SmoothPixmapTransform
    );

    codeViewer.setReadOnly(true);

    auto monoFont = QFont(u"Monospace"_s);
    monoFont.setStyleHint(QFont::TypeWriter);
    monoFont.setPointSize(10);
    codeViewer.setFont(monoFont);

#if defined(ENABLE_JSON_HIGHLIGHTING) || defined(ENABLE_JS_HIGHLIGHTING) || \
    defined(ENABLE_RUBY_HIGHLIGHTING)
    highlighter = new TreeSitterHighlighter(codeViewer.document());
#endif

    supportedWritingSystemsLabel.setWordWrap(true);

    mainLayout.setContentsMargins(4, 4, 4, 4);
    mainLayout.setSpacing(8);
    mainLayout.addWidget(&stack);
    mainLayout.addWidget(&supportedWritingSystemsLabel);
    mainLayout.addWidget(&toolbar);

    resize(screen()->geometry().width() / 2, screen()->geometry().height() / 2);

    connect(&locateButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(currentPath).absolutePath())
            );
        }
    });

    connect(&openButton, &QPushButton::clicked, this, [this] -> void {
        if (!currentPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(currentPath));
        }
    });

    connect(&beautifyButton, &QPushButton::clicked, this, [this] -> void {
        FFIString beautified;
        rpgm_beautify_json(toffistr(codeUtf8), &beautified);
        const QByteArray jsonCode = codeViewer.toPlainText().toUtf8();

        codeViewer.setPlainText(
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
    graphicsScene.clear();
    codeViewer.clear();
}

void AssetPreviewWidget::showAsset(const QString& path) {
    currentPath = path;

    supportedWritingSystemsLabel.hide();
    beautifyButton.hide();

    if (!lastTempFile.isEmpty()) {
        QFile::remove(lastTempFile);
        lastTempFile = QString();
    }

    if (path.endsWith(".png_"_L1) || path.endsWith(".rpgmvp"_L1) ||
        path.endsWith(".png"_L1) || path.endsWith(".jpg"_L1)) {
        loadGraphicsAsset(path);
    } else if (path.endsWith(".ttf"_L1) || path.endsWith(".otf"_L1)) {
        loadFontAsset(path);
    } else if (
        path.endsWith(".ogg_"_L1) || path.endsWith(".m4a_"_L1) ||
        path.endsWith(".rpgmvo"_L1) || path.endsWith(".rpgmvm"_L1) ||
        path.endsWith(".ogg"_L1) || path.endsWith(".m4a"_L1)
    ) {
        loadAudioAsset(path);
    } else if (path.endsWith(".webm"_L1) || path.endsWith(".mp4"_L1)) {
        loadVideoAsset(path);
    } else if (
        path.endsWith(".js"_L1) || path.endsWith(".json"_L1) ||
        path.endsWith(".rxdata"_L1) || path.endsWith(".rvdata"_L1) ||
        path.endsWith(".rvdata2"_L1)
    ) {
        loadTextAsset(path);
    } else {
        showPage(Page::Error, tr("Extension is unsupported."));
    }

    setWindowTitle(lastPathComponent(path).toString());
    show();
}

void AssetPreviewWidget::showPage(const Page page, const QString& error) {
    if (!error.isEmpty()) {
        errorLabel.setText(error);
    }

    stack.setCurrentIndex(u8(page));
}

void AssetPreviewWidget::loadGraphicsAsset(const QString& path) {
    const QByteArray utf8Path = path.toUtf8();
    QPixmap pixmap;
    bool loaded;

    if (path.endsWith("png"_L1) || path.endsWith("jpg"_L1)) {
        auto imageFile = QFile(path);

        if (!imageFile.open(QFile::ReadOnly)) {
            qCritical() << "Failed to load asset" << path << ':'
                        << imageFile.errorString();
            return;
        }

        const auto imageData = imageFile.readAll();
        loaded = pixmap.loadFromData(imageData);
    } else {
        ByteBuffer imageData;
        const FFIString error =
            rpgm_decrypt_asset(toffistr(utf8Path), &imageData);

        if (error.ptr != nullptr) {
            qCritical() << "Failed to decrypt asset"_L1 << path << ':'
                        << fromffistr(error);
            showPage(
                Page::Error,
                tr("Failed to decrypt asset %1: %2")
                    .arg(path, fromffistr(error))
            );
            rpgm_string_free(error);
            return;
        }

        loaded = pixmap.loadFromData(
            imageData.ptr + HEADER_LENGTH,
            imageData.len - HEADER_LENGTH
        );
        rpgm_buffer_free(imageData);
    }

    if (!loaded) {
        qCritical() << "Failed to load pixmap from"_L1 << path;
        showPage(Page::Error, tr("Failed to load pixmap from %1").arg(path));
        return;
    }

    graphicsScene.clear();
    const auto* const item = graphicsScene.addPixmap(pixmap);
    showPage(Page::Graphics);
    graphicsViewer.fitInView(item, Qt::KeepAspectRatio);
}

void AssetPreviewWidget::loadFontAsset(const QString& path) {
    const i32 fontID = QFontDatabase::addApplicationFont(path);

    if (fontID == -1) {
        qCritical() << "Failed to load font"_L1 << path;
        showPage(Page::Error, tr("Failed to load font %1").arg(path));
        return;
    }

    const QString family =
        QFontDatabase::applicationFontFamilies(fontID).value(0);
    auto* const textItem = graphicsScene.addText(
        u"The quick brown fox jumps over the lazy dog\n1234567890"_s
    );
    auto font = QFont(family, 24);
    textItem->setFont(font);

    const auto writingSystems = QFontDatabase::writingSystems(font.family());
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

    supportedWritingSystemsLabel.setText(
        tr("Supported writing systems: ") + stringWritingSystems.join(", "_L1)
    );

    showPage(Page::Graphics);
    graphicsViewer.fitInView(textItem, Qt::KeepAspectRatio);
    supportedWritingSystemsLabel.show();
}

void AssetPreviewWidget::loadAudioAsset(const QString& path) {
    if (!path.endsWith("ogg"_L1) && !path.endsWith("m4a"_L1)) {
        const QByteArray utf8Path = path.toUtf8();

        ByteBuffer decryptedData;
        const FFIString error =
            rpgm_decrypt_asset(toffistr(utf8Path), &decryptedData);

        if (error.ptr != nullptr) {
            qCritical() << "Failed to decrypt asset"_L1 << path << ':'
                        << fromffistr(error);
            showPage(
                Page::Error,
                tr("Failed to decrypt asset %1: %2")
                    .arg(path, fromffistr(error))
            );
            rpgm_string_free(error);
            return;
        }

        auto tempFile = QTemporaryFile(
            QDir::temp().filePath(u"rpgmtranslate-qt-XXXXXX.ogg"_s)
        );

        if (!tempFile.open()) {
            qCritical() << "Failed to open temporary file"_L1
                        << tempFile.fileName() << ':' << tempFile.errorString();
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
    if (const auto result = mediaPlayer.open(
            path.endsWith("ogg"_L1) || path.endsWith("m4a"_L1) ? path
                                                               : lastTempFile
        )) {
        mediaPlayer.play();
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
    if (const auto result = mediaPlayer.open(path)) {
        mediaPlayer.play();
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

void AssetPreviewWidget::loadTextAsset(const QString& path) {
    auto file = QFile(path);

    if (!file.open(QFile::ReadOnly)) {
        qCritical() << "Failed to open file"_L1 << path << ':'
                    << file.errorString();
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

    if (path.endsWith(".rxdata"_L1) || path.endsWith(".rvdata"_L1) ||
        path.endsWith(".rvdata2"_L1)) {
        FFIString json;
        const QByteArray filename = lastPathComponent(path).toUtf8();

        const FFIString error =
            rpgm_generate_json(toffistr(codeUtf8), toffistr(filename), &json);

        if (error.ptr != nullptr) {
            qCritical() << "Failed to generate JSON for file"_L1 << path << ':'
                        << QUtf8SV(error.ptr, isize(error.len));
            QMessageBox::critical(
                this,
                tr("Failed to generate JSON"),
                tr("Failed to generate JSON for file %1: %2")
                    .arg(path, QUtf8SV(error.ptr, isize(error.len)))
            );
            rpgm_string_free(error);
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

            beautifyButton.show();
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

        if (path.endsWith("json"_L1)) {
#ifdef ENABLE_JSON_HIGHLIGHTING
            rpgm_highlight_code(
                toffistr(codeView),
                HighlightLanguage::JSON,
                &highlights
            );
#endif

            beautifyButton.show();
        } else {
#ifdef ENABLE_JS_HIGHLIGHTING
            rpgm_highlight_code(
                toffistr(codeView),
                HighlightLanguage::JS,
                &highlights
            );
#endif
        }

        code = QString::fromUtf8(codeView);
    }

    codeViewer.setPlainText(code);

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