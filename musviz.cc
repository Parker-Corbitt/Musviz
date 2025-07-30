#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cerrno>
#include <cwchar>
#include <iostream>
#include <vector>
#include <cmath>
#include <complex>

const int FRAME_SIZE = 2048;

const int NUM_BARS = 32;

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

std::vector<std::vector<std::complex<double>>> convert_for_fft(std::vector<std::vector<double>> frames);

void cooley_tuckey(std::vector<std::complex<double>> &frames, int powTwo);

sf::Color hsvToRgb(float H, float S, float V);

void update_visuals(sf::VertexArray& bars, const std::vector<std::complex<double>>& fft_frame, const sf::Vector2u& window_size);

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "Usage: ./musvis <audio_file>" << std::endl;
        exit(-1);
    }

    sf::RenderWindow window(sf::VideoMode({1920, 1080}), "MusVis");

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

    std::vector<std::vector<std::complex<double>>> fft_frames = convert_for_fft(frames);

    for(int i = 0; i < fft_frames.size() - 1; i++)
    {
        cooley_tuckey(fft_frames[i], 2048);
    }
   
    std::cout << frames.size() << std::endl;
    std::cout << fft_frames.size() << std::endl;

    const auto samples = mono_viz.getSamples();
    auto sampleCount = mono_viz.getSampleCount();
    unsigned int sampleRate = mono_viz.getSampleRate();
    unsigned int channelCount = mono_viz.getChannelCount();

    std::cout << "Samples:\t" << samples << std::endl;
    std::cout << "Sample Count:\t " << sampleCount << std::endl;
    std::cout << "Sample Rate:\t" << sampleRate << std::endl;
    std::cout << "Channel Count\t" << channelCount << std::endl;

    sf::Sound sound(mono);

    sound.play();

    sf::VertexArray bars(sf::PrimitiveType::Lines, NUM_BARS * 2);

    // run the program as long as the window is open
    long lastFrame = -1;
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        while (std::optional event = window.pollEvent())
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
            update_visuals(bars, fft_frames[currentFrame], window.getSize());
            lastFrame = currentFrame;
        }
        
        window.clear(sf::Color::Black);
        window.draw(bars);
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
    }

    return viz_samples;
}

