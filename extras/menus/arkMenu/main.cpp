#include <pspkernel.h>
#include <kubridge.h>
#include <fstream>
#include "gfx.h"
#include "debug.h"
#include "common.h"
#include "system_mgr.h"
#include "gamemgr.h"
#include "browser.h"
#include "settingsmenu.h"
#include "settings_entries.h"
#include "net_mgr.h"
#include "ftp_driver.h"
#include "exit_mgr.h"
#include "lang.h"

PSP_MODULE_INFO("ARKMENU", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(17*1024);

using namespace std;

#define MAX_ENTRIES 5
static SystemEntry* entries[MAX_ENTRIES];

int main(int argc, char** argv){

    srand(time(NULL));

    int encoding = 5;
    sceIoDevctl("ms0:", 0x02425856, &encoding, sizeof(void*), NULL, 0);
    sceIoDevctl("ef0:", 0x02425856, &encoding, sizeof(void*), NULL, 0);

    intraFontInit();
    ya2d_init();

    // setup UMD disc
    sceUmdReplacePermit();

    // Load data (theme, config, font, etc)
    common::loadData(argc, argv);

    // check to run last game
    Controller pad;
    pad.update(1);
    if (pad.LT()){
        const char* last_game = common::getConf()->last_game;
        if (Eboot::isEboot(last_game)){
            Eboot* eboot = new Eboot(last_game);
            eboot->execute();
        }
        else if (Iso::isISO(last_game)){
            Iso* iso = new Iso(last_game);
            iso->execute();
        }
    }

    int n_entries = 2;

    // Setup FTP App
    if (common::getPspModel() != PSP_11000){
        entries[n_entries++] = new NetworkManager();
        // initialize FTP client driver for file browser
        Browser::ftp_driver = new FTPDriver();
    }
    // Setup settings and exit
    int max_settings = MAX_SETTINGS_OPTIONS;
    if (common::getPspModel() != PSP_GO) max_settings--;
    SettingsTable stab = { settings_entries, max_settings };
    entries[n_entries++] = new SettingsMenu(&stab, common::saveConf, false, true, true);
    entries[n_entries++] = new ExitManager();

    // Setup main App (Game or Browser)
    if (common::getConf()->main_menu == 0){
        entries[1] = new Browser();
        entries[0] = new GameManager();
        GameManager::updateGameList(NULL);
    }
    else{
        entries[0] = new Browser();
        entries[1] = new GameManager();
    }
    
    // Initialize Menu
    SystemMgr::initMenu(entries, n_entries);

    // Handle control to Menu
    SystemMgr::startMenu();

    // Cleanup data and exit
    SystemMgr::endMenu();
    common::deleteData();
    intraFontShutdown();
    ya2d_shutdown();

    sctrlKernelExitVSH(NULL);
    
    return 0;
}
