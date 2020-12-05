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

#include <string>

#define ERROR_CPUSAGE_TOO_FEW_ARGUMENTS					1
#define ERROR_CPUSAGE_HKLM_AND_HKCU						2
#define ERROR_CPUSAGE_NO_FILE							3
#define ERROR_CPUSAGE_PARAMETER_WITHOUT_SWITCH			4

int probe(std::wstring comprobeKey, std::wstring file, bool typeLibrary);
int export_to_file(std::wstring comprobeKey, std::wstring xmlFile, std::wstring dllFileParam,
	int dll_bitness, bool keep_com_paths, bool hklm, bool hkcu, bool unattended);

int probe_net(std::wstring comprobeKey, std::wstring assembly);
