/**
 ******************************************************************************
 * @file       pios_hal.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <pios_openlrs_rcvr_priv.h>
#include <pios_rfm22b_rcvr_priv.h>
#include <pios_hsum_priv.h>

#include <pios_modules.h>
#include <pios_sys.h>

#include <manualcontrolsettings.h>

#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
#include <openlrs.h>
#endif

#include <sanitycheck.h>

uintptr_t pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_NONE];

#if defined(PIOS_INCLUDE_RFM22B)
uint32_t pios_rfm22b_id;
uintptr_t pios_com_rf_id;
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

#if defined(PIOS_INCLUDE_PICOC)
uintptr_t pios_com_picoc_id;
#endif

#if defined(PIOS_INCLUDE_STORM32BGC)
uintptr_t pios_com_storm32bgc_id;
#endif

#if defined(PIOS_INCLUDE_USB_HID) || defined(PIOS_INCLUDE_USB_CDC)
uintptr_t pios_com_telem_usb_id;
#endif

#if defined(PIOS_INCLUDE_USB_CDC)
uintptr_t pios_com_vcp_id;
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
#define PIOS_COM_TELEM_USB_RX_BUF_LEN 65
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
#define PIOS_COM_MSP_RX_BUF_LEN 64
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
#define PIOS_COM_LIGHTTELEMETRY_TX_BUF_LEN 19
#endif

#ifndef PIOS_COM_PICOC_RX_BUF_LEN
#define PIOS_COM_PICOC_RX_BUF_LEN 128
#endif

#ifndef PIOS_COM_PICOC_TX_BUF_LEN
#define PIOS_COM_PICOC_TX_BUF_LEN 128
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

#ifndef PIOS_COM_RFM22B_RF_RX_BUF_LEN
#define PIOS_COM_RFM22B_RF_RX_BUF_LEN 640
#endif

#ifndef PIOS_COM_RFM22B_RF_TX_BUF_LEN
#define PIOS_COM_RFM22B_RF_TX_BUF_LEN 640
#endif

#ifndef PIOS_COM_STORM32BGC_RX_BUF_LEN
#define PIOS_COM_STORM32BGC_RX_BUF_LEN 32
#endif

#ifndef PIOS_COM_STORM32BGC_TX_BUF_LEN
#define PIOS_COM_STORM32BGC_TX_BUF_LEN 32
#endif

/**
 * @brief Flash a blink code.
 * @param[in] led_id The LED to blink
 * @param[in] code Number of blinks to do in a row
 */
void PIOS_HAL_Panic(uint32_t led_id, enum pios_hal_panic code) {
	for (int cnt = 0; cnt < 3; cnt++) {
		for (int32_t i = 0; i < code; i++) {
			PIOS_WDG_Clear();
			PIOS_LED_Toggle(led_id);
			PIOS_DELAY_WaitmS(175);
			PIOS_WDG_Clear();
			PIOS_LED_Toggle(led_id);
			PIOS_DELAY_WaitmS(175);
		}
		PIOS_DELAY_WaitmS(200);
		PIOS_WDG_Clear();
		PIOS_DELAY_WaitmS(200);
		PIOS_WDG_Clear();
		PIOS_DELAY_WaitmS(100);
		PIOS_WDG_Clear();
	}

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

/**
 * @brief Assign a device instance into the receiver map
 *
 * @param[in] receiver_type the receiver type index from MANUALCONTROL
 * @param[in] value handle of the device instance
 */
static void PIOS_HAL_SetReceiver(int receiver_type, uintptr_t value) {
	PIOS_HAL_SetTarget(pios_rcvr_group_map + receiver_type, value);
}

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
void PIOS_HAL_ConfigurePort(HwSharedPortTypesOptions port_type,
		const struct pios_usart_cfg *usart_port_cfg,
		const struct pios_com_driver *com_driver,
		uint32_t *i2c_id,
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

	case HWSHARED_PORTTYPES_FRSKYSPORTTELEMETRY:
#if defined(PIOS_INCLUDE_FRSKY_SPORT_TELEMETRY)
#if defined(STM32F30X)
		// F3 has internal inverters and can switch rx/tx pins
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

	case HWSHARED_PORTTYPES_PICOC:
#if defined(PIOS_INCLUDE_PICOC)
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_PICOC_RX_BUF_LEN, PIOS_COM_PICOC_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_picoc_id;
#endif /* PIOS_INCLUDE_PICOC */
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
			pios_rcvr_group_map[MANUALCONTROLSETTINGS_CHANNELGROUPS_SBUS] = sbus_rcvr_id;
		}
#endif  /* PIOS_INCLUDE_SBUS */
		break;

	case HWSHARED_PORTTYPES_STORM32BGC:
#if defined(PIOS_INCLUDE_STORM32BGC)
		usart_port_params.init.USART_BaudRate = 115200;

		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_STORM32BGC_RX_BUF_LEN, PIOS_COM_STORM32BGC_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_storm32bgc_id;
		PIOS_Modules_Enable(PIOS_MODULE_STORM32BGC);
#endif  /* PIOS_INCLUDE_STORM32BGC */
		break;

	case HWSHARED_PORTTYPES_TELEMETRY:
		PIOS_HAL_ConfigureCom(usart_port_cfg, &usart_port_params, PIOS_COM_TELEM_RF_RX_BUF_LEN, PIOS_COM_TELEM_RF_TX_BUF_LEN, com_driver, &port_driver_id);
		target = &pios_com_telem_serial_id;
		break;

	} /* port_type */

	PIOS_HAL_SetTarget(target, port_driver_id);
	PIOS_HAL_SetTarget(target2, port_driver_id);
}

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
	case HWSHARED_USB_VCPPORT_PICOC:
