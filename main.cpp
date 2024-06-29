#define STATIC_BUILD
#define CPP_TKINTER_SAFE_FUNCTIONS
#define TCL_THREADS
#include <tcl.h>
#include <tk.h>
#include <direct.h>
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>
#include <json/json.h>

FILE* logFile;
void writeLog(std::string logger, std::string content) {
	fprintf_s(logFile, "[%s,%s]\n", logger.c_str(), content.c_str());
	fflush(logFile);
}

#include "strings.h"
#include "help.h"
#include "language.h"
#include "data.h"
#include "versioninfo.h"
#include <mutex>
//#include "launch.h"

FILE* tclScriptLog;
bool darkMode = true;
std::map<std::string,std::string> types;
std::string pageCur;

std::vector<std::pair<Tcl_Obj**, int>> tclobjs;
Tcl_Interp* interp;
std::string call(const std::vector<std::string> strs, bool nolog=0) {
	Tcl_Obj** objs = (Tcl_Obj**)malloc(strs.size() * sizeof(Tcl_Obj*));
	if (objs == nullptr) return "";
	if(!nolog)fprintf(tclScriptLog, "> ");
	for (int i = 0; i < strs.size(); i++) {
		objs[i] = Tcl_NewStringObj(strs[i].c_str(), strs[i].size());
		Tcl_IncrRefCount(objs[i]);
		if(!nolog)fprintf(tclScriptLog, "%s ", strs[i].c_str());
	}
	if(!nolog)fprintf(tclScriptLog, "\n");
	Tcl_EvalObjv(interp, strs.size(), objs, 0);
	tclobjs.push_back({objs,strs.size()});
	const auto* result = Tcl_GetStringResult(interp);
	if (nolog) return result;
	if (std::string(result)!="")
		fprintf(tclScriptLog, "%s\n", result);
	fflush(tclScriptLog);
	return result;
}

// [](ClientData clientData, Tcl_Interp* interp, int arg, const char* argv[])->int
static void CreateCmd(std::string name, Tcl_CmdProc* proc, ClientData cd) {
	types[name] = "cmd";
	Tcl_CreateCommand(interp, name.c_str(), proc, cd, nullptr);
}

const std::string ROOT = ".";

static void control(std::string name, std::string type, std::vector<std::string> arg = {}) {
	std::vector<std::string> x = { type, name };
	for (const auto& i : arg) {
		x.push_back(i);
	}
	types[name] = type;
	call(x);
}

static void MyButton(std::string name, std::string textvariable = {}, std::string text = {}, std::string commandId = {}) {
	control(name, "ttk::label");
	if (textvariable != "")
		call({ name,"config","-textvariable",textvariable });
	else
		call({ name,"config","-text",text });
	call({ name,"config","-image","buttonImage","-compound","center","-foreground","white"});
	call({ "bind",name,"<Button-1>",commandId});
	call({ "bind",name,"<Enter>",name+" config -image buttonImageActive" });
	call({ "bind",name,"<Leave>",name+" config -image buttonImage" });
}

static void MyButtonIcon(std::string name, std::string textvariable = {}, std::string icon = {}, std::string commandId = {}, int width=0)  {
	control(name, "ttk::label");
	call({ name,"config","-image",icon,"-compound","left","-textvariable",textvariable,"-width",std::to_string(width) });
	call({ "bind",name,"<Button-1>",commandId });
	call({ "bind",name,"<Enter>",name + " config -background #19a842" });
	CreateCmd(name+".leave", [](ClientData clientData,
		Tcl_Interp* interp, int argc, const char* argv[])->int {
			if (darkMode) {
				call({ argv[1],"config","-background","#202020" });
			}
			else {
				call({ argv[1],"config","-background","#f0f0f0" });
			}
			return 0;
		}, 0);
	if (darkMode) {
		call({ name,"config","-background","#202020" });
	}
	else {
		call({ name,"config","-background","#f0f0f0" });
	}
	call({ "bind",name,"<Leave>",name+".leave "+name });
}

