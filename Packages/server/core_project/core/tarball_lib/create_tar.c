#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>

#include "./header/include.h"

int create_tarball(const char* binary_path, const char *out_tar_path, const char *entry_name) {
    struct archive *a;
    struct archive_entry *entry;
    struct stat st;
    char buffer[8192];
    int len;
    FILE *fp;

    if (stat(binary_path, &st) != 0){
        return STAT_FAIL;
    }

    a = archive_write_new();

    archive_write_set_format_pax_restricted(a);
    archive_write_add_filter_gzip(a);

    if (archive_write_open_filename(a, out_tar_path) != ARCHIVE_OK) {
        return ARCHIVE_OPEN_FAILURE;
    }

    entry = archive_entry_new();
    archive_entry_set_pathname(entry, entry_name);
    archive_entry_set_size(entry, st.st_size);
    archive_entry_set_filetype(entry, AE_IFREG); 
    archive_entry_set_perm(entry, st.st_mode);

    if (archive_write_header(a, entry) != ARCHIVE_OK) {
        archive_entry_free(entry);
        archive_write_free(a);
        return WRITING_HEADER_FAILURE;
    }

    fp = fopen(binary_path, "rb");
    if (!fp) {
        archive_entry_free(entry);
        archive_write_free(a);
        return BIN_OPEN_FAILURE;
    }

    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (archive_write_data(a, buffer, len) < 0) {
            fclose(fp);
            archive_entry_free(entry);
            archive_write_free(a);
            return DATA_WRITE_FAILURE;
        }
    }

    fclose(fp);
    archive_entry_free(entry);
    archive_write_close(a);
    archive_write_free(a);

    return TAR_CREATE_SUCCESS;
}


