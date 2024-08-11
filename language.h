
#ifndef RIVERLAUNCHER3_LANGUAGE_H
#define RIVERLAUNCHER3_LANGUAGE_H

#include <map>
#include <string>
#include <fstream>
#ifdef _WIN32
	#include <windows.h>
#endif
#include "resource.h"
#include "strings.h"
#include <json/json.h>

class Language;

// Create a class of StringVal for the mutable variable. 
class StringVal {
	std::string val;
	std::vector<StringVal> vS;
	bool isCombination = false;
	Language** lang = nullptr;
public:
	StringVal() {}
	StringVal(std::string v) { this->val = v; }
	StringVal(const char* v) { this->val = v; }
	StringVal(std::string k, Language** l) {
		this->val = k;
		this->lang = l;
	}
	StringVal(const char* k, Language** l) {
		this->val = k;
		this->lang = l;
	}
	StringVal(std::vector<StringVal> combination) {
		this->vS = combination;
		this->isCombination = true;
	}
	[[nodiscard]] std::string asString() const;
	[[nodiscard]] const char* c_str() const;
	operator std::string() { return this->asString(); }
};

class Language {
protected:
	std::string langID;
	std::string langName;
public:
	std::map<std::string, std::string> lang;
	Language(std::string languageID, std::string languageName, std::map<std::string, std::string> languageContent) {
		this->langID = languageID;
		this->langName = languageName;
		this->lang = languageContent;
	}
	Language(std::string languageID, std::string languageName, std::vector<std::pair<std::string, std::string>> languageContent) {
		this->langID = languageID;
		this->langName = languageName;
		this->lang = {};
		for (const auto& i : languageContent) {
			this->lang[i.first] = i.second;
		}
	}
	std::string getID() {
		return this->langID;
	}
	std::string getName() {
		return this->langName;
	}
	std::string localize(std::string key) {
		if (lang.contains(key)) return lang[key];
		else return key;
	}
};

[[nodiscard]] std::string StringVal::asString() const {
	if (this->isCombination) {
		std::string s;
		for (const auto& i : this->vS) {
			s += i.asString();
		}
		return s;
	}
	if (this->lang != nullptr) return (*(this->lang))->localize(this->val);
	return this->val;
}
[[nodiscard]] const char* StringVal::c_str() const {
	std::string x = this->asString();
	return x.c_str();
}

std::map<std::string, Language*> allLanguages;
std::vector<std::pair<std::string,int>> langManifest;

void initLanguages() {
	langManifest.push_back({ "empty", IDR_LANG_EMPTY });
	langManifest.push_back({ "en-GB", IDR_LANG_EN_GB });
	langManifest.push_back({ "zh-CN", IDR_LANG_ZH_CN });
	langManifest.push_back({ "zh-MO", IDR_LANG_ZH_MO });
	std::vector<std::pair<std::string,std::string>> vpss;
	Json::Value val;
	#ifdef _WIN32
		HRSRC hRsrc;
		HGLOBAL IDR;
		DWORD sz;
	#endif
	for (const auto& i : langManifest) {
		vpss = {};
		val = {};
		Json::Reader reader;
		#ifdef _WIN32
			hRsrc = FindResourceA(NULL, MAKEINTRESOURCEA(i.second), "lang");
			IDR = LoadResource(NULL, hRsrc);
			sz = SizeofResource(NULL, hRsrc);
			reader.parse(std::string( (const char*)LockResource(IDR), sz), val);
			FreeResource(IDR);
		#endif
		std::string name = "?";
		for (std::string i : val.getMemberNames()) {
			vpss.push_back(std::pair{ i, val[i].asString() });
			if (i == "lang.name") name = val[i].asString();
		}
		allLanguages[i.first] = new Language(i.first, name, vpss);
	}
}

std::string getDefaultLanguage() {
	#ifdef _WIN32
		wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
		GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);
		//writeLog("getDefaultLanguage() in language.h", Strings::ws2s(localeName));
		if (allLanguages.contains(Strings::ws2s(localeName))) return Strings::ws2s(localeName);
		else if (Strings::startsWith(Strings::ws2s(localeName), "en-")) {
			return "en-GB";
		}
		else if (Strings::startsWith(Strings::ws2s(localeName), "zh-")) {
			return "zh-CN";
		}
	#else
	#endif
	return "en-GB";
}

#endif //RIVERLAUNCHER3_HELP_H