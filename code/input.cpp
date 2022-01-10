#include "input.h"

std::string input_format_error::get_file() const {
	return file;
}

int input_format_error::get_line() const {
	return line;
}

std::string input_format_error::get_func() const {
	return func;
}

std::string input_format_error::get_info() const {
	return info;
}

void duka_populate(std::vector<float>& p_data_structure, const std::string& p_data_path, const std::string& p_log_path) {
	std::cout << "Creating signal..." << std::endl;
	std::ifstream _data_file{ p_data_path };
	std::string s{};
	int _Y{};
	int _m{};
	int _d{};
	int _H{};
	int _M{};
	int _S{};
	int _previous_ms{};
	int _line_count{ 0 };
	// Getting the first row data
	getline(_data_file, s, ',');
	_line_count++;
	auto _ret = sscanf(s.c_str(), "%d-%d-%d %d:%d:%d.%3d", &_Y, &_m, &_d, &_H, &_M, &_S, &_previous_ms);
	if (_ret < 6 || _ret > 7)
		throw input_format_error("Input Format Error: File " + p_data_path + " is formatted badly at line " + std::to_string(_line_count) + ".", __FILE__, __LINE__, __FUNCTION__);
	if (_ret == 6)
		_previous_ms = 0;
	tm _previous_date{};
	_previous_date.tm_year = _Y - 1900;
	_previous_date.tm_mon = _m - 1;
	_previous_date.tm_mday = _d;
	_previous_date.tm_hour = _H;
	_previous_date.tm_min = _M;
	_previous_date.tm_sec = _S;
	auto _previous_date_tt = mktime(&_previous_date);
	getline(_data_file, s, ',');
	float _data_element{};
	if (sscanf(s.c_str(), "%f", &_data_element) != 1)
		throw input_format_error("Input Format Error: File " + p_data_path + " is formatted badly at line " + std::to_string(_line_count) + ".", __FILE__, __LINE__, __FUNCTION__);
	p_data_structure.push_back(_data_element);
	getline(_data_file, s);
	// Visiting the data file
	while (getline(_data_file, s, ',')) {
		_line_count++;
		int _ms{};
		_ret = sscanf(s.c_str(), "%d-%d-%d %d:%d:%d.%3d", &_Y, &_m, &_d, &_H, &_M, &_S, &_ms);
		if (_ret < 6 || _ret > 7)
			throw input_format_error("Input Format Error: File " + p_data_path + " is formatted badly at line " + std::to_string(_line_count) + ".", __FILE__, __LINE__, __FUNCTION__);
		if (_ret == 6)
			_ms = 0;
		tm _date{};
		_date.tm_year = _Y - 1900;
		_date.tm_mon = _m - 1;
		_date.tm_mday = _d;
		_date.tm_hour = _H;
		_date.tm_min = _M;
		_date.tm_sec = _S;
		auto _date_tt = mktime(&_date);
		auto _diff = difftime(_date_tt, _previous_date_tt);
		_diff = _diff * 1000 + _ms - _previous_ms;
		_previous_date_tt = _date_tt;
		_previous_ms = _ms;
		for (auto i = 0; i < _diff - 1; i++) // Filling the gaps replicating the signal
			p_data_structure.push_back(_data_element);
		getline(_data_file, s, ',');
		if (sscanf(s.c_str(), "%f", &_data_element) != 1)
			throw input_format_error("Input Format Error: File " + p_data_path + " is formatted badly at line " + std::to_string(_line_count) + ".", __FILE__, __LINE__, __FUNCTION__);
		p_data_structure.push_back(_data_element);
		getline(_data_file, s);
	}
	_data_file.close();
	auto _now = std::time(0);
	std::stringstream _ss{};
	_ss << std::put_time(std::localtime(&_now), "%F %T");
	std::cout << "Signal creation completed on " << _ss.str() << '\n' << std::endl;
	std::ofstream _log_file{ p_log_path, std::ios_base::app };
	_log_file << "Signal creation completed on " << _ss.str() << '\n' << std::endl;
	_log_file.close();
}

void populate(std::vector<float>& p_data_structure, const std::string& p_data_path, const std::string& p_log_path) {
	std::cout << "Creating signal..." << std::endl;
	std::ifstream _data_file{ p_data_path };
	std::string s{};
	int _line_count{ 0 };
	float _data_element{};
	// Visiting the data file
	while (getline(_data_file, s)) {
		_line_count++;
		if (sscanf(s.c_str(), "%f", &_data_element) != 1)
			throw input_format_error("Input Format Error: File " + p_data_path + " is formatted badly at line " + std::to_string(_line_count) + ".", __FILE__, __LINE__, __FUNCTION__);
		p_data_structure.push_back(_data_element);
	}
	_data_file.close();
	auto _now = std::time(0);
	std::stringstream _ss{};
	_ss << std::put_time(std::localtime(&_now), "%F %T");
	std::cout << "Signal creation completed on " << _ss.str() << '\n' << std::endl;
	std::ofstream _log_file{ p_log_path, std::ios_base::app };
	_log_file << "Signal creation completed on " << _ss.str() << '\n' << std::endl;
	_log_file.close();
}