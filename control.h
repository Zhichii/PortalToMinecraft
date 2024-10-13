
#ifndef RIVERLAUNCHER3_CONTROL_H
#define RIVERLAUNCHER3_CONTROL_H
#include <tcl.h>
#include <tk.h>
#include <string>
#include <vector>
#include <map>

std::map<std::string,char> windows;
const std::string ROOT = ".";
std::string primaryColor;
std::string textColor;
std::string secondaryColor;
std::string hoverColor;
std::string selectColor;
BOOL dark = 0;
typedef int ClrUpdFunc(std::vector<std::string>);
std::map<std::string, ClrUpdFunc*> colorUpdates;
std::map<std::string, std::vector<std::string>> clrUpdCd;
std::vector<std::string> messageBoxes;
size_t msgBxs = 0;
FILE* tclScriptLog;
Tcl_Interp* interp;
Tk_Window mainWin;
HWND hWnd;
std::string call(const std::vector<std::string> strs, bool nolog = 1) {
	Tcl_Obj** objs = (Tcl_Obj**)(new char[strs.size() * sizeof(Tcl_Obj*)]);
	if (!nolog) {
		fprintf(tclScriptLog, "> ");
		for (int i = 0; i < strs.size(); i++) {
			fprintf(tclScriptLog, "%s ", strs[i].c_str());
		}
		fprintf(tclScriptLog, "\n");
	}
	for (int i = 0; i < strs.size(); i++) {
		objs[i] = Tcl_NewStringObj(strs[i].c_str(), (int)strs[i].size());
		Tcl_IncrRefCount(objs[i]);
	}
	if (interp == nullptr) writeLog("[interp] is null. ");
	Tcl_EvalObjv(interp, (int)strs.size(), objs, 0);
	const char* result = Tcl_GetStringResult(interp);
	std::string r = result;
	for (int i = 0; i < strs.size(); i++) {
		Tcl_DecrRefCount(objs[i]);
	}
	delete[] objs;
	if (!nolog) {
		if (r != "") fprintf(tclScriptLog, "%s\n", r.c_str());
		fflush(tclScriptLog);
	}
	return r;
}

// [](ClientData clientData, Tcl_Interp* interp, int arg, const char* argv[])->int
static void CreateCmd(std::string name, Tcl_CmdProc* proc, ClientData cd) {
	Tcl_CreateCommand(interp, name.c_str(), proc, cd, nullptr);
}

static void control(std::string name, std::string type, std::vector<std::string> arg = {}, bool nolog = 1) {
	std::vector<std::string> x = { type, name };
	for (const auto& i : arg) {
		x.push_back(i);
	}
	call(x, nolog);
	call({ name,"config","-font","font1" }, nolog);
	if (type == "toplevel") windows[name];
}

