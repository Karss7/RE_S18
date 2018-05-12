// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <commctrl.h>
#include <string>

#include <sstream>
#include <winuser.h>
#include <vector>
#include <commctrl.h>
#include <bitset>

using std::wstringstream;

#include <fstream>

//bool APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall, LPVOID lpReserved)
//{
//	switch (ulReasonForCall)
//	{
//	case DLL_PROCESS_ATTACH:
//		//MessageBox(nullptr, L"... is loading", L"MyDll.dll", MB_OK);
//        MessageBoxA(nullptr, "Loaded", "MyDll.dll", MB_OK);
//		break;
//	case DLL_THREAD_ATTACH:
//	case DLL_THREAD_DETACH:
//	case DLL_PROCESS_DETACH:
//		break;
//	default: break;
//	}
//	return true;
//}

#define IDD_DIALOG1                     101
#define IDD_EX_DLG                      101
#define IDC_EDIT1                       1001

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1002
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif



//extern __declspec(dllimport) char   g_szCmdLine[4096];
static const int bufferSize = 256;
static char* buffer = new char[bufferSize];

HWND g_hWnd = NULL;
HWND name_hWnd = NULL;
std::vector<HWND> vec_gWnd;
std::vector<HWND> child_windows;

HWND lv_hWnd = NULL;
HWND tc_hWnd = NULL;

WNDPROC wndProcOriginal = NULL;

void Log(std::string str){
    std::ofstream outst("D:\\Log\\Log.txt", std::ios::app | std::ios::out);
    outst << str << std::endl;
}

//S_LINK https://www.codeproject.com/Articles/10437/Extending-Task-Manager-with-DLL-Injection 

BOOL CALLBACK EnumProcNew(HWND hWnd, LPARAM p)
{
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);

    //Log("Pid: " + std::to_string(pid));
    //Log(std::to_string(((long)g_hWnd)));
    //int i = ::GetWindowText(g_hWnd, buffer, bufferSize);
    //Log(std::to_string(i) + " " + std::string(buffer));

    if(p == pid){
        if(g_hWnd == NULL)
            g_hWnd = hWnd;
        vec_gWnd.push_back(hWnd);
        //return FALSE;
    }
    //if(strcmp(ch, "Диспетчер задач") == 0)
        //name_hWnd = hWnd;

    return TRUE;
}

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM p)
{
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);

    if(p == pid){
        g_hWnd = hWnd;
        return FALSE;
    }

    return TRUE;
}


std::string GetCmdLineData(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if(hProcess != NULL){
        HANDLE hThread;
        char   szLibPath[_MAX_PATH];
        void*  pLibRemote = 0;
        DWORD  hLibModule = 0;

        HMODULE hKernel32 = ::GetModuleHandle("Kernel32");

        ::GetSystemDirectory(szLibPath, _MAX_PATH);

        strcat(szLibPath, "\\TaskExHook.dll");
        //wcscat_s(szLibPath, L"\\TaskExHook.dll");

        pLibRemote = ::VirtualAllocEx(hProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE);

        if(pLibRemote == NULL)
            return "Failed to get command line information...\r\n\r\n";

        ::WriteProcessMemory(hProcess, pLibRemote, (void*)szLibPath, sizeof(szLibPath), NULL);

        hThread = ::CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32, "LoadLibraryA"),
            pLibRemote, 0, NULL);

        if(hThread != NULL){
            ::WaitForSingleObject(hThread, INFINITE);
            ::GetExitCodeThread(hThread, &hLibModule);
            ::CloseHandle(hThread);

            ::VirtualFreeEx(hProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE);

            if(hLibModule != NULL){
                hThread = ::CreateRemoteThread(hProcess,
                    NULL, 0,
                    (LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32, "FreeLibrary"),
                    (void*)hLibModule,
                    0, NULL);

                if(hThread != NULL){
                    ::WaitForSingleObject(hThread, INFINITE);
                    ::GetExitCodeThread(hThread, &hLibModule);
                    ::CloseHandle(hThread);
                }
            }
        }

        CloseHandle(hProcess);

        return "Command Line:\r\n\t" + std::string("Not implemented");//std::string(g_szCmdLine);
    }

    return "Failed to get command line information...\r\n\r\n";
}

