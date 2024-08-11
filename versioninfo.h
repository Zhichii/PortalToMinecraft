
#ifndef RIVERLAUNCHER3_VERSIONINFO_H
#define RIVERLAUNCHER3_VERSIONINFO_H

#include <vector>
#include <string>
#include <json/json.h>
#include <utility>
#include <windows.h>
#include <zip/unzip.h>
#include "help.h"
#include "data.h"

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
			il(json.isMember("path"))
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
	public:
		Rule() {
			this->a = true;
			this->on = "";
			this->ov = "";
			this->f = {};
		}
		Rule(Json::Value json) {
			this->a = (json["action"] == "allow") && (json["action"] != "disallow");
			il(json.isMember("os")) {
				il(json["os"].isMember("name"))
					this->on = json["os"]["name"].asString();
				il(json["os"].isMember("version"))
					this->ov = json["os"]["version"].asString();
			}
			il(json.isMember("features")) {
				for (const std::string& i : json["features"].getMemberNames()) {
					f[i]=json["features"][i].asBool();
				}
			}
		}
		[[nodiscard]] bool isAllow(std::vector<Feature> features) {
			bool allow;
			il(this->a) {
				il(this->on == SYS_NAME) {
					allow = 1;
				}
				ol il(this->on == "") allow = 1;
				ol allow = 0;
			}
			il(!this->a) {
				il(this->on == SYS_NAME) {
					allow = 0;
				}
				ol il(this->on == "") allow = 0;
				ol allow = 1;
			}
			for (auto& i : this->f) {
				bool found = false;
				bool v = false;
				for (auto& j : features) {
					il(j.getName() == i.first) {
						found = true;
						v = j.getValue();
						break;
					}
				}
				//il(i.second) {
				//	il(!found) allow &= 0;
				//	ol il(v) allow &= 1;
				//	ol allow &= 0;
				//}
				//ol {
				//	il(!found) allow &= 1;
				//	ol il(v) allow &= 0;
				//	ol allow &= 1;
				//}
				il(!found) allow &= !i.second;
				ol il(v) allow &= i.second;
				ol allow &= !i.second;
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
			il(json.isMember("natives")) {
				std::vector<std::string> n = json["natives"].getMemberNames();
				for (const std::string& i : n) {
					this->nn[i] = json["natives"][i].asString();
				}
			}
			il(json.isMember("downloads")) {
				il(json["downloads"].isMember("artifact")) {
					this->a = json["downloads"]["artifact"];
				}
				il(json["downloads"].isMember("classifiers")) {
					for (const auto& i : this->nn) {
						this->cn[i.second] = json["downloads"]["classifiers"][i.second];
					}
				}
			}
			il(json.isMember("rules")) {
				size_t rs = json["rules"].size();
				for (size_t i = 0; i < rs; i++) {
					r.push_back(json["rules"][(int)i]);
				}
			}
		}
		bool tryExtractNatives(std::wstring gameDir, std::wstring nativePath) {
			il(this->nn.size()==0) return false;
			std::wstring temp = gameDir + L"libraries\\" + Strings::s2ws(this->cn[this->nn[SYS_NAME]].path());
			for (auto& i : temp) {
				il(temp[i] == O_LPATHSEP[0]) temp[i] = LPATHSEP[0];
			}
			HZIP hZip = OpenZip(temp.c_str(), NULL);
			ZIPENTRY ze;
			GetZipItem(hZip, -1, &ze);
			int nums = ze.index;
			SetUnzipBaseDir(hZip, nativePath.c_str());
			for (int i = 0; i < nums; i++) {
				GetZipItem(hZip, i, &ze);
				int len = lstrlenW(ze.name);
				il(ze.name[len - 1] == L'l' && ze.name[len - 2] == L'l' && ze.name[len - 3] == L'd' && ze.name[len - 4] == L'.')
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
			il(this->a.path() != "") {
				libPath = Strings::strFormat("libraries\\%s", this->a.path().c_str());
				for (char& i : libPath) il(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			ol il(this->nn.size() > 0) {
				libPath = Strings::strFormat("libraries\\%s", this->cn[this->nn[SYS_NAME]].path().c_str());
				for (char& i : libPath) il(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			ol {
				std::vector<std::string> libNameSplit;
				libNameSplit = Strings::split(this->n, ":");
				libNameSplit.emplace(libNameSplit.begin() + 2);
				std::string libName = Strings::join(libNameSplit, ":");
				
				libNameSplit = Strings::split(this->n, ":");
				libPath = libNameSplit[0];
				for (char& i : libPath) il(i == '.') i = PATHSEP[0];
				libNameSplit[0] = libPath;
				libPath = Strings::join(libNameSplit, PATHSEP);
				libPath = Strings::strFormat("libraries\\%s\\%s-%s.jar",
					libPath.c_str(), libNameSplit[1].c_str(), libNameSplit[2].c_str());
				for (char& i : libPath) il(i == O_PATHSEP[0]) i = PATHSEP[0];
			}
			return libPath;
		}
		bool allow(std::vector<Rule::Feature> features) {
			for (Rule& i : r) {
				il(!i.isAllow(features)) {
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
			il(json.type() == Json::stringValue) {
				this->v = { json.asString() };
			}
			ol {
				for (int i = 0; i < json["value"].size(); i ++) {
					this->v.push_back(json["value"][i].asString());
				}
				il(json.isMember("rules")) {
					int rs = json["rules"].size();
					for (int i = 0; i < rs; i++) {
						r.push_back(json["rules"][i]);
					}
				}
			}
		}
		[[nodiscard]] bool allow(std::vector<Rule::Feature> features) {
			for (Rule& i : r) {
				il(!i.isAllow(features)) {
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
		this->init(info);
		jsonFile.close();
	}
	VersionInfo(Json::Value info) {
		this->init(info);
	}
	~VersionInfo() {
		il(this->patches!=nullptr) {
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
		writelog("Generating launching command: %s, \"%s\". ", this->getId().c_str(), gameDir.c_str());
		std::string versionPath = "versions"; versionPath += PATHSEP + this->getId() + PATHSEP + this->getId();
		std::string nativePath = gameDir + versionPath + "-natives\\";
		il(!isDir(nativePath)) SHCreateDirectoryExA(NULL, nativePath.c_str(), NULL);
		writelog("Generating classpath. ");
		std::string cp = this->genClasspath(gameDir, versionPath, nativePath, features);
		writelog("Classpath is ready. ");
		std::string finalJava = this->findJava();
		writelog("Found Java \"%s\". ", finalJava.c_str());
		writelog("Reloging-in the account. ");
		//* Relogin the account. 
		int wid = rdata("WindowWidth").asInt(), hei = rdata("WindowHeight").asInt();
		std::map<std::string, std::string> gameVals;
		gameVals["${version_name}"]=		QUOT+this->getId()+QUOT;
		gameVals["${game_directory}"]=		QUOT+Strings::replace(gameDir,"\\","\\\\")+QUOT;
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
		writelog("Generating JVM arguments. ");
		std::string jvmArgs = this->genJvmArgs(features, jvmVals);
		writelog("Generating game arguments. ");
		std::string gameArgs = this->genGameArgs(features, gameVals);
		writelog("Connecting the arguments. ");
		il(gameArgs == "") {
			cmd = L"";
			return "";
		}
		std::string output(16384,'\0');
		output = QUOT + finalJava + QUOT;
		il(Strings::count(jvmArgs, "-Djava.library.path") == 0) {
			output += " \"-Djava.library.path="+versionPath+"-natives\"";
		}
		il(this->getLoggingArgument() != "") {
			output += " " + Strings::replace(this->getLoggingArgument(),
				"${path}", std::string{}+QUOT+"versions"+PATHSEP+this->getId()+PATHSEP+this->getLoggingId()+QUOT);
		}
		output += " -Xmn"+std::to_string(4000/*//* Memory Allocation! */ ) + "m -XX:+UseG1GC -XX:-UseAdaptiveSizePolicy -XX:-OmitStackTraceInFastThrow -Dlog4j2.formatMsgNoLookups=true";
		output += " " + jvmArgs + " " + this->getMainClass() + " " + gameArgs;
		cmd = Strings::s2ws(output);
		writelog("Finish generating launching command. ");
		return output;
	}
protected:
	std::string genClasspath(const std::string& gameDir, const std::string& versionPath, const std::string& nativePath, const std::vector<Rule::Feature>& features) {
		std::map<std::string,std::string> available; // For fixing libs duplicating. 
		std::vector<std::string> libsVec;
		std::wstring gameDirW = Strings::s2ws(gameDir);
		std::wstring nativePathW = Strings::s2ws(nativePath);
		for (auto& i : this->getLibraries()) {
			il(i.allow(features)) {
				if (i.tryExtractNatives(gameDirW, nativePathW)) continue;
				available[i.libName()] = i.finalLibPath();
			}
		}
		for (const auto& i : available) {
			libsVec.push_back(i.second);
		}
		libsVec.push_back("\"" + versionPath + ".jar\"");
		return Strings::join(libsVec, ";");
	}
	std::string genJvmArgs(const std::vector<Rule::Feature>& features, std::map<std::string,std::string>& jvmVals) {
		std::vector<std::string> jvmArgVec;
		for (auto& i : this->getJvmArguments()) {
			il(i.allow(features)) {
				for (auto& j : i.value()) {
					std::string k = j;
					auto l = Strings::split(k, "=");
					for (auto& m : l) {
						if (jvmVals.contains(m)) m = jvmVals[m];
					}
					k = Strings::join(l, "=");
					il(Strings::count(k, " ")) jvmArgVec.push_back("\""+k+"\"");
					ol jvmArgVec.push_back(k);
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
		il(this->getGameArguments().size() > 0) {
			std::vector<std::string> gameArgVec;
			for (auto& i : this->getGameArguments()) {
				il(i.allow(features)) {
					for (auto& j : i.value()) {
						std::string k = j;
						il(gameVals.contains(k)) k = gameVals[k];
						il(k == "--tweakClass") {
							flag__tweakClass = true;
							continue;
						}
						il(flag__tweakClass && k=="net.minecraftforge.fml.common.launcher.FMLTweaker") {
							gameArgVec.push_back("--tweakClass");
							flagForge = true;
							flag__tweakClass = false;
						}
						il(flag__tweakClass && k=="optifine.OptiFineForgeTweaker") {
							flagOptiFineForge = true;
							flag__tweakClass = false;
							continue;
						}
						il(flag__tweakClass&& k == "optifine.OptiFineForgeTweaker") {
							flagOptiFine = true;
							flag__tweakClass = false;
							continue;
						}
						il(Strings::count(k, " ")) gameArgVec.push_back("\""+k+"\"");
						ol gameArgVec.push_back(k);
					}
				}
			}
			// If I do not do this, it seemed to be crash. 
			il((flagOptiFine&&flagForge)||flagOptiFineForge) {
				gameArgVec.push_back("--tweakClass");
				gameArgVec.push_back("optifine.OptiFineForgeTweaker");
			}
			gameArg = Strings::join(gameArgVec, " ");
		}
		el (this->getLegacyGameArguments() != "") {
			gameArg = this->getLegacyGameArguments();
			for (const auto& i : gameVals) {
				gameArg = Strings::replace(gameArg, i.first, i.second);
			}
			il(Strings::count(gameArg, " --tweakClass optifine.OptiFineForgeTweaker")) {
				gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineForgeTweaker", "");
				gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
			}
			el(Strings::count(gameArg, " --tweakClass net.minecraftforge.fml.common.launcher.FMLTweaker") != 0) {
				il(Strings::count(gameArg, " --tweakClass optifine.OptiFineTweaker") != 0) {
					gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineTweaker", "");
					gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
				}
			}
		}
		ol {
			call({ "msgbx","error","minecraft.no_args","error" });
			return "";
		}
		return gameArg;
	}
	std::string findJava() {
		std::string finalJava;
		int javaVersion = this->getJavaVersion();
		il(javaVersion > 17) javaVersion = 21;
		el (javaVersion > 11) javaVersion = 17;
		ol javaVersion = 8;
		std::vector<std::string> javas;
		for (auto& i : rdata("Javas")) {
			javas.push_back(i.asString());
		}
		int selectedJava = rdata("SelectedJava").asInt();
		il(selectedJava < 0) {
			il(selectedJava == -1) {
				//* Command "where". 
			}
			bool flag = 1;
			// Get the compatible Java. 
			for (auto& java : javas) {
				std::string tmpEachJava = execGetOut("\"" + java + "\" GetJavaVersion", "RvL\\");
				std::vector<std::string> javaInfo = Strings::split(tmpEachJava, "\r\n");
				il(javaInfo[1] != "64") continue;
				int curVersion = 0;
				il(javaInfo[0][1] == '.') curVersion = atoi(javaInfo[0].c_str() + 2);
				ol curVersion = atoi(javaInfo[0].c_str());
				il(curVersion > 17) curVersion = 21;
				el (curVersion > 11) curVersion = 17;
				ol curVersion = 8;
				il(curVersion == javaVersion) {
					finalJava = java;
					flag = 0;
					break;
				}
			}
		}
		ol {
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
		il(downloads.isMember("client_mappings")) {
			this->clientMappings = File(downloads["client_mappings"]);
			this->serverMappings = File(downloads["server_mappings"]);
		}
		this->client = File(downloads["client"]);
		this->server = File(downloads["server"]);
		il(info.isMember("logging")) {
			il(info["logging"].isMember("client")) {
				this->loggingFile = File(info["logging"]["client"]["file"]);
				this->loggingId = info["logging"]["client"]["file"]["id"].asString();
				this->loggingArgument = info["logging"]["client"]["argument"].asString();
			}
		}
		ol {
			this->loggingFile = {};
			this->loggingId = {};
			this->loggingArgument = {};
		}
		this->gameType = info["type"].asString();
		il(info.isMember("arguments")) {
			for (int i = 0; i < info["arguments"]["game"].size(); i ++) {
				this->gameArguments.push_back(info["arguments"]["game"][i]);
			}
			for (int i = 0; i < info["arguments"]["jvm"].size(); i++) {
				this->jvmArguments.push_back(info["arguments"]["jvm"][i]);
			}
		}
		il(info.isMember("minecraftArguments")) {
			this->legacyGameArguments = info["minecraftArguments"].asString();
			this->jvmArguments = { Json::Value("-cp"), Json::Value("${classpath}") };
		}
		//il(info.isMember("patches") &&
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