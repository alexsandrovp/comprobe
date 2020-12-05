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
#include "xmlreg/registry.h"
#include "xmlreg/xmlreg.h"

#include <string>
#include <regex>
#include <iostream>

#include <windows.h>
#include <OleCtl.h>
#include <Oleauto.h>
#include <comdef.h>

#pragma comment(lib, "Oleaut32.lib")

using namespace std;

wstring buildPathRegex(wstring path)
{
	wstring ret;
	for (wchar_t c : path)
	{
		if (c == L'\\' || c == L'/') ret += L"[\\\\/]";
		else if (c == L'.') ret += L"\\.";
		else if (c == L'[') ret += L"\\[";
		else if (c == L']') ret += L"\\]";
		else if (c == L'^') ret += L"\\^";
		else if (c == L'$') ret += L"\\$";
		else if (c == L'|') ret += L"\\|";
		else if (c == L'?') ret += L"\\?";
		else if (c == L'*') ret += L"\\*";
		else if (c == L'+') ret += L"\\+";
		else if (c == L'(') ret += L"\\(";
		else if (c == L')') ret += L"\\)";
		else if (c == L'{') ret += L"\\{";
		else if (c == L'}') ret += L"\\}";
		else ret += c;
	}
	return ret;
}

wstring getD83Path(wstring file)
{
	auto size = GetShortPathNameW(file.c_str(), nullptr, 0);
	wchar_t* buffer = new wchar_t[size];
	size = GetShortPathNameW(file.c_str(), buffer, size);
	wstring ret(buffer, size);
	delete[] buffer;
	return ret;
}

wstring getFullPath(wstring file)
{
	auto size = GetFullPathNameW(file.c_str(), 0, nullptr, nullptr);
	wchar_t* buffer = new wchar_t[size];
	size = GetFullPathNameW(file.c_str(), size, buffer, nullptr);
	wstring ret(buffer, size);
	delete[] buffer;
	return ret;
}

wstring getFileDirectoryWOTrailingSlash(wstring file)
{
	auto pos = file.find_last_of(L'\\');
	if (pos == wstring::npos)
		pos = file.find_last_of(L'/');
	if (pos == 0 || pos == wstring::npos) return L"";
	return file.substr(0, pos);
}

int dllRegisterServer(wstring comprobeKey, wstring file)
{
	typedef HRESULT(__stdcall* pDllRegisterServer) (void);
	try
	{
		int netError = probe_net(comprobeKey, file);
		if (!netError)
		{
			wcout << "successfully probed .net COM" << endl;
		}

		HMODULE hDLL = LoadLibraryW(file.c_str());
		if (hDLL == NULL)
		{
			auto le = GetLastError();
			if (!netError) return 0;
			wcout << "error: Cannot load dll " << file << " (" << le << ")" << endl;
			return le;
		}

		pDllRegisterServer DllRegisterServer = (pDllRegisterServer)GetProcAddress(hDLL, "DllRegisterServer");
		if (DllRegisterServer == NULL)
		{
			auto le = GetLastError();
			FreeLibrary(hDLL);
			if (!netError) return 0;
			wcout << "error: cannot find function DllRegisterServer in dll (" << le << ")" << endl;
			return le;
		}

		HRESULT res = DllRegisterServer();
		DWORD err = GetLastError();

		wstring errorName;
		switch (res)
		{
		case S_OK:
			wcout << "successfully called DllRegisterServer" << endl;
			return 0;
		case SELFREG_E_TYPELIB:
			errorName = L"SELFREG_E_TYPELIB";
			break;
		case SELFREG_E_CLASS:
			errorName = L"SELFREG_E_TYPELIB";
			break;
		case E_OUTOFMEMORY:
			errorName = L"SELFREG_E_TYPELIB";
			break;
		case E_UNEXPECTED:
			errorName = L"SELFREG_E_TYPELIB";
			break;
		default:
			errorName = L"really unexpected";
		}

		wcout << "error: DllRegisterServer " << errorName << " error (" << res << ")" << endl;
		return res;
	}
	catch (exception& ex)
	{
		wcout << "error: std::exception in probe(dllRegisterServer): " << ex.what() << endl;
	}
	catch (...)
	{
		wcout << "error: unknown exception in probe(dllRegisterServer)" << endl;
	}
	return -1;
}

