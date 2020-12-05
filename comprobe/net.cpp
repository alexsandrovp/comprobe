/*
Copyright (c) 2020 Alex Vargas

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "resource.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <Windows.h>

using namespace std;

bool isDirectory(wstring path)
{
    auto attributes = GetFileAttributesW(path.c_str());
    return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool isFile(wstring path)
{
    auto attributes = GetFileAttributesW(path.c_str());
    return attributes != 0xFFFFFFFF && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

wstring extractBinResource(int nResourceId, wstring outName)
{
    HGLOBAL hResourceLoaded;
    HRSRC   hRes;
    DWORD   dwSizeRes;
    char* lpResLock;

    auto length = GetTempPathW(0, nullptr) + 1;
    wchar_t* buffer = new wchar_t[length];
    buffer[length - 1] = 0;
    length = GetTempPathW(length, buffer);
    wstring tempDir(buffer, length);
    delete[] buffer;
    
    wstringstream ss;
    ss << tempDir << rand() << GetCurrentProcessId() << GetTickCount64() << L"\\";
    tempDir = ss.str();

    CreateDirectoryW(tempDir.c_str(), nullptr);

    wstring outPath = tempDir + outName;

    hRes = FindResourceW(nullptr, MAKEINTRESOURCE(nResourceId), L"BINARY");
    if (!hRes) return L"";
    hResourceLoaded = LoadResource(nullptr, hRes);
    if (!hResourceLoaded) return L"";
    lpResLock = (char*)LockResource(hResourceLoaded);
    if (!lpResLock) return L"";
    dwSizeRes = SizeofResource(nullptr, hRes);

    std::ofstream file(outPath.c_str(), std::ios::binary);
    file.write((const char*)lpResLock, dwSizeRes);
    file.close();

    return outPath;
}

wstring extractNetProbe()
{
    return extractBinResource(IDR_COMPROBENET, L"comprobe.net.exe");
}

bool deleteNetProbe(wstring file)
{
    DeleteFileW(file.c_str());
    size_t pos = file.find_last_of(L'\\');
    if (pos != wstring::npos)
    {
        file = file.substr(0, pos);
        return RemoveDirectoryW(file.c_str());
    }
    return GetFileAttributesW(file.c_str()) == 0xFFFFFFFF;
}

int executeNetProbe(wstring exe, wstring comprobeKey, wstring assembly)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wstring aargs = L"\"" + exe + L"\" \"" + comprobeKey + L"\\\" \"" + assembly + L"\"";
    wchar_t* bargs = new wchar_t[aargs.length() + 1];
    for (size_t i = 0; i < aargs.length(); ++i) bargs[i] = aargs[i];
    bargs[aargs.length()] = 0;

    auto r = CreateProcessW(nullptr, bargs, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    delete[] bargs;
    if (!r)
    {
        auto le = GetLastError();
        wcout << "error: failed to launch net probe: " << le << endl;
        return le;
    }

    DWORD ret;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &ret);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (ret) wcout << "warning: launch net probe error: " << ret << ", probably not a .NET assembly" << endl;
    return ret;
}

int probe_net(wstring comprobeKey, wstring assembly)
{
    auto netprobe = extractNetProbe();
    if (netprobe.length() == 0)
    {
        wcout << "error: failed to extract netprobe" << endl;
        return 1;
    }

    int ret = executeNetProbe(netprobe, comprobeKey, assembly);

    if (!deleteNetProbe(netprobe))
    {
        wcout << "warning: failed to delete temporary files/folder: " << netprobe << endl;
    }

    return ret;
}