#ifndef _FUCKAOKANA_ARCHIVE_LIST_H
#define _FUCKAOKANA_ARCHIVE_LIST_H
#include "linked_list.h"
#include "archive.h"

aokana_file* get_file_from_archive_list(struct LinkedList<aokana_archive*>* list, std::string file_path, bool verbose);
LinkedList<aokana_file*>* find_files_from_archive_list(struct LinkedList<aokana_archive*>* list, std::string language, std::string file_name, bool verbose, bool allow_fallback = true);

#endif
