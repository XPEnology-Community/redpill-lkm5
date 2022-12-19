/*
 * This file contains structures/types used for compatibility with the mfg bios
 *
 * These are not original types used by the mfg BIOS, but rather recreations from available documentation unless
 * available in GPLed source (in which case they're #include'd)
 */
#ifndef REDPILL_SYNOBIOS_COMPAT_H
#define REDPILL_SYNOBIOS_COMPAT_H

#include <linux/synobios.h> //SYNO_* & SYNO* types

//Missing HWMON types... only in some toolchains
#ifndef HWMON_PSU_SENSOR_TEMP1
#define HWMON_PSU_SENSOR_TEMP1 "temperature_1"
#endif
#ifndef HWMON_PSU_SENSOR_TEMP2
#define HWMON_PSU_SENSOR_TEMP2 "temperature_2"
#endif
#ifndef HWMON_PSU_SENSOR_TEMP3
#define HWMON_PSU_SENSOR_TEMP3 "temperature_3"
#endif
#ifndef HWMON_PSU_SENSOR_FAN_VOLT
#define HWMON_PSU_SENSOR_FAN_VOLT "fan_voltage"
#endif

//These aren't defined in synobios.h (while sensors from other categories are... go figure)
#define HWMON_SYS_TZONE_REMOTE1_NAME  "Remote1"
#define HWMON_SYS_TZONE_REMOTE2_NAME  "Remote2"
#define HWMON_SYS_TZONE_LOCAL_NAME    "Local"
#define HWMON_SYS_TZONE_SYSTEM_NAME   "system"
#define HWMON_SYS_TZONE_ADT1_LOC_NAME "ADT1 Local"
#define HWMON_SYS_TZONE_ADT2_LOC_NAME "ADT2 Local"
#define HWMON_SYS_VSENS_VCC_NAME      "VCC"
#define HWMON_SYS_VSENS_VPP_NAME      "VPP"
#define HWMON_SYS_VSENS_V33_NAME      "V33"
#define HWMON_SYS_VSENS_V5_NAME       "V5"
#define HWMON_SYS_VSENS_V12_NAME      "V12"
#define HWMON_SYS_VSENS_ADT1_V33_NAME "ADT1 V33"
#define HWMON_SYS_VSENS_ADT2_V33_NAME "ADT2 V33"
#define HWMON_SYS_CURR_ADC_NAME       "ADC"

struct MfgCompatTime {
    unsigned char second;
    unsigned char minute;
    unsigned char hours;
    unsigned char wkday;
    unsigned char day;
    unsigned char month;
    unsigned char year;
};

enum MfgCompatFanStatus {
    MFGC_FAN_UNKNOWN = -1,
    MFGC_FAN_STOPPED =  0,
    MFGC_FAN_RUNNING =  1,
};

enum MfgCompatFanSpeed {
    MFGC_FAN_SPD_STOP1,
    MFGC_FAN_SPD_STOP2,

    //Fan speeds in 1-8 scale
    MFGC_FAN_SPD_1,
    MFGC_FAN_SPD_2,
    MFGC_FAN_SPD_3,
    MFGC_FAN_SPD_4,
    MFGC_FAN_SPD_5,
    MFGC_FAN_SPD_6,
    MFGC_FAN_SPD_7,
    MFGC_FAN_SPD_8,

    //Fan speeds in 1-18 scale, used for testing only
    MFGC_FAN_SPD_TST_1,
    MFGC_FAN_SPD_TST_2,
    MFGC_FAN_SPD_TST_3,
    MFGC_FAN_SPD_TST_4,
    MFGC_FAN_SPD_TST_5,
    MFGC_FAN_SPD_TST_6,
    MFGC_FAN_SPD_TST_7,
    MFGC_FAN_SPD_TST_8,
    MFGC_FAN_SPD_TST_9,
    MFGC_FAN_SPD_TST_10,
    MFGC_FAN_SPD_TST_11,
    MFGC_FAN_SPD_TST_12,
    MFGC_FAN_SPD_TST_13,
    MFGC_FAN_SPD_TST_14,
    MFGC_FAN_SPD_TST_15,
    MFGC_FAN_SPD_TST_16,
    MFGC_FAN_SPD_TST_17,
    MFGC_FAN_SPD_TST_18,

    MFGC_FAN_SPD_PWM = 1000
};

struct MfgCompatHddLedStatus {
    int hdd_no;
    SYNO_DISK_LED state;
    int pos_name_len; //includes null terminator
    char *pos_name;
};

enum MfgCompatGenericLedState {
    MFGC_LED_OFF,
    MFGC_LED_LIT,
    MFGC_LED_BLINK,
};

struct MfgCompatMemoryByte {
    unsigned char offset;
    unsigned char value;
};

