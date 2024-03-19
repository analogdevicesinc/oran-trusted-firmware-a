/*
 * Copyright(c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ddr_config.h"

ddr_function_config_t ddr_function_configurations[DDR_NUM_CONFIGURATIONS] = {
#ifdef DDR_PRIMARY_4GB_2RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_4gb_2rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_PRIMARY_4GB_2RANK_X16_1GBX16_3200
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx16_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx16_3200_phy_init      },
#endif
#ifdef DDR_PRIMARY_4GB_2RANK_X16_1GBX8_3200
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx8_3200_phy_init       },
#endif
#ifdef DDR_PRIMARY_4GB_1RANK_X16_2GBX8_3200
	{ .pre_reset_function = ddr_4gb_1rank_x16_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_1rank_x16_2gbx8_3200_phy_init       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX16_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx16_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx16_3200_phy_init      },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_2gb_1rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_PRIMARY_2GB_2RANK_X8_1GBX8_3200
	{ .pre_reset_function = ddr_2gb_2rank_x8_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_1gbx8_3200_phy_init	       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX8_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx8_3200_phy_init       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX8_MULTI_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_multi_3200_pre_reset_init, .phy_function = ddr_2gb_1rank_x16_1gbx8_multi_3200_phy_init },
#endif
#ifdef DDR_PRIMARY_2GB_2RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_2gb_2rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_PRIMARY_4GB_2RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_4gb_2rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x8_2gbx8_1600_phy_init	       },
#endif
#ifdef DDR_PRIMARY_4GB_2RANK_X16_1GBX16_1600
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx16_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx16_1600_phy_init      },
#endif
#ifdef DDR_PRIMARY_4GB_2RANK_X16_1GBX8_1600
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx8_1600_phy_init       },
#endif
#ifdef DDR_PRIMARY_4GB_1RANK_X16_2GBX8_1600
	{ .pre_reset_function = ddr_4gb_1rank_x16_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_1rank_x16_2gbx8_1600_phy_init       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX16_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx16_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx16_1600_phy_init      },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_2gb_1rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x8_2gbx8_1600_phy_init	       },
#endif
#ifdef DDR_PRIMARY_2GB_2RANK_X8_1GBX8_1600
	{ .pre_reset_function = ddr_2gb_2rank_x8_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_1gbx8_1600_phy_init	       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX8_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx8_1600_phy_init       },
#endif
#ifdef DDR_PRIMARY_2GB_1RANK_X16_1GBX8_MULTI_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_multi_1600_pre_reset_init, .phy_function = ddr_2gb_1rank_x16_1gbx8_multi_1600_phy_init },
#endif
#ifdef DDR_PRIMARY_2GB_2RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_2gb_2rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_2gbx8_1600_phy_init	       },
#endif

#ifdef DDR_SECONDARY_4GB_2RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_4gb_2rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_SECONDARY_4GB_2RANK_X16_1GBX16_3200
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx16_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx16_3200_phy_init      },
#endif
#ifdef DDR_SECONDARY_4GB_2RANK_X16_1GBX8_3200
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx8_3200_phy_init       },
#endif
#ifdef DDR_SECONDARY_4GB_1RANK_X16_2GBX8_3200
	{ .pre_reset_function = ddr_4gb_1rank_x16_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_4gb_1rank_x16_2gbx8_3200_phy_init       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX16_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx16_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx16_3200_phy_init      },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_2gb_1rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_SECONDARY_2GB_2RANK_X8_1GBX8_3200
	{ .pre_reset_function = ddr_2gb_2rank_x8_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_1gbx8_3200_phy_init	       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX8_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx8_3200_phy_init       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX8_MULTI_3200
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_multi_3200_pre_reset_init, .phy_function = ddr_2gb_1rank_x16_1gbx8_multi_3200_phy_init },
#endif
#ifdef DDR_SECONDARY_2GB_2RANK_X8_2GBX8_3200
	{ .pre_reset_function = ddr_2gb_2rank_x8_2gbx8_3200_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_2gbx8_3200_phy_init	       },
#endif
#ifdef DDR_SECONDARY_4GB_2RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_4gb_2rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x8_2gbx8_1600_phy_init	       },
#endif
#ifdef DDR_SECONDARY_4GB_2RANK_X16_1GBX16_1600
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx16_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx16_1600_phy_init      },
#endif
#ifdef DDR_SECONDARY_4GB_2RANK_X16_1GBX8_1600
	{ .pre_reset_function = ddr_4gb_2rank_x16_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_2rank_x16_1gbx8_1600_phy_init       },
#endif
#ifdef DDR_SECONDARY_4GB_1RANK_X16_2GBX8_1600
	{ .pre_reset_function = ddr_4gb_1rank_x16_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_4gb_1rank_x16_2gbx8_1600_phy_init       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX16_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx16_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx16_1600_phy_init      },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_2gb_1rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x8_2gbx8_1600_phy_init	       },
#endif
#ifdef DDR_SECONDARY_2GB_2RANK_X8_1GBX8_1600
	{ .pre_reset_function = ddr_2gb_2rank_x8_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_1gbx8_1600_phy_init	       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX8_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_1rank_x16_1gbx8_1600_phy_init       },
#endif
#ifdef DDR_SECONDARY_2GB_1RANK_X16_1GBX8_MULTI_1600
	{ .pre_reset_function = ddr_2gb_1rank_x16_1gbx8_multi_1600_pre_reset_init, .phy_function = ddr_2gb_1rank_x16_1gbx8_multi_1600_phy_init },
#endif
#ifdef DDR_SECONDARY_2GB_2RANK_X8_2GBX8_1600
	{ .pre_reset_function = ddr_2gb_2rank_x8_2gbx8_1600_pre_reset_init,	   .phy_function = ddr_2gb_2rank_x8_2gbx8_1600_phy_init	       },
#endif
};
