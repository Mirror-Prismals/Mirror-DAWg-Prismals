// DrumLoop_CLI.cpp
#define NOMINMAX

#include <jack/jack.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <memory>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global timing parameters
const double BPM = 145.0;
const double quarter_duration = 60.0 / BPM;
const double slot_duration = quarter_duration / 2.0; // eighth note

// Token amplitude mapping
std::unordered_map<std::string, float> tokenAmplitude = {
    {"*|", 0.3f},
    {"^|", 0.6f},
    {"v|", 0.15f},
    {"/|", 0.3f},
    {"*|,", 0.3f},
    {"**|", 0.3f}
};

// -------------------------------
// Drum Pattern Parsing
// -------------------------------
struct DrumEvent {
    double start_time;    // seconds
    std::string token;    // e.g., "*|", "^|", etc.
    double slot_duration; // seconds
};

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

std::vector<DrumEvent> parse_measure(const std::string& line, double measure_start_time, double slot_dur) {
    std::vector<DrumEvent> events;
    std::vector<std::pair<std::string, std::vector<std::string>>> tokens;
    std::vector<std::string> parts = split(line);
    for (size_t i = 0; i < parts.size(); i++) {
        std::string token = parts[i];
        if (!token.empty() && token[0] == '{') {
            std::vector<std::string> groupTokens;
            if (token.front() == '{')
                token = token.substr(1);
            if (!token.empty() && token.back() == '}') {
                token.pop_back();
                groupTokens.push_back(token);
            }
            else {
                groupTokens.push_back(token);
                while (++i < parts.size()) {
                    std::string t = parts[i];
                    if (!t.empty() && t.back() == '}') {
                        t.pop_back();
                        groupTokens.push_back(t);
                        break;
                    }
                    else {
                        groupTokens.push_back(t);
                    }
                }
            }
            tokens.push_back({ "group", groupTokens });
        }
        else {
            tokens.push_back({ "token", std::vector<std::string>{token} });
        }
    }
    for (size_t slot_index = 0; slot_index < tokens.size(); slot_index++) {
        double slot_start = measure_start_time + slot_index * slot_dur;
        if (tokens[slot_index].first == "token") {
            std::string tok = tokens[slot_index].second[0];
            if (tok == "_" || tok == "")
                continue;
            if (tokenAmplitude.find(tok) != tokenAmplitude.end()) {
                events.push_back({ slot_start, tok, slot_dur });
            }
        }
        else if (tokens[slot_index].first == "group") {
            std::vector<std::string> group = tokens[slot_index].second;
            size_t num_sub = group.size();
            double sub_duration = slot_dur / num_sub;
            for (size_t sub_index = 0; sub_index < num_sub; sub_index++) {
                std::string sub_tok = group[sub_index];
                double sub_start = slot_start + sub_index * sub_duration;
                if (sub_tok == "_" || sub_tok == "")
                    continue;
                if (tokenAmplitude.find(sub_tok) != tokenAmplitude.end()) {
                    events.push_back({ sub_start, sub_tok, sub_duration });
                }
            }
        }
    }
    return events;
}

std::vector<DrumEvent> parse_pattern(const std::string& pattern_text) {
    std::vector<DrumEvent> all_events;
    std::istringstream iss(pattern_text);
    std::string line;
    double measure_start = 0.0;
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty())
            continue;
        std::vector<std::string> parts = split(line);
        size_t num_slots = 0;
        for (size_t i = 0; i < parts.size(); i++) {
            std::string token = parts[i];
            if (!token.empty() && token[0] == '{') {
                num_slots++;
                while (i < parts.size() && parts[i].back() != '}')
                    i++;
            }
            else {
                num_slots++;
            }
        }
        double measure_duration = num_slots * slot_duration;

        std::vector<DrumEvent> events = parse_measure(line, measure_start, slot_duration);
        all_events.insert(all_events.end(), events.begin(), events.end());
        measure_start += measure_duration;
    }
    return all_events;
}

// -------------------------------
// Impulse Generation
// -------------------------------
std::vector<float> generate_impulse(float amplitude, float duration, int s_rate,
    const std::string& oscillator = "noise", double freq = 440.0) {
    size_t num_samples = static_cast<size_t>(duration * s_rate);
    std::vector<float> impulse(num_samples, 0.0f);
    for (size_t i = 0; i < num_samples; i++) {
        double t = static_cast<double>(i) / s_rate;
        double envelope = std::exp(-30.0 * t);
        double value = 0.0;
        if (oscillator == "noise")
            value = amplitude * (((double)rand() / RAND_MAX) * 2.0 - 1.0) * envelope;
        else if (oscillator == "sine")
            value = amplitude * std::sin(2 * M_PI * freq * t) * envelope;
        impulse[i] = static_cast<float>(value);
    }
    return impulse;
}

