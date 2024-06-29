
#ifndef RIVERLAUNCHER3_LAUNCH_H
#define RIVERLAUNCHER3_LAUNCH_H
#include <string>
#include <utility>
#include "help.h"
#include "data.h"
#include <windows.h>
#include <zip/unzip.h>

int launchInstance(std::string& output, const std::string& game_dir, VersionInfo*instInfo, const int selected_account_index, const std::vector<Feature> features) {
	std::string version_id = instInfo->getId();

	// Get libraries. 
	std::vector<std::string> libsVec;
	for (auto& i : instInfo->getLibraries()) {
		libsVec.push_back(i.finalLibPath());
	}
	libsVec.push_back("\"versions\\" + version_id + "\\" + version_id + ".jar\"");
	std::string libsAll = Strings::join(libsVec, ";");

	// Get arguments. 
	std::string gameArg;
	std::string jvmArg;
	if (info.isMember("arguments")) {
		std::vector<std::string> gameArgVec;
		std::vector<std::string> jvmArgVec;
		for (Json::Value i : info["arguments"]["game"]) {
			if (i.isString()) {
				if (Strings::count(i.asString(), " ") > 0) {
					gameArgVec.push_back("\"" + i.asString() + "\"");
				}
				else gameArgVec.push_back(i.asString());
			}
			else {
				Json::Value temp = i["rules"][0]["features"];
				int cnt = 0;
				std::map<std::string, char> rules;
				for (const auto& i : temp) {
					rules[i.asString()] = 0;
					cnt++;
				}
				int allow = 0;
				for (const auto& i : features) {
					if (rules.contains(i.getName())) allow++;
				}
				if (allow < cnt) continue;
				temp = i["value"];
				if (temp.isString()) {
					if (Strings::count(temp.asString(), " ") != 0) {
						gameArgVec.push_back(Strings::strFormat("\"%s\"", temp.asString().c_str()));
					}
					else gameArgVec.push_back(temp.asString());
				}
				else {
					for (Json::Value j : temp) {
						if (Strings::count(j.asString(), " ") != 0) {
							gameArgVec.push_back(Strings::strFormat("\"%s\"", j.asString().c_str()));
						}
						else gameArgVec.push_back(j.asString());
					}
				}
			}
		}
		for (Json::Value i : info["arguments"]["jvm"]) {
			if (i.isString()) {
				if (Strings::count(i.asString(), " ") > 0) {
					jvmArgVec.push_back("\"" + i.asString() + "\"");
				}
				else jvmArgVec.push_back(i.asString());
			}
			else {
				if (i.isMember("rules")) {
					bool allow = 0;
					for (Json::Value j : i["rules"]) {
						if (j["action"].asString() == "allow") {
							if (j.isMember("os")) {
								if (j["os"]["name"].asString() == "windows") {
									allow = 1;
								}
							}
							else allow = 1;
						}
						if (j["action"].asString() == "disallow") {
							if (j.isMember("os")) {
								if (j["os"]["name"].asString() == "windows") {
									allow = 0;
								}
							}
							else allow = 0;
						}
					}
					if (!allow) continue;
				}
				Json::Value temp = i["value"];
				if (temp.isString()) {
					if (Strings::count(temp.asString(), " ") != 0) {
						jvmArgVec.push_back(Strings::strFormat("\"%s\"", temp.asString().c_str()));
					}
					else jvmArgVec.push_back(temp.asString());
				}
				else {
					for (Json::Value j : temp) {
						if (Strings::count(j.asString(), " ") != 0) {
							jvmArgVec.push_back(Strings::strFormat("\"%s\"", j.asString().c_str()));
						}
						else jvmArgVec.push_back(j.asString());
					}
				}
			}
		}
		gameArg = Strings::join(gameArgVec, " ");
		jvmArg = Strings::join(jvmArgVec, " ");
	}
	else if (info.isMember("minecraftArguments")) {
		gameArg = info["minecraftArguments"].asString();
		jvmArg = "-cp " + libsAll;
	}
	else {
		//* message(localize("error"), localize("prompt.mcje.noArgs"));
		return 1;
	}

	// Get Java. 
	int javaVersion = info["javaVersion"]["majorVersion"].asInt();
	if (javaVersion > 11) javaVersion = 17;
	else javaVersion = 8;
	std::string selectedJava;
	selectedJava = rdata("Java").asString();
	if (selectedJava == "") {
		//*Auto find Java. 
	}

	//*Relogin account. 

	// Output the command.  
	output = "";
	output += "\"" + selectedJava + "\" \"-Dminecraft.client.jar=versions\\" + version_id + "\\" + version_id + ".jar\\\\\"";
	if (Strings::count(jvmArg, "-Djava.library.path") == 0) {
		output += " \"-Djava.library.path=versions\\" + version_id + "\\" + version_id + "-natives\\\\\"";
	}
	if (info.isMember("logging")) {
		if (info["logging"].isMember("client")) {
			output += " " + Strings::replace(info["logging"]["client"]["argument"].asString(),
					"${path}", 
						"\"versions\\" + version_id + "\\" + 
						info["logging"]["client"]["file"]["id"].asString() + "\""
				);
		}
	}
	// Fix Optifine & Forge tweaking problems. 
	if (Strings::count(gameArg, " --tweakClass optifine.OptiFineForgeTweaker") != 0) {
		gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineForgeTweaker", "");
		gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
	}
	else if (Strings::count(gameArg, " --tweakClass net.minecraftforge.fml.common.launcher.FMLTweaker") != 0) {
		if (Strings::count(gameArg, " --tweakClass optifine.OptiFineTweaker") != 0) {
			gameArg = Strings::replace(gameArg, " --tweakClass optifine.OptiFineTweaker", "");
			gameArg += " --tweakClass optifine.OptiFineForgeTweaker";
		}
	}

	int wid = rdata("WindowWidth").asInt(), hei = rdata("WindowHeight").asInt();
	gameArg = Strings::replace(gameArg, "${auth_player_name}", rdata("Accounts")[selected_account_index]["userName"].asString());
	gameArg = Strings::replace(gameArg, "${version_name}", "\"" + version_id + "\"");
	gameArg = Strings::replace(gameArg, "${game_directory}", "\"" + Strings::replace(game_dir, "\\", "\\\\") + "\"");
	gameArg = Strings::replace(gameArg, "${assets_root}", "assets\\");
	gameArg = Strings::replace(gameArg, "${assets_index_name}", info["assets"].asString());
	gameArg = Strings::replace(gameArg, "${auth_uuid}", rdata("Accounts")[selected_account_index]["userId"].asString());
	gameArg = Strings::replace(gameArg, "${auth_access_token}", rdata("Accounts")[selected_account_index]["userToken"].asString());
	gameArg = Strings::replace(gameArg, "${auth_session}", rdata("Accounts")[selected_account_index]["userToken"].asString());
	gameArg = Strings::replace(gameArg, "${user_type}", (rdata("Accounts")[selected_account_index]["usrType"].asString() == "please_support") ? ("legacy") : ("msa"));
	gameArg = Strings::replace(gameArg, "${clientId}", rdata("Accounts")[selected_account_index]["userId"].asString());
	gameArg = Strings::replace(gameArg, "${version_type}", info["type"].asString());
	gameArg = Strings::replace(gameArg, "${resolution_width}", std::to_string(wid));
	gameArg = Strings::replace(gameArg, "${resolution_height}", std::to_string(hei));
	gameArg = Strings::replace(gameArg, "${natives_directory}", Strings::ws2s(nativePath));
	gameArg = Strings::replace(gameArg, "${user_properties}", "{}");
	gameArg = Strings::replace(gameArg, "${client_id}", "00000000402b5328");
	gameArg = Strings::replace(gameArg, "${classpath_separator}", ";");
	gameArg = Strings::replace(gameArg, "${library_directory}", "libraries\\");
	jvmArg = Strings::replace(jvmArg, "${classpath}", libsAll);
	jvmArg = Strings::replace(jvmArg, "${natives_directory}", Strings::ws2s(nativePath));
	jvmArg = Strings::replace(jvmArg, "${launcher_name}", "RiverLauncher2");
	jvmArg = Strings::replace(jvmArg, "${launcher_version}", "3.0.0.0");
	output += " " + jvmArg + " " + info["mainClass"].asString() + " " + gameArg;
	if (Strings::count(gameArg, "--width") == 0 && 0) {
		output += Strings::strFormat(" --width %d --height %d", wid, hei);
	}
	return 0;
}

#endif //RIVERLAUNCHER3_LAUNCH_H
