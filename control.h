
#ifndef RIVERLAUNCHER3_CONTROL_H
#define RIVERLAUNCHER3_CONTROL_H
#include <tcl.h>
#include <tk.h>
#include <string>
#include <vector>
#include <map>

FILE* tclScriptLog;
std::string pageCur;
std::string primaryColor;
std::string textColor;
std::string secondaryColor;
std::string hoverColor;
std::string selectColor;
typedef int ClrUpdFunc(std::vector<std::string>);
std::map<std::string, ClrUpdFunc*> colorUpdates;
std::map<std::string, std::vector<std::string>> clrUpdCd;
std::vector<std::string> messageBoxes;
size_t msgBxs = 0;
Tcl_Interp* interp;
std::string call(const std::vector<std::string> strs, bool nolog = 1) {
	Tcl_Obj** objs = (Tcl_Obj**)(new char[strs.size() * sizeof(Tcl_Obj*)]);
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
	il(interp == nullptr) writeLog("[interp] is null. ");
	Tcl_EvalObjv(interp, strs.size(), objs, 0);
	const char* result = Tcl_GetStringResult(interp);
	std::string r = result;
	for (int i = 0; i < strs.size(); i++) {
		Tcl_DecrRefCount(objs[i]);
	}
	delete[] objs;
	il(!nolog) {
		il(r != "") fprintf(tclScriptLog, "%s\n", r.c_str());
		fflush(tclScriptLog);
	}
	return r;
}

// [](ClientData clientData, Tcl_Interp* interp, int arg, const char* argv[])->int
static void CreateCmd(std::string name, Tcl_CmdProc* proc, ClientData cd) {
	Tcl_CreateCommand(interp, name.c_str(), proc, cd, nullptr);
}

const std::string ROOT = ".";

static void control(std::string name, std::string type, std::vector<std::string> arg = {}, bool nolog = 1) {
	std::vector<std::string> x = { type, name };
	for (const auto& i : arg) {
		x.push_back(i);
	}
	call(x, nolog);
	call({ name,"config","-font","font1" }, nolog);
}

static void MyButtonIconActive(std::string name, std::string textvariable = {}, std::string icon = {}, std::string commandId = {}, int width = 0, std::string background = primaryColor) {
	control(name, "ttk::label");
	call({ name,"config","-image",icon,"-compound",((textvariable != "" ? "left" : "center")),"-textvariable",textvariable,"-width",std::to_string(width),"-background",background });
	call({ "bind",name,"<Button-1>",commandId });
	call({ "bind",name,"<Enter>",name + " config -image " + icon + "Active" + ((textvariable != "") ? (" -background " + hoverColor) : "") });
	call({ "bind",name,"<Leave>",name + " config -image " + icon + ((textvariable != "") ? (" -background " + primaryColor) : "") });
	colorUpdates[name] = [](std::vector<std::string> cd)->int {
		std::string name = cd[0];
		std::string icon = cd[1];
		std::string tvar = cd[2];
		call({ "bind",name,"<Enter>",name + " config -image " + icon + "Active" + ((tvar != "") ? (" -background " + hoverColor) : "") });
		call({ "bind",name,"<Leave>",name + " config -image " + icon + ((tvar != "") ? (" -background " + primaryColor) : "") });
		return 0;
		};
	clrUpdCd[name] = { name, icon, textvariable };
}

static void MyTab(std::string name, std::string textvariable = {}, std::string command = {}, std::string bg = "tabImage", std::string bgActive = "tabImageActive") {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image",bg,"-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command });
	call({ "bind",name,"<Enter>",name + " config -image " + bgActive });
	call({ "bind",name,"<Leave>",name + " config -image " + bg });
}

static void MyTabY(std::string name, std::string textvariable = {}, std::string command = {}) {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image","tabImageY","-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command });
	call({ "bind",name,"<Enter>",name + " config -image tabImageYActive" });
	call({ "bind",name,"<Leave>",name + " config -image tabImageY" });
}