// -------------------------------
// Drum Track Structure and Processing
// -------------------------------
struct DrumTrack {
    std::string name;
    std::string pattern;  // multi-line drum notation
    std::string oscillator;
    double freq;
    float impulse_duration;
    std::vector<float> buffer;  // pre-rendered audio
    double duration;
};

void process_track(DrumTrack& track, int s_rate) {
    std::vector<DrumEvent> events = parse_pattern(track.pattern);
    double track_total_duration = 0.0;
    if (!events.empty()) {
        for (const auto& e : events)
            track_total_duration = std::max(track_total_duration, e.start_time + e.slot_duration);
        track_total_duration += track.impulse_duration;
    }
    track.duration = track_total_duration;
    size_t total_samples = static_cast<size_t>(track_total_duration * s_rate);
    track.buffer.assign(total_samples, 0.0f);
    for (const auto& event : events) {
        float amplitude = tokenAmplitude[event.token];
        size_t start_sample = static_cast<size_t>(event.start_time * s_rate);
        if (event.token == "/|") {
            float grace_amp = amplitude * 0.8f;
            std::vector<float> grace_impulse = generate_impulse(grace_amp, track.impulse_duration, s_rate, track.oscillator, track.freq);
            std::vector<float> main_impulse = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            size_t delay_samples = static_cast<size_t>(0.015 * s_rate);
            for (size_t i = 0; i < grace_impulse.size(); i++) {
                if (start_sample + i < track.buffer.size())
                    track.buffer[start_sample + i] += grace_impulse[i];
            }
            size_t main_start = start_sample + delay_samples;
            for (size_t i = 0; i < main_impulse.size(); i++) {
                if (main_start + i < track.buffer.size())
                    track.buffer[main_start + i] += main_impulse[i];
            }
        }
        else if (event.token == "*|,") {
            std::vector<float> impulse1 = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            std::vector<float> impulse2 = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            size_t delay_samples = static_cast<size_t>(0.03 * s_rate);
            for (size_t i = 0; i < impulse1.size(); i++) {
                if (start_sample + i < track.buffer.size())
                    track.buffer[start_sample + i] += impulse1[i];
            }
            size_t second_start = start_sample + delay_samples;
            for (size_t i = 0; i < impulse2.size(); i++) {
                if (second_start + i < track.buffer.size())
                    track.buffer[second_start + i] += impulse2[i];
            }
        }
        else if (event.token == "**|") {
            std::vector<float> impulse1 = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            std::vector<float> impulse2 = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            size_t delay_samples = static_cast<size_t>(0.02 * s_rate);
            for (size_t i = 0; i < impulse1.size(); i++) {
                if (start_sample + i < track.buffer.size())
                    track.buffer[start_sample + i] += impulse1[i];
            }
            size_t second_start = start_sample + delay_samples;
            for (size_t i = 0; i < impulse2.size(); i++) {
                if (second_start + i < track.buffer.size())
                    track.buffer[second_start + i] += impulse2[i];
            }
        }
        else {
            std::vector<float> impulse = generate_impulse(amplitude, track.impulse_duration, s_rate, track.oscillator, track.freq);
            for (size_t i = 0; i < impulse.size(); i++) {
                if (start_sample + i < track.buffer.size())
                    track.buffer[start_sample + i] += impulse[i];
            }
        }
    }
}

std::vector<float> mix_tracks(const std::vector<DrumTrack>& tracks, int s_rate) {
    double final_duration = 0.0;
    for (const auto& track : tracks)
        final_duration = std::max(final_duration, track.duration);
    size_t final_samples = static_cast<size_t>(final_duration * s_rate);
    std::vector<float> final_mix(final_samples, 0.0f);
    for (const auto& track : tracks) {
        for (size_t i = 0; i < track.buffer.size() && i < final_samples; i++)
            final_mix[i] += track.buffer[i];
    }
    float max_val = 0.0f;
    for (size_t i = 0; i < final_mix.size(); i++)
        max_val = std::max(max_val, std::fabs(final_mix[i]));
    if (max_val > 0.0f) {
        float norm_factor = 0.9f / max_val;
        for (size_t i = 0; i < final_mix.size(); i++)
            final_mix[i] *= norm_factor;
    }
    return final_mix;
}

// -------------------------------
// Playback Instance Management
// -------------------------------
struct PlaybackInstance {
    std::shared_ptr<std::vector<float>> samples;
    size_t current_index;
};

std::vector<PlaybackInstance> activePlaybacks;
std::mutex playbackMutex;

