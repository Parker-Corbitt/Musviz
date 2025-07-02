#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cwchar>
#include <iostream>
#include <vector>
#include <cmath>

const int FRAME_SIZE = 2048;

/**
 * Next Steps
 * 1 - Window the buffer to eliminate the "click" - look at Hann or Hamming
 * 2 - Perform the FFT with cooley tuckey
 * 3 - Prepare data for visualization
 * 4 - Map data to visuals
 */

sf::SoundBuffer mono_conversion(const sf::SoundBuffer &buffer);

std::vector<double> normalize(sf::SoundBuffer &mono_viz);

std::vector<std::vector<double>> frame_split(std::vector<double> mono_viz);

void window_frames(std::vector<std::vector<double>> &frames);

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "Usage: ./musvis <audio_file>" << std::endl;
        exit(-1);
    }

    sf::RenderWindow window(sf::VideoMode({800, 600}), "MusVis");

    sf::SoundBuffer buffer;
    if(!buffer.loadFromFile(argv[1]))
    {
        std::cerr << "Error loading music file" << std::endl;
        return -1;
    }

    sf::SoundBuffer mono = mono_conversion(buffer);
    sf::SoundBuffer mono_viz = mono;

    std::vector<double> viz_samples = normalize(mono_viz);
    std::vector<std::vector<double>> frames = frame_split(viz_samples);
    window_frames(frames);
    

    const auto samples = mono_viz.getSamples();
    auto sampleCount = mono_viz.getSampleCount();
    unsigned int sampleRate = mono_viz.getSampleRate();
    unsigned int channelCount = mono_viz.getChannelCount();

    std::cout << "Samples:\t" << samples << std::endl;
    std::cout << "Sample Count:\t " << sampleCount << std::endl;
    std::cout << "Sample Rate:\t" << sampleRate << std::endl;
    std::cout << "Channel Count\t" << channelCount << std::endl;

    sf::Sound sound(mono);

    //std::cout << buffer.getChannelCount() << std::endl;
    //exit(-1);
    
    sound.play();

    // run the program as long as the window is open
    long lastFrame = -1;
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        while (const std::optional event = window.pollEvent())
        {
            // "close requested" event: we close the window
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        auto currentTime = sound.getPlayingOffset();
        long currentSample = currentTime.asSeconds() * sampleRate * channelCount;
        long currentFrame = currentSample / FRAME_SIZE;

        if(currentFrame > lastFrame)
        {
            std::cout << currentFrame << std::endl;
            lastFrame = currentFrame;
        }
        
        window.clear(sf::Color::Black);

        window.display();
    }
}

sf::SoundBuffer mono_conversion(const sf::SoundBuffer &buffer)
{
    const int16_t* samples = buffer.getSamples();
    std::vector<int16_t> mono_samples;

    for(long x = 0; x < buffer.getSampleCount() / 2; x++)
    {
        mono_samples.push_back((samples[2 * x] + samples[2 * x + 1]) / 2);
    }

    std::vector<sf::SoundChannel> channel_map = {sf::SoundChannel::Mono};
    sf::SoundBuffer mono;

    mono.loadFromSamples(mono_samples.data(), mono_samples.size(), 
                1, buffer.getSampleRate(), channel_map);

    //std::cout << buffer.getSampleCount() << std::endl;
    //std::cout << mono.getSampleCount() << std::endl;
    return mono;
}

std::vector<double> normalize(sf::SoundBuffer &mono_viz)
{
    auto samples = const_cast<int16_t*>(mono_viz.getSamples());
    std::vector<double> viz_samples;

    for (int x = 0; x < mono_viz.getSampleCount(); x++)
    {
        samples[x] = static_cast<double>(samples[x]);
        viz_samples.push_back(samples[x] / 32768.0);
        //std::cout << viz_samples[x] << std::endl;
    }

    return viz_samples;
}

std::vector<std::vector<double>> frame_split(std::vector<double> mono_viz)
{
    std::vector<std::vector<double>> frames;
    std::vector<double> mono_samples;

    //std::cout << mono_viz.size() << std::endl;

    long x = 0;
    while (x < mono_viz.size())
    {
        if ((x + 1) % FRAME_SIZE == 0)
        {
            frames.push_back(mono_samples);
            mono_samples.clear();
        }
        
        mono_samples.push_back(mono_viz[x]);
        x++;
    }

    //std::cout << frames.size() << std::endl;
    

    //Consider adding last non-empty frame here

    return frames;
}

//Hann windowing function, thanks wikipedia
void window_frames(std::vector<std::vector<double>> &frames)
{
    for(int x = 0; x < frames.size(); x++)
        for(int y = 0; y < frames[x].size(); y++)
        {
            double multiplier = 0.5 * (1 - cos(2 * M_PI * y / 2047));
            frames[x][y] = multiplier * frames[x][y];
        }
    
    return;
}
