/**
 ******************************************************************************
 * @file       pios_dio.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DIO Digital IO subsystem
 * @{
 *
 * @brief Provides an interface to configure and efficiently use GPIOs.
 *
 * @details DIO holds a GPIO specifier (bank, pin) in a pointer-width integer.
 * Inline functions in this module provide an efficient interface by which to
 * set/reset/toggle/read the GPIO.
 *
 * Partially inspired by the iotag subsystem in CleanFlight/BetaFlight.
 *
 * To use this module, first store a GPIO into a dio_tag_t -- e.g.
 *
 * dio_tag_t t = GPIOA_DIO(4);
 *
 * for pin A4 on STM32.   Then use the below functions to access it.
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef _PIOS_DIO_H
#define _PIOS_DIO_H

#ifdef STM32F10X_MD
#error no STM32F1 support yet
#endif

#ifdef STM32F4xx
enum dio_drive_strength {
	DIO_DRIVE_WEAK = 0,
	DIO_DRIVE_LIGHT,
	DIO_DRIVE_MEDIUM,
	DIO_DRIVE_STRONG
};
#else
enum dio_drive_strength {
	DIO_DRIVE_WEAK = 0,
	DIO_DRIVE_LIGHT = 0,
	DIO_DRIVE_MEDIUM = 1,
	DIO_DRIVE_STRONG = 3
};
#endif

enum dio_pin_function {
	DIO_PIN_INPUT = 0,
	DIO_PIN_OUTPUT = 1,
	DIO_PIN_ALTFUNC = 2,
	DIO_PIN_ANALOG = 3
};

enum dio_pull {
	DIO_PULL_NONE = 0,
	DIO_PULL_UP,
	DIO_PULL_DOWN
};

typedef uintptr_t dio_tag_t;

/**
 * @brief Configures a GPIO as alternate function.
 * Note that to configure drive strength, pullups, etc, DIO_output or
 * DIO_input should be called first.
 *
 * @param[in] t The GPIO specifier tag (created with e.g. GPIOA_DIO(3) )
 * @param[in] alt_func The alternate function specifier.
 */
static inline void DIO_altfunc(dio_tag_t t, int alt_func);

/**
 * @brief Configures a GPIO as analog.  Disables pullups/pulldowns, etc.
 *
 * @param[in] t The GPIO specifier tag (created with e.g. GPIOA_DIO(3) )
 */
static inline void DIO_analog(dio_tag_t t);

/**
 * @brief Configures a GPIO as an output.
 * @param[in] t The GPIO specifier tag (created with e.g. GPIOA_DIO(3) )
 * @param[in] open_collector true for open-collector + pull-up semantics
 * @param[in] dio_drive_strength The drive strength to use for the GPIO
 * @param[in] first_value Whether it should be initially driven high or low.
 */
static inline void DIO_output(dio_tag_t t, bool open_collector,
		enum dio_drive_strength strength, bool first_value);

/**
 * @brief Toggles an output GPIO to the opposite level.
 * @param[in] t The GPIO specifier tag
 */
static inline void DIO_toggle(dio_tag_t t);

/**
 * @brief Sets an output GPIO high.
 * @param[in] t The GPIO specifier tag
 */
static inline void DIO_high(dio_tag_t t);

/**
 * @brief Sets an output GPIO low.
 * @param[in] t The GPIO specifier tag
 */
static inline void DIO_low(dio_tag_t t);

/**
 * @brief Sets an output GPIO to a chosen logical level.
 * @param[in] t The GPIO specifier tag
 * @param[in] high Whether the GPIO should be high.
 */
static inline void DIO_set(dio_tag_t t, bool high);

/**
 * @brief Configures a GPIO as an input.
 * @param[in] t The GPIO specifier tag (created with e.g. GPIOA_DIO(3) )
 * @param[in] dio_pull Pullup/Pulldown configuration.
 */
static inline void DIO_input(dio_tag_t t, enum dio_pull pull);

/**
 * @brief Reads a GPIO logical value.
 * @param[in] t The GPIO specifier tag 
 * @retval True if the GPIO is high; otherwise low.
 */
static inline bool DIO_read(dio_tag_t t);

/* Implementation crud below here */

#ifdef STM32F10X_MD
#error no STM32F1 support yet
#endif

#define DIO_BASE ( (uintptr_t) GPIOA )

#define DIO_PORT_OFFSET(port) (((uintptr_t) (port)) - DIO_BASE)

#define DIO_MAKE_TAG(port, pin) ((uintptr_t) ((DIO_PORT_OFFSET(port) << 16) | ((uint16_t) (pin))))

#define GPIOA_DIO(pin_num) DIO_MAKE_TAG(GPIOA, 1<<(pin_num)) 
#define GPIOB_DIO(pin_num) DIO_MAKE_TAG(GPIOB, 1<<(pin_num)) 
#define GPIOC_DIO(pin_num) DIO_MAKE_TAG(GPIOC, 1<<(pin_num)) 
#define GPIOD_DIO(pin_num) DIO_MAKE_TAG(GPIOD, 1<<(pin_num)) 
#define GPIOE_DIO(pin_num) DIO_MAKE_TAG(GPIOE, 1<<(pin_num))
#define GPIOF_DIO(pin_num) DIO_MAKE_TAG(GPIOF, 1<<(pin_num))

