
#ifndef RIVERLAUNCHER3_DATA_H
#define RIVERLAUNCHER3_DATA_H

#include <cstdio>
#include <fstream>
#include "help.h"
#include <json/json.h>

Json::Value data;

void saveData() {
	std::ofstream ofs;
	ofs.open(S("RvL") + PATHSEP + "config.json", std::ios::out);
	Json::StyledWriter sw;
	std::string s = sw.write(data);
	ofs << s;
	ofs.close();
}

void initData() {
	// If the configuration file does not exist, create it. 
	if (!isExists(S("RvL") + PATHSEP + "config.json")) {
		data = Json::objectValue;
	}
	// If it does, read and parse it. 
	else {
		std::ifstream ifs;
		ifs.open("RvL\\config.json", std::ios::in);
		Json::Reader r;
		r.parse(ifs, data);
		ifs.close();
	}
	// Transform old key names into new key names, and
	// transform old data formats into new data formats.
	if ((!data.isMember("Language")) && data.isMember("SelectedLang")) {
		switch (data["SelectedLang"].asInt()) {
		case 1: {
			data["Language"] = "en-GB";
			break;
		}
		case 2: {
			data["Language"] = "zh-CN";
			break;
		}
		default: {
			data["Language"] = getDefaultLanguage();
			break;
		}
		}
		data.removeMember("SelectedLang");
	}
	if ((!data.isMember("GameDir")) && data.isMember("MinecraftDirectory")) {
		data["GameDir"] = data["MinecraftDirectory"];
		data.removeMember("MinecraftDirectory");
	}
	if (data.isMember("SelectedLaunch")) {
		data.removeMember("SelectedLaunch");
	}
	if ((!data.isMember("Javas")) && data.isMember("SelectedJava")) {
		data["Javas"] = Json::arrayValue;
		if (data["Javas"] != "\"AUTO\"") data["Javas"].append(data["SelectedJava"]);
		data["SelectedJava"] = 0;
	}
	if (data.isMember("Accounts") && data["Accounts"].type() == Json::stringValue) {
		Json::Reader r;
		std::string s = data["Accounts"].asString();
		r.parse(s, data["Accounts"]);
		if (data["Accounts"].type() != Json::arrayValue) data["Accounts"] = Json::arrayValue;
		for (int i = 0; i < data["Accounts"].size(); i++) {
			if (!data["Accounts"][i].isMember("userType")) data["Accounts"][i]["userType"] = "please_support";
			if (data["Accounts"][i]["userType"] == 0) data["Accounts"][i]["userType"] = "please_support";
			if (data["Accounts"][i]["userType"] == 1) data["Accounts"][i]["userType"] = "microsoft";
		}
	}
	// Check missing keys. 
	if (!data.isMember("Language")) data["Language"] = getDefaultLanguage();
	if (!data.isMember("GameDir")) data["GameDir"] = "";
	if (!data.isMember("SelectedJava")) data["SelectedJava"] = -1;
	if (!data.isMember("WindowHeight")) data["WindowHeight"] = 0;
	if (!data.isMember("WindowWidth")) data["WindowWidth"] = 0;
	if (!data.isMember("Javas")) data["Javas"] = Json::arrayValue;
	if (!data.isMember("Accounts")) data["Accounts"] = Json::arrayValue;
	if (!data.isMember("SelectedAccount")) data["SelectedAccount"] = 0;
	if (!data.isMember("Geometry")) data["Geometry"] = "1050x745+100+50";
	data["GameDir"] = Strings::formatDirStr(data["GameDir"].asString());
	saveData();
}

Json::Value& rdata(std::string key) {
	return data[key];
}

#endif //RIVERLAUNCHER3_DATA_H