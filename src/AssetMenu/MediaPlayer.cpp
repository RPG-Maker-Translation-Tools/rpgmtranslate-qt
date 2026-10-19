#ifdef ENABLE_ASSET_PLAYBACK
#include "MediaPlayer.hpp"

#include "Constants.hpp"
#include "Utils.hpp"

#include <QFile>
#include <QFileInfo>
#include <QHideEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {
constexpr i32 SECONDS_PER_MINUTE = 60;
constexpr i32 AUDIO_ONLY_FRAME_INTERVAL_MS = 10;
constexpr u32 AUDIO_PERIOD_SIZE_IN_FRAMES = 2048;
constexpr i32 VIDEO_MIN_WIDTH = 320;
constexpr i32 VIDEO_MIN_HEIGHT = 180;
}  // namespace

const auto toMMSS = [](const i32 secs) -> QString {
    return u"%1:%2"_qsv.arg(
        qsv(qitos(secs / SECONDS_PER_MINUTE, 2, '0')),
        qsv(qitos(secs % SECONDS_PER_MINUTE, 2, '0'))
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
    volumeSlider(new QSlider(Qt::Horizontal, progressWidget)),

    pauseButton(new QPushButton(this)) {
#ifdef DEBUG_BUILD
    av_log_set_level(AV_LOG_VERBOSE);
#else
    av_log_set_level(AV_LOG_QUIET);
#endif

    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    // The label is fixed to the size of the picture, which a layout would otherwise pin to the left.
    layout_->addWidget(mediaLabel, 1);

    mediaLabel->setAlignment(Qt::AlignCenter);

    volumeSlider->setRange(0, VOLUME_MAX);
    volumeSlider->setValue(VOLUME_MAX);
    volumeSlider->setFixedWidth(VOLUME_SLIDER_WIDTH);
    volumeSlider->setToolTip(tr("Volume"));

    progressLayout->addWidget(progressSlider);
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(volumeSlider);

    layout_->addWidget(progressWidget);
    layout_->addWidget(pauseButton);

    frameTimer.setSingleShot(false);

    connect(&frameTimer, &QTimer::timeout, this, &MediaPlayer::processTick);
    connect(progressSlider, &QSlider::sliderReleased, this, [this] -> void { seekSecond(progressSlider->value()); });
    // A click on the groove or a key moves the handle without a drag; the moves of a drag end in `sliderReleased`
    connect(progressSlider, &QSlider::actionTriggered, this, [this](const i32 action) -> void {
        if (action != QAbstractSlider::SliderMove) {
            seekSecond(progressSlider->sliderPosition());
        }
    });
    connect(volumeSlider, &QSlider::valueChanged, this, &MediaPlayer::applyVolume);
    connect(pauseButton, &QPushButton::clicked, this, &MediaPlayer::togglePlayback);
}

MediaPlayer::~MediaPlayer() {
    reset();
    tsf_close(soundFont);
}

auto MediaPlayer::open(const QString& filePath) -> Result<void, Notice> {
    reset();

    if (filePath.endsWith(u".mid"_qsv, Qt::CaseInsensitive) || filePath.endsWith(u".midi"_qsv, Qt::CaseInsensitive)) {
        return openMidi(filePath);
    }

    const string path = filePath.toStdString();

    i32 result;

    if (result = avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr); result < 0) {
        return Err(NOTICE("avformat_open_input: %1", Critical, Inline, svtostr(QUtf8SV(makeError(result).data()))));
    }

    if (result = avformat_find_stream_info(formatContext, nullptr); result < 0) {
        return Err(
            NOTICE("avformat_find_stream_info: %1", Critical, Inline, svtostr(QUtf8SV(makeError(result).data())))
        );
    }

    const AVCodec* videoCodec = nullptr;
    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (videoStreamIndex >= 0) {
        videoCodecContext = avcodec_alloc_context3(videoCodec);
        if (videoCodecContext == nullptr) {
            return Err(NOTICE("avcodec_alloc_context3 failed for video", Critical, Inline));
        }

        videoStream = formatContext->streams[videoStreamIndex];

        if (result = avcodec_parameters_to_context(videoCodecContext, videoStream->codecpar); result < 0) {
            return Err(NOTICE(
                "avcodec_parameters_to_context (video): %1",
                Critical,
                Inline,
                svtostr(QUtf8SV(makeError(result).data()))
            ));
        }

        if (result = avcodec_open2(videoCodecContext, videoCodec, nullptr); result < 0) {
            return Err(
                NOTICE("avcodec_open2 (video): %1", Critical, Inline, svtostr(QUtf8SV(makeError(result).data())))
            );
        }

        videoWidth = videoCodecContext->width;
        videoHeight = videoCodecContext->height;

        isAudioOnly = false;
        frameIntervalMS = VIDEO_TICK_INTERVAL_MS;
    } else {
        isAudioOnly = true;
        frameIntervalMS = AUDIO_ONLY_FRAME_INTERVAL_MS;
    }

    const AVCodec* audioCodec = nullptr;
    audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);

    if (isAudioOnly && audioStreamIndex < 0) {
        return Err(NOTICE("No playable stream (video or audio) found in: %1", Critical, Inline, filePath));
    }

    if (audioStreamIndex >= 0 && audioCodec != nullptr && !openAudio(audioCodec)) {
        swr_free(&resampler);
        avcodec_free_context(&audioCodecContext);
        audioStreamIndex = -1;
    }

    if (isAudioOnly && !audioDeviceOpen) {
        return Err(NOTICE("Failed to open audio device for: %1", Critical, Inline, filePath));
    }

    audioFrame = av_frame_alloc();
    packet = av_packet_alloc();

    if (audioFrame == nullptr || packet == nullptr) {
        return Err(NOTICE("FFmpeg frame/packet alloc failed", Critical, Inline));
    }

    if (!isAudioOnly) {
        frame = av_frame_alloc();
        rgbFrame = av_frame_alloc();

        if (frame == nullptr || rgbFrame == nullptr) {
            return Err(NOTICE("FFmpeg frame alloc failed", Critical, Inline));
        }

        const i32 bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoWidth, videoHeight, 1);

        rgbBuffer = scast<u8*>(av_malloc(scast<usize>(bufSize)));
        if (rgbBuffer == nullptr) {
            return Err(NOTICE("av_malloc failed for RGB buffer", Critical, Inline));
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

    // A stream can leave its length unknown, which is a large negative number
    configureControls(scast<i32>(max<i64>(0, formatContext->duration / AV_TIME_BASE)));

    // The device starts with something to play instead of silence
    if (isAudioOnly) {
        refillRingBuffer(audioTargetBytes());
    }

    return {};
}

void MediaPlayer::applyVolume() {
    if (audioDeviceOpen) {
        // Squared, so the slider follows loudness rather than amplitude.
        const f32 level = scast<f32>(volumeSlider->value()) / VOLUME_MAX;
        ma_device_set_master_volume(&device, level * level);
    }
}

void MediaPlayer::configureControls(const i32 durationSeconds) {
    // The previous clip's last picture would otherwise show through the audio panel, cropped.
    mediaLabel->clear();
    volumeSlider->setVisible(audioDeviceOpen);

    if (!isAudioOnly) {
        // Takes the room there is: the picture is scaled down to it, never up
        mediaLabel->setMinimumSize(VIDEO_MIN_WIDTH, VIDEO_MIN_HEIGHT);
        mediaLabel->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        layout_->setAlignment(mediaLabel, Qt::Alignment());
    } else {
        static constexpr i32 AUDIO_PANEL_WIDTH = 320;
        static constexpr i32 AUDIO_PANEL_HEIGHT = 96;
        videoWidth = AUDIO_PANEL_WIDTH;
        videoHeight = AUDIO_PANEL_HEIGHT;

        mediaLabel->setFixedSize(videoWidth, videoHeight);
        layout_->setAlignment(mediaLabel, Qt::AlignHCenter);
    }

    duration = durationSeconds;
    audioBytesPlayed.store(0, std::memory_order_relaxed);
    progressSlider->setValue(0);
    progressSlider->setRange(0, duration);
    progressLabel->setText(u"00:00/%1"_qsv.arg(toMMSS(duration)));
    pauseButton->setText(QObject::tr("Pause"));
}

auto MediaPlayer::openMidi(const QString& filePath) -> Result<void, Notice> {
    const QString soundFontPath = dataLocation() % u"/soundfont.sf2"_qsv;

    auto soundFontFile = QFile(soundFontPath);

    if (!soundFontFile.exists()) {
        return Err(
            NOTICE("MIDI playback needs a General MIDI SoundFont. Place one at %1.", Critical, Inline, soundFontPath)
        );
    }

    if (!soundFontFile.open(QFile::ReadOnly)) {
        return Err(NOTICE("Failed to open %1: %2", Critical, Inline, soundFontPath, soundFontFile.errorString()));
    }

    const QFileInfo soundFontInfo(soundFontFile);
    const i64 modifiedMS = soundFontInfo.lastModified().toMSecsSinceEpoch();

    if (soundFont != nullptr && modifiedMS == soundFontModifiedMS && soundFontInfo.size() == soundFontBytes) {
        // The device is gone, so nothing else touches it: only what the last clip left in it is cleared
        tsf_reset(soundFont);
    } else {
        tsf_close(soundFont);
        soundFont = nullptr;

        // TinySoundFont converts the samples into its own buffers, so the file's bytes are only needed while loading.
        const QByteArray soundFontData = soundFontFile.readAll();
        soundFont = tsf_load_memory(soundFontData.constData(), scast<i32>(soundFontData.size()));

        if (soundFont == nullptr) {
            return Err(NOTICE("%1 is not a valid SoundFont", Critical, Inline, soundFontPath));
        }

        soundFontModifiedMS = modifiedMS;
        soundFontBytes = soundFontInfo.size();
    }

    auto midiFile = QFile(filePath);

    if (!midiFile.open(QFile::ReadOnly)) {
        return Err(NOTICE("Failed to open %1: %2", Critical, Inline, filePath, midiFile.errorString()));
    }

    const QByteArray midiData = midiFile.readAll();
    midiHead = tml_load_memory(midiData.constData(), scast<i32>(midiData.size()));

    if (midiHead == nullptr) {
        return Err(NOTICE("%1 is not a valid MIDI file", Critical, Inline, filePath));
    }

    tml_get_info(midiHead, nullptr, nullptr, nullptr, nullptr, &midiLengthMS);

    tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, MIDI_SAMPLE_RATE, 0.0F);

    // The device callback renders from another thread, so every channel exists before it starts: setting one that
    // does not allocates.
    for (i32 channel = 0; channel < MIDI_TRACK_CHANNELS; channel++) {
        tsf_channel_set_presetnumber(soundFont, channel, 0, channel == MIDI_DRUM_CHANNEL);
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = MIDI_OUTPUT_CHANNELS;
    deviceConfig.sampleRate = MIDI_SAMPLE_RATE;
    deviceConfig.periodSizeInFrames = AUDIO_PERIOD_SIZE_IN_FRAMES;
    deviceConfig.periods = 2;
    deviceConfig.dataCallback = MediaPlayer::audioDataCallback;
    deviceConfig.pUserData = this;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        return Err(NOTICE("Failed to open audio device for: %1", Critical, Inline, filePath));
    }

    audioDeviceOpen = true;
    isMidi = true;
    isAudioOnly = true;
    frameIntervalMS = AUDIO_ONLY_FRAME_INTERVAL_MS;
    bytesPerSecond = scast<i64>(MIDI_SAMPLE_RATE) * MIDI_OUTPUT_CHANNELS * F32_SIZE;

    midiNext = midiHead;
    midiMS = 0;
    midiSeekMS.store(-1, std::memory_order_relaxed);
    midiDone.store(false, std::memory_order_relaxed);

    configureControls(scast<i32>(midiLengthMS / SECOND_MS));

    return {};
}

