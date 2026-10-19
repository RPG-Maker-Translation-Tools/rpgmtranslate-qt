#include "AboutWindow.hpp"

#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "config.h"
#include "miniaudio.h"
#include "ui_AboutWindow.h"

#include <glaze/version.hpp>

#include <QApplication>
#include <archive.h>
#include <quickjs.h>

#ifdef ENABLE_ASSET_PLAYBACK
extern "C" {
#include <libavutil/avutil.h>
}
#endif

#ifdef ENABLE_LIBGIT2
#include <git2/common.h>
#endif

#ifdef ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

#ifdef _WIN32
#include <ntverp.h>  // VER_PRODUCTBUILD
#include <sdkddkver.h>  // NTDDI_VERSION, WDK_NTDDI_VERSION
#endif

namespace {
constexpr QChar LINE_SEPARATOR = u'\u2028';

[[nodiscard]] auto produceBuildInfo() -> QString {
    QString out;

#ifdef __clang__
    out += u"Compiler: Clang "_qsv % u"" __clang_version__ % LINE_SEPARATOR;
#elifdef __INTEL_LLVM_COMPILER
    out += u"Compiler: Intel LLVM "_qsv % qsv(qitos(__INTEL_LLVM_COMPILER)) % LINE_SEPARATOR;
#elifdef __GNUC__
    out += u"Compiler: GCC "_qsv % u"" __VERSION__ % LINE_SEPARATOR;
#elifdef _MSC_VER
    out += u"Compiler: MSVC "_qsv % qsv(qitos(_MSC_FULL_VER)) % u" (toolset "_qsv % qsv(qitos(_MSC_VER)) % u')' %
           LINE_SEPARATOR;
#endif

#ifdef _LIBCPP_VERSION
    out += u"STL: libc++ "_qsv % qsv(qitos(_LIBCPP_VERSION)) % LINE_SEPARATOR;
#elifdef __GLIBCXX__
    out += u"STL: libstdc++ (date "_qsv % qsv(qitos(__GLIBCXX__)) % u')' % LINE_SEPARATOR;
#elifdef _MSVC_STL_VERSION
    out += u"STL: MSVC STL "_qsv % qsv(qitos(_MSVC_STL_VERSION)) % u" update "_qsv % qsv(qitos(_MSVC_STL_UPDATE)) %
           LINE_SEPARATOR;
#endif

#ifdef _WIN32
    out += u"Windows SDK: NTDDI "_qsv % qsv(qhtos(scast<u32>(NTDDI_VERSION))) % u", build "_qsv %
           qsv(qitos(VER_PRODUCTBUILD)) % LINE_SEPARATOR;
#ifdef WDK_NTDDI_VERSION
    out += u"WDK target: NTDDI "_qsv % qsv(qhtos(scast<u32>(WDK_NTDDI_VERSION))) % LINE_SEPARATOR;
#endif
#endif

    out += u"Rust version: "_qsv % RUST_VERSION % LINE_SEPARATOR;
    out += u"Built: "_qsv % u"" __DATE__ % u' ' % u"" __TIME__ % LINE_SEPARATOR;

    return out;
}

constexpr const char* ABOUT_TEMPLATE = QT_TRANSLATE_NOOP(
    "AboutWindow",
    "RPGMTranslate v%1 'Death of RPG Maker forums'**\u{2028}\u{2028}Direct dependencies:\u{2028}\u{2028}**fast_float %2\u{2028}enchantum\u{2028}miniaudio %3\u{2028}zmij\u{2028}glaze %4\u{2028}Qt %5\u{2028}%6\u{2028}FFmpeg %7\u{2028}libgit2 %8\u{2028}quickjs-ng %9\u{2028}Nuspell %10\u{2028}mimalloc %11\u{2028}TinySoundFont**\u{2028}\u{2028}Legal info:\u{2028}\u{2028}**RPGMTranslate is licensed under [WTFPL](https://www.wtfpl.net/).\u{2028}RPGMTranslate bundles assets and statically links to the libraries licensed by other terms. See \"Third Party Notice\" tab for more information.\u{2028}See [rpgmtranslate-qt](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt) GitHub repository for more information.**\u{2028}\u{2028}Build info:\u{2028}\u{2028}**%12"
);

[[nodiscard]] auto semver(const i32 major, const i32 minor, const i32 patch) -> Digits<char16_t> {
    Digits<char16_t> out{};
    out.length = 0;

    for (const i32 part : { major, minor, patch }) {
        if (out.length != 0) {
            out.buffer[out.length++] = u'.';
        }

        for (const char16_t chr : qitos(part).view()) {
            out.buffer[out.length++] = chr;
        }
    }

    return out;
}

#ifdef ENABLE_MIMALLOC
[[nodiscard]] auto mimallocVersion() -> Digits<char16_t> {
    const i32 version = mi_version();
    return semver(version / 10000, (version / 100) % 100, version % 100);
}
#endif
}

AboutWindow::AboutWindow(QWidget* const parent) : QDialog(parent), ui(setupUi()) {
    fill();
}

// The texts that are set in code. `retranslateUi` puts the placeholders of the `.ui` file back over them.
void AboutWindow::fill() {
#ifdef ENABLE_LIBGIT2
    i32 maj, min, pth;
    git_libgit2_version(&maj, &min, &pth);
#endif

    ui->aboutLabel->setText(
        tr(ABOUT_TEMPLATE)
            .arg(
                QStringView(RPGMT_VERSION),
                svtostr(u"" FASTFLOAT_VERSION_STR),
                svtostr(u"" MA_VERSION_STRING),
                QStringView(semver(glz::version.major, glz::version.minor, glz::version.patch).data()),
                svtostr(QL1SV(qVersion())),
                svtostr(QL1SV(archive_version_string())),
                svtostr(
#ifdef ENABLE_ASSET_PLAYBACK
                    QL1SV(av_version_info())
#else
                    u"DISABLED"
#endif
                ),
                svtostr(
#ifdef ENABLE_LIBGIT2
                    qsv(semver(maj, min, pth))
#else
                    u"DISABLED"
#endif
                ),
                svtostr(QL1SV(JS_GetVersion())),
                svtostr(NUSPELL_VERSION),
                svtostr(
#ifdef ENABLE_MIMALLOC
                    qsv(mimallocVersion())
#else
                    u"DISABLED"
#endif
                ),
                produceBuildInfo()
            )
    );

    auto thirdPartyNoticeFile = QFile(qApp->applicationDirPath() % u"/THIRD-PARTY-NOTICE.md"_qsv);

    if (!thirdPartyNoticeFile.open(QFile::ReadOnly)) {
        ui->thirdPartyLabel->setText(
            tr("Failed to load THIRD-PARTY-NOTICE.md: %1. It usually comes bundled with RPGMTranslate.")
                .arg(thirdPartyNoticeFile.errorString())
        );
        return;
    }

    ui->thirdPartyLabel->setText(QString::fromUtf8(thirdPartyNoticeFile.readAll()));
}

AboutWindow::~AboutWindow() {
    delete ui;
}

auto AboutWindow::setupUi() -> Ui::AboutWindow* {
    auto* const ui_ = new Ui::AboutWindow();
    ui_->setupUi(this);
    return ui_;
}

void AboutWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        fill();
    }

    QDialog::changeEvent(event);
}