static void MyButtonIconActive(std::string name, std::string textvariable = {}, std::string icon = {}, std::string commandId = {}, int width = 0, std::string background = primaryColor) {
	control(name, "ttk::label");
	call({ name,"config","-image",icon,"-compound",((textvariable != "" ? "left" : "center")),"-textvariable",textvariable,"-width",std::to_string(width),"-background",background });
	call({ "bind",name,"<Button-1>",commandId });
	call({ "bind",name,"<Enter>",name + " config -image " + icon + "Active" + ((textvariable != "") ? (" -background " + hoverColor) : "") });
	call({ "bind",name,"<FocusIn>",name + " config -image " + icon + "Active" + ((textvariable != "") ? (" -background " + hoverColor) : "") });
	call({ "bind",name,"<Leave>",name + " config -image " + icon + ((textvariable != "") ? (" -background " + primaryColor) : "") });
	call({ "bind",name,"<FocusOut>",name + " config -image " + icon + ((textvariable != "") ? (" -background " + primaryColor) : "") });
	colorUpdates[name] = [](std::vector<std::string> cd)->int {
		std::string name = cd[0];
		std::string icon = cd[1];
		std::string tvar = cd[2];
		call({ "bind",name,"<Enter>",name + " config -image " + icon + "Active" + ((tvar != "") ? (" -background " + hoverColor) : "") });
		call({ "bind",name,"<FocusIn>",name + " config -image " + icon + "Active" + ((tvar != "") ? (" -background " + hoverColor) : "") });
		call({ "bind",name,"<Leave>",name + " config -image " + icon + ((tvar != "") ? (" -background " + primaryColor) : "") });
		call({ "bind",name,"<FocusOut>",name + " config -image " + icon + ((tvar != "") ? (" -background " + primaryColor) : "") });
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
	call({ "bind",name,"<FocusIn>",name + " config -image " + bgActive });
	call({ "bind",name,"<Leave>",name + " config -image " + bg });
	call({ "bind",name,"<FocusOut>",name + " config -image " + bg });
}

static void MyTabY(std::string name, std::string textvariable = {}, std::string command = {}) {
	control(name, "ttk::label");
	call({ name,"config","-textvariable",textvariable });
	call({ name,"config","-image","tabYImage","-compound","center","-foreground","white" });
	call({ "bind",name,"<Button-1>",command });
	call({ "bind",name,"<Enter>",name + " config -image tabYImageActive" });
	call({ "bind",name,"<Leave>",name + " config -image tabYImage" });
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
	if (a >= 0xa) return a - 0xa + 'a';
	ef return a + '0';
}
int toNum(char a) {
	if (a >= 'a' && a <= 'f') return a - 'a' + 0xa;
	lf(a >= 'A' && a <= 'F') return a - 'A' + 0xa;
	ef return a - '0';
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
		if (this->head == nullptr) {
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
		if (this->tail == nullptr || this->head == nullptr) {
			this->head = nullptr;
			this->tail = nullptr;
			return;
		}
		if (this->head == this->tail) {
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
		if (a->prev != nullptr)  a->prev->next = b;
		if (a->next != nullptr)  a->next->prev = b;
		if (b->prev != nullptr)  b->prev->next = a;
		if (b->next != nullptr)  b->next->prev = a;
		auto aP = a->prev, aN = a->next;
		a->prev = b->prev; a->next = b->next;
		b->prev = aP;      b->next = aN;
		if (this->head == a) this->head = b;
		lf(this->head == b) this->head = a;
		if (this->tail == a) this->tail = b;
		lf(this->tail == b) this->tail = a;
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
	const /*static*/ std::string sHei = "68";
	const static size_t pItm = 68;
	const static size_t iFov = 8;
	const static size_t pFov = 8*pItm;
	size_t move;
	long long selection;
	bool enabled;
	bool selEnabled;
	bool lock = false;
	size_t width;
	size_t height;
	long long velocity;
	std::string frameid;
	struct ItemRecord {
		std::string text;
		std::string imageId;
		std::string ctrlId;
		std::string holdId;
		MyList* self;
		bool* enabled;
		size_t order;
		std::vector<std::string> commands;
		std::string color;
	};
	LinkList<ItemRecord> owned;
	std::string scroll;
	std::mutex scrolling;
	std::mutex writingScroll;
	MyList(std::string frameId, std::string scrollbarId="", bool select_able=true, int width = 500) {
		this->move = 0;
		this->velocity = 0;
		this->selection = 0;
		this->enabled = true;
		this->width = width;
		this->owned = {};
		this->frameid = frameId;
		this->scroll = scrollbarId;
		this->selEnabled = select_able;
		control(frameId, "frame");
		control(frameId+".content", "frame");
		CreateCmd(frameId+".yview", [](ClientData clientData, Tcl_Interp* interp, int argc, const char** argv)->int {
			MyList* x = (MyList*)clientData;
			x->yview(argv[1], atoi(argv[2]), (argc==4)?atoi(argv[3]):0);
			return 0;
			}, this);
		colorUpdates[frameId] = [](std::vector<std::string> cd)->int {
			MyList* self = (MyList*)std::atoll(cd[0].c_str());
			self->colorUpdate();
			return 0;
		};
		clrUpdCd[frameId] = { std::to_string((long long)this) };
		CreateCmd(frameId+".mw", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* t = (MyList*)clientData;
				t->yview("scroll", atoll(argv[1]), 0);
				return 0;
		}, this);
		CreateCmd(frameId+".b1", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* list = (MyList*)clientData;
				int item = std::atoi(argv[1]);
				ItemRecord& ir = list->owned[item];
				if (list->enabled) {
					list->userSelect(ir.order);
				}
				return 0;
		}, this);
		CreateCmd(frameId+".enter", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char* argv[])->int {
				MyList* list = (MyList*)clientData;
				int item = std::atoi(argv[1]);
				ItemRecord& ir = list->owned[item];
				if (list->enabled) {
					call({ ir.ctrlId+".img","config","-background",hoverColor });
					call({ ir.ctrlId+".txt","config","-background",hoverColor,"-foreground",textColor });
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
				int item = std::atoi(argv[1]);
				ItemRecord& ir = list->owned[item];
				if (list->enabled) {
					if (list->selEnabled && item == list->selection) {
						call({ ir.ctrlId+".img","config","-background",selectColor });
						call({ ir.ctrlId+".txt","config","-background",selectColor,"-foreground",textColor });
						for (int i = 0; i < ir.commands.size(); i += 2) {
							std::string t3 = ir.ctrlId+".button"+std::to_string(i/2);
							call({ t3,"config","-image",ir.commands[i],"-background",selectColor });
						}
					}
					else {
						call({ ir.ctrlId+".img","config","-background",primaryColor });
						call({ ir.ctrlId+".txt","config","-background",primaryColor,"-foreground",textColor });
						for (int i = 0; i < ir.commands.size(); i += 2) {
							std::string t3 = ir.ctrlId+".button"+std::to_string(i/2);
							call({ t3,"config","-image","blankImage","-background",primaryColor});
						}
					}
				}
				return 0;
		}, this);
	}
	void colorUpdate() {
		if (this->move < 0) this->move = 0;
		for (int i = 0; i < this->owned.size(); i++) {
			std::string wPar = this->owned[i].ctrlId;
			std::string wImg = wPar + ".img";
			std::string wTxt = wPar + ".txt";
			if (this->selEnabled && i==this->selection) {
				call({ wImg,"config","-background",selectColor });
				call({ wTxt,"config","-background",selectColor,"-foreground",textColor });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string wBtn = wPar+".button"+std::to_string(j/2);
					call({ wBtn,"config","-image", "blankImage","-background",selectColor });
				}
			}
			else {
				call({ wImg,"config","-background",primaryColor });
				call({ wTxt,"config","-background",primaryColor,"-foreground",textColor });
				for (int j = 0; j < this->owned[i].commands.size(); j += 2) {
					std::string wBtn = wPar+".button"+std::to_string(j/2);
					call({ wBtn,"config","-image", "blankImage","-background",primaryColor });
				}
			}
		}
	}
	void update() {
		colorUpdate();
		size_t offset = floor(this->move * 1. / pItm);
		size_t i = 0;
		for (size_t j = 0; j < this->owned.size(); j ++) {
			if (j >= offset && j < offset + iFov) {
				call({ "grid",this->owned[j].holdId,"-row",std::to_string(i) });
				i++;
			} else {
				call({ "grid","forget",this->owned[j].holdId });
			}
		}
		call({ "place",this->frameid+".content","-x","0","-y",std::to_string(-((long long)this->move)) });
		call({ "raise",this->frameid+".content" });
	}
	void userSelect(size_t item) {
		this->selection = item;
		if (selEnabled) this->colorUpdate();
		call({ this->frameid+".enter", std::to_string(this->owned[item].order) });
	}
	void select(size_t item) {
		this->selection = item;
		if (selEnabled) this->colorUpdate();
	}
	void add(std::string text, std::string imageId = {}, std::vector<std::string> functions = {}) {
		this->selection = 0;
		std::string wPar = this->frameid+".content."+std::to_string(this->owned.size());
		std::string wImg = wPar+".img";
		std::string wTxt = wPar+".txt";
		std::string wHol = this->frameid+"."+std::to_string(this->owned.size())+"hold";
		ItemRecord ir;
		ir.self = this;
		ir.enabled = &(this->enabled);
		ir.order = this->owned.size();
		ir.text = text;
		ir.imageId = imageId;
		ir.ctrlId = wPar;
		ir.holdId = wHol;
		ir.commands = functions;
		ir.color = primaryColor;
		this->owned.addEnd(ir);
		control(wPar, "frame");
		if (imageId != "<canvas>") {
			control(wImg, "ttk::label", { "-image",(imageId=="")?"horizontalImage":imageId,"-compound","center"});
		}
		ef {
			control(wImg,"canvas",{"-height",sHei,"-width",sHei,"-highlightthickness","0","-background",primaryColor});
			call({ wImg,"create","rect","2","2","66","66","-fill","#000000","-outline","" });
		}
		call({ "pack",wImg,"-side","left" });
		call({ "bind",wImg,"<MouseWheel>",	this->frameid + ".mw %D" });
		call({ "bind",wImg,"<Button-1>",	this->frameid+".b1 "+std::to_string(ir.order) });
		size_t wid = this->width - 34 * functions.size() - pItm * (imageId != "");
		control(wTxt,"labelframe",{"-bd","0","-labelanchor","w","-height",sHei,"-text",text,"-width",std::to_string(wid)});
		control(wHol,"labelframe",{"-height",sHei,"-width",std::to_string(this->width) });
		call({ "bind",wTxt,"<MouseWheel>",	this->frameid+".mw %D" });
		call({ "bind",wTxt,"<Button-1>",	this->frameid+".b1 "+std::to_string(ir.order) });
		call({ "bind",wPar,"<Enter>",		this->frameid+".enter "+std::to_string(ir.order) });
		call({ "bind",wPar,"<FocusIn>",		this->frameid+".enter "+std::to_string(ir.order) });
		call({ "bind",wPar,"<Leave>",		this->frameid+".leave "+std::to_string(ir.order) });
		call({ "bind",wPar,"<FocusOut>",	this->frameid+".leave "+std::to_string(ir.order) });
		call({ "pack",wTxt,"-side","left","-fill","y" });
		for (int i = functions.size()-2; i >= 0; i -= 2) {
			std::string wBtn = wPar + ".button" + std::to_string(i/2);
			MyButtonIconActive(wBtn, "", functions[i], functions[i+1], 0, primaryColor);
			call({ wBtn,"config","-image","blankImage","-background",primaryColor });
			call({ "bind",wBtn,"<MouseWheel>",this->frameid+".mw %D" });
			call({ "pack",wBtn,"-side","right" });
		}
		call({ "grid",wPar,"-row",std::to_string(this->owned.size()) });
		this->update();
	}
	void bind(size_t index, std::string seq, std::string cmdId) {
		if (index < 0 || index >= this->owned.size()) return;
		call({ "bind",this->owned[index].ctrlId+".txt",seq,cmdId});
		if (seq != "<Button-1>" &&
			seq != "<Enter>" &&
			seq != "<Leave>") return;
	}
	void yview(std::string cmd, long long first, long long second) {
		// Caculate this->move. 
		if (cmd == "scroll") {
			writingScroll.lock();
			this->velocity += first * -1 / 5;
			writingScroll.unlock();
		}
		if (cmd == "moveto") {
			writingScroll.lock();
			this->velocity = 0;
			writingScroll.unlock();
			this->move = 0;
			if (second != 0) this->move = first * this->owned.size() * pItm / second;
		}
		if (!this->scrolling.try_lock()) return;
		size_t max = this->owned.size() * pItm - pFov;
		if (this->owned.size() * pItm < pFov) max = 0;
		// Caculate scrollbar size. 
		long long hl = atoll(call({ "winfo","height",this->scroll }).c_str()) - 6;
		if (hl < 0) hl = 0;
		size_t h = hl;
		if (this->scroll == "") return;
		size_t siz = h;
		size_t pos = 0;
		if (this->owned.size() != 0) {
			pos = this->move * h / pItm / this->owned.size();
			siz = this->iFov * h / this->owned.size();
		}
		if (pos + siz + 5 > h) siz = h - pos - 5;
		call({ this->scroll + ".x","config","-height",std::to_string(siz) });
		// Scroll
		do {
			writingScroll.lock(); {
				if (this->velocity < 0) {
					if (this->move > -velocity) this->move += velocity;
					ef{
						this->velocity = 0;// -this->velocity * 0.125;
						this->move = 0;
					}
				}
				ef{
					if (this->move + velocity <= max) this->move += velocity;
					ef{
						this->velocity = 0;// -this->velocity * 0.125;
						this->move = max;
					}
				}
				if (this->velocity > 0) this->velocity--;
				if (this->velocity < 0) this->velocity++;
			} writingScroll.unlock();
			call({ "place",this->frameid+".content","-x","0","-y",std::to_string(-((long long)this->move)) });
			if (this->owned.size() != 0) pos = this->move * h / pItm / this->owned.size(); 
			call({ "place",this->scroll + ".x","-y",std::to_string(pos + 5) });
			RedrawWindow(hWnd, NULL, NULL, RDW_VALIDATE);
			call({ "update" });
			UpdateWindow(hWnd);
			Tcl_Sleep(5);
		} while (this->velocity != 0);
		this->scrolling.unlock();
	}
	size_t index() { return this->selection; }
	ItemRecord get(size_t ind) {
		return this->owned[ind];
	}
	void able(bool state=true) {
		this->enabled = state;
	}
	void clear() {
		LinkList<ItemRecord>::Node<ItemRecord>* cur = this->owned.head;
		while(cur!=nullptr) {
			std::string wPar = cur->value.ctrlId;
			//std::string wImg = wPar + ".img";
			//std::string wTxt = wPar + ".txt";
			//std::string wBtn;
			//call({ "destroy",wTxt });
			//call({ "destroy",wImg });
			//for (int i = 0; i < cur->value.commands.size(); i += 2) {
			//	wBtn = wPar+".button"+std::to_string(i / 2);
			//	call({ "destroy",wBtn });
			//}
			call({ "destroy",wPar });
			auto next = cur->next;
			delete cur;
			cur = next;
		}
		this->owned.head = nullptr;
		this->owned.sz = 0;
		this->owned.tail = nullptr;
	}
	~MyList() {
		owned.~LinkList();
		Tcl_DeleteCommand(interp, (this->frameid+".mw").c_str());
		Tcl_DeleteCommand(interp, (this->frameid+".n1").c_str());
		Tcl_DeleteCommand(interp, (this->frameid+".enter").c_str());
		Tcl_DeleteCommand(interp, (this->frameid+".leave").c_str());
		Tcl_DeleteCommand(interp, (this->frameid+".yview").c_str());
	}
};

