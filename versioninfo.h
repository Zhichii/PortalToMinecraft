
#ifndef RIVERLAUNCHER3_VERSIONINFO_H
#define RIVERLAUNCHER3_VERSIONINFO_H

#include <vector>
#include <string>
#include <json.h>
#include <utility>
#include <windows.h>
#include <zip/unzip.h>
#include "help.h"
#include "data.h"
#include "control.h"

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
			if(json.isMember("path"))
				this->p = json["path"].asString();
		}
		[[nodiscard]] std::string url() const { return this->u; }
		[[nodiscard]] std::string sha1() const { return this->h; }
		[[nodiscard]] long long size() const { return this->s; }
		[[nodiscard]] std::string path() const { return this->p; }
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
		Rule() {
			this->a = true;
			this->on = "";
			this->ov = "";
			this->f = {};
		}
		Rule(Json::Value json) {
			this->a = (json["action"] == "allow") && (json["action"] != "disallow");
			if(json.isMember("os")) {
				if(json["os"].isMember("name"))
					this->on = json["os"]["name"].asString();
				if(json["os"].isMember("version"))
					this->ov = json["os"]["version"].asString();
			}
			if(json.isMember("features")) {
				for (const std::string& i : json["features"].getMemberNames()) {
					f[i]=json["features"][i].asBool();
				}
			}
		}
		[[nodiscard]] bool isAllow(std::vector<Feature> features) {
			bool allow = 0;
			if(this->on == SYS_NAME || this->on == "") allow = 1;
			if(!this->a) allow = !allow;
			for (auto& i : this->f) {
				bool found = false;
				bool v = false;
				for (auto& j : features) {
					if(j.getName() == i.first) {
						found = true;
						v = j.getValue();
						break;
					}
				}
				//if(i.second) {
				//	if(!found) allow &= 0;
				//	lf(v) allow &= 1;
				//	ef allow &= 0;
				//}
				//ef {
				//	if(!found) allow &= 1;
				//	lf(v) allow &= 0;
				//	ef allow &= 1;
				//}
				if(!found) allow &= !i.second;
				lf(v) allow &= i.second;
				ef allow &= !i.second;
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
			if(json.isMember("natives")) {
				std::vector<std::string> n = json["natives"].getMemberNames();
				for (const std::string& i : n) {
					this->nn[i] = json["natives"][i].asString();
				}
			}
			if(json.isMember("downloads")) {
				if(json["downloads"].isMember("artifact")) {
					this->a = json["downloads"]["artifact"];
				}
				if(json["downloads"].isMember("classifiers")) {
					for (const auto& i : this->nn) {
						this->cn[i.second] = json["downloads"]["classifiers"][i.second];
					}
				}
			}
			if(json.isMember("rules")) {
				for (const auto& i : json["rules"]) {
					this->r.push_back(i);
				}
			}
		}
		bool tryExtractNatives(std::wstring gameDir, std::wstring nativePath) {
			if(this->nn.size()==0) return false;
			std::wstring temp = gameDir + L"libraries\\" + Strings::s2ws(this->cn[this->nn[SYS_NAME]].path());
			for (auto& i : temp) {
				if(temp[i] == O_LPATHSEP[0]) temp[i] = LPATHSEP[0];
			}
			HZIP hZip = OpenZip(temp.c_str(), NULL);
			ZIPENTRY ze;
			GetZipItem(hZip, -1, &ze);
			int nums = ze.index;
			SetUnzipBaseDir(hZip, nativePath.c_str());
			for (int i = 0; i < nums; i++) {
				GetZipItem(hZip, i, &ze);
				int len = lstrlenW(ze.name);
				if(ze.name[len - 1] == L'l' && ze.name[len - 2] == L'l' && ze.name[len - 3] == L'd' && ze.name[len - 4] == L'.')
					UnzipItem(hZip, i, ze.name);
			}
			CloseZip(hZip);
			return true;
		}
		std::string libName() {
			std::vector<std::string> libNameSplit;
			libNameSplit = Strings::split(this->n, ":");
			libNameSplit.emplace(libNameSplit.begin() + 2);
			return Strings::join(libNameSplit, ":");
		}
		std::string finalLibPath() {
			std::string libPath;
			if(this->a.path() != "") {
				libPath = Strings::strFormat("libraries\\%s", this->a.path().c_str());
				for (char& i : libPath) if(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			lf(this->nn.size() > 0) {
				libPath = Strings::strFormat("libraries\\%s", this->cn[this->nn[SYS_NAME]].path().c_str());
				for (char& i : libPath) if(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			ef{
				std::vector<std::string> libNameSplit = Strings::split(this->n,":");
				libPath = this->n;
				bool f = 1;
				for (char& i : libPath) {
					if(f && i=='.') i = PATHSEP[0];
					if(i==':') {
						f = 0;
						i = PATHSEP[0];
					}
				}
				libPath = Strings::strFormat("libraries\\%s\\%s-%s.jar",
					libPath.c_str(), libNameSplit[1].c_str(), libNameSplit[2].c_str());
				for (char& i : libPath) if(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			return libPath;
		}
		bool allow(std::vector<Rule::Feature> features) {
			for (Rule& i : r) {
				if(!i.isAllow(features)) {
					return false;
				}
			}
			return true;
		}
	};
	class ArgumentItem {
		std::vector<std::string> v;
		std::vector<Rule> r;
	public:
		ArgumentItem(Json::Value json) {
			if(json.type() == Json::stringValue) {
				this->v = { json.asString() };
			}
			ef {
				for (const auto& i : json["value"]) {
					this->v.push_back(i.asString());
				}
				if(json.isMember("rules")) {
					for (const auto& i : json["rules"]) {
						this->r.push_back(i);
					}
				}
			}
		}
		[[nodiscard]] bool allow(std::vector<Rule::Feature> features) {
			for (Rule& i : this->r) {
				if(!i.isAllow(features)) {
					return false;
				}
			}
			return true;
		}
		std::vector<std::string> value() { return this->v; }
	};
	VersionInfo(std::wstring jsonPath) {
		std::ifstream jsonFile(jsonPath);
		Json::Value info;
		Json::Reader reader;
		reader.parse(jsonFile, info);
		jsonFile.close();
		this->init(info);
	}
	VersionInfo(Json::Value info) {
		this->init(info);
	}
	~VersionInfo() {
		if(this->patches!=nullptr) {
			delete this->patches;
			this->patches = nullptr;
		}
	}
	std::string getId() { return this->id; }
	std::string getMainClass() { return this->mainClass; }
	std::vector<ArgumentItem> getGameArguments() { return this->gameArguments; }
	std::vector<ArgumentItem> getJvmArguments() { return this->jvmArguments; }
	std::vector<LibraryItem> getLibraries() { return this->libraries; }
	std::string getLegacyGameArguments() { return this->legacyGameArguments; }
	std::string getType() { return this->gameType; }
	int getJavaVersion() { return this->javaVersion; }
	std::string getLoggingArgument() { return this->loggingArgument; }
	std::string getLoggingId() { return this->loggingId; }
	std::string getAssetIndexId() { return this->assetIndexId; }
public:
	std::string genLaunchCmd(std::wstring&cmd, const std::string& gameDir, const int selectedAccount, const std::vector<Rule::Feature> features) {
		writeLog("Generating launching command: %s, \"%s\". ", this->getId().c_str(), gameDir.c_str());
		std::string versionPath = "versions"; versionPath += PATHSEP + this->getId() + PATHSEP + this->getId();
		std::string nativePath = gameDir + versionPath + "-natives\\";
		if(!isDir(nativePath)) SHCreateDirectoryExA(NULL, nativePath.c_str(), NULL);
		writeLog("Generating classpath. ");
		std::string cp = this->genClasspath(gameDir, versionPath, nativePath, features);
		writeLog("Classpath is ready. ");
		std::string finalJava = this->findJava();
		writeLog("Found Java \"%s\". ", finalJava.c_str());
		writeLog("Reloging-in the account. ");
		//* Relogin the account. 
		int wid = rdata("WindowWidth").asInt(), hei = rdata("WindowHeight").asInt();
		std::map<std::string, std::string> gameVals;
		gameVals["${version_name}"]=		this->getId();
		gameVals["${game_directory}"] =		gameDir;
		gameVals["${assets_root}"]=			"assets";
		gameVals["${assets_index_name}"]=	this->getAssetIndexId();
		gameVals["${version_type}"]=		this->getType();
		gameVals["${auth_access_token}"]=	rdata("Accounts")[selectedAccount]["userToken"].asString();
		gameVals["${auth_session}"]=		rdata("Accounts")[selectedAccount]["userToken"].asString();
		gameVals["${auth_player_name}"]=	rdata("Accounts")[selectedAccount]["userName"].asString();
		gameVals["${auth_uuid}"]=			rdata("Accounts")[selectedAccount]["userId"].asString();
		gameVals["${clientId}"]=			rdata("Accounts")[selectedAccount]["userId"].asString();
		gameVals["${client_id}"]=			rdata("Accounts")[selectedAccount]["userId"].asString();
		gameVals["${user_type}"]=			(rdata("Accounts")[selectedAccount]["userType"].asString()=="mojang") ? ("msa") : ("legacy");
		gameVals["${resolution_width}"]=	std::to_string(wid);
		gameVals["${resolution_height}"]=	std::to_string(hei);
		gameVals["${natives_directory}"]=	nativePath;
		gameVals["${user_properties}"]=		"{}";
		gameVals["${classpath_separator}"]=	";";
		gameVals["${library_directory}"]=	"libraries\\";
		std::map<std::string, std::string> jvmVals;
		jvmVals["${classpath}"]=			cp;
		jvmVals["${natives_directory}"]=	nativePath;
		jvmVals["${launcher_name}"]=		"RiverLauncher";
		jvmVals["${launcher_version}"]=		"3.0.0.0";
		writeLog("Generating JVM arguments. ");
		std::string jvmArgs = this->genJvmArgs(features, jvmVals);
		writeLog("Generating game arguments. ");
		std::string gameArgs = this->genGameArgs(features, gameVals);
		if(gameArgs == "") {
			call({ "msgbx","error","minecraft.no_args","error" });
			writeLog("Failed to generate game arguments. ");
			cmd = L"";
			return "";
		}
		writeLog("Connecting the arguments. ");
		std::string output(16384,'\0');
		output = QUOT + finalJava + QUOT;
		if(Strings::count(jvmArgs, "-Djava.library.path") == 0) {
			output += " \"-Djava.library.path="+versionPath+"-natives\"";
		}
		if(this->getLoggingArgument() != "") {
			output += " " + Strings::replace(this->getLoggingArgument(),
				"${path}", std::string{}+QUOT+"versions"+PATHSEP+this->getId()+PATHSEP+this->getLoggingId()+QUOT);
		}
		output += " -Xmn"+std::to_string(4000/*//* Memory Allocation! */ )+"m -XX:+UseG1GC -XX:-UseAdaptiveSizePolicy -XX:-OmitStackTraceInFastThrow -Dlog4j2.formatMsgNoLookups=true";
		output += " " + jvmArgs + " " + this->getMainClass() + " " + gameArgs;
		cmd = Strings::s2ws(output);
		writeLog("Finish generating launching command. ");
		return output;
	}
protected:
	std::string genClasspath(const std::string& gameDir, const std::string& versionPath, const std::string& nativePath, const std::vector<Rule::Feature>& features) {
		std::map<std::string,std::string> available; // For fixing libs duplicating. 
		std::vector<std::string> libsVec;
		std::wstring gameDirW = Strings::s2ws(gameDir);
		std::wstring nativePathW = Strings::s2ws(nativePath);
		for (auto& i : this->getLibraries()) {
			if(i.allow(features)) {
				if (i.tryExtractNatives(gameDirW, nativePathW)) continue;
				available[i.libName()] = i.finalLibPath();
			}
		}
		for (const auto& i : available) {
			libsVec.push_back(i.second);
		}
		libsVec.push_back(versionPath + ".jar");
		return Strings::join(libsVec, ";");
	}
	std::string genJvmArgs(const std::vector<Rule::Feature>& features, std::map<std::string,std::string>& jvmVals) {
		std::vector<std::string> jvmArgVec;
		for (auto& i : this->getJvmArguments()) {
			if(i.allow(features)) {
				for (auto& j : i.value()) {
					std::string k = j;
					auto l = Strings::split(k, "=");
					for (auto& m : l) {
						if (jvmVals.contains(m)) m = jvmVals[m];
					}
					k = Strings::join(l, "=");
					k = Strings::replace(k, "\\", "\\\\");
					k = Strings::replace(k, "\"", "\\\"");
					if(Strings::count(k, " ")) jvmArgVec.push_back(QUOT+k+QUOT);
					ef jvmArgVec.push_back(k);
				}
			}
		}
		return Strings::join(jvmArgVec, " ");
	}
	std::string genGameArgs(const std::vector<Rule::Feature>& features, std::map<std::string, std::string>& gameVals) {
		std::string gameArg;
		bool flagOptiFineForge = false;
		bool flagForge = false;
		bool flagOptiFine = false;
		bool flag__tweakClass = false;
		if(this->getGameArguments().size() > 0) {
			std::vector<std::string> gameArgVec;
			for (auto& i : this->getGameArguments()) {
				if(i.allow(features)) {
					for (auto& j : i.value()) {
						std::string k = j;
						if(gameVals.contains(k)) k = gameVals[k];
						if(k == "--tweakClass") {
							flag__tweakClass = true;
							continue;
						}
						if(flag__tweakClass && k=="net.minecraftforge.fml.common.launcher.FMLTweaker") {
							gameArgVec.push_back("--tweakClass");
							flagForge = true;
							flag__tweakClass = false;
						}
						if(flag__tweakClass && k=="optifine.OptiFineForgeTweaker") {
							flagOptiFineForge = true;
							flag__tweakClass = false;
							continue;
						}
						if(flag__tweakClass&& k == "optifine.OptiFineForgeTweaker") {
							flagOptiFine = true;
							flag__tweakClass = false;
							continue;
						}
						k = Strings::replace(k, "\\", "\\\\");
						k = Strings::replace(k, "\"", "\\\"");
						if(Strings::count(k, " ")) gameArgVec.push_back(QUOT+k+QUOT);
						ef gameArgVec.push_back(k);
					}
				}
			}
			// If I do not do this, it seemed to be crash. 
			if((flagOptiFine&&flagForge)||flagOptiFineForge) {
				gameArgVec.push_back("--tweakClass");
				gameArgVec.push_back("optifine.OptiFineForgeTweaker");
			}
			gameArg = Strings::join(gameArgVec, " ");
		}
		lf (this->getLegacyGameArguments() != "") {
			gameArg = this->getLegacyGameArguments();
			for (const auto& i : gameVals) {
				std::string k = i.second;
				k = Strings::replace(k, "\\", "\\\\");
				k = Strings::replace(k, "\"", "\\\"");
				if(Strings::count(k, " ")) k = QUOT+k+QUOT;
				gameArg = Strings::replace(gameArg, i.first, k);
			}
			if(Strings::count(gameArg, " --tweakClass optifine.OptiFineForgeTweaker")) {
				gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineForgeTweaker", "");
				gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
			}
			lf(Strings::count(gameArg, " --tweakClass net.minecraftforge.fml.common.launcher.FMLTweaker") != 0) {
				if(Strings::count(gameArg, " --tweakClass optifine.OptiFineTweaker") != 0) {
					gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineTweaker", "");
					gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
				}
			}
		}
		ef{
			return "";
		}
		return gameArg;
	}
	std::string findJava() {
		std::string finalJava;
		int javaVersion = this->getJavaVersion();
		if(javaVersion > 17) javaVersion = 21;
		lf (javaVersion > 11) javaVersion = 17;
		ef javaVersion = 8;
		std::vector<std::string> javas;
		for (auto& i : rdata("Javas")) {
			javas.push_back(i.asString());
		}
		int selectedJava = rdata("SelectedJava").asInt();
		if(selectedJava < 0) {
			if(selectedJava == -1) {
				//* Command "where". 
			}
			bool flag = 1;
			// Get the compatible Java. 
			for (auto& java : javas) {
				std::string tmpEachJava = execGetOut("\"" + java + "\" GetJavaVersion", "RvL\\");
				std::vector<std::string> javaInfo = Strings::split(tmpEachJava, "\r\n");
				if(javaInfo[1] != "64") continue;
				int curVersion = 0;
				if(javaInfo[0][1] == '.') curVersion = atoi(javaInfo[0].c_str() + 2);
				ef curVersion = atoi(javaInfo[0].c_str());
				if(curVersion > 17) curVersion = 21;
				lf (curVersion > 11) curVersion = 17;
				ef curVersion = 8;
				if(curVersion == javaVersion) {
					finalJava = java;
					flag = 0;
					break;
				}
			}
		}
		ef {
			finalJava = javas[selectedJava];
		}
		return finalJava;
	}
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
		if(downloads.isMember("client_mappings")) {
			this->clientMappings = File(downloads["client_mappings"]);
			this->serverMappings = File(downloads["server_mappings"]);
		}
		this->client = File(downloads["client"]);
		this->server = File(downloads["server"]);
		if(info.isMember("logging")) {
			if(info["logging"].isMember("client")) {
				this->loggingFile = File(info["logging"]["client"]["file"]);
				this->loggingId = info["logging"]["client"]["file"]["id"].asString();
				this->loggingArgument = info["logging"]["client"]["argument"].asString();
			}
		}
		ef {
			this->loggingFile = {};
			this->loggingId = {};
			this->loggingArgument = {};
		}
		this->gameType = info["type"].asString();
		if(info.isMember("arguments")) {
			for (size_t i = 0; i < info["arguments"]["game"].size(); i ++) {
				this->gameArguments.push_back(info["arguments"]["game"][(int)i]);
			}
			for (size_t i = 0; i < info["arguments"]["jvm"].size(); i++) {
				this->jvmArguments.push_back(info["arguments"]["jvm"][(int)i]);
			}
		}
		if(info.isMember("minecraftArguments")) {
			this->legacyGameArguments = info["minecraftArguments"].asString();
			this->jvmArguments = { Json::Value("-cp"), Json::Value("${classpath}") };
		}
		//if(info.isMember("patches") &&
		//	(info["patches"].size() > 0)) {
		//	this->patches = new VersionInfo(info["patches"][0]);
		//}
		this->patches = nullptr;
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