std::string GetModuleInfo(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if(hProcess){
        HMODULE modules[256];
        DWORD numMods = 0;

        std::stringstream ss;

        if(EnumProcessModules(hProcess, modules, 256, &numMods)){
            TCHAR sz[256];

            ss << "File: \r\n";
            if(GetModuleFileNameEx(hProcess, modules[0], sz, sizeof(sz))){
                ss << "\t" << sz << "\r\n";
            }

            ss << "\r\nModules:\r\n";

            for(int i=1; i<numMods; i++){
                if(GetModuleFileNameEx(hProcess, modules[i], sz, sizeof(sz))){
                    ss << "\t" << sz << "\r\n";
                }
            }
        }

        CloseHandle(hProcess);

        return ss.str();
    }

    return "Failed to get modules info.";
}

void ShowStats(HWND hDlg)
{
    HWND hChild = GetWindow(g_hWnd, GW_CHILD);
    hChild = GetWindow(hChild, GW_CHILD);
    int ic = SendMessage(hChild, LVM_GETITEMCOUNT, 0, 0);

    if(ic != 0){
        int pidColumn = -1;

        for(int i=0; i<20; i++){
            LVCOLUMN lvc;
            lvc.mask = LVCF_TEXT;
            lvc.pszText = new TCHAR[255];
            lvc.cchTextMax = 255;

            _tcscpy(lvc.pszText, "");

            SendMessage(hChild, LVM_GETCOLUMN, i, (LPARAM)&lvc);

            if(_tcscmp(lvc.pszText, "PID") == 0){
                pidColumn = i;
            }

            

            delete[] lvc.pszText;

            if(pidColumn != -1)
                break;
        }

        if(pidColumn != -1){
            int sel = SendMessage(hChild, LVM_GETSELECTIONMARK, 0, 0);

            if(sel == -1){
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), "Nothing Selected.");
            } else{
                LVITEM lvi;
                lvi.mask = LVIF_TEXT;
                lvi.pszText = new TCHAR[16];
                lvi.cchTextMax = 16;
                lvi.iItem = sel;
                lvi.iSubItem = pidColumn;

                SendMessage(hChild, LVM_GETITEMTEXT, sel, (LPARAM)&lvi);

                int pid = atoi(lvi.pszText);//_wtoi(lvi.pszText);
                

                delete[] lvi.pszText;

                std::string str1 = GetCmdLineData(pid);
                std::string str = GetModuleInfo(pid);

                SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), std::string(str1 + "\r\n\r\n" + str).c_str());
            }
        } else{
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), "PID Column now shown");
        }
    } else{
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), "Process List not shown.");
    }
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
    case WM_INITDIALOG:
    {
        ShowStats(hDlg);

        return TRUE;
    }

    case WM_COMMAND:
    {
        if(wParam == IDCANCEL){
            EndDialog(hDlg, IDCANCEL);
        }
    }
    }

    return FALSE;
}

BOOL GetAffinityProc(const DWORD pid, DWORD_PTR& procAffinity, DWORD_PTR& sysAffinity){

    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
    if(pHandle == NULL){
        Log("pHandle is NULL. Error " + std::to_string(GetLastError()));
        CloseHandle(pHandle);
        return NULL;
    }

    DWORD_PTR pAffinity, sAffinity;
    BOOL status = GetProcessAffinityMask(pHandle, &pAffinity, &sAffinity);
    if(status == FALSE){
        Log("status is FALSE. Error " + std::to_string(GetLastError()));
        return NULL;
    }
    CloseHandle(pHandle);

    procAffinity = pAffinity;
    sysAffinity = sAffinity;

    return TRUE;
}

