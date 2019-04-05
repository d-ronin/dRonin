/**
 ******************************************************************************
 * @file       pios_hal.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2017
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_HAL Hardware abstraction layer files
 * @{
 * @brief Code to initialize ports/devices for multiple targets
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

#include <pios.h>
#include <pios_hal.h>
#include <openpilot.h>

#include <pios_com_priv.h>
#include <pios_rcvr_priv.h>

#include <pios_modules.h>
#include <pios_sys.h>
#include <pios_thread.h>

#include <dacsettings.h>
#include <manualcontrolsettings.h>

#if defined(PIOS_INCLUDE_OPENLRS)
#include <openlrs.h>
#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
#include <pios_openlrs_rcvr_priv.h>
#endif
#endif

#if defined(PIOS_INCLUDE_UAVTALKRCVR)
#include <pios_uavtalkrcvr_priv.h>
#endif

#ifdef PIOS_INCLUDE_USART
#include <pios_usart_priv.h>
#include <pios_sbus_priv.h>
#include <pios_dsm_priv.h>
#include <pios_ppm_priv.h>
#include <pios_pwm_priv.h>
#include <pios_hsum_priv.h>

#include <pios_usb_cdc_priv.h>
#include <pios_usb_hid_priv.h>
#endif

#include <sanitycheck.h>

#ifdef STM32F0XX
uintptr_t pios_rcvr_group_map[1];
#else
uintptr_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];
#endif

#if defined(PIOS_INCLUDE_DAC_ANNUNCIATOR)
annuncdac_dev_t pios_dac_annunciator_id;
#endif

uintptr_t pios_com_gps_id;
uintptr_t pios_com_bridge_id;

#if defined(PIOS_INCLUDE_MAVLINK)
uintptr_t pios_com_mavlink_id;
#endif

#if defined(PIOS_INCLUDE_MSP_BRIDGE)
uintptr_t pios_com_msp_id;
#endif

#if defined(PIOS_INCLUDE_HOTT)
uintptr_t pios_com_hott_id;
#endif

#if defined(PIOS_INCLUDE_FRSKY_SENSOR_HUB)
uintptr_t pios_com_frsky_sensor_hub_id;
#endif

#if defined(PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY)
uintptr_t pios_com_frsky_sport_id;
#endif

#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
uintptr_t pios_com_lighttelemetry_id;
#endif

#if defined(PIOS_INCLUDE_TBSVTXCONFIG)
uintptr_t pios_com_tbsvtxconfig_id;
#endif

#if defined(PIOS_INCLUDE_USB_HID) || defined(PIOS_INCLUDE_USB_CDC)
uintptr_t pios_com_telem_usb_id;
#endif

#if defined(PIOS_INCLUDE_USB_CDC)
uintptr_t pios_com_vcp_id;
#endif

#if defined(PIOS_INCLUDE_OPENLRS)
uintptr_t pios_com_rf_id;
#endif

uintptr_t pios_com_telem_serial_id;

#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
#define PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN 40
uintptr_t pios_com_debug_id;
#endif  /* PIOS_INCLUDE_DEBUG_CONSOLE */

#ifndef PIOS_COM_TELEM_RF_RX_BUF_LEN
#define PIOS_COM_TELEM_RF_RX_BUF_LEN 512
#endif

#ifndef PIOS_COM_TELEM_RF_TX_BUF_LEN
#define PIOS_COM_TELEM_RF_TX_BUF_LEN 512
#endif

#ifndef PIOS_COM_GPS_RX_BUF_LEN
#define PIOS_COM_GPS_RX_BUF_LEN 32
#endif

#ifndef PIOS_COM_GPS_TX_BUF_LEN
#define PIOS_COM_GPS_TX_BUF_LEN 16
#endif

#ifndef PIOS_COM_TELEM_USB_RX_BUF_LEN
#define PIOS_COM_TELEM_USB_RX_BUF_LEN 129
#endif

#ifndef PIOS_COM_TELEM_USB_TX_BUF_LEN
#define PIOS_COM_TELEM_USB_TX_BUF_LEN 65
#endif

#ifndef PIOS_COM_BRIDGE_RX_BUF_LEN
#define PIOS_COM_BRIDGE_RX_BUF_LEN 65
#endif

#ifndef PIOS_COM_BRIDGE_TX_BUF_LEN
#define PIOS_COM_BRIDGE_TX_BUF_LEN 12
#endif

#ifndef PIOS_COM_MAVLINK_TX_BUF_LEN
#define PIOS_COM_MAVLINK_TX_BUF_LEN 128
#endif

#ifndef PIOS_COM_MSP_TX_BUF_LEN
#define PIOS_COM_MSP_TX_BUF_LEN 128
#endif

#ifndef PIOS_COM_MSP_RX_BUF_LEN
#define PIOS_COM_MSP_RX_BUF_LEN 65
#endif

#ifndef PIOS_COM_HOTT_RX_BUF_LEN
#define PIOS_COM_HOTT_RX_BUF_LEN 16
#endif

#ifndef PIOS_COM_HOTT_TX_BUF_LEN
#define PIOS_COM_HOTT_TX_BUF_LEN 16
#endif

#ifndef PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN
#define PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN 128
#endif

#ifndef PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN
#define PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN 22
#endif

#ifndef PIOS_COM_FRSKYSPORT_TX_BUF_LEN
#define PIOS_COM_FRSKYSPORT_TX_BUF_LEN 16
#endif

#ifndef PIOS_COM_FRSKYSPORT_RX_BUF_LEN
#define PIOS_COM_FRSKYSPORT_RX_BUF_LEN 16
#endif

#ifndef PIOS_COM_OPENLOG_TX_BUF_LEN
#define PIOS_COM_OPENLOG_TX_BUF_LEN 768
#endif

#ifndef PIOS_COM_TBSVTXCONFIG_TX_BUF_LEN
#define PIOS_COM_TBSVTXCONFIG_TX_BUF_LEN 32
#endif

#ifndef PIOS_COM_TBSVTXCONFIG_RX_BUF_LEN
#define PIOS_COM_TBSVTXCONFIG_RX_BUF_LEN 32
#endif

#ifdef PIOS_INCLUDE_HMC5883
#include "pios_hmc5883_priv.h"

static const struct pios_hmc5883_cfg external_hmc5883_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5883_ODR_75,
	.Meas_Conf           = PIOS_HMC5883_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5883_GAIN_1_9,
	.Mode                = PIOS_HMC5883_MODE_SINGLE,
	.Default_Orientation = PIOS_HMC5883_TOP_0DEG,
};
#endif

