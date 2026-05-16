#include "audio_manager.hpp"

#include "dr/dr_wav.h"

#include <stdexcept>

namespace audio {

AudioManager::AudioManager() {
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        throw std::runtime_error("OpenAL: Failed to open device");
    }

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context) {
        throw std::runtime_error("OpenAL: Failed to create context");
    }

    alcMakeContextCurrent(m_context);
}

AudioManager::~AudioManager() {
    alDeleteSources(static_cast<ALsizei>(m_sources.size()), m_sources.data());
    alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());

    alcDestroyContext(m_context);
    alcCloseDevice(m_device);
}

ALuint AudioManager::loadSound(const std::string& wavPath, const SoundSettings& settings) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 frameCount;

    short* sampleData = drwav_open_file_and_read_pcm_frames_s16(
        wavPath.c_str(), &channels, &sampleRate, &frameCount, nullptr);
    if (!sampleData) {
        throw std::runtime_error("WAV: Failed to open file at path " + wavPath);
    }

    ALenum format;
    if (channels == 1) {
        format = AL_FORMAT_MONO16;
    } else {
        format = AL_FORMAT_STEREO16;
    }

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, sampleData, frameCount * channels * sizeof(short), sampleRate);

    m_buffers.push_back(buffer);

    drwav_free(sampleData, nullptr);

    ALuint source;
    alGenSources(1, &source);

    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_PITCH, settings.pitch);
    alSourcef(source, AL_GAIN, settings.volume);
    alSourcefv(source, AL_POSITION, &settings.position[0]);
    alSourcei(source, AL_LOOPING, static_cast<int>(settings.loop));

    return buffer;
}

void AudioManager::moveListener(glm::vec3 position) { alListenerfv(AL_POSITION, &position[0]); }

void AudioManager::playSound(ALuint id) { alSourcePlay(id); }

void AudioManager::stopSound(ALuint id) { alSourceStop(id); }

} // namespace audio