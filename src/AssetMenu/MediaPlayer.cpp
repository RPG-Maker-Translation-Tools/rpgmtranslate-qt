#ifdef ENABLE_ASSET_PLAYBACK
#include "MediaPlayer.hpp"

#include "Constants.hpp"

#include <QHideEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

const auto toMMSS = [](const u32 secs) -> QString {
    return "%1:%2"_L1.arg(
        QL1SV(itos(secs / 60, 2, '0').data()),
        QL1SV(itos(secs % 60, 2, '0').data())
    );
};

MediaPlayer::MediaPlayer(QWidget* const parent) :
    QWidget(parent),
    layout_(new QVBoxLayout(this)),
    mediaLabel(new QLabel(this)),

    progressWidget(new QWidget(this)),
    progressLayout(new QHBoxLayout(progressWidget)),
    progressSlider(new QSlider(Qt::Horizontal, progressWidget)),
    progressLabel(new QLabel(progressWidget)),

    pauseButton(new QPushButton(this)) {
#ifdef DEBUG_BUILD
    av_log_set_level(AV_LOG_VERBOSE);
#elifdef RELEASE_BUILD
    av_log_set_level(AV_LOG_QUIET);
#endif

    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    layout_->addWidget(mediaLabel);

    mediaLabel->setAlignment(Qt::AlignCenter);

    progressLayout->addWidget(progressSlider);
    progressLayout->addWidget(progressLabel);

    layout_->addWidget(progressWidget);
    layout_->addWidget(pauseButton);

    frameTimer.setSingleShot(false);

    connect(&frameTimer, &QTimer::timeout, this, &MediaPlayer::processTick);
    connect(progressSlider, &QSlider::sliderReleased, this, [this] -> void {
        togglePlayback();
        seekSecond(progressSlider->value());
        togglePlayback();
    });
    connect(
        pauseButton,
        &QPushButton::clicked,
        this,
        &MediaPlayer::togglePlayback
    );
}

MediaPlayer::~MediaPlayer() {
    reset();
}