void MediaPlayer::renderMidi(f32* const output, const u32 frameCount) {
    const auto apply = [this](const tml_message& message, const bool audible) -> void {
        switch (message.type) {
            case TML_PROGRAM_CHANGE:
                tsf_channel_set_presetnumber(
                    soundFont,
                    message.channel,
                    message.program,
                    message.channel == MIDI_DRUM_CHANNEL
                );
                break;
            case TML_NOTE_ON:
                if (audible) {
                    tsf_channel_note_on(
                        soundFont,
                        message.channel,
                        message.key,
                        scast<f32>(message.velocity) / MIDI_MAX_VELOCITY
                    );
                }
                break;
            case TML_NOTE_OFF:
                if (audible) {
                    tsf_channel_note_off(soundFont, message.channel, message.key);
                }
                break;
            case TML_PITCH_BEND:
                tsf_channel_set_pitchwheel(soundFont, message.channel, message.pitch_bend);
                break;
            case TML_CONTROL_CHANGE:
                tsf_channel_midi_control(soundFont, message.channel, message.control, message.control_value);
                break;
            default:
                break;
        }
    };

    if (const i64 seekMS = midiSeekMS.exchange(-1, std::memory_order_acq_rel); seekMS >= 0) {
        // Silence everything and replay the score up to the target without its notes, so the channels come out
        // with the programs and controllers they would have had.
        tsf_reset(soundFont);

        for (i32 channel = 0; channel < MIDI_TRACK_CHANNELS; channel++) {
            tsf_channel_set_presetnumber(soundFont, channel, 0, channel == MIDI_DRUM_CHANNEL);
        }

        midiNext = midiHead;

        while (midiNext != nullptr && midiNext->time < seekMS) {
            apply(*midiNext, false);
            midiNext = midiNext->next;
        }

        midiMS = scast<f64>(seekMS);
    }

    static constexpr f64 MS_PER_FRAME = 1000.0 / MIDI_SAMPLE_RATE;

    u32 rendered = 0;

    while (rendered < frameCount) {
        const u32 block = min(MIDI_BLOCK_FRAMES, frameCount - rendered);

        while (midiNext != nullptr && midiNext->time <= midiMS) {
            apply(*midiNext, true);
            midiNext = midiNext->next;
        }

        tsf_render_float(soundFont, output + (scast<usize>(rendered) * MIDI_OUTPUT_CHANNELS), scast<i32>(block), 0);

        midiMS += block * MS_PER_FRAME;
        rendered += block;
    }

    audioBytesPlayed.fetch_add(scast<i64>(frameCount) * MIDI_OUTPUT_CHANNELS * F32_SIZE, std::memory_order_relaxed);
    midiDone.store(midiNext == nullptr && midiMS >= midiLengthMS + MIDI_TAIL_MS, std::memory_order_release);
}

