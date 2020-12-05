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

#pragma once

#include "comprobe.h"

#include <map>
#include <string>
#include <sstream>

class arguments
{
	int error_code = 0;
	bool unattended = false;
	bool type_library_mode = false;
	bool hkcr2hklm = false;
	bool hkcr2hkcu = false;
	bool keep_com_paths = false;
	int dll_bitness = 0;

	std::wstring dll;
	std::wstring xml;

public:
	arguments(int argc, wchar_t* argv[])
	{
		if (argc < 3)
		{
			error_code = ERROR_CPUSAGE_TOO_FEW_ARGUMENTS;
			return;
		}

		dll = argv[1];
		if (dll.length() == 0)
		{
			error_code = ERROR_CPUSAGE_NO_FILE;
			return;
		}

		xml = argv[2];
		if (xml.length() == 0)
		{
			error_code = ERROR_CPUSAGE_NO_FILE;
			return;
		}

		std::wstring current_switch;
		std::map<std::wstring, std::wstring> tokens;

		for (int i = 3; i < argc; ++i)
		{
			std::wstring token = argv[i];
			bool is_switch = current_switch.length() == 0 && token[0] == L'-';
			if (is_switch)
			{
				current_switch = token;
				if (current_switch == L"-y" || current_switch == L"--unattended")
				{
					tokens[L"unattended"] = L"true";
					current_switch = L"";
				}
				else if (current_switch == L"-tlb" || current_switch == L"--type-library")
				{
					tokens[L"type-library"] = L"true";
					current_switch = L"";
				}
				else if (current_switch == L"-hklm" || current_switch == L"--hkcr2hklm")
				{
					tokens[L"hkcr2hklm"] = L"true";
					current_switch = L"";
				}
				else if (current_switch == L"-hkcu" || current_switch == L"--hkcr2hkcu")
				{
					tokens[L"hkcr2hkcu"] = L"true";
					current_switch = L"";
				}
				else if (current_switch == L"-kcp" || current_switch == L"--keep-com-paths")
				{
					tokens[L"keep-com-paths"] = L"true";
					current_switch = L"";
				}
			}
			else
			{
				if (current_switch == L"-bits" || current_switch == L"--dll-bitness")
					tokens[L"dll-bitness"] = token;
				else if (current_switch.length() == 0)
				{
					error_code = ERROR_CPUSAGE_PARAMETER_WITHOUT_SWITCH;
					return;
				}

				current_switch = L"";
			}
		}

		unattended = tokens.find(L"unattended") != tokens.end();
		type_library_mode = tokens.find(L"type-library") != tokens.end();
		hkcr2hklm = tokens.find(L"hkcr2hklm") != tokens.end();
		hkcr2hkcu = tokens.find(L"hkcr2hkcu") != tokens.end();
		keep_com_paths = tokens.find(L"keep-com-paths") != tokens.end();

		auto btns = tokens.find(L"dll-bitness");
		if (btns != tokens.end())
		{
			if (btns->second == L"64") dll_bitness = 64;
			else if (btns->second == L"32") dll_bitness = 32;
			else dll_bitness = 0;
		}

		if (hkcr2hklm && hkcr2hkcu)
		{
			error_code = ERROR_CPUSAGE_HKLM_AND_HKCU;
			return;
		}
	}

	std::wstring getDLL() { return dll; }
	std::wstring getXML() { return xml; }

	int getDLLBitness() { return dll_bitness; }

	bool isTypeLibraryMode() { return type_library_mode; }
	bool isHKCR2HKCU() { return hkcr2hkcu; }
	bool isHKCR2HKLM() { return hkcr2hklm; }
	bool isUnattended() { return unattended; }
	bool keepComPaths() { return keep_com_paths; }

	int getError()
	{
		return error_code;
	}

	static std::wstring errorDescription(int error_code)
	{
		switch (error_code)
		{
		case ERROR_CPUSAGE_TOO_FEW_ARGUMENTS: return L"too few arguments";
		case ERROR_CPUSAGE_HKLM_AND_HKCU: return L"cannot use both --hkcr2hklm and --hkcr2hkcu";
		case ERROR_CPUSAGE_NO_FILE: return L"must specify dll input and xml output";
		case ERROR_CPUSAGE_PARAMETER_WITHOUT_SWITCH: return L"parameter without preceding switch";
		}

		std::wstringstream ss;
		ss << L"unpecified error: " << error_code;
		return ss.str();
	}
};