#define GET_DIO_PORT(dio) ( (GPIO_TypeDef *) (((dio) >> 16) + DIO_BASE) )

#define GET_DIO_PIN(dio)  ((dio) & 0xffff)

#define _DIO_PRELUDE \
	GPIO_TypeDef * gp = GET_DIO_PORT(t); \
	uint16_t pin = GET_DIO_PIN(t);

static inline void DIO_high(dio_tag_t t)
{
	_DIO_PRELUDE;

#ifdef STM32F4XX
	gp->BSRRL = pin;
#else
	gp->BSRR = pin;
#endif
}

static inline void DIO_low(dio_tag_t t)
{
	_DIO_PRELUDE;

#ifdef STM32F4XX
	gp->BSRRH = pin;
#else
	gp->BSRR = pin << 16;
#endif
}

static inline void DIO_set(dio_tag_t t, bool high)
{
	if (high) {
		DIO_high(t);
	} else {
		DIO_low(t);
	}
}

static inline void DIO_toggle(dio_tag_t t)
{
	_DIO_PRELUDE;

	DIO_set(t, ! (gp->ODR & pin) );
}

/* Some of the bitfields here are two bits wide; some are 1 bit wide */

// Uses regs twice, naughty.
#define DIO_SETREGS_FOURWIDE(reglow, reghigh, idx, val) \
	do { \
		int __pos = (idx); \
		if (__pos >= 8) { \
			__pos -= 8; \
			(reghigh) = ((reghigh) & ~(15 << (__pos * 4))) | ( (val) << (__pos * 4)); \
		} else { \
			(reglow) = ((reglow) & ~(15 << (__pos * 4))) | ( (val) << (__pos * 4)); \
		} \
	} while (0)

#define DIO_SETREG_TWOWIDE(reg, idx, val) \
	do { \
		int __pos = (idx); \
		(reg) = ((reg) & ~(3 << (__pos * 2))) | ( (val) << (__pos * 2)); \
	} while (0)

#define DIO_SETREG_ONEWIDE(reg, idx, val) \
	do { \
		int __pos = (idx); \
		(reg) = ((reg) & ~(1 << (__pos))) | ( (val) << __pos); \
	} while (0)

static inline void DIO_altfunc(dio_tag_t t, int alt_func)
{
	_DIO_PRELUDE;

	uint8_t pos = __builtin_ctz(pin);

	DIO_SETREGS_FOURWIDE(gp->AFR[0], gp->AFR[1], pos, alt_func);
	DIO_SETREG_TWOWIDE(gp->MODER, pos, DIO_PIN_ALTFUNC);
}

static inline void DIO_analog(dio_tag_t t)
{
	_DIO_PRELUDE;

	uint8_t pos = __builtin_ctz(pin);

	DIO_SETREG_TWOWIDE(gp->PUPDR, pos, DIO_PULL_NONE);

	DIO_SETREG_TWOWIDE(gp->MODER, pos, DIO_PIN_ANALOG);
}

static inline void DIO_output(dio_tag_t t, bool open_collector,
		enum dio_drive_strength strength, bool first_value)
{
	_DIO_PRELUDE;

	uint8_t pos = __builtin_ctz(pin);

	DIO_set(t, first_value);

	/* F0/F3/F4 are like this */
	if (open_collector) {
		/* Request pullup */
		DIO_SETREG_TWOWIDE(gp->PUPDR, pos, DIO_PULL_UP);
		/* Set as open drain output type */
		DIO_SETREG_ONEWIDE(gp->OTYPER, pos, 1);
	} else {
		/* Request no pullup / pulldown */
		DIO_SETREG_TWOWIDE(gp->PUPDR, pos, DIO_PULL_NONE);

		/* Set as normal output */
		DIO_SETREG_ONEWIDE(gp->OTYPER, pos, 0);
	}

	DIO_SETREG_TWOWIDE(gp->OSPEEDR, pos, strength);

	/* Set appropriate position to output */
	DIO_SETREG_TWOWIDE(gp->MODER, pos, DIO_PIN_OUTPUT);
}

static inline void DIO_input(dio_tag_t t, enum dio_pull pull)
{
	_DIO_PRELUDE;

	uint8_t pos = __builtin_ctz(pin);

	/* Set caller-specified pullup/pulldown */
	DIO_SETREG_TWOWIDE(gp->PUPDR, pos, pull);

	/* Set appropriate position to input */
	DIO_SETREG_TWOWIDE(gp->MODER, pos, DIO_PIN_INPUT);
}

static inline bool DIO_read(dio_tag_t t)
{
	_DIO_PRELUDE;

	return !! (gp->IDR & pin);
}

#endif /* _PIOS_DIO_H */

/**
 * @}
 * @}
 */
