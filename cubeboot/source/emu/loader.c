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

bool is_swiss(char* game_path) {
    if (game_path == NULL)
        return false;
    
    int len = strlen(game_path);
    if (len < 10)
        return false;
    
    char* filename = game_path;
    for (int i = len - 1; i >= 0; i--) {
        if (game_path[i] == '/' || game_path[i] == '\\') {
            filename = &game_path[i + 1];
            break;
        }
    }
    
    int filename_len = strlen(filename);
    if (filename_len < 10)
        return false;
    
    if (strcasecmp(filename + filename_len - 4, ".dol") != 0)
        return false;

    char prefix[6];
    memcpy(prefix, filename, 5);
    prefix[5] = '\0';
    if (strcasecmp(prefix, "swiss") != 0)
        return false;

    return true;
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
    dvd_custom_open_flash("/swiss/patches/apploader.img", FILE_ENTRY_TYPE_FILE, 0);
    file_status_t* status = dvd_custom_status();
    if (status != NULL && status->result == 0) {
        igr_type = "IGRType=Apploader";
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