/********************************
	ipl Flasher 


*********************************/
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <kubridge.h>

#include "pspipl_update.h"
#include "kbooti_update.h"

#include <ipl_block_large.h>
#include <ipl_block_01g.h>
#include <kbooti_ipl_block_01g.h>

#include <cipl_01G.h>
#include <cipl_02G.h>
#include <cipl_03G.h>
#include <cipl_04G.h>
#include <cipl_05G.h>
#include <cipl_07G.h>
#include <cipl_09G.h>
#include <cipl_11G.h>


PSP_MODULE_INFO("IPLFlasher", 0x0800, 1, 0); 
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VSH);

#define printf pspDebugScreenPrintf
#define WHITE 0xFFFFF1
#define GREEN 0x0000FF00

#define KBOOTI_UPDATE_PRX "kbooti_update.prx"

u32 sceSysregGetTachyonVersion(void);		// 0xE2A5D1EE

char msg[256];
int model;
static u8 orig_ipl[0x24000] __attribute__((aligned(64)));

u8* ipl_block = ipl_block_large;

int devkit, baryon_ver;

int ReadFile(char *file, int seek, void *buf, int size)
{
	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	if (seek > 0)
	{
		if (sceIoLseek(fd, seek, PSP_SEEK_SET) != seek)
		{
			sceIoClose(fd);
			return -1;
		}
	}

	int read = sceIoRead(fd, buf, size);
	
	sceIoClose(fd);
	return read;
}


////////////////////////////////////////
void ErrorExit(int milisecs, char *fmt, ...) 
{
	va_list list;
	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);
	printf(msg);
	sceKernelDelayThread(milisecs*1000);
	sceKernelExitGame(); 
}
////////////////////////////////////////
void flash_ipl(int size, u16 key)
{

	printf("Flashing IPL...");

//	if(ReadFile("ipl_block.bin", 0 , ipl_block_large , 0x4000) < 0)
//		ErrorExit(5000,"Failed to load custom ipl!\n");

	if(pspIplUpdateClearIpl() < 0)
		ErrorExit(5000,"Failed to clear ipl!\n");

	if (pspIplUpdateSetIpl(ipl_block , size, key ) < 0)
		ErrorExit(5000,"Failed to write ipl!\n");

	printf("Done.\n");

}

void flash_kbooti(int size)
{

	printf("Flashing KBOOTI...");

	if (pspKbootiUpdateKbooti(kbooti_ipl_block_01g, (sizeof(kbooti_ipl_block_01g))) < 0)
		ErrorExit(5000,"Failed to write kbooti!\n");

	printf("Done.\n");

}

void loadIplUpdateModule(){
	SceUID mod;
	//load module
	mod = sceKernelLoadModule("ipl_update.prx", 0, NULL);

	if (mod == 0x80020139) return; // SCE_ERROR_KERNEL_EXCLUSIVE_LOAD

	if (mod < 0) {
		ErrorExit(5000,"Could not load ipl_update.prx!\n");
	}

	mod = sceKernelStartModule(mod, 0, NULL, NULL, NULL);

	if (mod == 0x80020133) return; // SCE_ERROR_KERNEL_MODULE_ALREADY_STARTED

	if (mod < 0) {
		ErrorExit(5000,"Could not start module!\n");
	}
}

int is_ta88v3(void)
{
	u32 model, tachyon;

	tachyon = sceSysregGetTachyonVersion();
	model = kuKernelGetModel();

	if(model == 1 && tachyon == 0x00600000) {
		return 1;
	}

	return 0;
}