#ifdef PIOS_INCLUDE_HMC5983_I2C
#include "pios_hmc5983.h"

static const struct pios_hmc5983_cfg external_hmc5983_cfg = {
	.exti_cfg            = NULL,
	.M_ODR               = PIOS_HMC5983_ODR_75,
	.Meas_Conf           = PIOS_HMC5983_MEASCONF_NORMAL,
	.Gain                = PIOS_HMC5983_GAIN_1_9,
	.Mode                = PIOS_HMC5983_MODE_SINGLE,
	.Averaging           = PIOS_HMC5983_AVERAGING_1,
	.Orientation         = PIOS_HMC5983_TOP_0DEG,
};
#endif

#ifdef PIOS_INCLUDE_BMP280
#include "pios_bmp280_priv.h"

static const struct pios_bmp280_cfg external_bmp280_cfg = {
	.oversampling = BMP280_HIGH_RESOLUTION,
};
#endif

#ifdef PIOS_INCLUDE_MS5611
#include "pios_ms5611_priv.h"

static const struct pios_ms5611_cfg external_ms5611_cfg = {
	.oversampling             = MS5611_OSR_4096,
	.temperature_interleaving = 1,
	.use_0x76_address         = false,
};
#endif

#ifdef PIOS_INCLUDE_WS2811
#include <pios_ws2811.h>
#endif

static inline void PIOS_HAL_Err2811(bool on) {
	(void) on;
#ifdef PIOS_INCLUDE_WS2811
	if (pios_ws2811) {
		if (on) {
			PIOS_WS2811_set_all(pios_ws2811, 255, 16, 255);
		} else {
			PIOS_WS2811_set_all(pios_ws2811, 0, 32, 0);
		}

		PIOS_WS2811_trigger_update(pios_ws2811);
	}
#endif
}

/**
 * @brief Flash a blink code.
 * @param[in] led_id The LED to blink
 * @param[in] code Number of blinks to do in a row
 */
void PIOS_HAL_CriticalError(uint32_t led_id, enum pios_hal_panic code) {
#if defined(PIOS_TOLERATE_MISSING_SENSORS)
	if (code == PIOS_HAL_PANIC_MAG) {
		PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
		return;
	}

	if (code == PIOS_HAL_PANIC_BARO) {
		PIOS_SENSORS_SetMissing(PIOS_SENSOR_BARO);
		return;
	}
#endif

#if defined(PIOS_INCLUDE_ANNUNC)
	for (int cnt = 0; cnt < 3; cnt++) {
		for (int32_t i = 0; i < code; i++) {
			PIOS_WDG_Clear();
			PIOS_ANNUNC_On(led_id);
			PIOS_HAL_Err2811(true);
			PIOS_DELAY_WaitmS(175);
			PIOS_WDG_Clear();
			PIOS_HAL_Err2811(false);
			PIOS_ANNUNC_Off(led_id);
			PIOS_DELAY_WaitmS(175);
		}
		PIOS_DELAY_WaitmS(175);
		PIOS_WDG_Clear();
		PIOS_DELAY_WaitmS(175);
		PIOS_WDG_Clear();
		PIOS_DELAY_WaitmS(175);
		PIOS_WDG_Clear();
	}
#endif // PIOS_INCLUDE_ANNUNC
	PIOS_SYS_Reset();
}

/**
 * @brief Bind a device instance to a role.
 *
 * This allows us to check for duplicates and to eventually do something
 * intelligent baout them here.
 *
 * @param[out] target place dedicated for this role to store device id
 * @param[in] value handle of the device to store into this role.
 */
static void PIOS_HAL_SetTarget(uintptr_t *target, uintptr_t value) {
	if (target) {
#ifndef PIOS_NO_ALARMS
		if (*target) {
			set_config_error(SYSTEMALARMS_CONFIGERROR_DUPLICATEPORTCFG);
		}
#endif

		*target = value;
	}
}

#ifdef PIOS_INCLUDE_RCVR
/**
 * @brief Assign a device instance into the receiver map
 *
 * @param[in] receiver_type the receiver type index from MANUALCONTROL
 * @param[in] value handle of the device instance
 */
void PIOS_HAL_SetReceiver(int receiver_type, uintptr_t value) {
#ifdef STM32F0XX
	(void) receiver_type;
	PIOS_HAL_SetTarget(pios_rcvr_group_map, value);
#else
	PIOS_HAL_SetTarget(pios_rcvr_group_map + receiver_type, value);
#endif
}

/**
 * @brief Get the device instance from the receiver map
 *
 * @param[in] receiver_type the receiver type index from MANUALCONTROL
 * @return device instance handle
 */
uintptr_t PIOS_HAL_GetReceiver(int receiver_type) {
	if (receiver_type < 0) {
		return 0;
	} else if (receiver_type >= MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE) {
		return 0;
	}

#ifdef STM32F0XX
	return pios_rcvr_group_map[0];
#else
	return pios_rcvr_group_map[receiver_type];
#endif
}
#endif // PIOS_INCLUDE_RCVR

#if defined(PIOS_INCLUDE_USART) && defined(PIOS_INCLUDE_COM)
/**
 * @brief Configures USART and COM subsystems.
 *
 * @param[in] usart_port_cfg USART configuration
 * @param[in] rx_buf_len receive buffer size
 * @param[in] tx_buf_len transmit buffer size
 * @param[in] com_driver communications driver
 * @param[out] com_id id of the PIOS_Com instance
 */
void PIOS_HAL_ConfigureCom(const struct pios_usart_cfg *usart_port_cfg,
                           struct pios_usart_params *usart_port_params,
                           size_t rx_buf_len,
                           size_t tx_buf_len,
                           const struct pios_com_driver *com_driver,
                           uintptr_t *com_id)
{
	uintptr_t usart_id;
	if (PIOS_USART_Init(&usart_id, usart_port_cfg, usart_port_params)) {
		PIOS_Assert(0);
	}


	if (PIOS_COM_Init(com_id, com_driver, usart_id,
			rx_buf_len, tx_buf_len)) {
		PIOS_Assert(0);
	}
}
#endif  /* PIOS_INCLUDE_USART && PIOS_INCLUDE_COM */

#ifdef PIOS_INCLUDE_DSM
/**
 * @brief Configures a DSM receiver
 *
 * @param[in] usart_dsm_cfg Configuration for the USART for DSM mode.
 * @param[in] dsm_cfg Configuration for DSM on this target
 * @param[in] usart_com_driver The COM driver for this USART
 * @param[in] mode Mode in which to operate DSM driver; encapsulates binding
 */
