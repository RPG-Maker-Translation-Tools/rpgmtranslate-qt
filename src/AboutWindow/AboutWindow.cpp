#include "AboutWindow.hpp"

#include "MediaPlayer.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "magic_enum.hpp"
#include "nuspell-version.h"
#include "ui_AboutWindow.h"
#include "version.h"

#include <glaze/version.hpp>

#include <quickjs.h>

#ifdef ENABLE_ASSET_PLAYBACK
extern "C" {
#include <libavutil/avutil.h>
}
#endif

#ifdef ENABLE_LIBGIT2
#include <git2/common.h>
#endif

#include <archive.h>

#ifdef _WIN32
#include <ntverp.h>  // VER_PRODUCTBUILD
#include <sdkddkver.h>  // NTDDI_VERSION, WDK_NTDDI_VERSION
#endif

namespace {
constexpr auto widen(const string_view view) -> wstring {
    wstring str;
    str.reserve(view.size());
    for (const unsigned char chr : view) {
        str.push_back(scast<wchar>(chr));
    }
    return str;
}

inline auto produceBuildInfo() -> wstring {
    static constexpr wchar LINE_SEPARATOR = L'\u2028';
    wstring out;

#ifdef __clang__
    out += format(L"Compiler: Clang {}{}", widen(__clang_version__), LINE_SEPARATOR);
#elifdef __INTEL_LLVM_COMPILER
    out += format(L"Compiler: Intel LLVM {}{}", __INTEL_LLVM_COMPILER, LS);
#elifdef __GNUC__
    out += format(L"Compiler: GCC {}{}", widen(__VERSION__), LS);
#elifdef _MSC_VER
    out += format(L"Compiler: MSVC {} (toolset {}){}", _MSC_FULL_VER, _MSC_VER, LS);
#endif

#ifdef _LIBCPP_VERSION
    out += format(L"STL: libc++ {}{}", _LIBCPP_VERSION, LS);
#elifdef __GLIBCXX__
    out += format(L"STL: libstdc++ (date {}){}", __GLIBCXX__, LS);
#elifdef _MSVC_STL_VERSION
    out += format(L"STL: MSVC STL {} update {}{}", _MSVC_STL_VERSION, _MSVC_STL_UPDATE, LINE_SEPARATOR);
#endif

#ifdef _WIN32
    out += format(L"Windows SDK: NTDDI {:x}, build {}{}", NTDDI_VERSION, VER_PRODUCTBUILD, LINE_SEPARATOR);
#ifdef WDK_NTDDI_VERSION
    out += format(L"WDK target: NTDDI {:x}{}", WDK_NTDDI_VERSION, LINE_SEPARATOR);
#endif
#endif

    out += format(L"Built: {} {}{}", widen(__DATE__), widen(__TIME__), LINE_SEPARATOR);
    return out;
}

constexpr QStringView ABOUT_TEMPLATE =
    u"RPGMTranslate v%1**\u{2028}\u{2028}Direct dependencies:\u{2028}\u{2028}**fast_float %2\u{2028}jeaii-itoa\u{2028}magic_enum %3\u{2028}miniaudio %4\u{2028}zmij\u{2028}glaze %5\u{2028}Qt %6\u{2028}%7\u{2028}FFmpeg %8\u{2028}libgit2 %9\u{2028}quickjs-ng %10\u{2028}Nuspell %11**\u{2028}\u{2028}Legal info:\u{2028}\u{2028}**RPGMTranslate is licensed under [WTPFL](https://www.wtfpl.net/).\u{2028}RPGMTranslate bundles assets and statically links to the libraries licensed by other terms. See \"Third Party Notice\" tab for more information.\u{2028}See [rpgmtranslate-qt](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt) GitHub repository for more information.**\u{2028}\u{2028}Build info:\u{2028}\u{2028}**%12"_qsv;

constexpr i32 SEMVER_STRING_MAX_LEN = 8;
constexpr i32 SINGLE_DIGIT_MAX = 9;

[[nodiscard]] constexpr auto constructSemverString(const i32 maj, const i32 min, const i32 pth)
    -> array<char16_t, SEMVER_STRING_MAX_LEN> {
    const auto majStr = itos(maj);
    const auto minStr = itos(min);
    const auto pthStr = itos(pth);

    array<char16_t, SEMVER_STRING_MAX_LEN> buf{};
    i32 offset = 0;

    buf[offset++] = majStr[0];

    if (maj > SINGLE_DIGIT_MAX) {
        buf[offset++] = majStr[1];
    }

    buf[offset++] = '.';
    buf[offset++] = minStr[0];

    if (min > SINGLE_DIGIT_MAX) {
        buf[offset++] = minStr[1];
    }

    buf[offset++] = '.';
    buf[offset++] = pthStr[0];

    if (pth > SINGLE_DIGIT_MAX) {
        buf[offset++] = pthStr[1];
    }

    return buf;
}
}  // namespace

AboutWindow::AboutWindow(QWidget* const parent) : QDialog(parent), ui(setupUi()) {
    i32 maj, min, pth;
    git_libgit2_version(&maj, &min, &pth);

    ui->aboutLabel->setText(ABOUT_TEMPLATE.arg(
        svtostr(QL1SV(RPGMT_VERSION)),
        svtostr(QL1SV(FASTFLOAT_VERSION_STR)),
        QStringView(
            constructSemverString(MAGIC_ENUM_VERSION_MAJOR, MAGIC_ENUM_VERSION_MINOR, MAGIC_ENUM_VERSION_PATCH).data()
        ),
        svtostr(QL1SV(MA_VERSION_STRING)),
        QStringView(constructSemverString(glz::version.major, glz::version.minor, glz::version.patch).data()),
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
            QStringView(constructSemverString(maj, min, pth).data())
#else
            u"DISABLED"
#endif
        ),
        svtostr(QL1SV(JS_GetVersion())),
        svtostr(QL1SV(NUSPELL_VERSION)),
        produceBuildInfo()
    ));

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
    }

    QDialog::changeEvent(event);
}
