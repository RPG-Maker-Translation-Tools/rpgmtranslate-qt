#include "LintTooltip.hpp"

#include "ProjectSettings.hpp"
#include "Utils.hpp"
#include "rpgmtranslate.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QScreen>

static constexpr u8 H_PADDING = 8;
static constexpr u8 V_PADDING = 4;
static constexpr u8 BORDER_RADIUS = 4;

// TODO: Clickable suggestions for nuspell

LintTooltip::LintTooltip(QWidget* const parent) : QWidget(parent) {
    setWindowFlags(
        Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint |
        Qt::WindowTransparentForInput
    );
}

void LintTooltip::showAt(
    const QPoint& globalPos,
    const QString& text,
    const QString& captured,
    const ProjectSettings* projectSettings
) {
    // TODO
    if (captured.startsWith(R"(\N)"_L1, Qt::CaseInsensitive)) {
        QStringView capturedView = captured;
        capturedView.slice(3).chop(1);

        qDebug() << capturedView;

        const QByteArray path =
            (projectSettings->actualSourcePath() % u"/Actors.rvdata2").toUtf8();
        const QUtf8SV key = "name";
        const FFIString keyUtf8 = strtoffi(key);
        FFIString out;

        const bool success = rpgm_get_entity_data(
            strtoffi(path),
            projectSettings->engineType,
            RPGMFileType::Actors,
            capturedView.toUInt(),
            &keyUtf8,
            1,
            &out
        );

        tooltipText +=
            "%1\nThe following text will be inserted instead of this code: %2"_L1
                .arg(text, ffitostr(success ? out : rpgm_error()));
    } else if (captured.startsWith(R"(\V[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\P[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\G)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\C[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\I[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\af[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ac[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\an[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\pf[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\pc[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\pn[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nc[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ni[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nw[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\na[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ns[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nt[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ii[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\iw[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ia[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\is[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\it[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\en[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\et[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\n<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nc<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nr<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nd<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ndc<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ndr<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\nt<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ntc<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ntr<)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\faceindex[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\dg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\hc[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\playtime)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\map[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\qi[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\qw[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\qa[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\compare[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\caseSwitch[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\caseEval[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amhp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ahp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ahp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ammp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amtp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\atp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\atp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\aatk[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\adef[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amat[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amdf[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\aagi[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\aluk[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ahit[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\aeva[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\acri[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\acev[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amev[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amrf[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\acnt[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ahrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\atrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\atgr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\agrd[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\arec[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\apha[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amcr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\atcr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\apdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\amdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\afdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\aexr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emhp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ehp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ehp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emmp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emtp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\etp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\etp%[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eatk[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\edef[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emat[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emdf[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eagi[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eluk[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eexp[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\egold[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ehit[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eeva[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ecri[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ecev[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emev[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emrf[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ecnt[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\ehrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\etrg[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\etgr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\egrd[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\erec[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\epha[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emcr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\etcr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\epdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\emdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\efdr[)"_L1, Qt::CaseInsensitive)) {
    } else if (captured.startsWith(R"(\eexr[)"_L1, Qt::CaseInsensitive)) {
    } else {
        tooltipText = text;
    }

    const auto fontMetrics = QFontMetrics(font());
    const QSize textSize = fontMetrics.size(0, tooltipText);
    const u16 width = textSize.width() + (H_PADDING * 2);
    const u16 height = textSize.height() + (V_PADDING * 2);

    resize(width, height);

    QPoint pos = globalPos;
    if (const QScreen* const screen = qApp->screenAt(globalPos)) {
        const QRect screenRect = screen->availableGeometry();

        if (pos.x() + width > screenRect.right()) {
            pos.setX(screenRect.right() - width);
        }

        if (pos.x() < screenRect.left()) {
            pos.setX(screenRect.left());
        }

        if (pos.y() < screenRect.top()) {
            pos.setY(screenRect.top());
        }
    }

    move(pos);
    show();
    update();
}

void LintTooltip::hide() {
    QWidget::hide();
    tooltipText.clear();
}

void LintTooltip::paintEvent(QPaintEvent* const /* event */) {
    if (tooltipText.isEmpty()) {
        return;
    }

    const auto bodyRect = QRect(0, 0, width(), height());

    auto painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPalette& palette = this->palette();

    painter.setPen(palette.color(QPalette::Highlight));
    painter.setBrush(palette.color(QPalette::Base));
    painter.drawRect(bodyRect);

    painter.setPen(palette.color(QPalette::Text));
    painter.drawText(
        bodyRect.adjusted(H_PADDING, V_PADDING, -H_PADDING, -V_PADDING),
        Qt::AlignLeft | Qt::AlignTop,
        tooltipText
    );
}