auto MediaPlayer::openAudio(const AVCodec* const codec) -> bool {
    audioCodecContext = avcodec_alloc_context3(codec);

    if (audioCodecContext == nullptr) {
        return false;
    }

    audioStream = formatContext->streams[audioStreamIndex];

    if (avcodec_parameters_to_context(audioCodecContext, audioStream->codecpar) < 0 ||
        avcodec_open2(audioCodecContext, codec, nullptr) < 0) {
        return false;
    }

    const i32 channels = audioCodecContext->ch_layout.nb_channels;
    const i32 sampleRate = audioCodecContext->sample_rate;

    if (swr_alloc_set_opts2(
            &resampler,
            &audioCodecContext->ch_layout,
            AV_SAMPLE_FMT_FLT,
            sampleRate,
            &audioCodecContext->ch_layout,
            audioCodecContext->sample_fmt,
            sampleRate,
            0,
            nullptr
        ) < 0 ||
        swr_init(resampler) < 0) {
        return false;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.periodSizeInFrames = AUDIO_PERIOD_SIZE_IN_FRAMES;
    deviceConfig.periods = 2;
    deviceConfig.playback.channels = channels;
    deviceConfig.sampleRate = scast<u32>(sampleRate);
    deviceConfig.dataCallback = MediaPlayer::audioDataCallback;
    deviceConfig.pUserData = this;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        return false;
    }

    audioDeviceOpen = true;
    deviceLatency =
        scast<f64>(device.playback.internalPeriodSizeInFrames) * device.playback.internalPeriods / device.sampleRate;
    bytesPerSecond = scast<i64>(sampleRate) * channels * F32_SIZE;

    return true;
}

