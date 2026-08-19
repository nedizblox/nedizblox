#include "music.hpp"

#include <stb/stb_vorbis.c>

#include <vector>

namespace audio {

Music::Music() {
    alGenSources(1, &m_source);
    alGenBuffers(kNumBuffers, m_buffers);

    alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
}

Music::~Music() {
    stop();

    if (m_oggStream) stb_vorbis_close(m_oggStream);

    alDeleteSources(1, &m_source);
    alDeleteBuffers(kNumBuffers, m_buffers);
}

bool Music::load(const std::string& filename) {
    int error;
    m_oggStream = stb_vorbis_open_filename(filename.c_str(), &error, nullptr);
    if (!m_oggStream) {
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(m_oggStream);
    m_sampleRate = info.sample_rate;
    m_format = (info.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    for (size_t i = 0; i < kNumBuffers; ++i) {
        if (!streamChunk(m_buffers[i])) break;

        alSourceQueueBuffers(m_source, 1, &m_buffers[i]);
    }

    return true;
}

void Music::setPosition(const glm::vec3& position) {
    alSourcefv(m_source, AL_POSITION, &position.x);
}

void Music::setBackground(bool background) {
    alSourcei(m_source, AL_SOURCE_RELATIVE, background ? AL_TRUE : AL_FALSE);
}

void Music::setLooping(bool loop) {
    m_isLooping = loop;
}

void Music::setPitch(float pitch) {
    alSourcef(m_source, AL_PITCH, pitch);
}

void Music::setVolume(float volume) {
    alSourcef(m_source, AL_GAIN, volume);   
}

bool Music::streamChunk(ALuint buffer) {
    std::vector<short> pcmData(kBufferSize / sizeof(short));
    
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