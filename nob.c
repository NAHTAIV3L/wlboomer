#include <string.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"
#include <unistd.h>

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char* const program = nob_shift_args(&argc, &argv);
    char path[256];
    if (getcwd(path, sizeof(path)) == NULL) {
        nob_log(NOB_ERROR, "Could not get current path");
        return 1;
    }
    char path_flag[256];
    sprintf(path_flag, "-DINSTALL_PATH=\"%s\"", path);

    Nob_Cmd cmd = {0};

    const char* prog = "./wlboomer";
    const char* wl_protocols[] = {"./xdg-shell-protocol.c", "./xdg-shell-protocol.h",
                                  "./zwlr-screencopy-v1.c", "./zwlr-screencopy-v1.h",
                                  "./zxdg-output-v1.c",     "./zxdg-output-v1.h"};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "clean") == 0) {
            if (!nob_file_exists(prog)) {
                nob_log(NOB_INFO, "Nothing to be done");
                return 1;
            }
            cmd.count = 0;
            nob_cmd_append(&cmd, "rm", prog);
            nob_da_append_many(&cmd, wl_protocols, NOB_ARRAY_LEN(wl_protocols));
            if (!nob_cmd_run_sync(cmd)) return 1;
            return 0;
        }
    }

    const char* xdg_shell_path = "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", xdg_shell_path, "./xdg-shell-protocol.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", xdg_shell_path, "./xdg-shell-protocol.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    const char* xdg_output_path = "/usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", xdg_output_path, "./zxdg-output-v1.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", xdg_output_path, "./zxdg-output-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    const char* wlr_screencopy_path = "./wlr-screencopy-unstable-v1.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", wlr_screencopy_path, "./zwlr-screencopy-v1.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", wlr_screencopy_path, "./zwlr-screencopy-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-g");
    nob_cmd_append(&cmd, "-lwayland-client", "-lwayland-egl", "-lwayland-cursor",
        "-lxkbcommon", "-lEGL", "-lOpenGL");
    nob_cmd_append(&cmd, path_flag);
    nob_cmd_append(&cmd, "-o", prog);
    nob_cmd_append(&cmd, "./main.c",
        "./la.c",
        "./glad/glad.c",
        "./shader.c",
        "./utils.c",
        "./xdg-shell-protocol.c",
        "./zxdg-output-v1.c",
        "./zwlr-screencopy-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "run") == 0) {
            cmd.count = 0;
            nob_cmd_append(&cmd, prog);
            if (!nob_cmd_run_sync(cmd)) return 1;
        }
    }
    return 0;
}
