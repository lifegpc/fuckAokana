#include "fuckaokana_config.h"
#include "fuckaokana_version.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include "extract.h"
#include "fileop.h"
#include "getopt.h"
#include "wchar_util.h"

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_version() {
    printf("fuckAokana v" FUCKAOKANA_VERSION "\n");
}

void print_usage() {
    printf("%s", "Usage:\n\
fuckaokana e [Options] <archive> [<Output>] Extract an archive.\n");
}

void print_help() {
    print_usage();
    printf("%s", "\n\
Options:\n\
    -h, --help          Print this message and exit.\n\
    -V, --version       Show version and exit.\n");
}

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char** wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
    if (argc == 1) {
        print_help();
#if _WIN32
        if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
        return 0;
    }
    struct option opts[] = {
        {"help", 0, nullptr, 'h'},
        {"version", 0, nullptr, 'V'},
        nullptr,
    };
    int c;
    std::string shortopts = "-hV";
    std::string action = "";
    std::string input = "";
    std::string output = "";
    while ((c = getopt_long(argc, argv, shortopts.c_str(), opts, nullptr)) != -1) {
        switch (c) {
        case 'h':
            print_help();
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 0;
        case 'V':
            print_version();
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 0;
        case 1:
            if (!action.length()) {
                if (!strcmp(optarg, "e")) {
                    action = optarg;
                } else {
                    printf("Error: Unknown action \"%s\"\n", optarg);
#if _WIN32
                    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                    print_usage();
                    return 1;
                }
            } else if (action == "e") {
                if (!input.length()) {
                    input = optarg;
                } else if (!output.length()) {
                    output = optarg;
                } else {
                    printf("Error: Too much postional arguments.\n");
#if _WIN32
                    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                    print_usage();
                    return 1;
                }
            }
            break;
        case '?':
        default:
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            print_usage();
            return 1;
        }
    }
#if _WIN32
    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
    if (action == "e") {
        if (!output.length()) {
            output = fileop::basename(input);
            auto pos = output.find_last_of('.');
            if (pos != std::string::npos) {
                output = output.substr(0, pos);
            }
            auto dn = fileop::dirname(input);
            if (dn.length()) output = fileop::join(dn, output);
        }
        return extract_archive(input, output) ? 0 : 1;
    }
    return 0;
}
