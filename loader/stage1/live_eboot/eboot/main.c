#include <pspsdk.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <pspctrl.h>
#include <pspinit.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspsuspend.h>
#include <psppower.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "globals.h"
#include "rebootconfig.h"
#include "functions.h"
#include "graphics.h"
#include "ya2d_main.h"

PSP_MODULE_INFO("ARK Loader", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

#define ARK_LOADADDR 0x08D30000
#define ARK_SIZE 0x8000

// ARK.BIN requires these imports
//int SysMemUserForUser_91DE343C(void* unk);
int sceKernelPowerLock(unsigned int, unsigned int);

volatile ARKConfig config = {
    .magic = ARK_CONFIG_MAGIC,
    .arkpath = DEFAULT_ARK_PATH, // We can use argv[0] (eboot's path)
    .kxploit = {0}, // should be in same folder as eboot
    .exec_mode = DEV_UNK, // let stage 2 figure this one out
    .exploit_id = LIVE_EXPLOIT_ID,
    .recovery = 0,
};
volatile FunctionTable funcs = {
    .config = &config,
    // File IO
    .IoOpen = &sceIoOpen,
    .IoRead = &sceIoRead,
    .IoWrite = &sceIoClose,
    .IoClose = &sceIoWrite,
    .IoRemove = &sceIoRemove,
    // System
    .KernelLibcTime = &sceKernelLibcTime,
    .KernelLibcClock = &sceKernelLibcClock,
    .KernelPowerLock = &sceKernelPowerLock,
    .KernelDcacheWritebackAll = &sceKernelDcacheWritebackAll,
    .KernelIcacheInvalidateAll = &sceKernelIcacheInvalidateAll,
    .DisplaySetFrameBuf = &sceDisplaySetFrameBuf,
    // Threads
    .KernelCreateThread = &sceKernelCreateThread,
    .KernelDelayThread = &sceKernelDelayThread,
    .KernelStartThread = &sceKernelStartThread,
    .KernelExitThread = &sceKernelExitThread,
    .KernelExitDeleteThread = &sceKernelExitDeleteThread,
    .KernelWaitThreadEnd = &sceKernelWaitThreadEnd,
    .KernelCreateVpl = &sceKernelCreateVpl,
    .KernelTryAllocateVpl = &sceKernelTryAllocateVpl,
    .KernelFreeVpl = &sceKernelFreeVpl,
    .KernelDeleteVpl = &sceKernelDeleteVpl,
    .KernelDeleteFpl = &sceKernelDeleteFpl,
    // Modules
    .UtilityLoadModule = &sceUtilityLoadModule,
    .UtilityUnloadModule = &sceUtilityUnloadModule,
    .UtilityLoadNetModule = &sceUtilityLoadNetModule,
    .UtilityUnloadNetModule = &sceUtilityUnloadNetModule,
    // Sysmem
    //.SysMemUserForUser_91DE343C = &SysMemUserForUser_91DE343C,
    .KernelFreePartitionMemory = &sceKernelFreePartitionMemory,
    // Intr
    .KernelCpuSuspendIntr = &sceKernelCpuSuspendIntr,
    .KernelCpuResumeIntr = &sceKernelCpuResumeIntr,
    .KernelVolatileMemUnlock = &sceKernelVolatileMemUnlock,
    // Savedata
    .UtilitySavedataGetStatus = &sceUtilitySavedataGetStatus,
    .UtilitySavedataInitStart = &sceUtilitySavedataInitStart,
    .UtilitySavedataUpdate = &sceUtilitySavedataUpdate,
    .UtilitySavedataShutdownStart = &sceUtilitySavedataShutdownStart,
    .KernelAllocPartitionMemory = &sceKernelAllocPartitionMemory,
};

int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}
 
int CallbackThread(SceSize args, void *argp) {
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();

    return 0;
}
 
int SetupCallbacks(void) {
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

int main(int argc, char** argv){

    //SetupCallbacks();

    initScreen(&sceDisplaySetFrameBuf);
    
    PRTSTR("Stage 1 Starting");
    
    char* cwd = argv[0];
    
    // set kxploit path (same dir as this loader)
    int len = strlen(cwd) - sizeof("EBOOT.PBP") + 1;
    strncpy(config.kxploit, cwd, len);
    strcat(config.kxploit, K_FILE);
    
    // set install path device (makes it compatible with ef0)
    config.arkpath[0] = cwd[0];
    config.arkpath[1] = cwd[1];

    // set ARK stage 2 loader
    char loadpath[ARK_PATH_SIZE];
    strcpy(loadpath, config.arkpath);
    strcat(loadpath, ARK4_BIN);
    
    PRTSTR1("Loading Stage 2 at: %s", loadpath);
    
    // load ARK binary
    SceUID fd = sceIoOpen(loadpath, PSP_O_RDONLY, 0);
    sceIoRead(fd, (void *)(ARK_LOADADDR), ARK_SIZE);
    sceIoClose(fd);
    sceKernelDcacheWritebackAll();

    PRTSTR("Executing ARK Stage 2");
    // execute main function
    void (* hEntryPoint)(ARKConfig*, FunctionTable*) = (void*)ARK_LOADADDR;
    hEntryPoint(&config, &funcs);
    
    return 0;
}