static void MyTab(std::string name, std::string textvariable = {}, std::string command = {}, std::string bg = "tabImage", std::string bgActive = "tabImageActive") {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image",bg,"-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command});
	call({ "bind",name,"<Enter>",name+" config -image "+bgActive });
	call({ "bind",name,"<Leave>",name+" config -image "+bg });
}

static void MyTabY(std::string name, std::string textvariable = {}, std::string command = {}) {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image","tabImageY","-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command });
	call({ "bind",name,"<Enter>",name + " config -image tabImageYActive" });
	call({ "bind",name,"<Leave>",name + " config -image tabImageY" });
}

template <class T>
struct LinkList {
	template <class U>
	struct Node {
		Node* prev = nullptr;
		U value;
		Node* next = nullptr;
	};
	size_t sz = 0;
	size_t size() { return sz; }
	Node<T>* head = nullptr;
	Node<T>* tail = nullptr;
	LinkList() {
		head = nullptr;
		sz = 0;
		tail = nullptr;
	}
	void push_back(T val) {
		if (this->head== nullptr) {
			this->head = new Node<T>();
			this->head->value = val;
			this->tail = this->head;
		}
		else {
			this->tail->next = new Node<T>();
			this->tail->next->prev = this->tail;
			this->tail->next->value = val;
			this->tail = this->tail->next;
		}
		sz++;
	}
	~LinkList() {
		Node<T>* cur = head;
		while (cur != nullptr) {
			Node<T>* t = cur->next;
			free(cur);
			cur = t;
		}
	}
	[[nodicard]] T& operator [](size_t index) {
		Node<T>* cur = head;
		for (size_t i = 0; i < index; i ++) {
			cur = cur->next;
		}
		return cur->value;
	}
};

