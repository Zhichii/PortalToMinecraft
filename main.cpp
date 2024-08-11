#define STATIC_BUILD
#define TCL_USE_STATIC_PACKAGES
#define CPP_TKINTER_SAFE_FUNCTIONS
#define TCL_THREADS
#define il if
#define el else if
#define ol else
#include <tcl.h>
#include <tk.h>
#include <direct.h>
#include <vector>
#include <string>
#include <filesystem>
#include <windows.h>
#include <json/json.h>
#include <json/json_value.cpp>
#include <json/json_writer.cpp>
#include <json/json_reader.cpp>
#include <libpng/png.h>
#include <random>
char logCacheStr[16384];
char formatCacheStr[16384];
wchar_t formatCacheStrW[16384];
FILE* logFile;
void writelog(std::string format, ...) {
	va_list args;
	va_start(args, format);
	_vsprintf_s_l(logCacheStr, 16384, format.c_str(), NULL, args);
	fprintf_s(logFile, "%s\n", logCacheStr);
	fflush(logFile);
	va_end(args);
}

FILE* tclScriptLog;
std::map<std::string,std::string> types;
std::string pageCur;
const std::string bgColor="#333333";
const std::string darkString;

Tcl_Interp* interp;
std::string call(const std::vector<std::string> strs, bool nolog = 1) {
	Tcl_Obj** objs = (Tcl_Obj**)Tcl_Alloc(strs.size() * sizeof(Tcl_Obj*));
	il(!nolog) {
		fprintf(tclScriptLog, "> ");
		for (int i = 0; i < strs.size(); i++) {
			fprintf(tclScriptLog, "%s ", strs[i].c_str());
		}
		fprintf(tclScriptLog, "\n");
	}
	for (int i = 0; i < strs.size(); i++) {
		objs[i] = Tcl_NewStringObj(strs[i].c_str(), strs[i].size());
		Tcl_IncrRefCount(objs[i]);
	}
	il(interp == nullptr) writelog("[interp] is null. ");
	Tcl_EvalObjv(interp, strs.size(), objs, 0);
	const char* result = Tcl_GetStringResult(interp);
	std::string r = result;
	for (int i = 0; i < strs.size(); i++) {
		Tcl_DecrRefCount(objs[i]);
	}
	Tcl_Free((char*)objs);
	il(!nolog) {
		il(r != "") fprintf(tclScriptLog, "%s\n", r.c_str());
		fflush(tclScriptLog);
	}
	return r;
}

#define PATHSEP "\\"
#define DPATHSEP "\\\\"
#define LPATHSEP L"\\"
#define O_PATHSEP "/"
#define O_DPATHSEP "//"
#define O_LPATHSEP L"/"
#define QUOT "\""
#include "help.h"
#include "strings.h"
#include "data.h"
#include "versioninfo.h"
#include "language.h"

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

static void MyButtonIconActive(std::string name, std::string textvariable = {}, std::string icon = {}, std::string commandId = {}, int width=0, std::string background="#202020") {
	control(name, "ttk::label");
	call({ name,"config","-image",icon,"-compound",((textvariable!=""?"left":"center")),"-textvariable",textvariable,"-width",std::to_string(width),"-background",background});
	call({ "bind",name,"<Button-1>",commandId });
	call({ "bind",name,"<Enter>",name + " config -image "+icon+"Active"+((textvariable!="")?(" -background #484848"):"") });
	call({ "bind",name,"<Leave>",name + " config -image "+icon+((textvariable!="")?(" -background "+bgColor):"") });
}

static void MyTab(std::string name, std::string textvariable = {}, std::string command = {}, std::string bg = "tabImage", std::string bgActive = "tabImageActive") {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image",bg,"-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command});
	call({ "bind",name,"<Enter>",name+" config -image "+bgActive });
	call({ "bind",name,"<Leave>",name+" config -image "+bg });
}

struct ScrollBarInfo {
	std::string name;
	std::string parent;
	std::string command;
	int height;
};

std::vector<ScrollBarInfo> scrollInfos;

int myplace(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	const char* parent = argv[1];
	const char* name = argv[2];
	int y = atoi(argv[3]);
	int n = atoi(call({ "winfo","rooty",parent }).c_str());
	int h = atoi(call({ "winfo","height",parent }).c_str());
	int sh = atoi(call({ "winfo","height",name }).c_str());
	int x = y-n;
	if (x < 3) x = 3;
	if (x > h-sh-3) x = h-sh-3;
	call({ "place",name,"-y",std::to_string(x) });
	call({ argv[4],"moveto",std::to_string(x*1.f/h )});
	return 0;
}

