#pragma once

#define STAT_FAIL -1
#define ARCHIVE_OPEN_FAILURE -2
#define WRITING_HEADER_FAILURE -3
#define DATA_WRITE_FAILURE -4
#define BIN_OPEN_FAILURE   -5
#define TAR_CREATE_SUCCESS 0

int create_tarball(const char* binary_path, const char *out_tar_path, const char *entry_name);