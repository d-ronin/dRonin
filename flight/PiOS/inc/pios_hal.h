#ifndef PIOS_HAL_H
#define PIOS_HAL_H

#include <uavobjectmanager.h> /* XXX TODO */
#include <hwshared.h>

#ifdef PIOS_INCLUDE_USART
#include <pios_usart_priv.h>
#include <pios_sbus_priv.h>
#include <pios_dsm_priv.h>
#include <pios_ppm_priv.h>
#include <pios_pwm_priv.h>
#endif

#ifdef PIOS_INCLUDE_I2C
#include <pios_i2c_priv.h>
#else
struct pios_i2c_adapter_cfg;
#endif

#include <pios_usb_cdc_priv.h>
#include <pios_usb_hid_priv.h>

#if defined(PIOS_INCLUDE_OPENLRS)
#include <pios_openlrs_priv.h>
#endif /* PIOS_INCLUDE_OPENLRS */

struct pios_dsm_cfg;
struct pios_sbus_cfg;
struct pios_usb_cdc_cfg;
struct pios_usb_hid_cfg;
struct pios_usart_cfg;
struct pios_usart_params;
struct pios_i2c_adapter_cfg;
struct pios_ppm_cfg;
struct pios_pwm_cfg;

/**
 * @ brief LED panic codes for hardware failure
 */
enum pios_hal_panic {
	// start at 2, so that panic codes are obvious to user
	// PLEASE ENSURE USER DOCUMENTATION IS UPDATED IF MODIFYING THIS ENUM!!
	// avoid changing the numbers if possible as it will break docs for old releases
	PIOS_HAL_PANIC_IMU       = 2,
	PIOS_HAL_PANIC_MAG       = 3,
	PIOS_HAL_PANIC_BARO      = 4,
	PIOS_HAL_PANIC_FLASH     = 5,
	PIOS_HAL_PANIC_FILESYS   = 6,
	PIOS_HAL_PANIC_I2C_INT   = 7,
	PIOS_HAL_PANIC_I2C_EXT   = 8,
	PIOS_HAL_PANIC_SPI       = 9,
	PIOS_HAL_PANIC_CAN       = 10,
	PIOS_HAL_PANIC_ADC       = 11,
	PIOS_HAL_PANIC_OSD       = 12,
};

/* One slot per selectable receiver group.
 *  eg. PWM, PPM, GCS, SPEKTRUM1, SPEKTRUM2, SBUS
 * NOTE: No slot in this map for NONE.
 */
extern uintptr_t pios_rcvr_group_map[];

#if defined(PIOS_INCLUDE_SBUS) || defined(PIOS_INCLUDE_DSM) || defined(PIOS_INCLUDE_HOTT) || defined(PIOS_INCLUDE_GPS) || defined(PIOS_INCLUDE_OPENLRS) || defined(PIOS_INCLUDE_USB_CDC) || defined(PIOS_INCLUDE_USB_HID) || defined(PIOS_INCLUDE_MAVLINK)

#ifndef PIOS_INCLUDE_COM
#error Options defined that require PIOS_INCLUDE_COM!
#endif

#if !defined(PIOS_INCLUDE_USART) && !defined(FLIGHT_POSIX)
#error Options defined that require PIOS_INCLUDE_USART!
#endif

#endif

void PIOS_HAL_CriticalError(uint32_t led_id, enum pios_hal_panic code);

#ifdef PIOS_INCLUDE_USART
/* USART here is used as a proxy for hardware-ish capabilities... Hacky */

void PIOS_HAL_ConfigureCom(const struct pios_usart_cfg *usart_port_cfg, struct pios_usart_params *usart_port_params,
		size_t rx_buf_len, size_t tx_buf_len,
		const struct pios_com_driver *com_driver, uintptr_t *com_id);

void PIOS_HAL_ConfigurePort(HwSharedPortTypesOptions port_type,
		const struct pios_usart_cfg *usart_port_cfg,
		const struct pios_com_driver *com_driver,
		pios_i2c_t *i2c_id,
		const struct pios_i2c_adapter_cfg *i2c_cfg,
		const struct pios_ppm_cfg *ppm_cfg,
		const struct pios_pwm_cfg *pwm_cfg,
		uint32_t led_id,
		/* TODO: future work to factor most of these away */
		const struct pios_dsm_cfg *dsm_cfg,
		HwSharedDSMxModeOptions dsm_mode,
		const struct pios_sbus_cfg *sbus_cfg);
#endif

#ifdef PIOS_INCLUDE_I2C
int PIOS_HAL_ConfigureExternalBaro(HwSharedExtBaroOptions baro,
		pios_i2c_t *i2c_id,
		const struct pios_i2c_adapter_cfg *i2c_cfg);

int PIOS_HAL_ConfigureExternalMag(HwSharedMagOptions mag,
		HwSharedMagOrientationOptions orientation,
		pios_i2c_t *i2c_id,
		const struct pios_i2c_adapter_cfg *i2c_cfg);
#endif

void PIOS_HAL_ConfigureCDC(HwSharedUSB_VCPPortOptions port_type,
		uintptr_t usb_id,
		const struct pios_usb_cdc_cfg *cdc_cfg);

void PIOS_HAL_ConfigureHID(HwSharedUSB_HIDPortOptions port_type,
		uintptr_t usb_id,
		const struct pios_usb_hid_cfg *hid_cfg);

#if defined(PIOS_INCLUDE_OPENLRS)
#include <pios_openlrs.h>

void PIOS_HAL_ConfigureRFM22B(
		pios_spi_t spi_dev,
		uint8_t board_type, uint8_t board_rev,
		HwSharedRfBandOptions rf_band,
		const struct pios_openlrs_cfg *openlrs_cfg,
		pios_openlrs_t *handle);
#endif /* PIOS_INCLUDE_RFM22B */

void PIOS_HAL_ConfigureSerialSpeed(uintptr_t com_id,
		                HwSharedSpeedBpsOptions speed);

void PIOS_HAL_SetReceiver(int receiver_type, uintptr_t value);
uintptr_t PIOS_HAL_GetReceiver(int receiver_type);

#ifdef PIOS_INCLUDE_DAC
#include <pios_fskdac.h>
#include <pios_annuncdac.h>

int PIOS_HAL_ConfigureDAC(dac_dev_t dac);
#endif

#endif