void MediaPlayer::reset() {
    frameTimer.stop();

    // The device callback reads the decoder, packet, ring and score on its own thread until the device is gone.
    if (audioDeviceOpen) {
        ma_device_uninit(&device);
        audioDeviceOpen = false;
    }

    clearFrames();
    demuxEnded = false;
    wallBase = 0;
    deviceLatency = 0;

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

    swr_free(&resampler);

    avcodec_free_context(&videoCodecContext);
    videoCodecContext = nullptr;

    avcodec_free_context(&audioCodecContext);
    audioCodecContext = nullptr;

    avformat_close_input(&formatContext);
    formatContext = nullptr;

    tml_free(midiHead);
    midiHead = nullptr;
    midiNext = nullptr;

    isMidi = false;

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

    if (paused) {
        wallBase += scast<f64>(wallClock.elapsed()) / SECOND_MS;
    } else {
        wallClock.restart();
    }

    if (audioDeviceOpen) {
        paused ? ma_device_stop(&device) : ma_device_start(&device);
    }
}

void MediaPlayer::play() {
    finished = false;
    wallBase = 0;
    wallClock.start();

    // The device is new for every clip, the slider is not.
    applyVolume();

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
    const i32 channels = audioCodecContext->ch_layout.nb_channels;
    const i32 samples = audioFrame->nb_samples;
    const usize frameBytes = scast<usize>(samples) * channels * F32_SIZE;

    const u32 head = ringHead.load(std::memory_order_relaxed);
    const u32 used = head - ringTail.load(std::memory_order_acquire);

    if (RING_CAPACITY - used < frameBytes) {
        return;
    }

    convertBuffer.resize(frameBytes);
    u8* output = convertBuffer.data();

    const i32 converted = swr_convert(
        resampler,
        &output,
        samples,
        ccast<const u8**>(rcast<const u8* const*>(audioFrame->extended_data)),
        samples
    );

    if (converted <= 0) {
        return;
    }

    const usize convertedBytes = scast<usize>(converted) * channels * F32_SIZE;

    // The ring wraps, so the copy is at most two contiguous runs.
    const usize start = head & RING_MASK;
    const usize firstRun = min<usize>(convertedBytes, RING_CAPACITY - start);

    memcpy(pcmBuffer.data() + start, output, firstRun);
    memcpy(pcmBuffer.data(), output + firstRun, convertedBytes - firstRun);

    ringHead.store(head + convertedBytes, std::memory_order_release);
}