struct MyScrollBar {
	int dragPosition;
	std::string frameName;
	std::string buttonName;
	std::string command;
	MyScrollBar(std::string name, std::string command = {}) {
		control(name, "frame");
		control(name + ".x", "frame");
		call({ name,"config","-background",secondaryColor,"-width","30" });
		call({ name + ".x","config","-background",primaryColor,"-width","20","-height","30" });
		call({ "bind", name + ".x","<Enter>", name + ".x config -background " + hoverColor });
		call({ "bind", name + ".x","<Leave>", name + ".x config -background " + primaryColor });
		call({ "place", name + ".x", "-x","-10", "-relx","0.5", "-y","5" });
		this->command = command;
		this->frameName = name;
		this->buttonName = name + ".x";
		this->dragPosition = 0;
		CreateCmd(name + ".start", [](ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[])->int {
			auto* self = ((MyScrollBar*)clientData);
			std::string parent = self->frameName;
			std::string name = self->buttonName;
			self->dragPosition = atoi(argv[1]);
			return 0;
			}, this);
		CreateCmd(name + ".moving", [](ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[])->int {
			auto* self = ((MyScrollBar*)clientData);
			std::string parent = self->frameName;
			std::string name = self->buttonName;
			int y = atoi(argv[1]);
			int n = atoi(call({ "winfo","rooty",parent }).c_str());
			int h = atoi(call({ "winfo","height",parent }).c_str());
			int sh = atoi(call({ "winfo","height",name }).c_str());
			int x = y - n - self->dragPosition;
			if (x < 0) x = 0;
			if (x > h - sh) x = h - sh;
			call({ self->command,"moveto",std::to_string(x),std::to_string(h) });
			return 0;
			}, this);
		call({ "bind",name + ".x","<ButtonPress-1>",name + ".start %y" });
		call({ "bind",name + ".x","<B1-Motion>",name + ".moving %Y" });
		colorUpdates[name] = [](std::vector<std::string> cd)->int {
			std::string name = cd[0];
			call({ name,"config","-background",secondaryColor });
			return 0;
			};
		colorUpdates[name + ".x"] = [](std::vector<std::string> cd)->int {
			std::string name = cd[0];
			call({ "bind", name,"<Enter>", name + " config -background " + hoverColor });
			call({ "bind", name,"<Leave>", name + " config -background " + primaryColor });
			return 0;
			};
		clrUpdCd[name] = { name };
		clrUpdCd[name + ".x"] = { name + ".x" };
	}
};

char toHex(int a) {
	il(a >= 0xa) return a - 0xa + 'a';
	ol return a + '0';
}
int toNum(char a) {
	il(a >= 'a' && a <= 'f') return a - 'a' + 0xa;
	el(a >= 'A' && a <= 'F') return a - 'A' + 0xa;
	ol return a - '0';
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
		el(this->head == b) this->head = a;
		il(this->tail == a) this->tail = b;
		el(this->tail == b) this->tail = a;
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
		CreateCmd(frameId+".yview", [](ClientData clientData, Tcl_Interp* interp, int argc, const char** argv)->int {
			MyList* x = (MyList*)clientData;
			x->yview(argv[1], atoi(argv[2]), (argc==4)?atoi(argv[3]):0);
			return 0;
			}, this);
		control(frameId, "frame", {"-height",std::to_string(height),"-width",std::to_string(width),"-relief","raised"});
		colorUpdates[frameId] = [](std::vector<std::string> cd)->int {
			MyList* self = (MyList*)std::atoll(cd[0].c_str());
			self->colorUpdate();
			return 0;
		};
		clrUpdCd[frameId] = { std::to_string((long long)this) };
		CreateCmd(frameId+".mw", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* t = (MyList*)clientData;
				int delta = atoi(argv[1]);
				t->yview("scroll", ((delta<0)*2-1)*3, 0);
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
					call({ ir.ctrlId+".image","config","-background",hoverColor });
					call({ ir.ctrlId+".text","config","-background",hoverColor,"-foreground",textColor });
					for (int i = 0; i < ir.commands.size(); i += 2) {
						std::string t3 = ir.ctrlId+".button"+std::to_string(i/2);
						call({ t3,"config","-image",ir.commands[i],"-background",hoverColor });
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
				call({ t2,"config","-background",selectColor });
				call({ t22,"config","-background",selectColor,"-foreground",textColor });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string t3 = t1+".button"+std::to_string(j/2);
					call({ t3,"config","-image", "blankImage","-background",selectColor });
				}
			}
			else {
				call({ t2,"config","-background",primaryColor });
				call({ t22,"config","-background",primaryColor,"-foreground",textColor });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string t3 = t1+".button"+std::to_string(j/2);
					call({ t3,"config","-image", "blankImage","-background",primaryColor });
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
		ir.color = primaryColor;
		this->owned.addEnd(ir);
		control(t1, "frame", {"-width",std::to_string(this->width),"-relief","raised"});
		il(imageId != "") {
			il(imageId != "<canvas>") {
				control(t2, "ttk::label", { "-image",imageId,"-compound","center","-width","8" });
			}
			ol{
				control(t2,"canvas",{"-height","84","-width","84","-highlightthickness","0","-background",primaryColor});
				call({ t2,"create","rect","2","2","80","80","-fill","#000000","-outline","" });
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
			MyButtonIconActive(t3, "", functions[i], functions[i+1], 0, primaryColor);
			call({ t3,"config","-image","blankImage","-background",primaryColor });
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
	void yview(std::string cmd, int first, int second) {
		il(cmd == "scroll") {
			this->offset += first;
		}
		il(cmd == "moveto") {
			this->offset = this->owned.size()-this->fieldOfView;
			if (second != 0) {
				int t = first*this->owned.size()/second;
				il(t < (this->owned.size()-this->fieldOfView)) {
					this->offset = t;
				}
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
		il(pos+siz > h-5) siz = h-pos-5;
		call({ "place",this->scroll+".x","-y",std::to_string(pos+5) });
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
			}
			call({ "destroy",t1 });
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

#endif // RIVERLAUNCHER3_CONTROL_H