struct MyList {
	const static int fieldOfView=6;
	int offset;
	int selection;
	bool enabled;
	bool lock = false;
	std::vector<int> size;
	std::string frameid;
	struct ItemRecord {
		std::string text;
		std::string imageId;
		std::string ctrlId;
		// BindingRecord
		MyList* self;
		bool* enabled;
		int order;
		std::string bindB1Id;
		std::string bindEnterId;
		std::string bindLeaveId;
		std::vector<std::string> commands;
	};
	LinkList<ItemRecord> owned;
	std::string scroll;
	MyList(std::string frameId, int height = 100, int width = 50, std::string scrollbarId="") {
		this->offset = 0;
		this->selection = -1;
		this->enabled = true;
		this->size = { height, width };
		this->owned = {};
		this->frameid = frameId;
		this->scroll = scrollbarId;
		types[frameId] = "frame";
		CreateCmd(frameId+".yview", [](ClientData clientData, Tcl_Interp* interp, int arg, const char** argv)->int {
			MyList* x = (MyList*)clientData;
			x->yview(argv[1], atof(argv[2]));
			return 0;
			}, this);
		call({ "frame",frameId,"-height",std::to_string(height),"-width",std::to_string(width),"-borderwidth","1","-relief","raised" });
	}
	void colorUpdate() {
		if (this->offset < 0) this->offset = 0;
		for (int i = 0; i < this->owned.size(); i++) {
			if (i == this->selection) {
				call({ this->owned[i].ctrlId+".text","config","-background","#18572a","-foreground","white" });
			}
			else {
				if (darkMode) call({ this->owned[i].ctrlId+".text","config","-background","#202020","-foreground","white" });
				else call({ this->owned[i].ctrlId+".text","config","-background","#f0f0f0","-foreground","black" });
			}
		}
	}
	void update() {
		colorUpdate();
		for (int j = 0; j < this->owned.size(); j ++) {
			if (j >= this->offset && j < this->offset + this->fieldOfView) {
				call({ "grid",this->owned[j].ctrlId,"-row",std::to_string(j) });
			} else {
				call({ "grid","forget",this->owned[j].ctrlId });
			}
		}
	}
	void userSelect(int item) {
		this->selection = item;
		this->colorUpdate();
	}
	void select(int item) {
		this->selection = item;
		this->colorUpdate();
	}
	void add(std::string text, std::string imageId = {}, std::vector<std::string> functions = {}) {
		this->selection = 0;
		std::string t1 = this->frameid+"."+std::to_string(this->owned.size());
		std::string t2 = t1+".text";
		ItemRecord ir;
		ir.self = this;
		ir.enabled = &(this->enabled);
		ir.order = this->owned.size();
		ir.text = text;
		ir.imageId = imageId;
		ir.ctrlId = t1;
		ir.commands = functions;
		this->owned.push_back(ir);
		types[t1] = "frame";
		types[t2] = "ttk::label";
		call({ "frame",t1,"-width",std::to_string(this->size[1]),"-borderwidth","1","-relief","raised" });
		call({ "ttk::label",t2,"-text",text,"-image",imageId,"-compound","left","-width",std::to_string(this->size[1]-((int)(2.5f * functions.size())))});
		CreateCmd(t1 + ".mw", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* t = (MyList*)clientData;
				int delta = atoi(argv[1]);
				t->yview("scroll", ((delta<0)*2-1)*5);
				return 0;
			}, this);
		CreateCmd(t2 + ".b1", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				ItemRecord* ir = (ItemRecord*)clientData;
				if (*(ir->enabled)) {
					ir->self->userSelect(ir->order);
				}
				return 0;
			}, &this->owned[this->owned.size() - 1]);
		CreateCmd(t2 + ".enter", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				ItemRecord* ir = (ItemRecord*)clientData;
				if (*(ir->enabled)) {
					call({ ir->ctrlId+".text","config","-background","#19a842" });
					//call({ ir->ctrlId,"config","-background","#19a842" });
				}
				return 0;
			}, &this->owned[this->owned.size() - 1]);
		CreateCmd(t2 + ".leave", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				ItemRecord*ir = (ItemRecord*)clientData;
				ir->self->colorUpdate();
				return 0;
			}, &this->owned[this->owned.size() - 1]);
		call({ "bind",t2,"<MouseWheel>",t1+".mw %D" });
		call({ "bind",t2,"<Button-1>",t2+".b1" });
		call({ "bind",t2,"<Enter>",t2+".enter" });
		call({ "bind",t2,"<Leave>",t2+".leave" });
		call({ "grid",t2,"-column","1","-row","1" });
		for (int i = 0; i < functions.size(); i += 2) {
			std::string t3 = t1 + ".button" + std::to_string(i / 2);
			types[t3] = "ttk::label";
			MyButtonIcon(t3, "", functions[i], functions[i + 1]);
			call({ "bind",t3,"<MouseWheel>",t1+".mw %D" });
			call({ "grid",t3,"-column",std::to_string(i / 2 + 2),"-row","1" });
		}
		this->update();
	}
	void bind(int index, std::string seq, std::string cmdId) {
		if (index < 0 || index >= this->owned.size()) return;
		call({ "bind",this->frameid+"."+std::to_string(index)+".text",seq,cmdId});
		if (seq != "<Button-1>" &&
			seq != "<Enter>" &&
			seq != "<Leave>") return;
	}
	void yview(std::string cmd, double offs) {
		if (cmd == "scroll") {
			this->offset += (int)offs;
		}
		if (cmd == "moveto") {
			this->offset = this->owned.size() - this->fieldOfView;
			if (offs * (this->owned.size()) < (this->owned.size() - this->fieldOfView)) {
				this->offset = offs * (this->owned.size());
			}
		}
		if (this->offset > ((long long)this->owned.size() - this->fieldOfView)) {
			this->offset = this->owned.size() - this->fieldOfView;
		}
		if (this->offset < 0) this->offset = 0;
		double beg = 0.0;
		double end = 1.0;
		if (this->owned.size() != 0) beg = ((double)this->offset) / this->owned.size();
		if (this->owned.size() != 0) end = ((double)this->offset + this->fieldOfView) / this->owned.size();
		if (beg < 0.0) beg = 0.0;
		if (end > 1.0) end = 1.0;
		this->update();
		if (this->scroll == "") return;
		if (this->owned.size() != 0) call({ this->scroll,"set",std::to_string(beg),std::to_string(end) });
		else call({ this->scroll,"set","0.0","1.0" });
	}
	int index() { return this->selection; }
	ItemRecord get(int ind = -1) {
		if (ind == -1) ind = this->index();
		return this->owned[ind];
	}
	void able(bool state=true) {
		this->enabled = state;
	}
	void clear() {
		LinkList<ItemRecord>::Node<ItemRecord>* cur = this->owned.head;
		while(cur!=nullptr) {
			call({ "destroy",cur->value.ctrlId+".text"});
			types[cur->value.ctrlId] = "";
			std::vector<std::string> functions = cur->value.commands;
			for (int i = 0; i < functions.size(); i += 2) {
				std::string t3 = cur->value.ctrlId+".button"+std::to_string(i / 2);
				types[t3] = "";
				call({ "destroy",t3 });
			}
			call({ "destroy",cur->value.ctrlId });
			types[cur->value.ctrlId] = "";
			cur = cur->next;
		}
		this->owned.~LinkList();
		this->owned = {};
	}
	~MyList() {
		Tcl_DeleteCommand(interp, (this->frameid + ".yview").c_str());
	}
};