void MediaPlayer::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        volumeSlider->setToolTip(tr("Volume"));
        pauseButton->setText(finished ? tr("Done") : (paused ? tr("Resume") : tr("Pause")));
    }

    QWidget::changeEvent(event);
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
        const i32 seconds = scast<i32>(bytes / bytesPerSecond);

        if (!progressSlider->isSliderDown()) {
            progressSlider->setValue(seconds);
            progressLabel->setText(u"%1/%2"_qsv.arg(toMMSS(seconds), toMMSS(duration)));
        }

        if (isMidi) {
            if (midiDone.load(std::memory_order_acquire)) {
                finishPlayback();
            }

            return;
        }

        // Half a second ahead keeps the device fed between ticks.
        if (!refillRingBuffer(audioTargetBytes()) &&
            ringHead.load(std::memory_order_acquire) == ringTail.load(std::memory_order_acquire)) {
            finishPlayback();
        }

        return;
    }

    const f64 clock = clockSeconds();

    // Demux until a frame is due next and the audio ring is not running dry; other packets are decoded on the way.
    u32 packets = 0;

    while (!demuxEnded && frameQueue.size() < MAX_QUEUED_FRAMES && (frameQueue.empty() || audioStarving()) &&
           packets++ < MAX_PACKETS_PER_TICK) {
        readPacket();
    }

    // The picture follows the clock: of the frames that are due only the newest is shown, the older ones are late.
    AVFrame* due = nullptr;

    while (!frameQueue.empty() && framePts(frameQueue.front()) <= clock) {
        av_frame_free(&due);
        due = frameQueue.front();
        frameQueue.pop_front();
    }

    if (due != nullptr) {
        const i32 seconds = scast<i32>(framePts(due));

        if (seconds != playbackSecond) {
            playbackSecond = seconds;

            if (!progressSlider->isSliderDown()) {
                progressSlider->setValue(seconds);
                progressLabel->setText(u"%1/%2"_qsv.arg(toMMSS(seconds), toMMSS(duration)));
            }
        }

        if (const auto result = showFrame(due); !result) {
            frameTimer.stop();
            finished = true;
        }

        av_frame_free(&due);
    }

    // The audio left in the ring is still to be heard.
    if (demuxEnded && frameQueue.empty() &&
        (!audioDeviceOpen || ringHead.load(std::memory_order_acquire) == ringTail.load(std::memory_order_acquire))) {
        finishPlayback();
    }
}

