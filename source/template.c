#include <gccore.h>
#include <wiiuse/wpad.h>
#include <inttypes.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>

#include "../include/fatMounter.h"
#include "mload/usb2storage.h"
#include "mload/mload_init.h"
#include "tools.h"

#define SYSTEM_MENU_TID     (u64)0x0000000100000002

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

const DISC_INTERFACE* sd = &__io_wiisd;

static u32 additional_key_count = (u32)MAX_ELEMENTS(additional_keys);
static const char *priiloader_files[] = {
    "content/title_or.tmd",
    "data/loader.ini",
    "data/hackshas.ini",
    "data/hacksh_s.ini",
    "data/password.txt",
    "data/main.nfo",
    "data/main.bin"
};
static const u32 priiloader_files_count=(u32) MAX_ELEMENTS(priiloader_files);

int wii_init(void) {
	// Initialise the video system
	VIDEO_Init();
	// This function initialises the attached controllers
	WPAD_Init();
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);
	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	// Make the display visible
	VIDEO_SetBlack(FALSE);
	// Flush the video register changes to the hardware
	VIDEO_Flush();
	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	return 1;
}

//**Functions from SysMenuInfo.c in SysCheck HD//**
u32 GetSysMenuVersion(void)
{
	static u64 TitleID ATTRIBUTE_ALIGN(32) = 0x0000000100000002LL;
	static u32 tmdSize ATTRIBUTE_ALIGN(32);

	// Get the stored TMD size for the system menu
	if (ES_GetStoredTMDSize(TitleID, &tmdSize) < 0) return false;

	signed_blob *TMD = (signed_blob *)memalign(32, (tmdSize+32)&(~31));
	memset(TMD, 0, tmdSize);

	// Get the stored TMD for the system menu
	if (ES_GetStoredTMD(TitleID, TMD, tmdSize) < 0) return false;

	// Get the system menu version from TMD
	tmd *rTMD = (tmd *)(TMD+(0x140/sizeof(tmd *)));
	u32 version = rTMD->title_version;

	free(TMD);

	// Return the system menu version
	return version;
}
// Function from SysMenuInfo.c in SysCheck HD
char GetSysMenuRegion(u32 sysVersion) {
	switch(sysVersion)
	{
		case 1:  //Pre-launch
		case 97: //2.0U
		case 193: //2.2U
		case 225: //3.0U
		case 257: //3.1U
		case 289: //3.2U
		case 353: //3.3U
		case 385: //3.4U
		case 417: //4.0U
		case 449: //4.1U
		case 54449: // mauifrog 4.1U
		case 481: //4.2U
		case 513: //4.3U
		case 545:
		case 609:
			return 'U';
			break;
		case 130: //2.0E
		case 162: //2.1E
		case 194: //2.2E
		case 226: //3.0E
		case 258: //3.1E
		case 290: //3.2E
		case 354: //3.3E
		case 386: //3.4E
		case 418: //4.0E
		case 450: //4.1E
		case 54450: // mauifrog 4.1E
		case 482: //4.2E
		case 514: //4.3E
		case 546:
		case 610:
			return 'E';
			break;
		case 128: //2.0J
		case 192: //2.2J
		case 224: //3.0J
		case 256: //3.1J
		case 288: //3.2J
		case 352: //3.3J
		case 384: //3.4J
		case 416: //4.0J
		case 448: //4.1J
		case 54448: // mauifrog 4.1J
		case 480: //4.2J
		case 512: //4.3J
		case 544:
		case 608:
			return 'J';
			break;
		case 326: //3.3K
		case 390: //3.5K
		case 454: //4.1K
		case 54454: // mauifrog 4.1K
		case 486: //4.2K
		case 518: //4.3K
			return 'K';
			break;
	}
	return 'X';
}
//** Function from SysMenuInfo.c in SysCheck HD
float GetSysMenuNintendoVersion(u32 sysVersion)
{
	float ninVersion = 0.0;

	switch (sysVersion)
	{
		case 33:
			ninVersion = 1.0f;
			break;

		case 97:
		case 128:
		case 130:
			ninVersion = 2.0f;
			break;

		case 162:
			ninVersion = 2.1f;
			break;

		case 192:
		case 193:
		case 194:
			ninVersion = 2.2f;
			break;

		case 224:
		case 225:
		case 226:
			ninVersion = 3.0f;
			break;

		case 256:
		case 257:
		case 258:
			ninVersion = 3.1f;
			break;

		case 288:
		case 289:
		case 290:
			ninVersion = 3.2f;
			break;

		case 352:
		case 353:
		case 354:
		case 326:
			ninVersion = 3.3f;
			break;

		case 384:
		case 385:
		case 386:
			ninVersion = 3.4f;
			break;

		case 390:
			ninVersion = 3.5f;
			break;

		case 416:
		case 417:
		case 418:
			ninVersion = 4.0f;
			break;

		case 448:
		case 449:
		case 450:
		case 454:
		case 54448: // mauifrog's custom version
		case 54449: // mauifrog's custom version
		case 54450: // mauifrog's custom version
		case 54454: // mauifrog's custom version
			ninVersion = 4.1f;
			break;

		case 480:
		case 481:
		case 482:
		case 486:
			ninVersion = 4.2f;
			break;

		case 512:
		case 513:
		case 514:
		case 518:
		case 544:
		case 545:
		case 546:
		case 608:
		case 609:
		case 610:
			ninVersion = 4.3f;
			break;
	}

	return ninVersion;
}

//** End Functions from SysMenuInfo.c in SysCheck HD//**
void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

priiloader_files_count = 0;

