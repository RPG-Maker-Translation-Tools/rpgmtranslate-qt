extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/avutil.h>
}

#include "AboutWindow.hpp"
#include "Constants.hpp"
#include "ui_AboutWindow.h"
#include "version.h"

#ifdef ENABLE_LIBGIT2
#include <git2/common.h>
#endif

#include <archive.h>

AboutWindow::AboutWindow(QWidget* const parent) :
    QDialog(parent),
    ui(setupUi()) {
    ui->versionLabel->setText(
        u"RPGMTranslate v"_s + QString::fromLatin1(APP_VERSION)
    );
    ui->qtVersionLabel->setText(u"Qt "_s + QString::fromLatin1(qVersion()));

    ui->libarchiveVersionLabel->setText(
        QString::fromLatin1(archive_version_string())
    );

#ifdef ENABLE_LIBGIT2
    i32 maj;
    i32 min;
    i32 pth;

    git_libgit2_version(&maj, &min, &pth);
    ui->libgit2VersionLabel->setText(u"libgit2 %1.%2.%3"_s.arg(
        QL1SV(itos(maj).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
            .toString()
#endif
            ,
        QL1SV(itos(min).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
            .toString()
#endif
            ,
        QL1SV(itos(pth).data())
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
            .toString()
#endif
    ));
#endif

#ifdef ENABLE_ASSET_PLAYBACK
    ui->ffmpegVersionLabel->setText(
        "FFmpeg %1"_L1.arg(QString::fromLatin1(av_version_info()))
    );
#endif

    // TODO: Nuspell does not yet provide the version.
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
