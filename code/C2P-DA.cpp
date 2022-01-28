//#define SPEED_TEST

#pragma region Notes
// Legend:
// [G]: General
// [C]: Convention
//
// Notes:
// [G] Structures have been instantiated so that allocation is contiguous, when possible
// [C] Prefix '_' indicates a local variable
// [C] Prefix 'p_' indicates a parameter
#pragma endregion

#pragma region Includes
#include <windows.h>
#undef max
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <map>
#include <algorithm>
#include <future>
#include <unordered_map>
#include <filesystem>
#include <deque>
#include "input.h"
#include "utils.h"
#pragma endregion

#pragma region Configuration
#define JSON_PATH ./config.json
#define EXTERNAL_MULTITHREADING std::thread::hardware_concurrency()
#define __STRINGIFY(x) #x
#define MY_STRINGIFY(x) __STRINGIFY(x)
#pragma endregion

int done{ 0 };
bool go{ false };
std::mutex ext_m{};
std::condition_variable ext_cv{};
std::condition_variable int_cv{};

void external_thread(int, int, int, int, int, int, std::vector<float>&, std::vector<std::vector<std::tuple<int, int, float>>>&, std::unordered_map<std::string, int>&, Config& p_config, int);

int main()
{
    try {

        #pragma region External preparation
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS); // Setting real-time process priority 
        std::cout << "C2P-DA" << std::endl;
        Config _config{ std::string{ MY_STRINGIFY(JSON_PATH) } };
        // Putting program to sleep until...
        tm timeout_tm{};
        timeout_tm.tm_year = std::stoi(_config.year) - 1900;
        timeout_tm.tm_mon = std::stoi(_config.month) - 1;
        timeout_tm.tm_mday = std::stoi(_config.day);
        timeout_tm.tm_hour = std::stoi(_config.hours);
        timeout_tm.tm_min = std::stoi(_config.minutes);
        timeout_tm.tm_sec = std::stoi(_config.seconds);
        timeout_tm.tm_isdst = -1;
        time_t _timeout_time_t{};
        try {
            _timeout_time_t = mktime(&timeout_tm);
        }
        catch (...) {
            throw std::logic_error("Logic Error: The algorithm schedule date is not valid.");
        }
        std::stringstream _ss{};
        if (difftime(std::time(0), _timeout_time_t) < 0) {
            auto _timeout_tp = std::chrono::system_clock::from_time_t(mktime(&timeout_tm));
            _ss << std::put_time(std::localtime(&_timeout_time_t), "%F %T");
            std::cout << "Execution starting on " << _ss.str() << "\n\n" << std::endl;
            std::this_thread::sleep_until(_timeout_tp);
        }
        #pragma endregion

        int _fileno{ 0 };
        int _total_files{ static_cast<int>(std::distance(std::filesystem::directory_iterator(_config.data_path), std::filesystem::directory_iterator{})) };
        for (auto const& file : std::filesystem::directory_iterator(_config.data_path)) {

            #pragma region Internal preparation
            std::string _logfilename{ std::string{ _config.logs_path } + std::string{ _config.filename } + std::string{ "___" } + std::to_string(++_fileno) + std::string{ ".txt" } };
            std::string _ratesfilename{ std::string{ _config.rates_path } + std::string{ _config.filename } + std::string{ "___" } + std::to_string(_fileno) + std::string{ "_rates.txt" } };
            // Logging start
            auto _now = std::time(0);
            _ss.str("");
            _ss << std::put_time(std::localtime(&_now), "%F %T");
            std::cout << "File " << _fileno << " of " << _total_files << " execution started on " << _ss.str() << std::endl;
            std::ofstream _log_file{ _logfilename };
            _log_file << "File " << _fileno << " of " << _total_files << " execution started on " << _ss.str() << '\n' << std::endl;
            _log_file.close();
            // Populating data structure
            std::vector<float> _ask_signal{}; // Data structure that will contain the main signal
            if (_config.dukascopy_stock_input_data)
                duka_populate(_ask_signal, file.path().u8string(), _logfilename);
            else
                populate(_ask_signal, file.path().u8string(), _logfilename);
            // Declaration of other data structures
            std::vector<std::vector<std::tuple<int, int, float>>> _patterns(_config.number_of_subwindows); // Final container for patterns
            std::unordered_map<std::string, int> _references{}; // Data structure that prevents redundancy in patterns
            std::vector<float> _rates(_ask_signal.size() / 1000); // Data structure used to keep track of processing times throughout the execution
            auto _start_4rate = std::chrono::high_resolution_clock::now(); // Variable used to compute processing times throughout the execution
            #pragma endregion

            if (_ask_signal.size() >= static_cast<uint64_t>(_config.max_subwindow) * 2) {
                std::cout << "File " << _fileno << " of " << _total_files << " pattern detection: 0.00% (0/" << _ask_signal.size() / 1000 << ")";

                #pragma region Thread Creation
                std::vector<std::thread> _threads{};
                for (unsigned int i = 0; i < EXTERNAL_MULTITHREADING; i++)
                    _threads.emplace_back(std::thread{ external_thread,
                        EXTERNAL_MULTITHREADING,
                        EXTERNAL_MULTITHREADING * _config.difference_between_subwindows,
                        !_config.approximation_enabled && _config.number_of_subwindows == 1 ? _config.min_subwindow : _config.min_subwindow + i * _config.difference_between_subwindows,
                        !_config.approximation_enabled && _config.number_of_subwindows == 1 ? _config.max_subwindow : _config.min_subwindow + i * _config.difference_between_subwindows + ((_config.number_of_subwindows + EXTERNAL_MULTITHREADING - 1 - i) / EXTERNAL_MULTITHREADING - 1) * _config.difference_between_subwindows * EXTERNAL_MULTITHREADING,
                        !_config.approximation_enabled && _config.number_of_subwindows == 1 ? 1 : (_config.number_of_subwindows + EXTERNAL_MULTITHREADING - 1 - i) / EXTERNAL_MULTITHREADING,
                        _config.beta,
                        std::ref(_ask_signal),
                        std::ref(_patterns),
                        std::ref(_references),
                        std::ref(_config),
                        i });
                #pragma endregion

                #pragma region Main thread cycle
                std::unique_lock<std::mutex> _ul{ ext_m };
                int _checkpoint{ 0 };
                while (true) {
                    ext_cv.wait(_ul, [] { return !go && done == EXTERNAL_MULTITHREADING; });
                    _checkpoint += 1;
                    if (_checkpoint * 1000 >= static_cast<int>(_ask_signal.size())) {
                        std::cout << "\rFile " << _fileno << " of " << _total_files << " pattern detection: 100.00% (" << _ask_signal.size() / 1000 << "/" << _ask_signal.size() / 1000 << ")" << std::endl;
                        done = 0;
                        break;
                    }
                    std::cout << "\rFile " << _fileno << " of " << _total_files << " pattern detection: " << std::fixed << std::setprecision(2) << 100 * static_cast<float>(_checkpoint) / (_ask_signal.size() / 1000) << "% (" << _checkpoint << "/" << _ask_signal.size() / 1000 << ")";
                    _rates[static_cast<uint64_t>(_checkpoint) - 1] = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start_4rate).count() / 10e5);
                    _start_4rate = std::chrono::high_resolution_clock::now();
                    go = true;
                    int_cv.notify_all();
                }
                #pragma endregion

                #pragma region Final statements
                _log_file = std::ofstream{ _logfilename, std::ios_base::app };
                #ifndef SPEED_TEST
                for (int i = 0; i < _config.number_of_subwindows; i++)
                    if (_patterns[i].size()) {
                        if (_config.number_of_subwindows == 1)
                            std::sort(_patterns[0].begin(), _patterns[0].end(), [](std::tuple<int, int, float> x, std::tuple<int, int, float> y) { return std::get<1>(x) < std::get<1>(y); });
                        _log_file << "The following " << _patterns[i].size() << " consecutive pattern(s) have been detected (subwindow width = " << _config.min_subwindow + i * _config.difference_between_subwindows << "):" << std::endl;
                        for (int j = 0; j < _patterns[i].size(); j++)
                            _log_file << "Pattern 1: [" << std::get<0>(_patterns[i][j]) - (_config.min_subwindow + i * _config.difference_between_subwindows) + 1 << ", " << std::get<0>(_patterns[i][j]) << "]\tPattern 2: [" << std::get<1>(_patterns[i][j]) - (_config.min_subwindow + i * _config.difference_between_subwindows) + 1 << ", " << std::get<1>(_patterns[i][j]) << (_config.approximation_enabled ? "]\twith conf = " : "]\twith distance = ") << std::fixed << std::setprecision(2) << std::get<2>(_patterns[i][j]) << "\n";
                        _log_file << std::endl;
                    }
                std::ofstream _rates_file{ _ratesfilename };
                for (int i = 0; i < _rates.size(); i++)
                    _rates_file << "[" << i + 1 << "] " << _rates[i] << std::endl;
                _rates_file.close();
                #endif
                _now = std::time(0);
                _ss.str("");
                _ss << std::put_time(std::localtime(&_now), "%F %T");
                std::cout << "File " << _fileno << " of " << _total_files << " execution ended on " << _ss.str() << "\n" << std::endl;
                _log_file << "File " << _fileno << " of " << _total_files << " execution ended on " << _ss.str() << "\n" << std::endl;
                _log_file.close();      
                for (auto& thread : _threads)
                    thread.join();
                _ul.unlock();
                #pragma endregion

            }
            else {
                std::cout << "File " << _fileno << " of " << _total_files << " pattern detection: 100.00% (" << _ask_signal.size() / 1000 << "/" << _ask_signal.size() / 1000 << ")" << std::endl;
                _log_file = std::ofstream{ _logfilename, std::ios_base::app };
                #ifndef SPEED_TEST
                std::ofstream _rates_file{ _ratesfilename };
                _rates_file.close();
                #endif              
                std::cout << "File " << _fileno << " of " << _total_files << " execution ended on " << _ss.str() << "\n" << std::endl;
                _log_file << "File " << _fileno << " of " << _total_files << " execution ended on " << _ss.str() << "\n" << std::endl;
                _log_file.close();
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << std::endl << e.what() << std::endl << std::fflush;
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

void external_thread(int p_siblings, int p_difference_between_subwindows, int p_min_subwindow, int p_max_subwindow, int p_number_of_subwindows, int p_beta, std::vector<float>& p_ask_signal, std::vector<std::vector<std::tuple<int, int, float>>>& p_patterns, std::unordered_map<std::string, int>& p_references, Config& p_config, int p_whoami) {
    try {

        #pragma region Preparation
        std::vector<int> _subwindows_set(p_number_of_subwindows); // Set of subwindows that will be considered throughout the code
        std::vector<std::vector<int>> _queue(p_ask_signal.size()); // Booking data structure
        if (!p_config.approximation_enabled && p_config.number_of_subwindows == 1)
            _queue[static_cast<uint64_t>(p_min_subwindow) - 1 + p_whoami].push_back(p_min_subwindow);
        else {
            int _accumulator{ p_min_subwindow };
            for (auto& value : _subwindows_set) {
                value = _accumulator;
                _queue[static_cast<uint64_t>(_accumulator) - 1].push_back(_accumulator);
                _accumulator += p_difference_between_subwindows;
            }
        }
        std::vector<TempPatterns> _temp_patterns(p_number_of_subwindows); // Temporary container for patterns
        std::vector<JunkCoefficientData> _junk_coefficient_data(p_number_of_subwindows); // Data structure that stores auxiliary data used for creating TempPatterns objects
        std::vector<Point> _abs_maxs(p_number_of_subwindows); // Data structure containing absolute maxima for each sliding subwindow
        std::vector<Point> _abs_mins(p_number_of_subwindows, Point{ true }); // Data structure containing absolute minima for each sliding subwindow
        Point _local_abs_max{ 0, p_ask_signal[0] }; // Local absolute maximum: useful for computing subwindows' maxima dynamically
        Point _local_abs_min{ 0, p_ask_signal[0] }; // Local absolute minimum: useful for computing subwindows' maxima dynamically
        int _min_expiration_M{ -1 }; // Minimum expiration for maxima
        int _min_expiration_m{ -1 }; // Minimum expiration for minima
        auto _last_value = p_ask_signal[0]; // Last value read from the main signal
        float _medium{ -1 };
        bool _asc{ false }; // This variable (together with the ones below) is needed to handle peak widths
        bool _desc{ false };
        bool _const{ false };
        bool _go_peak{ true };
        int _current_label{ -1 };
        std::vector<std::pair<float, LabeledPointOnAxis>> _values{};
        std::map<float, LabeledPointOnAxis> _values_const{};
        std::map<int, std::map<int, float>, std::greater<int>> _peaks{};
        std::map<int, std::pair<int, float>> _labels{};
        bool _neg_asc{ false };
        bool _neg_desc{ false };
        bool _neg_const{ false };
        bool _neg_go_peak{ true };
        int _neg_current_label{ -1 };
        std::vector<std::pair<float, LabeledPointOnAxis>> _neg_values{};
        std::map<float, LabeledPointOnAxis> _neg_values_const{};
        std::map<int, std::map<int, float>, std::greater<int>> _neg_peaks{};
        std::map<int, std::pair<int, float>> _neg_labels{};
        std::deque<std::tuple<float, int, float>> _s_values{};
        if (!p_config.approximation_enabled)
            _s_values.push_front(std::tuple<float, int, float>{ float{ 0 }, 0, _last_value });
        int _next_t{ 2 };
        auto _bookers = _queue[static_cast<uint64_t>(_next_t) - 1];
        #pragma endregion

        while (true) {

            #pragma region Updating local maximum and minimum (A1)
            auto _current_value = p_ask_signal[static_cast<uint64_t>(_next_t) - 1];
            if (_current_value >= _local_abs_max.value) // The '=' allows us to update maxima with the most recent value
                _local_abs_max.update(_next_t - 1, _current_value);
            if (_current_value <= _local_abs_min.value) // the '=' allows us to update minima with the most recent value
                _local_abs_min.update(_next_t - 1, _current_value);
            #pragma endregion

            #pragma region Updating global peak widths or stationary values (B1)
            if (p_config.approximation_enabled) {
                if (_current_value == _last_value) { // Constant values
                    // For both peaks and negative peaks
                    if (!_go_peak)
                        _values_const[_current_value] = LabeledPointOnAxis{ _next_t - 1, _current_label };
                    if (!_neg_go_peak)
                        _neg_values_const[_current_value] = LabeledPointOnAxis{ _next_t - 1, _neg_current_label };
                    if (!_const) {
                        _medium = static_cast<float>(_next_t) - 2;
                        _asc = false;
                        _neg_desc = false;
                        _desc = false;
                        _neg_asc = false;
                        _const = true;
                        _neg_const = true;
                    }
                    _medium += 0.5;
                }
                else if (_current_value > _last_value) { // Ascending values
                    // For peaks
                    if (!_asc) {
                        if (_go_peak) {
                            _current_label++;
                            _go_peak = false;
                            _values.push_back(std::pair<float, LabeledPointOnAxis>{ _last_value, LabeledPointOnAxis{ _next_t - 2, _current_label } });
                        }
                        _asc = true;
                        _desc = false;
                        _const = false;
                    }
                    _values.push_back(std::pair<float, LabeledPointOnAxis>{ _current_value, LabeledPointOnAxis{ _next_t - 1, _current_label } });
                    // For negative peaks
                    if (!_neg_desc) {
                        if (!_neg_go_peak) { // New negative peak must be created and labels must be checked
                            _neg_labels[_neg_current_label] = std::pair<int, float>{ !_neg_const ? _next_t - 2 : static_cast<int>(floor(_medium)), _last_value };
                            _neg_peaks[!_neg_const ? _next_t - 2 : static_cast<int>(floor(_medium))] = std::map<int, float>{};
                            auto _item = _neg_values.back();
                            _neg_values.pop_back();
                            auto it = _neg_values_const.find(_item.first);
                            if (it != _neg_values_const.end())
                                _neg_values_const.erase(it->first);
                            _neg_go_peak = true;
                            int cnt{ 0 };
                            for (auto it = _neg_values.rbegin(); it != _neg_values.rend(); it++) {
                                cnt++;
                                auto _first_desc_labels = it->second.labels;
                                if (_first_desc_labels.size() > 1 || _first_desc_labels[0] != _neg_current_label) {
                                    while (_last_value < _neg_labels[_first_desc_labels[0]].second) {
                                        it->second.labels = std::vector<int>{ _neg_current_label };
                                        auto it2 = _neg_values_const.find(it->first);
                                        if (it2 != _neg_values_const.end())
                                            it2->second.labels = std::vector<int>{ _neg_current_label };
                                        if (cnt == _neg_values.size())
                                            break;
                                        it++;
                                        cnt++;
                                        _first_desc_labels = it->second.labels;
                                    }
                                    while (_last_value == _neg_labels[_first_desc_labels[0]].second) {
                                        it->second.labels.push_back(_neg_current_label);
                                        auto it2 = _neg_values_const.find(it->first);
                                        if (it2 != _neg_values_const.end())
                                            it2->second.labels.push_back(_neg_current_label);
                                        if (cnt == _neg_values.size())
                                            break;
                                        it++;
                                        cnt++;
                                        _first_desc_labels = it->second.labels;
                                    }
                                    break;
                                }
                            }
                        }
                        _neg_asc = false;
                        _neg_desc = true;
                        _neg_const = false;
                    }
                    bool _stop{ false };
                    for (auto it = _neg_values.rbegin(); it != _neg_values.rend();)
                        if (it->first <= _current_value) {
                            if (it->first == _current_value)
                                _stop = true;
                            auto _item = _neg_values.back();
                            _neg_values.pop_back();
                            it = _neg_values.rbegin();
                            auto _pos = _item.second.position;
                            auto _post_pos = _pos;
                            auto _labs = _item.second.labels;
                            auto it2 = _neg_values_const.find(_item.first);
                            if (it2 != _neg_values_const.end()) {
                                auto _item_const = it2->second;
                                _neg_values_const.erase(it2->first);
                                _post_pos = _item_const.position;
                                for (auto const q : _item_const.labels) {
                                    auto _neg_pk = _neg_labels[q].first;
                                    _neg_peaks[_neg_pk][_item_const.position] = (_item.first - _last_value) / (_current_value - _last_value) + _next_t - 2 - _post_pos;
                                }
                            }
                            for (auto const q : _labs) {
                                auto _neg_pk = _neg_labels[q].first;
                                _neg_peaks[_neg_pk][_pos] = (_item.first - _last_value) / (_current_value - _last_value) + _next_t - 2 - _post_pos;
                            }
                            if (_item.first < _current_value && _neg_values.size() > 0) {
                                auto _rival = _neg_values.back();
                                _labs = _rival.second.labels;
                                if (_labs.size() > 1 && std::count(_labs.begin(), _labs.end(), _neg_current_label)) {
                                    _pos = _rival.second.position;
                                    _post_pos = _pos;
                                    it2 = _neg_values_const.find(_rival.first);
                                    if (it2 != _neg_values_const.end()) {
                                        auto _item_const = it2->second;
                                        _post_pos = _item_const.position;
                                        for (auto const q : _item_const.labels) {
                                            auto _neg_pk = _neg_labels[q].first;
                                            auto _val = _neg_labels[q].second;
                                            _neg_peaks[_neg_pk][_item_const.position] = _next_t - 1 - (_post_pos + (_current_value - _rival.first) / (_val - _rival.first));
                                        }
                                    }
                                    for (auto const q : _labs) {
                                        auto _neg_pk = _neg_labels[q].first;
                                        auto _val = _neg_labels[q].second;
                                        _neg_peaks[_neg_pk][_pos] = _next_t - 1 - (_post_pos + (_current_value - _rival.first) / (_val - _rival.first));
                                    }
                                }
                            }
                        }
                        else {
                            if (!_stop) {
                                auto _pos = it->second.position;
                                auto _post_pos = _pos;
                                auto _labs = it->second.labels;
                                auto it2 = _neg_values_const.find(it->first);
                                if (it2 != _neg_values_const.end()) {
                                    auto _item_const = it2->second;
                                    _post_pos = _item_const.position;
                                    for (auto const q : _item_const.labels) {
                                        auto _neg_pk = _neg_labels[q].first;
                                        _neg_peaks[_neg_pk][_item_const.position] = _next_t - 1 - (_post_pos + (_current_value - it->first) / (_last_value - it->first));
                                    }
                                }
                                for (auto const q : _labs) {
                                    auto _neg_pk = _neg_labels[q].first;
                                    _neg_peaks[_neg_pk][_pos] = _next_t - 1 - (_post_pos + (_current_value - it->first) / (_last_value - it->first));
                                }
                            }
                            break;
                        }
                }
                else { // Descending values
                    // For peaks
                    if (!_desc) {
                        if (!_go_peak) { // New peak must be created and labels must be checked
                            _labels[_current_label] = std::pair<int, float>{ !_const ? _next_t - 2 : static_cast<int>(floor(_medium)), _last_value };
                            _peaks[!_const ? _next_t - 2 : static_cast<int>(floor(_medium))] = std::map<int, float>{};
                            auto _item = _values.back();
                            _values.pop_back();
                            auto it = _values_const.find(_item.first);
                            if (it != _values_const.end())
                                _values_const.erase(it->first);
                            _go_peak = true;
                            int cnt{ 0 };
                            for (auto it = _values.rbegin(); it != _values.rend(); it++) {
                                cnt++;
                                auto _first_asc_labels = it->second.labels;
                                if (_first_asc_labels.size() > 1 || _first_asc_labels[0] != _current_label) {
                                    while (_last_value > _labels[_first_asc_labels[0]].second) {
                                        it->second.labels = std::vector<int>{ _current_label };
                                        auto it2 = _values_const.find(it->first);
                                        if (it2 != _values_const.end())
                                            it2->second.labels = std::vector<int>{ _current_label };
                                        if (cnt == _values.size())
                                            break;
                                        it++;
                                        cnt++;
                                        _first_asc_labels = it->second.labels;
                                    }
                                    while (_last_value == _labels[_first_asc_labels[0]].second) {
                                        it->second.labels.push_back(_current_label);
                                        auto it2 = _values_const.find(it->first);
                                        if (it2 != _values_const.end())
                                            it2->second.labels.push_back(_current_label);
                                        if (cnt == _values.size())
                                            break;
                                        it++;
                                        cnt++;
                                        _first_asc_labels = it->second.labels;
                                    }
                                    break;
                                }
                            }
                        }
                        _asc = false;
                        _desc = true;
                        _const = false;
                    }
                    bool _stop{ false };
                    for (auto it = _values.rbegin(); it != _values.rend();)
                        if (it->first >= _current_value) {
                            if (it->first == _current_value)
                                _stop = true;
                            auto _item = _values.back();
                            _values.pop_back();
                            it = _values.rbegin();
                            auto _pos = _item.second.position;
                            auto _post_pos = _pos;
                            auto _labs = _item.second.labels;
                            auto it2 = _values_const.find(_item.first);
                            if (it2 != _values_const.end()) {
                                auto _item_const = it2->second;
                                _values_const.erase(it2->first);
                                _post_pos = _item_const.position;
                                for (auto const q : _item_const.labels) {
                                    auto _pk = _labels[q].first;
                                    _peaks[_pk][_item_const.position] = (_item.first - _last_value) / (_current_value - _last_value) + _next_t - 2 - _post_pos;
                                }
                            }
                            for (auto const q : _labs) {
                                auto _pk = _labels[q].first;
                                _peaks[_pk][_pos] = (_item.first - _last_value) / (_current_value - _last_value) + _next_t - 2 - _post_pos;
                            }
                            if (_item.first > _current_value && _values.size() > 0) {
                                auto _rival = _values.back();
                                _labs = _rival.second.labels;
                                if (_labs.size() > 1 && std::count(_labs.begin(), _labs.end(), _current_label)) {
                                    _pos = _rival.second.position;
                                    _post_pos = _pos;
                                    it2 = _values_const.find(_rival.first);
                                    if (it2 != _values_const.end()) {
                                        auto _item_const = it2->second;
                                        _post_pos = _item_const.position;
                                        for (auto const q : _item_const.labels) {
                                            auto _pk = _labels[q].first;
                                            auto _val = _labels[q].second;
                                            _peaks[_pk][_item_const.position] = _next_t - 1 - (_post_pos + (_current_value - _rival.first) / (_val - _rival.first));
                                        }
                                    }
                                    for (auto const q : _labs) {
                                        auto _pk = _labels[q].first;
                                        auto _val = _labels[q].second;
                                        _peaks[_pk][_pos] = _next_t - 1 - (_post_pos + (_current_value - _rival.first) / (_val - _rival.first));
                                    }
                                }
                            }
                        }
                        else {
                            if (!_stop) {
                                auto _pos = it->second.position;
                                auto _post_pos = _pos;
                                auto _labs = it->second.labels;
                                auto it2 = _values_const.find(it->first);
                                if (it2 != _values_const.end()) {
                                    auto _item_const = it2->second;
                                    _post_pos = _item_const.position;
                                    for (auto const q : _item_const.labels) {
                                        auto _pk = _labels[q].first;
                                        _peaks[_pk][_item_const.position] = _next_t - 1 - (_post_pos + (_current_value - it->first) / (_last_value - it->first));
                                    }
                                }
                                for (auto const q : _labs) {
                                    auto _pk = _labels[q].first;
                                    _peaks[_pk][_pos] = _next_t - 1 - (_post_pos + (_current_value - it->first) / (_last_value - it->first));
                                }
                            }
                            break;
                        }
                    // For negative peaks
                    if (!_neg_asc) {
                        if (_neg_go_peak) {
                            _neg_current_label++;
                            _neg_go_peak = false;
                            _neg_values.push_back(std::pair<float, LabeledPointOnAxis>{ _last_value, LabeledPointOnAxis{ _next_t - 2, _neg_current_label } });
                        }
                        _neg_asc = true;
                        _neg_desc = false;
                        _neg_const = false;
                    }
                    _neg_values.push_back(std::pair<float, LabeledPointOnAxis>{ _current_value, LabeledPointOnAxis{ _next_t - 1, _neg_current_label } });
                }
            }
            else if (_current_value == _last_value) {
                std::get<0>(_s_values.front()) += 0.5;
                std::get<1>(_s_values.front()) += 1;
            }
            else
                _s_values.push_front(std::tuple<float, int, float>{ static_cast<float>(_next_t) - 1, 0, _current_value });
            _last_value = _current_value;
            #pragma endregion

            if (_bookers.size()) {

                #pragma region Updating global maxima and minima (A2)
                bool _flag_M{ false };
                bool _flag_m{ false };
                // Maxima upwards update
                auto _upwards_visiter_4max = p_min_subwindow;
                if (_local_abs_max.value >= _abs_maxs[(_upwards_visiter_4max - p_min_subwindow) / p_difference_between_subwindows].value) { // The '=' allows us to update maxima with the most recent value
                    _flag_M = true;
                    _min_expiration_M = _local_abs_max.position - (_next_t - _upwards_visiter_4max) + 1; // Minimum expiration of maxima is updated: + 1 because it gets decremented later
                    _abs_maxs[(_upwards_visiter_4max - p_min_subwindow) / p_difference_between_subwindows] = _local_abs_max;
                    _upwards_visiter_4max += p_difference_between_subwindows;
                    while (_upwards_visiter_4max <= p_max_subwindow && _local_abs_max.value >= _abs_maxs[(_upwards_visiter_4max - p_min_subwindow) / p_difference_between_subwindows].value) {
                        _abs_maxs[(_upwards_visiter_4max - p_min_subwindow) / p_difference_between_subwindows] = _local_abs_max;
                        _upwards_visiter_4max += p_difference_between_subwindows;
                    }
                }
                // Minima upwards update
                auto _upwards_visiter_4min = p_min_subwindow;
                if (_local_abs_min.value <= _abs_mins[(_upwards_visiter_4min - p_min_subwindow) / p_difference_between_subwindows].value) { // The '=' allows us to update minima with the most recent value
                    _flag_m = true;
                    _min_expiration_m = _local_abs_min.position - (_next_t - _upwards_visiter_4min) + 1; // Minimum expiration of minima is updated: + 1 because it gets decremented later
                    _abs_mins[(_upwards_visiter_4min - p_min_subwindow) / p_difference_between_subwindows] = _local_abs_min;
                    _upwards_visiter_4min += p_difference_between_subwindows;
                    while (_upwards_visiter_4min <= p_max_subwindow && _local_abs_min.value <= _abs_mins[(_upwards_visiter_4min - p_min_subwindow) / p_difference_between_subwindows].value) {
                        _abs_mins[(_upwards_visiter_4min - p_min_subwindow) / p_difference_between_subwindows] = _local_abs_min;
                        _upwards_visiter_4min += p_difference_between_subwindows;
                    }
                }
                _local_abs_max = Point{};
                _local_abs_min = Point{ true };
                // Invalid maxima downwards check
                if (_min_expiration_M <= 0 || _flag_M) { // If(maximum with smallest expiration expired || one or more maxima have been updated)
                    if (_min_expiration_M <= 0) // Maximum with smallest expiration expired
                        _min_expiration_M = std::numeric_limits<int>::max(); // We need to perform this operation beforehand in order to pair up with situation in which flag_M = true
                    auto _invalid_count = 0;
                    auto _downwards_visiter = p_max_subwindow;
                    while (_downwards_visiter >= _upwards_visiter_4max) { // Then check for invalid elements downwards until needed
                        if (_abs_maxs[(_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows].position < _next_t - _downwards_visiter) // Invalid
                            _invalid_count++;
                        else {
                            if (_invalid_count > 0) { // Call my_max() in order to calculate invalid subwindows' maxima
                                auto _recheck = _invalid_count * p_difference_between_subwindows;
                                _min_expiration_M = my_max(_next_t - _downwards_visiter - _recheck, _downwards_visiter, _recheck, false, _min_expiration_M, _abs_maxs, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                                _invalid_count = 0;
                            }
                            auto _expiration = _abs_maxs[(_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows].position - (_next_t - _downwards_visiter) + 1;
                            if (_expiration < _min_expiration_M)
                                _min_expiration_M = _expiration;
                        }
                        _downwards_visiter -= p_difference_between_subwindows;
                    }
                    if (_invalid_count > 0) // It is possible that the cycle ends with invalid subwindows
                        if (_downwards_visiter < p_min_subwindow) { // Smallest subwindow's maximum is invalid: exceptional procedure in my_max()
                            auto _recheck = p_min_subwindow + (_invalid_count - 1) * p_difference_between_subwindows;
                            _min_expiration_M = my_max(_next_t - _recheck, _downwards_visiter, _recheck, true, _min_expiration_M, _abs_maxs, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                        }
                        else {
                            auto _recheck = _invalid_count * p_difference_between_subwindows;
                            _min_expiration_M = my_max(_next_t - _downwards_visiter - _recheck, _downwards_visiter, _recheck, false, _min_expiration_M, _abs_maxs, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                        }
                }
                // Invalid minima downwards check
                if (_min_expiration_m <= 0 || _flag_m) { // If(minimum with smallest expiration expired || one or more minima have been updated)
                    if (_min_expiration_m <= 0) // Minimum with smallest expiration expired
                        _min_expiration_m = std::numeric_limits<int>::max(); // We need to perform this operation beforehand in order to pair up with situation in which flag_m = true
                    auto _invalid_count = 0;
                    auto _downwards_visiter = p_max_subwindow;
                    while (_downwards_visiter >= _upwards_visiter_4min) { // Then check for invalid elements downwards until needed
                        if (_abs_mins[(_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows].position < _next_t - _downwards_visiter) // Invalid
                            _invalid_count++;
                        else {
                            if (_invalid_count > 0) { // Call my_min() in order to calculate invalid subwindows' minima
                                auto _recheck = _invalid_count * p_difference_between_subwindows;
                                _min_expiration_m = my_min(_next_t - _downwards_visiter - _recheck, _downwards_visiter, _recheck, false, _min_expiration_m, _abs_mins, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                                _invalid_count = 0;
                            }
                            auto _expiration = _abs_mins[(_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows].position - (_next_t - _downwards_visiter) + 1;
                            if (_expiration < _min_expiration_m)
                                _min_expiration_m = _expiration;
                        }
                        _downwards_visiter -= p_difference_between_subwindows;
                    }
                    if (_invalid_count > 0) // It is possible that the cycle ends with invalid subwindows
                        if (_downwards_visiter < p_min_subwindow) { // Smallest subwindow's minimum is invalid: exceptional procedure in my_min()
                            auto _recheck = p_min_subwindow + (_invalid_count - 1) * p_difference_between_subwindows;
                            _min_expiration_m = my_min(_next_t - _recheck, _downwards_visiter, _recheck, true, _min_expiration_m, _abs_mins, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                        }
                        else {
                            auto _recheck = _invalid_count * p_difference_between_subwindows;
                            _min_expiration_m = my_min(_next_t - _downwards_visiter - _recheck, _downwards_visiter, _recheck, false, _min_expiration_m, _abs_mins, p_min_subwindow, p_difference_between_subwindows, p_ask_signal, _next_t);
                        }
                }
                #pragma endregion

                for (auto const current_subwindow_width : _bookers) {

                    #pragma region Retrieving subwindow basic features (C)
                    auto _absolute_maximum = _abs_maxs[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].value;
                    auto _absolute_minimum = _abs_mins[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].value;
                    auto _subwindow_height = _absolute_maximum - _absolute_minimum;
                    float _start_coefficient{ 0 };
                    float _end_coefficient{ 0 };
                    if (_subwindow_height) {
                        _start_coefficient = (p_ask_signal[static_cast<uint64_t>(_next_t) - current_subwindow_width] - _absolute_minimum) / _subwindow_height;
                        _end_coefficient = (p_ask_signal[static_cast<uint64_t>(_next_t) - 1] - _absolute_minimum) / _subwindow_height;
                    }
                    #pragma endregion

                    if ((std::distance(_peaks.begin(), _peaks.lower_bound(_next_t - current_subwindow_width)) >= 2 || std::distance(_neg_peaks.begin(), _neg_peaks.lower_bound(_next_t - current_subwindow_width)) >= 2) && abs(_start_coefficient - _end_coefficient) < p_config.start_end_dislocation_limit || !p_config.approximation_enabled) { // Else ignore signal

                        #pragma region Creating peak tuples: (position, value, width) (B2)
                        std::vector<std::tuple<int, float, float>> _filtered_peak_tuples{};
                        std::vector<std::tuple<int, float, float>> _filtered_neg_peak_tuples{};
                        if (p_config.approximation_enabled) {
                            std::vector<std::tuple<int, float, float>> _peak_tuples{};
                            std::vector<std::tuple<int, float, float>> _neg_peak_tuples{};
                            std::future<void> _promise = std::async(std::launch::async, tuples_creator, current_subwindow_width, std::ref(p_ask_signal), _next_t, std::ref(_peaks), std::ref(_peak_tuples));
                            std::future<void> _neg_promise = std::async(std::launch::async, neg_tuples_creator, current_subwindow_width, std::ref(p_ask_signal), _next_t, std::ref(_neg_peaks), std::ref(_neg_peak_tuples));
                            _promise.get();
                            _neg_promise.get();
                            peaks_filter(current_subwindow_width, _subwindow_height, _absolute_minimum, _peak_tuples, true, _filtered_peak_tuples, p_config);
                            peaks_filter(current_subwindow_width, _subwindow_height, _absolute_minimum, _neg_peak_tuples, false, _filtered_neg_peak_tuples, p_config);
                        }
                        #pragma endregion

                        #pragma region Pushing signal data into storage window (D)
                        std::pair<float, float> _coefficients{ _start_coefficient, _end_coefficient };
                        if (_start_coefficient != _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].previous_start_coefficient) {
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].previous_start_coefficient = _start_coefficient;
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_start_coefficient_counter = 0;
                        }
                        else
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_start_coefficient_counter++;
                        if (_end_coefficient != _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].previous_end_coefficient) {
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].previous_end_coefficient = _end_coefficient;
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_end_coefficient_counter = 0;
                        }
                        else
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_end_coefficient_counter++;
                        _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].add_one(
                            _next_t - 1,
                            _coefficients,
                            _filtered_peak_tuples,
                            _filtered_neg_peak_tuples,
                            _absolute_maximum,
                            _absolute_minimum,
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_start_coefficient_counter,
                            _junk_coefficient_data[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_end_coefficient_counter);
                        #pragma endregion

                        #pragma region Sigma code portion (E)
                        if (p_config.approximation_enabled) {
                            auto i = find_first_available_signal(_temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length, _next_t - 1, current_subwindow_width);
                            std::map<float, int, std::greater<float>> _results{};
                            while (i >= 0) {
                                std::pair<std::string, float> _result{ "", float{ -1 } };
                                check_patterns(current_subwindow_width, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows], i, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length - 1, _result, p_config);
                                if (_result.first.length() == 2) {
                                    if (_result.first.compare("11") == 0) // Both start and end coefficients have failed the compare operation
                                        i -= std::max(_temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_start_coefficient_counters[i], _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_end_coefficient_counters[i]);
                                    else if (_result.first.compare("01") == 0) // End coefficient has failed the compare operation
                                        i -= _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_end_coefficient_counters[i];
                                    else if (_result.first.compare("10") == 0) // Start coefficient has failed the compare operation
                                        i -= _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].same_start_coefficient_counters[i];
                                }
                                else if (_result.first.compare("0") == 0) { // Compare operation was successful in its entirety
                                    if (p_config.greedy_sigma_enabled) {
                                        std::string _reference{ "" };
                                        reference_getter(p_references, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows], i, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length - 1, current_subwindow_width, _reference);
                                        std::unique_lock<std::mutex> ul{ ext_m };
                                        if (!p_references.count(_reference)) {
                                            p_patterns[(current_subwindow_width - p_config.min_subwindow) / p_config.difference_between_subwindows].push_back(std::tuple<int, int, float>{
                                                _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps[i],
                                                _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps.back(),
                                                _result.second });
                                            p_references[_reference] = 1;
                                        }
                                        else
                                            p_references[_reference]++;
                                        break;
                                    }
                                    else
                                        _results[_result.second] = i;
                                }
                                i--;
                            }
                            if (!p_config.greedy_sigma_enabled) {
                                int _results_saved{ 0 };
                                for (auto const [confidence, position] : _results) {
                                    std::string _reference{ "" };
                                    reference_getter(p_references, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows], position, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length - 1, current_subwindow_width, _reference);
                                    std::unique_lock<std::mutex> ul{ ext_m };
                                    p_patterns[(current_subwindow_width - p_config.min_subwindow) / p_config.difference_between_subwindows].push_back(std::tuple<int, int, float>{
                                        _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps[position],
                                        _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps.back(),
                                        confidence });
                                    if (!p_references.count(_reference))
                                        p_references[_reference] = 1;
                                    else
                                        p_references[_reference]++;
                                    ul.unlock();
                                    if (p_config.knn && ++_results_saved >= p_config.knn)
                                        break;
                                }
                            }
                        }
                        else {
                            int i{ p_config.number_of_subwindows > 1 ?
                                _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length - 1 - current_subwindow_width :
                                find_first_available_signal(_temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length, _next_t - 1, current_subwindow_width) };
                            std::map<double, int> _results{};
                            while (i >= 0) {
                                _results[compute_distance(current_subwindow_width, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows], i, _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length - 1, _s_values, _next_t)] = i;
                                i--;
                            }
                            int _results_saved{ 0 };
                            for (auto const [distance, position] : _results) {
                                std::unique_lock<std::mutex> ul{ ext_m };
                                p_patterns[(current_subwindow_width - p_config.min_subwindow) / p_config.difference_between_subwindows].push_back(std::tuple<int, int, float>{
                                    _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps[position],
                                    _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps.back(),
                                    static_cast<float>(distance) });
                                ul.unlock();
                                if (p_config.knn && ++_results_saved >= p_config.knn)
                                    break;
                            }
                        }
                        #pragma endregion

                    }

                    if (p_config.partial_search && _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].length > 0 && _next_t - 1 - _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].timestamps[0] >= p_config.storage_window * current_subwindow_width)

                        #pragma region Popping signal data out of storage window (F)
                        _temp_patterns[(current_subwindow_width - p_min_subwindow) / p_difference_between_subwindows].remove_one();
                        #pragma endregion

                    #pragma region Booking next step (G)
                    auto _booking = _next_t + current_subwindow_width / (p_config.approximation_enabled ? p_beta : current_subwindow_width);
                    if (!p_config.approximation_enabled && p_config.number_of_subwindows == 1)
                        _booking += p_siblings - 1;
                    if (_booking <= p_ask_signal.size())
                        _queue[static_cast<uint64_t>(_booking) - 1].push_back(current_subwindow_width);
                    #pragma endregion

                }
            }

            #pragma region Cycle instructions (H)
            _min_expiration_M--;
            _min_expiration_m--;
            if (!(_next_t % 1000)) {
                std::unique_lock<std::mutex> _ul{ ext_m };
                if (++done == p_siblings)
                    ext_cv.notify_one();
                if (_next_t == static_cast<int>(p_ask_signal.size()))
                    break;
                int_cv.wait(_ul, [] { return go; });
                if (!--done) {
                    go = false;
                    int_cv.notify_all();
                    ext_cv.notify_one();
                }
                else
                    int_cv.wait(_ul, [] { return !go; });
            }
            if (p_config.approximation_enabled) {
                if (_peaks.size()) {
                    auto _last_peak = _peaks.rbegin();
                    if (_last_peak->first <= _next_t - p_max_subwindow)
                        _peaks.erase(_last_peak->first);
                }
                if (_neg_peaks.size()) {
                    auto _last_neg_peak = _neg_peaks.rbegin();
                    if (_last_neg_peak->first <= _next_t - p_max_subwindow)
                        _neg_peaks.erase(_last_neg_peak->first);
                }
            }
            else if (p_config.partial_search && _s_values.size()) {
                auto _last_s_value = _s_values.back();
                if (static_cast<int>(std::get<0>(_last_s_value) + static_cast<float>(std::get<1>(_last_s_value)) / 2) <= _next_t - p_max_subwindow * (p_config.storage_window + 1))
                    _s_values.pop_back();
            }
            _queue[static_cast<uint64_t>(_next_t) - 1].clear();
            _next_t++;
            if (_next_t > static_cast<int>(p_ask_signal.size())) {
                std::unique_lock<std::mutex> _ul{ ext_m };
                if (++done == p_siblings)
                    ext_cv.notify_one();
                break;
            }
            _bookers = _queue[static_cast<uint64_t>(_next_t) - 1];
            #pragma endregion

        }
    }
    catch (std::exception& e) {
        std::cerr << std::endl << e.what() << std::endl << std::fflush;
        exit(EXIT_FAILURE);
    }
}