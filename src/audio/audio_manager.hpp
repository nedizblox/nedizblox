#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace audio {

class AudioManager {
public:
    struct SoundSettings {
        float pitch{1.0f};
        float volume{1.0f};

        bool loop{false};

        glm::vec3 position{};
    };

    AudioManager();
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    ALuint loadSound(const std::string& wavPath, const SoundSettings& settings);

    void moveListener(const glm::vec3& position);

    void playSound(ALuint id);
    void stopSound(ALuint id);

private:
    ALCdevice* m_device;
    ALCcontext* m_context;

    std::vector<ALuint> m_buffers;
    std::vector<ALuint> m_sources;
};

} // namespace audio