int registerTypeLib(wstring file)
{
	try
	{
		ITypeLib* ptlib;
		HRESULT result = LoadTypeLib(file.c_str(), &ptlib);
		wstring reason;
		_com_error ce(result);
		reason = ce.ErrorMessage();

		/*
		switch (result)
		{
		case S_OK: break;
		case E_INVALIDARG: reason = L"invalid arguments"; break;
		case E_OUTOFMEMORY: reason = L"out of memory"; break;
		case TYPE_E_IOERROR: reason = L"io error"; break;
		case TYPE_E_INVALIDSTATE: reason = L"invalid state"; break;
		case TYPE_E_INVDATAREAD: reason = L"invalid data read"; break;
		case TYPE_E_UNSUPFORMAT: reason = L"unsupported format"; break;
		case TYPE_E_UNKNOWNLCID: reason = L"unknown LCID"; break;
		case TYPE_E_CANTLOADLIBRARY: reason = L"can't load library"; break;
		default: reason = L"unknown reason";
		}
		*/

		if (result != S_OK)
		{
			wcout << "error: failed to load type library: " << reason << endl;
			return result;
		}

		result = RegisterTypeLib(ptlib, file.c_str(), nullptr);

		ce = _com_error(result);
		reason = ce.ErrorMessage();
		/*
		switch (result)
		{
		case S_OK: break;
		case E_INVALIDARG: reason = L"invalid arguments"; break;
		case E_OUTOFMEMORY: reason = L"out of memory"; break;
		case TYPE_E_IOERROR: reason = L"io error"; break;
		case TYPE_E_REGISTRYACCESS: reason = L"no registry access"; break;
		case TYPE_E_INVALIDSTATE: reason = L"invalid state"; break;
		default: reason = L"unknown reason";
		}
		*/

		if (result != S_OK)
		{
			wcout << "error: failed to register type library: " << reason << endl;
			return result;
		}

		return 0;
	}
	catch (exception& ex)
	{
		wcout << "error: std::exception in probe(registerTypeLib): " << ex.what() << endl;
	}
	catch (...)
	{
		wcout << "error: unknown error in probe(registerTypeLib)" << endl;
	}
	return -1;
}

int replace_variables(const wstring& key,
	const wregex & fileparam_regex,
	const wregex& dir_regex, const wregex& dir83_regex,
	const wregex& file_regex, const wregex& file83_regex)
{
	auto props = winreg::enumerateProperties(HKEY_CURRENT_USER, key, 0);
	for (auto prop : props)
	{
		auto type = winreg::getPropertyType(HKEY_CURRENT_USER, key, prop, 0);
		if (type == REG_SZ)
		{
			wstring val = winreg::getString(HKEY_CURRENT_USER, key, prop, L"", 0);
			if (val.length() > 0)
			{
				if (regex_search(val, file_regex))
				{
					val = regex_replace(val, file_regex, L"%file%");
					if (!winreg::setString(HKEY_CURRENT_USER, key, prop, val, 0))
					{
						wcout << "error: failed to set value (file replacement)" << endl;
						return 1;
					}
				}
				else if (regex_search(val, file83_regex))
				{
					val = regex_replace(val, file83_regex, L"%file83%");
					if (!winreg::setString(HKEY_CURRENT_USER, key, prop, val, 0))
					{
						wcout << "error: failed to set value (file 8.3 replacement)" << endl;
						return 1;
					}
				}
				else if (regex_search(val, dir_regex))
				{
					val = regex_replace(val, dir_regex, L"%dir%");
					if (!winreg::setString(HKEY_CURRENT_USER, key, prop, val, 0))
					{
						wcout << "error: failed to set value (dir replacement)" << endl;
						return 1;
					}
				}
				else if (regex_search(val, dir83_regex))
				{
					val = regex_replace(val, dir83_regex, L"%dir83");
					if (!winreg::setString(HKEY_CURRENT_USER, key, prop, val, 0))
					{
						wcout << "error: failed to set value (dir 8.3 replacement)" << endl;
						return 1;
					}
				}
				else if (regex_search(val, fileparam_regex))
				{
					val = regex_replace(val, fileparam_regex, L"%file%");
					if (!winreg::setString(HKEY_CURRENT_USER, key, prop, val, 0))
					{
						wcout << "error: failed to set value (dir 8.3 replacement)" << endl;
						return 1;
					}
				}
			}
		}
	}

	auto subkeys = winreg::enumerateSubkeys(HKEY_CURRENT_USER, key, 0);
	for (auto subkey : subkeys)
	{
		int r = replace_variables(key + L"\\" + subkey,
			fileparam_regex,
			dir_regex, dir83_regex,
			file_regex, file83_regex);
		if (r) return r;
	}
	return 0;
}

