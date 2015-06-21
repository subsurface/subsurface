#ifndef QTHELPERFROMC_H
#define QTHELPERFROMC_H

bool getProxyString(char **buffer);
bool canReachCloudServer();
void updateWindowTitle();
bool isCloudUrl(const char *filename);
void subsurface_mkdir(const char *dir);
char *get_file_name(const char *fileName);
void copy_image_and_overwrite(const char *cfileName, const char *path, const char *cnewName);
char *hashstring(char *filename);
bool picture_exists(struct picture *picture);
const char *local_file_path(struct picture *picture);
void savePictureLocal(struct picture *picture, const char *data, int len);

#endif // QTHELPERFROMC_H