static void PIOS_HAL_ConfigureDSM(const struct pios_usart_cfg *usart_dsm_cfg,
                                  struct pios_usart_params *usart_port_params,
                                  const struct pios_dsm_cfg *dsm_cfg,
                                  const struct pios_com_driver *usart_com_driver,
                                  HwSharedDSMxModeOptions mode)
{
	uintptr_t usart_dsm_id;
	if (PIOS_USART_Init(&usart_dsm_id, usart_dsm_cfg, usart_port_params)) {
		PIOS_Assert(0);
	}

	uintptr_t dsm_id;
	if (PIOS_DSM_Init(&dsm_id, dsm_cfg, usart_com_driver,
				usart_dsm_id, mode)) {
		PIOS_Assert(0);
	}

	uintptr_t dsm_rcvr_id;
	if (PIOS_RCVR_Init(&dsm_rcvr_id, &pios_dsm_rcvr_driver, dsm_id)) {
		PIOS_Assert(0);
	}
	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_DSM, dsm_rcvr_id);
}

#endif

#ifdef PIOS_INCLUDE_HSUM
/**
 * @brief Configures a HSUM receiver
 *
 * @param[in] usart_hsum_cfg Configuration for the USART for DSM mode.
 * @param[in] usart_com_driver The COM driver for this USART
 * @param[in] proto SUMH/SUMD?
 */
static void PIOS_HAL_ConfigureHSUM(const struct pios_usart_cfg *usart_hsum_cfg,
                                   struct pios_usart_params *usart_port_params,
                                   const struct pios_com_driver *usart_com_driver,
                                   enum pios_hsum_proto *proto)
{
	uintptr_t usart_hsum_id;
	if (PIOS_USART_Init(&usart_hsum_id, usart_hsum_cfg, usart_port_params)) {
		PIOS_Assert(0);
	}

	uintptr_t hsum_id;
	if (PIOS_HSUM_Init(&hsum_id, usart_com_driver,
				usart_hsum_id, *proto)) {
		PIOS_Assert(0);
	}

	uintptr_t hsum_rcvr_id;
	if (PIOS_RCVR_Init(&hsum_rcvr_id, &pios_hsum_rcvr_driver, hsum_id)) {
		PIOS_Assert(0);
	}

	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_HOTTSUM,
			hsum_rcvr_id);
}
#endif

#ifdef PIOS_INCLUDE_SRXL
/**
 * @brief Configures a Multiplex SRXL receiver
 *
 * @param[in] usart_srxl_cfg Configuration for the USART for SRXL mode.
 * @param[in] usart_com_driver The COM driver for this USART
 */
static void PIOS_HAL_ConfigureSRXL(const struct pios_usart_cfg *usart_srxl_cfg,
		struct pios_usart_params *usart_port_params,
		const struct pios_com_driver *usart_com_driver)
{
	uintptr_t usart_srxl_id;
	if (PIOS_USART_Init(&usart_srxl_id, usart_srxl_cfg, usart_port_params)) {
		PIOS_Assert(0);
	}

	uintptr_t srxl_id;
	if (PIOS_SRXL_Init(&srxl_id, usart_com_driver, usart_srxl_id)) {
		PIOS_Assert(0);
	}

	uintptr_t srxl_rcvr_id;
	if (PIOS_RCVR_Init(&srxl_rcvr_id, &pios_srxl_rcvr_driver, srxl_id)) {
		PIOS_Assert(0);
	}
	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_SRXL, srxl_rcvr_id);
}

#endif // PIOS_INCLUDE_SRXL

#ifdef PIOS_INCLUDE_IBUS
/**
 * @brief Configures an IBus receiver
 *
 * @param[in] usart_ibus_cfg Configuration for the USART for IBus
 * @param[in] usart_port_params USART port parameters
 * @param[in] usart_com_driver The COM driver for this USART
 */
static void PIOS_HAL_ConfigureIBus(const struct pios_usart_cfg *usart_ibus_cfg,
		struct pios_usart_params *usart_port_params,
		const struct pios_com_driver *usart_com_driver)
{
	uintptr_t usart_ibus_id;
	if (PIOS_USART_Init(&usart_ibus_id, usart_ibus_cfg, usart_port_params))
		PIOS_Assert(0);

	uintptr_t ibus_id;
	if (PIOS_IBus_Init(&ibus_id, usart_com_driver, usart_ibus_id))
		PIOS_Assert(0);

	uintptr_t ibus_rcvr_id;
	if (PIOS_RCVR_Init(&ibus_rcvr_id, &pios_ibus_rcvr_driver, ibus_id))
		PIOS_Assert(0);

	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_IBUS,
			ibus_rcvr_id);
}
#endif

#ifdef PIOS_INCLUDE_CROSSFIRE
/**
 * @brief Configures a TBS Crossfire receiver
 *
 * @param[in] usart_crsf_cfg Configuration for the USART for the TBS Crossfire
 * @param[in] usart_port_params USART port parameters
 * @param[in] usart_com_driver The COM driver for this USART
 */
static void PIOS_HAL_ConfigureTbsCrossfire(const struct pios_usart_cfg *usart_crsf_cfg,
		struct pios_usart_params *usart_port_params,
		const struct pios_com_driver *usart_com_driver)
{
	uintptr_t usart_crsf_id;
	if (PIOS_USART_Init(&usart_crsf_id, usart_crsf_cfg, usart_port_params))
		PIOS_Assert(0);

	uintptr_t crsf_id;
	if (PIOS_Crossfire_Init(&crsf_id, usart_com_driver, usart_crsf_id))
		PIOS_Assert(0);

	uintptr_t crsf_rcvr_id;
	if (PIOS_RCVR_Init(&crsf_rcvr_id, &pios_crossfire_rcvr_driver, crsf_id))
		PIOS_Assert(0);

	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_TBSCROSSFIRE,
			crsf_rcvr_id);
}
#endif // PIOS_INCLUDE_CROSSFIRE

/** @brief Configure a [flexi/main/rcvr/etc] port.
 *
 * Not all of these parameters will be defined for each port.  Caller may pass
 * NULL but is responsible for ensuring illegal modes also do not exist in the
 * target's UAVO definition.
 *
 * Hopefully more of these can be inferred with time and the arg list can
 * greatly decrease in size.
 *
 * @param[in] port_type protocol to speak on this port
 * @param[in] usart_port_cfg serial configuration for most modes on this port
  * @param[in] com_driver communications driver for serial on this port
 * @param[out] i2c_id ID of I2C peripheral if operated in I2C mode
 * @param[in] i2c_Cfg Adapter configuration/registers for I2C mode
 * @param[in] ppm_cfg Configuration/registers for PPM mode
 * @param[in] pwm_cfg Configuration/registers for PWM mode
 * @param[in] led_id LED to blink when there's panics
 * @param[in] dsm_cfg DSM configuration for this port
 * @param[in] dsm_mode Mode in which to operate DSM driver; encapsulates binding
 * @param[in] sbus_cfg SBUS configuration for this port
 */

