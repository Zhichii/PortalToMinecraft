
#ifndef RIVERLAUNCHER3_VERSIONINFO_H
#define RIVERLAUNCHER3_VERSIONINFO_H

#include <vector>
#include <string>
#include <json/json.h>
#include "help.h"

class VersionInfo {
public:
	class File {
		std::string u;
		std::string h;
		long long s;
		std::string p;
	public:
		File() {
			this->u = "";
			this->h = "";
			this->s = 0;
		}
		File(Json::Value json) {
			this->u = json["url"].asString();
			this->h = json["sha1"].asString();
			this->s = json["size"].asInt64();
			if (json.isMember("path"))
				this->p = json["path"].asString();
		}
		[[nodiscard]] std::string url() const { return this->u; }
		[[nodiscard]] std::string sha1() const { return this->h; }
		[[nodiscard]] long long size() const { return this->s; }
		[[nodiscard]] std::string path() const { return this->u; }
	};
	class Rule {
		bool a;
		std::string on;
		std::string ov;
		std::map<std::string,bool> f;
	public:
		class Feature {
			std::string name;
			bool value;
		public:
			Feature(std::string n, bool v) {
				this->name = n;
				this->value = v;
			}
			std::string getName() { return this->name; }
			bool getValue() { return this->value; }
		};
	public:
		Rule() {
			this->a = true;
			this->on = "";
			this->ov = "";
			this->f = {};
		}
		Rule(Json::Value json) {
			this->a = (json["action"] == "allow") && (json["action"] != "disallow");
			if (json.isMember("os")) {
				if (json["os"].isMember("name"))
					this->on = json["os"]["name"].asString();
				if (json["os"].isMember("version"))
					this->ov = json["os"]["version"].asString();
			}
			if (json.isMember("features")) {
				for (const std::string& i : json["features"].getMemberNames()) {
					f[i]=json["features"][i].asBool();
				}
			}
		}
		[[nodiscard]] bool isAllow(std::vector<Feature> features) const {
			bool allow;
			if (this->a) {
				if (this->on == SYS_NAME) {
					allow = 1;
				}
				else allow = 1;
			}
			if (!this->a) {
				if (this->ov == SYS_NAME) {
					allow = 0;
				}
				else allow = 1;
			}
			for (Feature& i : features) {
				if (!f.contains(i.getName())) continue;
				else {
					if (i.getValue() != i.getValue()) allow &= 0;
					else allow &= 1;
				}
			}
			return allow;
		}
	};
	class LibraryItem {
		std::string n;
		File a;
		std::map<std::string,File> cn;
		std::map<std::string,std::string> nn;
		std::vector<Rule> r;
	public:
		LibraryItem(Json::Value json) {
			this->n = json["name"].asString();
			if (json.isMember("natives")) {
				std::vector<std::string> n = json["natives"].getMemberNames();
				for (const std::string& i : n) {
					this->nn[i] = json["natives"][i].asString();
				}
			}
			if (json.isMember("downloads")) {
				if (json["downloads"].isMember("artifact")) {
					this->a = json["downloads"]["artifact"];
				}
				if (json["downloads"].isMember("classifiers")) {
					for (const auto& i : this->nn) {
						this->cn[i.second] = json["downloads"]["classifiers"][i.second];
					}
				}
			}
			if (json.isMember("rules")) {
				size_t rs = json["rules"].size();
				for (size_t i = 0; i < rs; i++) {
					r.push_back(json["rules"][(int)i]);
				}
			}
		}
		[[nodiscard]] std::string finalLibPath() {
			std::string libPath;
			if (this->a.path() != "") {
				libPath = Strings::strFormat("libraries\\%s", this->a.path().c_str());
				for (char& i : libPath) if (i == ANOTHER_PATH_SEP[0]) i = PATH_SEP[0];
			}
			else if (this->nn.size() > 0) {
				libPath = Strings::strFormat("libraries\\%s", this->cn[this->nn[SYS_NAME]].path().c_str());
				for (char& i : libPath) if (i == ANOTHER_PATH_SEP[0]) i = PATH_SEP[0];
			}
			else {
				std::vector<std::string> libNameSplit;
				libNameSplit = Strings::split(this->n, ":");
				libNameSplit.emplace(libNameSplit.begin() + 2);
				std::string libName = Strings::join(libNameSplit, ":");
				
				libNameSplit = Strings::split(this->n, ":");
				libPath = libNameSplit[0];
				for (char& i : libPath) if (i == '.') i = PATH_SEP[0];
				libNameSplit[0] = libPath;
				libPath = Strings::join(libNameSplit, PATH_SEP);
				libPath = Strings::strFormat("libraries\\%s\\%s-%s.jar",
					libPath.c_str(), libNameSplit[1].c_str(), libNameSplit[2].c_str());
				for (char& i : libPath) if (i == ANOTHER_PATH_SEP[0]) i = PATH_SEP[0];
			}
			return libPath;
		}
		[[nodiscard]] bool allow() const {
			for (const Rule& i : r) {
				if (!i.isAllow({})) {
					return false;
				}
			}
			return true;
		}
	};
	class ArgumentItem {
		std::vector<std::string> value;
		std::vector<Rule> r;
	public:
		ArgumentItem(Json::Value json) {
			if (json.type() == Json::stringValue) {
				this->value = { json.asString() };
			}
			else {
				for (int i = 0; i < json["value"].size(); i ++) {
					this->value.push_back(json["value"][i].asString());
				}
				if (json.isMember("rules")) {
					int rs = json["rules"].size();
					for (int i = 0; i < rs; i++) {
						r.push_back(json["rules"][i]);
					}
				}
			}
		}
		[[nodiscard]] bool allow(std::vector<Rule::Feature> features) const {
			for (const Rule& i : r) {
				if (!i.isAllow(features)) {
					return false;
				}
			}
			return true;
		}
	};
	VersionInfo(std::string jsonPath) {
		std::ifstream jsonFile(jsonPath);
		Json::Value info;
		Json::Reader reader;
		reader.parse(jsonFile, info);
		this->init(info);
		jsonFile.close();
	}
	VersionInfo(Json::Value info) {
		this->init(info);
	}
	~VersionInfo() {
		if (this->patches) {
			delete this->patches;
			this->patches = nullptr;
		}
	}
	std::string getId() { return this->id; }
	std::string getMainClass() { return this->mainClass; }
	std::vector<LibraryItem> getLibraries() { return this->libraries; }
	std::string getType() { return this->gameType; }
protected:
	void init(Json::Value info) {
		this->id = info["id"].asString();
		this->mainClass = info["mainClass"].asString();
		this->assetIndex = File(info["assetIndex"]);
		this->assetIndexTotalSize = info["assetIndex"]["totalSize"].asInt64();
		this->assetIndexId = info["assetIndex"]["id"].asString();
		this->complianceLevel = info["complianceLevel"].asInt();
		this->javaVersion = info["javaVersion"]["majorVersion"].asInt();
		Json::Value downloads = info["downloads"];
		if (downloads.isMember("client_mappings")) {
			this->clientMappings = File(downloads["client_mappings"]);
			this->serverMappings = File(downloads["server_mappings"]);
		}
		this->client = File(downloads["client"]);
		this->server = File(downloads["server"]);
		this->loggingFile = File(info["logging"]["client"]["file"]);
		this->loggingId = info["logging"]["client"]["file"]["id"].asString();
		this->loggingArgument = info["logging"]["client"]["argument"].asString();
		this->gameType = info["type"].asString();
		if (info.isMember("arguments")) {
			for (int i = 0; i < info["arguments"]["game"].size(); i ++) {
				this->gameArguments.push_back(info["arguments"]["game"][i]);
			}
			for (int i = 0; i < info["arguments"]["jvm"].size(); i++) {
				this->jvmArguments.push_back(info["arguments"]["game"][i]);
			}
		}
		if (info.isMember("minecraftArguments")) {
			this->legacyGameArguments = info["minecraftArguments"].asString();
			this->jvmArguments = { Json::Value("-cp"), Json::Value("${classpath}") };
		}
		if (info.isMember("patches") &&
			(info["patches"].size() > 0)) {
			this->patches = new VersionInfo(info["patches"][0]);
		}
		else this->patches = nullptr;
		for (Json::Value i : info["libraries"]) {
			this->libraries.push_back(i);
		}
	}
	std::string id;
	std::string mainClass;
	File assetIndex;
	long long assetIndexTotalSize;
	std::string assetIndexId;
	int complianceLevel;
	int javaVersion;
	File client;
	File server;
	File clientMappings;
	File serverMappings;
	File loggingFile;
	std::string loggingId;
	std::string loggingArgument;
	std::string gameType;
	std::vector<LibraryItem> libraries;
	std::vector<ArgumentItem> gameArguments;
	std::string legacyGameArguments;
	std::vector<ArgumentItem> jvmArguments;
	VersionInfo*patches;
};

#endif //RIVERLAUNCHER3_VERSIONINFO_H