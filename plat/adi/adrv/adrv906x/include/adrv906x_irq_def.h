/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ADRV906X_IRQ_DEF_H__
#define __ADI_ADRV906X_IRQ_DEF_H__

#define IRQ_L4_SCBINIT_INTR_0           32
#define IRQ_L4_ECC_WRN_INTR_0           33
#define IRQ_L4_ECC_ERR_INTR_0           34
#define IRQ_L4_SCBINIT_INTR_1           35
#define IRQ_L4_ECC_WRN_INTR_1           36
#define IRQ_L4_ECC_ERR_INTR_1           37
#define IRQ_L4_SCBINIT_INTR_2           38
#define IRQ_L4_ECC_WRN_INTR_2           39
#define IRQ_L4_ECC_ERR_INTR_2           40
#define IRQ_L4CTL_0_SWU_INTR            41
#define IRQ_L4CTL_1_SWU_INTR            42
#define IRQ_L4CTL_2_SWU_INTR            43
#define IRQ_MDMA_CH0_DONE_INTR_0                44
#define IRQ_MDMA_CH0_ERR_INTR_0         45
#define IRQ_MDMA_CH1_DONE_INTR_0                46
#define IRQ_MDMA_CH1_ERR_INTR_0         47
#define IRQ_MDMA_CH0_DONE_INTR_1                48
#define IRQ_MDMA_CH0_ERR_INTR_1         49
#define IRQ_MDMA_CH1_DONE_INTR_1                50
#define IRQ_MDMA_CH1_ERR_INTR_1         51
#define IRQ_MDMA_CH0_DONE_INTR_2                52
#define IRQ_MDMA_CH0_ERR_INTR_2         53
#define IRQ_MDMA_CH1_DONE_INTR_2                54
#define IRQ_MDMA_CH1_ERR_INTR_2         55
#define IRQ_MDMA_CH0_DONE_INTR_3                56
#define IRQ_MDMA_CH0_ERR_INTR_3         57
#define IRQ_MDMA_CH1_DONE_INTR_3                58
#define IRQ_MDMA_CH1_ERR_INTR_3         59
#define IRQ_GPT_A55_IRQ_PIPED_0         60
#define IRQ_GPT_A55_IRQ_PIPED_1         61
#define IRQ_GPT_A55_IRQ_PIPED_2         62
#define IRQ_GPT_A55_IRQ_PIPED_3         63
#define IRQ_GPT_A55_IRQ_PIPED_4         64
#define IRQ_GPT_A55_IRQ_PIPED_5         65
#define IRQ_GPT_A55_IRQ_PIPED_6         66
#define IRQ_GPT_A55_IRQ_PIPED_7         67
#define IRQ_WATCHDOG_A55_TIMEOUT_PIPED_0                68
#define IRQ_WATCHDOG_A55_TIMEOUT_PIPED_1                69
#define IRQ_WATCHDOG_A55_TIMEOUT_PIPED_2                70
#define IRQ_WATCHDOG_A55_TIMEOUT_PIPED_3                71
#define IRQ_A55_PERI_MAILBOX_INTERRUPT_PIPED_2          72
#define IRQ_A55_PERI_MAILBOX_INTERRUPT_PIPED_3          73
#define IRQ_CBDDE_DONE_INTR_0           74
#define IRQ_CBDDE_ERR_INTR_0            75
#define IRQ_RUE_IRQ             76
#define IRQ_OIF_IRQ             77
#define IRQ_CB_DONE_INTR_0              78
#define IRQ_QSFP_INTERRUPT              79
#define IRQ_XCORR_DONE_INT_PIPED                80
#define IRQ_MB_SPI0_TO_A55              81
#define IRQ_ALT_A55_AHB_ERROR_INDICATION_PIPED          82
#define IRQ_NCNTHPIRQ_0         26
#define IRQ_NCNTPNSIRQ_0                30
#define IRQ_NCNTPSIRQ_0         29
#define IRQ_NCNTVIRQ_0          27
#define IRQ_NCNTHVIRQ_0         28
#define IRQ_NCNTHPIRQ_1         26
#define IRQ_NCNTPNSIRQ_1                30
#define IRQ_NCNTPSIRQ_1         29
#define IRQ_NCNTVIRQ_1          27
#define IRQ_NCNTHVIRQ_1         28
#define IRQ_NCNTHPIRQ_2         26
#define IRQ_NCNTPNSIRQ_2                30
#define IRQ_NCNTPSIRQ_2         29
#define IRQ_NCNTVIRQ_2          27
#define IRQ_NCNTHVIRQ_2         28
#define IRQ_NCNTHPIRQ_3         26
#define IRQ_NCNTPNSIRQ_3                30
#define IRQ_NCNTPSIRQ_3         29
#define IRQ_NCNTVIRQ_3          27
#define IRQ_NCNTHVIRQ_3         28
#define IRQ_NVCPUMNTIRQ_0               25
#define IRQ_NVCPUMNTIRQ_1               25
#define IRQ_NVCPUMNTIRQ_2               25
#define IRQ_NVCPUMNTIRQ_3               25
#define IRQ_NPMUIRQ_0           23
#define IRQ_NPMUIRQ_1           23
#define IRQ_NPMUIRQ_2           23
#define IRQ_NPMUIRQ_3           23
#define IRQ_NCLUSTERPMUIRQ              83
#define IRQ_GIC_PMU_INT         84
#define IRQ_POSTED_HRESP_LVL_A55_PIPED          85
#define IRQ_FF_FEATURE_DONE_PIPED               86
#define IRQ_FF_PROGRAM_DONE_PIPED               87
#define IRQ_XCORR_ECC_ERROR_IRQ_PIPED           88
#define IRQ_XCORR_ECC_ERROR_WARNING_PIPED               89
#define IRQ_XCORR_DATA_REQUEST_INT_PIPED                90
#define IRQ_TEMP_SENSOR_INT0            91
#define IRQ_TEMP_SENSOR_INT1            92
#define IRQ_RFFE_0_INT_SYNC             93
#define IRQ_RFFE_1_INT_SYNC             94
#define IRQ_DYINGGASPDETECTION_POWERCONTROL             95
#define IRQ_GPIO_TO_GIC_SYNC_0          96
#define IRQ_GPIO_TO_GIC_SYNC_1          97
#define IRQ_GPIO_TO_GIC_SYNC_2          98
#define IRQ_GPIO_TO_GIC_SYNC_3          99
#define IRQ_GPIO_TO_GIC_SYNC_4          100
#define IRQ_GPIO_TO_GIC_SYNC_5          101
#define IRQ_GPIO_TO_GIC_SYNC_6          102
#define IRQ_GPIO_TO_GIC_SYNC_7          103
#define IRQ_GPIO_TO_GIC_SYNC_8          104
#define IRQ_GPIO_TO_GIC_SYNC_9          105
#define IRQ_GPIO_TO_GIC_SYNC_10         106
#define IRQ_GPIO_TO_GIC_SYNC_11         107
#define IRQ_GPIO_TO_GIC_SYNC_12         108
#define IRQ_GPIO_TO_GIC_SYNC_13         109
#define IRQ_GPIO_TO_GIC_SYNC_14         110
#define IRQ_GPIO_TO_GIC_SYNC_15         111
#define IRQ_GPIO_TO_GIC_SYNC_16         112
#define IRQ_GPIO_TO_GIC_SYNC_17         113
#define IRQ_GPIO_TO_GIC_SYNC_18         114
#define IRQ_GPIO_TO_GIC_SYNC_19         115
#define IRQ_GPIO_TO_GIC_SYNC_20         116
#define IRQ_GPIO_TO_GIC_SYNC_21         117
#define IRQ_GPIO_TO_GIC_SYNC_22         118
#define IRQ_GPIO_TO_GIC_SYNC_23         119
#define IRQ_PIMC0_EXT_IRQ_0             120
#define IRQ_PIMC0_EXT_IRQ_1             121
#define IRQ_NCOMMIRQ_0          22
#define IRQ_NCOMMIRQ_1          22
#define IRQ_NCOMMIRQ_2          22
#define IRQ_NCOMMIRQ_3          22
#define IRQ_CTIIRQ_0            24
#define IRQ_CTIIRQ_1            24
#define IRQ_CTIIRQ_2            24
#define IRQ_CTIIRQ_3            24
#define IRQ_PIMC1_EXT_IRQ_0             122
#define IRQ_PIMC1_EXT_IRQ_1             123
#define IRQ_A55_TRU_INTR0               124
#define IRQ_A55_TRU_INTR1               125
#define IRQ_A55_TRU_INTR2               126
#define IRQ_A55_TRU_INTR3               127
#define IRQ_O_PDS_TX0_ARM_IRQ_0         128
#define IRQ_O_PDS_TX0_ARM_IRQ_1         129
#define IRQ_O_PDS_TX0_ARM_IRQ_2         130
#define IRQ_O_PDS_TX0_ARM_IRQ_3         131
#define IRQ_O_PDS_TX0_ARM_IRQ_4         132
#define IRQ_O_PDS_TX0_ARM_IRQ_5         133
#define IRQ_O_PDS_TX0_ARM_IRQ_6         134
#define IRQ_O_PDS_TX0_ARM_IRQ_7         135
#define IRQ_O_PDS_TX1_ARM_IRQ_0         136
#define IRQ_O_PDS_TX1_ARM_IRQ_1         137
#define IRQ_O_PDS_TX1_ARM_IRQ_2         138
#define IRQ_O_PDS_TX1_ARM_IRQ_3         139
#define IRQ_O_PDS_TX1_ARM_IRQ_4         140
#define IRQ_O_PDS_TX1_ARM_IRQ_5         141
#define IRQ_O_PDS_TX1_ARM_IRQ_6         142
#define IRQ_O_PDS_TX1_ARM_IRQ_7         143
#define IRQ_O_PDS_TX2_ARM_IRQ_0         144
#define IRQ_O_PDS_TX2_ARM_IRQ_1         145
#define IRQ_O_PDS_TX2_ARM_IRQ_2         146
#define IRQ_O_PDS_TX2_ARM_IRQ_3         147
#define IRQ_O_PDS_TX2_ARM_IRQ_4         148
#define IRQ_O_PDS_TX2_ARM_IRQ_5         149
#define IRQ_O_PDS_TX2_ARM_IRQ_6         150
#define IRQ_O_PDS_TX2_ARM_IRQ_7         151
#define IRQ_O_PDS_TX3_ARM_IRQ_0         152
#define IRQ_O_PDS_TX3_ARM_IRQ_1         153
#define IRQ_O_PDS_TX3_ARM_IRQ_2         154
#define IRQ_O_PDS_TX3_ARM_IRQ_3         155
#define IRQ_O_PDS_TX3_ARM_IRQ_4         156
#define IRQ_O_PDS_TX3_ARM_IRQ_5         157
#define IRQ_O_PDS_TX3_ARM_IRQ_6         158
#define IRQ_O_PDS_TX3_ARM_IRQ_7         159
#define IRQ_RFFE_2_INT_SYNC             160
#define IRQ_RFFE_3_INT_SYNC             161
#define IRQ_W_SD_INTR_PIPED             162
#define IRQ_W_SD_WAKEUP_INTR_PIPED              163
#define IRQ_PIMC2_EXT_IRQ_0             164
#define IRQ_PIMC2_EXT_IRQ_1             165
#define IRQ_PIMC3_EXT_IRQ_0             166
#define IRQ_PIMC3_EXT_IRQ_1             167
#define IRQ_ANT_CAL_INTRPT              168
#define IRQ_DDR_EVENT_COUNT_OVERFLOW_OR_HW_EN_EXPIRY_INTR               169
#define IRQ_O_DFI_INTERNAL_ERR_INTR             170
#define IRQ_O_DFI_PHYUPD_ERR_INTR               171
#define IRQ_O_DFI_ALERT_ERR_INTR                172
#define IRQ_O_ECC_AP_ERR_INTR           173
#define IRQ_O_ECC_AP_ERR_INTR_FAULT             174
#define IRQ_ECC_CORRECTED_ERR_INTR              175
#define IRQ_ECC_CORRECTED_ERR_INTR_FAULT                176
#define IRQ_O_ECC_UNCORRECTED_ERR_INTR          177
#define IRQ_O_ECC_UNCORRECTED_ERR_INTR_FAULT            178
#define IRQ_SBR_DONE_INTR               179
#define IRQ_O_DWC_DDRPHY_INT_N          180
#define IRQ_I2C_IRQ_S2F_PIPED_0         181
#define IRQ_I2C_IRQ_S2F_PIPED_1         182
#define IRQ_I2C_IRQ_S2F_PIPED_2         183
#define IRQ_I2C_IRQ_S2F_PIPED_3         184
#define IRQ_I2C_IRQ_S2F_PIPED_4         185
#define IRQ_I2C_IRQ_S2F_PIPED_5         186
#define IRQ_I2C_IRQ_S2F_PIPED_6         187
#define IRQ_I2C_IRQ_S2F_PIPED_7         188
#define IRQ_MACSEC_IRQ_0                189
#define IRQ_MACSEC_IRQ_1                190
#define IRQ_RADIO_CONTROL_INTRPT                191
#define IRQ_O_PDS_RX0_ARM_IRQ_0         192
#define IRQ_O_PDS_RX0_ARM_IRQ_1         193
#define IRQ_O_PDS_RX0_ARM_IRQ_2         194
#define IRQ_O_PDS_RX0_ARM_IRQ_3         195
#define IRQ_O_PDS_RX0_ARM_IRQ_4         196
#define IRQ_O_PDS_RX0_ARM_IRQ_5         197
#define IRQ_O_PDS_RX0_ARM_IRQ_6         198
#define IRQ_O_PDS_RX0_ARM_IRQ_7         199
#define IRQ_O_PDS_RX1_ARM_IRQ_0         200
#define IRQ_O_PDS_RX1_ARM_IRQ_1         201
#define IRQ_O_PDS_RX1_ARM_IRQ_2         202
#define IRQ_O_PDS_RX1_ARM_IRQ_3         203
#define IRQ_O_PDS_RX1_ARM_IRQ_4         204
#define IRQ_O_PDS_RX1_ARM_IRQ_5         205
#define IRQ_O_PDS_RX1_ARM_IRQ_6         206
#define IRQ_O_PDS_RX1_ARM_IRQ_7         207
#define IRQ_O_PDS_RX2_ARM_IRQ_0         208
#define IRQ_O_PDS_RX2_ARM_IRQ_1         209
#define IRQ_O_PDS_RX2_ARM_IRQ_2         210
#define IRQ_O_PDS_RX2_ARM_IRQ_3         211
#define IRQ_O_PDS_RX2_ARM_IRQ_4         212
#define IRQ_O_PDS_RX2_ARM_IRQ_5         213
#define IRQ_O_PDS_RX2_ARM_IRQ_6         214
#define IRQ_O_PDS_RX2_ARM_IRQ_7         215
#define IRQ_O_PDS_RX3_ARM_IRQ_0         216
#define IRQ_O_PDS_RX3_ARM_IRQ_1         217
#define IRQ_O_PDS_RX3_ARM_IRQ_2         218
#define IRQ_O_PDS_RX3_ARM_IRQ_3         219
#define IRQ_O_PDS_RX3_ARM_IRQ_4         220
#define IRQ_O_PDS_RX3_ARM_IRQ_5         221
#define IRQ_O_PDS_RX3_ARM_IRQ_6         222
#define IRQ_O_PDS_RX3_ARM_IRQ_7         223
#define IRQ_NFAULTIRQ_0         224
#define IRQ_NFAULTIRQ_1         225
#define IRQ_NFAULTIRQ_2         226
#define IRQ_NFAULTIRQ_3         227
#define IRQ_NFAULTIRQ_4         228
#define IRQ_C2C_SWU_INTR                229
#define IRQ_ETH_IRQ_TX_TIMESTAMP_0              230
#define IRQ_ETH_IRQ_TX_TIMESTAMP_1              231
#define IRQ_MMI_SWU_INTR                232
#define IRQ_TOD_IRQ             233
#define IRQ_A55_PERI_SWU_INTR_PIPED             235
#define IRQ_NERRIRQ_0           236
#define IRQ_NERRIRQ_1           237
#define IRQ_NERRIRQ_2           238
#define IRQ_NERRIRQ_3           239
#define IRQ_NERRIRQ_4           240
#define IRQ_TELE_TS_OVERFLOW_INTERRUPT          241
#define IRQ_MS_DDE_ERR_INTR_GATED_0             242
#define IRQ_MS_DDE_ERR_INTR_GATED_1             243
#define IRQ_MS_DDE_ERR_INTR_GATED_2             244
#define IRQ_MS_DDE_ERR_INTR_GATED_3             245
#define IRQ_MS_DDE_DONE_INTR_GATED_0            246
#define IRQ_MS_DDE_DONE_INTR_GATED_1            247
#define IRQ_MS_DDE_DONE_INTR_GATED_2            248
#define IRQ_MS_DDE_DONE_INTR_GATED_3            249
#define IRQ_MS_STAT_DDE_ERR_INTR_GATED_0                250
#define IRQ_MS_STAT_DDE_ERR_INTR_GATED_1                251
#define IRQ_DEBUG_DDE_ERR_INTR_0                252
#define IRQ_DEBUG_DDE_ERR_INTR_1                253
#define IRQ_MS_STAT_DDE_DONE_INTR_GATED_0               254
#define IRQ_MS_STAT_DDE_DONE_INTR_GATED_1               255
#define IRQ_O_PDS_ORX0_ARM_IRQ_0                256
#define IRQ_O_PDS_ORX0_ARM_IRQ_1                257
#define IRQ_O_PDS_ORX0_ARM_IRQ_2                258
#define IRQ_O_PDS_ORX0_ARM_IRQ_3                259
#define IRQ_O_PDS_ORX0_ARM_IRQ_4                260
#define IRQ_O_PDS_ORX0_ARM_IRQ_5                261
#define IRQ_O_PDS_ORX0_ARM_IRQ_6                262
#define IRQ_O_PDS_ORX0_ARM_IRQ_7                263
#define IRQ_DEBUG_DDE_DONE_INTR_0               264
#define IRQ_DEBUG_DDE_DONE_INTR_1               265
#define IRQ_IRQ_SPI_QUAD_RX_DDEERR_PIPED                266
#define IRQ_IRQ_SPI_QUAD_TX_DDEERR_PIPED                267
#define IRQ_IRQ_SPI_QUAD_TX_PIPED               268
#define IRQ_IRQ_SPI_QUAD_RX_PIPED               269
#define IRQ_IRQ_SPI_QUAD_ERR_PIPED              270
#define IRQ_IRQ_SPI_QUAD_STAT_PIPED             271
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_0               272
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_1               273
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_2               274
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_3               275
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_4               276
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_5               277
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_6               278
#define IRQ_I_STREAM_PROC_INTERRUPT_ARM_7               279
#define IRQ_I_PDS_TX0_INTR_IRQ_1                280
#define IRQ_I_PDS_TX1_INTR_IRQ_1                281
#define IRQ_I_PDS_TX2_INTR_IRQ_1                282
#define IRQ_I_PDS_TX3_INTR_IRQ_1                283
#define IRQ_NIC_DMA_RX_STATUS_INTR_GATED_0              284
#define IRQ_NIC_DMA_RX_STATUS_INTR_GATED_1              285
#define IRQ_NIC_DMA_RX_ERR_INTR_GATED_0         286
#define IRQ_NIC_DMA_RX_ERR_INTR_GATED_1         287
#define IRQ_TX0_DFE_IRQ_0               288
#define IRQ_TX0_DFE_IRQ_1               289
#define IRQ_TX0_DFE_IRQ_2               290
#define IRQ_TX0_DFE_IRQ_3               291
#define IRQ_TX0_DFE_IRQ_4               292
#define IRQ_TX0_DFE_IRQ_5               293
#define IRQ_TX0_DFE_IRQ_6               294
#define IRQ_TX0_DFE_IRQ_7               295
#define IRQ_TX0_DFE_IRQ_8               296
#define IRQ_TX1_DFE_IRQ_0               297
#define IRQ_TX1_DFE_IRQ_1               298
#define IRQ_TX1_DFE_IRQ_2               299
#define IRQ_TX1_DFE_IRQ_3               300
#define IRQ_TX1_DFE_IRQ_4               301
#define IRQ_TX1_DFE_IRQ_5               302
#define IRQ_TX1_DFE_IRQ_6               303
#define IRQ_TX1_DFE_IRQ_7               304
#define IRQ_TX1_DFE_IRQ_8               305
#define IRQ_EAST_RFPLL_PLL_LOCKED_SYNC          306
#define IRQ_WEST_RFPLL_PLL_LOCKED_SYNC          307
#define IRQ_CLKPLL_PLL_LOCKED_SYNC              308
#define IRQ_DDR_CL_SWU_INTR             310
#define IRQ_DDR_DL_SWU_INTR             311
#define IRQ_INJECT_DBG_ERROR            312
#define IRQ_INJECT_DBG_STAT             313
#define IRQ_I_M4_ARM_AHB_ERROR_INDICATION_0             314
#define IRQ_I_M4_ARM_AHB_ERROR_INDICATION_1             315
#define IRQ_DEBUG_DDE_DONE_INTR_2               316
#define IRQ_DEBUG_DDE_ERR_INTR_2                317
#define IRQ_ANTENNA_CAL_DDE_ERR_INTR_0          318
#define IRQ_ANTENNA_CAL_DDE_DONE_INTR_0         319
#define IRQ_MDMA_CH0_DONE_INTR_4                320
#define IRQ_MDMA_CH0_ERR_INTR_4         321
#define IRQ_MDMA_CH1_DONE_INTR_4                322
#define IRQ_MDMA_CH1_ERR_INTR_4         323
#define IRQ_MDMA_CH0_DONE_INTR_5                324
#define IRQ_MDMA_CH0_ERR_INTR_5         325
#define IRQ_MDMA_CH1_DONE_INTR_5                326
#define IRQ_MDMA_CH1_ERR_INTR_5         327
#define IRQ_C2C_NON_CRIT_INTR           328
#define IRQ_C2C_CRIT_INTR               329
#define IRQ_EMAC_1G_SBD_INTR_PIPED              330
#define IRQ_EMAC_1G_SBD_PERCH_TX_INTR_PIPED             331
#define IRQ_EMAC_1G_SBD_PERCH_RX_INTR_PIPED             332
#define IRQ_CLOCK_STATUS_IN_0           333
#define IRQ_CLOCK_STATUS_IN_1           334
#define IRQ_SPI_REG_MAIN_STREAMPROC_ERROR_STATUS                335
#define IRQ_RS_TO_A55_GIC_TRU_0         336
#define IRQ_RS_TO_A55_GIC_TRU_1         337
#define IRQ_RS_TO_A55_GIC_TRU_2         338
#define IRQ_RS_TO_A55_GIC_TRU_3         339
#define IRQ_RS_TO_A55_GIC_TRU_4         340
#define IRQ_RS_TO_A55_GIC_TRU_5         341
#define IRQ_RS_TO_A55_GIC_TRU_6         342
#define IRQ_RS_TO_A55_GIC_TRU_7         343
#define IRQ_RS_TO_A55_GIC_TRU_8         344
#define IRQ_RS_TO_A55_GIC_TRU_9         345
#define IRQ_RS_TO_A55_GIC_TRU_10                346
#define IRQ_RS_TO_A55_GIC_TRU_11                347
#define IRQ_RS_TO_A55_GIC_TRU_12                348
#define IRQ_RS_TO_A55_GIC_TRU_13                349
#define IRQ_RS_TO_A55_GIC_TRU_14                350
#define IRQ_RS_TO_A55_GIC_TRU_15                351
#define IRQ_RS_TO_A55_GIC_TRU_16                352
#define IRQ_RS_TO_A55_GIC_TRU_17                353
#define IRQ_RS_TO_A55_GIC_TRU_18                354
#define IRQ_RS_TO_A55_GIC_TRU_19                355
#define IRQ_RS_TO_A55_GIC_TRU_20                356
#define IRQ_RS_TO_A55_GIC_TRU_21                357
#define IRQ_RS_TO_A55_GIC_TRU_22                358
#define IRQ_RS_TO_A55_GIC_TRU_23                359
#define IRQ_RS_TO_A55_GIC_TRU_24                360
#define IRQ_RS_TO_A55_GIC_TRU_25                361
#define IRQ_RS_TO_A55_GIC_TRU_26                362
#define IRQ_RS_TO_A55_GIC_TRU_27                363
#define IRQ_RS_TO_A55_GIC_TRU_28                364
#define IRQ_RS_TO_A55_GIC_TRU_29                365
#define IRQ_RS_TO_A55_GIC_TRU_30                366
#define IRQ_RS_TO_A55_GIC_TRU_31                367
#define IRQ_O_PDS_ORX0_INTR_IRQ_3               368
#define IRQ_IRQ_SPI_1_RX_DDEERR_PIPED           369
#define IRQ_IRQ_SPI_1_TX_DDEERR_PIPED           370
#define IRQ_IRQ_SPI_1_TX_PIPED          371
#define IRQ_IRQ_SPI_1_RX_PIPED          372
#define IRQ_IRQ_SPI_1_ERR_PIPED         373
#define IRQ_IRQ_SPI_1_STAT_PIPED                374
#define IRQ_IRQ_SPI_2_RX_DDEERR_PIPED           375
#define IRQ_IRQ_SPI_2_TX_DDEERR_PIPED           376
#define IRQ_IRQ_SPI_2_TX_PIPED          377
#define IRQ_IRQ_SPI_2_RX_PIPED          378
#define IRQ_IRQ_SPI_2_ERR_PIPED         379
#define IRQ_IRQ_SPI_2_STAT_PIPED                380
#define IRQ_IRQ_SPI_3_RX_DDEERR_PIPED           381
#define IRQ_IRQ_SPI_3_TX_DDEERR_PIPED           382
#define IRQ_IRQ_SPI_3_TX_PIPED          383
#define IRQ_IRQ_SPI_3_RX_PIPED          384
#define IRQ_IRQ_SPI_3_ERR_PIPED         385
#define IRQ_IRQ_SPI_3_STAT_PIPED                386
#define IRQ_IRQ_SPI_4_RX_DDEERR_PIPED           387
#define IRQ_IRQ_SPI_4_TX_DDEERR_PIPED           388
#define IRQ_IRQ_SPI_4_TX_PIPED          389
#define IRQ_IRQ_SPI_4_RX_PIPED          390
#define IRQ_IRQ_SPI_4_ERR_PIPED         391
#define IRQ_IRQ_SPI_4_STAT_PIPED                392
#define IRQ_IRQ_SPI_5_RX_DDEERR_PIPED           393
#define IRQ_IRQ_SPI_5_TX_DDEERR_PIPED           394
#define IRQ_IRQ_SPI_5_TX_PIPED          395
#define IRQ_IRQ_SPI_5_RX_PIPED          396
#define IRQ_IRQ_SPI_5_ERR_PIPED         397
#define IRQ_IRQ_SPI_5_STAT_PIPED                398
#define IRQ_IRQ_SPI_6_RX_DDEERR_PIPED           399
#define IRQ_IRQ_SPI_6_TX_DDEERR_PIPED           400
#define IRQ_IRQ_SPI_6_TX_PIPED          401
#define IRQ_IRQ_SPI_6_RX_PIPED          402
#define IRQ_IRQ_SPI_6_ERR_PIPED         403
#define IRQ_IRQ_SPI_6_STAT_PIPED                404
#define IRQ_W_INTR_PIPED                405
#define IRQ_W_WAKEUP_INTR_PIPED         406
#define IRQ_GNSS_INTERRUPT              407
#define IRQ_NIC_DMA_TX_STATUS_INTR_GATED_0              408
#define IRQ_NIC_DMA_TX_STATUS_INTR_GATED_1              409
#define IRQ_NIC_DMA_TX_ERR_INTR_GATED_0         410
#define IRQ_NIC_DMA_TX_ERR_INTR_GATED_1         411
#define IRQ_GP_INTERRUPT_SYNC_0         412
#define IRQ_GP_INTERRUPT_SYNC_1         413
#define IRQ_SPU_IRQ0            414
#define IRQ_SPU_IRQ1            415
#define IRQ_SPU_IRQ2            416
#define IRQ_SPU_IRQ3            417
#define IRQ_SPU_IRQ4            418
#define IRQ_TE_HREQ_ACK_IRQ_PIPED               419
#define IRQ_TE_ERESP_RDY_IRQ_PIPED              420
#define IRQ_TE_FAULT_GP_INTR_PIPED              421
#define IRQ_TE_H_HINF_IRQ_PIPED         422
#define IRQ_O_DL_ANT_CAL_CAP_DONE_INTRPT                423
#define IRQ_O_DL_ANT_CAL_DDE_DONE_INTRPT                424
#define IRQ_O_UL_ANT_CAL_CAP_DONE_INTRPT                425
#define IRQ_O_UL_ANT_CAL_DDE_DONE_INTRPT                426
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_0          427
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_1          428
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_2          429
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_3          430
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_4          431
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_5          432
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_6          433
#define IRQ_O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_7          434
#define IRQ_O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_0          435
#define IRQ_O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_1          436
#define IRQ_O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_2          437
#define IRQ_O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_3          438
#define IRQ_GPIO_TO_GIC_SYNC_24         439
#define IRQ_GPIO_TO_GIC_SYNC_25         440
#define IRQ_GPIO_TO_GIC_SYNC_26         441
#define IRQ_GPIO_TO_GIC_SYNC_27         442
#define IRQ_GPIO_TO_GIC_SYNC_28         443
#define IRQ_GPIO_TO_GIC_SYNC_29         444
#define IRQ_GPIO_TO_GIC_SYNC_30         445
#define IRQ_GPIO_TO_GIC_SYNC_31         446
#define IRQ_C2C_OUT_HW_INTERRUPT_16             447             /* L4_SCBINIT_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_17             448             /* L4_ECC_WRN_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_18             449             /* L4_ECC_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_19             450             /* L4_SCBINIT_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_20             451             /* L4_ECC_WRN_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_21             452             /* L4_ECC_ERR_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_22             453             /* L4_SCBINIT_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_23             454             /* L4_ECC_WRN_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_24             455             /* L4_ECC_ERR_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_25             456             /* MDMA_CH0_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_26             457             /* MDMA_CH0_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_27             458             /* MDMA_CH1_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_28             459             /* MDMA_CH1_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_29             460             /* MDMA_CH0_DONE_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_30             461             /* MDMA_CH0_ERR_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_31             462             /* MDMA_CH1_DONE_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_32             463             /* MDMA_CH1_ERR_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_33             464             /* MDMA_CH0_DONE_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_34             465             /* MDMA_CH0_ERR_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_35             466             /* MDMA_CH1_DONE_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_36             467             /* MDMA_CH1_ERR_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_37             468             /* MDMA_CH0_DONE_INTR_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_38             469             /* MDMA_CH0_ERR_INTR_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_39             470             /* MDMA_CH1_DONE_INTR_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_40             471             /* MDMA_CH1_ERR_INTR_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_41             472             /* A55_PERI_MAILBOX_INTERRUPT_PIPED_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_42             473             /* A55_PERI_MAILBOX_INTERRUPT_PIPED_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_43             474             /* CBDDE_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_44             475             /* CBDDE_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_45             476             /* RUE_IRQ */
#define IRQ_C2C_OUT_HW_INTERRUPT_46             477             /* OIF_IRQ */
#define IRQ_C2C_OUT_HW_INTERRUPT_47             478             /* CB_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_48             479             /* QSFP_INTERRUPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_49             480             /* ALT_A55_AHB_ERROR_INDICATION_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_50             481             /* POSTED_HRESP_LVL_A55_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_51             482             /* TEMP_SENSOR_INT0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_52             483             /* TEMP_SENSOR_INT1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_53             484             /* RFFE_0_INT_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_54             485             /* RFFE_1_INT_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_55             486             /* GPIO_TO_GIC_SYNC_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_56             487             /* GPIO_TO_GIC_SYNC_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_57             488             /* GPIO_TO_GIC_SYNC_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_58             489             /* GPIO_TO_GIC_SYNC_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_59             490             /* GPIO_TO_GIC_SYNC_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_60             491             /* GPIO_TO_GIC_SYNC_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_61             492             /* GPIO_TO_GIC_SYNC_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_62             493             /* GPIO_TO_GIC_SYNC_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_63             494             /* GPIO_TO_GIC_SYNC_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_64             495             /* GPIO_TO_GIC_SYNC_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_65             496             /* GPIO_TO_GIC_SYNC_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_66             497             /* GPIO_TO_GIC_SYNC_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_67             498             /* GPIO_TO_GIC_SYNC_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_68             499             /* GPIO_TO_GIC_SYNC_13 */
#define IRQ_C2C_OUT_HW_INTERRUPT_69             500             /* GPIO_TO_GIC_SYNC_14 */
#define IRQ_C2C_OUT_HW_INTERRUPT_70             501             /* GPIO_TO_GIC_SYNC_15 */
#define IRQ_C2C_OUT_HW_INTERRUPT_71             502             /* GPIO_TO_GIC_SYNC_16 */
#define IRQ_C2C_OUT_HW_INTERRUPT_72             503             /* GPIO_TO_GIC_SYNC_17 */
#define IRQ_C2C_OUT_HW_INTERRUPT_73             504             /* GPIO_TO_GIC_SYNC_18 */
#define IRQ_C2C_OUT_HW_INTERRUPT_74             505             /* GPIO_TO_GIC_SYNC_19 */
#define IRQ_C2C_OUT_HW_INTERRUPT_75             506             /* GPIO_TO_GIC_SYNC_20 */
#define IRQ_C2C_OUT_HW_INTERRUPT_76             507             /* GPIO_TO_GIC_SYNC_21 */
#define IRQ_C2C_OUT_HW_INTERRUPT_77             508             /* GPIO_TO_GIC_SYNC_22 */
#define IRQ_C2C_OUT_HW_INTERRUPT_78             509             /* GPIO_TO_GIC_SYNC_23 */
#define IRQ_C2C_OUT_HW_INTERRUPT_79             510             /* PIMC0_EXT_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_80             511             /* PIMC0_EXT_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_81             512             /* PIMC1_EXT_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_82             513             /* PIMC1_EXT_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_83             514             /* A55_TRU_INTR0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_84             515             /* A55_TRU_INTR1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_85             516             /* A55_TRU_INTR2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_86             517             /* A55_TRU_INTR3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_87             518             /* RFFE_2_INT_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_88             519             /* RFFE_3_INT_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_89             520             /* PIMC2_EXT_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_90             521             /* PIMC2_EXT_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_91             522             /* PIMC3_EXT_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_92             523             /* PIMC3_EXT_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_93             524             /* ANT_CAL_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_94             525             /* I2C_IRQ_S2F_PIPED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_95             526             /* I2C_IRQ_S2F_PIPED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_96             527             /* I2C_IRQ_S2F_PIPED_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_97             528             /* I2C_IRQ_S2F_PIPED_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_98             529             /* I2C_IRQ_S2F_PIPED_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_99             530             /* I2C_IRQ_S2F_PIPED_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_100            531             /* I2C_IRQ_S2F_PIPED_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_101            532             /* I2C_IRQ_S2F_PIPED_7 */
#define IRQ_PL011_UART_INTR_0           533
#define IRQ_PL011_UART_INTR_1           534
#define IRQ_PL011_UART_INTR_2           535
#define IRQ_PL011_UART_INTR_3           536
#define IRQ_SERDES_INTERRUPT_SYNC               537
#define IRQ_ETHPLL_LOCKED_SYNC          538
#define IRQ_ARM0_MEMORY_ECC_ERROR               539
#define IRQ_ARM1_MEMORY_ECC_ERROR               540
#define IRQ_ML_DPD_DONE_INTR_PIPED              541
#define IRQ_ML_DPD_ERR_INTR_PIPED               542
#define IRQ_C2C_OUT_HW_INTERRUPT_102            543             /* MACSEC_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_103            544             /* MACSEC_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_104            545             /* RADIO_CONTROL_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_105            546             /* ETH_IRQ_TX_TIMESTAMP_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_106            547             /* ETH_IRQ_TX_TIMESTAMP_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_107            548             /* TELE_TS_OVERFLOW_INTERRUPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_108            549             /* MS_DDE_ERR_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_109            550             /* MS_DDE_ERR_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_110            551             /* MS_DDE_ERR_INTR_GATED_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_111            552             /* MS_DDE_ERR_INTR_GATED_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_112            553             /* MS_DDE_DONE_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_113            554             /* MS_DDE_DONE_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_114            555             /* MS_DDE_DONE_INTR_GATED_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_115            556             /* MS_DDE_DONE_INTR_GATED_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_116            557             /* MS_STAT_DDE_ERR_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_117            558             /* MS_STAT_DDE_ERR_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_118            559             /* DEBUG_DDE_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_119            560             /* DEBUG_DDE_ERR_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_120            561             /* MS_STAT_DDE_DONE_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_121            562             /* MS_STAT_DDE_DONE_INTR_GATED_1 */
#define IRQ_TZCINT_DDR          563
#define IRQ_TZCINT_L4_0         564
#define IRQ_TZCINT_L4_1         565
#define IRQ_TZCINT_L4_2         566
#define IRQ_TX_ORX_MAP_CHANGE           567
#define IRQ_PIMC_DDE_DONE_INTR_0                568
#define IRQ_PIMC_DDE_ERR_INTR_0         569
#define IRQ_GPINT_INTERRUPT_SECONDARY_TO_PRIMARY                570
#define IRQ_RTC_INT             571
#define IRQ_C2C_OUT_HW_INTERRUPT_122            572             /* DEBUG_DDE_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_123            573             /* DEBUG_DDE_DONE_INTR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_124            574             /* I_STREAM_PROC_INTERRUPT_ARM_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_125            575             /* I_STREAM_PROC_INTERRUPT_ARM_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_126            576             /* I_STREAM_PROC_INTERRUPT_ARM_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_127            577             /* I_STREAM_PROC_INTERRUPT_ARM_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_128            578             /* I_STREAM_PROC_INTERRUPT_ARM_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_129            579             /* I_STREAM_PROC_INTERRUPT_ARM_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_130            580             /* I_STREAM_PROC_INTERRUPT_ARM_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_131            581             /* I_STREAM_PROC_INTERRUPT_ARM_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_132            582             /* MSP_RX_STATUS_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_133            583             /* MSP_RX_STATUS_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_134            584             /* MSP_RX_ERR_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_135            585             /* MSP_RX_ERR_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_136            586             /* TX0_DFE_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_137            587             /* TX0_DFE_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_138            588             /* TX0_DFE_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_139            589             /* TX0_DFE_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_140            590             /* TX0_DFE_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_141            591             /* TX0_DFE_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_142            592             /* TX0_DFE_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_143            593             /* TX0_DFE_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_144            594             /* TX0_DFE_IRQ_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_145            595             /* TX1_DFE_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_146            596             /* TX1_DFE_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_147            597             /* TX1_DFE_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_148            598             /* TX1_DFE_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_149            599             /* TX1_DFE_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_150            600             /* TX1_DFE_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_151            601             /* TX1_DFE_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_152            602             /* TX1_DFE_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_153            603             /* TX1_DFE_IRQ_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_154            604             /* EAST_RFPLL_PLL_LOCKED_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_155            605             /* WEST_RFPLL_PLL_LOCKED_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_156            606             /* CLKPLL_PLL_LOCKED_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_157            607             /* INJECT_DBG_ERROR */
#define IRQ_C2C_OUT_HW_INTERRUPT_158            608             /* INJECT_DBG_STAT */
#define IRQ_C2C_OUT_HW_INTERRUPT_159            609             /* DEBUG_DDE_DONE_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_160            610             /* DEBUG_DDE_ERR_INTR_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_161            611             /* ANTENNA_CAL_DDE_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_162            612             /* ANTENNA_CAL_DDE_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_163            613             /* MDMA_CH0_DONE_INTR_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_164            614             /* MDMA_CH0_ERR_INTR_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_165            615             /* MDMA_CH1_DONE_INTR_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_166            616             /* MDMA_CH1_ERR_INTR_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_167            617             /* MDMA_CH0_DONE_INTR_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_168            618             /* MDMA_CH0_ERR_INTR_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_169            619             /* MDMA_CH1_DONE_INTR_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_170            620             /* MDMA_CH1_ERR_INTR_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_171            621             /* C2C_NON_CRIT_INTR */
#define IRQ_C2C_OUT_HW_INTERRUPT_172            622             /* EMAC_1G_SBD_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_173            623             /* EMAC_1G_SBD_PERCH_TX_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_174            624             /* EMAC_1G_SBD_PERCH_RX_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_175            625             /* SPI_REG_MAIN_STREAMPROC_ERROR_STATUS */
#define IRQ_C2C_OUT_HW_INTERRUPT_176            626             /* RS_TO_A55_GIC_TRU_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_177            627             /* RS_TO_A55_GIC_TRU_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_178            628             /* RS_TO_A55_GIC_TRU_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_179            629             /* RS_TO_A55_GIC_TRU_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_180            630             /* RS_TO_A55_GIC_TRU_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_181            631             /* RS_TO_A55_GIC_TRU_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_182            632             /* RS_TO_A55_GIC_TRU_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_183            633             /* RS_TO_A55_GIC_TRU_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_184            634             /* RS_TO_A55_GIC_TRU_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_185            635             /* RS_TO_A55_GIC_TRU_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_186            636             /* RS_TO_A55_GIC_TRU_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_187            637             /* RS_TO_A55_GIC_TRU_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_188            638             /* RS_TO_A55_GIC_TRU_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_189            639             /* RS_TO_A55_GIC_TRU_13 */
#define IRQ_C2C_OUT_HW_INTERRUPT_190            640             /* RS_TO_A55_GIC_TRU_14 */
#define IRQ_C2C_OUT_HW_INTERRUPT_191            641             /* RS_TO_A55_GIC_TRU_15 */
#define IRQ_C2C_OUT_HW_INTERRUPT_192            642             /* O_PDS_ORX0_INTR_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_193            643             /* MSP_TX_STATUS_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_194            644             /* MSP_TX_STATUS_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_195            645             /* MSP_TX_ERR_INTR_GATED_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_196            646             /* MSP_TX_ERR_INTR_GATED_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_197            647             /* GP_INTERRUPT_SYNC_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_198            648             /* SPU_IRQ0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_199            649             /* SPU_IRQ1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_200            650             /* SPU_IRQ2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_201            651             /* SPU_IRQ3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_202            652             /* SPU_IRQ4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_203            653             /* TE_HREQ_ACK_IRQ_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_204            654             /* TE_ERESP_RDY_IRQ_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_205            655             /* TE_FAULT_GP_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_206            656             /* TE_H_HINF_IRQ_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_207            657             /* O_DL_ANT_CAL_CAP_DONE_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_208            658             /* O_DL_ANT_CAL_DDE_DONE_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_209            659             /* O_UL_ANT_CAL_CAP_DONE_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_210            660             /* O_UL_ANT_CAL_DDE_DONE_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_211            661             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_212            662             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_213            663             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_214            664             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_215            665             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_216            666             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_217            667             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_218            668             /* O_DL_ANT_CAL_DDE_ANT_DONE_INTRPT_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_219            669             /* O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_220            670             /* O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_221            671             /* O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_222            672             /* O_UL_ANT_CAL_DDE_ANT_DONE_INTRPT_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_223            673             /* GPIO_TO_GIC_SYNC_24 */
#define IRQ_C2C_OUT_HW_INTERRUPT_224            674             /* GPIO_TO_GIC_SYNC_25 */
#define IRQ_C2C_OUT_HW_INTERRUPT_225            675             /* GPIO_TO_GIC_SYNC_26 */
#define IRQ_C2C_OUT_HW_INTERRUPT_226            676             /* GPIO_TO_GIC_SYNC_27 */
#define IRQ_C2C_OUT_HW_INTERRUPT_227            677             /* GPIO_TO_GIC_SYNC_28 */
#define IRQ_C2C_OUT_HW_INTERRUPT_228            678             /* GPIO_TO_GIC_SYNC_29 */
#define IRQ_C2C_OUT_HW_INTERRUPT_229            679             /* GPIO_TO_GIC_SYNC_30 */
#define IRQ_C2C_OUT_HW_INTERRUPT_230            680             /* GPIO_TO_GIC_SYNC_31 */
#define IRQ_C2C_OUT_HW_INTERRUPT_231            681             /* ETHPLL_LOCKED_SYNC */
#define IRQ_C2C_OUT_HW_INTERRUPT_232            682             /* ARM0_MEMORY_ECC_ERROR */
#define IRQ_C2C_OUT_HW_INTERRUPT_233            683             /* ARM1_MEMORY_ECC_ERROR */
#define IRQ_C2C_OUT_HW_INTERRUPT_234            684             /* ML_DPD_DONE_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_235            685             /* ML_DPD_ERR_INTR_PIPED */
#define IRQ_C2C_OUT_HW_INTERRUPT_236            686             /* TZCINT_DDR */
#define IRQ_C2C_OUT_HW_INTERRUPT_237            687             /* TZCINT_L4_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_238            688             /* TZCINT_L4_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_239            689             /* TZCINT_L4_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_240            690             /* PIMC_DDE_DONE_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_241            691             /* PIMC_DDE_ERR_INTR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_242            692             /* V_UART_INTR_0_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_243            693             /* V_UART_INTR_1_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_244            694             /* I_ALT_M4_AHB_ERROR_INDICATION */
#define IRQ_C2C_OUT_HW_INTERRUPT_245            695             /* TX2_DFE_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_246            696             /* TX2_DFE_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_247            697             /* TX2_DFE_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_248            698             /* TX2_DFE_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_249            699             /* TX2_DFE_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_250            700             /* TX2_DFE_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_251            701             /* TX2_DFE_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_252            702             /* TX2_DFE_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_253            703             /* TX2_DFE_IRQ_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_254            704             /* TX3_DFE_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_255            705             /* TX3_DFE_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_256            706             /* TX3_DFE_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_257            707             /* TX3_DFE_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_258            708             /* TX3_DFE_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_259            709             /* TX3_DFE_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_260            710             /* TX3_DFE_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_261            711             /* TX3_DFE_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_262            712             /* TX3_DFE_IRQ_8 */
#define IRQ_C2C_OUT_HW_INTERRUPT_263            713             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_0,O_CDDC_RSSI_READY_TXRX_1_IRQ_0,O_CDDC_RSSI_READY_TXRX_2_IRQ_0,O_CDDC_RSSI_READY_TXRX_3_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_264            714             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_1,O_CDDC_RSSI_READY_TXRX_1_IRQ_1,O_CDDC_RSSI_READY_TXRX_2_IRQ_1,O_CDDC_RSSI_READY_TXRX_3_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_265            715             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_2,O_CDDC_RSSI_READY_TXRX_1_IRQ_2,O_CDDC_RSSI_READY_TXRX_2_IRQ_2,O_CDDC_RSSI_READY_TXRX_3_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_266            716             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_3,O_CDDC_RSSI_READY_TXRX_1_IRQ_3,O_CDDC_RSSI_READY_TXRX_2_IRQ_3,O_CDDC_RSSI_READY_TXRX_3_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_267            717             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_4,O_CDDC_RSSI_READY_TXRX_1_IRQ_4,O_CDDC_RSSI_READY_TXRX_2_IRQ_4,O_CDDC_RSSI_READY_TXRX_3_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_268            718             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_5,O_CDDC_RSSI_READY_TXRX_1_IRQ_5,O_CDDC_RSSI_READY_TXRX_2_IRQ_5,O_CDDC_RSSI_READY_TXRX_3_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_269            719             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_6,O_CDDC_RSSI_READY_TXRX_1_IRQ_6,O_CDDC_RSSI_READY_TXRX_2_IRQ_6,O_CDDC_RSSI_READY_TXRX_3_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_270            720             /* O_CDDC_RSSI_READY_TXRX_0_IRQ_7,O_CDDC_RSSI_READY_TXRX_1_IRQ_7,O_CDDC_RSSI_READY_TXRX_2_IRQ_7,O_CDDC_RSSI_READY_TXRX_3_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_271            721             /* I_APD_HIGH_TXRX_0,I_APD_HIGH_TXRX_1,I_APD_HIGH_TXRX_2,I_APD_HIGH_TXRX_3 */
#define IRQ_V_UART_INTR_0_0             722
#define IRQ_V_UART_INTR_0_1             723
#define IRQ_V_UART_INTR_1_1             724
#define IRQ_TX2_DFE_IRQ_0               725
#define IRQ_TX2_DFE_IRQ_1               726
#define IRQ_TX2_DFE_IRQ_2               727
#define IRQ_TX2_DFE_IRQ_3               728
#define IRQ_TX2_DFE_IRQ_4               729
#define IRQ_TX2_DFE_IRQ_5               730
#define IRQ_TX2_DFE_IRQ_6               731
#define IRQ_TX2_DFE_IRQ_7               732
#define IRQ_TX2_DFE_IRQ_8               733
#define IRQ_TX3_DFE_IRQ_0               734
#define IRQ_TX3_DFE_IRQ_1               735
#define IRQ_TX3_DFE_IRQ_2               736
#define IRQ_TX3_DFE_IRQ_3               737
#define IRQ_TX3_DFE_IRQ_4               738
#define IRQ_TX3_DFE_IRQ_5               739
#define IRQ_TX3_DFE_IRQ_6               740
#define IRQ_TX3_DFE_IRQ_7               741
#define IRQ_TX3_DFE_IRQ_8               742
#define IRQ_C2C_OUT_HW_INTERRUPT_272            743             /* I_APD_LOW_TXRX_0,I_APD_LOW_TXRX_1,I_APD_LOW_TXRX_2,I_APD_LOW_TXRX_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_273            744             /* I_HB2_HIGH_TXRX_0,I_HB2_HIGH_TXRX_1,I_HB2_HIGH_TXRX_2,I_HB2_HIGH_TXRX_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_274            745             /* I_HB2_LOW_TXRX_0,I_HB2_LOW_TXRX_1,I_HB2_LOW_TXRX_2,I_HB2_LOW_TXRX_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_275            746             /* CAPTURE_DBG_ERROR_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_276            747             /* CAPTURE_DBG_STAT_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_277            748             /* CAPTURE_DBG_ERROR_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_278            749             /* CAPTURE_DBG_STAT_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_279            750             /* ETH_IRQ_TX_TIMESTAMP_FIFO_FULL_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_280            751             /* ETH_IRQ_TX_TIMESTAMP_FIFO_FULL_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_281            752             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_0,O_CDUC_TSSI_READY_TXRX_1_IRQ_0,O_CDUC_TSSI_READY_TXRX_2_IRQ_0,O_CDUC_TSSI_READY_TXRX_3_IRQ_0 */
#define IRQ_C2C_OUT_HW_INTERRUPT_282            753             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_1,O_CDUC_TSSI_READY_TXRX_1_IRQ_1,O_CDUC_TSSI_READY_TXRX_2_IRQ_1,O_CDUC_TSSI_READY_TXRX_3_IRQ_1 */
#define IRQ_C2C_OUT_HW_INTERRUPT_283            754             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_2,O_CDUC_TSSI_READY_TXRX_1_IRQ_2,O_CDUC_TSSI_READY_TXRX_2_IRQ_2,O_CDUC_TSSI_READY_TXRX_3_IRQ_2 */
#define IRQ_C2C_OUT_HW_INTERRUPT_284            755             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_3,O_CDUC_TSSI_READY_TXRX_1_IRQ_3,O_CDUC_TSSI_READY_TXRX_2_IRQ_3,O_CDUC_TSSI_READY_TXRX_3_IRQ_3 */
#define IRQ_C2C_OUT_HW_INTERRUPT_285            756             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_4,O_CDUC_TSSI_READY_TXRX_1_IRQ_4,O_CDUC_TSSI_READY_TXRX_2_IRQ_4,O_CDUC_TSSI_READY_TXRX_3_IRQ_4 */
#define IRQ_C2C_OUT_HW_INTERRUPT_286            757             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_5,O_CDUC_TSSI_READY_TXRX_1_IRQ_5,O_CDUC_TSSI_READY_TXRX_2_IRQ_5,O_CDUC_TSSI_READY_TXRX_3_IRQ_5 */
#define IRQ_C2C_OUT_HW_INTERRUPT_287            758             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_6,O_CDUC_TSSI_READY_TXRX_1_IRQ_6,O_CDUC_TSSI_READY_TXRX_2_IRQ_6,O_CDUC_TSSI_READY_TXRX_3_IRQ_6 */
#define IRQ_C2C_OUT_HW_INTERRUPT_288            759             /* O_CDUC_TSSI_READY_TXRX_0_IRQ_7,O_CDUC_TSSI_READY_TXRX_1_IRQ_7,O_CDUC_TSSI_READY_TXRX_2_IRQ_7,O_CDUC_TSSI_READY_TXRX_3_IRQ_7 */
#define IRQ_C2C_OUT_HW_INTERRUPT_289            760             /* RC_DYN_GLBL_CNTRL_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_290            761             /* TX0_DFE_IRQ_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_291            762             /* TX0_DFE_IRQ_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_292            763             /* TX0_DFE_IRQ_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_293            764             /* TX0_DFE_IRQ_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_294            765             /* TX1_DFE_IRQ_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_295            766             /* TX1_DFE_IRQ_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_296            767             /* TX1_DFE_IRQ_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_297            768             /* TX1_DFE_IRQ_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_298            769             /* TX2_DFE_IRQ_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_299            770             /* TX2_DFE_IRQ_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_300            771             /* TX2_DFE_IRQ_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_301            772             /* TX2_DFE_IRQ_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_302            773             /* TX3_DFE_IRQ_9 */
#define IRQ_C2C_OUT_HW_INTERRUPT_303            774             /* TX3_DFE_IRQ_10 */
#define IRQ_C2C_OUT_HW_INTERRUPT_304            775             /* TX3_DFE_IRQ_11 */
#define IRQ_C2C_OUT_HW_INTERRUPT_305            776             /* TX3_DFE_IRQ_12 */
#define IRQ_C2C_OUT_HW_INTERRUPT_306            777             /* RC_DYN_CNTRL0_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_307            778             /* RC_DYN_CNTRL1_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_308            779             /* RC_DYN_CNTRL2_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_309            780             /* RC_DYN_CNTRL1_INTRPT */
#define IRQ_C2C_OUT_HW_INTERRUPT_310            781             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_311            782             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_312            783             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_313            784             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_314            785             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_315            786             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_316            787             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_317            788             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_318            789             /* Not connected */
#define IRQ_C2C_OUT_HW_INTERRUPT_319            790             /* Not connected */
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_0              791
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_0              791
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_0              791
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_0              791
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_1              792
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_1              792
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_1              792
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_1              792
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_2              793
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_2              793
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_2              793
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_2              793
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_3              794
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_3              794
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_3              794
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_3              794
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_4              795
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_4              795
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_4              795
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_4              795
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_5              796
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_5              796
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_5              796
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_5              796
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_6              797
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_6              797
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_6              797
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_6              797
#define IRQ_O_CDDC_RSSI_READY_TXRX_0_IRQ_7              798
#define IRQ_O_CDDC_RSSI_READY_TXRX_1_IRQ_7              798
#define IRQ_O_CDDC_RSSI_READY_TXRX_2_IRQ_7              798
#define IRQ_O_CDDC_RSSI_READY_TXRX_3_IRQ_7              798
#define IRQ_I_APD_HIGH_TXRX_0           799
#define IRQ_I_APD_HIGH_TXRX_1           799
#define IRQ_I_APD_HIGH_TXRX_2           799
#define IRQ_I_APD_HIGH_TXRX_3           799
#define IRQ_I_APD_LOW_TXRX_0            800
#define IRQ_I_APD_LOW_TXRX_1            800
#define IRQ_I_APD_LOW_TXRX_2            800
#define IRQ_I_APD_LOW_TXRX_3            800
#define IRQ_I_HB2_HIGH_TXRX_0           801
#define IRQ_I_HB2_HIGH_TXRX_1           801
#define IRQ_I_HB2_HIGH_TXRX_2           801
#define IRQ_I_HB2_HIGH_TXRX_3           801
#define IRQ_I_HB2_LOW_TXRX_0            802
#define IRQ_I_HB2_LOW_TXRX_1            802
#define IRQ_I_HB2_LOW_TXRX_2            802
#define IRQ_I_HB2_LOW_TXRX_3            802
#define IRQ_CAPTURE_DBG_ERROR_0         803
#define IRQ_CAPTURE_DBG_STAT_0          804
#define IRQ_CAPTURE_DBG_ERROR_1         805
#define IRQ_CAPTURE_DBG_STAT_1          806
#define IRQ_A55_PERI_MDMA_CH0_DONE_INTR_PIPED_0         807
#define IRQ_A55_PERI_MDMA_CH0_DONE_INTR_PIPED_1         808
#define IRQ_A55_PERI_MDMA_CH0_ERR_INTR_PIPED_0          809
#define IRQ_A55_PERI_MDMA_CH0_ERR_INTR_PIPED_1          810
#define IRQ_A55_PERI_MDMA_CH1_DONE_INTR_PIPED_0         811
#define IRQ_A55_PERI_MDMA_CH1_DONE_INTR_PIPED_1         812
#define IRQ_A55_PERI_MDMA_CH1_ERR_INTR_PIPED_0          813
#define IRQ_A55_PERI_MDMA_CH1_ERR_INTR_PIPED_1          814
#define IRQ_ETH_IRQ_MAC_RX_ERROR_0              815
#define IRQ_ETH_IRQ_MAC_RX_ERROR_1              816
#define IRQ_ETH_IRQ_MAC_TX_ERROR_0              817
#define IRQ_ETH_IRQ_MAC_TX_ERROR_1              818
#define IRQ_ETH_IRQ_PCS_RX_ERROR_0              819
#define IRQ_ETH_IRQ_PCS_RX_ERROR_1              820
#define IRQ_ETH_IRQ_TX_TIMESTAMP_FIFO_FULL_0            821
#define IRQ_ETH_IRQ_TX_TIMESTAMP_FIFO_FULL_1            822
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_0             823
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_0             823
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_0             823
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_0             823
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_1             824
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_1             824
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_1             824
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_1             824
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_2             825
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_2             825
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_2             825
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_2             825
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_3             826
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_3             826
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_3             826
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_3             826
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_4             827
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_4             827
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_4             827
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_4             827
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_5             828
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_5             828
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_5             828
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_5             828
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_6             829
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_6             829
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_6             829
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_6             829
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_7             830
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_7             830
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_7             830
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_7             830
#define IRQ_O_PDS_TX0_NPD_ARM_IRQ_8             831
#define IRQ_O_PDS_TX1_NPD_ARM_IRQ_8             831
#define IRQ_O_PDS_TX2_NPD_ARM_IRQ_8             831
#define IRQ_O_PDS_TX3_NPD_ARM_IRQ_8             831
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_0              832
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_0              832
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_0              832
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_0              832
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_1              833
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_1              833
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_1              833
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_1              833
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_2              834
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_2              834
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_2              834
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_2              834
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_3              835
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_3              835
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_3              835
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_3              835
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_4              836
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_4              836
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_4              836
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_4              836
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_5              837
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_5              837
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_5              837
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_5              837
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_6              838
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_6              838
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_6              838
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_6              838
#define IRQ_O_CDUC_TSSI_READY_TXRX_0_IRQ_7              839
#define IRQ_O_CDUC_TSSI_READY_TXRX_1_IRQ_7              839
#define IRQ_O_CDUC_TSSI_READY_TXRX_2_IRQ_7              839
#define IRQ_O_CDUC_TSSI_READY_TXRX_3_IRQ_7              839
#define IRQ_I_M4_ARM_WATCHDOG_TIMEOUT_0         840
#define IRQ_I_M4_ARM_WATCHDOG_TIMEOUT_1         841
#define IRQ_RC_DYN_GLBL_CNTRL_INTRPT            842
#define IRQ_TX0_DFE_IRQ_9               843
#define IRQ_TX0_DFE_IRQ_10              844
#define IRQ_TX0_DFE_IRQ_11              845
#define IRQ_TX0_DFE_IRQ_12              846
#define IRQ_TX1_DFE_IRQ_9               847
#define IRQ_TX1_DFE_IRQ_10              848
#define IRQ_TX1_DFE_IRQ_11              849
#define IRQ_TX1_DFE_IRQ_12              850
#define IRQ_TX2_DFE_IRQ_9               851
#define IRQ_TX2_DFE_IRQ_10              852
#define IRQ_TX2_DFE_IRQ_11              853
#define IRQ_TX2_DFE_IRQ_12              854
#define IRQ_TX3_DFE_IRQ_9               855
#define IRQ_TX3_DFE_IRQ_10              856
#define IRQ_TX3_DFE_IRQ_11              857
#define IRQ_TX3_DFE_IRQ_12              858
#define IRQ_I_PDS_TX0_INTR_IRQ_8                859
#define IRQ_I_PDS_TX1_INTR_IRQ_8                860
#define IRQ_I_PDS_TX2_INTR_IRQ_8                861
#define IRQ_I_PDS_TX3_INTR_IRQ_8                862
#define IRQ_I_PDS_TX0_INTR_IRQ_9                863
#define IRQ_I_PDS_TX1_INTR_IRQ_9                864
#define IRQ_I_PDS_TX2_INTR_IRQ_9                865
#define IRQ_I_PDS_TX3_INTR_IRQ_9                866
#define IRQ_RC_DYN_CNTRL0_INTRPT                867
#define IRQ_RC_DYN_CNTRL1_INTRPT                868
#define IRQ_RC_DYN_CNTRL2_INTRPT                869
#define IRQ_I_SBET_ARM_INTERRUPT                870

#endif /* __ADI_ADRV906X_IRQ_DEF_H__ */