#ifdef PIOS_INCLUDE_USART
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
		const struct pios_sbus_cfg *sbus_cfg)
{
	uintptr_t port_driver_id;
	uintptr_t *target = NULL, *target2 = NULL;

	struct pios_usart_params usart_port_params;

	usart_port_params.init.USART_BaudRate            = 57600;
	usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
	usart_port_params.init.USART_Parity              = USART_Parity_No;
	usart_port_params.init.USART_StopBits            = USART_StopBits_1;
	usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_port_params.init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

	usart_port_params.rx_invert   = false; // Only used by F3 targets
	usart_port_params.tx_invert   = false; // Only used by F3 targets
	usart_port_params.rxtx_swap   = false; // Only used by F3 targets
	usart_port_params.single_wire = false; // Only used by F3 targets

	// If there is a hardware inverter for this port
	if (sbus_cfg != NULL) {
		// Enable inverter gpio clock
		if (sbus_cfg->gpio_clk_func != NULL)
			(*sbus_cfg->gpio_clk_func)(sbus_cfg->gpio_clk_periph, ENABLE);

		// Initialize hardware inverter GPIO pin
		GPIO_Init(sbus_cfg->inv.gpio, (GPIO_InitTypeDef*)&sbus_cfg->inv.init);

		// Enable Inverter for Sbus
		if (port_type == HWSHARED_PORTTYPES_SBUS)
			GPIO_WriteBit(sbus_cfg->inv.gpio, sbus_cfg->inv.init.GPIO_Pin, sbus_cfg->gpio_inv_enable);
		else
			GPIO_WriteBit(sbus_cfg->inv.gpio, sbus_cfg->inv.init.GPIO_Pin, sbus_cfg->gpio_inv_disable);
	}

	switch (port_type) {

	case HWSHARED_PORTTYPES_DISABLED:
		break;

	case HWSHARED_PORTTYPES_COMBRIDGE:
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_BRIDGE_RX_BUF_LEN, PIOS_COM_BRIDGE_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_bridge_id;
		//module is enabled by the USB port config rather than here
		break;

	case HWSHARED_PORTTYPES_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, 0, PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_debug_id;
#endif  /* PIOS_INCLUDE_DEBUG_CONSOLE */
		break;

	case HWSHARED_PORTTYPES_DSM:
#if defined(PIOS_INCLUDE_DSM)
		if (dsm_cfg && usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 115200;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_No;
			usart_port_params.init.USART_StopBits            = USART_StopBits_1;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx;

			PIOS_HAL_ConfigureDSM(usart_port_cfg, &usart_port_params, dsm_cfg, com_driver, dsm_mode);
		}
#endif  /* PIOS_INCLUDE_DSM */
		break;

	case HWSHARED_PORTTYPES_FRSKYSENSORHUB:
#if defined(PIOS_INCLUDE_FRSKY_SENSOR_HUB)
		usart_port_params.init.USART_BaudRate            = 57600;
		usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
		usart_port_params.init.USART_Parity              = USART_Parity_No;
		usart_port_params.init.USART_StopBits            = USART_StopBits_1;
		usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		usart_port_params.init.USART_Mode                = USART_Mode_Tx;

		usart_port_params.rx_invert   = false;
		usart_port_params.tx_invert   = true;
		usart_port_params.single_wire = false;

		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, 0, PIOS_COM_FRSKYSENSORHUB_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_frsky_sensor_hub_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOFRSKYSENSORHUBBRIDGE);
#endif /* PIOS_INCLUDE_FRSKY_SENSOR_HUB */
		break;

	case HWSHARED_PORTTYPES_FRSKYSPORTNONINVERTED:
#if defined(PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY)
		usart_port_params.single_wire = true;

		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_FRSKYSPORT_RX_BUF_LEN, PIOS_COM_FRSKYSPORT_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_frsky_sport_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOFRSKYSPORTBRIDGE);
#endif /* PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY */
		break;

	case HWSHARED_PORTTYPES_FRSKYSPORTTELEMETRY:
#if defined(PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY)
#if defined(STM32F30X) || defined(STM32F0XX)
		// F0 / F3 has internal inverters and can switch rx/tx pins
		usart_port_params.rx_invert   = true;
		usart_port_params.tx_invert   = true;
		usart_port_params.single_wire = true;
#endif // STM32F30X

#if defined(USE_STM32F4xx_BRAINFPVRE1)
		usart_port_params.single_wire = true;
#endif // USE_STM32F4xx_BRAINFPVRE1

		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_FRSKYSPORT_RX_BUF_LEN, PIOS_COM_FRSKYSPORT_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_frsky_sport_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOFRSKYSPORTBRIDGE);
#endif /* PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY */
		break;

	case HWSHARED_PORTTYPES_GPS:
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_GPS_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_gps_id;
		PIOS_Modules_Enable(PIOS_MODULE_GPS);
		break;

	case HWSHARED_PORTTYPES_HOTTSUMD:
	case HWSHARED_PORTTYPES_HOTTSUMH:
#if defined(PIOS_INCLUDE_HSUM)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 115200;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_No;
			usart_port_params.init.USART_StopBits            = USART_StopBits_1;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx;

			enum pios_hsum_proto proto;
			switch (port_type) {
			case HWSHARED_PORTTYPES_HOTTSUMD:
				proto = PIOS_HSUM_PROTO_SUMD;
				break;
			case HWSHARED_PORTTYPES_HOTTSUMH:
				proto = PIOS_HSUM_PROTO_SUMH;
				break;
			default:
				PIOS_Assert(0);
				break;
			}
			PIOS_HAL_ConfigureHSUM(usart_port_cfg, &usart_port_params, com_driver, &proto);
		}
#endif  /* PIOS_INCLUDE_HSUM */
		break;

	case HWSHARED_PORTTYPES_HOTTTELEMETRY:
#if defined(PIOS_INCLUDE_HOTT)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_HOTT_RX_BUF_LEN, PIOS_COM_HOTT_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_hott_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOHOTTBRIDGE);
#endif /* PIOS_INCLUDE_HOTT */
		break;

	case HWSHARED_PORTTYPES_I2C:
