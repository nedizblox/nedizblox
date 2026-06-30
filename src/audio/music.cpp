#include "music.hpp"

#include <stb/stb_vorbis.c>

#include "core/logger.hpp"

#include <vector>
#include <format>

namespace audio {

Music::Music() {
    alGenSources(1, &m_source);
    alGenBuffers(NUM_BUFFERS, m_buffers);

    alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
}

Music::~Music() {
    stop();

    if (m_oggStream) stb_vorbis_close(m_oggStream);

    alDeleteSources(1, &m_source);
    alDeleteBuffers(NUM_BUFFERS, m_buffers);
}

bool Music::open(const std::string& filename) {
    int error;
    m_oggStream = stb_vorbis_open_filename(filename.c_str(), &error, nullptr);
    if (!m_oggStream) {
        core::logger::err(std::format("Failed to open OGG file: {} (Error: {})", filename, error));
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(m_oggStream);
    m_sampleRate = info.sample_rate;
    m_format = (info.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    for (size_t i = 0; i < NUM_BUFFERS; ++i) {
        if (!streamChunk(m_buffers[i])) break;

        alSourceQueueBuffers(m_source, 1, &m_buffers[i]);
    }

    return true;
}

bool Music::streamChunk(ALuint buffer) {
    std::vector<short> pcmData(BUFFER_SIZE / sizeof(short));
    
    int channels = stb_vorbis_get_info(m_oggStream).channels;
    int samplesRead = stb_vorbis_get_samples_short_interleaved(m_oggStream, channels, pcmData.data(), pcmData.size());
    
    int bytesRead = samplesRead * channels * sizeof(short);

    if (bytesRead == 0) {
        if (m_isLooping) {
            stb_vorbis_seek_start(m_oggStream);

            samplesRead = stb_vorbis_get_samples_short_interleaved(m_oggStream, channels, pcmData.data(), pcmData.size());
            bytesRead = samplesRead * channels * sizeof(short);
        } else {
            return false;
        }
    }

    alBufferData(buffer, m_format, pcmData.data(), bytesRead, m_sampleRate);

    return true;
}

void Music::play() {
    alSourcePlay(m_source);
}

void Music::stop() {
    alSourceStop(m_source);
    
    ALint queued;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    while (queued--) {
        ALuint buffer;
        alSourceUnqueueBuffers(m_source, 1, &buffer);
    }
}

void Music::update() {
    ALint processed;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);

    while (processed--) {
        ALuint buffer;
        alSourceUnqueueBuffers(m_source, 1, &buffer);
        
        if (streamChunk(buffer)) {
            alSourceQueueBuffers(m_source, 1, &buffer);
        }
    }

    ALint state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        ALint queued;
        alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
        if (queued > 0) {
            alSourcePlay(m_source);
        }
    }
}

} // namespace audio