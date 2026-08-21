#pragma once

#include "Aliases.hpp"
#include "CodeViewer.hpp"
#include "GraphicsAssetViewer.hpp"
#include "rpgmtranslate_rs.h"

#ifdef ENABLE_ASSET_PLAYBACK
#include "MediaPlayer.hpp"
#endif

#include <QWidget>

class AssetPreviewWidget final : public QWidget {
    Q_OBJECT

   public:
    explicit AssetPreviewWidget(QWidget* parent = nullptr);
    ~AssetPreviewWidget() override;

    void showAsset(const QString& path);
    void clear();

   private:
    enum class Page : u8 {
        Error,
        Graphics,
        Media,
        Text,
    };

    void showPage(Page page, const QString& error = QString());

    void loadGraphicsAsset(const QString& path, const QString& extension);
    void loadFontAsset(const QString& path);
    void loadAudioAsset(const QString& path, const QString& extension);
    void loadVideoAsset(const QString& path);
    void loadTextAsset(const QString& path, const QString& extension);

    void performSearch();
    void navigateSearch(i32 delta);

    void updateFontPreview();

    QString currentPath;
    QString lastTempFile;

    QByteArray codeUtf8;
    QByteArray formatSource;
    HighlightLanguage formatLanguage;
    QList<QTextCursor> searchResults;

#ifdef ENABLE_ASSET_PLAYBACK
    MediaPlayer* const mediaPlayer;
#endif

    CodeViewer* const codeViewer;

    QStackedWidget* const stack;

    QLabel* const errorLabel;
    QLabel* const supportedWritingSystemsLabel;

    QWidget* const toolbar;
    QPushButton* const locateButton;
    QPushButton* const openButton;

    // JSON
    QPushButton* const beautifyButton;

    // Code
    QWidget* const searchContainer;
    QHBoxLayout* const searchContainerLayout;
    QLineEdit* const searchInput;
    QPushButton* const regularExpressionButton;
    QPushButton* const matchWholeButton;
    QPushButton* const matchCaseButton;
    QPushButton* const searchButton;
    QLabel* const searchResultsLabel;
    QPushButton* const searchPrevButton;
    QPushButton* const searchNextButton;

    // Images
    QWidget* const scaleContainer;
    QHBoxLayout* const scaleContainerLayout;
    QSlider* const scaleSlider;
    QLabel* const scaleLabel;

    // Fonts
    QWidget* const fontSampleContainer;
    QHBoxLayout* const fontSampleContainerLayout;
    QLineEdit* const fontSampleInput;
    QSpinBox* const fontSizeInput;

    QHBoxLayout* const toolbarLayout;
    QVBoxLayout* const mainLayout;

    QGraphicsScene* const graphicsScene;
    GraphicsAssetViewer* const graphicsViewer;

    i32 currentFontID = -1;
    i32 currentSearchIndex = -1;
};