// Launch
MyList* versionList;

// Language
MyList* languageList;



std::string getRoot(std::string rt) {
	return rt == "" ? "." : rt;
}

void chTheme(std::string rt_) {
	std::string rt = getRoot(rt_);
	if (darkMode) {
		call({ rt,"config","-background","#202020" });
		call({ rt,"config","-foreground","white" });
	}
	else {
		call({ rt,"config","-background","#f0f0f0" });
		call({ rt,"config","-foreground","black" });
	}
	std::string a = call({ "winfo","children",rt });
	std::vector<std::string> children = Strings::split(a, " ");
	for (const auto& i : children) {
		if (types[i] == "frame")
			chTheme(i);
		else if (types[i] == "ttk::entry")
			continue;
		else {
			if (darkMode) 
				call({ i,"config","-background","#202020" });
			else
				call({ i,"config","-background","#f0f0f0" });
			std::string x = call({ i,"config","-image" });
			x = Strings::slice(x, 22);
			if (x != "") {
				std::string y = call({ i,"config","-compound" });
				if (Strings::count(y, "center")) {
					continue;
				}
			}
			if (darkMode)
				call({ i,"config","-foreground","white" });
			else 
				call({ i,"config","-foreground","black" });
		}
	}
	if (rt == ".") {
		versionList->colorUpdate();
	}
}



std::vector<VersionInfo*> listVersion() {
	std::vector<VersionInfo*> versions;
	std::string versionDir = rdata("GameDir").asString() + "versions";
	if (isDir(versionDir)) {
		for (auto& v : std::filesystem::directory_iterator::directory_iterator(versionDir)) {
			std::string fileName = v.path().filename().string();
			std::string jsonPath = versionDir + PATH_SEP + fileName + PATH_SEP + fileName + ".json";
			if (!isExists(jsonPath)) continue;
			VersionInfo*x = new VersionInfo(jsonPath);
			versions.push_back(x);
		}
	}
	return versions;
}

int launchGame(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::thread thr([argv]()->int {
		std::vector<VersionInfo*> versions = listVersion();
		std::string cmd = "echo x";
		//launchInstance(cmd, rdata("GameDir").asString(), versions[atoi(argv[1])], rdata("SelectedAccount").asInt(), {});
		writeLog("launch", cmd);
		for (auto i : versions) {
			delete i;
		}
		std::string state = "DEFAULT 0010";
		execNotThrGetOutInvoke(cmd, &state, rdata("GameDir").asString(), [](const std::string& o, void* b)->int {
			std::string* c = (std::string*)b;
			printf("%s\n", (*c).c_str());
			if (o.starts_with("---- Minecraft Crash Report ----"))
				(*c)[8] = '1';
			if (o.starts_with("#@!@# Game crashed! Crash report saved to: #@!@#"))
				(*c)[9] = '1';
			if (Strings::slice(o, 11) == "[Client thread/INFO]: Stopping!")
				(*c)[10] = '0';
			if (Strings::slice(o, 13).starts_with("[main] ERROR FabricLoader/"))
				(*c) = "FABRIC. " + Strings::between(o, "[main] ERROR FabricLoader/", "\r");
			if (o == "")
				(*c)[11] = '1';
			if ((*c).starts_with("FABRIC. ")) {
				//message(localize("error").c_str(), strFormat(localize("tell.minecraft.fabric_crash").c_str(), slice((*c), 8)).c_str());
				(*c) = "DEFAULT 0000";
			}
			if ((*c) == "DEFAULT 1111") {
				//message(localize("error").c_str(), localize("tell.minecraft.crash").c_str());
				(*c) = "DEFAULT 0000";
			}
			return 0;
			});
		});
	thr.detach();
	return 0;
}

