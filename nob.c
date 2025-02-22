#include <string.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"
#include <unistd.h>

#define PREFIX "/usr/local"
#define SHADER_PATH PREFIX"/share/wlboomer"
#define BINDIR PREFIX"/bin"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char* const program = nob_shift_args(&argc, &argv);

    Nob_Cmd cmd = {0};

    const char* prog = "./wlboomer";
    const char* wl_protocols[] = {
        "./xdg-shell-protocol.c", "./xdg-shell-protocol.h",
        "./zwlr-screencopy-v1.c", "./zwlr-screencopy-v1.h",
        "./zwlr-layer-shell-v1.c", "./zwlr-layer-shell-v1.h",
    };

    int install = 0;
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
        if (strcmp(argv[i], "install") == 0) install = 1;
        if (strcmp(argv[i], "uninstall") == 0) {
            if (nob_file_exists(SHADER_PATH)) {
                cmd.count = 0;
                nob_cmd_append(&cmd, "rm", "-r", SHADER_PATH);
                if (!nob_cmd_run_sync(cmd)) return 1;
            }
            if (nob_file_exists(PREFIX"/wlboomer")) {
                cmd.count = 0;
                nob_cmd_append(&cmd, "rm", PREFIX"/wlboomer");
                if (!nob_cmd_run_sync(cmd)) return 1;
            }
            return 0;
        }
    }

    const char* xdg_shell_path = "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", xdg_shell_path, "./xdg-shell-protocol.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", xdg_shell_path, "./xdg-shell-protocol.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    const char* wlr_screencopy_path = "./wlr-screencopy-unstable-v1.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", wlr_screencopy_path, "./zwlr-screencopy-v1.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", wlr_screencopy_path, "./zwlr-screencopy-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    const char* wlr_layer_shell_path = "./wlr-layer-shell-unstable-v1.xml";
    nob_cmd_append(&cmd, "wayland-scanner", "client-header", wlr_layer_shell_path, "./zwlr-layer-shell-v1.h");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;
    nob_cmd_append(&cmd, "wayland-scanner", "private-code", wlr_layer_shell_path, "./zwlr-layer-shell-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1; cmd.count = 0;

    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "-Wall", "-g");
    if (install) {
        nob_cmd_append(&cmd, "-DSHADER_PATH=\""SHADER_PATH"\"");
    }

    nob_cmd_append(&cmd, "-lwayland-client", "-lwayland-egl", "-lwayland-cursor",
                   "-lxkbcommon", "-lEGL", "-lOpenGL", "-I/usr/X11R7/include", "-L/usr/X11R7/lib64");
    nob_cmd_append(&cmd, "-o", prog);
    nob_cmd_append(&cmd, "./main.c",
                   "./glad/glad.c",
                   "./screencopy_frame.c",
                   "./layer_surface.c",
                   "./keyboard.c",
                   "./pointer.c",
                   "./seat.c",
                   "./registry.c",
                   "./output.c",
                   "./xdg-shell-protocol.c",
                   "./zwlr-layer-shell-v1.c",
                   "./zwlr-screencopy-v1.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    cmd.count = 0;

    if (install) {
        if (!nob_mkdir_if_not_exists(SHADER_PATH)) return 1;
        cmd.count = 0;
        nob_cmd_append(&cmd, "cp", "main.vert", SHADER_PATH);
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
        nob_cmd_append(&cmd, "cp", "main.frag", SHADER_PATH);
        if (!nob_cmd_run_sync(cmd)) return 1;
        cmd.count = 0;
        nob_cmd_append(&cmd, "cp", prog, BINDIR"/wlboomer");
        if (!nob_cmd_run_sync(cmd)) return 1;
    }
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "run") == 0) {
            cmd.count = 0;
            nob_cmd_append(&cmd, prog);
            if (!nob_cmd_run_sync(cmd)) return 1;
        }
    }
    return 0;
}
