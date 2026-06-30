#pragma once

#include <AL/al.h>
#include <string>

struct stb_vorbis;

namespace audio {

class Music {
public:
    Music();
    ~Music();

    bool open(const std::string& filename);
    
    void play();
    void stop();
    
    void update(); 

private:
    static const size_t NUM_BUFFERS = 3;
    static const size_t BUFFER_SIZE = 40960;

    ALuint m_source = 0;
    ALuint m_buffers[NUM_BUFFERS]{};
    
    stb_vorbis* m_oggStream = nullptr;

    ALenum m_format = 0;
    ALsizei m_sampleRate = 0;

    bool m_isLooping = true;

    bool streamChunk(ALuint buffer);
};

} // namespace audio