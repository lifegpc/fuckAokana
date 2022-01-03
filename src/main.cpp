#include "fuckaokana_config.h"
#include "fuckaokana_version.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include "extract.h"
#include "extract_all.h"
#include "fileop.h"
#include "getopt.h"
#include "wchar_util.h"

#if HAVE_FFMPEG
#include "extract_cgs.h"
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavfilter/avfilter.h"
    #include "libavformat/avformat.h"
    #include "libavutil/avutil.h"
    #include "libswscale/swscale.h"
}
#endif

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_version() {
    printf("fuckAokana v" FUCKAOKANA_VERSION "\n");
#if HAVE_FFMPEG
    printf("FFMPEG library version:\n");
    auto v = avutil_version();
    printf("libavutil   %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avcodec_version();
    printf("libavcodec  %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avformat_version();
    printf("libavformat %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avfilter_version();
    printf("libavfilter %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = swscale_version();
    printf("libswscale  %3u.%3u.%3u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    printf("FFMPEG library license:\n");
    printf("libavutil   %s\n", avutil_license());
    printf("libavcodec  %s\n", avcodec_license());
    printf("libavformat %s\n", avformat_license());
    printf("libavfilter %s\n", avfilter_license());
    printf("libswscale  %s\n", swscale_license());
#endif
}

void print_usage() {
    printf("%s", "Usage:\n\
fuckaokana e [Options] <archive> [<Output>] Extract an archive.\n"
#if HAVE_FFMPEG
"fuckaokana c [Options] [<datadir>] [<Output>] Extract and merge all CGs.\n"
#endif
"fuckaokana a [Options] [<datadir>] [<Output>] Extract all archive.\n");
}

void print_help() {
    print_usage();
    printf("%s", "\n\
Options:\n\
    -h, --help          Print this message and exit.\n\
    -V, --version       Show version and exit.\n\
    -y, --yes           Overwrite existed output file.\n\
    -v, --verbose       Enable verbose logging.\n");
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
        {"yes", 0, nullptr, 'y'},
        {"verbose", 0, nullptr, 'v'},
        nullptr,
    };
    int c;
    std::string shortopts = "-hVyv";
    std::string action = "";
    std::string input = "";
    std::string output = "";
    bool yes = false;
    bool verbose = false;
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
        case 'y':
            yes = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 1:
            if (!action.length()) {
                if (!strcmp(optarg, "e") || !strcmp(optarg, "a")) {
                    action = optarg;
#if HAVE_FFMPEG
                } else if (!strcmp(optarg, "c")) {
                    action = optarg;
#endif
                } else {
                    printf("Error: Unknown action \"%s\"\n", optarg);
#if _WIN32
                    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                    print_usage();
                    return 1;
                }
            } else if (action == "e" || action == "a" || action == "c") {
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
        if (!input.length()) {
            printf("Archive path is needed.\n");
            print_usage();
            return 1;
        }
        if (!output.length()) {
            output = fileop::basename(input);
            auto pos = output.find_last_of('.');
            if (pos != std::string::npos) {
                output = output.substr(0, pos);
            } else {
                output += "_extract";
            }
            auto dn = fileop::dirname(input);
            if (dn.length()) output = fileop::join(dn, output);
        }
        return extract_archive(input, output, yes, verbose) ? 0 : 1;
    } else if (action == "a") {
        if (!input.length()) {
            input = ".";
        }
        if (!output.length()) {
            output = input;
        }
        return extract_all_archive(input, output, yes, verbose) ? 0 : 1;
#if HAVE_FFMPEG
    } else if (action == "c") {
        if (!input.length()) {
            input = ".";
        }
        if (!output.length()) {
            output = fileop::join(input, "cg");
        }
        return extract_cgs(input, output, yes, verbose) ? 0 : 1;
#endif
    }
    return 0;
}