int pageGame(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::vector<VersionInfo*> versions = listVersion();
	versionList->clear();
	size_t n = 0;
	for (auto& i : versions) {
		std::string image;
		if (i->getType() == "release")	image = "releaseImage";
		if (i->getType() == "snapshot")	image = "snapshotImage";
		versionList->add(i->getId(), image, {"launchImage","launch "+std::to_string(n),"editImage",""});
		n++;
	}
	versionList->yview("", 0);
	for (auto i : versions) {
		delete i;
	}
	call({ "pack","forget",pageCur });
	pageCur = ".pageGame";
	call({ "pack",pageCur });
	return 0;
}

int pageDown(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	call({ "pack","forget",pageCur });
	pageCur = ".pageDown";
	call({ "pack",pageCur });
	return 0;
}

int pageMods(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	call({ "pack","forget",pageCur });
	pageCur = ".pageMods";
	call({ "pack",pageCur });
	return 0;
}

int pageAcco(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	call({ "pack","forget",pageCur });
	pageCur = ".pageAcco";
	call({ "pack",pageCur });
	return 0;
}

int pageSett(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	call({ "pack","forget",pageCur });
	pageCur = ".pageSett";
	call({ "pack",pageCur });
	return 0;
}

int selectLanguage(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	if (argc == 1) {
		rdata("Language") = "auto";
		languageList->userSelect(0);
		currentLanguage = allLanguages[getDefaultLanguage()];
	}
	else {
		rdata("Language") = argv[1];
		languageList->userSelect(atoi(argv[2]));
		currentLanguage = allLanguages[argv[1]];
	}
	for (const auto& i : currentLanguage->lang)
		call({ "set",i.first,i.second });
	flushData();
	call({ "set","none","" });
	call({ "wm","title",getRoot(ROOT),currentLanguage->localize("title") });
	return 0;
}

int pageLang(ClientData clientData, Tcl_Interp* interp, int arg, const char* argv[]) {
	languageList->update();
	call({ "pack","forget",pageCur });
	pageCur = ".pageLang";
	call({ "pack",pageCur });
	return 0;
}