LRESULT APIENTRY FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_COMMAND){
        if(LOWORD(wParam) == 2112){
            int ic = SendMessage(lv_hWnd, LVM_GETITEMCOUNT, 0, 0);
            Log("List view items count: " + std::to_string(ic));

            int pidColumn = -1;
            for(int i=0; i<20; i++){
                LVCOLUMN lvc;
                lvc.mask = LVCF_TEXT;
                lvc.pszText = new char[255];
                lvc.cchTextMax = 255;

                strcpy(lvc.pszText, "");

                SendMessage(lv_hWnd, LVM_GETCOLUMN, i, (LPARAM)&lvc);

                if(strcmp(lvc.pszText, "ИД процесса") == 0){
                    pidColumn = i;
                }

                delete[] lvc.pszText;

                if(pidColumn != -1)
                    break;
            }

            //int iPos = ListView_GetNextItem(lv_hWnd, -1, LVNI_SELECTED);
            if(pidColumn != -1){
                int iPos = SendMessage(lv_hWnd, LVM_GETSELECTIONMARK, 0, 0);
                    if(iPos != -1){
                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.pszText = new char[16];
                    lvi.cchTextMax = 16;
                    lvi.iItem = iPos;
                    lvi.iSubItem = pidColumn;
                    SendMessage(lv_hWnd, LVM_GETITEMTEXT, iPos, (LPARAM)&lvi);

                    Log("Selected PID: " + std::string(lvi.pszText));

                    int pid = atoi(lvi.pszText);
                    
                    ////S_LINK https://msdn.microsoft.com/en-us/library/windows/desktop/ms682623(v=vs.85).aspx
                    //DWORD aProcesses[1024], cbNeeded, cProcesses;
                    //unsigned int i;

                    //if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)){
                    //    Log("EnumProcesses error " + std::to_string(GetLastError()));
                    //    return CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
                    //}
                    //cProcesses = cbNeeded / sizeof(DWORD);

                    //for(int i = 0; i < cProcesses; i++){
                    //    if(aProcesses[i] != 0)
                    //        ;
                    //}
                    DWORD_PTR pAffinity, sAffinity;
                    
                    GetAffinityProc(pid, pAffinity, sAffinity);

                    if(pAffinity && sAffinity)
                        std::ofstream("D:\\Log\\Affinity.txt", std::ios::out | std::ios::app)
                            << "Process affinity: " << std::bitset<8>(pAffinity).to_string() << std::endl
                            << "System affinity: " << sAffinity << std::endl;

                    delete[] lvi.pszText;
                }
            }

        }
    }

    return CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
}

void GetDebugPrivs()
{
    HANDLE hToken;
    LUID sedebugnameValue;
    TOKEN_PRIVILEGES tp;

    if(::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)){
        if(!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)){
            ::CloseHandle(hToken);
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = sedebugnameValue;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if(!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)){
            ::CloseHandle(hToken);
        }

        ::CloseHandle(hToken);
    }
}

void LogHWND(HWND i_hwnd){
    std::ofstream output("D:\\Log\\LogHWND.txt", std::ios::app | std::ios::out);
    output << i_hwnd << std::endl;
}

BOOL CALLBACK EnumChildsProc(HWND hwnd, LPARAM lp){
    int tid = GetThreadId(hwnd);
    
    GetClassName(hwnd, buffer, bufferSize);
    if(strcmp(buffer, "SysListView32") == 0){
        if(lv_hWnd == NULL) //child_windows.push_back(hwnd);
            lv_hWnd = hwnd;
    }
    else if(strcmp(buffer, "SysTabControl32") == 0){
        if(tc_hWnd == NULL)
            tc_hWnd = hwnd;
    }

    if(lv_hWnd && tc_hWnd)
        return FALSE;

    return TRUE;
}

