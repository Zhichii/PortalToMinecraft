#define STATIC_BUILD
#define TCL_USE_STATIC_PACKAGES
#define CPP_TKINTER_SAFE_FUNCTIONS
#define _CRTDBG_MAP_ALLOC
#define CURL_STATICLIB
#define TCL_THREADS
#define il if
#define el else if
#define ol else
#include <stdlib.h>
#include <crtdbg.h>
#include <tcl.h>
#include <dwmapi.h>
#include <direct.h>
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>
#include <tk.h>
#include <tkPlatDecls.h>
#include <libpng/png.h>
#include <mutex>
#include <random>
#include <curl/curl.h>

FILE* logFile;
void writeLog(std::string format, ...) {
	va_list args;
	va_start(args, format);
	char* temp = new char[16384];
	_vsprintf_s_l(temp, 16384, format.c_str(), NULL, args);
	fprintf_s(logFile, "%s\n", temp);
	delete[] temp;
	fflush(logFile);
	va_end(args);
}

#define PATHSEP "\\"
#define DPATHSEP "\\\\"
#define LPATHSEP L"\\"
#define O_PATHSEP "/"
#define O_DPATHSEP "//"
#define O_LPATHSEP L"/"
#define QUOT "\""
#include <json.h>
#include "help.h"
#include "strings.h"
#include "data.h"
#include "language.h"
#include "control.h"
#include "versioninfo.h"
#include "net.h"
#include "thread.h"

