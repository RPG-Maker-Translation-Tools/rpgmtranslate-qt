#pragma once

#ifdef ENABLE_ASSET_PLAYBACK
#include "Aliases.hpp"
#include "FWD.hpp"
#include "Notice.hpp"
#include "miniaudio.h"
#include "tml.h"
#include "tsf.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

class MediaPlayer final : public QWidget {
    Q_OBJECT

   public:
    explicit MediaPlayer(QWidget* parent = nullptr);
    ~MediaPlayer() override;

    [[nodiscard]] auto open(const QString& filePath) -> Result<void, Notice>;
    void play();

   protected:
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    [[nodiscard]] auto showFrame(const AVFrame* source) -> Result<void, Notice>;

    void togglePlayback();
    void applyVolume();
    void processTick();

    [[nodiscard]] auto openAudio(const AVCodec* codec) -> bool;
    void decodeAudio();

    // Sizes the picture or the audio panel, and sets the progress bar up for a clip of the given length.
    void configureControls(i32 durationSeconds);

    // MIDI has nothing for FFmpeg to decode: the file is a score that TinySoundFont plays on the device's own
    // callback, from the SoundFont next to the executable. It shares the progress bar and device with the rest.
    [[nodiscard]] auto openMidi(const QString& filePath) -> Result<void, Notice>;
    void renderMidi(f32* output, u32 frameCount);

    void readPacket();
    void decodeVideoFrames();
    void clearFrames();

    // The time the picture has to catch up with: what the speakers have played, or the wall clock without audio.
    [[nodiscard]] auto clockSeconds() const -> f64;
    [[nodiscard]] auto framePts(const AVFrame* source) const -> f64;
    [[nodiscard]] auto audioStarving() const -> bool;
    [[nodiscard]] auto audioTargetBytes() const -> usize;
    auto refillRingBuffer(usize minBytes) -> bool;

    void seekSecond(i32 second);
    void rewindTo(i32 second);
    void finishPlayback();
    void reset();

    static void audioDataCallback(ma_device* device, void* output, const void* input, u32 frameCount);

    [[nodiscard]] static auto makeError(const i32 errnum) -> array<char, AV_ERROR_MAX_STRING_SIZE> {
        array<char, AV_ERROR_MAX_STRING_SIZE> buf{};
        av_strerror(errnum, buf.data(), AV_ERROR_MAX_STRING_SIZE);
        return buf;
    }

    static constexpr i32 MAX_OPUS_FRAME_SAMPLES = 5760;
    static constexpr i32 MAX_VORBIS_FRAME_SAMPLES = 8192;
    static constexpr i32 F32_SIZE = sizeof(f32);

    static constexpr i32 MAX_FRAME_SIZE = MAX_VORBIS_FRAME_SAMPLES * 2 * F32_SIZE;
    // Video demuxing runs ahead of what is shown, so the audio it meets on the way must fit without being dropped.
    static constexpr i32 RING_CAPACITY = 1 << 20;
    static constexpr i32 RING_MASK = RING_CAPACITY - 1;

    // Demuxing and decoding stay on the GUI thread; one tick reads at most this many packets
    static constexpr u32 MAX_PACKETS_PER_TICK = 64;

    static constexpr i32 VOLUME_MAX = 100;
    static constexpr i32 VOLUME_SLIDER_WIDTH = 100;

    static constexpr i32 MIDI_SAMPLE_RATE = 44100;
    static constexpr i32 MIDI_OUTPUT_CHANNELS = 2;
    static constexpr i32 MIDI_TRACK_CHANNELS = 16;
    // Messages are applied between blocks this long, which is 1.5 ms: finer than any score is written.
    static constexpr u32 MIDI_BLOCK_FRAMES = 64;
    // Let the last notes ring out before the track counts as finished.
    static constexpr u32 MIDI_TAIL_MS = 1000;
    static constexpr i32 MIDI_DRUM_CHANNEL = 9;
    static constexpr f32 MIDI_MAX_VELOCITY = 127.0F;

    // Short enough that a frame is shown within a few milliseconds of its time, at no cost while there is none due.
    static constexpr i32 VIDEO_TICK_INTERVAL_MS = 5;
    static constexpr usize MAX_QUEUED_FRAMES = 32;

    // SPSC ring of interleaved float PCM: the GUI thread writes `ringHead`, the device callback reads `ringTail`.
    // Both are free-running byte counts that wrap in 32 bits; reset to 0 only with the device stopped.
    array<u8, RING_CAPACITY> pcmBuffer;
    // Decoded video frames waiting for their time, oldest first.
    deque<AVFrame*> frameQueue;
    // Decoded frame converted to interleaved float, before it is copied into the ring.
    vector<u8> convertBuffer;

    ma_device device;
    ma_device_config deviceConfig;

    QTimer frameTimer;
    QElapsedTimer wallClock;
    QTimer progressTimer;

    QVBoxLayout* const layout_;
    QLabel* const mediaLabel;

    QWidget* const progressWidget;
    QHBoxLayout* const progressLayout;
    QSlider* const progressSlider;
    QLabel* const progressLabel;
    QSlider* const volumeSlider;

    QPushButton* const pauseButton;

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* videoCodecContext = nullptr;
    AVCodecContext* audioCodecContext = nullptr;
    SwsContext* swscaleContext = nullptr;
    SwrContext* resampler = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* rgbFrame = nullptr;
    AVFrame* audioFrame = nullptr;
    AVPacket* packet = nullptr;
    AVStream* audioStream = nullptr;
    AVStream* videoStream = nullptr;

    u8* rgbBuffer = nullptr;

    // Kept between clips: loading one is slow, and it only changes with its file
    tsf* soundFont = nullptr;
    i64 soundFontModifiedMS = 0;
    i64 soundFontBytes = -1;
    // The score's first message, which owns the list, and the next one due; the latter and `midiMS` belong to the
    // device callback while the device runs.
    tml_message* midiHead = nullptr;
    tml_message* midiNext = nullptr;
    f64 midiMS = 0;
    u32 midiLengthMS = 0;
    // Where the GUI asked to seek to, taken by the callback; -1 when there is no request.
    atomicI64 midiSeekMS = -1;
    atomicBool midiDone = false;

    i32 duration = 0;
    i64 playbackSecond = 0;

    // Media time at the last (re)start of `wallClock`, and how long the device takes to play what it was handed.
    f64 wallBase = 0;
    f64 deviceLatency = 0;

    atomicI64 audioBytesPlayed = 0;
    i64 bytesPerSecond = 0;

    atomicU32 ringHead = 0;
    atomicU32 ringTail = 0;

    i32 videoStreamIndex = -1;
    i32 audioStreamIndex = -1;
    u16 videoWidth = 0;
    u16 videoHeight = 0;
    u16 frameIntervalMS = 0;

    bool demuxEnded = false;
    bool audioDeviceOpen = false;

    bool paused = false;
    bool finished = false;
    bool isAudioOnly = false;
    bool isMidi = false;
};
#endif