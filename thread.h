
#ifndef RIVERLAUNCHER3_THREAD_H
#define RIVERLAUNCHER3_THREAD_H
#include <tcl.h>

struct ThreadEvent {
	Tcl_Event ev; // How did Python think of this?!
	ClientData cd;
};

#endif // RIVERLAUNCHER3_THREAD_H