int readPNG(png_bytep png_file_data, size_t file_size, png_uint_32& width, png_uint_32& height, png_uint_32& bytesPerRow, png_bytepp output) {
	png_bytep stream = png_file_data;
	bool notpng = png_sig_cmp(png_file_data, 0, file_size);
	if (notpng) {
		return 1;
	}
	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING, 
		(png_voidp)NULL, NULL, NULL);
	if (!png_ptr) {
		return 2;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return 3;
	}
	png_set_read_fn(png_ptr, &stream, [](png_structp png_ptr, png_bytep output, size_t sz) {
		png_byte** io_ptr = (png_byte**)png_get_io_ptr(png_ptr);
		if (io_ptr == NULL)
			return;
		memcpy(output, *io_ptr, sz);
		(*io_ptr) += sz;
	});
	png_read_info(png_ptr, info_ptr);
	int bit_depth = 0;
	int color_type = -1;
	png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
		&width,
		&height,
		&bit_depth,
		&color_type,
		NULL, NULL, NULL);
	if (retval != 1) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 4;
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if (png_get_valid(png_ptr, info_ptr,
		PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
	(*output) = new png_byte[bytesPerRow * height];
	for (int i = 0; i < height; i++) {
		png_read_row(png_ptr, (*output)+bytesPerRow*i, NULL);
	}
	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	return 0;
}

const std::string contentPaddingStr = "80 0";
const int comeAniSz = 11;
const char* comeAnim[comeAniSz] = { "-680","-560","-320","-200","-40","60","110","130","110","95","80" };
const int goAniSz = 12;
const char* goAnim[goAniSz] = { "80","110","170","95","85","65","60","-40","-200","-320","-560","-680" };
const int animStepWait = 1;
static std::mutex doingAnim;
void swiPage(std::string page, bool noAnimation) {
	if (page==pageCur) return;
	if (!doingAnim.try_lock()) return;
	std::string x,x2;
	x = call({ "winfo","x",pageCur });
	call({ "pack",page }); call({ "update" });
	x2 = call({ "winfo","x",page });
	call({ "pack","forget",page }); call({ "update" });
	if (!noAnimation) {
		for (int i = 0; i < goAniSz; i ++) {
			call({ "place",pageCur,"-x",x,"-y",goAnim[i] });
			call({ "update" });
			Tcl_Sleep(animStepWait);
		}
		Tcl_Sleep(125);
	}
	call({ "place","forget",pageCur });
	call({ "update" });
	pageCur = page;
	if (!noAnimation) {
		for (int i = 0; i < comeAniSz; i ++) {
			call({ "place",pageCur,"-x",x2,"-y",comeAnim[i] });
			call({ "update" });
			Tcl_Sleep(animStepWait);
		}
	}
	call({ "place","forget",pageCur });
	call({ "pack",pageCur,"-pady",contentPaddingStr });
	doingAnim.unlock();
}

void fillColor(std::string rt) {
	call({ rt,"config","-background",primaryColor });
	std::string x = call({ rt,"config","-image" });
	std::string y = call({ rt,"config","-compound" });
	il(x != "unknown option \"-image\"" && x != "-image image Image {} {}") {
		il(Strings::count(y, "center") == 0) call({ rt,"config","-foreground",textColor });
	} ol call({ rt,"config","-foreground",textColor });
	il(Strings::count(y,"center")==0) call({ rt,"config","-foreground",textColor });
	std::string a = call({ "winfo","children",rt });
	if (a == "") return;
	std::vector<std::string> children = Strings::split(a, " ");
	for (const auto& i : children) fillColor(i);
}

void colorUpdate() {
	fillColor(ROOT);
	for (const auto& i : colorUpdates) {
		i.second(clrUpdCd[i.first]);
	}
}

std::vector<std::string> listVersion() {
	std::vector<std::string> versions;
	std::string versionDir = rdata("GameDir").asString() + "versions";
	il(isDir(versionDir)) {
		auto iter = std::filesystem::directory_iterator::directory_iterator(versionDir);
		for (auto& v : iter) {
			std::wstring fileName = v.path().filename().wstring();
			std::wstring jsonPath = Strings::s2ws(versionDir) + LPATHSEP + fileName + LPATHSEP + fileName + L".json";
			il(!isExists(jsonPath)) {
				writeLog("Listing versions: File \"%s\" does not exist. ", Strings::ws2s(jsonPath).c_str());
				continue;
			}
			try {
				std::ifstream jsonFile(jsonPath);
				Json::Value info;
				Json::Reader reader;
				reader.parse(jsonFile, info);
				std::string pre = "?:";
				il(info["type"] == "release") pre = "r:";
				el (info["type"] == "snapshot") pre = "s:";
				ol pre = "x:";
				versions.push_back(pre + info["id"].asString());
				jsonFile.close();
				writeLog("Initialized version \"%s\". ", info["id"].asCString());
			}
			catch (std::exception e) {
				writeLog("Listing versions: Failed to initialize \"%s\": %s", Strings::ws2s(jsonPath).c_str(), e.what());
			}
		}
	}
	return versions;
}

int launchGame(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::thread thr([argv]()->int {
		std::wstring av = Strings::s2ws(argv[1]);
		writeLog("Launching game \"%s\"...", argv[1]);
		VersionInfo* target = nullptr;
		std::string versionDir = rdata("GameDir").asString() + "versions";
		std::wstring jsonPath = Strings::s2ws(versionDir) + LPATHSEP + av + LPATHSEP + av + L".json";
		il(!isExists(jsonPath)) {
			writeLog("Launching: File \"%s\" does not exist. ", Strings::ws2s(jsonPath).c_str());
			target = nullptr;
		}
		else {
			try {
				target = new VersionInfo(jsonPath);
			}
			catch (std::exception e) {
				writeLog("Launching: Failed to initialize json \"%s\": %s", Strings::ws2s(jsonPath).c_str(), e.what());
				delete target;
				target = nullptr;
			}
		}
		il(target == nullptr) {
			call({ "msgbx","error","minecraft.unable","error" });
			return 0;
		}
		std::wstring cmd;
		std::vector<VersionInfo::Rule::Feature> features;
		if (rdata("WindowHeight").asInt() * rdata("WindowWidth").asInt()) {
			features.push_back({ "has_custom_resolution",true });
		}
		std::string cmdA = target->genLaunchCmd(cmd, rdata("GameDir").asString(), rdata("SelectedAccount").asInt(), features);
		delete target;
		il(cmdA == "") return 0;
		writeLog("Launch: %s", cmdA.c_str());
		std::map<std::string,std::string> state = {};
		execNotThrGetOutInvoke(cmd, &state, Strings::s2ws(rdata("GameDir").asString()), [](const std::string& o, void* r)->int {
			il(r == nullptr) return 1;
			std::map<std::string,std::string>& record = *(std::map<std::string,std::string>*)r;
			il(o.starts_with("---- Minecraft Crash Report ----"))
				record["crashReport"] = "1";
			il(o.starts_with("#@!@# Game crashed! Crash report saved to: #@!@#"))
				record["reportSaved"] = "1";
			il(Strings::slice(o, 11) == "[Client thread/INFO]: Stopping!")
				record["normalStopping"] = "1";
			il(Strings::slice(o, 13).starts_with("[main] ERROR FabricLoader/")) {
				record["isFabric"] = "1";
				record["fabricError"] = Strings::between(o, "[main] ERROR FabricLoader/", "\r");
			}
			il(Strings::slice(o, 13).starts_with("[main] ERROR FabricLoader - ")) {
				record["isFabric"] = "1";
				record["fabricError"] = Strings::between(o, "[main] ERROR FabricLoader - ", "\r");
			}
			il(o == "") record["lastSpace"] = "1";
			il(record.contains("isFabric")) {
				std::string error = Strings::strFormat(currentLanguage->localize("minecraft.fabric_crash"),
					record["fabricError"]).c_str();
				call({ "msgbx","error","<text>","error", error });
				record = {};
			}
			il(record.contains("crashReport") &&
			    record.contains("reportSaved") &&
			    !record.contains("normalStopping")) {
				call({ "msgbx","error","minecraft.crash","error" });
				record = {};
			}
			return 0;
			});
		});
	thr.detach();
	call({ "msgbx","prompt","minecraft.loading","info" });
	Tcl_Sleep(50); // Add a delay for the thread to copy argv[1]. 
	return 0;
}

int pageGame(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::vector<std::string> versions = listVersion();
	MyList* versionList = (MyList*)clientData;
	versionList->clear();
	size_t n = 0;
	for (auto& i : versions) {
		std::string image;
		il(i[0]=='r') image = "releaseImage";
		il(i[0]=='s') image = "snapshotImage";
		il(i[0]=='x') image = "brokenImage";
		std::string id = Strings::slice(i, 2);
		versionList->add(id, image, {"launchImage","launch "+id,"editImage",""});
		n++;
	}
	versionList->yview("", 0, 0);
	if (argc == -1) return 0;
	swiPage(".pageGame", argc == -2);
	return 0;
}

int pageDown(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	if (argc == -1) return 0;
	swiPage(".pageDown", argc == -2);
	return 0;
}

int pageMods(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	if (argc == -1) return 0;
	swiPage(".pageMods", argc == -2);
	return 0;
}

int addAccount(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	msgBxs++;
	std::string name = ".window" + std::to_string(msgBxs);
	messageBoxes.push_back(name);
	control(name, "toplevel");
	std::string title = "dialog";
	std::string content = "accounts.add.ask_type";
	call({ "wm","title",name,currentLanguage->localize(title) });
	control(name + ".text", "ttk::label", { "-text",currentLanguage->localize(content) });
	control(name + ".ok", "ttk::button", { "-text",currentLanguage->localize("ok") });
	call({ name + ".ok","config","-command","destroy " + name });
	call({ "grid",name + ".text" });
	call({ "grid",name + ".ok" });
	fillColor(name);
	return 0;
}

int selectAccount(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	size_t selected = std::atoi(argv[1]);
	rdata("SelectedAccount") = selected;
	((MyList*)clientData)->userSelect(selected);
	saveData();
	return 0;
}

int pageAcco(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	Json::Value accounts = rdata("Accounts");
	MyList* accountList = (MyList*)clientData;
	accountList->clear();
	size_t n = 0;
	for (const auto& i : accounts) {
		writeLog("UT: %s. ", i["userType"].asCString());
		il(i["userType"].asString() == "mojang") {
			accountList->add(i["userName"].asString()+"\n      "+currentLanguage->localize("accounts.microsoft"), "<canvas>", {"editImage",""});
			png_byte* png_file;
			size_t file_size;
			{
				FILE* fp = fopen("assets\\test_skin.png", "rb");
				fseek(fp, 0, SEEK_END);
				fpos_t sz2; fgetpos(fp, &sz2);
				file_size = sz2;
				png_file = new png_byte[file_size];
				fseek(fp, 0, SEEK_SET);
				fread(png_file, 1, file_size, fp);
				fclose(fp);
			}
			png_uint_32 width;
			png_uint_32 height;
			png_uint_32 bytesPerRow;
			png_byte* data;
			int a = readPNG(png_file, file_size, width, height, bytesPerRow, &data);
			if (a == 0) {
				writeLog("Started to paint canvas. ");
				std::string clr = "#xxxxxx";
				for (int i = 8; i < 16; i++) {
					for (int j = 8; j < 16; j++) {
						clr[1] = toHex(data[i*bytesPerRow+j*4+0]/16 % 16);
						clr[2] = toHex(data[i*bytesPerRow+j*4+0]/1 % 16);
						clr[3] = toHex(data[i*bytesPerRow+j*4+1]/16 % 16);
						clr[4] = toHex(data[i*bytesPerRow+j*4+1]/1 % 16);
						clr[5] = toHex(data[i*bytesPerRow+j*4+2]/16 % 16);
						clr[6] = toHex(data[i*bytesPerRow+j*4+2]/1 % 16);
						call({ accountList->owned[accountList->owned.size()-1].ctrlId+".image","create","rect",
							std::to_string(2+(i-8)*10),   std::to_string(2+(j-8)*10),
							std::to_string(2+(i-8)*10+10),std::to_string(2+(j-8)*10+10),
							"-fill",clr,"-outline","" });
					}
				}
				writeLog("Successfully painted! ");
			}
			else {
				writeLog("Error when trying to paint [%s]: %d", accountList->owned.tail->value.ctrlId, a);
			}
			delete[] data;
			delete[] png_file;
		}
		ol{
			accountList->add(i["userName"].asString()+"\n      "+currentLanguage->localize("accounts.legacy"), "steveImage", {"editImage",""});
		}
		accountList->bind(accountList->owned.size()-1, "<Button-1>", "selectAccount "+std::to_string(accountList->owned.size()-1));
	}
	accountList->select(rdata("SelectedAccount").asInt());
	accountList->yview("", 0, 0);
	if (argc == -1) return 0;
	swiPage(".pageAcco", argc == -2);
	return 0;
}

int pageSett(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::thread thr([](){
		//FolderDialog();
		});
	Tk_PhotoHandle ph = Tk_FindPhoto(interp, "tabImage");
	Tk_PhotoImageBlock pib;
	Tk_PhotoGetImage(ph, &pib);
	Tk_PhotoPutBlock(interp, ph, &pib, 0, 0, 30, 10, TK_PHOTO_COMPOSITE_OVERLAY);
	thr.detach();
	if (argc == -1) return 0;
	swiPage(".pageSett", argc == -2);
	return 0;
}

int selectLanguage(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	MyList* languageList = (MyList*)clientData;
	il(argc == 1) {
		rdata("Language") = "auto";
		languageList->userSelect(0);
		currentLanguage = allLanguages[getDefaultLanguage()];
	}
	ol {
		rdata("Language") = argv[1];
		languageList->userSelect(atoi(argv[2]));
		currentLanguage = allLanguages[argv[1]];
	}
	saveData();
	for (const auto& i : currentLanguage->lang)
		call({ "set",i.first,i.second });
	call({ "set","none","" });
	call({ "wm","title",ROOT,currentLanguage->localize("title") });
	return 0;
}

int pageLang(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	((MyList*)clientData)->update();
	((MyList*)clientData)->yview("", 0, 0);
	if (argc == -1) return 0;
	swiPage(".pageLang", argc == -2);
	return 0;
}

Tk_Window mainWin;
std::string geometry;
Tcl_ThreadId mainThr;

int main() {

	if (mkdir("RvL\\") != 0 && errno == ENOENT) {
		MessageBoxA(nullptr, "Failed to make dir [RvL\\]. ", "Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	logFile = fopen("RvL\\log.txt", "w");
	tclScriptLog = fopen("RvL\\tcl.txt", "w");
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Initialize Tcl/Tk. 
	writeLog("Initializing Tcl/Tk. ");
	interp = Tcl_CreateInterp();
	Tcl_Init(interp);
	Tk_Init(interp);

	// Initialize libcurl. 
	writeLog("Initializing cURL. ");
	curl_global_init(0);

	// Initialize for ATL. 
	CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED);

	{
		HRSRC hRsrc = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_JAVACLASS_GETJAVAVERSION), "javaclass");
		HGLOBAL IDR = LoadResource(NULL, hRsrc);
		DWORD size = SizeofResource(NULL, hRsrc);
		FILE* javaClass = fopen("RvL\\GetJavaVersion.class", "wb");
		fwrite(LockResource(IDR), sizeof(char), size, javaClass);
		fclose(javaClass);
		FreeResource(IDR);
	}

	writeLog("Initialize other things. ");
	initLanguages();
	initData();
	// Set the default language. 
	{
		std::string t = rdata("Language").asString();
		il(!allLanguages.contains(t)) t = getDefaultLanguage();
		currentLanguage = allLanguages[t];
	}

	for (const auto& i : currentLanguage->lang)
		call({ "set",i.first,i.second });
	call({ "set","none","" });

	call({ "wm","iconbitmap",ROOT,"assets\\icon.ico" });
	call({ "wm","title",ROOT,currentLanguage->localize("title") });
	HWND hwnd = GetActiveWindow();
	BOOL value = TRUE;
	mainWin = Tk_MainWindow(interp);
	writeLog("%lld %lld %lld", hwnd, Tk_GetHWND(Tk_WindowId(mainWin)), Tk_WindowId(mainWin));
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	call({ "wm","geometry",ROOT,rdata("Geometry").asString() });

	writeLog("Creating images. ");
	// Icons. 
	call({ "image","create","photo","releaseImage","-file","assets\\icon\\release.png" });
	call({ "image","create","photo","snapshotImage","-file","assets\\icon\\snapshot.png" });
	call({ "image","create","photo","oldImage","-file","assets\\icon\\old.png" });
	call({ "image","create","photo","fabricImage","-file","assets\\icon\\fabric.png" });
	call({ "image","create","photo","optifineImage","-file","assets\\icon\\optifine.png" });
	call({ "image","create","photo","titleImage","-file","assets\\icon.png" });
	// Controls v1. 
	call({ "image","create","photo","buttonImage","-file","assets\\control\\button.png" });
	call({ "image","create","photo","buttonImageActive","-file","assets\\control\\buttonActive.png" });
	// Controls. 
	call({ "image","create","photo","tabImage","-file","assets\\ctrl\\tab.png" });
	call({ "image","create","photo","tabImageActive","-file","assets\\ctrl\\tabActive.png" });
	call({ "image","create","photo","tabYImage","-file","assets\\ctrl\\tabY.png" });
	call({ "image","create","photo","tabYImageActive","-file","assets\\ctrl\\tabYActive.png" });
	// Icon Buttons. 
	call({ "image","create","photo","launchImage","-file","assets\\iconbutton\\launch.png" });
	call({ "image","create","photo","launchImageActive","-file","assets\\iconbutton\\launch_active.png" });
	call({ "image","create","photo","editImage","-file","assets\\iconbutton\\edit.png" });
	call({ "image","create","photo","editImageActive","-file","assets\\iconbutton\\edit_active.png" });
	call({ "image","create","photo","addImage","-file","assets\\iconbutton\\add.png" });
	call({ "image","create","photo","addImageActive","-file","assets\\iconbutton\\add_active.png" });
	call({ "image","create","photo","brightImage","-file","assets\\iconbutton\\bright.png" });
	call({ "image","create","photo","brightImageActive","-file","assets\\iconbutton\\bright_active.png" });
	call({ "image","create","photo","darkImage","-file","assets\\iconbutton\\dark.png" });
	call({ "image","create","photo","darkImageActive","-file","assets\\iconbutton\\dark_active.png" });
	// Misc. 
	call({ "image","create","photo","blankImage","-file","assets\\misc\\blank.png" });
	call({ "image","create","photo","horizontalImage","-file","assets\\misc\\horizontal.png" });
	call({ "image","create","photo","steveImage","-file","assets\\misc\\steve.png" });
	// Font
	if (AddFontResourceExA("assets\\font.otf", FR_PRIVATE, 0) != 0) {
		writeLog("Font successfully added. ");
	}
	else writeLog("Font adding failed. ");
	call({ "font","create","font1","-family","Source Han Sans CN","-size","10","-weight","bold" });

	primaryColor = "#1f1f1f";
	secondaryColor = "#000000";
	textColor = "#ffffff";
	hoverColor = "#4c4c4c";
	selectColor = "#353535";
	CreateCmd("chTheme", [](ClientData clientData,
		Tcl_Interp* interp, int argc, const char* argv[])->int {
			il(primaryColor == "#e6e6e6") {
				primaryColor = "#1f1f1f";
				secondaryColor = "#000000";
				textColor = "#ffffff";
				hoverColor = "#4c4c4c";
				selectColor = "#353535";
			}
			ol {
				primaryColor = "#e6e6e6";
				secondaryColor = "#ffffff";
				textColor = "#000000";
				hoverColor = "#cfcfcf";
				selectColor = "#b8b8b8";
			}
			colorUpdate();
			std::string img = (primaryColor=="#e6e6e6")?"brightImage":"darkImage";
			call({ "bind",".themeButton","<Enter>",".themeButton config -image "+img+"Active" });
			call({ "bind",".themeButton","<Leave>",".themeButton config -image "+img });
			call({ ".themeButton","config","-image",img+"Active" });
			return 0;
		}, 0);

	writeLog("Creating tabs. ");
	control(".tabs", "frame", { "-width","40" });
	control(".tabs.tabTitle", "ttk::label", { "-textvariable","title","-borderwidth","30","-image","titleImage","-compound","top" });
	call({ "grid",".tabs.tabTitle" });
	MyTab(".tabs.game", "item.game", "swiGame"); call({ "grid",".tabs.game" });
	MyTab(".tabs.down", "item.down", "swiDown"); call({ "grid",".tabs.down" });
	MyTab(".tabs.acco", "item.acco", "swiAcco"); call({ "grid",".tabs.acco" });
	MyTab(".tabs.sett", "item.sett", "swiSett"); call({ "grid",".tabs.sett" });
	MyTab(".tabs.mods", "item.mods", "swiMods"); call({ "grid",".tabs.mods" });
	MyTab(".tabs.lang", "item.lang", "swiLang"); call({ "grid",".tabs.lang" });
	call({ "pack",".tabs","-side","left","-anchor","n" });

	control(".pageGame", "frame", { "-borderwidth","30" });
	control(".pageDown", "frame", { "-borderwidth","30" });
	control(".pageMDow", "frame", { "-borderwidth","30" });
	control(".pageMods", "frame", { "-borderwidth","30" });
	control(".pageAcco", "frame", { "-borderwidth","30" });
	control(".pageSett", "frame", { "-borderwidth","30" });
	control(".pageLang", "frame", { "-borderwidth","30" });
	control(".pageFabr", "frame", { "-borderwidth","30" });
	control(".pageOpti", "frame", { "-borderwidth","30" });
	control(".pageGame.title", "ttk::label", { "-textvariable","item.game" }); call({ "grid",".pageGame.title" });
	control(".pageDown.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageDown.title" });
	control(".pageMDow.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageMDow.title" });
	control(".pageAcco.title", "ttk::label", { "-textvariable","item.acco" }); call({ "grid",".pageAcco.title" });
	control(".pageSett.title", "ttk::label", { "-textvariable","item.sett" }); call({ "grid",".pageSett.title" });
	control(".pageMods.title", "ttk::label", { "-textvariable","item.mods" }); call({ "grid",".pageMods.title" });
	control(".pageLang.title", "ttk::label", { "-textvariable","item.lang" }); call({ "grid",".pageLang.title" });

	CreateCmd("msgbx", messageBox, 0);

	// Contents: Launch. 

	control(".pageGame.content", "frame");
	MyScrollBar* gameScro = new MyScrollBar(".pageGame.content.scro",".pageGame.content.list.yview");
	MyList* gameList = new MyList(".pageGame.content.list", ".pageGame.content.scro", false);
	call({ "pack",".pageGame.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageGame.content.list","-side","left" });
	call({ "grid",".pageGame.content" });
	CreateCmd("launch", launchGame, 0);
	
	// Contents: Account. 

	control(".pageAcco.content", "frame");
	MyScrollBar* accoScro = new MyScrollBar(".pageAcco.content.scro", ".pageAcco.content.list.yview");
	MyList* accoList  = new MyList(".pageAcco.content.list", ".pageAcco.content.scro");
	MyButtonIconActive(".pageAcco.content.add", "accounts.add", "addImage", "addAcc");
	call({ "pack",".pageAcco.content.add","-side","top","-fill","x" });
	call({ "pack",".pageAcco.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageAcco.content.list","-side","left" });
	call({ "grid",".pageAcco.content" });
	CreateCmd("addAcc", addAccount, accoList);
	CreateCmd("selectAccount", selectAccount, accoList);

	// Contents: Settings. 
	
	control(".pageSett.content", "frame");
	control(".pageSett.content.tabs", "frame");
	control(".pageSett.content.pageGame", "frame");
	control(".pageSett.content.pageGame.size", "frame");
	control(".pageSett.content.pageGame.size.text", "ttk::label", {"-text","655","-image","horizontalImage","-compound","left","-foreground",textColor });
	control(".pageSett.content.pageGame.java", "frame");
	control(".pageSett.content.pageGame.memo", "frame");
	control(".pageSett.content.pageGame.memv", "frame");
	control(".pageSett.content.pagePers", "frame");
	control(".pageSett.content.pageLaun", "frame");
	call({"grid",".pageSett.content"});
	call({"grid",".pageSett.content.tabs","-column","1","-row","1"});
	call({"grid",".pageSett.content.pageGame","-column","1","-row","2"});
	call({"grid",".pageSett.content.pageGame.size","-column","1","-row","1"});
	call({"grid",".pageSett.content.pageGame.size.text","-column","1","-row","1"});
	call({"grid",".pageSett.content.pageGame.java","-column","1","-row","2"});
	call({"grid",".pageSett.content.pageGame.memo","-column","1","-row","3"});
	call({"grid",".pageSett.content.pageGame.memv","-column","1","-row","4"});
	//call({"grid",".pageSett.content.pagePers","-column","1","-row","2"});
	//call({"grid",".pageSett.content.pageLaun","-column","1","-row","2"});

	// Contents: Language. 

	control(".pageLang.content", "frame");
	MyScrollBar* langScro = new MyScrollBar(".pageLang.content.scro", ".pageLang.content.list.yview");
	MyList* langList = new MyList(".pageLang.content.list", ".pageLang.content.scro");
	call({ "pack",".pageLang.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageLang.content.list","-side","left" });
	call({ "grid",".pageLang.content" });
	CreateCmd("selectLanguage", selectLanguage, langList);
	call({ ".pageLang.content.scro","config","-command",".pageLang.content.list.yview" });
	langList->add("lang.auto");
	langList->bind(0, "<Button-1>", "selectLanguage");
	for (const auto& i : allLanguages) {
		std::string name = i.second->getName();
		il(i.second->getName() == "empty") name = currentLanguage->localize("lang.empty");
		langList->add(name);
		langList->bind(langList->owned.size()-1, "<Button-1>", "selectLanguage "+i.first+" "+std::to_string(langList->owned.size()-1));
	}
	il(rdata("Language") == "auto") {
		langList->select(0);
	}
	else for (int i = 0; i < langManifest.size(); i++) {
		il(langManifest[i].first == currentLanguage->getID()) {
			langList->select(i+1);
		}
	}
	call({ langList->get(0).ctrlId+".text","config","-textvariable","lang.auto"});
	call({ langList->get(1).ctrlId+".text","config","-textvariable","lang.empty"});

	// Others. 
	CreateCmd("swiGame", pageGame, gameList);
	CreateCmd("swiDown", pageDown,        0);
	CreateCmd("swiAcco", pageAcco, accoList);
	CreateCmd("swiSett", pageSett,        0);
	CreateCmd("swiMods", pageMods,        0);
	CreateCmd("swiLang", pageLang, langList);
	MyButtonIconActive(".themeButton", "", "darkImage", "chTheme");
	call({ "place",".themeButton","-x","-89","-y","0","-relx","1" });

	if (isExists("assets\\background.png")) {
		call({ "image","create","photo","background","-file","assets\\background.png" });
		control(".bg", "ttk::label", { "-image","background" });
		call({ "place",".bg","-x","0","-y","0" });
		call({ "update" });
		std::string bgHei = "-"+std::to_string(std::atoi(call({ "winfo","height",".bg" }).c_str())/2);
		std::string bgWid = "-"+std::to_string(std::atoi(call({ "winfo","width",".bg" }).c_str())/2);
		call({ "place",".bg","-relx","0.5","-rely","0.5","-x",bgWid,"-y",bgHei });
		call({ "lower",".bg" });
	}

	pageCur = "";
	pageGame(gameList, interp, -1, nullptr);
	pageDown(       0, interp, -1, nullptr);
	pageAcco(accoList, interp, -1, nullptr);
	pageSett(       0, interp, -1, nullptr);
	pageMods(       0, interp, -1, nullptr);
	pageLang(langList, interp, -1, nullptr);
	colorUpdate();
	call({ "update" });
	std::string t = ".pageGame";
	pageGame(gameList, interp, -1, nullptr);
	call({ "update" });
	swiPage(t, 1);
	//
	//// Start the mainloop. 
	//while (1) {
	//	for (int i = 0; i < 100; i++) {
	//		for (int j = 0; j < 10;j++)Tcl_DoOneEvent(TCL_DONT_WAIT);
	//		Tcl_Sleep(2);
	//		il(call({ "winfo","exists","." }, 1) != "1") break;
	//	}
	//	continue;
	//	// Refresh version list. 
	//	{
	//		std::vector<std::string> versions = listVersion();
	//		versionList->clear();
	//		size_t n = 0;
	//		for (auto& i : versions) {
	//			std::string image;
	//			il(i[0]=='r')	image = "releaseImage";
	//			il(i[0]=='s')	image = "snapshotImage";
	//			std::string id = Strings::slice(i, 2);
	//			versionList->add(id, image, {"launchImage","launch " + id,"editImage",""});
	//			n++;
	//		}
	//		versionList->yview("", 0);
	//	}
	//}
	//
	CreateCmd("on_close", [](ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[])->int {
		writeLog("Closing application.");
		rdata("Geometry") = call({ "winfo","geometry","." });
		Tk_DestroyWindow(mainWin);
		return 0;
	}, 0);
	call({ "wm","protocol",".","WM_DELETE_WINDOW","on_close" });
	call({ "wm","protocol",".","WM_SAVE_YOURSELF","on_close" });

	// Threads
	std::thread thr([]()->int {
		writeLog("Trying to get the version manifest...");
		char* dat;
		size_t sz;
		int status = URLGet("https://launchermeta.mojang.com/mc/game/version_manifest.json", dat, sz);
		std::fstream ostr = std::fstream("test.json", std::ios::out);
		ostr.write(dat, sz);
		ostr.close();
		delete dat;
		if (status) writeLog("Failed to get the version manifest at step %d. ", status);
		else writeLog("Successfully got the version manifest! ");
		return 0;
	});
	thr.detach();
	auto evSetup = [](ClientData clientData, int flags) {

	};
	auto evCheck = [](ClientData clientData, int flags) {

	};
	Tcl_CreateEventSource(evSetup, evCheck, 0);
	Tcl_ThreadId gameUpdThread;	Tcl_CreateThread(&gameUpdThread, [](ClientData clientData)->unsigned {
		Tcl_Event* ev = (Tcl_Event*)Tcl_Alloc(sizeof(ThreadEvent));
		ev->proc = [](Tcl_Event* evPtr, int flags)->int {

			return 0;
		};
		Tcl_ThreadQueueEvent(mainThr, ev, TCL_QUEUE_HEAD);
		return 0;
	}, 0, TCL_THREAD_STACK_DEFAULT, TCL_THREAD_NOFLAGS);

	Tk_MainLoop();

	// Clean the things.

	writeLog("Started to clean things. ");
	Tcl_DeleteEventSource(evSetup, evCheck, 0);
	for (auto i : allLanguages) {
		delete i.second;
	}
	delete gameScro;
	delete gameList;
	delete accoScro;
	delete accoList;
	delete langScro;
	delete langList;
	CoUninitialize();
	curl_global_cleanup();
	Tcl_DeleteInterp(interp);
	Tcl_Finalize();
	writeLog("Deleted Tcl Interpreter. ");
	if (RemoveFontResourceExA("assets\\font.otf", FR_PRIVATE, 0) != 0) {
		writeLog("Font successfully removed. ");
	}
	else writeLog("Font removing failed. ");
	saveData();
	writeLog("Programme ended. ");
	fclose(logFile);
	fclose(tclScriptLog);
	_CrtDumpMemoryLeaks();
	return 0;
}