auto MediaPlayer::open(const QString& filePath) -> result<void, QString> {
    reset();
    const string path = filePath.toStdString();

    i32 result;

    if (result =
            avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr);
        result < 0) {
        return Err("avformat_open_input: %1"_L1.arg(makeError(result)));
    }

    if (result = avformat_find_stream_info(formatContext, nullptr);
        result < 0) {
        return Err("avformat_find_stream_info: %1"_L1.arg(makeError(result)));
    }

    const AVCodec* videoCodec = nullptr;
    videoStreamIndex = av_find_best_stream(
        formatContext,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        &videoCodec,
        0
    );

    if (videoStreamIndex >= 0) {
        videoCodecContext = avcodec_alloc_context3(videoCodec);
        if (videoCodecContext == nullptr) {
            return Err(u"avcodec_alloc_context3 failed for video"_s);
        }

        videoStream = formatContext->streams[videoStreamIndex];

        if (result = avcodec_parameters_to_context(
                videoCodecContext,
                videoStream->codecpar
            );
            result < 0) {
            return Err("avcodec_parameters_to_context (video): %1"_L1.arg(
                makeError(result)
            ));
        }

        if (result = avcodec_open2(videoCodecContext, videoCodec, nullptr);
            result < 0) {
            return Err("avcodec_open2 (video): %1"_L1.arg(makeError(result)));
        }

        videoWidth = videoCodecContext->width;
        videoHeight = videoCodecContext->height;

        const AVRational fps = videoStream->avg_frame_rate;

        isAudioOnly = false;
        frameIntervalMS = i32(av_rescale(SECOND_MS, fps.den, fps.num));
    } else {
        isAudioOnly = true;
        frameIntervalMS = 10;
    }

    const AVCodec* audioCodec = nullptr;
    audioStreamIndex = av_find_best_stream(
        formatContext,
        AVMEDIA_TYPE_AUDIO,
        -1,
        -1,
        &audioCodec,
        0
    );

    if (isAudioOnly && audioStreamIndex < 0) {
        return Err(
            "No playable stream (video or audio) found in: %1"_L1.arg(path)
        );
    }

    if (audioStreamIndex >= 0 && audioCodec != nullptr) {
        audioCodecContext = avcodec_alloc_context3(audioCodec);

        if (audioCodecContext != nullptr) {
            audioStream = formatContext->streams[audioStreamIndex];
            const AVCodecParameters* const codecpar = audioStream->codecpar;

            const bool success =
                avcodec_parameters_to_context(audioCodecContext, codecpar) >=
                    0 &&
                avcodec_open2(audioCodecContext, audioCodec, nullptr) >= 0;

            if (!success) {
                avcodec_free_context(&audioCodecContext);
                audioCodecContext = nullptr;
                audioStreamIndex = -1;
            }

            switch (audioCodecContext->sample_fmt) {
                case AV_SAMPLE_FMT_S16:
                case AV_SAMPLE_FMT_S16P:
                    sampleSize = I16_SIZE;
                    break;
                case AV_SAMPLE_FMT_FLT:
                case AV_SAMPLE_FMT_FLTP:
                    sampleSize = F32_SIZE;
                    break;
                default:
                    avcodec_free_context(&audioCodecContext);
                    audioCodecContext = nullptr;
                    audioStreamIndex = -1;
            }

            if (audioStreamIndex != -1) {
                planarFormat =
                    audioCodecContext->sample_fmt == AV_SAMPLE_FMT_S16P ||
                    audioCodecContext->sample_fmt == AV_SAMPLE_FMT_FLTP;

                u32 maxFrameSamples;

                switch (codecpar->codec_id) {
                    case AV_CODEC_ID_AAC:
                        maxFrameSamples = u32(audioCodecContext->frame_size);
                        break;
                    case AV_CODEC_ID_OPUS:
                        maxFrameSamples = MAX_OPUS_FRAME_SAMPLES;
                        break;
                    case AV_CODEC_ID_VORBIS:
                        maxFrameSamples = MAX_VORBIS_FRAME_SAMPLES;
                        break;
                    default:
                        avcodec_free_context(&audioCodecContext);
                        audioCodecContext = nullptr;
                        audioStreamIndex = -1;
                }

                if (audioStreamIndex != -1) {
                    const u8 frameSize =
                        audioCodecContext->ch_layout.nb_channels * sampleSize;

                    deviceConfig =
                        ma_device_config_init(ma_device_type_playback);
                    deviceConfig.playback.format = (sampleSize == I16_SIZE)
                                                       ? ma_format_s16
                                                       : ma_format_f32;
                    deviceConfig.periodSizeInFrames = 2048;
                    deviceConfig.periods = 2;
                    deviceConfig.playback.channels =
                        audioCodecContext->ch_layout.nb_channels;
                    deviceConfig.sampleRate =
                        u32(audioCodecContext->sample_rate);
                    deviceConfig.dataCallback = MediaPlayer::audioDataCallback;
                    deviceConfig.pUserData = this;

                    if (ma_device_init(nullptr, &deviceConfig, &device) !=
                        MA_SUCCESS) {
                        avcodec_free_context(&audioCodecContext);
                        audioCodecContext = nullptr;
                        audioStreamIndex = -1;
                    } else {
                        audioDeviceOpen = true;
                        bytesPerSecond =
                            (audioCodecContext->sample_rate *
                             audioCodecContext->ch_layout.nb_channels *
                             sampleSize);
                    }
                }
            }
        }
    }

    if (isAudioOnly && !audioDeviceOpen) {
        return Err("Failed to open audio device for: %1"_L1.arg(path));
    }

    audioFrame = av_frame_alloc();
    packet = av_packet_alloc();

    if (audioFrame == nullptr || packet == nullptr) {
        return Err(u"FFmpeg frame/packet alloc failed"_s);
    }

    if (!isAudioOnly) {
        frame = av_frame_alloc();
        rgbFrame = av_frame_alloc();

        if (frame == nullptr || rgbFrame == nullptr) {
            return Err(u"FFmpeg frame alloc failed"_s);
        }

        const i32 bufSize = av_image_get_buffer_size(
            AV_PIX_FMT_RGB24,
            videoWidth,
            videoHeight,
            1
        );

        rgbBuffer = as<u8*>(av_malloc(usize(bufSize)));
        if (rgbBuffer == nullptr) {
            return Err(u"av_malloc failed for RGB buffer"_s);
        }

        av_image_fill_arrays(
            rgbFrame->data,
            rgbFrame->linesize,
            rgbBuffer,
            AV_PIX_FMT_RGB24,
            videoWidth,
            videoHeight,
            1
        );
    }

    if (!isAudioOnly) {
        mediaLabel->clear();
        mediaLabel->setFixedSize(videoWidth, videoHeight);
    } else {
        static constexpr u16 AUDIO_PANEL_WIDTH = 320;
        static constexpr u16 AUDIO_PANEL_HEIGHT = 96;
        videoWidth = AUDIO_PANEL_WIDTH;
        videoHeight = AUDIO_PANEL_HEIGHT;

        mediaLabel->setFixedSize(videoWidth, videoHeight);
    }

    duration = formatContext->duration / AV_TIME_BASE;
    audioBytesPlayed.store(0, std::memory_order_relaxed);
    progressSlider->setValue(0);
    progressSlider->setRange(0, duration);
    progressLabel->setText("00:00/%1"_L1.arg(toMMSS(duration)));
    pauseButton->setText(QObject::tr("Pause"));

    return {};
}

