#ifndef _BF_PLATFORM_H
#define _BF_PLATFORM_H

#include <pios.h>
#include <pios_thread.h>

#ifdef PIOS_INCLUDE_4WAY

#include <openpilot.h>

#include <pios_inlinedelay.h>
#include <pios_dio.h>

#include <mixersettings.h>

#define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define MAX_SUPPORTED_MOTORS 10

typedef dio_tag_t IO_t;

typedef struct serialPort_s serialPort_t;
typedef int ioConfig_t;

// NONE initializer for IO_t variable
#define IO_NONE ((IO_t)0)

struct serialPort_s {
	uintptr_t com;
};

typedef struct {
	bool enabled;
	IO_t io;
} pwmOutputPort_t;

#define IOCFG_IPU 0
#define IOCFG_OUT_PP 1
#define IOCFG_AF_PP 2

/* We'll implement the IO and PWM calls last, as they are not needed to
 * do confirmation of "hello world" serial protocol signalling.
 */
static inline bool IORead(IO_t io)
{
	return dio_read(io);
}

static inline void IOLo(IO_t io)
{
	dio_low(io);
}

static inline void IOHi(IO_t io)
{
	dio_high(io);
}

static inline void IOConfigGPIO(IO_t io, ioConfig_t cfg)
{
	if (cfg) {
		dio_set_output(io, false, DIO_DRIVE_MEDIUM, true);
	} else {
		dio_set_input(io, DIO_PULL_UP);
	}
}

// Handled by the layer above-- the UAVO MSP Bridge-- in our code
static inline void pwmDisableMotors(void)
{
}

static inline void pwmEnableMotors(void)
{
}

static inline bool should_be_enabled(int type)
{
	switch (type) {
		case MIXERSETTINGS_MIXER1TYPE_DISABLED:
		case MIXERSETTINGS_MIXER1TYPE_MOTOR:
			return true;
		default:
			return false;
	}
}

/* Here be dragons */
#define set_disabled_token_paste(b) \
	do { \
		int saved_idx = (b) - 1; \
		if (saved_idx < MAX_SUPPORTED_MOTORS) { \
			pwm_val[saved_idx].enabled = pwm_val[saved_idx].enabled && \
				should_be_enabled(mixerSettings.Mixer ## b ## Type); \
		} \
	} while (0)

static inline pwmOutputPort_t *pwmGetMotors(void)
{
	static pwmOutputPort_t pwm_val[MAX_SUPPORTED_MOTORS];
	dio_tag_t dios_tmp[MAX_SUPPORTED_MOTORS];
	int num_mot = PIOS_Servo_GetPins(dios_tmp, MAX_SUPPORTED_MOTORS);

	MixerSettingsData mixerSettings;
	MixerSettingsGet(&mixerSettings);

	int i;

	for (i = 0; i < num_mot; i++) {
		pwm_val[i].enabled = true;
		pwm_val[i].io = dios_tmp[i];
	}

	for (; i < MAX_SUPPORTED_MOTORS; i++) {
		pwm_val[i].enabled = false;
		pwm_val[i].io = IO_NONE;
	}

	set_disabled_token_paste(1);
	set_disabled_token_paste(2);
	set_disabled_token_paste(3);
	set_disabled_token_paste(4);
	set_disabled_token_paste(5);
	set_disabled_token_paste(6);
	set_disabled_token_paste(7);
	set_disabled_token_paste(8);
	set_disabled_token_paste(9);
	set_disabled_token_paste(10);

	return pwm_val;
}

/* Trivial wrappers on top of current delay mechanisms */
static inline void delayMicroseconds(uint32_t us)
{
	PIOS_WDG_Clear();
	PIOS_DELAY_WaituS(us);
	PIOS_WDG_Clear();
}

static inline uint32_t micros(void)
{
	return PIOS_DELAY_GetuS();
}

static inline uint32_t millis(void)
{
	/* Can't use the thread time because interrupts disabled */
	return micros() / 1000;
}

/* Need these first */
static inline uint8_t serialRead(serialPort_t *instance)
{
	uint8_t c;

	while (PIOS_COM_ReceiveBuffer(instance->com, &c, 1, 3000) < 1);

	return c;
}

static inline void serialWrite(serialPort_t *instance, uint8_t ch)
{
	PIOS_COM_SendBuffer(instance->com, &ch, sizeof(ch));
}

/* These two are just used for blocking, so we make the above calls block */
static inline uint32_t serialRxBytesWaiting(const serialPort_t *instance)
{
	return 1;
}

static inline uint32_t serialTxBytesFree(const serialPort_t *instance)
{
	return 1;
}

/* These are used to cork/uncork VCP packets.  We don't have this infrastructure
 * (and really it isn't a win) -- omitting... */
static inline void serialBeginWrite(serialPort_t *instance)
{
}

static inline void serialEndWrite(serialPort_t *instance)
{
}

#endif /* PIOS_INCLUDE_4WAY */

#endif /* _BF_PLATFORM_H */