struct MyEdit {
	std::string id;
	std::string content;
	MyEdit(std::string name) {
		this->id = name;
		this->content = "";
		call({ "set",name+".var","666" });
		control(name, "ttk::label", { "-compound","center","-textvariable",name + ".var","-image","texteditImage"});
		call({ "bind",name,"<ButtonPress-1>","focus " + name });
		call({ "bind",name,"<FocusIn>",name+" config -image texteditImageActive" });
		call({ "bind",name,"<FocusOut>",name+" config -image texteditImage" });
		CreateCmd(name + ".key", [](ClientData clientData,
			Tcl_Interp* interp, int argc, const char** argv)->int {
				std::string name = Strings::slice1(argv[0], 0, -4);
				char a[3];
				a[0] = (char)std::atoi(argv[1]);
				a[1] = 0;
				//writeLog("ah! %s", name.c_str());
				call({ "set",name+".var",call({"set",name+".var"})+a });
				return 1;
			}, 0);
	}
	~MyEdit() {

	}
};

void turnDark(std::string name) {
	if (call({ "winfo","exists",name }) == "0") return;
	call({ "update" });
	HWND hWnd = GetParent((HWND)strtoull(call({ "winfo","id",name }).c_str(), nullptr, 16));
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
	std::string curGeo = call({ "wm","geometry",name });
	std::string curGeo2 = curGeo;
	size_t pos = Strings::find(curGeo, "+")[0] - 1;
	if (curGeo2[pos] == '0') curGeo2[pos] = '1';
	else curGeo2[pos]--;
	call({ "wm","geometry",name,curGeo2 });
	call({ "update" });
	call({ "wm","geometry",name,curGeo });
}

