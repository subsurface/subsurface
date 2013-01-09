#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

struct device_info {
	const char *model;
	uint32_t deviceid;

	const char *nickname;
	struct device_info *next;
	gboolean saved;
};

extern struct device_info *get_device_info(const char *model, uint32_t deviceid);
extern struct device_info *get_different_device_info(const char *model, uint32_t deviceid);
extern struct device_info *create_device_info(const char *model, uint32_t deviceid);
extern struct device_info *remove_device_info(const char *model, uint32_t deviceid);
extern void clear_device_saved_status(void);

#endif
