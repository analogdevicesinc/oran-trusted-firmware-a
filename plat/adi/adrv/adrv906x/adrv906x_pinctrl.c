/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>

#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <adrv906x_pin_def.h>
#include <adrv906x_pinctrl.h>

#define BYTE_WORD_SIZE_SHIFT                    (0x2)

void pinctrl_set_pad_drv_strn(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_cmos_pad_ds_t pad_ds)
{
	uint32_t offset = (pad_num / PAD_DS_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t ds_addr_0 = baseaddr + CMOS_PAD_DS0N_OFFSET + offset;
	uintptr_t ds_addr_1 = baseaddr + CMOS_PAD_DS1N_OFFSET + offset;
	uintptr_t ds_addr_2 = baseaddr + CMOS_PAD_DS2N_OFFSET + offset;
	uintptr_t ds_addr_3 = baseaddr + CMOS_PAD_DS3N_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_DS_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (0 == ((uint32_t)pad_ds & 0x1))
		mmio_write_32(ds_addr_0, mmio_read_32(ds_addr_0) & ~bitmask);
	else
		mmio_write_32(ds_addr_0, mmio_read_32(ds_addr_0) | bitmask);

	if (0 == ((uint32_t)pad_ds & 0x2))
		mmio_write_32(ds_addr_1, mmio_read_32(ds_addr_1) & ~bitmask);
	else
		mmio_write_32(ds_addr_1, mmio_read_32(ds_addr_1) | bitmask);

	if (0 == ((uint32_t)pad_ds & 0x4))
		mmio_write_32(ds_addr_2, mmio_read_32(ds_addr_2) & ~bitmask);
	else
		mmio_write_32(ds_addr_2, mmio_read_32(ds_addr_2) | bitmask);

	if (0 == ((uint32_t)pad_ds & 0x8))
		mmio_write_32(ds_addr_3, mmio_read_32(ds_addr_3) & ~bitmask);
	else
		mmio_write_32(ds_addr_3, mmio_read_32(ds_addr_3) | bitmask);
}

void pinctrl_get_pad_drv_strn(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_cmos_pad_ds_t *pad_ds)
{
	uint32_t offset = (pad_num / PAD_DS_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t ds_addr_0 = baseaddr + CMOS_PAD_DS0N_OFFSET + offset;
	uintptr_t ds_addr_1 = baseaddr + CMOS_PAD_DS1N_OFFSET + offset;
	uintptr_t ds_addr_2 = baseaddr + CMOS_PAD_DS2N_OFFSET + offset;
	uintptr_t ds_addr_3 = baseaddr + CMOS_PAD_DS3N_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_DS_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	*pad_ds = 0;
	if ((mmio_read_32(ds_addr_0) & bitmask))
		*pad_ds |= 0x1;
	if ((mmio_read_32(ds_addr_1) & bitmask))
		*pad_ds |= 0x2;
	if ((mmio_read_32(ds_addr_2) & bitmask))
		*pad_ds |= 0x4;
	if ((mmio_read_32(ds_addr_3) & bitmask))
		*pad_ds |= 0x8;
}

void pinctrl_set_pad_st_en(const uintptr_t baseaddr, uint32_t pad_num, bool enable)
{
	uint32_t offset = (pad_num / PAD_ST_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t stn_addr = baseaddr + CMOS_PAD_ST0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_ST_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (false == enable)
		mmio_write_32(stn_addr, mmio_read_32(stn_addr) & ~bitmask);
	else
		mmio_write_32(stn_addr, mmio_read_32(stn_addr) | bitmask);
}

void pinctrl_get_pad_st_en(const uintptr_t baseaddr, uint32_t pad_num, bool *enable)
{
	uint32_t offset = (pad_num / PAD_ST_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t stn_addr = baseaddr + CMOS_PAD_ST0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_ST_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (mmio_read_32(stn_addr) & bitmask)
		*enable = true;
	else
		*enable = false;
}

void pinctrl_set_pad_pen(const uintptr_t baseaddr, uint32_t pad_num, bool enable)
{
	uint32_t offset = (pad_num / PAD_PE_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t pen_addr = baseaddr + CMOS_PAD_PE0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_PE_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (false == enable)
		mmio_write_32(pen_addr, mmio_read_32(pen_addr) & ~bitmask);
	else
		mmio_write_32(pen_addr, mmio_read_32(pen_addr) | bitmask);
}

void pinctrl_get_pad_pen(const uintptr_t baseaddr, uint32_t pad_num, bool *enable)
{
	uint32_t offset = (pad_num / PAD_PE_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t pen_addr = baseaddr + CMOS_PAD_PE0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_PE_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (mmio_read_32(pen_addr) & bitmask)
		*enable = true;
	else
		*enable = false;
}

void pinctrl_set_pad_ps(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_pad_pupd_t pull)
{
	uint32_t offset = (pad_num / PAD_PS_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t psn_addr = baseaddr + CMOS_PAD_PS0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_PS_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (PULL_DOWN == pull)
		mmio_write_32(psn_addr, mmio_read_32(psn_addr) & ~bitmask);
	else
		mmio_write_32(psn_addr, mmio_read_32(psn_addr) | bitmask);
}

void pinctrl_get_pad_ps(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_pad_pupd_t *pull)
{
	uint32_t offset = (pad_num / PAD_PS_BIT_WIDTH) << BYTE_WORD_SIZE_SHIFT;
	uintptr_t psn_addr = baseaddr + CMOS_PAD_PS0_OFFSET + offset;
	uint32_t bitpos = pad_num % PAD_PS_BIT_WIDTH;
	uint32_t bitmask = (0x1 << bitpos);

	if (mmio_read_32(psn_addr) & bitmask)
		*pull = PULL_UP;
	else
		*pull = PULL_DOWN;
}

uint32_t pinctrl_get_pinmux_sel(const uintptr_t baseaddr, uint32_t pin_num)
{
	uint32_t offset = pin_num << BYTE_WORD_SIZE_SHIFT;
	uintptr_t mux_sel_addr = baseaddr + GPIO_CTRL_MUX_SEL_OFFSET + offset;

	return mmio_read_32(mux_sel_addr);
}

void pinctrl_set_pinmux_sel(const uintptr_t baseaddr, uint32_t pin_num, uint32_t sel)
{
	uint32_t offset = pin_num << BYTE_WORD_SIZE_SHIFT;
	uintptr_t mux_sel_addr = baseaddr + GPIO_CTRL_MUX_SEL_OFFSET + offset;
	uint32_t bitmask = sel;

	mmio_write_32(mux_sel_addr, bitmask);
}

pinmux_source_t pinctrl_get_pinmux(const uintptr_t baseaddr, uint32_t pin_num)
{
	uint32_t pinmux_src_idx;

	pinmux_src_idx = pinctrl_get_pinmux_sel(baseaddr, pin_num);
	return (pinmux_source_t)pinmux_config[pin_num][pinmux_src_idx];
}

bool pinctrl_set_pinmux(const uintptr_t baseaddr, uint32_t pin_num, pinmux_source_t pinmux_src)
{
	bool ret = false;
	uint32_t pinmux_src_idx;

	for (pinmux_src_idx = 0; pinmux_src_idx < ADRV906X_PINMUX_SRC_PER_PIN; pinmux_src_idx++) {
		if ((pinmux_source_t)(pinmux_config[pin_num][pinmux_src_idx]) == pinmux_src) {
			pinctrl_set_pinmux_sel(baseaddr, pin_num, pinmux_src_idx);
			ret = true;
			break;
		}
	}

	return ret;
}

bool pinctrl_s_gpio_to_source(unsigned int gpio, pinmux_source_t *source)
{
	if (gpio > ADRV906X_S_GPIO_COUNT)
		return false;
	if (source)
		*source = A55_GPIO_S_0 + gpio;
	return true;
}

bool pinctrl_ns_gpio_to_source(unsigned int gpio, pinmux_source_t *source)
{
	if (gpio > ADRV906X_NS_GPIO_COUNT)
		return false;
	if (source)
		*source = A55_GPIO_NS_0 + gpio;
	return true;
}

bool pinctrl_gpio_to_source(unsigned int gpio, bool secure, pinmux_source_t *source)
{
	if (secure)
		return pinctrl_s_gpio_to_source(gpio, source);
	return pinctrl_ns_gpio_to_source(gpio, source);
}