void ClearLogs(){
    std::ofstream("D:\\Log\\Log.txt") << "";
    std::ofstream("D:\\Log\\LogHWND.txt") << "";
    std::ofstream("D:\\Log\\Affinity.txt") << "";
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if((ul_reason_for_call == DLL_PROCESS_ATTACH)){
        MessageBoxA(nullptr, "Loaded", "MyDll.dll", MB_OK);
        
        ClearLogs();

        Log("EnumProc Starts");
        EnumWindows(EnumProcNew, GetCurrentProcessId());
        Log("EnumProc Ends");

        //MessageBoxA(nullptr, "After GetWindowsText", "debug", MB_OK);

        //Log("All hwnds with the pid " + std::to_string(GetCurrentProcessId()));
        //for (HWND h_wnd : vec_gWnd)
        //{
        //    LogHWND(h_wnd);
        //    //int len = GetWindowText(g_hWnd, buffer, bufferSize);
        //    //Log(std::to_string(len) + " " + std::string(buffer));
        //}
        //Log("VECTOR HWNDS END");

        HWND hwnd2 = FindWindow(NULL, "Диспетчер задач");
        g_hWnd = hwnd2;

        LogHWND(g_hWnd);

        if(g_hWnd){
            //Log("id is:" + std::to_string(GetCurrentProcessId()));
            ////Log(std::to_string(GetCurrentProcessId()));
            ////Log("inside");
            //MessageBoxA(NULL, "Inside", "g_hWmd", MB_OK);

            ////char sz[256];
            //

            //Log("Trying to GetWindowText");
            //wchar_t chars[256];
            //int len = GetWindowTextW(g_hWnd, chars, 256);
            //Log("Ended");
            //if(len == 0){
            //    Log("GetWindowText error: " + std::to_string(GetLastError()));
            //    return TRUE;
            //}
             
            //Log(std::string(buffer));
            //Log(std::to_string(len));
            //MessageBoxW(NULL, chars, L"debug", MB_OK);

            SetWindowText(g_hWnd, "Extended Task Manager");

            EnumChildWindows(g_hWnd, EnumChildsProc, GetCurrentThreadId());
            //Log("child_windows size: " + std::to_string(child_windows.size()));
            /*for (HWND__* ch_hwnd : child_windows)
            {
                LogHWND(ch_hwnd);
            }*/
            //HWND chl_hwnd = child_windows.front();
            //LogHWND(chl_hwnd);
            LogHWND(tc_hWnd);
            LogHWND(lv_hWnd);
            MessageBox(NULL, "childs found", "dbg", MB_OK);



            HMENU hMenu = GetMenu(g_hWnd);
            int numMenus = GetMenuItemCount(hMenu);
            Log("numMenus: " + std::to_string(numMenus));

            //HMENU hPopup = CreatePopupMenu();
            TCHAR* sz = new TCHAR[256];
            GetMenuString(hMenu, numMenus - 1, sz, 256, MF_BYPOSITION);

            if(strcmp(sz, "Get Extended Info")){
                BOOL success = AppendMenu(hMenu, MF_STRING | MF_ENABLED, 2112, "Get Extended Info");
                if(!success)
                    Log("AppendMenuError " + std::to_string(GetLastError()));
                //AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_POPUP, (UINT_PTR)hPopup, "Extensions");

                numMenus = GetMenuItemCount(hMenu);
                Log("Now numMenus: " + std::to_string(numMenus));

                SetLastError(0);
                wndProcOriginal = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)FilterProc);
                if(wndProcOriginal == NULL)
                    Log("SetWindowLong error: " + std::to_string(GetLastError()));

                //Sleep(1000);
                DrawMenuBar(g_hWnd);
            }
            

            /*int ic = 0;
            for(int i=0; i<100; ++i){
                ic = PostMessage(chl_hwnd, LVM_GETITEMCOUNT, 0, 0);
                if(ic > 0)
                    break;
                Sleep(2000);
            }
            Log("item count is " + std::to_string(ic));*/

            MessageBox(NULL, "Finished", "dbg", MB_OK);

            //MessageBox(NULL, "name changed", "dbg", MB_OK);

            //HMENU hMenu = GetMenu(g_hWnd);
            //int numMenus = GetMenuItemCount(hMenu);

            //HMENU hCheck = GetSubMenu(hMenu, numMenus - 1);

            //TCHAR sz[256];

            //GetMenuString(hMenu, numMenus - 1, sz, sizeof(sz), MF_BYPOSITION);
            //

            ////if(strcmp(sz, L"Extensions")){
            //if(_tcscmp(sz, "Extensions")){
            //    HMENU hPopup = CreatePopupMenu();

            //    AppendMenu(hPopup, MF_STRING, 2112, "Get Extended Info");
            //    AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_POPUP, (UINT_PTR)hPopup, "Extensions");

            //    //Subclass the window with our own window procedure.
            //    wndProcOriginal = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG)(WNDPROC)FilterProc); //SetWindowLongPtr

            //    DrawMenuBar(g_hWnd);

            //    GetDebugPrivs();
            //}
        }
    }

    return TRUE;
}