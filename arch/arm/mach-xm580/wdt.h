// Device registers

//Value to write to unlock the register. Write 0 to lock it.
#define WATCHDOG_REGISTER_UNLOCK (void*)0xfe160c00
//Write WATCHDOG_STATE_ON or WATCHDOG_STATE_OFF
#define WATCHDOG_REGISTER_STATE (void*)0xfe160008
//guessing that it is the timeleft register as 0 is written when WATCHDOG_IOCTL_WRITE
#define WATCHDOG_REGISTER_TIMELEFT (void*)0xfe16000c

// Device register values
#define WATCHDOG_STATE_OFF 0
#define WATCHDOG_STATE_ON 3
#define WATCHDOG_REGISTER_UNLOCK_VALUE 0x1acce551

// ioctl() values
#define WATCHDOG_IOCTL_START 0x5700
#define WATCHDOG_IOCTL_WRITE 0x5702
#define WATCHDOG_IOCTL_STOP  0x5703
#define WATCHDOG_IOCTL_SETTIMEOUT 0x40045701