void MediaPlayer::reset() {
    frameTimer.stop();

    ringHead.store(0, std::memory_order_relaxed);
    ringTail.store(0, std::memory_order_relaxed);

    sws_freeContext(swscaleContext);
    swscaleContext = nullptr;

    av_free(rgbBuffer);
    rgbBuffer = nullptr;

    av_frame_free(&rgbFrame);
    rgbFrame = nullptr;

    av_frame_free(&audioFrame);
    audioFrame = nullptr;

    av_frame_free(&frame);
    frame = nullptr;

    av_packet_free(&packet);
    packet = nullptr;

    avcodec_free_context(&videoCodecContext);
    videoCodecContext = nullptr;

    avcodec_free_context(&audioCodecContext);
    audioCodecContext = nullptr;

    avformat_close_input(&formatContext);
    formatContext = nullptr;

    if (audioDeviceOpen) {
        ma_device_uninit(&device);
        audioDeviceOpen = false;
    }

    paused = false;
    finished = false;
}

void MediaPlayer::togglePlayback() {
    if (finished) {
        seekSecond(0);
        finished = false;
        paused = true;
    }

    paused = !paused;
    pauseButton->setText(paused ? tr("Resume") : tr("Pause"));
    paused ? frameTimer.stop() : frameTimer.start(frameIntervalMS);
    paused ? ma_device_stop(&device) : ma_device_start(&device);
}

void MediaPlayer::play() {
    finished = false;

    if (audioDeviceOpen) {
        ma_device_start(&device);
    }

    frameTimer.start(frameIntervalMS);

    adjustSize();
    show();
    raise();
    activateWindow();
}

void MediaPlayer::decodeAudio() {
    const u8 channels = audioCodecContext->ch_layout.nb_channels;
    const u32 samples = u32(audioFrame->nb_samples);
    const usize frameBytes = usize(samples) * channels * sampleSize;

    const usize used = ringHead.load(std::memory_order_relaxed) -
                       ringTail.load(std::memory_order_acquire);
    if (RING_CAPACITY - used < frameBytes) {
        return;
    }

    const usize head = ringHead.load(std::memory_order_relaxed);

    const auto ringWrite =
        [this](const usize pos, const u8* const src, const usize len) -> void {
        for (const auto idx : range(0, len)) {
            pcmBuffer[(pos + idx) & RING_MASK] = src[idx];
        }
    };

    if (!planarFormat) {
        ringWrite(head, audioFrame->data[0], frameBytes);
    } else {
        for (const auto sample : range(0, samples)) {
            for (const auto channel : range(0, channels)) {
                const usize dstPos =
                    head +
                    (((usize(sample) * channels) + channel) * sampleSize);
                ringWrite(
                    dstPos,
                    audioFrame->data[channel] + (usize(sample) * sampleSize),
                    sampleSize
                );
            }
        }
    }

    ringHead.store(head + frameBytes, std::memory_order_release);
}

void MediaPlayer::finishPlayback() {
    frameTimer.stop();
    paused = true;
    finished = true;

    pauseButton->setText(tr("Done"));
}

void MediaPlayer::processTick() {
    if (isAudioOnly) {
        const i64 bytes = audioBytesPlayed.load(std::memory_order_relaxed);
        const i64 seconds = bytes / bytesPerSecond;

        if (!progressSlider->isSliderDown()) {
            progressSlider->setValue(seconds);
            progressLabel->setText(
                "%1/%2"_L1.arg(toMMSS(seconds), toMMSS(duration))
            );
        }

        if (!refillRingBuffer(AUDIO_BUFFER_THRESHOLD) &&
            ringHead.load(std::memory_order_acquire) ==
                ringTail.load(std::memory_order_acquire)) {
            finishPlayback();
        }

        return;
    }

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            const i32 result = avcodec_send_packet(videoCodecContext, packet);
            av_packet_unref(packet);

            if (result < 0 && result != AVERROR(EAGAIN)) {
                return;
            }

            if (avcodec_receive_frame(videoCodecContext, frame) == 0) {
                const i64 timestamp = (frame->pts != AV_NOPTS_VALUE)
                                          ? frame->pts
                                          : frame->best_effort_timestamp;

                const i64 seconds =
                    floor(f64(timestamp) * av_q2d(videoStream->time_base));

                if (seconds != playbackSecond) {
                    playbackSecond = seconds;

                    if (!progressSlider->isSliderDown()) {
                        progressSlider->setValue(seconds);
                        progressLabel->setText(
                            "%1/%2"_L1.arg(toMMSS(seconds), toMMSS(duration))
                        );
                    }
                }

                if (const auto result = showFrame(); !result) {
                    frameTimer.stop();
                    finished = true;
                }

                return;
            }
        } else if (packet->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(audioCodecContext, packet) >= 0) {
                while (avcodec_receive_frame(audioCodecContext, audioFrame) ==
                       0) {
                    decodeAudio();
                }
            }

            av_packet_unref(packet);
        } else {
            av_packet_unref(packet);
        }
    }

    avcodec_send_packet(videoCodecContext, nullptr);
    if (avcodec_receive_frame(videoCodecContext, frame) == 0) {
        const auto result = showFrame();

        if (!result) {
            qCritical() << "Failed to show frame: %1"_L1.arg(result.error());
        }
    }

    finishPlayback();
}

