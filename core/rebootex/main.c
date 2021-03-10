/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 */

#include "rebootex.h"

RebootBufferConfiguration* reboot_conf = (RebootBufferConfiguration*)REBOOTEX_CONFIG;
ARKConfig* ark_config = (ARKConfig*)ARK_CONFIG;

// sceReboot Main Function
int (* sceReboot)(int, int, int, int) = (void *)(REBOOT_TEXT);

// Instruction Cache Invalidator
void (* sceRebootIcacheInvalidateAll)(void) = NULL;

// Data Cache Invalidator
void (* sceRebootDacheWritebackInvalidateAll)(void) = NULL;

// Sony PRX Decrypter Function Pointer
int (* SonyPRXDecrypt)(void *, unsigned int, unsigned int *) = NULL;
int (* origCheckExecFile)(unsigned char * addr, void * arg2) = NULL;

// UnpackBootConfig
int (* UnpackBootConfig)(char * buffer, int length) = NULL;

// PRO GZIP Decrypt Support
int PROPRXDecrypt(void * prx, unsigned int size, unsigned int * newsize)
{

    // GZIP Packed PRX File
    if ( (_lb((unsigned)prx + 0x150) == 0x1F && _lb((unsigned)prx + 0x151) == 0x8B)
            || (*(unsigned int *)(prx + 0x130) == 0xC01DB15D) )
    {
        // Read GZIP Size
        unsigned int compsize = *(unsigned int *)(prx + 0xB0);
        
        // Return GZIP Size
        *newsize = compsize;
        
        // Remove PRX Header
        memcpy(prx, prx + 0x150, compsize);
        
        // Fake Decrypt Success
        return 0;
    }
    
    // Decrypt Sony PRX Files
    return SonyPRXDecrypt(prx, size, newsize);
}


int _sceKernelCheckExecFile(unsigned char * addr, void * arg2)
{
    //scan structure
    //6.31 kernel modules use type 3 PRX... 0xd4~0x10C is zero padded
    int pos = 0; for(; pos < 0x38; pos++)
    {
        //nonzero byte encountered
        if(addr[pos + 212])
        {
            //forward to unsign function?
            return origCheckExecFile(addr, arg2);
        }
    }

    //return success
    return 0;
}

void loadCoreModuleStartCommon(){

    // Calculate Text Address and size
    u32 text_addr = FindTextAddrByName("sceLoaderCore");
    u32 top_addr = text_addr+0x8000; // read 32KB at most (more than enough to scan loadcore)
    
    // Fetch Original Decrypt Function Stub
    SonyPRXDecrypt = (void *)FindImportRange("memlmd", 0xEF73E85B, text_addr, top_addr);
    origCheckExecFile = (void *)FindImportRange("memlmd", 0x6192F715, text_addr, top_addr);

    // save this configuration to restore loadcore later on
    RebootexFunctions* rex_funcs = REBOOTEX_FUNCTIONS;
    rex_funcs->rebootex_decrypt = &PROPRXDecrypt;
    rex_funcs->rebootex_checkexec = &_sceKernelCheckExecFile;
    rex_funcs->orig_decrypt = SonyPRXDecrypt;
    rex_funcs->orig_checkexec = origCheckExecFile;

    u32 decrypt_call = JAL(SonyPRXDecrypt);
    u32 check_call = JAL(origCheckExecFile);

    // Hook Signcheck Function Calls
    int count = 0;
    u32 addr;
    for (addr = text_addr; addr<top_addr; addr+=4){
        u32 data = _lw(addr);
        if (data == decrypt_call){
            _sw(JAL(PROPRXDecrypt), addr);
        }
        else if (data == check_call){
            _sw(JAL(_sceKernelCheckExecFile), addr);
        }
    }
}

// Invalidate Instruction and Data Cache
void flushCache(void)
{
    // Invalidate Data Cache
    sceRebootDacheWritebackInvalidateAll();
    // Invalidate Instruction Cache
    sceRebootIcacheInvalidateAll();
}

u32 findRebootFunctions(u32 reboot_start){
    register void (* Icache)(void) = NULL;
    register void (* Dcache)(void) = NULL;
    u32 reboot_end = 0;
    // find functions
    for (u32 addr = reboot_start; ; addr+=4){
        u32 data = _lw(addr);
        if (data == 0xBD01FFC0){ // works on PSP and Vita
            u32 a = addr;
            do {a-=4;} while (_lw(a) != 0x40088000);
            Dcache = (void*)a;
        }
        else if (data == 0xBD14FFC0){ // works on PSP and Vita
            u32 a = addr;
            do {a-=4;} while (_lw(a) != 0x40088000);
            Icache = (void*)a;
        }
        else if (data == 0x3A230001){
            u32 a = addr;
            do {a-=4;} while (_lw(a) != 0x27BDFFF0);
            pspemuLfatOpen = (void*)a;
        }
        else if (data == 0x3466507E){
            UnpackBootConfig = addr-12;
        }
        else if (strcmp("ApplyPspRelSection", (char*)addr) == 0){
            reboot_end = (addr & -0x4); // found end of reboot buffer
            break;
        }
    }
    sceRebootIcacheInvalidateAll = Icache;
    sceRebootDacheWritebackInvalidateAll = Dcache;
    Icache();
    Dcache();
    return reboot_end;
}

// Entry Point
int _arkReboot(int arg1, int arg2, int arg3, int arg4) __attribute__((section(".text.startup")));
int _arkReboot(int arg1, int arg2, int arg3, int arg4)
{

    u32 reboot_start = REBOOT_TEXT;
    u32 reboot_end = findRebootFunctions(reboot_start); // scan for reboot functions
    
    // patch reboot buffer
    if (ark_config->magic == ARK_CONFIG_MAGIC){
        if (IS_PSP(ark_config)) patchRebootBufferPSP(reboot_start, reboot_end);
        else if (IS_VITA(ark_config)) patchRebootBufferVita(reboot_start, reboot_end);
        else colorDebug(0xff); // unknown device (?), don't touch it
    }
    else colorDebug(0xff); // incorrect configuration
    
    // Flush Cache
    flushCache();
    
    // Forward Call
    return sceReboot(arg1, arg2, arg3, arg4);
}
