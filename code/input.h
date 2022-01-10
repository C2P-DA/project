#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "simdjson.h"

class input_format_error : public std::exception {
	const std::string file;
	int line;
	const std::string func;
	const std::string info;

public:
	input_format_error(const std::string& p_msg, const std::string& p_file, int p_line, const std::string& p_func, const std::string& p_info = "") : std::exception(p_msg.c_str()), file(p_file), line(p_line), func(p_func), info(p_info) {}
	std::string get_file() const;
	int get_line() const;
	std::string get_func() const;
	std::string get_info() const;

};

void duka_populate(std::vector<float>&, const std::string&, const std::string&);

void populate(std::vector<float>&, const std::string&, const std::string&);

class Config {
public:
	std::string year{};
	std::string month{};
	std::string day{};
	std::string hours{};
	std::string minutes{};
	std::string seconds{};
	bool approximation_enabled{ false };
	bool partial_search{ false };
	bool dukascopy_stock_input_data{ false };
	int difference_between_subwindows{ -1 };
	int beta{ -1 };
	float start_end_dislocation_limit{ -1 };
	int max_peaks_per_pattern{ -1 };
	float min_peak_width{ -1 };
	float points_dislocation_limit{ -1 };
	float min_conf{ -1 };
	bool height_affects_conf{ false };
	bool distance_affects_conf{ false };
	bool greedy_sigma_enabled{ false };
	int knn{ -1 };
	float storage_window{ -1 };
	int min_subwindow{ -1 };
	int max_subwindow{ -1 };
	int number_of_subwindows{ -1 };
	std::string data_path{};
	std::string logs_path{};
	std::string rates_path{};
	std::string filename{};

