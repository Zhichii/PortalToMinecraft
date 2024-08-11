#ifndef CPP_STRINGS
#define CPP_STRINGS

#include <vector>
#include <string>
#include <map>

namespace Strings {
	
	// Convert string to wide string. 
	std::wstring s2ws(const std::string& s) {
		const char* str = s.c_str();
		size_t lenO = s.size();
		size_t len = s.size() + 1;
		size_t lenO_ = lenO;
		size_t len_ = len;
		wchar_t* wstr = new wchar_t[len];
		memset(wstr, 0, len*2);
		#ifdef _WIN32
			errno_t i = MultiByteToWideChar(CP_UTF8, MB_USEGLYPHCHARS, str, lenO, wstr, len);
		#else
			mbstowcs(wstr, str, len);
		#endif
		std::wstring ret(wstr);
		if (i == EILSEQ) {
			writelog("Failed to convert string to wstring. ");
			writelog("LenO: %u. Len: %u. Errcode: %u", lenO_, len_, i);
		}
		delete[] wstr;
		return ret;
	}

	// Convert wide string to string. 
	std::string ws2s(const std::wstring& ws) {
		const wchar_t* wstr = ws.c_str();
		size_t lenO = ws.size();
		size_t len = 2 * ws.size() + 1;
		size_t lenO_ = lenO;
		size_t len_ = len;
		char* str = new char[len];
		memset(str, 0, len);
		#ifdef _WIN32
			errno_t i = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, wstr, lenO, str, len, NULL, NULL);// wcstombs_s(&lenO, str, len - 1, wstr, len);
		#else
			errno_t i = wcstombs(str, wstr, len);
		#endif
		std::string ret(str);
		if (i == EILSEQ) {
			writelog("Failed to convert wstring to string. ");
			writelog("LenO: %u. Len: %u. Errcode: %u", lenO_, len_, i);
		}
		delete[] str;
		return ret;
	}

	// Format the string, same with sprintf(). Note: use .c_str() in the arguments. 
	std::string strFormat(const std::string format, ...) {
		std::string dst = "";
		va_list args;
		va_start(args, format);
		_vsprintf_s_l(formatCacheStr, 16384, format.c_str(), NULL, args);
		va_end(args);
		return formatCacheStr;
	}
	std::wstring strFormat(const std::wstring format, ...) {
		std::string dst = "";
		va_list args;
		va_start(args, format);
		_vswprintf_s_l(formatCacheStrW, 16384, format.c_str(), NULL, args);
		va_end(args);
		return formatCacheStrW;
	}
	
	// Find the position of the substring in the string. 
	std::vector<int> find(const std::string& str, const std::string& substr) {
		std::vector<int> vi;
		for (int i = 0; i < str.size(); i++) {
			for (int j = 0; j < substr.size(); j++) {
				if ((i + j) > str.size()) break;
				if (str[i + j] != substr[j]) break;
				if (j == (substr.size() - 1)) {
					vi.push_back(i);
					break;
				}
			}
		}
		return vi;
	}

	// Count how many time the substring appear in the string. 
	int count(const std::string& str, const std::string& substr) {
		int cnt = 0;
		for (int i = 0; i < str.size(); i++) {
			for (int j = 0; j < substr.size(); j++) {
				if ((i + j) > str.size()) break;
				if (str[i + j] != substr[j]) break;
				if (j == (substr.size() - 1)) {
					cnt++;
					break;
				}
			}
		}
		return cnt;
	}
	int count(const std::wstring& str, const std::wstring& substr) {
		int cnt = 0;
		for (int i = 0; i < str.size(); i++) {
			for (int j = 0; j < substr.size(); j++) {
				if ((i + j) > str.size()) break;
				if (str[i + j] != substr[j]) break;
				if (j == (substr.size() - 1)) {
					cnt++;
					break;
				}
			}
		}
		return cnt;
	}

	// Slice the string as Python do. 
	// Include head without tail. 
	std::string slice(const std::string& baseStr, long long from = 0, long long to = 0, int step = 1) {
		std::string newStr;
		if (from == to) to = baseStr.size() - to;
		if (from < 0) from = baseStr.size() + from;
		if (to <= 0) to = baseStr.size() + to;
		if (step == 0) step = 1;
		if (from > to) step = -step;
		if (to > baseStr.size()) to = baseStr.size();
		for (int i = from; (from < to) ? (i < to) : (i > to); i += step) {
			newStr += baseStr[i];
		}
		return newStr;
	}

	// Replace the occurrence of the string "from" to the string "to" in the string. 
	std::string replace(const std::string& baseStr, const std::string& from, const std::string& to) {
		if (baseStr.size() < from.size()) return baseStr;
		std::string output;
		const int cnt = count(baseStr, from);
		const int baseLen = baseStr.size();
		const int fromLen = from.size();
		const int toLen = to.size();
		int cur = 0, curLast = 0;
		for (int i = 0; i < baseLen; i++) {
			for (int j = 0; j < from.size(); j++) {
				if ((i + j) > baseLen) break;
				if (baseStr[i + j] != from[j]) {
					output += baseStr[i];
					break;
				}
				if (j == fromLen - 1) {
					output += to;
					i += j;
					break;
				}
			}
		}
		return output;
	}
	std::wstring replace(const std::wstring& baseStr, const std::wstring& from, const std::wstring& to) {
		if (baseStr.size() < from.size()) return baseStr;
		std::wstring output;
		const int cnt = count(baseStr, from);
		const int baseLen = baseStr.size();
		const int fromLen = from.size();
		const int toLen = to.size();
		int cur = 0, curLast = 0;
		for (int i = 0; i < baseLen; i++) {
			for (int j = 0; j < from.size(); j++) {
				if ((i + j) > baseLen) break;
				if (baseStr[i + j] != from[j]) {
					output += baseStr[i];
					break;
				}
				if (j == fromLen - 1) {
					output += to;
					i += j;
					break;
				}
			}
		}
		return output;
	}

	// Split the string with the separator. 
	std::vector<std::string> split(const std::string& baseStr, const std::string& sep) {
		std::vector<std::string> output;
		if (baseStr.size() < sep.size()) {
			output.push_back(baseStr);
			return output;
		}
		const int cnt = count(baseStr, sep);
		const int baseLen = baseStr.size();
		const int sepLen = sep.size();
		int cur = 0, curLast = 0;
		std::string tmp2;
		for (int i = 0; i < baseLen; i++) {
			for (int j = 0; j < sepLen; j++) {
				if ((i + j) > baseLen) break;
				if (baseStr[i + j] != sep[j]) {
					tmp2 += baseStr[i];
					break;
				}
				if (j == sepLen - 1) {
					output.push_back(tmp2);
					tmp2 = "";
					i += j;
					break;

				}
			}
		}
		output.push_back(tmp2);
		return output;
	}

	// Join the string with the separator. 
	std::string join(const std::vector<std::string>& vec, const std::string& joiner) {
		if (vec.size() == 0) return "";
		std::string output;
		output += vec[0];
		;	for (int i = 1; i < vec.size(); i++) {
			output += joiner;
			output += vec[i];
		}
		return output;
	}

	// Get the string between two strings in the string. 
	std::string between(const std::string& baseStr, const std::string& start, const std::string& end) {
		std::string output = baseStr;
		std::vector<int> startPos = find(baseStr, start);
		if (start == "" || startPos.size() == 0);
		else {
			output = slice(output, startPos[0] + start.size(), output.size());
		}
		std::vector<int> endPos = find(output, end);
		if (end == "" || endPos.size() == 0);
		else {
			output = slice(output, 0, endPos[0]);

		}
		return output;
	}

	// Check if str starts with substr. 
	bool startsWith(const std::string& str, const std::string& substr) {
		if (str.size() < substr.size()) return false;
		if (str == substr) return true;
		for (int i = 0; i < substr.size(); i++) {
			if (substr[i] != str[i]) return false;
		}
		return true;
	}

	// Format the string of a directory. 
	std::string formatDirStr(const std::string& path) {
		std::string o = path;
		for (int i = 0; i < o.size(); i++) {
			if (o[i] == O_PATHSEP[0]) {
				o[i] = PATHSEP[0];
			}
		}
		while (count(o, DPATHSEP)) {
			o = replace(o, DPATHSEP, PATHSEP);
		}
		std::vector<int> pos = find(o, ":");
		if (pos.size() != 0) {
			int p = pos[0];
			o[p] = '?';
			o = replace(o, ":", "");
			o[p] = ':';
			if (o.size() > p+1) {
				if (o[p+1] != PATHSEP[0])
					o = replace(o, ":", std::string(":") + PATHSEP);
			}
		}
		if (startsWith(path, "\\\\")) {
			o = "\\" + o;
		}
		if (o[o.size()-1] != PATHSEP[0]) {
			o += PATHSEP;
		}
		return o;
	}

}

#endif