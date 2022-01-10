#pragma once

#include <deque>
#include <vector>
#include <tuple>
#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include "input.h"

class TempPatterns {
public:
	std::deque<int> timestamps{};
	std::deque<std::pair<float, float>> start_end_coefficients{};
	std::deque<std::vector<std::tuple<int, float, float>>> peaks{};
	std::deque<std::vector<std::tuple<int, float, float>>> neg_peaks{};
	std::deque<float> absolute_maxima{};
	std::deque<float> absolute_minima{};
	std::deque<int> same_start_coefficient_counters{};
	std::deque<int> same_end_coefficient_counters{};
	int length{ 0 };

	void add_one(int, std::pair<float, float>&, std::vector<std::tuple<int, float, float>>&, std::vector<std::tuple<int, float, float>>&, float, float, int, int);
	void remove_one();
};

class JunkCoefficientData {
public:
	float previous_start_coefficient{ -1 };
	float previous_end_coefficient{ -1 };
	int same_start_coefficient_counter{ 0 };
	int same_end_coefficient_counter{ 0 };

	JunkCoefficientData() {}
};

class Point {
public:
	int position{ -1 };
	float value{ -1 };

	Point() {}
	Point(int p_position, float p_value) : position(p_position), value(p_value) {}
	Point(bool p_min_init) {
		if (p_min_init)
			value = std::numeric_limits<float>::max();
	}
	void update(int, float);
};

class LabeledPointOnAxis {
public:
	int position{ -1 };
	std::vector<int> labels{};

	LabeledPointOnAxis() {}
	LabeledPointOnAxis(int p_position, int p_label) : position(p_position) {
		labels.push_back(p_label);
	}
};

int my_max(int, int, int, bool, int, std::vector<Point>&, int, int, std::vector<float>&, int);

int my_min(int, int, int, bool, int, std::vector<Point>&, int, int, std::vector<float>&, int);

void peaks_filter(int, float, float, std::vector<std::tuple<int, float, float>>&, bool, std::vector<std::tuple<int, float, float>>&, Config&);

int find_first_available_signal(std::deque<int>&, int, int, int);

bool validator(int, std::vector<std::tuple<int, float, float>>&, float, float, std::vector<std::tuple<int, float, float>>&, float, float, float&, Config&);

void check_patterns(int, TempPatterns&, int, int, std::pair<std::string, float>&, Config&);

class StopWatch {
public:
	std::chrono::steady_clock::time_point start{};
	unsigned long long ns{ 0 };

	StopWatch() {}
	void pause();
	void go();
	void reset();
};

void tuples_creator(const int, const std::vector<float>&, const int, std::map<int, std::map<int, float>, std::greater<int>>&, std::vector<std::tuple<int, float, float>>&);

void neg_tuples_creator(const int, const std::vector<float>&, const int, std::map<int, std::map<int, float>, std::greater<int>>&, std::vector<std::tuple<int, float, float>>&);

void reference_getter(std::unordered_map<std::string, int>&, TempPatterns&, int, int, int, std::string&);

double compute_distance(int, TempPatterns&, int, int, std::deque<std::tuple<float, int, float>>&, int);