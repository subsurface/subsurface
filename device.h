#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

struct device_info {
	const char *model;
	uint32_t deviceid;

	const char *serial_nr;
	const char *firmware;
	const char *nickname;
	struct device_info *next;
};

extern struct device_info *get_device_info(const char *model, uint32_t deviceid);
extern struct device_info *get_different_device_info(const char *model, uint32_t deviceid);
extern struct device_info *create_device_info(const char *model, uint32_t deviceid);
extern struct device_info *remove_device_info(const char *model, uint32_t deviceid);
extern struct device_info *head_of_device_info_list(void);

#ifdef __cplusplus
}
#endif

#endif
