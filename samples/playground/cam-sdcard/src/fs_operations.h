#include <stdbool.h>



int fs_op_init(const char *disk_drive_name, const char *mnt_pt);
int fs_op_lsdir(const char *path);
int fs_op_deinit(void);