#if defined(PIOS_INCLUDE_I2C)
		if (i2c_id && i2c_cfg) {
			if (PIOS_I2C_Init(i2c_id, i2c_cfg)) {
				PIOS_Assert(0);
				AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_CRITICAL);
			}
			if (PIOS_I2C_CheckClear(*i2c_id) != 0)
				AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_CRITICAL);

			if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) == SYSTEMALARMS_ALARM_UNINITIALISED)
				AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
		}
#endif  /* PIOS_INCLUDE_I2C */
		break;

	case HWSHARED_PORTTYPES_LIGHTTELEMETRYTX:
#if defined(PIOS_INCLUDE_LIGHTTELEMETRY)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, 0, PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_lighttelemetry_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE);
#endif
		break;

	case HWSHARED_PORTTYPES_MAVLINKTX:
#if defined(PIOS_INCLUDE_MAVLINK)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, 0, PIOS_COM_MAVLINK_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_mavlink_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOMAVLINKBRIDGE);
#endif          /* PIOS_INCLUDE_MAVLINK */
		break;

	case HWSHARED_PORTTYPES_MAVLINKTX_GPS_RX:
#if defined(PIOS_INCLUDE_MAVLINK)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_GPS_RX_BUF_LEN, PIOS_COM_MAVLINK_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_mavlink_id;
		target2 = &pios_com_gps_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOMAVLINKBRIDGE);
		PIOS_Modules_Enable(PIOS_MODULE_GPS);
#endif          /* PIOS_INCLUDE_MAVLINK */
		break;

	case HWSHARED_PORTTYPES_MSP:
#if defined(PIOS_INCLUDE_MSP_BRIDGE)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_MSP_RX_BUF_LEN, PIOS_COM_MSP_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_msp_id;
		PIOS_Modules_Enable(PIOS_MODULE_UAVOMSPBRIDGE);
#endif
		break;

	case HWSHARED_PORTTYPES_OPENLOG:
#if defined(PIOS_INCLUDE_OPENLOG)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, 0, PIOS_COM_OPENLOG_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_openlog_logging_id;
#endif /* PIOS_INCLUDE_OPENLOG */
		break;

	case HWSHARED_PORTTYPES_VTXCONFIGTBSSMARTAUDIO:
#if defined(PIOS_INCLUDE_TBSVTXCONFIG)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate = 5000;
			usart_port_params.init.USART_StopBits = USART_StopBits_2;
			usart_port_params.init.USART_Parity   = USART_Parity_No;
			usart_port_params.single_wire         = true;
			PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_TBSVTXCONFIG_RX_BUF_LEN, PIOS_COM_TBSVTXCONFIG_TX_BUF_LEN, com_driver, &port_driver_id);
			target = &pios_com_tbsvtxconfig_id;
			PIOS_Modules_Enable(PIOS_MODULE_VTXCONFIG);
		}
#endif /* PIOS_INCLUDE_TBSVTXCONFIG */
		break;

	case HWSHARED_PORTTYPES_PPM:
#if defined(PIOS_INCLUDE_PPM)
		if (ppm_cfg) {
			uintptr_t ppm_id;
			PIOS_PPM_Init(&ppm_id, ppm_cfg);

			uintptr_t ppm_rcvr_id;
			if (PIOS_RCVR_Init(&ppm_rcvr_id, &pios_ppm_rcvr_driver, ppm_id)) {
				PIOS_Assert(0);
			}

			PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_PPM, ppm_rcvr_id);
		}
#endif  /* PIOS_INCLUDE_PPM */
		break;

	case HWSHARED_PORTTYPES_PWM:
#if defined(PIOS_INCLUDE_PWM)
		if (pwm_cfg) {
			uintptr_t pwm_id;
			PIOS_PWM_Init(&pwm_id, pwm_cfg);

			uintptr_t pwm_rcvr_id;
			if (PIOS_RCVR_Init(&pwm_rcvr_id, &pios_pwm_rcvr_driver, pwm_id)) {
				PIOS_Assert(0);
			}

			PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_PWM, pwm_rcvr_id);
		}
#endif  /* PIOS_INCLUDE_PWM */
		break;

	case HWSHARED_PORTTYPES_SBUS:
	case HWSHARED_PORTTYPES_SBUSNONINVERTED:
#if defined(PIOS_INCLUDE_SBUS) && defined(PIOS_INCLUDE_USART)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 100000;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_Even;
			usart_port_params.init.USART_StopBits            = USART_StopBits_2;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx;

			if (port_type == HWSHARED_PORTTYPES_SBUS)
				usart_port_params.rx_invert = true;

			uintptr_t usart_sbus_id;
			if (PIOS_USART_Init(&usart_sbus_id, usart_port_cfg, &usart_port_params)) {
				PIOS_Assert(0);
			}
			uintptr_t sbus_id;
			if (PIOS_SBus_Init(&sbus_id, com_driver, usart_sbus_id)) {
				PIOS_Assert(0);
			}
			uintptr_t sbus_rcvr_id;
			if (PIOS_RCVR_Init(&sbus_rcvr_id, &pios_sbus_rcvr_driver, sbus_id)) {
				PIOS_Assert(0);
			}
			PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS, sbus_rcvr_id);
		}
#endif  /* PIOS_INCLUDE_SBUS */
		break;

	case HWSHARED_PORTTYPES_TELEMETRY:
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_telem_serial_id;
		break;

	case HWSHARED_PORTTYPES_SRXL:
#if defined(PIOS_INCLUDE_SRXL)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 115200;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_No;
			usart_port_params.init.USART_StopBits            = USART_StopBits_1;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx;

			PIOS_HAL_ConfigureSRXL(usart_port_cfg, &usart_port_params, com_driver);
		}
#endif  /* PIOS_INCLUDE_SRXL */
		break;

	case HWSHARED_PORTTYPES_IBUS:
#if defined(PIOS_INCLUDE_IBUS)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 115200;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_No;
			usart_port_params.init.USART_StopBits            = USART_StopBits_1;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx;

			PIOS_HAL_ConfigureIBus(usart_port_cfg, &usart_port_params, com_driver);
		}
#endif  /* PIOS_INCLUDE_IBUS */
		break;

	case HWSHARED_PORTTYPES_TBSCROSSFIRE:
#if defined(PIOS_INCLUDE_CROSSFIRE)
		if (usart_port_cfg) {
			usart_port_params.init.USART_BaudRate            = 420000;
			usart_port_params.init.USART_WordLength          = USART_WordLength_8b;
			usart_port_params.init.USART_Parity              = USART_Parity_No;
			usart_port_params.init.USART_StopBits            = USART_StopBits_1;
			usart_port_params.init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			usart_port_params.init.USART_Mode                = USART_Mode_Rx|USART_Mode_Tx;

			PIOS_HAL_ConfigureTbsCrossfire(usart_port_cfg, &usart_port_params, com_driver);
			PIOS_Modules_Enable(PIOS_MODULE_UAVOCROSSFIRETELEMETRY);
		}
