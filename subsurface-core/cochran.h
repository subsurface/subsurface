// Commander log fields
#define CMD_SEC				1
#define CMD_MIN				0
#define CMD_HOUR			3
#define CMD_DAY				2
#define CMD_MON				5
#define CMD_YEAR			4
#define CME_START_OFFSET		6		// 4 bytes
#define CMD_WATER_CONDUCTIVITY		25		// 1 byte, 0=low, 2=high
#define CMD_START_SGC			42		// 2 bytes
#define CMD_START_TEMP			45		// 1 byte, F
#define CMD_START_DEPTH			56		// 2 bytes, /4=ft
#define CMD_START_PSI			62
#define CMD_SIT				68		// 2 bytes, minutes
#define CMD_NUMBER			70		// 2 bytes
#define CMD_ALTITUDE			73		// 1 byte, /4=Kilofeet
#define CMD_END_OFFSET			128		// 4 bytes
#define CMD_MIN_TEMP			153		// 1 byte, F
#define CMD_BT				166		// 2 bytes, minutes
#define CMD_MAX_DEPTH			168		// 2 bytes, /4=ft
#define CMD_AVG_DEPTH			170		// 2 bytes, /4=ft
#define CMD_O2_PERCENT			210		// 8 bytes, 4 x 2 byte, /256=%

// EMC log fields
#define EMC_SEC				0
#define EMC_MIN				1
#define EMC_HOUR			2
#define EMC_DAY				3
#define EMC_MON				4
#define EMC_YEAR			5
#define EMC_START_OFFSET		6		// 4 bytes
#define EMC_WATER_CONDUCTIVITY		24		// 1 byte bits 0:1, 0=low, 2=high
#define EMC_START_DEPTH			42		// 2 byte, /256=ft
#define EMC_START_TEMP			55		// 1 byte, F
#define EMC_SIT				84		// 2 bytes, minutes, LE
#define EMC_NUMBER			86		// 2 bytes
#define EMC_ALTITUDE			89		// 1 byte, /4=Kilofeet
#define EMC_O2_PERCENT			144		// 20 bytes, 10 x 2 bytes, /256=%
#define EMC_HE_PERCENT			164		// 20 bytes, 10 x 2 bytes, /256=%
#define EMC_END_OFFSET			256		// 4 bytes
#define EMC_MIN_TEMP			293		// 1 byte, F
#define EMC_BT				304		// 2 bytes, minutes
#define EMC_MAX_DEPTH			306		// 2 bytes, /4=ft
#define EMC_AVG_DEPTH			310		// 2 bytes, /4=ft