#if defined(PIOS_INCLUDE_PICOC)
		{
			if (PIOS_COM_Init(&pios_com_picoc_id,
					&pios_usb_cdc_com_driver,
					pios_usb_cdc_id,
					PIOS_COM_PICOC_RX_BUF_LEN,
					PIOS_COM_PICOC_TX_BUF_LEN)) {
				PIOS_Assert(0);
			}
		}
#endif  /* PIOS_INCLUDE_PICOC */
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

#if defined(PIOS_INCLUDE_RFM22B)
/** @brief Configure RFM22B radio.
 *
 * @param[in] radio_type What goes over this radio link
 * @param[in] board_type Target board type
 * @param[in] board_rev Target board revision
 * @param[in] max_power Maximum configured output power
 * @param[in] max_speed Maximum configured speed
 * @param[in] openlrs_cfg Configuration for radio in openlrs mode
 * @param[in] rfm22b_cfg Configuration for radio in TauLink mode
 * @param[in] min_chan Minimum channel id.
 * @param[in] max_chan Maximum channel id
 * @param[in] coord_id 0 if we are coordinator, else the coord's radio id.
 * @param[in] status_inst Which instance number to save RFM22BStatus to
 */
void PIOS_HAL_ConfigureRFM22B(HwSharedRadioPortOptions radio_type,
		uint8_t board_type, uint8_t board_rev,
		HwSharedMaxRfPowerOptions max_power,
		HwSharedMaxRfSpeedOptions max_speed,
		HwSharedRfBandOptions rf_band,
		const struct pios_openlrs_cfg *openlrs_cfg,
		const struct pios_rfm22b_cfg *rfm22b_cfg,
		uint8_t min_chan, uint8_t max_chan, uint32_t coord_id,
		int status_inst)
{
	/* Initalize the RFM22B radio COM device. */
	RFM22BStatusInitialize();
	RFM22BStatusCreateInstance();

#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
	OpenLRSInitialize();
#endif

	RFM22BStatusData rfm22bstatus;
	RFM22BStatusGet(&rfm22bstatus);
	RFM22BStatusInstSet(1,&rfm22bstatus);

	// Initialize out status object.
	rfm22bstatus.BoardType     = board_type;
	rfm22bstatus.BoardRevision = board_rev;

	if (radio_type == HWSHARED_RADIOPORT_OPENLRS) {
#if defined(PIOS_INCLUDE_OPENLRS_RCVR)
		uintptr_t openlrs_id;

		PIOS_OpenLRS_Init(&openlrs_id, PIOS_RFM22_SPI_PORT, 0, openlrs_cfg, rf_band);

		{
			uintptr_t rfm22brcvr_id;
			PIOS_OpenLRS_Rcvr_Init(&rfm22brcvr_id, openlrs_id);
			uintptr_t rfm22brcvr_rcvr_id;
			if (PIOS_RCVR_Init(&rfm22brcvr_rcvr_id, &pios_openlrs_rcvr_driver, rfm22brcvr_id)) {
				PIOS_Assert(0);
			}
			PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_OPENLRS, rfm22brcvr_rcvr_id);
		}
#endif /* PIOS_INCLUDE_OPENLRS_RCVR */
	} else if (radio_type == HWSHARED_RADIOPORT_DISABLED ||
			max_power == HWSHARED_MAXRFPOWER_0) {
		// When radio disabled, it is ok for init to fail. This allows
		// boards without populating this component.
		if (PIOS_RFM22B_Init(&pios_rfm22b_id, PIOS_RFM22_SPI_PORT, rfm22b_cfg->slave_num, rfm22b_cfg, rf_band) == 0) {
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_0);
			rfm22bstatus.DeviceID = PIOS_RFM22B_DeviceID(pios_rfm22b_id);
			rfm22bstatus.BoardRevision = PIOS_RFM22B_ModuleVersion(pios_rfm22b_id);
		} else {
			pios_rfm22b_id = 0;
		}

		rfm22bstatus.LinkState = RFM22BSTATUS_LINKSTATE_DISABLED;
	} else {
		// always allow receiving PPM when radio is on
		bool ppm_mode    = radio_type == HWSHARED_RADIOPORT_TELEMPPM ||
				radio_type == HWSHARED_RADIOPORT_PPM;
		bool ppm_only    = radio_type == HWSHARED_RADIOPORT_PPM;
		bool is_oneway   = radio_type == HWSHARED_RADIOPORT_PPM;
		// Sparky2 can only receive PPM only

		/* Configure the RFM22B device. */
		if (PIOS_RFM22B_Init(&pios_rfm22b_id, PIOS_RFM22_SPI_PORT, rfm22b_cfg->slave_num, rfm22b_cfg, rf_band)) {
			PIOS_Assert(0);
		}

		rfm22bstatus.DeviceID = PIOS_RFM22B_DeviceID(pios_rfm22b_id);
		rfm22bstatus.BoardRevision = PIOS_RFM22B_ModuleVersion(pios_rfm22b_id);

		/* Configure the radio com interface */
		if (PIOS_COM_Init(&pios_com_rf_id, &pios_rfm22b_com_driver,
				pios_rfm22b_id,
				PIOS_COM_RFM22B_RF_RX_BUF_LEN,
				PIOS_COM_RFM22B_RF_TX_BUF_LEN)) {
			PIOS_Assert(0);
		}

#ifndef PIOS_NO_TELEM_ON_RF
		/* Set Telemetry to use RFM22b if no other telemetry is configured (USB always overrides anyway) */
		if (!pios_com_telem_serial_id) {
			pios_com_telem_serial_id = pios_com_rf_id;
		}
#endif
		rfm22bstatus.LinkState = RFM22BSTATUS_LINKSTATE_ENABLED;

		/* Set the radio configuration parameters. */
		PIOS_RFM22B_Config(pios_rfm22b_id, max_speed, min_chan, max_chan, coord_id, is_oneway, ppm_mode, ppm_only);

		// XXX TODO: Factor these power switches out.
		/* Set the modem Tx poer level */
		switch (max_power) {
		case HWSHARED_MAXRFPOWER_125:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_0);
			break;
		case HWSHARED_MAXRFPOWER_16:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_1);
			break;
		case HWSHARED_MAXRFPOWER_316:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_2);
			break;
		case HWSHARED_MAXRFPOWER_63:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_3);
			break;
		case HWSHARED_MAXRFPOWER_126:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_4);
			break;
		case HWSHARED_MAXRFPOWER_25:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_5);
			break;
		case HWSHARED_MAXRFPOWER_50:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_6);
			break;
		case HWSHARED_MAXRFPOWER_100:
			PIOS_RFM22B_SetTxPower(pios_rfm22b_id, RFM22_tx_pwr_txpow_7);
			break;
		default:
			// do nothing
			break;
		}

		/* Reinitialize the modem. */
		PIOS_RFM22B_Reinit(pios_rfm22b_id);

