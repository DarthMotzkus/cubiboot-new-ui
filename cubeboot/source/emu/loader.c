#ifdef IPL_CODE
#include "../boot.h"

#include <string.h>
#include "../os.h"
#include "../flippy_sync.h"
#include "../usbgecko.h"
#include "tweaks.h"

static int setup_argv(const char** string_list, char* buffer, struct __argv* argv_struct, u32 magic) {
    int buffer_len = 0;
    int string_count = 0;
    
    while(string_list[string_count] != NULL) {
        const char* str = string_list[string_count];
        int str_len = strlen(str) + 1;
        memcpy(buffer + buffer_len, str, str_len);
        buffer_len += str_len;
        string_count++;
    }
    
    argv_struct->argvMagic = magic;
    argv_struct->commandLine = buffer;
    argv_struct->length = buffer_len;
    
    DCFlushRange(buffer, buffer_len);
    DCFlushRange(argv_struct, sizeof(struct __argv));
    
    return buffer_len;
}

static bool starts_with_swiss(const char* name, int len) {
    if (len < 5)
        return false;

    char prefix[6];
    memcpy(prefix, name, 5);
    prefix[5] = '\0';
    return strcasecmp(prefix, "swiss") == 0;
}

// True when this .dol is Swiss itself, in which case it is run directly rather than being
// handed to Swiss with Autoload= -- asking Swiss to autoload a copy of itself resets the
// console to the stock IPL.
//
// The name to test is usually the file's, but an app folder is always <name>/default.dol, so
// the file says nothing and the folder is what identifies it. Both spellings therefore work:
// swiss-gc.dol sitting loose, and apps/swiss/default.dol.
bool is_swiss(char* game_path) {
    if (game_path == NULL)
        return false;

    int len = strlen(game_path);

    // split off the last component
    int base = len;
    while (base > 0 && game_path[base - 1] != '/' && game_path[base - 1] != '\\')
        base--;

    const char* filename = &game_path[base];
    int filename_len = len - base;

    if (filename_len < 5 || strcasecmp(filename + filename_len - 4, ".dol") != 0)
        return false;

    if (starts_with_swiss(filename, filename_len))
        return true;

    // Not named for itself: fall back to the folder holding it, which is how an app is
    // identified. Only default.dol earns that -- any other name in the folder is some
    // other program that happens to live beside Swiss.
    if (strcasecmp(filename, "default.dol") != 0)
        return false;

    int dir_end = base > 0 ? base - 1 : 0;   // step over the separator
    int dir_start = dir_end;
    while (dir_start > 0 && game_path[dir_start - 1] != '/' && game_path[dir_start - 1] != '\\')
        dir_start--;

    return starts_with_swiss(&game_path[dir_start], dir_end - dir_start);
}

// Like is_swiss(), but matches a Swiss *disc image* (any extension) by basename prefix.
// Used to route a Swiss .iso/.gcm through cubeboot's native apploader boot instead of the
// Swiss autoload path -- autoloading a Swiss disc THROUGH Swiss is Swiss-in-Swiss and
// resets to the stock IPL. Normal games don't start with "swiss", so they're unaffected.
bool is_swiss_image(char* game_path) {
    if (game_path == NULL)
        return false;

    char* filename = game_path;
    int len = strlen(game_path);
    for (int i = len - 1; i >= 0; i--) {
        if (game_path[i] == '/' || game_path[i] == '\\') {
            filename = &game_path[i + 1];
            break;
        }
    }

    if (strlen(filename) < 5)
        return false;

    char prefix[6];
    memcpy(prefix, filename, 5);
    prefix[5] = '\0';
    return strcasecmp(prefix, "swiss") == 0;
}

void chainload_swiss_game(char* game_path, bool passthrough) {
    dol_info_t info;

    // dirty hack
    if (is_swiss(game_path)) {
        info = load_dol_file(game_path, false);
        run(info.entrypoint);
    }

    info = load_dol_file("/swiss-gc.dol", true);
    
    char autoload_arg[256];
    if (passthrough) {
        strcpy(autoload_arg, "Autoload=dvd:/*.gcm");
    } else {
        strcpy(autoload_arg, "Autoload=");
        const char* dev = emu_get_device();
        memcpy(autoload_arg + strlen(autoload_arg), dev, strlen(dev) + 1);
        strcat(autoload_arg, ":");
        strcat(autoload_arg, game_path);
    }

    char* igr_type = NULL;
    const char* igr_dev = emu_get_device();
    if (igr_dev != NULL && strcmp(igr_dev, "fldrv") == 0) {
        // A FlippyDrive comes back to cubiboot on its own: the drive autoloads our DOL
        // from its flash on every reboot, so Swiss's plain Reboot IGR already lands on
        // the menu. The Apploader IGR every other device needs does not work here --
        // no apploader.img on the card is involved, or required.
        igr_type = "IGRType=Reboot";
    } else {
        dvd_custom_open_flash("/swiss/patches/apploader.img", FILE_ENTRY_TYPE_FILE, 0);
        file_status_t* status = dvd_custom_status();
        if (status != NULL && status->result == 0) {
            igr_type = "IGRType=Apploader";
        }
    }

    const char* arg_list[] = {
        "swiss-gc.dol",
        autoload_arg,
        "AutoBoot=Yes",
        "BS2Boot=No",
        "Prefer Clean Boot=No",
        igr_type,
        NULL
    };

    char* argz = (void*)info.max_addr + 32;
    struct __argv* args = (void*)(info.entrypoint + 8);
    int argz_len = setup_argv(arg_list, argz, args, ARGV_MAGIC);

    const char *env_list[] = {
        "CUBEBOOT=1",
        NULL
    };
    char* envz = argz + argz_len + 32;
    struct __argv* envp = (void*)(info.entrypoint + 40);
    setup_argv(env_list, envz, envp, ENVP_MAGIC);

    run(info.entrypoint);
}   

/*void chainload_boot_game(gm_file_entry_t *boot_entry, bool passthrough) {
    chainload_swiss_game(passthrough ? NULL : boot_entry->path, passthrough);
}*/
#endif