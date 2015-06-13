#ifndef QTHELPERFROMC_H
#define QTHELPERFROMC_H

bool getProxyString(char **buffer);
bool canReachCloudServer();
void updateWindowTitle();
bool isCloudUrl(const char *filename);

#endif // QTHELPERFROMC_H
