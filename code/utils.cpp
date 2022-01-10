#include "utils.h"

void TempPatterns::add_one(int p_T, std::pair<float, float>& p_SEC, std::vector<std::tuple<int, float, float>>& p_P, std::vector<std::tuple<int, float, float>>& p_NP, float p_AMX, float p_AMN, int p_SSCC, int p_SECC) {
	timestamps.push_back(p_T);
	start_end_coefficients.push_back(p_SEC);
	peaks.push_back(p_P);
	neg_peaks.push_back(p_NP);
	absolute_maxima.push_back(p_AMX);
	absolute_minima.push_back(p_AMN);
	same_start_coefficient_counters.push_back(p_SSCC);
	same_end_coefficient_counters.push_back(p_SECC);
	length++;
}

void TempPatterns::remove_one() {
	timestamps.pop_front();
	start_end_coefficients.pop_front();
	peaks.pop_front();
	neg_peaks.pop_front();
	absolute_maxima.pop_front();
	absolute_minima.pop_front();
	same_start_coefficient_counters.pop_front();
	same_end_coefficient_counters.pop_front();
	length--;
}

void Point::update(int p_position, float p_value) {
	position = p_position;
	value = p_value;
}

int my_max(int p_start, int p_downwards_visiter, int p_length, bool p_flag_exceptional, int p_min_expiration_M, std::vector<Point>& p_abs_maxs, int p_min_subwindow, int p_difference_between_subwindows, std::vector<float>& p_ask_signal, int p_next_t) {
	Point _current_max = p_flag_exceptional ? Point{} : p_abs_maxs[(p_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows];
	if (p_flag_exceptional)
		for (auto i = p_start + p_length - 1; i > p_start + p_length - p_downwards_visiter - 1; i--)
			if (p_ask_signal[i] > _current_max.value) // The '=' is missing because we want to keep the most recent value as a maximum (remember: we're visiting backwards)
				_current_max.update(i, p_ask_signal[i]);
	auto _count = 0;
	for (auto i = !p_flag_exceptional ? p_start + p_length - 1 : p_start + p_length - p_downwards_visiter - 1; i > p_start - 1; i--) {
		if (p_ask_signal[i] > _current_max.value) // The '=' is missing because we want to keep the most recent value as a maximum (remember: we're visiting backwards)
			_current_max.update(i, p_ask_signal[i]);
		_count++;
		if (!(_count % p_difference_between_subwindows)) {
			p_abs_maxs[(p_downwards_visiter + _count - p_min_subwindow) / p_difference_between_subwindows] = _current_max;
			auto _expiration = _current_max.position - (p_next_t - (p_downwards_visiter + _count)) + 1;
			if (_expiration < p_min_expiration_M)
				p_min_expiration_M = _expiration;
		}
	}
	return p_min_expiration_M;
}

int my_min(int p_start, int p_downwards_visiter, int p_length, bool p_flag_exceptional, int p_min_expiration_m, std::vector<Point>& p_abs_mins, int p_min_subwindow, int p_difference_between_subwindows, std::vector<float>& p_ask_signal, int p_next_t) {
	Point _current_min = p_flag_exceptional ? Point{ true } : p_abs_mins[(p_downwards_visiter - p_min_subwindow) / p_difference_between_subwindows];
	if (p_flag_exceptional)
		for (auto i = p_start + p_length - 1; i > p_start + p_length - p_downwards_visiter - 1; i--)
			if (p_ask_signal[i] < _current_min.value) // The '=' is missing because we want to keep the most recent value as a minimum (remember: we're visiting backwards)
				_current_min.update(i, p_ask_signal[i]);
	auto _count = 0;
	for (auto i = !p_flag_exceptional ? p_start + p_length - 1 : p_start + p_length - p_downwards_visiter - 1; i > p_start - 1; i--) {
		if (p_ask_signal[i] < _current_min.value) // The '=' is missing because we want to keep the most recent value as a minimum (remember: we're visiting backwards)
			_current_min.update(i, p_ask_signal[i]);
		_count++;
		if (!(_count % p_difference_between_subwindows)) {
			p_abs_mins[(p_downwards_visiter + _count - p_min_subwindow) / p_difference_between_subwindows] = _current_min;
			auto _expiration = _current_min.position - (p_next_t - (p_downwards_visiter + _count)) + 1;
			if (_expiration < p_min_expiration_m)
				p_min_expiration_m = _expiration;
		}
	}
	return p_min_expiration_m;
}

void peaks_filter(int p_window_width, float p_window_height, float p_absolute_minimum, std::vector<std::tuple<int, float, float>>& p_values, bool p_positive, std::vector<std::tuple<int, float, float>>& p_result, Config& p_config) {
	if (p_positive) {
		std::tuple<int, float, float> _current_highest_peak{ -1, float{ -1 }, float{ -1 } };
		int _sum_4_avg{ 0 };
		int _cnt_4_avg{ 0 };
		for (auto it = p_values.rbegin(); it != p_values.rend(); it++)
			if (std::get<2>(*it) >= p_config.min_peak_width) // Peak's width must be considerable
				if ((std::get<1>(*it) - p_absolute_minimum) / p_window_height >= (float) 1 / p_config.max_peaks_per_pattern) // Peak's height must be considerably high
					if (std::get<1>(_current_highest_peak) == -1) { // No peaks have been spotted yet
						_current_highest_peak = *it;
						_sum_4_avg = std::get<0>(_current_highest_peak);
						_cnt_4_avg = 1;
					}
					else if (static_cast<float>(std::get<0>(*it) - std::get<0>(_current_highest_peak)) / p_window_width >= (float) 1 / p_config.max_peaks_per_pattern) { // Previously spotted peak is far enough to be considered separately
						p_result.push_back(_current_highest_peak);
						_current_highest_peak = *it;
						_sum_4_avg = std::get<0>(_current_highest_peak);
						_cnt_4_avg = 1;
					}
					else if (std::get<2>(*it) >= std::get<2>(_current_highest_peak)) {
						if (std::get<2>(*it) == std::get<2>(_current_highest_peak)) {
							_sum_4_avg += std::get<0>(*it);
							_current_highest_peak = std::tuple<int, float, float>{ _sum_4_avg / ++_cnt_4_avg, std::get<1>(_current_highest_peak), std::get<2>(_current_highest_peak) };
						}
						else {
							_current_highest_peak = *it;
							_sum_4_avg = std::get<0>(_current_highest_peak);
							_cnt_4_avg = 1;
						}
					}
		if (std::get<1>(_current_highest_peak) != -1)
			p_result.push_back(_current_highest_peak);
	}
	else {
		std::tuple<int, float, float> _current_lowest_peak{ -1, float{ -1 }, float{ -1 } };
		int _sum_4_avg{ 0 };
		int _cnt_4_avg{ 0 };
		for (auto it = p_values.rbegin(); it != p_values.rend(); it++)
			if (std::get<2>(*it) >= p_config.min_peak_width) // Peak's width must be considerable
				if ((std::get<1>(*it) - p_absolute_minimum) / p_window_height <= (float) (p_config.max_peaks_per_pattern - 1) / p_config.max_peaks_per_pattern) // Peak's height must be considerably high
					if (std::get<1>(_current_lowest_peak) == -1) { // No peaks have been spotted yet
						_current_lowest_peak = *it;
						_sum_4_avg = std::get<0>(_current_lowest_peak);
						_cnt_4_avg = 1;
					}
					else if (static_cast<float>(std::get<0>(*it) - std::get<0>(_current_lowest_peak)) / p_window_width >= (float) 1 / p_config.max_peaks_per_pattern) { // Previously spotted peak is far enough to be considered separately
						p_result.push_back(_current_lowest_peak);
						_current_lowest_peak = *it;
						_sum_4_avg = std::get<0>(_current_lowest_peak);
						_cnt_4_avg = 1;
					}
					else if (std::get<2>(*it) >= std::get<2>(_current_lowest_peak)) {
						if (std::get<2>(*it) == std::get<2>(_current_lowest_peak)) {
							_sum_4_avg += std::get<0>(*it);
							_current_lowest_peak = std::tuple<int, float, float>{ _sum_4_avg / ++_cnt_4_avg, std::get<1>(_current_lowest_peak), std::get<2>(_current_lowest_peak) };
						}
						else {
							_current_lowest_peak = *it;
							_sum_4_avg = std::get<0>(_current_lowest_peak);
							_cnt_4_avg = 1;
						}
					}
		if (std::get<1>(_current_lowest_peak) != -1)
			p_result.push_back(_current_lowest_peak);
	}
}

int find_first_available_signal(std::deque<int>& p_data, int p_length, int p_index, int p_window_width) {
	int _l{ 0 };
	int _r{ p_length - 1 };
	if (_l > _r)
		return -1;
	int _m{ -1 };
	while (_l <= _r) {
		_m = (_l + _r) / 2;
		if (p_index - p_data[_m] - p_window_width < 0)
			_r = _m - 1;
		else if (p_index - p_data[_m] - p_window_width > 0)
			_l = _m + 1;
		else
			return _m;
	}
	return p_index - p_data[_m] - p_window_width > 0 ? _m : _m - 1;
}

bool validator(int p_window_width, std::vector<std::tuple<int, float, float>>& p_first_filtered_peaks, float p_first_window_height, float p_first_absolute_minimum, std::vector<std::tuple<int, float, float>>& p_second_filtered_peaks, float p_second_window_height, float p_second_absolute_minimum, float& p_confidence, Config& p_config) {
	std::vector<std::tuple<int, float, float>>* _least_large{};
	std::vector<std::tuple<int, float, float>>* _other{};
	float* _llam{};
	float* _oam{};
	float* _llwh{};
	float* _owh{};
	if (p_first_filtered_peaks.size() <= p_second_filtered_peaks.size()) { // Differentiating peak data structures by size
		_least_large = &p_first_filtered_peaks;
		_other = &p_second_filtered_peaks;
		_llam = &p_first_absolute_minimum;
		_oam = &p_second_absolute_minimum;
		_llwh = &p_first_window_height;
		_owh = &p_second_window_height;
	}
	else {
		_least_large = &p_second_filtered_peaks;
		_other = &p_first_filtered_peaks;
		_llam = &p_second_absolute_minimum;
		_oam = &p_first_absolute_minimum;
		_llwh = &p_second_window_height;
		_owh = &p_first_window_height;
	}
	if (_least_large->size() < 2)
		return false;
	int _found{ 0 };
	double _confidence{ 0 };
	for (auto i = 0; i < _least_large->size(); i++) {
		float _x1{ static_cast<float>(std::get<0>((*_least_large)[i])) / p_window_width };
		float _y1{ (std::get<1>((*_least_large)[i]) - *_llam) / *_llwh };
		for (auto j = 0; j < _other->size(); j++) {
			float _x2{ static_cast<float>(std::get<0>((*_other)[j])) / p_window_width };
			float _y2{ (std::get<1>((*_other)[j]) - *_oam) / *_owh };
			double _distance{ static_cast<float>(sqrt(pow((static_cast<double>(_x2) - static_cast<double>(_x1)), 2) + pow((static_cast<double>(_y2) - static_cast<double>(_y1)), 2))) };
			if (_distance < static_cast<double>(p_config.points_dislocation_limit)) { // Distance between the two points is low
				_found++;
				_confidence += log10(1 + 9 * (1 - _distance / static_cast<double>(p_config.points_dislocation_limit)));
				break;
			}
		}
	}
	if (static_cast<float>(_found) / _least_large->size() == 1) {
		p_confidence = static_cast<float>(_confidence / _least_large->size());
		return true;
	}
	if (_least_large->size() != _other->size() ||
		abs((std::get<1>(_least_large->front()) - *_llam) / *_llwh - (std::get<1>(_least_large->back()) - *_llam) / *_llwh) >= (float) 1 / p_config.max_peaks_per_pattern ||
		abs((std::get<1>(_other->front()) - *_llam) / *_llwh - (std::get<1>(_other->back()) - *_llam) / *_llwh) >= (float) 1 / p_config.max_peaks_per_pattern)
		return false;
	_found = 0;
	_confidence = 0;
	for (auto i = 0; i < _least_large->size(); i++) {
		float _x1{ 1 - static_cast<float>(std::get<0>((*_least_large)[i])) / p_window_width };
		float _y1{ (std::get<1>((*_least_large)[i]) - *_llam) / *_llwh };
		for (auto j = 0; j < _other->size(); j++) {
			float _x2{ static_cast<float>(std::get<0>((*_other)[j])) / p_window_width };
			float _y2{ (std::get<1>((*_other)[j]) - *_oam) / *_owh };
			double _distance{ sqrt(pow((static_cast<double>(_x2) - static_cast<double>(_x1)), 2) + pow((static_cast<double>(_y2) - static_cast<double>(_y1)), 2)) };
			if (_distance < static_cast<double>(p_config.points_dislocation_limit)) { // Distance between the two points is low
				_found++;
				_confidence += log10(1 + 9 * (1 - _distance / static_cast<double>(p_config.points_dislocation_limit)));
				break;
			}
		}
	}
	if (static_cast<float>(_found) / _least_large->size() == 1) {
		p_confidence = static_cast<float>(_confidence / _least_large->size());
		return true;
	}
	else
		return false;
}

void check_patterns(int p_window_width, TempPatterns& p_temp_patterns, int p_first_index, int p_second_index, std::pair<std::string, float>& p_result, Config& p_config) {
	std::string _partial_result{ "" };
	_partial_result += abs(p_temp_patterns.start_end_coefficients[p_first_index].first - p_temp_patterns.start_end_coefficients[p_second_index].first) >= p_config.points_dislocation_limit ? "1" : "0";
	_partial_result += abs(p_temp_patterns.start_end_coefficients[p_first_index].second - p_temp_patterns.start_end_coefficients[p_second_index].second) >= p_config.points_dislocation_limit ? "1" : "0";
	if (_partial_result.compare("00") != 0)
		p_result = std::pair<std::string, float>{ _partial_result, float{ -1 } };
	else {
		auto _first_window_height = p_temp_patterns.absolute_maxima[p_first_index] - p_temp_patterns.absolute_minima[p_first_index];
		auto _second_window_height = p_temp_patterns.absolute_maxima[p_second_index] - p_temp_patterns.absolute_minima[p_second_index];
		float _confidence_M{ -1 };
		float _confidence_m{ -1 };
		if (validator(p_window_width, p_temp_patterns.peaks[p_first_index], _first_window_height, p_temp_patterns.absolute_minima[p_first_index], p_temp_patterns.peaks[p_second_index], _second_window_height, p_temp_patterns.absolute_minima[p_second_index], _confidence_M, p_config) && // Check on positive peaks
			validator(p_window_width, p_temp_patterns.neg_peaks[p_first_index], _first_window_height, p_temp_patterns.absolute_minima[p_first_index], p_temp_patterns.neg_peaks[p_second_index], _second_window_height, p_temp_patterns.absolute_minima[p_second_index], _confidence_m, p_config)) { // Check on negative peaks
			auto _confidence = (_confidence_M + _confidence_m) / 2;
			int _divider{ 1 };
			float _height_ratio{ -1 };
			float _distance_ratio{ -1 };
			if (p_config.height_affects_conf) {
				_height_ratio = _first_window_height >= _second_window_height ? _first_window_height / _second_window_height : _second_window_height / _first_window_height;
				_confidence += log10(1 + 9 * (2 - (_height_ratio <= 2 ? _height_ratio : 2)));
				_divider++;
			}
			if (p_config.distance_affects_conf) {
				_distance_ratio = static_cast<float>(p_temp_patterns.timestamps[p_second_index] - p_temp_patterns.timestamps[p_first_index] - p_window_width) / p_window_width;
				_confidence += exp(-2 * (_distance_ratio <= 1 ? _distance_ratio : 1));
				_divider++;
			}
			_confidence /= _divider;
			if (_confidence >= p_config.min_conf)
				p_result = std::pair<std::string, float>{ "0", _confidence };
			else
				p_result = std::pair<std::string, float>{ "1", float{ -1 } };
		}
		else
			p_result = std::pair<std::string, float>{ "1", float{ -1 } };
	}
}

void StopWatch::pause() {
	ns += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
}

void StopWatch::go() {
	start = std::chrono::high_resolution_clock::now();
}

void StopWatch::reset() {
	ns = 0;
}

std::mutex m{};
std::mutex neg_m{};

void tuples_creator(const int p_booker, const std::vector<float>& p_ask_signal, const int p_next_t, std::map<int, std::map<int, float>, std::greater<int>>& p_peaks, std::vector<std::tuple<int, float, float>>& p_peak_tuples) {
	for (auto& item : p_peaks) {
		auto _first_item = item.second.begin();
		auto _left_pos = p_next_t - p_booker;
		auto _left_pos_bkp = _left_pos;
		bool _condition{ true };
		if (item.first <= _left_pos)
			_condition = false;
		else if (_left_pos < _first_item->first);
		else if (p_ask_signal[_left_pos] < p_ask_signal[item.first]);
		else {
			auto _left_value = p_ask_signal[_left_pos];
			bool _found{ false };
			for (auto i = _left_pos + 1; i < item.first; i++)
				if (p_ask_signal[i] < _left_value) {
					_found = true;
					break;
				}
			_condition = _found;
		}
		if (_condition) {
			if (_left_pos > _first_item->first)
				_left_pos = item.second.lower_bound(_left_pos)->first;
			std::lock_guard<std::mutex> lg{ m };
			p_peak_tuples.push_back(std::tuple<int, float, float>{ item.first - _left_pos_bkp, p_ask_signal[item.first], _left_pos <= _first_item->first ? _first_item->second : item.second[_left_pos] });
		}
		else
			return;
	}
}

void neg_tuples_creator(const int p_booker, const std::vector<float>& p_ask_signal, const int p_next_t, std::map<int, std::map<int, float>, std::greater<int>>& p_neg_peaks, std::vector<std::tuple<int, float, float>>& p_neg_peak_tuples) {
	for (auto& item : p_neg_peaks) {
		auto _first_item = item.second.begin();
		auto _left_pos = p_next_t - p_booker;
		auto _left_pos_bkp = _left_pos;
		bool _condition{ true };
		if (item.first <= _left_pos)
			_condition = false;
		else if (_left_pos < _first_item->first);
		else if (p_ask_signal[_left_pos] > p_ask_signal[item.first]);
		else {
			auto _left_value = p_ask_signal[_left_pos];
			bool _found{ false };
			for (auto i = _left_pos + 1; i < item.first; i++)
				if (p_ask_signal[i] > _left_value) {
					_found = true;
					break;
				}
			_condition = _found;
		}
		if (_condition) {
			if (_left_pos > _first_item->first)
				_left_pos = item.second.lower_bound(_left_pos)->first;
			std::lock_guard<std::mutex> lg{ neg_m };
			p_neg_peak_tuples.push_back(std::tuple<int, float, float>{ item.first - _left_pos_bkp, p_ask_signal[item.first], _left_pos <= _first_item->first ? _first_item->second : item.second[_left_pos] });
		}
		else
			return;
	}
}

void reference_getter(std::unordered_map<std::string, int>& p_references, TempPatterns& p_temp_patterns, int p_first_index, int p_second_index, int p_subwindow_width, std::string& p_result) {
	std::tuple<int, float, float> _first_biggest_peak{ -1, float{ -1}, float{ -1} };
	std::tuple<int, float, float> _second_biggest_peak{ -1, float{ -1}, float{ -1} };
	for (auto const& peak : p_temp_patterns.peaks[p_first_index])
		if (std::get<2>(peak) > std::get<2>(_first_biggest_peak))
			_first_biggest_peak = peak;
	for (auto const& peak : p_temp_patterns.neg_peaks[p_first_index])
		if (std::get<2>(peak) > std::get<2>(_first_biggest_peak))
			_first_biggest_peak = peak;
	for (auto const& peak : p_temp_patterns.peaks[p_second_index])
		if (std::get<2>(peak) > std::get<2>(_second_biggest_peak))
			_second_biggest_peak = peak;
	for (auto const& peak : p_temp_patterns.neg_peaks[p_second_index])
		if (std::get<2>(peak) > std::get<2>(_second_biggest_peak))
			_second_biggest_peak = peak;
	p_result = std::to_string(p_temp_patterns.timestamps[p_second_index] + 1 - p_subwindow_width + std::get<0>(_second_biggest_peak)) + ":" + std::to_string(p_temp_patterns.timestamps[p_first_index] + 1 - p_subwindow_width + std::get<0>(_first_biggest_peak));
}

double compute_distance(int p_subwindow_width, TempPatterns& p_temp_patterns, int p_first_index, int p_second_index, std::deque<std::tuple<float, int, float>>& p_s_values, int p_next_t) {
	double _distance{ 0 };
	int _remaining{ p_subwindow_width };
	int it{ 0 };
	while (p_next_t - p_subwindow_width <= static_cast<int>(std::get<0>(p_s_values[it]) - static_cast<float>(std::get<1>(p_s_values[it])) / 2))
		it++;
	int it2{ 0 };
	int _quantity{ std::get<1>(p_s_values[it]) - (static_cast<int>(std::get<0>(p_s_values[it]) + static_cast<float>(std::get<1>(p_s_values[it])) / 2) - (p_next_t - p_subwindow_width)) };
	int _quantity2{ std::get<1>(p_s_values[it2]) + 1 };
	while (true) {
		int _final_quantity{ std::min({ _quantity, _quantity2, _remaining }) };
		double _absolute_distance{ (static_cast<double>(std::get<2>(p_s_values[it2])) - p_temp_patterns.absolute_minima[p_second_index]) - (static_cast<double>(std::get<2>(p_s_values[it])) - p_temp_patterns.absolute_minima[p_first_index]) };
		_distance += _final_quantity * pow(_absolute_distance, 2);
		_remaining -= _final_quantity;
		_quantity -= _final_quantity;
		_quantity2 -= _final_quantity;
		if (!_remaining)
			break;
		if (!_quantity)
			_quantity = std::get<1>(p_s_values[++it]) + 1;
		if (!_quantity2)
			_quantity2 = std::get<1>(p_s_values[++it2]) + 1;
	}
	return sqrt(_distance);
}