// JACK process callback: mix all active playback instances
int process(jack_nframes_t nframes, void* arg) {
    jack_default_audio_sample_t* out = static_cast<jack_default_audio_sample_t*>(
        jack_port_get_buffer(static_cast<jack_port_t*>(arg), nframes));
    for (jack_nframes_t i = 0; i < nframes; i++) {
        float mix = 0.0f;
        {
            std::lock_guard<std::mutex> lock(playbackMutex);
            for (auto it = activePlaybacks.begin(); it != activePlaybacks.end(); ) {
                if (it->current_index < it->samples->size()) {
                    mix += (*it->samples)[it->current_index];
                    it->current_index++;
                    ++it;
                }
                else {
                    // Remove finished playback
                    it = activePlaybacks.erase(it);
                }
            }
        }
        out[i] = mix;
    }
    return 0;
}

// -------------------------------
// Main Function with CLI
// -------------------------------
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

    // Predefine some drum tracks
    std::vector<DrumTrack> tracks;
    tracks.push_back({ "shadow_fang_clap", "_ _ _ _ ^| _ _ _ _ _ _ _ ^| _", "noise", 440.0, 0.25f, {}, 0.0 });
    tracks.push_back({ "shadow_fang_hats",
                       "^| ^| ^| ^| ^| ^| {*| *| *|} ^| ^|\n"
                       "^| ^| ^| ^| {*| *| *| *| *|} ^| ^|\n"
                       "^| ^| ^| ^| ^| ^| {*| *| *|} ^| ^|\n"
                       "^| {*| *| *|} ^| ^| ^| ^| ^|",
                       "noise", 440.0, 0.01f, {}, 0.0 });
    tracks.push_back({ "shadow_fang_808",
                       "*| _ _ *| _ _ ^| _ *|\n"
                       "_ *| _ _ *| *| _ ^| _\n"
                       "_ *| _ *| _ ^| *| _ _",
                       "sine", 400.0, 0.5f, {}, 0.0 });

    int s_rate = 44100;
    for (size_t i = 0; i < tracks.size(); i++)
        process_track(tracks[i], s_rate);

    // Initialize JACK client
    const char* client_name = "DrumLoopCLI";
    jack_client_t* client = jack_client_open(client_name, JackNullOption, nullptr);
    if (!client) {
        std::cerr << "Failed to open JACK client" << std::endl;
        return 1;
    }
    jack_nframes_t jack_sr = jack_get_sample_rate(client);
    if (jack_sr != s_rate) {
        std::cerr << "Warning: JACK sample rate (" << jack_sr
            << ") differs from expected (" << s_rate << ")." << std::endl;
    }
    jack_port_t* output_port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!output_port) {
        std::cerr << "Failed to register JACK output port" << std::endl;
        jack_client_close(client);
        return 1;
    }
    jack_set_process_callback(client, process, output_port);
    if (jack_activate(client)) {
        std::cerr << "Cannot activate JACK client" << std::endl;
        jack_client_close(client);
        return 1;
    }
    std::cout << "JACK client activated." << std::endl;

    // CLI loop: type commands to retrigger playback
    std::cout << "Commands:" << std::endl;
    std::cout << "  all            -> retrigger full mix playback" << std::endl;
    std::cout << "  track <index>  -> retrigger an individual track (0-indexed)" << std::endl;
    std::cout << "  quit           -> exit" << std::endl;

    std::string input;
    std::cout << "\nEnter command: ";
    while (std::getline(std::cin, input)) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        if (command == "quit") {
            break;
        }
        else if (command == "all") {
            std::vector<float> mix = mix_tracks(tracks, s_rate);
            auto mix_ptr = std::make_shared<std::vector<float>>(std::move(mix));
            {
                std::lock_guard<std::mutex> lock(playbackMutex);
                activePlaybacks.push_back({ mix_ptr, 0 });
            }
            std::cout << "Triggered full mix playback." << std::endl;
        }
        else if (command == "track") {
            int index;
            if (iss >> index) {
                if (index >= 0 && index < static_cast<int>(tracks.size())) {
                    // Use the pre-rendered buffer for the track
                    auto track_ptr = std::make_shared<std::vector<float>>(tracks[index].buffer);
                    {
                        std::lock_guard<std::mutex> lock(playbackMutex);
                        activePlaybacks.push_back({ track_ptr, 0 });
                    }
                    std::cout << "Triggered playback for track " << index << " (" << tracks[index].name << ")." << std::endl;
                }
                else {
                    std::cout << "Invalid track index. Use 0 to " << tracks.size() - 1 << "." << std::endl;
                }
            }
            else {
                std::cout << "Usage: track <index>" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command." << std::endl;
        }
        std::cout << "\nEnter command: ";
    }

    // Cleanup: deactivate JACK client and close
    jack_deactivate(client);
    jack_client_close(client);
    std::cout << "Exiting." << std::endl;
    return 0;
}
