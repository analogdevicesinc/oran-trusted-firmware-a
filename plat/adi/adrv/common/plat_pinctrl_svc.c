/*
 * Copyright (c) 2022~2025, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>

#include <plat_err.h>
#include <plat_pinctrl.h>
#include <plat_pinctrl_svc.h>
#include <plat_sip_svc.h>
#include <platform_def.h>
#include <lib/smccc.h>

/*
 * The following defines the bit mask for incoming argument 5
 */
#define PINCTRL_x5_BIT_PIN_ST           U(0x1)          //Incoming request for Input pin to be configured as Schmitt Trigger
#define PINCTRL_x5_BIT_PIN_ENABLE_PU_PD U(0x2)          //Enablement of PullUP/PullDOWN
#define PINCTRL_x5_BIT_PIN_PU_PD_SEL    U(0x4)          //When Enabled, Sets PullDOWN(0) or Pullup(1)

/* SMC GET result defines*/
#define ADI_GET_BITFIELD_1_PIN_CONFIGURED_BIT_POSITION (63U)
#define ADI_GET_BITFIELD_1_PIN_NUM_SHIFT (16U)
#define ADI_GET_BITFIELD_1_MUX_SEL_SHIFT (0U)

/*
 * Bit Mask Info for ADI's Pinctrl Word
 */
#define ADI_CONFIG_DRIVE_STRENGTH_MASK                        (0x0000000FU)
#define ADI_CONFIG_DRIVE_STRENGTH_MASK_BIT_POSITION           (0U)
#define ADI_CONFIG_SCHMITT_TRIG_ENABLE_MASK                   (0x00000010U)
#define ADI_CONFIG_SCHMITT_TRIG_ENABLE_MASK_BIT_POSITION      (4U)
#define ADI_CONFIG_PULL_UP_DOWN_ENABLEMENT_MASK               (0x00000020U)
#define ADI_CONFIG_PULL_UP_DOWN_ENABLEMENT_MASK_BIT_POSITION  (5U)
#define ADI_CONFIG_PULLUP_ENABLE_MASK                         (0x00000040U)
#define ADI_CONFIG_PULLUP_ENABLE_MASK_BIT_POSITION            (6)
#define ADI_CONFIG_MUX_SEL_MASK                               (0x000000FFU)

typedef enum {
	INIT	= 0,
	SET	= 1,
	GET	= 2
} func_id_t;

/*
 * PinMUX service SMC handler
 */
uintptr_t plat_pinctrl_smc_handler(unsigned int smc_fid,
				   u_register_t x1,
				   u_register_t x2,
				   u_register_t x3,
				   u_register_t x4,
				   void *cookie,
				   void *handle,
				   u_register_t flags)
{
	func_id_t id;
	uintptr_t base_addr;
	plat_pinctrl_settings settings;
	bool result;

	/*
	 * Retrieve the additional SMC parameters
	 */
	u_register_t x5 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);
	u_register_t x6 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X6);
	u_register_t x7 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X7);

	id = (func_id_t)x1;
	switch (id) {
	case INIT:
		/* Pinmux Initialization
		 * If anything to initialize, do it here. */
		plat_runtime_warn_message("PINCTRL service: INIT not currently implemented");
		SMC_RET1(handle, SMC_UNK);
		break;

	case SET:
		/* PinMUX SET command received*/
		settings.pin_pad = x2;
		settings.src_mux = x3;
		settings.drive_strength = x4;
		base_addr = (uintptr_t)x6;

		settings.extended_options = x7;

		if ((x5 & PINCTRL_x5_BIT_PIN_ST) == 0x00U)
			settings.schmitt_trigger_enable = false;
		else
			settings.schmitt_trigger_enable = true;

		if ((x5 & PINCTRL_x5_BIT_PIN_ENABLE_PU_PD) == 0x00U)
			settings.pullup_pulldown_enablement = false;
		else
			settings.pullup_pulldown_enablement = true;

		if ((x5 & PINCTRL_x5_BIT_PIN_PU_PD_SEL) == 0x00U)
			settings.pullup = false;
		else
			settings.pullup = true;

		result = plat_secure_pinctrl_set(settings, is_caller_secure(flags), base_addr);
		SMC_RET2(handle, SMC_OK, result);
		break;

	case GET:
		/* PinMUX GET command received*/
		unsigned long a2 = 0, a3 = 0;
		settings.pin_pad = x2;
		base_addr = (uintptr_t)x6;
		settings.extended_options = x7;
		result = plat_secure_pinctrl_get(&settings, is_caller_secure(flags), base_addr);

		a2 = BIT(ADI_GET_BITFIELD_1_PIN_CONFIGURED_BIT_POSITION);
		a2 |= settings.pin_pad << ADI_GET_BITFIELD_1_PIN_NUM_SHIFT;
		a2 |= settings.src_mux << ADI_GET_BITFIELD_1_MUX_SEL_SHIFT;

		a3 = settings.drive_strength;
		if (settings.schmitt_trigger_enable)
			a3 |= BIT(ADI_CONFIG_SCHMITT_TRIG_ENABLE_MASK_BIT_POSITION);
		if (settings.pullup_pulldown_enablement)
			a3 |= BIT(ADI_CONFIG_PULL_UP_DOWN_ENABLEMENT_MASK_BIT_POSITION);
		if (settings.pullup)
			a3 |= BIT(ADI_CONFIG_PULLUP_ENABLE_MASK_BIT_POSITION);

		SMC_RET4(handle, SMC_OK, result, a2, a3);
		break;

	default:
		plat_runtime_warn_message("PINCTRL service: Unexpected command");
		SMC_RET1(handle, SMC_UNK);
		break;
	}

	plat_runtime_warn_message("PINCTRL service: Unexpected FID");
	SMC_RET1(handle, SMC_UNK);
}
