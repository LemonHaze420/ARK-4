#ifndef ARKMENU_CONF_H
#define ARKMENU_CONF_H

#define CONFIG_PATH "ARKMENU.BIN"

typedef struct {
    unsigned char fast_gameboot; // skip pmf/at3 and gameboot animation
    unsigned char language; // default language for the menu
    unsigned char font; // default font (either the ones in flash0 or the custom one in THEME.ARK
    unsigned char plugins; // enable or disable plugins in game
    unsigned char scan_save; // enable or disable scanning savedata
    unsigned char scan_cat; // allow scanning for categorized content in /ISO and /PSP/GAME
    unsigned char scan_dlc; // allow scanning for DLC files (PBOOT.PBP)
    unsigned char swap_buttons; // whether to swap Cross and Circle
    unsigned char animation; // the background animation of the menu
    unsigned char main_menu; // default menu opened at startup (game by default)
    unsigned char sort_entries; // sort entries by name
    unsigned char show_recovery; // show recovery menu entry
    unsigned char show_fps; // show menu FPS
    unsigned char text_glow; // enable/disable text glowing function
    unsigned char screensaver; // Screensaver time (or disabled)
    unsigned char redirect_ms0; // redirect ms0 to ef0
    unsigned char vshcolor; // Advanced VSH Menu color
} t_conf;

#endif