void classicipl_menu(){

	int size;
	(void)size_ipl_block_large;

	printf("Classic cIPL installation.\n");

	ipl_block = ipl_block_large;
	if( model == 0 ) {
		memcpy( ipl_block_large , ipl_block_01g, 0x4000);
	}

	loadIplUpdateModule();

	size = pspIplUpdateGetIpl(orig_ipl);

	if(size < 0) {
		ErrorExit(5000,"Failed to get ipl!\n");
	}

	printf("\nCustom ipl Flasher for 6.6x.\n\n\n");

	int ipl_type = 0;

	if( size == 0x24000 ) {
		printf("Custom ipl is installed\n");
		size -= 0x4000;
		memmove( ipl_block_large + 0x4000 , orig_ipl + 0x4000 , size);
		ipl_type = 1;
	} else if( size == 0x20000 ) {
		printf("Raw ipl \n");
		memmove( ipl_block_large + 0x4000, orig_ipl, size);
	} else {
		printf("ipl size;%08X\n", size);
		ErrorExit(5000,"Unknown ipl!\n");
	}

	printf(" Press X to ");

	if( ipl_type ) {
		printf("Re");
	}

	printf("install CIPL\n");

	if( ipl_type ) {
		printf(" Press O to Erase CIPL and Restore Raw IPL\n");
	}

	printf(" Press L to use New cIPL.");

	printf(" Press R to cancel\n\n");
    
	while (1) {
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);

		if (pad.Buttons & PSP_CTRL_CROSS) {
			flash_ipl( size+0x4000, 0 );
			break; 
		} else if ( (pad.Buttons & PSP_CTRL_CIRCLE) && ipl_type ) {		
			printf("Flashing IPL...");

			if(pspIplUpdateClearIpl() < 0) {
				ErrorExit(5000,"Failed to clear ipl!\n");
			}

			if (pspIplUpdateSetIpl( ipl_block_large + 0x4000 , size, 0 ) < 0) {
				ErrorExit(5000,"Failed to write ipl!\n");
			}

			printf("Done.\n");
			break; 
		} else if (pad.Buttons & PSP_CTRL_RTRIGGER) {
			ErrorExit(2000,"Cancelled by user.\n");
		}
		else if (pad.Buttons & PSP_CTRL_LTRIGGER) {
			pspDebugScreenClear();
			return newipl_menu();
		}

		sceKernelDelayThread(10000);
	}

	ErrorExit(5000,"\nInstall complete. Restarting in 5 seconds...\n");
}

void devtoolipl_menu(){

	int size;
	SceUID mod;

	printf("DevTool cIPL installation.\n");

	//load module
	mod = sceKernelLoadModule(KBOOTI_UPDATE_PRX, 0, NULL);

	if (mod < 0) {
		ErrorExit(5000,"Could not load " KBOOTI_UPDATE_PRX "!\n");
	}

	mod = sceKernelStartModule(mod, 0, NULL, NULL, NULL);

	if (mod < 0) {
		ErrorExit(5000,"Could not start module!\n");
	}

	size = pspKbootiUpdateGetKbootiSize();

	if(size < 0) {
		ErrorExit(5000,"Failed to get kbooti!\n");
	}

	printf("\nCustom kbooti Flasher for kbooti.\n\n\n");

	int ipl_type = 0;
	const int IPL_SIZE = 0x21000;
	const int CIPL_SIZE = 0x23D10;

	if(size == CIPL_SIZE) {
		printf("Custom kbooti is installed\n");
		ipl_type = 1;
	} else if( size == IPL_SIZE ) {
		printf("Raw kbooti \n");
	} else {
		printf("kbooti size: %08X\n", size);
		ErrorExit(5000,"Unknown kbooti!\n");
	}

	printf(" Press X to ");

	if( ipl_type ) {
		printf("Re");
	}

	printf("install CKBOOTI\n");

	if( ipl_type ) {
		printf(" Press O to Erase CKBOOTI and Restore Raw KBOOTI\n");
	}

	printf(" Press R to cancel\n\n");
    
	while (1) {
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);

		if (pad.Buttons & PSP_CTRL_CROSS) {
			flash_kbooti( size );
			break; 
		} else if ( (pad.Buttons & PSP_CTRL_CIRCLE) && ipl_type ) {		
			printf("Flashing KBOOTI...");

			if (pspKbootiUpdateRestoreKbooti() < 0) {
				ErrorExit(5000,"Failed to write kbooti!\n");
			}

			printf("Done.\n");
			break; 
		} else if (pad.Buttons & PSP_CTRL_RTRIGGER) {
			ErrorExit(2000,"Cancelled by user.\n");
		}

		sceKernelDelayThread(10000);
	}

	ErrorExit(5000,"\nInstall complete. Restarting in 5 seconds...\n");
}