int probe(wstring comprobeKey, wstring file, bool typeLibrary)
{
	return typeLibrary ? registerTypeLib(file) : dllRegisterServer(comprobeKey, file);
}

int export_to_file(wstring comprobeKey, wstring xmlFile, wstring dllFileParam, int dll_bitness,
	bool keep_com_paths, bool hklm, bool hkcu, bool unattended)
{
	if (!keep_com_paths)
	{
		wstring fullFile = getFullPath(dllFileParam);
		wstring fullDir = getFileDirectoryWOTrailingSlash(fullFile);
		wstring d83File = getD83Path(fullFile);
		wstring d83Dir = getFileDirectoryWOTrailingSlash(d83File);

		wcout << "file path: " << fullFile << endl;
		wcout << "folder path: " << fullDir << endl;
		wcout << "file path (8.3): " << d83File << endl;
		wcout << "folder path (8.3): " << d83Dir << endl;

		d83File = buildPathRegex(d83File);
		d83Dir = buildPathRegex(d83Dir);
		fullFile = buildPathRegex(fullFile);
		fullDir = buildPathRegex(fullDir);
		wstring dllFile = buildPathRegex(dllFileParam);

		wcout << "file param regex: " << dllFile << endl;
		wcout << "file regex: " << fullFile << endl;
		wcout << "folder regex: " << fullDir << endl;
		wcout << "file regex (8.3): " << d83File << endl;
		wcout << "folder regex (8.3): " << d83Dir << endl;

		wregex dir_regex(fullDir);
		wregex file_regex(fullFile);

		wregex dir83_regex(d83Dir);
		wregex file83_regex(d83File);

		wregex fileparam_regex(dllFile);

		int r = replace_variables(comprobeKey + L"hkcr",
			fileparam_regex,
			dir_regex, dir83_regex,
			file_regex, file83_regex);

		if (r)
		{
			wcout << "error: (" << r << ") replacing variables " << endl;
			return r;
		}
	}

	REGSAM redirection = dll_bitness == 32 ? KEY_WOW64_32KEY : dll_bitness == 64 ? KEY_WOW64_64KEY : 0;
	if (hklm) return export_reg(xmlFile, HKEY_CURRENT_USER, comprobeKey + L"hkcr", 0, HKEY_LOCAL_MACHINE, L"Software\\Classes", redirection, unattended, false);
	else if (hkcu) return export_reg(xmlFile, HKEY_CURRENT_USER, comprobeKey + L"hkcr", 0, HKEY_CURRENT_USER, L"Software\\Classes", redirection, unattended, false);
	return export_reg(xmlFile, HKEY_CURRENT_USER, comprobeKey + L"hkcr", 0, HKEY_CLASSES_ROOT, L"", redirection, unattended, false);
}