#endif  /* PIOS_INCLUDE_CROSSFIRE */
		break;
	} /* port_type */

	PIOS_HAL_SetTarget(target, port_driver_id);
	PIOS_HAL_SetTarget(target2, port_driver_id);
}
#endif /* PIOS_INCLUDE_USART */

#if defined(PIOS_INCLUDE_USB_CDC)
/** @brief Configure USB CDC.
 *
 * @param[in] port_type The service provided over USB CDC communications
 * @param[in] usb_id ID of the USB device
 * @param[in] cdc_cfg Platform-specific CDC configuration
 */
void PIOS_HAL_ConfigureCDC(HwSharedUSB_VCPPortOptions port_type,
		uintptr_t usb_id,
		const struct pios_usb_cdc_cfg *cdc_cfg)
{
	uintptr_t pios_usb_cdc_id;

	if (port_type != HWSHARED_USB_VCPPORT_DISABLED) {
		if (PIOS_USB_CDC_Init(&pios_usb_cdc_id, cdc_cfg, usb_id)) {
			PIOS_Assert(0);
		}
	}

	switch (port_type) {
	case HWSHARED_USB_VCPPORT_DISABLED:
		break;
	case HWSHARED_USB_VCPPORT_USBTELEMETRY:
	{
		if (PIOS_COM_Init(&pios_com_telem_usb_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						PIOS_COM_TELEM_USB_RX_BUF_LEN,
						PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
	break;
	case HWSHARED_USB_VCPPORT_COMBRIDGE:
	{
		if (PIOS_COM_Init(&pios_com_vcp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						PIOS_COM_BRIDGE_RX_BUF_LEN,
						PIOS_COM_BRIDGE_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
		PIOS_Modules_Enable(PIOS_MODULE_COMUSBBRIDGE);
	}
	break;
	case HWSHARED_USB_VCPPORT_MSP:
#if defined(PIOS_INCLUDE_MSP_BRIDGE)
	{
		if (PIOS_COM_Init(&pios_com_msp_id, &pios_usb_cdc_com_driver, pios_usb_cdc_id,
						PIOS_COM_MSP_RX_BUF_LEN,
						PIOS_COM_MSP_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
		PIOS_Modules_Enable(PIOS_MODULE_UAVOMSPBRIDGE);
	}
#endif
	break;
	case HWSHARED_USB_VCPPORT_DEBUGCONSOLE:
#if defined(PIOS_INCLUDE_DEBUG_CONSOLE)
		{
			if (PIOS_COM_Init(&pios_com_debug_id,
					&pios_usb_cdc_com_driver,
					pios_usb_cdc_id, 0,
					PIOS_COM_DEBUGCONSOLE_TX_BUF_LEN)) {
				PIOS_Assert(0);
			}
		}
#endif  /* PIOS_INCLUDE_DEBUG_CONSOLE */
		break;
	}
}
#endif

#if defined(PIOS_INCLUDE_USB_HID)
/** @brief Configure USB HID.
 *
 * @param[in] port_type The service provided over USB HID communications
 * @param[in] usb_id ID of the USB device
 * @param[in] hid_cfg Platform-specific HID configuration
 */
void PIOS_HAL_ConfigureHID(HwSharedUSB_HIDPortOptions port_type,
		uintptr_t usb_id, const struct pios_usb_hid_cfg *hid_cfg)
{
	uintptr_t pios_usb_hid_id;
	if (PIOS_USB_HID_Init(&pios_usb_hid_id, hid_cfg, usb_id)) {
		PIOS_Assert(0);
	}

	switch (port_type) {
	case HWSHARED_USB_HIDPORT_DISABLED:
		break;
	case HWSHARED_USB_HIDPORT_USBTELEMETRY:
	{
		if (PIOS_COM_Init(&pios_com_telem_usb_id,
				&pios_usb_hid_com_driver, pios_usb_hid_id,
				PIOS_COM_TELEM_USB_RX_BUF_LEN,
				PIOS_COM_TELEM_USB_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}
	}
	break;
	}
}

#endif  /* PIOS_INCLUDE_USB_HID */

#if defined(PIOS_INCLUDE_OPENLRS)
/** @brief Configure RFM22B radio.
 *
 * @param[in] radio_type What goes over this radio link
 * @param[in] board_type Target board type
 * @param[in] board_rev Target board revision
 * @param[in] openlrs_cfg Configuration for radio in openlrs mode
 */
void PIOS_HAL_ConfigureRFM22B(pios_spi_t spi_dev,
		uint8_t board_type, uint8_t board_rev,
		HwSharedRfBandOptions rf_band,
		HwSharedMaxRfPowerOptions rf_power,
		const struct pios_openlrs_cfg *openlrs_cfg,
		pios_openlrs_t *handle)
{
	OpenLRSInitialize();

	pios_openlrs_t openlrs_id;

	if (PIOS_OpenLRS_Init(&openlrs_id, spi_dev, 0, openlrs_cfg, rf_band,
				rf_power)) {
		return;
	}

	*handle = openlrs_id;

	PIOS_OpenLRS_Start(openlrs_id);

#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
	uintptr_t rfm22brcvr_id;
	uintptr_t rfm22brcvr_rcvr_id;

	PIOS_OpenLRS_Rcvr_Init(&rfm22brcvr_id, openlrs_id);

	if (PIOS_RCVR_Init(&rfm22brcvr_rcvr_id, &pios_openlrs_rcvr_driver, rfm22brcvr_id)) {
		PIOS_Assert(0);
	}

	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_OPENLRS, rfm22brcvr_rcvr_id);
#endif /* PIOS_INCLUDE_OPENLRS_RCVR */
}
#endif /* PIOS_INCLUDE_OPENLRS */

/* Needs some safety margin over 1000. */
#define BT_COMMAND_DELAY 1100

#define BT_COMMAND_QDELAY 350

void PIOS_HAL_ConfigureSerialSpeed(uintptr_t com_id,
		HwSharedSpeedBpsOptions speed) {
	switch (speed) {
		case HWSHARED_SPEEDBPS_1200:
			PIOS_COM_ChangeBaud(com_id, 1200);
			break;
		case HWSHARED_SPEEDBPS_2400:
			PIOS_COM_ChangeBaud(com_id, 2400);
			break;
		case HWSHARED_SPEEDBPS_4800:
			PIOS_COM_ChangeBaud(com_id, 4800);
			break;
		case HWSHARED_SPEEDBPS_9600:
			PIOS_COM_ChangeBaud(com_id, 9600);
			break;
		case HWSHARED_SPEEDBPS_19200:
			PIOS_COM_ChangeBaud(com_id, 19200);
			break;
		case HWSHARED_SPEEDBPS_38400:
			PIOS_COM_ChangeBaud(com_id, 38400);
			break;
		case HWSHARED_SPEEDBPS_57600:
			PIOS_COM_ChangeBaud(com_id, 57600);
			break;
		case HWSHARED_SPEEDBPS_INITHM10:
			PIOS_COM_ChangeBaud(com_id, 9600);

			PIOS_COM_SendString(com_id,"AT+BAUD4"); // 115200
			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);

			PIOS_COM_ChangeBaud(com_id, 115200);

			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);
			PIOS_COM_SendString(com_id,"AT+NAMEdRonin");
			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);
			PIOS_COM_SendString(com_id,"AT+PASS000000");
			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);
			PIOS_COM_SendString(com_id,"AT+POWE3");
			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);
			PIOS_COM_SendString(com_id,"AT+RESET");
			PIOS_Thread_Sleep(BT_COMMAND_QDELAY);
			break;

		case HWSHARED_SPEEDBPS_INITHC06:
			/* Some modules default to 38400, some to 9600.
			 * Best effort to work with 38400. */

			/* ~4.5 second init time. */
			PIOS_COM_ChangeBaud(com_id, 38400);
			PIOS_COM_SendString(com_id,"AT+BAUD8"); // 115200
			PIOS_Thread_Sleep(BT_COMMAND_DELAY);

			PIOS_COM_ChangeBaud(com_id, 9600);
			PIOS_COM_SendString(com_id,"AT+BAUD8"); 
			PIOS_Thread_Sleep(BT_COMMAND_DELAY);

			PIOS_COM_ChangeBaud(com_id, 115200); 
			PIOS_COM_SendString(com_id,"AT+NAMEdRonin");
			PIOS_Thread_Sleep(BT_COMMAND_DELAY);
			PIOS_COM_SendString(com_id,"AT+PIN0000");
			PIOS_Thread_Sleep(BT_COMMAND_DELAY);
			break;

		case HWSHARED_SPEEDBPS_INITHC05:
			/* Some modules default to 38400, some to 9600.
			 * Best effort to work with 38400. */

			/* Not silence delimited; but usually requires you to
			 * push a button at magical timing */
			PIOS_COM_ChangeBaud(com_id, 38400);
			PIOS_COM_SendString(com_id,"AT+UART=115200,0,0\r\n"); // 9600
			PIOS_Thread_Sleep(BT_COMMAND_DELAY/2);
			PIOS_COM_ChangeBaud(com_id, 9600);
			PIOS_COM_SendString(com_id,"AT+UART=115200,0,0\r\n"); // 9600
			PIOS_Thread_Sleep(BT_COMMAND_DELAY/2);

			PIOS_COM_ChangeBaud(com_id, 115200);

			PIOS_COM_SendString(com_id,"AT+NAME=dRonin\r\n");
			PIOS_Thread_Sleep(BT_COMMAND_DELAY/2);
			PIOS_COM_SendString(com_id,"AT+PSWD=0000\r\n");
			PIOS_Thread_Sleep(BT_COMMAND_DELAY/2);

			break;

		case HWSHARED_SPEEDBPS_115200:
			PIOS_COM_ChangeBaud(com_id, 115200);
			break;

		case HWSHARED_SPEEDBPS_230400:
			PIOS_COM_ChangeBaud(com_id, 230400);
			break;
	}
}


#ifdef PIOS_INCLUDE_I2C
static int PIOS_HAL_ConfigureI2C(pios_i2c_t *id,
		const struct pios_i2c_adapter_cfg *cfg) {
#ifndef FLIGHT_POSIX
	if (!*id) {
		// Not already initialized.
		if (PIOS_I2C_Init(id, cfg)) {
			PIOS_DEBUG_Assert(0);
			AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_CRITICAL);

			return -1;
		}
	}

	if (PIOS_I2C_CheckClear(*id) != 0) {
		AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_CRITICAL);
		return -2;
	}

	if (AlarmsGet(SYSTEMALARMS_ALARM_I2C) ==
			SYSTEMALARMS_ALARM_UNINITIALISED) {
		AlarmsSet(SYSTEMALARMS_ALARM_I2C, SYSTEMALARMS_ALARM_OK);
	}

	PIOS_WDG_Clear();
#endif /* FLIGHT_POSIX */

	return 0;
}
#endif // PIOS_INCLUDE_I2C

