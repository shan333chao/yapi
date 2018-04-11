/*
wow64ext demo

Copyright (c) 2010-2018 <http://ez8.co> <orca.zhang@yahoo.com>
This library is released under the MIT License.

Please see LICENSE file or visit https://github.com/ez8-co/wow64ext for details.
*/
#include "stdafx.h"
#include "../wow64ext.hpp"

#include <TlHelp32.h>

DWORD64 WINAPI MyGetModuleHandle(HANDLE hProcess, const TCHAR* moduleName)
{
	RemoteWriter modName(hProcess, moduleName, (lstrlen(moduleName) + 1) * sizeof(TCHAR));
	if (!modName) return NULL;

	DWORD64 hKernel32 = GetModuleHandle64(hProcess, _T("kernel32.dll"));
	struct Param {
		DWORD64 func;
		DWORD64 arg;
		DWORD64 result;
	} param = { GetProcAddress64(hProcess, hKernel32,
#ifdef UNICODE
		"GetModuleHandleW"
#else
		"GetModuleHandleA"
#endif // !UNICODE
	), modName, NULL };
	if (!param.func) return NULL;

	RemoteWriter p(hProcess, &param, sizeof(param));
	if (!p) return NULL;

	const unsigned char shellcode[] = { 0x40, 0x53, 0x48, 0x83, 0xec, 0x20, 0x48, 0x8b, 0xd9, 0x48, 0x85, 0xc9, 0x74, 0x0a, 0x48, 0x8b,
										0x49, 0x08, 0xff, 0x13, 0x48, 0x89, 0x43, 0x10, 0x33, 0xc0, 0x48, 0x83, 0xc4, 0x20, 0x5b, 0xc3 };
	/*
	DWORD WINAPI helpler(Param* param) {
00007FF7E0E81080 40 53                push        rbx
00007FF7E0E81082 48 83 EC 20          sub         rsp,20h
00007FF7E0E81086 48 8B D9             mov         rbx,rcx
	91: 			if(param) param->result = param->func(param->arg);
00007FF7E0E81089 48 85 C9             test        rcx,rcx
00007FF7E0E8108C 74 0A                je          helpler+18h (07FF7E0E81098h)
00007FF7E0E8108E 48 8B 49 08          mov         rcx,qword ptr [rcx+8]
00007FF7E0E81092 FF 13                call        qword ptr [rbx]
00007FF7E0E81094 48 89 43 10          mov         qword ptr [rbx+10h],rax
	92: 			return 0;
00007FF7E0E81098 33 C0                xor         eax,eax
	93: 		}
00007FF7E0E8109A 48 83 C4 20          add         rsp,20h
00007FF7E0E8109E 5B                   pop         rbx
00007FF7E0E8109F C3                   ret
	*/
	RemoteWriter sc(hProcess, shellcode, sizeof(shellcode) + 1, PAGE_EXECUTE_READWRITE);
	if (!sc) return NULL;

	HANDLE hThread = CreateRemoteThread64(hProcess, NULL, 100, sc, p, 0, NULL);
	if (!hThread) return FALSE;
	CloseHandle(hThread);

	ReadProcessMemory64(hProcess, p, &param, sizeof(param), NULL);
	return param.result;
}

