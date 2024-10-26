/*
 * Copyright (c) 2024 Patryk Biel <pbiel7@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include "adxl345.h"

#include <zephyr/drivers/sensor.h>

//TODO(bielpa): Consider changing error codes checks from < 0 to != 0

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static void adxl345_gpio_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
    LOG_INF("Dupa");
    /*
        - disable interrupts -> V
        - call handler
            - submit work OR -> V
            - run in own thread
    */

    struct adxl345_dev_data *data = CONTAINER_OF(cb, struct adxl345_dev_data, gpio_cb); 
    const struct adxl345_dev_config *config = data->dev->config;

    gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_DISABLE);

#if defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&data->work);
#elif defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
    #error "ADXL345_TRIGGER_OWN_THREAD not implementd"
#endif
}

static void adxl345_thread_cb(const struct device *dev)
{
    LOG_INF("%s", __FUNCTION__);

    struct adxl345_dev_data *data = dev->data;

    k_mutex_lock(data->trigger_mutex);


    k_mutex_unlock(data->trigger_mutex);

    /*
      for each handler:
        - check is handler for interrupt
        - check is interrupt for handler triggered
        - call handler
      enable interrupts
    */

    int rc;
    uint8_t value;
    rc = adxl345_reg_read_byte(dev, ADXL345_INT_SOURCE_REG, &value);
}


#if defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
void adxl345_work_cb(struct k_work *work)
{
    struct adxl345_dev_data *data = CONTAINER_OF(work, struct adxl345_dev_data, work);

    adxl345_thread_cb(data->dev);
}
#endif

int adxl345_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
    int ret;
    uint8_t mask = 0;
    struct adxl345_dev_data *data = dev->data;
    const struct adxl345_dev_config *config = dev->config;

    if (!config->interrupt.port) {
        return -ENOTSUP;
    }

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

    k_mutex_lock(&data->trigger_mutex, K_FOREVER);

    switch (trig->type) {
        case SENSOR_TRIG_DATA_READY:
            LOG_ERR("Not implemented: SENSOR_TRIG_DATA_READY");
            break;
        case SENSOR_TRIG_TAP: // TODO(bielpa): consider removing
            LOG_ERR("Not implemented: SENSOR_TRIG_TAP");
            break;
        case SENSOR_TRIG_DOUBLE_TAP: // TODO(bielpa): consider removing
            LOG_ERR("Not implemented: SENSOR_TRIG_DOUBLE_TAP");
            break;
        case SENSOR_TRIG_MOTION:
            mask |= ADXL345_ACTIVITY;
            data->act_handler = handler;
            break;
        case SENSOR_TRIG_STATIONARY:
            LOG_ERR("Not implemented: SENSOR_TRIG_STATIONARY");
            break;
        case SENSOR_TRIG_FREEFALL: // TODO(bielpa): consider removing
            LOG_ERR("Not implemented: SENSOR_TRIG_FREEFALL");
            break;
        default:
		    LOG_ERR("Unsupported sensor trigger");
		    return -ENOTSUP;
    }

    k_mutex_unlock(&data->trigger_mutex);

    ret = adxl345_reg_write_byte(dev, ADXL345_INT_ENEABLE_REG, mask);
    if (ret < 0) {
        LOG_ERR("Could not write an interrupt configuration to the device");
        return ret;
    }

    //TODO(bielpa): Can I call this regardlesss of the previous call? E.g. when interrupts have been enabled previously
    ret = gpio_pin_interrupt_configure_dt(&config->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

    return 0;
}

int adxl345_init_interrupt(const struct device *dev)
{
	const struct adxl345_dev_config *config = dev->config;
    struct adxl345_dev_data *data = dev->data;

    int ret;

    if (!gpio_is_ready_dt(&config->interrupt)) {
        LOG_ERR("GPIO port: %s not ready", config->interrupt.port->name);
        return -ENODEV;
    }

    ret = k_mutex_init(&data->trigger_mutex);
    if (ret) {
        LOG_ERR("Could not initialize mutex");
        return ret;
    }

    ret = gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Could not configure interrupt for GPIO: %s", config->interrupt.port->name);
        return ret;
    }

    gpio_init_callback(&data->gpio_cb, 
        adxl345_gpio_callback, 
        BIT(config->interrupt.pin)
    );

    ret = gpio_add_callback(config->interrupt.port, &data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Could not add callback callback for GPIO: %s", config->interrupt.port->name);
        return ret;
    }

    data->dev = dev;

#if defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
    k_work_init(&data->work, adxl345_work_cb);
#endif
    LOG_INF("TODO(bielpa): REMOVE: Configured properly");
    return 0;
}