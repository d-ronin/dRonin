#ifndef _BF_PLATFORM_H
#define _BF_PLATFORM_H

#include <pios.h>
#include <pios_thread.h>

#ifdef PIOS_INCLUDE_4WAY
#define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define MAX_SUPPORTED_MOTORS 8

// XXX
typedef void *IO_t;

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

// XXX
#define IOCFG_IPU 0
#define IOCFG_OUT_PP 1
#define IOCFG_AF_PP 2

/* We'll implement the IO and PWM calls last, as they are not needed to
 * do confirmation of "hello world" serial protocol signalling.
 */
static inline bool IORead(IO_t io)
{
	return (GPIOB->IDR & GPIO_Pin_4) != 0;
}

static inline void IOLo(IO_t io)
{
	GPIOB->BSRRH = GPIO_Pin_4;
}

static inline void IOHi(IO_t io)
{
	GPIOB->BSRRL = GPIO_Pin_4;
}

static inline void IOConfigGPIO(IO_t io, ioConfig_t cfg)
{
	GPIO_InitTypeDef settings = {
		.GPIO_Pin = GPIO_Pin_4,
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd = GPIO_PuPd_UP
	};

	if (cfg) {
		settings.GPIO_Mode = GPIO_Mode_OUT;
	} else {
		settings.GPIO_Mode = GPIO_Mode_IN;
	}

	GPIO_Init(GPIOB, &settings);
}

static inline void pwmDisableMotors(void)
{
}

static inline void pwmEnableMotors(void)
{
}

static pwmOutputPort_t pwm_tmp[MAX_SUPPORTED_MOTORS] = { { .enabled = 1, .io="hi"} };

static inline pwmOutputPort_t *pwmGetMotors(void)
{
	return pwm_tmp;
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
