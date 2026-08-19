#pragma once

#include <AL/al.h>

#include <glm/glm.hpp>

#include <string>

namespace audio {

class Sound {
public:
    Sound();
    ~Sound();

    bool load(const std::string& filename);

    void setPosition(const glm::vec3& position);
    void setBackground(bool background);
    
    void setLooping(bool loop);

    void setPitch(float pitch);
    void setVolume(float volume);

    void play();
    void stop();

private:
    ALuint m_buffer = 0;
    ALuint m_source = 0;
};

} // namespace audio