int main() {

	// Initialize Tcl/Tk. 
	interp = Tcl_CreateInterp();
	Tcl_Init(interp);
	Tk_Init(interp);

	mkdir("RvL\\");
	logFile = fopen("RvL\\log.txt", "w");
	tclScriptLog = fopen("RvL\\tcl.txt", "w");

	initLanguages();
	initData();
	// Set the default language. 
	{
		std::string t = rdata("Language").asString();
		if (!allLanguages.contains(t)) t = getDefaultLanguage();
		currentLanguage = allLanguages[t];
	}

	for (const auto& i : currentLanguage->lang)
		call({ "set",i.first,i.second });
	call({ "set","none","" });
	// Initialize for ATL. 
	CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED);

	call({ "image","create","photo","buttonImage","-file","lib\\res\\control\\button.png" });
	call({ "image","create","photo","buttonImageActive","-file","lib\\res\\control\\buttonActive.png" });
	call({ "image","create","photo","tabImage","-file","lib\\res\\control\\tab.png" });
	call({ "image","create","photo","tabImageActive","-file","lib\\res\\control\\tabActive.png" });
	call({ "image","create","photo","tabYImage","-file","lib\\res\\control\\tabY.png" });
	call({ "image","create","photo","tabYImageActive","-file","lib\\res\\control\\tabYActive.png" });
	call({ "image","create","photo","releaseImage","-file","lib\\res\\icon\\release.png" });
	call({ "image","create","photo","snapshotImage","-file","lib\\res\\icon\\snapshot.png" });
	call({ "image","create","photo","oldImage","-file","lib\\res\\icon\\old.png" });
	call({ "image","create","photo","fabricImage","-file","lib\\res\\icon\\fabric.png" });
	call({ "image","create","photo","optifineImage","-file","lib\\res\\icon\\optifine.png" });
	call({ "image","create","photo","launchImage","-file","lib\\res\\icon\\launch.png" });
	call({ "image","create","photo","editImage","-file","lib\\res\\icon\\edit.png" });
	call({ "image","create","photo","onImage","-file","lib\\res\\icon\\on.png" });
	call({ "image","create","photo","offImage","-file","lib\\res\\icon\\off.png" });
	call({ "image","create","photo","titleImage","-file","lib\\res\\icon.png" });

	call({ "wm","iconbitmap",getRoot(ROOT),"lib\\res\\icon.ico" });
	call({ "wm","geometry",getRoot(ROOT),"600x400+25+25" });
	call({ "wm","title",getRoot(ROOT),currentLanguage->localize("title") });

	control(".tabs", "frame", { "-width","40" });
	control(".pageGame", "frame", { "-borderwidth","30" });
	control(".pageDown", "frame", { "-borderwidth","30" });
	control(".pageMDow", "frame", { "-borderwidth","30" });
	control(".pageMods", "frame", { "-borderwidth","30" });
	control(".pageAcco", "frame", { "-borderwidth","30" });
	control(".pageSett", "frame", { "-borderwidth","30" });
	control(".pageLang", "frame", { "-borderwidth","30" });
	control(".pageFabr", "frame", { "-borderwidth","30" });
	control(".pageOpti", "frame", { "-borderwidth","30" });
	pageCur = ".pageGame";

	control(".tabs.tabTitle", "ttk::label", { "-textvariable","title","-borderwidth","30","-image","titleImage","-compound","top" });
	call({ "grid",".tabs.tabTitle" });
	CreateCmd("swiGame", pageGame, 0);
	CreateCmd("swiDown", pageDown, 0);
	CreateCmd("swiAcco", pageAcco, 0);
	CreateCmd("swiSett", pageSett, 0);
	CreateCmd("swiMods", pageMods, 0);
	CreateCmd("swiLang", pageLang, 0);
	call({ "image","create","photo","gameBBG","-file","lib\\res\\control\\gameBBG.png" });
	call({ "image","create","photo","downBBG","-file","lib\\res\\control\\downBBG.png" });
	call({ "image","create","photo","accoBBG","-file","lib\\res\\control\\accoBBG.png" });
	call({ "image","create","photo","settBBG","-file","lib\\res\\control\\settBBG.png" });
	call({ "image","create","photo","modsBBG","-file","lib\\res\\control\\modsBBG.png" });
	call({ "image","create","photo","langBBG","-file","lib\\res\\control\\langBBG.png" });
	MyTab(".tabs.game", "item.game", "swiGame", "gameBBG"); call({ "grid",".tabs.game" });
	MyTab(".tabs.down", "item.down", "swiDown", "downBBG"); call({ "grid",".tabs.down" });
	MyTab(".tabs.acco", "item.acco", "swiAcco", "accoBBG"); call({ "grid",".tabs.acco" });
	MyTab(".tabs.sett", "item.sett", "swiSett", "settBBG"); call({ "grid",".tabs.sett" });
	MyTab(".tabs.mods", "item.mods", "swiMods", "modsBBG"); call({ "grid",".tabs.mods" });
	MyTab(".tabs.lang", "item.lang", "swiLang", "langBBG"); call({ "grid",".tabs.lang" });
	control(".pageGame.title", "ttk::label", { "-textvariable","item.game" }); call({ "grid",".pageGame.title" });
	control(".pageDown.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageDown.title" });
	control(".pageMDow.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageMDow.title" });
	control(".pageAcco.title", "ttk::label", { "-textvariable","item.acco" }); call({ "grid",".pageAcco.title" });
	control(".pageSett.title", "ttk::label", { "-textvariable","item.sett" }); call({ "grid",".pageSett.title" });
	control(".pageMods.title", "ttk::label", { "-textvariable","item.mods" }); call({ "grid",".pageMods.title" });
	control(".pageLang.title", "ttk::label", { "-textvariable","item.lang" }); call({ "grid",".pageLang.title" });
	call({ "pack",".tabs","-side","left","-anchor","n" });

	// Contents: Launch. 

	control(".pageGame.content", "frame", {"-height", "114"});
	control(".pageGame.content.scro", "ttk::scrollbar");
	control(".pageGame.content.pass", "ttk::label", { "-height","20" });
	versionList = new MyList(".pageGame.content.list", 20, 40, ".pageGame.content.scro");
	call({ "pack",".pageGame.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageGame.content.list","-side","left" });
	call({ "pack",".pageGame.content.pass","-side","left" });
	call({ "grid",".pageGame.content" });
	CreateCmd("launch", launchGame, 0);
	call({ ".pageGame.content.scro","config","-command",".pageGame.content.list.yview" });

	// Content: Language. 

	control(".pageLang.content", "frame");
	control(".pageLang.content.scro", "ttk::scrollbar");
	languageList = new MyList(".pageLang.content.list", 20, 40, ".pageLang.content.scro");
	call({ "pack",".pageLang.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageLang.content.list","-side","left" });
	call({ "grid",".pageLang.content" });
	CreateCmd("selectLanguage", selectLanguage, 0);
	CreateCmd(".pageLang.content.list.yview", [](ClientData clientData, Tcl_Interp* interp, int arg, const char** argv)->int {
		languageList->yview(argv[1], atof(argv[2]));
		return 0;
		}, 0);
	call({ ".pageLang.content.scro","config","-command",".pageLang.content.list.yview" });
	languageList->add("lang.auto");
	languageList->bind(0, "<Button-1>", "selectLanguage");
	for (const auto& i : allLanguages) {
		std::string name = i.second->getName();
		if (i.second->getName() == "empty") name = currentLanguage->localize("lang.empty");
		languageList->add(name);
		languageList->bind(languageList->owned.size()-1, "<Button-1>", "selectLanguage "+i.first+" "+std::to_string(languageList->owned.size()-1));
	}
	if (rdata("Language") == "auto") {
		languageList->select(0);
	}
	else for (int i = 0; i < langManifest.size(); i++) {
		if (langManifest[i].first == currentLanguage->getID()) {
			languageList->select(i+1);
		}
	}
	languageList->yview("", 0);
	call({ languageList->get(0).ctrlId+".text","config","-textvariable","lang.auto"});
	call({ languageList->get(1).ctrlId+".text","config","-textvariable","lang.empty"});

	// Others. 

	control(".sep","ttk::separator",{ "-orient","vertical" });
	call({"pack",".sep","-side","left","-fill","y"});
	call({ "pack",pageCur });
	if (pageCur==".pageGame") pageGame(0,interp,0,nullptr);
	if (pageCur==".pageDown") pageDown(0,interp,0,nullptr);
	if (pageCur==".pageAcco") pageAcco(0,interp,0,nullptr);
	if (pageCur==".pageSett") pageSett(0,interp,0,nullptr);
	if (pageCur==".pageMods") pageMods(0,interp,0,nullptr);
	if (pageCur==".pageLang") pageLang(0,interp,0,nullptr);
	CreateCmd("chTheme", [](ClientData clientData,
		Tcl_Interp* interp, int argc, const char* argv[])->int {
			darkMode = !darkMode;
			call({ ".themeButton","config","-image",darkMode ? "onImage" : "offImage" });
			chTheme(ROOT);
			return 0;
		}, 0);
	MyButtonIcon(".themeButton", "", darkMode?"onImage":"offImage", "chTheme");
	call({ "place",".themeButton","-x","-44","-y","0","-relx","1" });
	chTheme(ROOT);

	// Start the mainloop. 
	Tk_MainLoop();

	// Clean the things. 
	for (auto i : tclobjs) {
		for (int j = 0; j < i.second; j++) {
			TclFreeObj(i.first[j]);
		}
		free(i.first);
	}
	for (auto i : allLanguages) {
		delete i.second;
	}
	delete versionList;
	delete languageList;
	fclose(logFile);
	fclose(tclScriptLog);

	return 0;
}