	Config(const std::string& p_json_path) {
		if (!std::filesystem::exists(p_json_path))
			throw std::filesystem::filesystem_error::filesystem_error(p_json_path, std::error_code{ ENOENT, std::system_category() });
		simdjson::dom::parser _parser;
		simdjson::dom::element _config = _parser.load(p_json_path);
		try {
			year = _config["year"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: year field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (year.length() != 4 || !std::all_of(year.begin(), year.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: year field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			month = _config["month"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: month field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (month.length() != 2 || !std::all_of(month.begin(), month.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: month field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			day = _config["day"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: day field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (day.length() != 2 || !std::all_of(day.begin(), day.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: day field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			hours = _config["hours"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: hours field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (hours.length() != 2 || !std::all_of(hours.begin(), hours.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: hours field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			minutes = _config["minutes"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: minutes field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (minutes.length() != 2 || !std::all_of(minutes.begin(), minutes.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: minutes field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			seconds = _config["seconds"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: seconds field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (seconds.length() != 2 || !std::all_of(seconds.begin(), seconds.end(), [](char c) { return std::isdigit(c); }))
			throw input_format_error("Input Format Error: seconds field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
		try {
			min_subwindow = std::stoi(std::string{ _config["minSubwindow"] });
		}
		catch (...) {
			throw input_format_error("Input Format Error: minSubwindow field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (min_subwindow < 1)
			throw std::logic_error("Logic Error: minSubwindow field inside configuration file must be a positive integer.");
		try {
			max_subwindow = std::stoi(std::string{ _config["maxSubwindow"] });
		}
		catch (...) {
			throw input_format_error("Input Format Error: maxSubwindow field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (max_subwindow < 1)
			throw std::logic_error("Logic Error: maxSubwindow field inside configuration file must be a positive integer.");
		if (min_subwindow > max_subwindow)
			throw std::logic_error("Logic Error: minSubwindow must not be greater than maxSubwindow.");
		try {
			approximation_enabled = _config["approximationEnabled"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: approximationEnabled field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		try {
			partial_search = _config["partialSearch"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: partialSearch field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		try {
			dukascopy_stock_input_data = _config["dukascopyStockInputData"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: dukascopyStockInputData field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (approximation_enabled) {
			try {
				difference_between_subwindows = std::stoi(std::string{ _config["ifApproximationEnabled"]["differenceBetweenSubwindows"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: differenceBetweenSubwindows field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (min_subwindow == max_subwindow)
				difference_between_subwindows = 1;
			if (difference_between_subwindows < 1)
				throw std::logic_error("Logic Error: differenceBetweenSubwindows field inside configuration file must be a positive integer.");
			if (min_subwindow % difference_between_subwindows || max_subwindow % difference_between_subwindows)
				throw std::logic_error("Logic Error: Both minSubwindow and maxSubwindow must be multiples of differenceBetWeenSubwindows.");
			try {
				beta = std::stoi(std::string{ _config["ifApproximationEnabled"]["beta"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: beta field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (beta < 1 || beta > min_subwindow)
				throw std::logic_error("Logic Error: beta field inside configuration file must be a positive integer between 1 and minSubwindow.");
			int _op2{};
			int _res{};
			try {
				std::string _sedl{ _config["ifApproximationEnabled"]["startEndDislocationLimit"] };
				_res = sscanf(_sedl.c_str(), "%f / %d", &start_end_dislocation_limit, &_op2);
			}
			catch (...) {
				throw input_format_error("Input Format Error: startEndDislocationLimit field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (_res != 2)
				throw input_format_error("Input Format Error: startEndDislocationLimit field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
			else
				start_end_dislocation_limit /= _op2;
			if (start_end_dislocation_limit <= 0 || start_end_dislocation_limit > 1)
				throw std::logic_error("Logic Error: startEndDislocationLimit field inside configuration file must be greater than 0 and lower than (or equal to) 1.");
			try {
				max_peaks_per_pattern = std::stoi(std::string{ _config["ifApproximationEnabled"]["maxPeaksPerPattern"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: maxPeaksPerPattern field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (max_peaks_per_pattern < 1)
				throw std::logic_error("Logic Error: maxPeaksPerPattern field inside configuration file must be a positive integer.");
			try {
				min_peak_width = std::stof(std::string{ _config["ifApproximationEnabled"]["minPeakWidth"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: minPeakWidth field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (min_peak_width < 0)
				throw std::logic_error("Logic Error: minPeakWidth field inside configuration file must not be negative.");
			try {
				std::string _pdl{ _config["ifApproximationEnabled"]["pointsDislocationLimit"] };
				_res = sscanf(_pdl.c_str(), "%f / %d", &points_dislocation_limit, &_op2);
			}
			catch (...) {
				throw input_format_error("Input Format Error: pointsDislocationLimit field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (_res != 2)
				throw input_format_error("Input Format Error: pointsDislocationLimit field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
			else
				points_dislocation_limit /= _op2;
			if (points_dislocation_limit <= 0 || points_dislocation_limit > 1)
				throw std::logic_error("Logic Error: pointsDislocationLimit field inside configuration file must be greater than 0 and lower than (or equal to) 1.");
			try {
				std::string _mc{ _config["ifApproximationEnabled"]["minConf"] };
				_res = sscanf(_mc.c_str(), "%f / %d", &min_conf, &_op2);
			}
			catch (...) {
				throw input_format_error("Input Format Error: minConf field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (_res != 2)
				throw input_format_error("Input Format Error: minConf field inside configuration file is formatted badly.", __FILE__, __LINE__, __FUNCTION__);
			else
				min_conf /= _op2;
			if (min_conf < 0 || min_conf > 1)
				throw std::logic_error("Logic Error: minConf field inside configuration file must be between 0 and 1.");
			try {
				height_affects_conf = _config["ifApproximationEnabled"]["heightAffectsConf"];
			}
			catch (...) {
				throw input_format_error("Input Format Error: heightAffectsConf field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			try {
				distance_affects_conf = _config["ifApproximationEnabled"]["distanceAffectsConf"];
			}
			catch (...) {
				throw input_format_error("Input Format Error: distanceAffectsConf field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			try {
				greedy_sigma_enabled = _config["ifApproximationEnabled"]["greedySigmaEnabled"];
			}
			catch (...) {
				throw input_format_error("Input Format Error: greedySigmaEnabled field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
		}
		else
			difference_between_subwindows = 1;
		number_of_subwindows = (max_subwindow - min_subwindow) / difference_between_subwindows + 1;
		if (!greedy_sigma_enabled) {
			try {
				knn = std::stoi(std::string{ _config["ifNotGreedySigmaEnabled"]["knn"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: knn field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (knn < 0)
				throw std::logic_error("Logic Error: knn field inside configuration file must be a positive integer (or 0).");
		}
		if (partial_search) {
			try {
				storage_window = std::stof(std::string{ _config["ifPartialSearch"]["storageWindow"] });
			}
			catch (...) {
				throw input_format_error("Input Format Error: storageWindow field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
			}
			if (storage_window < 2)
				throw std::logic_error("Logic Error: storageWindow field inside configuration file must be at least 2.");
		}
		try {
			data_path = _config["dataPath"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: dataPath field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (!std::filesystem::exists(data_path))
			throw std::filesystem::filesystem_error::filesystem_error(data_path, std::error_code{ ENOENT, std::system_category() });
		try {
			logs_path = _config["logsPath"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: logsPath field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (!std::filesystem::exists(logs_path))
			throw std::filesystem::filesystem_error::filesystem_error(logs_path, std::error_code{ ENOENT, std::system_category() });
		try {
			rates_path = _config["ratesPath"];
		}
		catch (...) {
			throw input_format_error("Input Format Error: ratesPath field inside configuration file is not valid.", __FILE__, __LINE__, __FUNCTION__);
		}
		if (!std::filesystem::exists(rates_path))
			throw std::filesystem::filesystem_error::filesystem_error(rates_path, std::error_code{ ENOENT, std::system_category() });
		filename = year + "_" + month + "_" + day + "_" + hours + "_" + minutes + "_" + seconds;
	}
};