void newipl_menu(){
	int size, ipl_type;
	u16 ipl_key = 0;

	struct {
		unsigned char* buf;
		size_t size;
	} ipl_table[] = {
		{(unsigned char*)cipl_01G, size_cipl_01G},
		{(unsigned char*)cipl_02G, size_cipl_02G},
		{(unsigned char*)cipl_03G, size_cipl_03G},
		{(unsigned char*)cipl_04G, size_cipl_04G},
		{(unsigned char*)cipl_05G, size_cipl_05G},
		{(unsigned char*)NULL, 0}, // 6g
		{(unsigned char*)cipl_07G, size_cipl_07G},
		{(unsigned char*)NULL, 0}, // 8g
		{(unsigned char*)cipl_09G, size_cipl_09G},
		{(unsigned char*)NULL, 0}, // 10g
		{(unsigned char*)cipl_11G, size_cipl_11G},
	};

	int supported_models = sizeof(ipl_table)/sizeof(ipl_table[0]);

	printf("New cIPL installation.\n");

	// New cIPL should only be run via 6.61 FW
	if(devkit != 0x06060110 ) {
		ErrorExit(5000,"6.61 FW SUPPORTED ONLY!\n");
	}

	if(model >= supported_models || ipl_table[model].buf == NULL) {
		ErrorExit(5000,"This installer does not support this model.\n");
	}

	ipl_block = ipl_table[model].buf;
	if (model > 2){
		ipl_key = (model==4)?2:1;
	}

	loadIplUpdateModule();

	size = ipl_table[model].size;

	printf("\nCustom ipl Flasher for 6.61.\n\n\n");

	printf(" Press X to ");

	if( ipl_type ) {
		printf("Re");
	}

	printf("install CIPL\n");

	if( ipl_type ) {
		printf(" Press O to Erase CIPL and Restore Raw IPL\n");
	}

	printf(" Press R to cancel\n\n");
    
	while (1) {
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad, 1);

		if (pad.Buttons & PSP_CTRL_CROSS) {
			flash_ipl( size, ipl_key );
			break; 
		} else if ( (pad.Buttons & PSP_CTRL_CIRCLE) && ipl_type ) {		
			printf("Flashing IPL...");

			if(pspIplUpdateClearIpl() < 0) {
				ErrorExit(5000,"Failed to clear ipl!\n");
			}

			if (pspIplUpdateSetIpl( ipl_block, size,  ipl_key) < 0) {
				ErrorExit(5000,"Failed to write ipl!\n");
			}

			printf("Done.\n");
			break; 
		} else if (pad.Buttons & PSP_CTRL_RTRIGGER) {
			ErrorExit(2000,"Cancelled by user.\n");
		}

		sceKernelDelayThread(10000);
	}

	ErrorExit(5000,"\nInstall complete. Restarting in 5 seconds...\n");

}


int main() 
{
	SceUID kpspident;

	pspDebugScreenInit();
	pspDebugScreenSetTextColor(WHITE);
	devkit = sceKernelDevkitVersion();

	if(devkit != 0x06060010 && devkit != 0x06060110) {
		ErrorExit(5000,"FW ERROR!\n");
	}

	kpspident = pspSdkLoadStartModule("kpspident.prx", PSP_MEMORY_PARTITION_KERNEL);

	if (kpspident < 0) {
		ErrorExit(5000, "kpspident.prx loaded failed\n");
	}

	model = kuKernelGetModel();

	if (sceSysconGetBaryonVersion(&baryon_ver) < 0) {
		ErrorExit(5000, "Could not determine baryon version!\n");
	}

	if (baryon_ver == 0x00020601) {
		devtoolipl_menu();
	}
	else if(!(model == 0 || model == 1) || is_ta88v3()) {
		newipl_menu();
	}
	else {
		classicipl_menu();
	}

	return 0;
}