static void MyScrollBar(std::string name, std::string command = {}) {
	control(name, "frame");
	types[name]="my::nofill";
	control(name+".x", "frame");
	call({ name,"config","-background","#1e1e1e","-width","20" });
	call({ name+".x","config","-background",bgColor,"-width","14","-height","30"});
	call({ "bind", name+".x","<Enter>", name+".x config -background "+"#484848" });
	call({ "bind", name+".x","<Leave>", name+".x config -background "+bgColor });
	call({ "place", name+".x", "-x","3", "-y","3" });
	call({ "bind", name+".x","<B1-Motion>","myplace "+name+" "+name+".x %Y "+command });
}

static void MyTabY(std::string name, std::string textvariable = {}, std::string command = {}) {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image","tabImageY","-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command });
	call({ "bind",name,"<Enter>",name + " config -image tabImageYActive" });
	call({ "bind",name,"<Leave>",name + " config -image tabImageY" });
}

char toHex(int a) {
	il(a>=0xa) return a-0xa+'a';
	ol return a+'0';
}
int toNum(char a) {
	il(a>='a'&&a<='f') return a-'a'+0xa;
	el(a>='A'&&a<='F') return a-'A'+0xa;
	ol return a-'0';
}

template <class T>
struct LinkList {
	template <class U>
	struct Node {
		Node<U>* prev = nullptr;
		U value;
		Node<U>* next = nullptr;
		Node() {
			this->prev = nullptr;
			this->next = nullptr;
		}
	};
	Node<T>* head = nullptr;
	Node<T>* tail = nullptr;
	size_t sz = 0;
	size_t size() { return sz; }
	LinkList() {
		head = nullptr;
		this->sz = 0;
		tail = nullptr;
	}
	void addEnd(T val) {
		il(this->head == nullptr) {
			this->head = new Node<T>();
			this->head->prev = nullptr;
			this->head->value = val;
			this->head->next = nullptr;
			this->tail = this->head;
		}
		else {
			this->tail->next = new Node<T>();
			this->tail->next->prev = this->tail;
			this->tail->next->value = val;
			this->tail->next->next = nullptr;
			this->tail = this->tail->next;
		}
		this->sz++;
	}
	void delEnd() {
		il(this->tail == nullptr || this->head == nullptr) {
			this->head = nullptr;
			this->tail = nullptr;
			return;
		}
		il(this->head == this->tail) {
			this->~LinkList();
		}
		else {
			this->tail = this->tail->prev;
			delete this->tail->next;
			this->tail->next = nullptr;
		}
	}
	void reverse() {
		Node<T>* curI = this->head;
		this->head = this->tail;
		this->tail = curI;
		while (curI != nullptr) {
			Node<T>* t = curI->prev;
			curI->prev = curI->next;
			curI->next = t;
			curI = curI->prev;
		}
	}
	static void swap(Node<T>* a, Node<T>* b) {
		il(a->prev != nullptr)  a->prev->next = b;
		il(a->next != nullptr)  a->next->prev = b;
		il(b->prev != nullptr)  b->prev->next = a;
		il(b->next != nullptr)  b->next->prev = a;
		auto aP = a->prev, aN = a->next;
		a->prev = b->prev; a->next = b->next;
		b->prev = aP;      b->next = aN;
		il(this->head == a) this->head = b;
		el (this->head == b) this->head = a;
		il(this->tail == a) this->tail = b;
		el (this->tail == b) this->tail = a;
	}
	~LinkList() {
		Node<T>* cur = this->head;
		while (cur != nullptr) {
			Node<T>* t = cur->next;
			cur->prev = nullptr;
			cur->next = nullptr;
			delete cur;
			cur = t;
		}
		this->head = nullptr;
		this->sz = 0;
		this->tail = nullptr;
	}
	[[nodiscard]] T& operator [](size_t index) {
		Node<T>* cur = head;
		for (size_t i = 0; i < index; i++) {
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
	bool selEnabled;
	bool lock = false;
	int width;
	int height;
	std::string frameid;
	struct ItemRecord {
		std::string text;
		std::string imageId;
		std::string ctrlId;
		MyList* self;
		bool* enabled;
		int order;
		std::vector<std::string> commands;
		std::string color;
	};
	LinkList<ItemRecord> owned;
	std::string scroll;
	MyList(std::string frameId, std::string scrollbarId="", bool selection=true, int height = 40, int width = 40) {
		this->offset = 0;
		this->selection = 0;
		this->enabled = true;
		this->height = height;
		this->width = width;
		this->owned = {};
		this->frameid = frameId;
		this->scroll = scrollbarId;
		this->selEnabled = selection;
		types[frameId] = "frame";
		CreateCmd(frameId+".yview", [](ClientData clientData, Tcl_Interp* interp, int arg, const char** argv)->int {
			MyList* x = (MyList*)clientData;
			x->yview(argv[1], atof(argv[2]));
			return 0;
			}, this);
		call({ "frame",frameId,"-height",std::to_string(height),"-width",std::to_string(width),"-relief","raised" });
		CreateCmd(frameId+".mw", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* t = (MyList*)clientData;
				int delta = atoi(argv[1]);
				t->yview("scroll", ((delta<0)*2-1)*3);
				return 0;
		}, this);
		CreateCmd(frameId+".b1", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* list = (MyList*)clientData;
				int item = std::atoi(argv[1]);
				ItemRecord& ir = list->owned[item];
				il(list->enabled) {
					list->userSelect(ir.order);
				}
				return 0;
		}, this);
		CreateCmd(frameId+".enter", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* list = (MyList*)clientData;
				int item = std::atoi(argv[1]);
				ItemRecord& ir = list->owned[item];
				il(list->enabled) {
					call({ ir.ctrlId+".image","config","-background","#484848" });
					call({ ir.ctrlId+".text","config","-background","#484848" });
					for (int i = 0; i < ir.commands.size(); i += 2) {
						std::string t3 = ir.ctrlId+".button"+std::to_string(i/2);
						call({ t3,"config","-image",ir.commands[i],"-background","#484848" });
					}
				}
				return 0;
		}, this);
		CreateCmd(frameId+".leave", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* list = (MyList*)clientData;
				il(list->enabled) {
					list->colorUpdate();
				}
				return 0;
		}, this);
	}
	void colorUpdate() {
		il(this->offset < 0) this->offset = 0;
		for (int i = 0; i < this->owned.size(); i++) {
			std::string t1 = this->owned[i].ctrlId;
			std::string t2 = t1 + ".image";
			std::string t22 = t1 + ".text";
			il(this->selEnabled && i==this->selection) {
				call({ t2,"config","-background","#18572a" });
				call({ t22,"config","-background","#18572a","-foreground","white" });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string t3 = t1+".button"+std::to_string(j/2);
					call({ t3,"config","-image", "blankSelectedImage","-background","#18572a"});
				}
			}
			else {
				call({ t2,"config","-background",bgColor });
				call({ t22,"config","-background",bgColor,"-foreground","white" });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string t3 = t1+".button"+std::to_string(j/2);
					call({ t3,"config","-image", "blankImage","-background",bgColor });
				}
			}
		}
	}
	void update() {
		colorUpdate();
		for (int j = 0; j < this->owned.size(); j ++) {
			il(j >= this->offset && j < this->offset + this->fieldOfView) {
				call({ "grid",this->owned[j].ctrlId,"-row",std::to_string(j) });
			} else {
				call({ "grid","forget",this->owned[j].ctrlId });
			}
		}
	}
	void userSelect(int item) {
		this->selection = item;
		il(selEnabled) this->colorUpdate();
		call({ this->frameid+".enter", std::to_string(this->owned[item].order) });
	}
	void select(int item) {
		this->selection = item;
		il(selEnabled) this->colorUpdate();
	}
	void add(std::string text, std::string imageId = {}, std::vector<std::string> functions = {}) {
		this->selection = 0;
		std::string t1 = this->frameid+"."+std::to_string(this->owned.size());
		std::string t2 = t1+".image";
		std::string t22 = t1+".text";
		ItemRecord ir;
		ir.self = this;
		ir.enabled = &(this->enabled);
		ir.order = this->owned.size();
		ir.text = text;
		ir.imageId = imageId;
		ir.ctrlId = t1;
		ir.commands = functions;
		ir.color = bgColor;
		this->owned.addEnd(ir);
		types[t1] = "frame";
		types[t2] = (imageId=="<canvas>")?"ttk::label":"ttk::canvas";
		call({ "frame",t1,"-width",std::to_string(this->width),"-relief","raised" });
		il(imageId != "") {
			il(imageId != "<canvas>") {
				control(t2, "ttk::label", { "-image",imageId,"-compound","center","-width","4" });
			}
			ol{
				control(t2,"canvas",{"-height","44","-width","44","-highlightthickness","0","-background",bgColor});
				call({ t2,"create","rect","2","2","42","42","-fill","#000000","-outline","" });
			}
			call({ "grid",t2,"-column","1","-row","1" });
			call({ "bind",t2,"<MouseWheel>", this->frameid + ".mw %D" });
			call({ "bind",t2,"<Button-1>",	 this->frameid+".b1 "+std::to_string(ir.order) });
		}
		control(t22,"ttk::label",{"-image","horizontalImage","-compound","left","-text",text,"-width",std::to_string(this->width-(4*functions.size())-6*(imageId!=""))});
		call({ "bind",t22,"<MouseWheel>",this->frameid+".mw %D" });
		call({ "bind",t22,"<Button-1>",	 this->frameid+".b1 "+std::to_string(ir.order) });
		call({ "bind",t1,"<Enter>",		 this->frameid+".enter "+std::to_string(ir.order) });
		call({ "bind",t1,"<Leave>",		 this->frameid+".leave "+std::to_string(ir.order) });
		call({ "grid",t22,"-column","2","-row","1" });
		for (int i = 0; i < functions.size(); i += 2) {
			std::string t3 = t1 + ".button" + std::to_string(i/2);
			types[t3] = "ttk::label";
			MyButtonIconActive(t3, "", functions[i], functions[i+1], 0, bgColor);
			call({ t3,"config","-image","blankImage","-background",bgColor });
			call({ "bind",t3,"<MouseWheel>",this->frameid+".mw %D" });
			call({ "grid",t3,"-column",std::to_string(i/2+3),"-row","1" });
		}
		this->update();
	}
	void bind(int index, std::string seq, std::string cmdId) {
		il(index < 0 || index >= this->owned.size()) return;
		call({ "bind",this->frameid+"."+std::to_string(index)+".text",seq,cmdId});
		il(seq != "<Button-1>" &&
			seq != "<Enter>" &&
			seq != "<Leave>") return;
	}
	void yview(std::string cmd, double offs) {
		il(cmd == "scroll") {
			this->offset += (int)offs;
		}
		il(cmd == "moveto") {
			this->offset = this->owned.size() - this->fieldOfView;
			il(offs * (this->owned.size()) < (this->owned.size() - this->fieldOfView)) {
				this->offset = offs * (this->owned.size());
			}
		}
		il(this->offset > ((long long)this->owned.size() - this->fieldOfView)) {
			this->offset = this->owned.size() - this->fieldOfView;
		}
		il(this->offset < 0) this->offset = 0;
		this->update();
		int h = atoi(call({ "winfo","height",this->scroll }).c_str())-6;
		il(h < 0) h = 0;
		il(this->scroll == "") return;
		int pos = 0;
		int siz = h;
		il(this->owned.size() != 0) {
			pos = (this->offset * h) / this->owned.size();
			siz = (this->fieldOfView * h) / this->owned.size();
		}
		il(pos < 0) pos = 0;
		il(pos + siz > h) siz = h - pos;
		call({ "place",this->scroll+".x","-y",std::to_string(pos + 3) });
		call({ this->scroll+".x","config","-height",std::to_string(siz) });
	}
	int index() { return this->selection; }
	ItemRecord get(int ind = -1) {
		il(ind == -1) ind = this->index();
		return this->owned[ind];
	}
	void able(bool state=true) {
		this->enabled = state;
	}
	void clear() {
		LinkList<ItemRecord>::Node<ItemRecord>* cur = this->owned.head;
		size_t i = 1;
		while(cur!=nullptr) {
			std::string t1 = this->frameid + "." + std::to_string(i);
			std::string t2 = t1 + ".image";
			std::string t22 = t1 + ".text";
			std::string t3;
			call({ "destroy",t22 });
			call({ "destroy",t2 });
			for (int i = 0; i < cur->value.commands.size(); i += 2) {
				t3 = t1+".button"+std::to_string(i / 2);
				call({ "destroy",t3 });
				types[t3] = "";
			}
			call({ "destroy",t1 });
			types[t2] = "";
			types[t1] = "";
			Tcl_DeleteCommand(interp, (t1+".mw").c_str());
			Tcl_DeleteCommand(interp, (t2+".b1").c_str());
			Tcl_DeleteCommand(interp, (t2+".enter").c_str());
			Tcl_DeleteCommand(interp, (t2+".leave").c_str());
			i++;
			auto t = cur->next;
			delete cur;
			cur = t;
		}
		this->owned.head = nullptr;
		this->owned.sz = 0;
		this->owned.tail = nullptr;
	}
	~MyList() {
		owned.~LinkList();
		Tcl_DeleteCommand(interp, (this->frameid + ".mw").c_str());
		Tcl_DeleteCommand(interp, (this->frameid + ".n1").c_str());
		Tcl_DeleteCommand(interp, (this->frameid + ".enter").c_str());
		Tcl_DeleteCommand(interp, (this->frameid + ".leave").c_str());
		Tcl_DeleteCommand(interp, (this->frameid + ".yview").c_str());
	}
};


