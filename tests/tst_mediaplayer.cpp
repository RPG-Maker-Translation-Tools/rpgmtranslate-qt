// The media player of the asset preview, run on miniaudio's null backend, which needs no sound card: what it says when
// a file can't be played, the controls of a clip, and that a clip plays to its end. Clips are made here: PCM WAV for
// audio, concatenated JPEGs (raw MJPEG) for video.
#include "Constants.hpp"
#include "MediaPlayer.hpp"

#include <QBuffer>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QtTest>

namespace {
constexpr i32 SAMPLE_RATE = 8000;

auto le32(const u32 value) -> QByteArray {
    QByteArray bytes(4, '\0');
    qToLittleEndian(value, bytes.data());
    return bytes;
}

auto le16(const u16 value) -> QByteArray {
    QByteArray bytes(2, '\0');
    qToLittleEndian(value, bytes.data());
    return bytes;
}

// Mono 16-bit PCM of a square wave
auto wavBytes(const i32 seconds) -> QByteArray {
    const u32 dataSize = static_cast<u32>(seconds * SAMPLE_RATE * 2);

    QByteArray data(static_cast<qsizetype>(dataSize), '\0');
    for (qsizetype index = 0; index < data.size() / 2; index++) {
        qToLittleEndian<i16>((index / 20) % 2 == 0 ? 8000 : -8000, data.data() + index * 2);
    }

    return "RIFF" + le32(36 + dataSize) + "WAVEfmt " + le32(16) + le16(1) + le16(1) + le32(SAMPLE_RATE) +
           le32(SAMPLE_RATE * 2) + le16(2) + le16(16) + "data" + le32(dataSize) + data;
}

auto mjpegBytes(const QColor& color) -> QByteArray {
    QImage image(64, 48, QImage::Format_RGB32);
    image.fill(color);

    QByteArray frame;
    QBuffer buffer(&frame);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 100);

    // Raw MJPEG is JPEGs one after another
    return frame + frame + frame;
}

void writeFile(const QString& path, const QByteArray& bytes) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
}

auto sliderOf(const MediaPlayer& player, const bool volume) -> QSlider* {
    for (QSlider* const slider : player.findChildren<QSlider*>()) {
        if ((slider->toolTip() == QObject::tr("Volume")) == volume) {
            return slider;
        }
    }

    return nullptr;
}

auto progressLabelOf(const MediaPlayer& player) -> QLabel* {
    for (QLabel* const label : player.findChildren<QLabel*>()) {
        if (label->text().contains(u'/')) {
            return label;
        }
    }

    return nullptr;
}
}  // namespace

class TestMediaPlayer final : public QObject {
    Q_OBJECT

    QTemporaryDir dir;

    [[nodiscard]] auto path(const QString& name) const -> QString { return dir.filePath(name); }

   private slots:
    void initTestCase() {
        QVERIFY(dir.isValid());
        MediaPlayer::nullAudioBackend = true;
        // The SoundFont is looked for here
        qApp->setProperty("data-location", dir.path());
    }

    void aFileThatIsNotThereIsReported() {
        MediaPlayer player;
        const auto result = player.open(path(u"missing.wav"_s));

        QVERIFY(!result);
        QVERIFY2(result.error().english().startsWith(u"avformat_open_input"_qsv), qPrintable(result.error().english()));
    }

    void aFileThatIsNotMediaIsReported() {
        writeFile(path(u"notes.wav"_s), "just some words, nothing to play in them at all");

        MediaPlayer player;
        const auto result = player.open(path(u"notes.wav"_s));

        QVERIFY(!result);
        QVERIFY2(result.error().english().startsWith(u"avformat_open_input"_qsv), qPrintable(result.error().english()));
    }

    void anAudioClipSetsTheControlsUp() {
        writeFile(path(u"three.wav"_s), wavBytes(3));

        MediaPlayer player;
        QVERIFY(player.open(path(u"three.wav"_s)).has_value());

        QCOMPARE(sliderOf(player, false)->maximum(), 3);
        QCOMPARE(progressLabelOf(player)->text(), u"00:00/00:03"_s);
        QCOMPARE(player.findChild<QPushButton*>()->text(), u"Pause"_s);
        // There is a device, so there is a volume to set
        QVERIFY(!sliderOf(player, true)->isHidden());
    }

    void anAudioClipPlaysToItsEnd() {
        writeFile(path(u"one.wav"_s), wavBytes(1));

        MediaPlayer player;
        QVERIFY(player.open(path(u"one.wav"_s)).has_value());
        player.play();

        auto* const button = player.findChild<QPushButton*>();
        QTRY_COMPARE_WITH_TIMEOUT(button->text(), u"Done"_s, 10000);

        // The button of a finished clip plays it again
        button->click();
        QCOMPARE(button->text(), u"Pause"_s);
    }