void MediaPlayer::readPacket() {
    if (av_read_frame(formatContext, packet) < 0) {
        demuxEnded = true;

        avcodec_send_packet(videoCodecContext, nullptr);
        decodeVideoFrames();

        if (audioCodecContext != nullptr && avcodec_send_packet(audioCodecContext, nullptr) >= 0) {
            while (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
                decodeAudio();
            }
        }

        return;
    }

    if (packet->stream_index == videoStreamIndex) {
        if (avcodec_send_packet(videoCodecContext, packet) >= 0) {
            decodeVideoFrames();
        }
    } else if (packet->stream_index == audioStreamIndex) {
        if (avcodec_send_packet(audioCodecContext, packet) >= 0) {
            while (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
                decodeAudio();
            }
        }
    }

    av_packet_unref(packet);
}

void MediaPlayer::decodeVideoFrames() {
    while (avcodec_receive_frame(videoCodecContext, frame) == 0) {
        // A new reference to the decoder's buffers, not a copy of the picture.
        if (AVFrame* const queued = av_frame_clone(frame)) {
            frameQueue.push_back(queued);
        }

        av_frame_unref(frame);
    }
}

void MediaPlayer::clearFrames() {
    for (AVFrame* queued : frameQueue) {
        av_frame_free(&queued);
    }

    frameQueue.clear();
}

auto MediaPlayer::clockSeconds() const -> f64 {
    if (audioDeviceOpen) {
        // What the device was handed is not yet what is heard: it still has its own buffers to play.
        const f64 played = scast<f64>(audioBytesPlayed.load(std::memory_order_relaxed)) / scast<f64>(bytesPerSecond);
        return max(0.0, played - deviceLatency);
    }

    return wallBase + (paused ? 0.0 : scast<f64>(wallClock.elapsed()) / SECOND_MS);
}

auto MediaPlayer::framePts(const AVFrame* const source) const -> f64 {
    const i64 timestamp = source->best_effort_timestamp != AV_NOPTS_VALUE ? source->best_effort_timestamp : source->pts;

    if (timestamp == AV_NOPTS_VALUE) {
        return 0.0;
    }

    const i64 start = videoStream->start_time != AV_NOPTS_VALUE ? videoStream->start_time : 0;

    return scast<f64>(timestamp - start) * av_q2d(videoStream->time_base);
}

// Half a second of what the device plays, and no more than half of the ring: a frame or two must always fit behind it.
auto MediaPlayer::audioTargetBytes() const -> usize {
    return scast<usize>(min<i64>(bytesPerSecond / 2, RING_CAPACITY / 2));
}

auto MediaPlayer::audioStarving() const -> bool {
    if (!audioDeviceOpen) {
        return false;
    }

    // A quarter of a second ahead is plenty, and far from filling the ring.
    const i64 target = min<i64>(bytesPerSecond / 4, RING_CAPACITY / 2);
    const i64 buffered = ringHead.load(std::memory_order_acquire) - ringTail.load(std::memory_order_acquire);

    return buffered < target;
}

