#ifndef _INSTALL_H
#define _INSTALL_H
#define TITLE_ID "NPUZ01234"
#define ARK_X "SCPS10084"
#define CONTENT_ID "IP9100-PCSI00011_00-PSMRUNTIME000000"
#define CONTENT_ID_ARK "IP9100-PCSI00011_00-PSMRUNTIME000001"


size_t GetTotalNeededDirectories(int _ARK_X);
void createPspEmuDirectories(int _ARK_X);
void placePspGameData(char* gameID);
void createBubble(char* gameID);
void copySaveFiles();
void doInstall();
int installPS1Plugin();
#endif
