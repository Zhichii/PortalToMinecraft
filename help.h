
#ifndef RIVERLAUNCHER3_HELP_H
#define RIVERLAUNCHER3_HELP_H

#include <vector>
#include <string>
#include <cstdarg>	
#include <thread>
#ifdef _WIN32
#include <atlbase.h>
#include <ShlObj.h>
std::vector<HANDLE> HCOE; // Handles Close On End
#else
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#endif 
#include "language.h"
#define S std::string
#ifdef _WIN32
#define SYS_NAME "windows"
#else
#define SYS_NAME "osx"
#endif
Language* currentLanguage = nullptr;

// Return if the path exists. 
bool isExists(const std::string& pathName) {
	#ifdef _WIN32
		struct _stat32 st;
		if (_stat32(pathName.c_str(), &st) == 0)
			return true;
	#else
		struct stat st;
		if (stat(pathName.c_str(), &st) == 0)
			return true;
	#endif
	//DebugBreak();
	writeLog("File [%s] doesn't exists. ", pathName.c_str());
	return false;
}
bool isExists(const std::wstring& pathName) {
#ifdef _WIN32
	struct _stat32 st;
	if (_wstat32(pathName.c_str(), &st) == 0)
		return true;
#else
	struct stat st;
	if (wstat(pathName.c_str(), &st) == 0)
		return true;
#endif
	writeLog("File w[%s] doesn't exists. ", Strings::ws2s(pathName).c_str());
	return false;
}

// Return if the path exists and is a directory. 
bool isDir(const std::string& pathName) {
	#ifdef _WIN32
		struct _stat32 st;
		if (_stat32(pathName.c_str(), &st) == 0)
			return st.st_mode & S_IFDIR;
	#else
		struct stat st;
		if (stat(pathName.c_str(), &st) == 0)
			return st.st_mode & S_IFDIR;
	#endif
	else return false;
}
bool isDir(const std::wstring& pathName) {
	#ifdef _WIN32
		struct _stat32 st;
		if (_wstat32(pathName.c_str(), &st) == 0)
			return st.st_mode & S_IFDIR;
	#else
		struct stat st;
		if (stat(pathName.c_str(), &st) == 0)
			return st.st_mode & S_IFDIR;
	#endif
	else return false;
}

// Open a file-open dialog. 
std::string FileDialog(const std::string &path = "", const std::string& title = "", const char* filter = "All files (*.*)\0*.*\0\0") {
	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	std::string tmp(MAX_PATH, 0);
	ofn.lpstrFile = tmp.data();
	if (path != "") ofn.lpstrInitialDir = path.c_str();
	if (title != "") ofn.lpstrTitle = title.c_str();
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
	if (GetOpenFileNameA(&ofn)) return tmp.data();
	else return "";
}

// Open a folder-open dialog. 
std::string FolderDialog(std::string path = "", std::string title = "") {
	std::string ret;
	CComPtr<IFileOpenDialog> spFileOpenDialog;
	int n = 0;
	if (SUCCEEDED(n = spFileOpenDialog.CoCreateInstance(__uuidof(FileOpenDialog)))) {
		FILEOPENDIALOGOPTIONS options;
		if (SUCCEEDED(spFileOpenDialog->GetOptions(&options))) {
			spFileOpenDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			if (path != "") {
				LPITEMIDLIST pidl;
				CComPtr<IShellItem> psi;
				SHParseDisplayName(Strings::s2ws(path).c_str(), 0, &pidl, SFGAO_FOLDER, 0);
				SHCreateShellItem(nullptr, nullptr, pidl, &psi);
				spFileOpenDialog->SetFolder(psi);
			}
			if (title != "") spFileOpenDialog->SetTitle(Strings::s2ws(title).c_str());
			if (SUCCEEDED(spFileOpenDialog->Show(GetConsoleWindow()))) {
				CComPtr<IShellItem> spResult;
				if (SUCCEEDED(spFileOpenDialog->GetResult(&spResult))) {
					wchar_t* name;
					if (SUCCEEDED(spResult->GetDisplayName(SIGDN_FILESYSPATH, &name))) {
						ret = Strings::ws2s(name);
						CoTaskMemFree(name);
					}
				}
			}
		}
	}
	return std::move(ret);
}