auto MediaPlayer::showFrame(const AVFrame* const source) -> Result<void, Notice> {
    // The picture fits the label, and is never larger than the video, which is what `rgbBuffer` holds
    const QSize shown = QSize(videoWidth, videoHeight)
                            .scaled(mediaLabel->size(), Qt::KeepAspectRatio)
                            .boundedTo({ videoWidth, videoHeight });
    const i32 shownWidth = max(1, shown.width());
    const i32 shownHeight = max(1, shown.height());

    swscaleContext = sws_getCachedContext(
        swscaleContext,
        source->width,
        source->height,
        AVPixelFormat(source->format),
        shownWidth,
        shownHeight,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );

    if (swscaleContext == nullptr) {
        return Err(NOTICE("sws_getCachedContext failed", Critical, Inline));
    }

    sws_scale(swscaleContext, source->data, source->linesize, 0, source->height, rgbFrame->data, rgbFrame->linesize);

    mediaLabel->setPixmap(
        QPixmap::fromImage(
            QImage(rgbFrame->data[0], shownWidth, shownHeight, rgbFrame->linesize[0], QImage::Format_RGB888)
        )
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
    auto* const self = scast<MediaPlayer*>(device->pUserData);

    if (self->isMidi) {
        self->renderMidi(scast<f32*>(output), frameCount);
        return;
    }

    const usize channels = self->audioCodecContext->ch_layout.nb_channels;
    const u32 bytesNeeded = scast<u32>(frameCount * channels * F32_SIZE);

    u8* const dst = scast<u8*>(output);

    const u32 tail = self->ringTail.load(std::memory_order_relaxed);
    const u32 available = self->ringHead.load(std::memory_order_acquire) - tail;
    const u32 copySize = min(available, bytesNeeded);
    self->audioBytesPlayed.fetch_add(copySize, std::memory_order_relaxed);

    // The ring wraps, so the copy is at most two contiguous runs.
    const u32 start = tail & RING_MASK;
    const u32 firstRun = min(copySize, scast<u32>(RING_CAPACITY) - start);

    memcpy(dst, self->pcmBuffer.data() + start, firstRun);
    memcpy(dst + firstRun, self->pcmBuffer.data(), copySize - firstRun);

    if (copySize < bytesNeeded) {
        memset(dst + copySize, 0, bytesNeeded - copySize);
    }

    self->ringTail.store(tail + copySize, std::memory_order_release);
}

auto MediaPlayer::refillRingBuffer(const usize minBytes) -> bool {
    u32 packets = 0;

    while ((ringHead.load(std::memory_order_relaxed) - ringTail.load(std::memory_order_acquire)) < minBytes) {
        // A file with no audio to find is read to its end over several ticks, not within one
        if (packets++ == MAX_PACKETS_PER_TICK) {
            return true;
        }

        if (av_read_frame(formatContext, packet) < 0) {
            if (avcodec_send_packet(audioCodecContext, nullptr) >= 0) {
                while (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
                    decodeAudio();
                }
            }

            return false;
        }

        if (packet->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(audioCodecContext, packet) >= 0) {
                while (avcodec_receive_frame(audioCodecContext, audioFrame) == 0) {
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

void MediaPlayer::seekSecond(const i32 second) {
    // The device callback touches the ring and decoder, so the device is stopped while they are rewound. Pausing and
    // resuming would start a device that was paused.
    const bool resume = audioDeviceOpen && ma_device_is_started(&device);

    if (resume) {
        ma_device_stop(&device);
    }

    rewindTo(second);

    if (resume) {
        ma_device_start(&device);
    }
}

void MediaPlayer::rewindTo(const i32 second) {
    if (isMidi) {
        // The callback does the seeking, on its own thread, when the device next runs.
        midiSeekMS.store(scast<i64>(second) * SECOND_MS, std::memory_order_release);
        midiDone.store(false, std::memory_order_relaxed);
        audioBytesPlayed.store(second * bytesPerSecond, std::memory_order_relaxed);
        finished = false;
        progressLabel->setText(u"%1/%2"_qsv.arg(toMMSS(second), toMMSS(duration)));
        return;
    }

    const i64 timestamp = second == 0 ? 0
                                      : av_rescale(
                                            second,
                                            isAudioOnly ? audioStream->time_base.den : videoStream->time_base.den,
                                            isAudioOnly ? audioStream->time_base.num : videoStream->time_base.num
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

    audioBytesPlayed.store(second * bytesPerSecond, std::memory_order_relaxed);

    // Frames up to the target are decoded again; the clock drops the ones before it.
    clearFrames();
    demuxEnded = false;
    wallBase = second;
    wallClock.restart();

    ringHead.store(0, std::memory_order_relaxed);
    ringTail.store(0, std::memory_order_relaxed);

    finished = false;

    // With video the tick demuxes, since this would throw the video packets away.
    if (isAudioOnly && audioDeviceOpen) {
        refillRingBuffer(audioTargetBytes());
    }

    progressLabel->setText(u"%1/%2"_qsv.arg(toMMSS(second), toMMSS(duration)));
}
#endif