std::vector<std::string> messageBoxes;
size_t msgBxs=0;

std::string getRoot(std::string rt) {
	return rt == "" ? "." : rt;
}

int messageBox(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	msgBxs++;
	std::string name = ".window" + std::to_string(msgBxs);
	messageBoxes.push_back(name);
	call({ "toplevel",name });
	std::string title = argv[1];
	std::string content = argv[2];
	std::string level = argv[3];
	call({ "wm","title",name,currentLanguage->localize(title) });
	if (content != "<text>")
		control(name + ".text", "ttk::label", { "-text",currentLanguage->localize(content) });
	else
		control(name + ".text", "ttk::label", { "-text",argv[4] });
	control(name + ".ok", "ttk::button", { "-text",currentLanguage->localize("ok") });
	call({ name + ".ok","config","-command","destroy " + name });
	call({ "grid",name + ".text" });
	call({ "grid",name + ".ok" });
	return 0;
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
				writelog("Listing versions: File \"%s\" does not exist. ", Strings::ws2s(jsonPath).c_str());
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
				writelog("Initialized version \"%s\". ", info["id"].asCString());
			}
			catch (std::exception e) {
				writelog("Listing versions: Failed to initialize \"%s\": %s", Strings::ws2s(jsonPath).c_str(), e.what());
			}
		}
	}
	return versions;
}

