#ifndef TK_HELPER
#define TK_HELPER
#include <tcl.h>
#include <tk.h>
#include <vector>
#include <string>
#include "strings.h"

namespace TkHelper {

	bool tclInited = false;
	bool topleveled = false;
	Tcl_Interp* global_interp;
	long long ids = 0;
	void init();
	int TclHelper_Eval(const std::vector<std::string> strs) {
		if (!tclInited) init();
		Tcl_Obj** objs = (Tcl_Obj**)malloc(strs.size() * sizeof(Tcl_Obj*));
		if (objs == nullptr) return TCL_ERROR;
		for (int i = 0; i < strs.size(); i++) {
			objs[i] = Tcl_NewStringObj(strs[i].c_str(), strs[i].size());
			Tcl_IncrRefCount(objs[i]);
			printf("%s ", strs[i].c_str());
		}
		Tcl_EvalObjv(global_interp, strs.size(), objs, 0);
		printf("\n");
		for (int i = 0; i < strs.size(); i++) {
			TclFreeObj(objs[i]);
		}
		free(objs);
		return 0;
	}

	struct Object {
		std::string t;
		std::string n;
		//Object master;
		std::string mT;
		std::string mN;
	public:
		Object(std::string type = "null", std::string name = "") {
			this->t = type;
			this->n = name;
		}
		std::string type() { return this->t; }
		std::string name() { return this->n; }
		bool isNull() { return this->t == "null"; }
		void callPre(const std::string& cmd) {
			if (this->isNull()) return;
			TclHelper_Eval({ cmd, this->n });
		}
		void operator()(const std::string& cmd) { callPre(cmd); }
		void callNext(const std::string& cmd, const std::vector<std::string>& arg) {
			if (this->isNull()) return;
			std::vector<std::string> args = { this->n };
			if (cmd != "") args.push_back(cmd);
			for (const std::string& i : args) args.push_back(i);
			TclHelper_Eval(args);
		}
		void operator[](const std::vector<std::string>& arg) { callNext("", arg); }
	};
	Object default_root;
	Object create(std::string type, Object master = {}, std::vector<std::string> arg = {}) {
			Object self;
			if (type != "toplevel" && master.isNull()) {
				if (!topleveled) {
					default_root = create("toplevel");
					self.mT = master.type();
					self.mN = master.name();
					topleveled = true;  
				}
				master = default_root;
			}
			if (type == "toplevel") topleveled = true;
			std::vector<std::string> args;
			args.push_back(type);
			self.t = type;
			self.n = master.n + "." + std::to_string(ids);
			ids++;
			args.push_back(self.n);
			for (const auto& i : arg) args.push_back(i);
			TclHelper_Eval(args);
			return self;
		}

	void init() {
		global_interp = Tcl_CreateInterp();
		Tcl_Init(global_interp);
		Tk_Init(global_interp);
		default_root = Object("toplevel", "");
		topleveled = true;
		tclInited = true;
	}

}

#endif // TK_HELPER