#pragma once

#ifdef ENABLE_ASSET_PLAYBACK
#include "Aliases.hpp"
#include "FWD.hpp"
#include "miniaudio.h"

#include <QTimer>
#include <QWidget>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class MediaPlayer final : public QWidget {
    Q_OBJECT

   public:
    explicit MediaPlayer(QWidget* parent = nullptr);
    ~MediaPlayer() override;

    [[nodiscard]] auto open(const QString& filePath) -> result<void, QString>;
    void play();

   protected:
    void hideEvent(QHideEvent* event) override;

   private:
    [[nodiscard]] auto showFrame() -> result<void, QString>;

    void togglePlayback();
    void processTick();

    void decodeAudio();
    auto refillRingBuffer(usize minBytes) -> bool;

    void seekSecond(u32 second);
    void finishPlayback();
    void reset();

    static void audioDataCallback(
        ma_device* device,
        void* output,
        const void* input,
        u32 frameCount
    );

    [[nodiscard]] static auto makeError(const i32 errnum)
        -> array<char, AV_ERROR_MAX_STRING_SIZE> {
        array<char, AV_ERROR_MAX_STRING_SIZE> buf{};
        av_strerror(errnum, buf.data(), AV_ERROR_MAX_STRING_SIZE);
        return buf;
    }

    static constexpr u16 MAX_OPUS_FRAME_SAMPLES = 5760;
    static constexpr u16 MAX_VORBIS_FRAME_SAMPLES = 8192;
    static constexpr u8 I16_SIZE = sizeof(i16);
    static constexpr u8 F32_SIZE = sizeof(f32);

    static constexpr u32 MAX_FRAME_SIZE =
        MAX_VORBIS_FRAME_SAMPLES * 2 * F32_SIZE;
    static constexpr u32 RING_CAPACITY = MAX_FRAME_SIZE * 2;
    static constexpr u32 RING_MASK = RING_CAPACITY - 1;

    static constexpr u16 AUDIO_BUFFER_THRESHOLD = 4096;

    array<u8, RING_CAPACITY> pcmBuffer;

    ma_device device;
    ma_device_config deviceConfig;

    QTimer frameTimer;
    QTimer progressTimer;

    QVBoxLayout* const layout_;
    QLabel* const mediaLabel;

    QWidget* const progressWidget;
    QHBoxLayout* const progressLayout;
    QSlider* const progressSlider;
    QLabel* const progressLabel;

    QPushButton* const pauseButton;

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* videoCodecContext = nullptr;
    AVCodecContext* audioCodecContext = nullptr;
    SwsContext* swscaleContext = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* rgbFrame = nullptr;
    AVFrame* audioFrame = nullptr;
    AVPacket* packet = nullptr;
    AVStream* audioStream = nullptr;
    AVStream* videoStream = nullptr;

    u8* rgbBuffer = nullptr;

    i64 duration = 0;
    i64 playbackSecond = 0;

    atomicI64 audioBytesPlayed = 0;
    i64 bytesPerSecond = 0;

    atomicU32 ringHead = 0;
    atomicU32 ringTail = 0;

    i32 videoStreamIndex = -1;
    i32 audioStreamIndex = -1;
    u16 videoWidth = 0;
    u16 videoHeight = 0;
    u16 frameIntervalMS = 0;

    u8 sampleSize = 0;

    bool planarFormat = false;
    bool audioDeviceOpen = false;

    bool paused = false;
    bool finished = false;
    bool isAudioOnly = false;
};
#endif