auto MediaPlayer::showFrame() -> result<void, QString> {
    swscaleContext = sws_getCachedContext(
        swscaleContext,
        frame->width,
        frame->height,
        AVPixelFormat(frame->format),
        videoWidth,
        videoHeight,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );

    if (swscaleContext == nullptr) {
        return Err(u"sws_getCachedContext failed"_s);
    }

    sws_scale(
        swscaleContext,
        frame->data,
        frame->linesize,
        0,
        frame->height,
        rgbFrame->data,
        rgbFrame->linesize
    );

    mediaLabel->setPixmap(
        QPixmap::fromImage(QImage(
            rgbFrame->data[0],
            videoWidth,
            videoHeight,
            rgbFrame->linesize[0],
            QImage::Format_RGB888
        ))
    );
    mediaLabel->repaint();
    return {};
}

void MediaPlayer::audioDataCallback(
    ma_device* const device,
    void* const output,
    const void* const /* input */,
    const u32 frameCount
) {
    auto* const self = as<MediaPlayer*>(device->pUserData);

    const usize channels = self->audioCodecContext->ch_layout.nb_channels;
    const usize bytesNeeded = usize(frameCount) * channels * self->sampleSize;

    u8* const dst = as<u8*>(output);

    if (self->isAudioOnly) {
        self->refillRingBuffer(bytesNeeded);
    }

    const usize tail = self->ringTail.load(std::memory_order_relaxed);
    const usize available =
        self->ringHead.load(std::memory_order_acquire) - tail;
    const usize copySize = (available < bytesNeeded) ? available : bytesNeeded;
    self->audioBytesPlayed.fetch_add(copySize, std::memory_order_relaxed);

    for (const auto idx : range(0, copySize)) {
        dst[idx] = self->pcmBuffer[(tail + idx) & RING_MASK];
    }

    if (copySize < bytesNeeded) {
        memset(dst + copySize, 0, bytesNeeded - copySize);
    }

    self->ringTail.store(tail + copySize, std::memory_order_release);
}

auto MediaPlayer::refillRingBuffer(const usize minBytes) -> bool {
    while ((ringHead.load(std::memory_order_acquire) -
            ringTail.load(std::memory_order_relaxed)) < minBytes) {
        if (av_read_frame(formatContext, packet) < 0) {
            if (avcodec_send_packet(audioCodecContext, nullptr) >= 0) {
                while (avcodec_receive_frame(audioCodecContext, audioFrame) ==
                       0) {
                    decodeAudio();
                }
            }

            return false;
        }

        if (packet->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(audioCodecContext, packet) >= 0) {
                while (avcodec_receive_frame(audioCodecContext, audioFrame) ==
                       0) {
                    decodeAudio();
                }
            }
        }

        av_packet_unref(packet);
    }

    return true;
}

void MediaPlayer::hideEvent(QHideEvent* const event) {
    reset();
    QWidget::hideEvent(event);
}

void MediaPlayer::seekSecond(const u32 second) {
    const i64 timestamp = second == 0
                              ? 0
                              : av_rescale(
                                    second,
                                    isAudioOnly ? audioStream->time_base.den
                                                : videoStream->time_base.den,
                                    isAudioOnly ? audioStream->time_base.num
                                                : videoStream->time_base.num
                                );

    if (avformat_seek_file(
            formatContext,
            isAudioOnly ? audioStreamIndex : videoStreamIndex,
            0,
            timestamp,
            timestamp,
            AVSEEK_FLAG_BACKWARD
        ) < 0) {
        return;
    }

    avformat_flush(formatContext);

    if (videoCodecContext != nullptr) {
        avcodec_flush_buffers(videoCodecContext);
    }

    if (audioCodecContext != nullptr) {
        avcodec_flush_buffers(audioCodecContext);
    }

    audioBytesPlayed.store(
        i64(second) * bytesPerSecond,
        std::memory_order_relaxed
    );

    ringHead.store(0, std::memory_order_relaxed);
    ringTail.store(0, std::memory_order_relaxed);

    finished = false;

    if (audioDeviceOpen) {
        refillRingBuffer(AUDIO_BUFFER_THRESHOLD);
    }

    progressLabel->setText("%1/%2"_L1.arg(toMMSS(second), toMMSS(duration)));
}
#endif