int launchGame(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::thread thr([argv]()->int {
		std::wstring av = Strings::s2ws(argv[1]);
		writelog("Launching game \"%s\"...", argv[1]);
		VersionInfo* target = nullptr;
		std::string versionDir = rdata("GameDir").asString() + "versions";
		std::wstring jsonPath = Strings::s2ws(versionDir) + LPATHSEP + av + LPATHSEP + av + L".json";
		il(!isExists(jsonPath)) {
			writelog("Launching: File \"%s\" does not exist. ", Strings::ws2s(jsonPath).c_str());
			target = nullptr;
		}
		else {
			try {
				target = new VersionInfo(jsonPath);
			}
			catch (std::exception e) {
				writelog("Launching: Failed to initialize json \"%s\": %s", Strings::ws2s(jsonPath).c_str(), e.what());
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
		writelog("Launch: %s", cmdA.c_str());
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
	versionList->yview("", 0);
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageGame";
	call({ "pack",pageCur });
	return 0;
}

int pageDown(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageDown";
	call({ "pack",pageCur });
	return 0;
}

int pageMods(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageMods";
	call({ "pack",pageCur });
	return 0;
}

int addAccount(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	msgBxs++;
	std::string name = ".window" + std::to_string(msgBxs);
	messageBoxes.push_back(name);
	call({ "toplevel",name });
	std::string title = "dialog";
	std::string content = "accounts.add.prompt";
	call({ "wm","title",name,currentLanguage->localize(title) });
	control(name + ".text", "ttk::label", { "-text",currentLanguage->localize(content) });
	control(name + ".ok", "ttk::button", { "-text",currentLanguage->localize("ok") });
	call({ name + ".ok","config","-command","destroy " + name });
	call({ "grid",name + ".text" });
	call({ "grid",name + ".ok" });
	return 0;
}

int selectAccount(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	size_t selected = std::atoi(argv[1]);
	rdata("SelectedAccount") = selected;
	((MyList*)clientData)->userSelect(selected);
	flushData();
	return 0;
}

int pageAcco(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	Json::Value accounts = rdata("Accounts");
	MyList* accountList = (MyList*)clientData;
	accountList->clear();
	size_t n = 0;
	for (const auto& i : accounts) {
		il(i["userType"] == "please_support") {
			accountList->add(i["userName"].asString()+"\n      accounts.please_support", "steveImage", {"editImage",""});
		}
		el (i["userType"] == "mojang") {
			accountList->add(i["userName"].asString(), "<canvas>", {"editImage",""});
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
			writelog("Started to paint canvas. ");
			do {
				png_byte* stream = png_file;
				bool notpng = png_sig_cmp(png_file, 0, file_size);
				if (notpng) {
					delete[] png_file;
					break;
				}
				png_structp png_ptr = png_create_read_struct(
					PNG_LIBPNG_VER_STRING, 
					(png_voidp)NULL, NULL, NULL);
				if (!png_ptr) {
					delete[] png_file;
					break;
				}
				png_infop info_ptr = png_create_info_struct(png_ptr);
				if (!info_ptr) {
					delete[] png_file;
					png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
					break;
				}
				png_set_read_fn(png_ptr, &stream, [](png_structp png_ptr, png_bytep output, size_t sz) {
					png_byte** io_ptr = (png_byte**)png_get_io_ptr(png_ptr);
					if (io_ptr == NULL)
						return;
					memcpy(output, *io_ptr, sz);
					(*io_ptr) += sz;
				});
				png_read_info(png_ptr, info_ptr);
				png_uint_32 width = 0;
				png_uint_32 height = 0;
				int bit_depth = 0;
				int color_type = -1;
				png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
					&width,
					&height,
					&bit_depth,
					&color_type,
					NULL, NULL, NULL);
				if (retval!=1) {
					delete[] png_file;
					png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
					break;
				}
				if (color_type == PNG_COLOR_TYPE_PALETTE)
					png_set_palette_to_rgb(png_ptr);
				if (png_get_valid(png_ptr, info_ptr,
					PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
				png_read_update_info(png_ptr, info_ptr);
				const png_uint_32 bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
				png_byte* rowData = new png_byte[bytesPerRow];
				std::string clr = "#xxxxxx";
				for (int i = 0; i < height; i++) {
					png_read_row(png_ptr, rowData, NULL);
					if (i < 8) continue;
					if (i > 15)continue;
					for (int j = 0; j < 16; j++) {
						if (j < 8) continue;
						int k = i - 8;
						int l = j - 8;
						clr[1] = toHex(rowData[j*4+0] / 16 % 16);
						clr[2] = toHex(rowData[j*4+0] / 1 % 16);
						clr[3] = toHex(rowData[j*4+1] / 16 % 16);
						clr[4] = toHex(rowData[j*4+1] / 1 % 16);
						clr[5] = toHex(rowData[j*4+2] / 16 % 16);
						clr[6] = toHex(rowData[j*4+2] / 1 % 16);
						call({ accountList->owned[accountList->owned.size() - 1].ctrlId + ".image","create","rect",
							std::to_string(2 + k * 5),std::to_string(2 + l * 5),std::to_string(2 + k * 5 + 5),std::to_string(2 + l * 5 + 5),
							"-fill",clr,"-outline","" });
					}
				}
				png_read_end(png_ptr, NULL);
				png_destroy_read_struct(&png_ptr,& info_ptr, (png_infopp)NULL);
				delete[] png_file; 
				delete[] rowData;
				writelog("Successfully painted! ");
			} while (false); // Used "while" for "break; ".
			writelog("Stop painting. ");
		}
		accountList->bind(accountList->owned.size()-1, "<Button-1>", "selectAccount "+std::to_string(accountList->owned.size()-1));
	}
	accountList->select(rdata("SelectedAccount").asInt());
	accountList->yview("", 0);
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageAcco";
	call({ "pack",pageCur });
	return 0;
}

int pageSett(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	std::thread thr([](){
		//FolderDialog();
		});
	thr.detach();
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageSett";
	call({ "pack",pageCur });
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
	for (const auto& i : currentLanguage->lang)
		call({ "set",i.first,i.second });
	flushData();
	call({ "set","none","" });
	call({ "wm","title",getRoot(ROOT),currentLanguage->localize("title") });
	return 0;
}

int pageLang(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	((MyList*)clientData)->update();
	((MyList*)clientData)->yview("", 0);
	if (argc == -1) return 0;
	call({ "pack","forget",pageCur });
	pageCur = ".pageLang";
	call({ "pack",pageCur });
	return 0;
}

void fillColor(std::string rt_, std::string color=bgColor) {
	std::string rt = getRoot(rt_);
	call({ rt,"config","-background",color });
	call({ rt,"config","-foreground","white" });
	std::string a = call({ "winfo","children",rt });
	std::vector<std::string> children = Strings::split(a, " ");
	for (const auto& i : children) {
		il(types[i] == "my::nofill") continue;
		il(types[i] == "frame")
			fillColor(i);
		el (types[i] == "ttk::entry")
			continue;
		ol {
			call({ i,"config","-background",color });
			std::string x = call({ i,"config","-image" });
			x = Strings::slice(x, 22);
			il(x != "") {
				std::string y = call({ i,"config","-compound" });
				il(Strings::count(y, "center")) {
					continue;
				}
			}
			call({ i,"config","-foreground","#ffffff" });
		}
	}
}

int main() {

	mkdir("RvL\\");
	logFile = fopen("RvL\\log.txt", "w");
	tclScriptLog = fopen("RvL\\tcl.txt", "w");

	// Initialize Tcl/Tk. 
	writelog("Initializing Tcl/Tk. ");
	interp = Tcl_CreateInterp();
	Tcl_Init(interp);
	Tk_Init(interp);
	Tk_Window mainWin = Tk_MainWindow(interp);

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

	writelog("Initialize other things. ");
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

	writelog("Creating images. ");
	// Controls v1. 
	call({ "image","create","photo","buttonImage","-file","assets\\control\\button.png" });
	call({ "image","create","photo","buttonImageActive","-file","assets\\control\\buttonActive.png" });
	// Icons. 
	call({ "image","create","photo","releaseImage","-file","assets\\icon\\release.png" });
	call({ "image","create","photo","snapshotImage","-file","assets\\icon\\snapshot.png" });
	call({ "image","create","photo","oldImage","-file","assets\\icon\\old.png" });
	call({ "image","create","photo","fabricImage","-file","assets\\icon\\fabric.png" });
	call({ "image","create","photo","optifineImage","-file","assets\\icon\\optifine.png" });
	call({ "image","create","photo","titleImage","-file","assets\\icon.png" });
	// Controls. 
	call({ "image","create","photo","tabImage","-file","assets\\ctrl\\tab.png" });
	call({ "image","create","photo","tabImageActive","-file","assets\\ctrl\\tabActive.png" });
	call({ "image","create","photo","tabYImage","-file","assets\\ctrl\\tabY.png" });
	call({ "image","create","photo","tabYImageActive","-file","assets\\ctrl\\tabYActive.png" });
	call({ "image","create","photo","launchImage","-file","assets\\ctrl\\launch.png" });
	call({ "image","create","photo","launchImageActive","-file","assets\\ctrl\\launch_active.png" });
	call({ "image","create","photo","editImage","-file","assets\\ctrl\\edit.png" });
	call({ "image","create","photo","editImageActive","-file","assets\\ctrl\\edit_active.png" });
	call({ "image","create","photo","addImage","-file","assets\\ctrl\\add.png" });
	call({ "image","create","photo","addImageActive","-file","assets\\ctrl\\add_active.png" });
	// Misc. 
	call({ "image","create","photo","blankImage","-file","assets\\misc\\blank.png" });
	call({ "image","create","photo","blankSelectedImage","-file","assets\\misc\\blankSelected.png" });
	call({ "image","create","photo","horizontalImage","-file","assets\\misc\\horizontal.png" });
	call({ "image","create","photo","transparentImage","-file","assets\\misc\\blankTrans.png" });
	call({ "image","create","photo","steveImage","-file","assets\\misc\\steve.png" });

	call({ "wm","iconbitmap",getRoot(ROOT),"assets\\icon.ico" });
	call({ "wm","geometry",getRoot(ROOT),"600x400+25+25" });
	call({ "wm","title",getRoot(ROOT),currentLanguage->localize("title") });

	writelog("Creating tabs. ");
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
	MyTab(".tabs.game", "item.game", "swiGame"); call({ "grid",".tabs.game" });
	MyTab(".tabs.down", "item.down", "swiDown"); call({ "grid",".tabs.down" });
	MyTab(".tabs.acco", "item.acco", "swiAcco"); call({ "grid",".tabs.acco" });
	MyTab(".tabs.sett", "item.sett", "swiSett"); call({ "grid",".tabs.sett" });
	MyTab(".tabs.mods", "item.mods", "swiMods"); call({ "grid",".tabs.mods" });
	MyTab(".tabs.lang", "item.lang", "swiLang"); call({ "grid",".tabs.lang" });
	control(".pageGame.title", "ttk::label", { "-textvariable","item.game" }); call({ "grid",".pageGame.title" });
	control(".pageDown.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageDown.title" });
	control(".pageMDow.title", "ttk::label", { "-textvariable","item.down" }); call({ "grid",".pageMDow.title" });
	control(".pageAcco.title", "ttk::label", { "-textvariable","item.acco" }); call({ "grid",".pageAcco.title" });
	control(".pageSett.title", "ttk::label", { "-textvariable","item.sett" }); call({ "grid",".pageSett.title" });
	control(".pageMods.title", "ttk::label", { "-textvariable","item.mods" }); call({ "grid",".pageMods.title" });
	control(".pageLang.title", "ttk::label", { "-textvariable","item.lang" }); call({ "grid",".pageLang.title" });
	call({ "pack",".tabs","-side","left","-anchor","n" });
	
	CreateCmd("msgbx", messageBox, 0);
	CreateCmd("myplace", myplace, 0);

	// Contents: Launch. 

	control(".pageGame.content", "frame", {"-height", "114"});
	MyScrollBar(".pageGame.content.scro",".pageGame.content.list.yview");
	MyList* gameList = new MyList(".pageGame.content.list", ".pageGame.content.scro", false);
	call({ "pack",".pageGame.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageGame.content.list","-side","left" });
	call({ "grid",".pageGame.content" });
	CreateCmd("launch", launchGame, 0);
	call({ ".pageGame.content.scro","config","-command",".pageGame.content.list.yview" });
	
	// Contents: Account. 

	control(".pageAcco.content", "frame");
	MyScrollBar(".pageAcco.content.scro", ".pageAcco.content.list.yview");
	MyList* accoList  = new MyList(".pageAcco.content.list", ".pageAcco.content.scro");
	MyButtonIconActive(".pageAcco.content.add", "accounts.add", "addImage", "addAcc");
	call({ "pack",".pageAcco.content.add","-side","top","-fill","x" });
	call({ "pack",".pageAcco.content.scro","-side","right","-fill","y" });
	call({ "pack",".pageAcco.content.list","-side","left" });
	call({ "grid",".pageAcco.content" });
	CreateCmd("addAcc", addAccount, accoList);
	CreateCmd("selectAccount", selectAccount, accoList);
	call({ ".pageAcco.content.scro","config","-command",".pageAcco.content.list.yview" });

	// Contents: Settings. 
	
	control(".pageSett.content", "frame");
	control(".pageSett.content.tabs", "frame");
	control(".pageSett.content.pageGame", "frame");
	control(".pageSett.content.pageGame.size", "frame");
	control(".pageSett.content.pageGame.size.text", "ttk::label", {"-text","655","-image","horizontalImage","-compound","center","-foreground","white"});
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
	MyScrollBar(".pageLang.content.scro", ".pageLang.content.list.yview");
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
	control(".sep","ttk::separator",{ "-orient","vertical" });
	call({"pack",".sep","-side","left","-fill","y"});
	call({ "pack",pageCur });
	fillColor(getRoot(ROOT),bgColor);
	std::string t = pageCur;
	pageGame(gameList, interp, -1, nullptr);
	pageDown(       0, interp, -1, nullptr);
	pageAcco(accoList, interp, -1, nullptr);
	pageSett(       0, interp, -1, nullptr);
	pageMods(       0, interp, -1, nullptr);
	pageLang(langList, interp, -1, nullptr);
	call({ "update" });
	il(t==".pageGame") { pageGame(gameList, interp, 0, nullptr); }
	il(t==".pageDown") { pageDown(       0, interp, 0, nullptr); }
	il(t==".pageAcco") { pageAcco(accoList, interp, 0, nullptr); }
	il(t==".pageSett") { pageSett(       0, interp, 0, nullptr); }
	il(t==".pageMods") { pageMods(       0, interp, 0, nullptr); }
	il(t==".pageLang") { pageLang(langList, interp, 0, nullptr); }
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
	Tk_MainLoop();

	// Clean the things.
	
	for (auto i : allLanguages) {
		delete i.second;
	}
	delete langList;
	delete gameList;
	fclose(logFile);
	fclose(tclScriptLog);
	CoUninitialize();

	return 0;
}