int PIOS_HAL_ConfigureExternalBaro(HwSharedExtBaroOptions baro,
		pios_i2c_t *i2c_id,
		const struct pios_i2c_adapter_cfg *i2c_cfg)
{
#if !defined(PIOS_INCLUDE_I2C)
	return -1;
#else
	if (baro == HWSHARED_EXTBARO_NONE) {
		return 1;
	}

	int ret = PIOS_HAL_ConfigureI2C(i2c_id, i2c_cfg);

	if (ret) goto done;

	switch(baro) {
#if defined(PIOS_INCLUDE_BMP280)
	case HWSHARED_EXTBARO_BMP280:
		ret = PIOS_BMP280_Init(&external_bmp280_cfg, *i2c_id) ;

		if (ret) goto done;

		ret = PIOS_BMP280_Test();

		if (ret) goto done;

		break;
#endif // PIOS_INCLUDE_BMP280

#if defined(PIOS_INCLUDE_MS5611)
	case HWSHARED_EXTBARO_MS5611:
		ret = PIOS_MS5611_Init(&external_ms5611_cfg, *i2c_id);

		if (ret) goto done;

		ret = PIOS_MS5611_Test();

		if (ret) goto done;

		break;
#endif // PIOS_INCLUDE_MS5611

	default:
		PIOS_Assert(0);	// Should be unreachable
		break;
	}

done:
	if (ret) {
		PIOS_SENSORS_SetMissing(PIOS_SENSOR_BARO);
	}

	return ret;
#endif /* PIOS_INCLUDE_I2C */
}

