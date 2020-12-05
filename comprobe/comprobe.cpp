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

#include "comprobe.h"
#include "arguments.hpp"

#include "xmlreg/registry.h"

#include <sstream>
#include <iostream>

#include <Windows.h>

using namespace std;

void deleteEmptyKey(wstring key)
{
	auto subks = winreg::enumerateSubkeys(HKEY_CURRENT_USER, key);
	for (auto k : subks)
	{
		auto kk = key.length() == 0 ? k : key + L"\\" + k;
		deleteEmptyKey(kk);
	}
	subks = winreg::enumerateSubkeys(HKEY_CURRENT_USER, key);
	if (subks.size() == 0)
	{
		auto props = winreg::enumerateProperties(HKEY_CURRENT_USER, key);
		if (props.size() == 0) winreg::deleteKey(HKEY_CURRENT_USER, key, L"");
	}
}

bool mergeIntoHKCR(wstring comprobeKey)
{
	wstring subk = comprobeKey + L"\\hklm\\Software\\Classes";
	if (winreg::keyExists(HKEY_CURRENT_USER, subk) && !winreg::copyKey(HKEY_CURRENT_USER, subk, HKEY_CURRENT_USER, comprobeKey + L"\\hkcr"))
	{
		wcout << "error: failed to copy hklm\\software\\classes into hkcr" << endl;
		return false;
	}

	subk = comprobeKey + L"\\hkcu\\Software\\Classes";
	if (winreg::keyExists(HKEY_CURRENT_USER, subk) && !winreg::copyKey(HKEY_CURRENT_USER, subk, HKEY_CURRENT_USER, comprobeKey + L"\\hkcr"))
	{
		wcout << "error: failed to copy hkcu\\software\\classes into hkcr" << endl;
		return false;
	}
	return true;
}

int remap_then_probe(wstring comprobeKey, wstring file, bool typeLibrary)
{
	int ret = -1;
	if (winreg::remap::start(HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, comprobeKey + L"hkcr", 0))
	{
		if (winreg::remap::start(HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, comprobeKey + L"hklm", 0))
		{
			if (winreg::remap::start(HKEY_USERS, HKEY_CURRENT_USER, comprobeKey + L"hku", 0))
			{
				if (winreg::remap::start(HKEY_CURRENT_USER, HKEY_CURRENT_USER, comprobeKey + L"hkcu", 0))
				{
					winreg::createKey(HKEY_LOCAL_MACHINE, L"Software\\Classes");
					winreg::createKey(HKEY_CURRENT_USER, L"Software\\Classes");

					ret = probe(comprobeKey, file, typeLibrary);

					winreg::remap::stop(HKEY_CURRENT_USER);
				}
				winreg::remap::stop(HKEY_USERS);
			}
			winreg::remap::stop(HKEY_LOCAL_MACHINE);
		}
		winreg::remap::stop(HKEY_CLASSES_ROOT);
	}
	return ret;
}

int wmain(int argc, wchar_t* argv[])
{
    arguments args(argc, argv);
    if (args.getError())
    {
        wcout << "parameter error: " << arguments::errorDescription(args.getError()) << endl;
        return args.getError();
    }

    //SetEnvironmentVariable(L"OAPERUSERTLIBREG", L"1");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    OaEnablePerUserTLibRegistration();

	wstringstream comprobeKey;
	srand((unsigned int)GetTickCount64());
	comprobeKey << "comprobe\\" << rand() << GetCurrentProcessId() << GetTickCount64() <<"\\";
	int r = remap_then_probe(comprobeKey.str(), args.getDLL(), args.isTypeLibraryMode());

	if (!r)
	{
		deleteEmptyKey(comprobeKey.str());
		mergeIntoHKCR(comprobeKey.str());

		r = export_to_file(comprobeKey.str(), args.getXML(), args.getDLL(), args.getDLLBitness(),
			args.keepComPaths(), args.isHKCR2HKLM(), args.isHKCR2HKCU(), args.isUnattended());
		if (r) wcout << "fatal: failed to export xml (" << r << ")" << endl;
	}
	else wcout << "fatal: (" << r << ") failed to probe from " << args.getDLL() << endl;
	
	winreg::killKey(HKEY_CURRENT_USER, comprobeKey.str() + L"\\hkcr");
	winreg::killKey(HKEY_CURRENT_USER, comprobeKey.str() + L"\\hkcu");
	winreg::killKey(HKEY_CURRENT_USER, comprobeKey.str() + L"\\hklm");
	winreg::killKey(HKEY_CURRENT_USER, comprobeKey.str() + L"\\hku");
	deleteEmptyKey(L"comprobe");

    return r;
}

