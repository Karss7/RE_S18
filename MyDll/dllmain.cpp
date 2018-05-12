// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

const char* MainWinwowTitle = "Диспетчер задач";    //Are used to find main window and pid column
const char* PidColumnTitle = "ИД процесса";

std::string LogPath = "D:\\Log\\";

HWND g_hWnd = NULL;                 //Main window HWND
//std::vector<HWND> vec_gWnd;       //All windows of the main process
//std::vector<HWND> child_windows;  //All child windows of main process

HWND lv_hWnd = NULL;                //list view HWND
HWND tc_hWnd = NULL;                //tab control HWND

WNDPROC wndProcOriginal = NULL;     //my message handler function pointer

void Log(std::string str){
    std::ofstream outst(LogPath + "Log.txt", std::ios::app | std::ios::out);
    outst << str << std::endl;
}
void LogHWND(HWND i_hwnd){
    std::ofstream output(LogPath + "LogHWND.txt", std::ios::app | std::ios::out);
    output << i_hwnd << std::endl;
}
void ClearLogs(){
    std::ofstream(LogPath + "Log.txt") << "";
    std::ofstream(LogPath + "LogHWND.txt") << "";
    //std::ofstream("D:\\Log\\Affinity.txt") << "";
}

//S_LINK https://www.codeproject.com/Articles/10437/Extending-Task-Manager-with-DLL-Injection 

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM p)
{
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);

    if(p == pid){
        if(g_hWnd == NULL)
            g_hWnd = hWnd;
        //vec_gWnd.push_back(hWnd);
    }
    return TRUE;
}

BOOL GetAffinityProc(const DWORD pid, DWORD_PTR& procAffinity, DWORD_PTR& sysAffinity){
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
    if(pHandle == NULL){
        Log("GetAffinityProc pHandle is NULL. Error " + std::to_string(GetLastError()));
        CloseHandle(pHandle);
        return NULL;
    }

    DWORD_PTR pAffinity, sAffinity;
    BOOL status = GetProcessAffinityMask(pHandle, &pAffinity, &sAffinity);
    if(status == FALSE){
        Log("GetAffinityProc status is FALSE. Error " + std::to_string(GetLastError()));
        CloseHandle(pHandle);
        return NULL;
    }

    procAffinity = pAffinity;
    sysAffinity = sAffinity;

    CloseHandle(pHandle);
    return TRUE;
}