int PIOS_HAL_ConfigureExternalMag(HwSharedMagOptions mag,
		HwSharedMagOrientationOptions orientation,
		pios_i2c_t *i2c_id,
		const struct pios_i2c_adapter_cfg *i2c_cfg)
{
#if !defined(PIOS_INCLUDE_I2C)
	return -1;
#else
	if (!HwSharedMagOrientationIsValid(orientation)) {
		return -2;
	}

	/* internal mag should be handled in pios_board_init */
	if (mag == HWSHARED_MAG_NONE || mag == HWSHARED_MAG_INTERNAL) {
		return 1;
	}

	if (PIOS_HAL_ConfigureI2C(i2c_id, i2c_cfg))
		goto mag_fail;

	switch (mag) {
#ifdef PIOS_INCLUDE_HMC5883
	case HWSHARED_MAG_EXTERNALHMC5883:
		if (PIOS_HMC5883_Init(*i2c_id, &external_hmc5883_cfg))
			goto mag_fail;

		if (PIOS_HMC5883_Test())
			goto mag_fail;

		/* XXX: Lame.  Move driver to HwShared constants.
		 * Note: drivers rotate around vehicle Z-axis, this rotates around ??
		 * This is dubious because both the sensor and vehicle Z-axis point down
		 * in bottom orientation, whereas this is rotating around an upwards vector. */
		enum pios_hmc5883_orientation hmc5883_orientation = 
			(orientation == HWSHARED_MAGORIENTATION_TOP0DEGCW)      ? PIOS_HMC5883_TOP_0DEG      : 
			(orientation == HWSHARED_MAGORIENTATION_TOP90DEGCW)     ? PIOS_HMC5883_TOP_90DEG     : 
			(orientation == HWSHARED_MAGORIENTATION_TOP180DEGCW)    ? PIOS_HMC5883_TOP_180DEG    : 
			(orientation == HWSHARED_MAGORIENTATION_TOP270DEGCW)    ? PIOS_HMC5883_TOP_270DEG    : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM0DEGCW)   ? PIOS_HMC5883_BOTTOM_0DEG   : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM90DEGCW)  ? PIOS_HMC5883_BOTTOM_270DEG : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM180DEGCW) ? PIOS_HMC5883_BOTTOM_180DEG : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM270DEGCW) ? PIOS_HMC5883_BOTTOM_90DEG  : 
			external_hmc5883_cfg.Default_Orientation;

		PIOS_HMC5883_SetOrientation(hmc5883_orientation);
		break;
#endif /* PIOS_INCLUDE_HMC5883 */

#ifdef PIOS_INCLUDE_HMC5983_I2C
	case HWSHARED_MAG_EXTERNALHMC5983:
		if (PIOS_HMC5983_I2C_Init(*i2c_id, &external_hmc5983_cfg))
			goto mag_fail;

		if (PIOS_HMC5983_Test())
			goto mag_fail;

		/* Annoying to do this, but infecting low-level drivers with UAVO deps is yucky
		 * Note: drivers rotate around vehicle Z-axis, this rotates around ??
		 * This is dubious because both the sensor and vehicle Z-axis point down
		 * in bottom orientation, whereas this is rotating around an upwards vector. */
		enum pios_hmc5983_orientation hmc5983_orientation = 
			(orientation == HWSHARED_MAGORIENTATION_TOP0DEGCW)      ? PIOS_HMC5983_TOP_0DEG      : 
			(orientation == HWSHARED_MAGORIENTATION_TOP90DEGCW)     ? PIOS_HMC5983_TOP_90DEG     : 
			(orientation == HWSHARED_MAGORIENTATION_TOP180DEGCW)    ? PIOS_HMC5983_TOP_180DEG    : 
			(orientation == HWSHARED_MAGORIENTATION_TOP270DEGCW)    ? PIOS_HMC5983_TOP_270DEG    : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM0DEGCW)   ? PIOS_HMC5983_BOTTOM_0DEG   : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM90DEGCW)  ? PIOS_HMC5983_BOTTOM_270DEG : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM180DEGCW) ? PIOS_HMC5983_BOTTOM_180DEG : 
			(orientation == HWSHARED_MAGORIENTATION_BOTTOM270DEGCW) ? PIOS_HMC5983_BOTTOM_90DEG  : 
			external_hmc5983_cfg.Orientation;

		PIOS_HMC5983_SetOrientation(hmc5983_orientation);
		break;
#endif /* PIOS_INCLUDE_HMC5983_I2C */

	default:
		PIOS_Assert(0);	/* Should be unreachable */
	}

	return 0;

mag_fail:
	PIOS_SENSORS_SetMissing(PIOS_SENSOR_MAG);
	return -2;
#endif /* PIOS_INCLUDE_I2C */
}

#ifdef PIOS_INCLUDE_DAC
int PIOS_HAL_ConfigureDAC(dac_dev_t dac)
{
	DACSettingsInitialize();

	DACSettingsData settings;

	DACSettingsGet(&settings);

	switch (settings.DACPrimaryUse) {
#ifdef PIOS_INCLUDE_DAC_FSK
	case DACSETTINGS_DACPRIMARYUSE_FSK1200:
		if (pios_com_lighttelemetry_id) {
			return -1;
		}

		fskdac_dev_t fskdac;

		if (PIOS_FSKDAC_Init(&fskdac, dac)) {
			return -1;
		}

		if (PIOS_COM_Init(&pios_com_lighttelemetry_id,
					&pios_fskdac_com_driver,
					(uintptr_t) fskdac, 0,
					PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN)) {
			return -1;
		}

		PIOS_Modules_Enable(PIOS_MODULE_UAVOLIGHTTELEMETRYBRIDGE);
		break;
#endif // PIOS_INCLUDE_DAC_FSK

#ifdef PIOS_INCLUDE_DAC_ANNUNCIATOR
	case DACSETTINGS_DACPRIMARYUSE_ANNUNCIATORS:
		if (PIOS_ANNUNCDAC_Init(&pios_dac_annunciator_id, dac)) {
			return -1;
		}
		break;
#endif
	case DACSETTINGS_DACPRIMARYUSE_DISABLED:
		break;
	default:
		return -1;
	}

	return 0;
}
#endif

void PIOS_HAL_InitUAVTalkReceiver()
{
#if defined(PIOS_INCLUDE_UAVTALKRCVR)
	UAVTalkReceiverInitialize();
	uintptr_t pios_uavtalk_id;

	if (PIOS_UAVTALKRCVR_Init(&pios_uavtalk_id)) {
		PIOS_Assert(0);
	}

	uintptr_t pios_uavtalk_rcvr_id;
	if (PIOS_RCVR_Init(&pios_uavtalk_rcvr_id,
				&pios_uavtalk_rcvr_driver,
				pios_uavtalk_id)) {
		PIOS_Assert(0);
	}

	PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_UAVTALK,
			pios_uavtalk_rcvr_id);
#endif	/* PIOS_INCLUDE_UAVTALKRCVR */
}
