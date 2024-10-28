/*
 * Copyright (c) 2022, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>

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
		WARN("PINCTRL service: INIT not currently implemented\n");
		SMC_RET1(handle, SMC_UNK);
		break;
	case SET:
		/* PinMUX SET command received*/
		plat_pinctrl_settings settings;
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

		bool result = plat_secure_pinctrl_set(settings, is_caller_secure(flags), base_addr);

		SMC_RET2(handle, SMC_OK, result);

		break;
	case GET:
		/* Future location of PINCTRL Get operation. */
		WARN("PINCTRL service: GET not currently implemented.\n");
		SMC_RET1(handle, SMC_UNK);
		break;
	default:
		WARN("PINCTRL service: Unexpected command\n");
		SMC_RET1(handle, SMC_UNK);
		break;
	}

	WARN("PINCTRL service: Unexpected FID\n");
	SMC_RET1(handle, SMC_UNK);
}
