#ifndef _FUCKAOKANA_EXTRACT_H
#define _FUCKAOKANA_EXTRACT_H
#include <string>

bool extract_archive(std::string input, std::string output, bool overwrite, bool verbose);
bool extract_unityfs(std::string input, std::string output, bool overwrite, bool verbose);

#endif