static void RetrieveSystemMenuKeys(bool Wii)
{
    signed_blob *sysmenu_stmd = NULL;
    u32 sysmenu_stmd_size = 0;
    
    tmd *sysmenu_tmd = NULL;
    tmd_content *sysmenu_boot_content = NULL;
    
    char content_path[ISFS_MAXPATH] = {0};
    u8 *sysmenu_boot_content_data = NULL;
    u32 sysmenu_boot_content_size = 0;
    
	u8 *binary_body = NULL;

	bool priiloader = false;

    /* Get System Menu TMD boot content entry */
    sysmenu_boot_content = &(sysmenu_tmd->contents[sysmenu_tmd->boot_index]);
    
    /* Check for Priiloader */
	for(u32 i = 0; i < priiloader_files_count; i++)
	{
		sprintf(content_path, "/title/%08x/%08x/%s", TITLE_UPPER(SYSTEM_MENU_TID), TITLE_LOWER(SYSTEM_MENU_TID), priiloader_files[i]);
		if (CheckIfFlashFileSystemFileExists(content_path))
		{
			priiloader = true;
			printf("Priiloader detected!\n");
		}
    
    /* Generate boot content path and read it */
    sprintf(content_path, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(SYSTEM_MENU_TID), TITLE_LOWER(SYSTEM_MENU_TID), \
            priiloader ? (0x10000000 | sysmenu_boot_content->cid) : sysmenu_boot_content->cid);
    sysmenu_boot_content_data = (u8*)ReadFileFromFlashFileSystem(content_path, &sysmenu_boot_content_size);
    if (!sysmenu_boot_content_data && priiloader)
    {
        sprintf(content_path, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(SYSTEM_MENU_TID), TITLE_LOWER(SYSTEM_MENU_TID), sysmenu_boot_content->cid);
        sysmenu_boot_content_data = (u8*)ReadFileFromFlashFileSystem(content_path, &sysmenu_boot_content_size);
    }
    
    if (!sysmenu_boot_content_data)
    {
        printf("Failed to read System Menu boot content data!\n\n");
        goto out;
    }
}

char *gcvt(double value, int ndigit, char *buf);

}
//---------------------------------------------------------------------------------
int main(int argc, char **argv, char** env) {
//---------------------------------------------------------------------------------
	wii_init();
	//FILE * fh;
	char sys_menu[3];
	char outstr[1024];
	//char tmpstr[1024];
	int curr_ios = IOS_GetVersion();
	int curr_ios_ver = IOS_GetRevisionMajor();
	int curr_ios_rev = IOS_GetRevision();
	int hw_rev = SYS_GetHollywoodRevision();
	u32 boot2_ver,num_titles;
	u32 region = CONF_GetRegion();
	ES_GetBoot2Version(&boot2_ver);
	ES_GetNumTitles(&num_titles);
	
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	printf("\x1b[2;0H");
	strcpy(outstr," ____  ____  ____  ___  _____ \n");
	strcat(outstr,"(  _ \\(  _ \\(_  _)/ __)(  _  )\n");
	strcat(outstr," )___/ )   / _)(_ \\__ \\ )(_)( \n");
	strcat(outstr,"(__)  (_)\\_)(____)(___/(_____)\n");
	strcat(outstr, " \n");                      
	strcat(outstr,"Priso......... v1.0.1\n");
	strcat(outstr,"Author......... Ilovemyhouse\n");
	strcat(outstr,"Compiled....... 21.12.2020 17:39\n");
	strcat(outstr, " \n"); 
	strcat(outstr,"WARNING! THIS TOOL WORKED ON MY 4.3E WII AND I'M NOT RESPONSABLE IF YOUR WII OR WIIU WILL GET BRICKED! IF YOU ENCOUNTER A BUG OR ISSUE; IMMEDIATLY CONTACT Ilovemyhouse ON DISCORD. BY CONTINUING; YOU ACCEPT TO THESE TERMES. IF YOU DON'T AGREE; PRESS HOME TO QUIT. THIS TOOLS IS COMMING AS IT, THERE IS NO WARRANTY!\n");
	strcat(outstr, " \n");
	strcat(outstr, "Press A to show the infos.\n");
	strcat(outstr, "Press B to show a Debug text.\n");
	strcat(outstr, "Press HOME to exit this tool.\n");
	strcat(outstr, " \n");
	
	//*Main output
	printf("%s",outstr);

	while(1) {
		// Wait for the next frame
		VIDEO_WaitVSync();
		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();
		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);
		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME )
		{
			printf("Exiting...\n");
			sleep(0);
			exit(0);
		}
		// Debug
		//if ( pressed & WPAD_BUTTON_B ) printf("Debug.\n");
		//Display console region
		if ( pressed & WPAD_BUTTON_A )
		{
			printf("Current IOS.... %d v%d r%d\n",curr_ios,curr_ios_ver,curr_ios_rev);
			printf("Boot2.......... v%d\n",boot2_ver);
			//printf("%d\n", region); //Only for debug; shows the region code.
			if (region == 0) {
				printf("Console Region. NTSC-J - Japan\n");
			} else if (region == 1) {
				printf("Console Region. NTSC-U - USA\n");
			} else if (region == 2) {
				printf("Console Region. PAL - Europe\n");	
			} else if (region == 4) {
				printf("Console Region. NTSC-K - Korean\n");	
			} else if (region == 5) {
				printf("Console Region. NTSC-C - China\n");	
			}
			if (num_titles != 0)
			{
				printf("Titles.......... %d\n",num_titles);
			}
			printf("Hollywood Rev... %d\n",hw_rev);
			gcvt(GetSysMenuNintendoVersion(GetSysMenuVersion()),3,sys_menu);
			printf("System Menu..... %s%c\n", sys_menu,GetSysMenuRegion(GetSysMenuVersion()));
		}
	}
	return 0;
}
