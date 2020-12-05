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

using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace comprobe.net
{
    static class Register
    {
        private static readonly IntPtr HKCR = (IntPtr)unchecked((Int32)2147483648);
        private static readonly IntPtr HKCU = (IntPtr)unchecked((Int32)2147483649);
        private static readonly IntPtr HKLM = (IntPtr)unchecked((Int32)2147483650);
        private static readonly IntPtr HKU = (IntPtr)unchecked((Int32)2147483651);

        private const uint PermRead = 2147483648;
        private const uint PermWrite = 1073741824;
        private const uint SEM_FAILCRITICALERRORS = 1;
        private const uint SEM_NOOPENFILEERRORBOX = 32768;

        [DllImport("advapi32.dll", EntryPoint = "RegCreateKeyEx", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern int RegCreateKeyEx(IntPtr hKey, string lpSubKey, uint reserved, string lpClass, uint dwOptions, uint samDesired, uint lpSecurityAttributes, out IntPtr phkResult, out uint lpdwDisposition);

        [DllImport("advapi32.dll", EntryPoint = "RegOpenKeyEx", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern int RegOpenKeyEx(IntPtr hKey, string lpSubKey, uint dwOptions, uint samDesired, out IntPtr phkResult);

        [DllImport("advapi32.dll", EntryPoint = "RegCloseKey", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern int RegCloseKey(IntPtr hKey);

        [DllImport("advapi32.dll", EntryPoint = "RegOverridePredefKey", SetLastError = true)]
        private static extern int RegOverridePredefKey(IntPtr hKey, IntPtr hTarget);
        
        [DllImport("Kernel32.dll", SetLastError = true)]
        private static extern void SetErrorMode(UInt32 errorMode);
        
        [DllImport("Oleaut32.dll", SetLastError = true)]
        private static extern void OaEnablePerUserTLibRegistration();

        private static IntPtr openKey(IntPtr hive, string key)
        {
            IntPtr newKey = IntPtr.Zero;
            int result = RegOpenKeyEx(hive, key, 0, PermWrite | PermRead, out newKey);
            if (result != 0) throw new Exception("RegCreateKeyEx failed with error " + result);
            return newKey;
        }

        private static int remap(IntPtr srcHive, IntPtr dstHive, string dstKey)
        {
            IntPtr dstHandle = openKey(dstHive, dstKey);
            int result = RegOverridePredefKey(srcHive, dstHandle);
            RegCloseKey(dstHandle);
            return result;
        }

        private static int restore(IntPtr srcHive)
        {
            return RegOverridePredefKey(srcHive, IntPtr.Zero);
        }

        public static void RegisterServer(string baseKey, string file)
        {
            try
            {
                SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
                OaEnablePerUserTLibRegistration();

                Assembly assembly = Assembly.LoadFrom(file);
                assembly.GetExportedTypes();

                bool result = false;
                if (remap(HKCR, HKCU, baseKey + "\\hkcr") == 0)
                {
                    if (remap(HKLM, HKCU, baseKey + "\\hklm") == 0)
                    {
                        if (remap(HKU, HKCU, baseKey + "\\hku") == 0)
                        {
                            if (remap(HKCU, HKCU, baseKey + "\\hkcu") == 0)
                            {
                                result = new RegistrationServices().RegisterAssembly(assembly, AssemblyRegistrationFlags.SetCodeBase);
                                restore(HKCU);
                            }
                            restore(HKU);
                        }
                        restore(HKLM);
                    }
                    restore(HKCR);
                }

                if (!result) throw new Exception("RegistrationServices.RegisterAssembly error");
            }
            finally
            {
                restore(HKCU);
                restore(HKU);
                restore(HKLM);
                restore(HKCR);
            }
        }
    }
}