#if defined(PIOS_INCLUDE_RFM22B_RCVR)
		{
			uintptr_t rfm22brcvr_id;
			PIOS_RFM22B_Rcvr_Init(&rfm22brcvr_id, pios_rfm22b_id);
			uintptr_t rfm22brcvr_rcvr_id;
			if (PIOS_RCVR_Init(&rfm22brcvr_rcvr_id, &pios_rfm22b_rcvr_driver, rfm22brcvr_id)) {
				PIOS_Assert(0);
			}
			PIOS_HAL_SetReceiver(MANUALCONTROLSETTINGS_CHANNELGROUPS_RFM22B, rfm22brcvr_rcvr_id);
		}
#endif /* PIOS_INCLUDE_RFM22B_RCVR */
	}

	RFM22BStatusInstSet(status_inst, &rfm22bstatus);
}
#endif /* PIOS_INCLUDE_RFM22B */

/* Needs some safety margin over 1000. */
#define BT_COMMAND_DELAY 1100

#define BT_COMMAND_QDELAY 350

void PIOS_HAL_ConfigureSerialSpeed(uintptr_t com_id,
		HwSharedSpeedBpsOptions speed) {
	switch (speed) {
		case HWSHARED_SPEEDBPS_1200:
			PIOS_COM_ChangeBaud(com_id, 2400);
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