struct MfgCompatMemoryUInt {
    unsigned char address;
    unsigned char value;
};

struct MfgCompatCPLDReg {
    unsigned char hddLedCtrl;
    unsigned char hddPwrState;
    unsigned char hwModelNum;
    unsigned char fanState;
};

struct MfgCompatGPIOPin {
    int num;
    int val;
};

struct MfgCompatRtcEvent {
    unsigned char minutes; //BCD format
    unsigned char hours; //BCD format
    unsigned char weekdays;  //7 bit field, Sun => Sat
};

struct MfgCompatAutoPwrOn {
    int num;
    bool enabled;
    struct MfgCompatRtcEvent events[100];
};

struct MfgCompatPowerStatus {
    bool primary_ok;
    bool secondary_ok;
};

enum MfgCompatBackplaneStatus {
    MFGC_BKPLANE_UNK = -1,
    MFGC_BKPLANE_ERR = 0,
    MFGC_BKPLANE_OK = 1,
};

struct MfgCompatPWMState {
    int channel;
    int freq_hz;
    int duty_cycle;
    int rpm;
};

struct MfgCompatSuperIOMem {
    unsigned char ldn;
    unsigned char reg;
    unsigned char val;
};

struct MfgCompatBusPacket {
    long num;
    long len;
    char msg[128];
};

struct MfgCompatCPUState {
    unsigned int cpu;
    char clock[16];
#if defined(CONFIG_SYNO_GRANTLEY) || defined(CONFIG_SYNO_PURLEY)
    unsigned int core[CONFIG_SYNO_MULTI_CPU_NUM];
#endif
};

enum MfgCompatCopyBtnState {
    MFGC_BTN_DOWN = 0, //aka pressed
    MFGC_BTN_UP = 1, //aka not pressed
};

typedef enum {
    POWER_STATUS_BAD = 0,
    POWER_STATUS_GOOD,
} SYNO_POWER_STATUS;

typedef struct _tag_POWER_INFO {
    SYNO_POWER_STATUS power_1;
    SYNO_POWER_STATUS power_2;
} POWER_INFO;

typedef int (*mfgc_void_cb)(void); //int f(void)
typedef int (*mfgc_time_cb)(struct MfgCompatTime *); //int f(MfgCompatTime *)
typedef int (*mfgc_get_fan_state_cb)(int, enum MfgCompatFanStatus *); //int f(int, MfgCompatFanStatus *)
typedef int (*mfgc_set_fan_state_cb)(enum MfgCompatFanStatus, enum MfgCompatFanSpeed); //int f(MfgCompatFanStatus, MfgCompatFanSpeed)
typedef int (*mfgc_hwmon_sensor_cb)(SYNO_HWMON_SENSOR_TYPE *); //int f(SYNO_HWMON_SENSOR_TYPE *)
//TODO: this list is not complete - add all callback types

#ifdef CONFIG_SYNO_PORT_MAPPING_V2
typedef int (*mfgc_set_hdd_led_cb)(struct MfgCompatHddLedStatus *status);
#else
typedef int (*mfgc_set_hdd_led_cb)(int, SYNO_DISK_LED state); //int f(void)
#endif

