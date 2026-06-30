#include "audio_manager.hpp"

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
    alcDestroyContext(m_context);
    alcCloseDevice(m_device);
}

void AudioManager::addSound(const std::string& name, std::unique_ptr<Sound> sound) {
    m_sounds[name] = std::move(sound);
}

void AudioManager::addMusic(const std::string& name, std::unique_ptr<Music> music) {
    m_music[name] = std::move(music);
}

void AudioManager::moveListener(const core::camera::SphericalCamera& camera) {
    glm::vec3 position = camera.getPosition();

    alListenerfv(AL_POSITION, &position.x);

    glm::vec3 at = glm::normalize(camera.target - position);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float orientation[6] = {at.x, at.y, at.z, up.x, up.y, up.z};
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioManager::update() {
    for (auto& [name, music] : m_music) {
        music->update();
    }
}

} // namespace audio