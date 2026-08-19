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

void AudioManager::addSound(const std::string& name, const Properties& properties) {
    if (properties.fileName.empty() || m_sounds.contains(name))
        return;

    std::unique_ptr<Sound> sound = std::make_unique<Sound>();
    if (!sound->load(properties.fileName)) {
        throw std::runtime_error("Failed to load sound: " + properties.fileName);
    }

    sound->setPosition(properties.position);
    sound->setBackground(properties.background);

    sound->setLooping(properties.loop);

    sound->setPitch(properties.pitch);
    sound->setVolume(properties.volume);

    m_sounds[name] = std::move(sound);
}

void AudioManager::addMusic(const std::string& name, const Properties& properties) {
    if (properties.fileName.empty() || m_music.contains(name))
        return;

    std::unique_ptr<Music> music = std::make_unique<Music>();
    if (!music->load(properties.fileName)) {
        throw std::runtime_error("Failed to load music: " + properties.fileName);
    }

    music->setPosition(properties.position);
    music->setBackground(properties.background);

    music->setLooping(properties.loop);

    music->setPitch(properties.pitch);
    music->setVolume(properties.volume);

    m_music[name] = std::move(music);
}

void AudioManager::moveListener(const glm::vec3& position, const glm::vec3& at, const glm::vec3& up) {
    alListenerfv(AL_POSITION, &position.x);

    float orientation[6] = {at.x, at.y, at.z, up.x, up.y, up.z};
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioManager::changeSoundProperties(const std::string& name, const Properties& properties) {
    if (!m_sounds.contains(name))
        throw std::runtime_error("Failed to find sound \"" + name + "\"");

    auto& sound = m_sounds[name];

    sound->setPosition(properties.position);
    sound->setBackground(properties.background);

    sound->setLooping(properties.loop);

    sound->setPitch(properties.pitch);
    sound->setVolume(properties.volume);
}

void AudioManager::changeMusicProperties(const std::string& name, const Properties& properties) {
    if (!m_music.contains(name))
        throw std::runtime_error("Failed to find music \"" + name + "\"");

    auto& music = m_music[name];

    music->setPosition(properties.position);
    music->setBackground(properties.background);

    music->setLooping(properties.loop);

    music->setPitch(properties.pitch);
    music->setVolume(properties.volume);
}

void AudioManager::update() {
    for (auto& [name, music] : m_music) {
        music->update();
    }
}

} // namespace audio