int execThr(std::string cmd, const std::string& path = "") {
	HANDLE hRead, hWrite;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		DWORD ret = GetLastError();
		return ret ? ret : -1;
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoA(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	const char* cwd;
	if (path != "") cwd = path.c_str();
	else cwd = nullptr;
	if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, NULL,
		NULL, cwd, &si, &pi)) {
		DWORD ret = GetLastError();
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return ret ? ret : -1;
	}

	CloseHandle(hWrite);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}

std::string execGetOut(std::string cmd, const std::string& path = "") {
	std::string output;
	HANDLE hRead, hWrite;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		DWORD ret = GetLastError();
		return "";
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoA(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	const char* cwd;
	if (path != "") cwd = path.c_str();
	else cwd = nullptr;
	if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, NULL,
		NULL, cwd, &si, &pi)) {
		DWORD ret = GetLastError();
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return "";
	}

	CloseHandle(hWrite);
	char buf[512] = {};
	DWORD bytesRead;
	while (ReadFile(hRead, (char*)buf, 512, &bytesRead, NULL)) {
		output += buf;
		memset(buf, 0, MAX_PATH + 1);
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return output;
}

typedef int FUNC_EXEC(const std::string& o, void* c);

int execThrGetOut(std::string cmd, void*c, const std::string& path = "", FUNC_EXEC* fe = nullptr) {
	HANDLE hRead, hWrite;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		DWORD ret = GetLastError();
		return 1;
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoA(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	const char* cwd;
	if (path != "") cwd = path.c_str();
	else cwd = nullptr;
	if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, NULL,
		NULL, cwd, &si, &pi)) {
		DWORD ret = GetLastError();
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return 1;
	}
	CloseHandle(hWrite);

	std::thread thr([&]()->int {
		char buf[512] = {};
		DWORD bytesRead;
		while (ReadFile(hRead, (char*)buf, 512, &bytesRead, NULL)) {
			if (fe) fe(buf, c);
			memset(buf, 0, MAX_PATH + 1);
		}
		if (fe) fe("", c);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return 0;
		});
	thr.detach();
	return 0;
}

int execNotThrGetOutInvoke(std::wstring cmd, void* c, const std::wstring& path = L"", FUNC_EXEC* fe = nullptr) {
	HANDLE hRead, hWrite;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		DWORD ret = GetLastError();
		return 1;
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoW(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_SHOW;
	si.dwFlags = STARTF_USESTDHANDLES;
	const wchar_t* cwd;
	if (path != L"") cwd = path.c_str();
	else cwd = nullptr;
	if (!CreateProcessW(NULL, cmd.data(), NULL, NULL, TRUE, NULL,
		NULL, cwd, &si, &pi)) {
		DWORD ret = GetLastError();
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return 1;
	}
	CloseHandle(hWrite);
	size_t n = HCOE.size();
	HCOE.push_back(hRead);
	HCOE.push_back(pi.hThread);
	HCOE.push_back(pi.hProcess);

	char buf[2048] = {};
	DWORD bytesRead;
	while (ReadFile(hRead, buf, 2048, &bytesRead, NULL)) {
		if (fe) fe(buf, c);
		memset(buf, 0, 2048);
	}
	if (fe) fe("", c);
	CloseHandle(hRead);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}

inline void _stdcall EnDebugBreak() {
	__try {
		DebugBreak();
	}
	__except (GetExceptionCode() == EXCEPTION_BREAKPOINT ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
	}
}

#endif //RIVERLAUNCHER3_HELP_H