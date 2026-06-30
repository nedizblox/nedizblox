#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include "sound.hpp"
#include "music.hpp"

#include <string>
#include <unordered_map>
#include <memory>

#include "core/camera.hpp"

namespace audio {

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void addSound(const std::string& name, std::unique_ptr<Sound> sound);
    void addMusic(const std::string& name, std::unique_ptr<Music> sound);

    void moveListener(const core::camera::SphericalCamera& camera);

    const std::unique_ptr<Sound>& getSound(const std::string& name) { return m_sounds[name]; }
    const std::unique_ptr<Music>& getMusic(const std::string& name) { return m_music[name]; }

    void update();

private:
    ALCdevice* m_device;
    ALCcontext* m_context;

    std::unordered_map<std::string, std::unique_ptr<Sound>> m_sounds;
    std::unordered_map<std::string, std::unique_ptr<Music>> m_music;
};

} // namespace audio