int messageBox(ClientData clientData, Tcl_Interp* interp, int argc, const char* argv[]) {
	msgBxs++;
	std::string name = ".w" + std::to_string(msgBxs);
	call({ "bind",name,"<KeyPress>","keyGot %k" });
	messageBoxes.push_back(name);
	call({ "toplevel",name },0);
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
	turnDark(name);
	return 0;
}

unsigned int mRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
	return ((unsigned int)r) << 24 | ((unsigned int)g) << 16 | ((unsigned int)b) << 8 | ((unsigned int)a) << 0;
}
unsigned char gR(unsigned int clr) { return (clr > 24) % 0xff; }
unsigned char gG(unsigned int clr) { return (clr > 16) % 0xff; }
unsigned char gB(unsigned int clr) { return (clr > 8 ) % 0xff; }
unsigned char gA(unsigned int clr) { return (clr > 0 ) % 0xff; }
unsigned int readPixel(Tk_PhotoImageBlock blk, unsigned int x, unsigned int y) {
	unsigned char* i = blk.pixelPtr+blk.pixelSize*(blk.width*y+x);
	return mRGBA(i[blk.offset[0]], i[blk.offset[1]], i[blk.offset[2]], i[blk.offset[3]]);
}
void setPixel(Tk_PhotoImageBlock blk, unsigned int x, unsigned int y, unsigned int color) {
	unsigned char* i = blk.pixelPtr+blk.pixelSize*(blk.width*y+x);
	i[blk.offset[0]] = gR(color);
	i[blk.offset[1]] = gG(color);
	i[blk.offset[2]] = gB(color);
	i[blk.offset[3]] = gA(color);
}

#endif // RIVERLAUNCHER3_CONTROL_H