std::vector<std::vector<double>> frame_split(std::vector<double> mono_viz)
{
    std::vector<std::vector<double>> frames;
    std::vector<double> mono_samples;

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

std::vector<std::vector<std::complex<double>>> convert_for_fft(std::vector<std::vector<double>> frames)
{
    std::vector<std::vector<std::complex<double>>> fft_frames;
    std::complex<double> tmp;

    for(int i = 0; i < frames.size(); i++)
    {
        std::vector<std::complex<double>> complex_frame;

        for(int j = 0; j < frames[i].size(); j++)
        {
            tmp.real(frames[i][j]);
            tmp.imag(0.0);
            complex_frame.push_back(tmp);
        }

        fft_frames.push_back(complex_frame);
        complex_frame.clear();
    }

    return fft_frames;
}

//When done recursively, runtime is O(N log N) assuming that N is a power of 2
void cooley_tuckey(std::vector<std::complex<double>> &frames, int powTwo)
{
    if (frames.size() == 1)
        return;
    else
    {
        std::vector<std::complex<double>> eframes;
        std::vector<std::complex<double>> oframes;
        for(int i = 0; i < frames.size(); i++)
        {
            if(i % 2 == 0)
                eframes.push_back(frames[i]);
            else
                oframes.push_back(frames[i]);
        }

        cooley_tuckey(eframes, powTwo / 2);
        cooley_tuckey(oframes, powTwo / 2);

        for (int i = 0; i < (powTwo / 2) - 1; i++)
        {
            std::complex<double> x, y, twiddle;

            x = eframes[i];
            twiddle = std::polar(1.0, -2.0 * M_PI * i / powTwo);
            y = twiddle * oframes[i];

            frames[i] = x + y;
            frames[i + (powTwo / 2)] = x - y;
        }
    }
}

/**
 * @brief Converts HSV(Hue, Saturation, Value) to RGB color.
 * @param H Hue in degrees (0 - 360).
 * @param S Saturation (0.0 - 1.0).
 * @param V Value (0.0 - 1.0).
 * @return sf::Color object representing the RGB color.
 */
sf::Color hsvToRgb(float H, float S, float V) {
    float C = V * S;
    float X = C * (1.0f - std::abs(fmod(H / 60.0f, 2.0f) - 1.0f));
    float m = V - C;
    float r = 0, g = 0, b = 0;

    if (H >= 0 && H < 60) {
        r = C; g = X; b = 0;
    } else if (H >= 60 && H < 120) {
        r = X; g = C; b = 0;
    } else if (H >= 120 && H < 180) {
        r = 0; g = C; b = X;
    } else if (H >= 180 && H < 240) {
        r = 0; g = X; b = C;
    } else if (H >= 240 && H < 300) {
        r = X; g = 0; b = C;
    } else { // H >= 300 && H < 360
        r = C; g = 0; b = X;
    }

    return sf::Color(
        static_cast<uint8_t>((r + m) * 255),
        static_cast<uint8_t>((g + m) * 255),
        static_cast<uint8_t>((b + m) * 255)
    );
}

/**
 * @brief Updates the vertex array to draw the visualizer bars based on FFT results.
 */
void update_visuals(sf::VertexArray& bars, const std::vector<std::complex<double>>& fft_frame, const sf::Vector2u& window_size) {
    float bar_width = static_cast<float>(window_size.x) / NUM_BARS;

    // We only need the first half of the FFT results (due to symmetry)
    int num_bins_to_process = FRAME_SIZE / 2;

    for (int i = 0; i < NUM_BARS; ++i) {
        // --- Map visualization bars to frequency bins ---
        // Logarithmic scaling: makes lower frequencies (bass, mids) more prominent
        int low_freq_bin = static_cast<int>(std::pow(static_cast<float>(num_bins_to_process), static_cast<float>(i) / NUM_BARS));
        int high_freq_bin = static_cast<int>(std::pow(static_cast<float>(num_bins_to_process), static_cast<float>(i + 1) / NUM_BARS));
        if (high_freq_bin <= low_freq_bin) high_freq_bin = low_freq_bin + 1;

        // --- Calculate average magnitude for the bins in this bar ---
        double avg_magnitude = 0.0;
        for (int j = low_freq_bin; j < high_freq_bin && j < num_bins_to_process; ++j) {
            avg_magnitude += std::abs(fft_frame[j]);
        }
        avg_magnitude /= (high_freq_bin - low_freq_bin);

        // --- Convert magnitude to a logarithmic scale (decibels-like) ---
        // This makes the visualization more responsive to both loud and quiet sounds.
        // The values are tweaked for good visual results.
        float bar_height = 20.0f * std::log10(avg_magnitude * 100.0f + 1.0f);
        bar_height = std::max(0.f, bar_height) * 10.f; // Clamp at 0 and scale up
        if (bar_height > window_size.y) bar_height = window_size.y;

        // --- Set vertex positions ---
        float x_pos = i * bar_width;
        bars[i * 2].position = {x_pos, static_cast<float>(window_size.y)}; // Bottom of the bar
        bars[i * 2 + 1].position = {x_pos, window_size.y - bar_height}; // Top of the bar

        // --- Set vertex colors for a nice gradient ---
        float hue = static_cast<float>(i) / NUM_BARS * 360.f;
        // **FIX**: Use our custom hsvToRgb function for backward compatibility
        sf::Color color = hsvToRgb(hue, 1.f, 1.f); 
        bars[i * 2].color = sf::Color(color.r/2, color.g/2, color.b/2); // Darker at the bottom
        bars[i * 2 + 1].color = color; // Brighter at the top
    }
}
