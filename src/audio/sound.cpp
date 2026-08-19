#include "sound.hpp"

#include <dr/dr_wav.h>

#include <vector>

namespace audio {

Sound::Sound() {
    alGenSources(1, &m_source);
}

Sound::~Sound() {
    if (m_source) alDeleteSources(1, &m_source);
    if (m_buffer) alDeleteBuffers(1, &m_buffer);
}

bool Sound::load(const std::string& filename) {
    drwav wav;
    if (!drwav_init_file(&wav, filename.c_str(), nullptr)) {
        return false;
    }

    std::vector<drwav_int16> pcmData(wav.totalPCMFrameCount * wav.channels);
    drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pcmData.data());

    ALenum format;
    if (wav.channels == 1) {
        format = AL_FORMAT_MONO16;
    } else {
        format = AL_FORMAT_STEREO16;
    }

    alGenBuffers(1, &m_buffer);
    alBufferData(m_buffer, format, pcmData.data(), pcmData.size() * sizeof(drwav_int16), wav.sampleRate);

    alSourcei(m_source, AL_BUFFER, m_buffer);

    drwav_uninit(&wav);
    return true;
}

void Sound::setPosition(const glm::vec3& position) {
    alSourcefv(m_source, AL_POSITION, &position.x);
}

void Sound::setBackground(bool background) {
    alSourcei(m_source, AL_SOURCE_RELATIVE, background ? AL_TRUE : AL_FALSE);
}

void Sound::setLooping(bool loop) {
    alSourcei(m_source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void Sound::setPitch(float pitch) {
    alSourcef(m_source, AL_PITCH, pitch);
}

void Sound::setVolume(float volume) {
    alSourcef(m_source, AL_GAIN, volume);   
}

void Sound::play() {
    alSourcePlay(m_source);
}

void Sound::stop() {
    alSourceStop(m_source);
}

} // namespace audio