int main()
{
#if 0
	X64Caller RtlCreateUserThread("RtlCreateUserThread");
	if (!RtlCreateUserThread) return 0;
	X64Caller LdrUnloadDll("LdrUnloadDll");
	if (!LdrUnloadDll) return 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32 = { sizeof(pe32) };
	if (Process32First(hSnapshot, &pe32)) {
		do {
			// _tprintf_s(_T("%s [%u]\n"), pe32.szExeFile, pe32.th32ProcessID);
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			BOOL isRemoteWow64 = FALSE;
			IsWow64Process(hProcess, &isRemoteWow64);
			if (!isRemoteWow64) {
				DWORD64 wow64DllBaseAddr = GetModuleHandle64(hProcess, _T("x64.dll"));
				if (wow64DllBaseAddr) {
					DWORD64 ret = RtlCreateUserThread(hProcess, NULL, FALSE, 0, 0, NULL, LdrUnloadDll, wow64DllBaseAddr, NULL, NULL);
				}
			}
		} while (Process32Next(hSnapshot, &pe32));
	}
#endif
	typedef int (NTAPI *RTL_ADJUST_PRIVILEGE)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
	RTL_ADJUST_PRIVILEGE RtlAdjustPrivilege = (RTL_ADJUST_PRIVILEGE)GetProcAddress(detail::hNtDll, "RtlAdjustPrivilege");
	RtlAdjustPrivilege(20, 1, 0, NULL);

	struct MessageBoxParam {
		DWORD64 MessageBoxA;
		DWORD64 hWnd;
		DWORD64 lpCaption;
		DWORD64 lpText;
		UINT uType;
	} param = { NULL, NULL, NULL, NULL, MB_OK };

	const TCHAR* szCaption = _T("Hello World!");
	const TCHAR* szText = _T("From ez8.co");

	/*
	    DWORD WINAPI MessageBoxDelegator(MessageBoxParam* param)
		{
	00007FF69EAA1080 48 8B C1             mov         rax,rcx  
			return param ? (param->MessageBoxA(param->hWnd, param->lpCaption, param->lpText, param->uType)) : 0;
	00007FF69EAA1083 48 85 C9             test        rcx,rcx  
	00007FF69EAA1086 74 13                je          MessageBoxDelegator+1Bh (07FF69EAA109Bh)  
	00007FF69EAA1088 44 8B 49 20          mov         r9d,dword ptr [rcx+20h]  
	00007FF69EAA108C 4C 8B 41 18          mov         r8,qword ptr [rcx+18h]  
	00007FF69EAA1090 48 8B 51 10          mov         rdx,qword ptr [rcx+10h]  
	00007FF69EAA1094 48 8B 49 08          mov         rcx,qword ptr [rcx+8]  
	00007FF69EAA1098 48 FF 20             jmp         qword ptr [rax]  
	00007FF69EAA109B 33 C0                xor         eax,eax  
		}
	00007FF69EAA109D C3                   ret 
	*/

	const char* shellcode = "\x48\x8b\xc1\x48\x85\xc9\x74\x13\x44\x8b\x49\x20\x4c\x8b\x41\x18\x48\x8b\x51\x10\x48\x8b\x49\x08\x48\xff\x20\x33\xc0\xc3";

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32 = { sizeof(pe32) };
	if (Process32First(hSnapshot, &pe32)) {
		do {
			if (_tcsicmp(pe32.szExeFile, _T("explorer.exe")))
				continue;
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

			DWORD64 user32Dll = GetModuleHandle64(hProcess, _T("user32.dll"));
			if(!user32Dll) continue;

			DWORD64 tmp = MyGetModuleHandle(hProcess, _T("user32.dll"));

#ifdef UNICODE
			param.MessageBoxA = GetProcAddress64(hProcess, user32Dll, "MessageBoxW");
#else
			param.MessageBoxA = GetProcAddress64(hProcess, user32Dll, "MessageBoxA");
#endif

			if(!param.MessageBoxA) continue;

			RemoteWriter caption(hProcess, szCaption, (lstrlen(szCaption) + 1) * sizeof(TCHAR));
			if (!(param.lpCaption = caption)) continue;

			RemoteWriter text(hProcess, szText, (lstrlen(szText) + 1) * sizeof(TCHAR));
			if (!(param.lpText = text)) continue;

			RemoteWriter p(hProcess, &param, sizeof(param));
			if (!p) continue;

			RemoteWriter sc(hProcess, shellcode, strlen(shellcode) + 1, PAGE_EXECUTE_READWRITE);
			if (!sc) continue;

			HANDLE handle = CreateRemoteThread64(hProcess, NULL, 0, sc, p, 0, NULL);
			WaitForSingleObject(handle, 1000);
			DWORD ret = 0;
			GetExitCodeThread(handle, &ret);
		} while (Process32Next(hSnapshot, &pe32));
	}

    return 0;
}

