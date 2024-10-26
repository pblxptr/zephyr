/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_
#define ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif
#include <zephyr/sys/util.h>

/* ADXL345 communication commands */
#define ADXL345_WRITE_CMD          0x00
#define ADXL345_READ_CMD           0x80
#define ADXL345_MULTIBYTE_FLAG     0x40

/* Registers */
#define ADXL345_DEVICE_ID_REG      0x00
#define ADXL345_THRESH_ACT		   0x24
#define ADXL345_THRESH_INACT	   0x25
#define ADXL345_ACT_INACT_CTL	   0x27
#define ADXL345_RATE_REG           0x2c
#define ADXL345_POWER_CTL_REG      0x2d
#define ADXL345_INT_ENEABLE_REG    0x2E
#define ADXL345_INT_MAP_REG        0x2F
#define ADXL345_INT_SOURCE_REG     0x30
#define ADXL345_DATA_FORMAT_REG    0x31
#define ADXL345_X_AXIS_DATA_0_REG  0x32
#define ADXL345_FIFO_CTL_REG       0x38
#define ADXL345_FIFO_STATUS_REG    0x39

#define ADXL345_PART_ID            0xe5

#define ADXL345_RANGE_2G           0x0
#define ADXL345_RANGE_4G           0x1
#define ADXL345_RANGE_8G           0x2
#define ADXL345_RANGE_16G          0x3
#define ADXL345_RATE_25HZ          0x8
#define ADXL345_ENABLE_MEASURE_BIT (1 << 3)
#define ADXL345_FIFO_STREAM_MODE   (1 << 7)
#define ADXL345_FIFO_COUNT_MASK    0x3f
#define ADXL345_COMPLEMENT         0xfc00

#define ADXL345_MAX_FIFO_SIZE      32

/* ADXL345_INT_ENABLE */
#define ADXL345_DATA_READY         BIT(7)
#define ADXL345_SINGLE_TAP         BIT(6)
#define ADXL345_DOUBLE_TAP         BIT(5)
#define ADXL345_ACTIVITY           BIT(4)
#define ADXL345_INACTIVITY         BIT(3)
#define ADXL345_FREE_FALL          BIT(2)
#define ADXL345_FIFO_WATERMARK     BIT(1)
#define ADXL345_FIFO_OVERRUN       BIT(0)

struct adxl345_dev_data {
	unsigned int sample_number;

	int16_t bufx[ADXL345_MAX_FIFO_SIZE];
	int16_t bufy[ADXL345_MAX_FIFO_SIZE];
	int16_t bufz[ADXL345_MAX_FIFO_SIZE];

#if defined(CONFIG_ADXL345_TRIGGER)
	struct gpio_callback gpio_cb;
	const struct device *dev;
	struct k_mutex trigger_mutex;
	sensor_trigger_handler_t act_handler;
#if defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif
};

struct adxl345_sample {
	int16_t x;
	int16_t y;
	int16_t z;
};

union adxl345_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
};

typedef bool (*adxl345_bus_is_ready_fn)(const union adxl345_bus *bus);
typedef int (*adxl345_reg_access_fn)(const struct device *dev, uint8_t cmd,
				     uint8_t reg_addr, uint8_t *data, size_t length);

struct adxl345_dev_config {
	const union adxl345_bus bus;
	adxl345_bus_is_ready_fn bus_is_ready;
	adxl345_reg_access_fn reg_access;
#if defined(CONFIG_ADXL345_TRIGGER)
	struct gpio_dt_spec interrupt;
	uint8_t int_enable_cfg;
	uint8_t int_map_cfg;
#endif
};

int adxl345_reg_write_byte(const struct device *dev, uint8_t addr, uint8_t val);
int adxl345_reg_read_byte(const struct device *dev, uint8_t addr, uint8_t *buf);

#if defined(CONFIG_ADXL345_TRIGGER)
int adxl345_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);
int adxl345_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADX345_ADX345_H_ */
