#pragma once

#include <AL/al.h>

#include <string>

#include <glm/glm.hpp>

struct stb_vorbis;

namespace audio {

class Music {
public:
    Music();
    ~Music();

    bool load(const std::string& filename);

    void setPosition(const glm::vec3& position);
    void setBackground(bool background);
    
    void setLooping(bool loop);

    void setPitch(float pitch);
    void setVolume(float volume);
    
    void play();
    void stop();
    
    void update(); 

private:
    static constexpr size_t kNumBuffers = 3;
    static constexpr size_t kBufferSize = 40960;

    ALuint m_source = 0;
    ALuint m_buffers[kNumBuffers]{};
    
    stb_vorbis* m_oggStream = nullptr;

    ALenum m_format = 0;
    ALsizei m_sampleRate = 0;

    bool m_isLooping = true;

    bool streamChunk(ALuint buffer);
};

} // namespace audio