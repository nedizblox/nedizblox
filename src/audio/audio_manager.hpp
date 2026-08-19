#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include "sound.hpp"
#include "music.hpp"

#include <string>
#include <unordered_map>
#include <memory>

namespace audio {

class AudioManager {
public:
    struct Properties {
        std::string fileName;

        glm::vec3 position{};
        bool background = true;

        bool loop = false;

        float pitch = 1.0f;
        float volume = 1.0f;
    };
    
    AudioManager();
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void addSound(const std::string& name, const Properties& properties);
    void addMusic(const std::string& name, const Properties& properties);

    void moveListener(const glm::vec3& position, const glm::vec3& at, const glm::vec3& up);

    void changeSoundProperties(const std::string& name, const Properties& properties);
    void changeMusicProperties(const std::string& name, const Properties& properties);

    void update();

private:
    ALCdevice* m_device;
    ALCcontext* m_context;

    std::unordered_map<std::string, std::unique_ptr<Sound>> m_sounds;
    std::unordered_map<std::string, std::unique_ptr<Music>> m_music;
};

} // namespace audio