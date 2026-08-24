#include "AboutWindow.hpp"

#include "MediaPlayer.hpp"
#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "magic_enum.hpp"
#include "miniaudio.h"
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
constexpr QChar LINE_SEPARATOR = u'\u2028';

// std::format has no hex specifier we can use here without pulling QString through an
// intermediate wstring, so this is a small local formatter instead.
[[nodiscard]] auto toHex(const u32 value) -> QString {
    static constexpr char16_t DIGITS[] = u"0123456789abcdef";

    QString out;
    bool started = false;

    for (i32 shift = 28; shift >= 0; shift -= 4) {
        const u32 nibble = (value >> shift) & 0xFU;

        if (nibble != 0) {
            started = true;
        }

        if (started) {
            out += DIGITS[nibble];
        }
    }

    if (out.isEmpty()) {
        out += u'0';
    }

    return out;
}

[[nodiscard]] auto produceBuildInfo() -> QString {
    QString out;

#ifdef __clang__
    out += u"Compiler: Clang "_qsv % QL1SV(__clang_version__) % LINE_SEPARATOR;
#elifdef __INTEL_LLVM_COMPILER
    out += u"Compiler: Intel LLVM "_qsv % itos(__INTEL_LLVM_COMPILER).qsv() % LINE_SEPARATOR;
#elifdef __GNUC__
    out += u"Compiler: GCC "_qsv % QL1SV(__VERSION__) % LINE_SEPARATOR;
#elifdef _MSC_VER
    out += u"Compiler: MSVC "_qsv % itos(_MSC_FULL_VER).qsv() % u" (toolset "_qsv % itos(_MSC_VER).qsv() % u")"_qsv %
           LINE_SEPARATOR;
#endif

#ifdef _LIBCPP_VERSION
    out += u"STL: libc++ "_qsv % itos(_LIBCPP_VERSION).qsv() % LINE_SEPARATOR;
#elifdef __GLIBCXX__
    out += u"STL: libstdc++ (date "_qsv % itos(__GLIBCXX__).qsv() % u")"_qsv % LINE_SEPARATOR;
#elifdef _MSVC_STL_VERSION
    out += u"STL: MSVC STL "_qsv % itos(_MSVC_STL_VERSION).qsv() % u" update "_qsv % itos(_MSVC_STL_UPDATE).qsv() %
           LINE_SEPARATOR;
#endif

#ifdef _WIN32
    out += u"Windows SDK: NTDDI "_qsv % toHex(NTDDI_VERSION) % u", build "_qsv % itos(VER_PRODUCTBUILD).qsv() %
           LINE_SEPARATOR;
#ifdef WDK_NTDDI_VERSION
    out += u"WDK target: NTDDI "_qsv % toHex(WDK_NTDDI_VERSION) % LINE_SEPARATOR;
#endif
#endif

    out += u"Built: "_qsv % QL1SV(__DATE__) % u' ' % QL1SV(__TIME__) % LINE_SEPARATOR;

    return out;
}

constexpr QStringView ABOUT_TEMPLATE =
    u"RPGMTranslate v%1 'Death of RPG Maker forums'**\u{2028}\u{2028}Direct dependencies:\u{2028}\u{2028}**fast_float %3\u{2028}jeaii-itoa\u{2028}magic_enum %4\u{2028}miniaudio %5\u{2028}zmij\u{2028}glaze %6\u{2028}Qt %7\u{2028}%8\u{2028}FFmpeg %9\u{2028}libgit2 %10\u{2028}quickjs-ng %11\u{2028}Nuspell %12**\u{2028}\u{2028}Legal info:\u{2028}\u{2028}**RPGMTranslate is licensed under [WTPFL](https://www.wtfpl.net/).\u{2028}RPGMTranslate bundles assets and statically links to the libraries licensed by other terms. See \"Third Party Notice\" tab for more information.\u{2028}See [rpgmtranslate-qt](https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt) GitHub repository for more information.**\u{2028}\u{2028}Build info:\u{2028}\u{2028}**%13"_qsv;

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
#ifdef ENABLE_LIBGIT2
    i32 maj, min, pth;
    git_libgit2_version(&maj, &min, &pth);
#endif

    ui->aboutLabel->setText(ABOUT_TEMPLATE.arg(
        QStringView(RPGMT_VERSION),
        svtostr(QL1SV()),
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