//List of known indexes in the mfgBIOS vtable. The table can be recovered by shim/bios_shim.c. Some of its entries are
// replaced by shim/bios/bios_shims_collection.c
//The following indexes were determined based on
// - Jun's module code
// - Looking at the symbols when BIOS is loaded
// - Observing logs from mfgBIOS
#define VTK_STRUCT_OWNER         0 //you shouldn't really modify this
#define VTK_GET_BRAND            1 //Sig: int f(void)
#define VTK_GET_MODEL            2 //Sig: int f(void)
#define VTK_GET_CPLD_VER         3 //Sig: int f(void)
#define VTK_RTC_GET_TIME         4 //Sig: int f(MfgCompatTime *)
#define VTK_RTC_SET_TIME         5 //Sig: int f(MfgCompatTime *)
#define VTK_GET_FAN_STATE        6 //Sig: int f(int, MfgCompatFanStatus *) | present in: DS918+; not: DS3615xs
#define VTK_SET_FAN_STATE        7 //Sig: int f(MfgCompatFanStatus, MfgCompatFanSpeed)
#define VTK_GET_SYS_TEMP         8 //Sig: int f(SYNO_THERMAL_TEMP *) | present in: DS3615xs; not: DS918+
#define VTK_GET_CPU_TEMP         9 //Sig: int f(SYNOCPUTEMP *)
#define VTK_SET_DISK_LED        10 //Sig: varies, see mfgc_set_hdd_led_cb type
#define VTK_SET_PWR_LED         11 //Sig: int f(MfgCompatGenericLedState)
#define VTK_GET_CPLD_REG        12 //Sig: int f(MfgCompatCPLDReg *)
#define VTK_SET_PMU_MEM_BYTE    13 //Sig: int f(MfgCompatMemoryByte *)
#define VTK_GET_PMU_MEM_BYTE    14 //Sig: int f(MfgCompatMemoryByte *)
#define VTK_SET_GPIO_PIN        15 //Sig: int f(MfgCompatGPIOPin *)
#define VTK_GET_GPIO_PIN        16 //Sig: int f(MfgCompatGPIOPin *)
#define VTK_SET_GPIO_PIN_BLINK  17 //Sig: int f(MfgCompatGPIOPin *)
#define VTK_RTC_SET_APWR        18 //Sig: int f(MfgCompatAutoPwrOn *) | set auto power on
#define VTK_RTC_GET_APWR        19 //Sig: int f(MfgCompatAutoPwrOn *) | get auto power on
#define VTK_RTC_INT_APWR        20 //Sig: int f(void) | initialize auto power on. present in: DS918+; not: DS3615xs
#define VTK_RTC_UINT_APWR       21 //Sig: int f(void) | uninitialize auto power on. present in: DS918+; not: DS3615xs
#define VTK_SET_ALR_LED         22 //Sig: int f(MfgCompatGenericLedState) | alarm led
#define VTK_GET_BUZ_CLR         23 //Sig: int f(unsigned char *)
#define VTK_SET_BUZ_CLR         24 //Sig: int f(unsigned char)
#define VTK_GET_PWR_STATUS      25 //Sig: int f(MfgCompatPowerStatus *)
#define VTK_GET_BKPLANE_STATUS  26 //Sig: int f(MfgCompatBackplaneStatus *) | backplane status
#define VTK_INT_MOD_TPE         27
#define VTK_UNINIT              28 //Sig: int f(void)
#define VTK_SET_CPU_FAN_STATUS  29 //Sig: int f(MfgCompatFanStatus, MfgCompatFanSpeed)
#define VTK_SET_PHY_LED         30 //Sig: int f(MfgCompatGenericLedState) | present in: DS620; not: DS3615xs, DS918+
#define VTK_SET_HDD_ACT_LED     31 //Sig: int f(MfgCompatGenericLedState)
#define VTK_SET_PWM             32 //Sig: int f(MfgCompatPWMState *)
#define VTK_GET_MICROP_ID       33
#define VTK_SET_MICROP_ID       34 //Sig: int f(void)
#define VTK_GET_SIO_MEM         35 //Sig: int f(MfgCompatSuperIOMem *)
#define VTK_SET_SIO_MEM         36 //Sig: int f(MfgCompatSuperIOMem *)
#define VTK_SEND_LCD_PKT        37 //Sig: int f(MfgCompatBusPacket *)
#define VTK_GET_MEM_UINT        38 //Sig: int f(MfgCompatMemoryUInt *)
#define VTK_SET_MEM_UINT        39 //Sig: int f(MfgCompatMemoryUInt *)
#define VTK_GET_CPU_INF         40 //Sig: void f(MfgCompatCPUState*, uint)
#define VTK_SET_HA_LED          41 //present in: RC18015xs+ (and other HA units? don't have other); not: DS3615xs,DS918+
#define VTK_GET_CPY_BTN         42 //Sig: MfgCompatCopyBtnState f(void) | present in: DS718+

#if RP_MODULE_TARGET_VER == 6
#define VTK_SET_SAFE_REMOVE_LED       43 //Sig: int f(bool) | present in: DS3615xs; not: DS918+
#define VTK_GET_SYS_CURRENT           44 //Sig: int f(SYNO_DISK_INTF_INFO *)
#define VTK_GET_HWMON_FAN_RPM         45 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_PSU_STATUS      46 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_VOLTAGE         47 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_HDD_BKPLANE     48 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_THERMAL         49 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_CURRENT         50 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)

#elif RP_MODULE_TARGET_VER == 7 //moved VTK_SET_SAFE_REMOVE_LED and VTK_GET_SYS_CURRENT below HWMON_ stuff (sic!)
#define VTK_GET_HWMON_FAN_RPM         43 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_PSU_STATUS      44 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_VOLTAGE         45 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_HDD_BKPLANE     46 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_THERMAL         47 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_GET_HWMON_CURRENT         48 //Sig: int f(SYNO_HWMON_SENSOR_TYPE *)
#define VTK_SET_SAFE_REMOVE_LED       49 //Sig: int f(bool) | present in: DS3615xs; not: DS918+
#define VTK_GET_SYS_CURRENT           50 //Sig: int f(SYNO_DISK_INTF_INFO *)
#endif //RP_MODULE_TARGET_VER

#define VTK_GET_HDD_IFACE       51

#define VTK_SIZE 52

#endif //REDPILL_SYNOBIOS_COMPAT_H