    void thePauseButtonPausesAndResumes() {
        writeFile(path(u"long.wav"_s), wavBytes(20));

        MediaPlayer player;
        QVERIFY(player.open(path(u"long.wav"_s)).has_value());
        player.play();

        auto* const button = player.findChild<QPushButton*>();
        button->click();
        QCOMPARE(button->text(), u"Resume"_s);
        button->click();
        QCOMPARE(button->text(), u"Pause"_s);
    }

    void aClickOnTheBarSeeks() {
        writeFile(path(u"seek.wav"_s), wavBytes(20));

        MediaPlayer player;
        QVERIFY(player.open(path(u"seek.wav"_s)).has_value());
        player.play();

        auto* const slider = sliderOf(player, false);
        slider->setSliderPosition(7);
        emit slider->actionTriggered(QAbstractSlider::SliderPageStepAdd);

        QCOMPARE(progressLabelOf(player)->text(), u"00:07/00:20"_s);
    }

    void aVideoClipShowsItsPicturesAndEnds() {
        writeFile(path(u"clip.mjpeg"_s), mjpegBytes(Qt::red));

        MediaPlayer player;
        QVERIFY(player.open(path(u"clip.mjpeg"_s)).has_value());
        // No sound in it, so there is no device and no volume
        QVERIFY(sliderOf(player, true)->isHidden());
        player.play();

        QLabel* picture = nullptr;
        for (QLabel* const label : player.findChildren<QLabel*>()) {
            if (label != progressLabelOf(player)) {
                picture = label;
            }
        }
        QVERIFY(picture != nullptr);

        QTRY_VERIFY_WITH_TIMEOUT(!picture->pixmap().isNull(), 10000);
        const QImage shown = picture->pixmap().toImage();
        const QColor center = shown.pixelColor(shown.width() / 2, shown.height() / 2);
        QVERIFY2(center.red() > 200 && center.green() < 60 && center.blue() < 60, qPrintable(center.name()));

        QTRY_COMPARE_WITH_TIMEOUT(player.findChild<QPushButton*>()->text(), u"Done"_s, 10000);
    }

    void aSecondClipReplacesTheFirst() {
        writeFile(path(u"first.wav"_s), wavBytes(3));
        writeFile(path(u"second.wav"_s), wavBytes(5));

        MediaPlayer player;
        QVERIFY(player.open(path(u"first.wav"_s)).has_value());
        QVERIFY(player.open(path(u"second.wav"_s)).has_value());

        QCOMPARE(sliderOf(player, false)->maximum(), 5);
        QCOMPARE(progressLabelOf(player)->text(), u"00:00/00:05"_s);
    }

    void hidingThePlayerStopsTheClip() {
        writeFile(path(u"hide.wav"_s), wavBytes(20));

        MediaPlayer player;
        QVERIFY(player.open(path(u"hide.wav"_s)).has_value());
        player.play();
        QVERIFY(player.isVisible());

        // The clip is playing: the bar moves
        auto* const bar = sliderOf(player, false);
        QTRY_VERIFY_WITH_TIMEOUT(bar->value() >= 1, 5000);

        player.hide();
        // And it stops with the window: the bar stays where it was
        const i32 stoppedAt = bar->value();
        QTest::qWait(1300);
        QCOMPARE(bar->value(), stoppedAt);

        // The clip is closed with the window: a new one opens as from nothing
        QVERIFY(player.open(path(u"hide.wav"_s)).has_value());
        QCOMPARE(progressLabelOf(player)->text(), u"00:00/00:20"_s);
    }

    void midiWithoutASoundFontSaysSo() {
        writeFile(path(u"tune.mid"_s), "MThd");

        MediaPlayer player;
        const auto result = player.open(path(u"tune.mid"_s));

        QVERIFY(!result);
        QVERIFY(result.error().english().contains(u"SoundFont"_s));
    }

    void midiWithABrokenSoundFontSaysSo() {
        writeFile(path(u"tune.mid"_s), "MThd");
        writeFile(path(u"soundfont.sf2"_s), "this is not a soundfont");

        MediaPlayer player;
        const auto result = player.open(path(u"tune.mid"_s));

        QVERIFY(!result);
        QVERIFY(result.error().english().contains(u"not a valid SoundFont"_s));

        QFile::remove(path(u"soundfont.sf2"_s));
    }
};

QTEST_MAIN(TestMediaPlayer)
#include "tst_mediaplayer.moc"