LRESULT APIENTRY FilterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_COMMAND){
        if(LOWORD(wParam) == 2112){
            HWND hWndHdr = (HWND)SendMessage(lv_hWnd, LVM_GETHEADER, 0, 0);
            int columnCount = (int)SendMessage(hWndHdr, HDM_GETITEMCOUNT,0,0L);

            int itemCount = SendMessage(lv_hWnd, LVM_GETITEMCOUNT, 0, 0);

            int pidColumn = -1;
            int affinityColumn = -1;
            for(int i=0; i<20; i++){
                LVCOLUMN lvc;
                lvc.mask = LVCF_TEXT;
                lvc.pszText = new char[255];
                lvc.cchTextMax = 255;

                strcpy(lvc.pszText, "");

                SendMessage(lv_hWnd, LVM_GETCOLUMN, i, (LPARAM)&lvc);

                if(strcmp(lvc.pszText, PidColumnTitle) == 0){
                    pidColumn = i;
                }
                if(strcmp(lvc.pszText, "Affinity Mask") == 0)
                    affinityColumn = i;

                delete[] lvc.pszText;

            }

            if(affinityColumn == -1){
                affinityColumn = columnCount;
                //Add new column
                //S_LINK https://msdn.microsoft.com/en-us/library/windows/desktop/hh298344(v=vs.85).aspx
                LVCOLUMN lvc;
                int iCol;

                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvc.iSubItem = columnCount;
                lvc.pszText = "Affinity Mask";
                lvc.cx = 100;
                lvc.fmt = LVCFMT_LEFT;

                if(!ListView_InsertColumn(lv_hWnd, columnCount, &lvc)){
                    Log("InsertColumn error " + std::to_string(GetLastError()));
                }
            }

            //int iPos = ListView_GetNextItem(lv_hWnd, -1, LVNI_SELECTED);
            if(pidColumn != -1){
                for(int i=0; i<itemCount; ++i){
                //int iPos = SendMessage(lv_hWnd, LVM_GETSELECTIONMARK, 0, 0);
                //if(iPos != -1){
                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.pszText = new char[16];
                    lvi.cchTextMax = 16;
                    lvi.iItem = i;
                    lvi.iSubItem = pidColumn;
                    SendMessage(lv_hWnd, LVM_GETITEMTEXT, i, (LPARAM)&lvi);

                    //Log("Selected PID: " + std::string(lvi.pszText));

                    int pid = atoi(lvi.pszText);
                    
                    DWORD_PTR pAffinity = NULL, sAffinity = NULL;
                    
                    BOOL status = GetAffinityProc(pid, pAffinity, sAffinity);

                    std::string strAffinity = std::bitset<8>(pAffinity).to_string();
                    /*std::ofstream("D:\\Log\\Affinity.txt", std::ios::out | std::ios::app)
                    << "Process affinity: " << strAffinity << std::endl
                    << "System affinity: " << sAffinity << std::endl;*/

                    if(status && pAffinity && sAffinity){
                        char* chstr = new char[strAffinity.length() + 1];
                        strcpy(chstr, strAffinity.c_str());

                        //MessageBox(g_hWnd, strAffinity.c_str(), "Process Affinity Mask", MB_OK);

                        ListView_SetItemText(lv_hWnd, i, affinityColumn, chstr);
                    }

                    delete[] lvi.pszText;
                }
            }
        }
    }

    return CallWindowProc(wndProcOriginal, hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK EnumChildsProc(HWND hwnd, LPARAM lp){
    int tid = GetThreadId(hwnd);

    int bufferSize = 256;
    char* buffer = new char[bufferSize];

    GetClassName(hwnd, buffer, bufferSize);
    if(strcmp(buffer, "SysListView32") == 0){
        if(lv_hWnd == NULL) 
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

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if((ul_reason_for_call == DLL_PROCESS_ATTACH)){
        ClearLogs();

        //MessageBoxA(nullptr, "Loaded", "MyDll.dll", MB_OK);
        /*Log("EnumProc Starts");
        EnumWindows(EnumProcNew, GetCurrentProcessId());
        Log("EnumProc Ends");*/

        //MessageBoxA(nullptr, "After GetWindowsText", "debug", MB_OK);

        g_hWnd = FindWindow(NULL, MainWinwowTitle);

        if(g_hWnd){
            Log("Main window handler is obtained");
            LogHWND(g_hWnd);
            
            SetWindowText(g_hWnd, "Extended Task Manager");

            EnumChildWindows(g_hWnd, EnumChildsProc, GetCurrentThreadId());

            if(tc_hWnd && lv_hWnd)
                Log("Child windows handlers are obtained");
            else{
                Log("Child windows handlers are not obtained. Error: " + std::to_string(GetLastError()));
                return FALSE;
            }
            LogHWND(tc_hWnd);
            LogHWND(lv_hWnd);

            HMENU hMenu = GetMenu(g_hWnd);
            TCHAR* sz = new TCHAR[256];
            int numMenus = GetMenuItemCount(hMenu);
            GetMenuString(hMenu, numMenus - 1, sz, 256, MF_BYPOSITION);

            if(strcmp(sz, "Obtain Affinity Masks")){
                BOOL success = AppendMenu(hMenu, MF_STRING | MF_ENABLED, 2112, "Obtain Affinity Masks");
                if(!success){
                    Log("AppendMenu Error " + std::to_string(GetLastError()));
                    return FALSE;
                }
                numMenus = GetMenuItemCount(hMenu);
                Log("Menu added");
                Log("Now number of menus is: " + std::to_string(numMenus));

                SetLastError(0);
                wndProcOriginal = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)FilterProc);
                if(wndProcOriginal == NULL){
                    Log("SetWindowLong error: " + std::to_string(GetLastError()));
                    return FALSE;
                }

                DrawMenuBar(g_hWnd);
            }
        }
        else{
            Log("Main window handler was not obtained. Error " + std::to_string(GetLastError()));
            MessageBox(NULL, "Try to reload program", "Failed", MB_OK);
        }
        
    }

    return TRUE;
}