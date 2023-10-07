// /*
//  * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
//  *
//  * SPDX-License-Identifier: Unlicense OR CC0-1.0
//  */

// #include <stdlib.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "esp_log.h"
// #include "usb/usb_host.h"

// #define CLIENT_NUM_EVENT_MSG        5

// #define ACTION_OPEN_DEV             0x01
// #define ACTION_GET_DEV_INFO         0x02
// #define ACTION_GET_DEV_DESC         0x04
// #define ACTION_GET_CONFIG_DESC      0x08
// #define ACTION_GET_STR_DESC         0x10
// #define ACTION_CLOSE_DEV            0x20
// #define ACTION_EXIT                 0x40

// typedef struct {
//     usb_host_client_handle_t client_hdl;
//     uint8_t dev_addr;
//     usb_device_handle_t dev_hdl;
//     uint32_t actions;
// } class_driver_t;

// static const char *TAG = "CLASS";

// static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
// {
//     class_driver_t *driver_obj = (class_driver_t *)arg;
//     switch (event_msg->event) {
//         case USB_HOST_CLIENT_EVENT_NEW_DEV:
//             if (driver_obj->dev_addr == 0) {
//                 driver_obj->dev_addr = event_msg->new_dev.address;
//                 //Open the device next
//                 driver_obj->actions |= ACTION_OPEN_DEV;
//             }
//             break;
//         case USB_HOST_CLIENT_EVENT_DEV_GONE:
//             if (driver_obj->dev_hdl != NULL) {
//                 //Cancel any other actions and close the device next
//                 driver_obj->actions = ACTION_CLOSE_DEV;
//             }
//             break;
//         default:
//             //Should never occur
//             abort();
//     }
// }

// static void action_open_dev(class_driver_t *driver_obj)
// {
//     assert(driver_obj->dev_addr != 0);
//     ESP_LOGI(TAG, "Opening device at address %d", driver_obj->dev_addr);
//     ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
//     //Get the device's information next
//     driver_obj->actions &= ~ACTION_OPEN_DEV;
//     driver_obj->actions |= ACTION_GET_DEV_INFO;
// }

// static void action_get_info(class_driver_t *driver_obj)
// {
//     assert(driver_obj->dev_hdl != NULL);
//     ESP_LOGI(TAG, "Getting device information");
//     usb_device_info_t dev_info;
//     ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
//     ESP_LOGI(TAG, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
//     ESP_LOGI(TAG, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
//     //Todo: Print string descriptors

//     //Get the device descriptor next
//     driver_obj->actions &= ~ACTION_GET_DEV_INFO;
//     driver_obj->actions |= ACTION_GET_DEV_DESC;
// }

// static void action_get_dev_desc(class_driver_t *driver_obj)
// {
//     assert(driver_obj->dev_hdl != NULL);
//     ESP_LOGI(TAG, "Getting device descriptor");
//     const usb_device_desc_t *dev_desc;
//     ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
//     usb_print_device_descriptor(dev_desc);
//     //Get the device's config descriptor next
//     driver_obj->actions &= ~ACTION_GET_DEV_DESC;
//     driver_obj->actions |= ACTION_GET_CONFIG_DESC;
// }

// static void action_get_config_desc(class_driver_t *driver_obj)
// {
//     assert(driver_obj->dev_hdl != NULL);
//     ESP_LOGI(TAG, "Getting config descriptor");
//     const usb_config_desc_t *config_desc;
//     ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
//     usb_print_config_descriptor(config_desc, NULL);
//     //Get the device's string descriptors next
//     driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
//     driver_obj->actions |= ACTION_GET_STR_DESC;
// }

// static void action_get_str_desc(class_driver_t *driver_obj)
// {
//     assert(driver_obj->dev_hdl != NULL);
//     usb_device_info_t dev_info;
//     ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
//     if (dev_info.str_desc_manufacturer) {
//         ESP_LOGI(TAG, "Getting Manufacturer string descriptor");
//         usb_print_string_descriptor(dev_info.str_desc_manufacturer);
//     }
//     if (dev_info.str_desc_product) {
//         ESP_LOGI(TAG, "Getting Product string descriptor");
//         usb_print_string_descriptor(dev_info.str_desc_product);
//     }
//     if (dev_info.str_desc_serial_num) {
//         ESP_LOGI(TAG, "Getting Serial Number string descriptor");
//         usb_print_string_descriptor(dev_info.str_desc_serial_num);
//     }
//     //Nothing to do until the device disconnects
//     driver_obj->actions &= ~ACTION_GET_STR_DESC;
// }

// static void aciton_close_dev(class_driver_t *driver_obj)
// {
//     ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
//     driver_obj->dev_hdl = NULL;
//     driver_obj->dev_addr = 0;
//     //We need to exit the event handler loop
//     driver_obj->actions &= ~ACTION_CLOSE_DEV;
//     driver_obj->actions |= ACTION_EXIT;
// }

// void class_driver_task(void *arg)
// {
//     SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
//     class_driver_t driver_obj = {0};

//     //Wait until daemon task has installed USB Host Library
//     xSemaphoreTake(signaling_sem, portMAX_DELAY);

//     ESP_LOGI(TAG, "Registering Client");
//     usb_host_client_config_t client_config = {
//         .is_synchronous = false,    //Synchronous clients currently not supported. Set this to false
//         .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
//         .async = {
//             .client_event_callback = client_event_cb,
//             .callback_arg = (void *)&driver_obj,
//         },
//     };
//     ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));

//     while (1) {
//         if (driver_obj.actions == 0) {
//             usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
//         } else {
//             if (driver_obj.actions & ACTION_OPEN_DEV) {
//                 action_open_dev(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_GET_DEV_INFO) {
//                 action_get_info(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_GET_DEV_DESC) {
//                 action_get_dev_desc(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_GET_CONFIG_DESC) {
//                 action_get_config_desc(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_GET_STR_DESC) {
//                 action_get_str_desc(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_CLOSE_DEV) {
//                 aciton_close_dev(&driver_obj);
//             }
//             if (driver_obj.actions & ACTION_EXIT) {
//                 break;
//             }
//         }
//     }

//     ESP_LOGI(TAG, "Deregistering Client");
//     ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj.client_hdl));

//     //Wait to be deleted
//     xSemaphoreGive(signaling_sem);
//     vTaskSuspend(NULL);
// }

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "usb/usb_host.h"

#define CLIENT_NUM_EVENT_MSG        5

#define ACTION_OPEN_DEV             0x01
#define ACTION_GET_DEV_INFO         0x02
#define ACTION_GET_DEV_DESC         0x04
#define ACTION_GET_CONFIG_DESC      0x08
#define ACTION_GET_STR_DESC         0x10
#define ACTION_CLOSE_DEV            0x20
#define ACTION_EXIT                 0x40
#define ACTION_DRIVER_ACTION_TRANSFER 0x08

typedef struct {
    usb_host_client_handle_t client_hdl;
    uint8_t dev_addr;
    usb_device_handle_t dev_hdl;
    uint32_t actions;
} class_driver_t;

static const char *TAG = "CLASS";

bool first_send = 1;

bool first_usb_use = 1;


static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    class_driver_t *driver_obj = (class_driver_t *)arg;
    switch (event_msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            if (driver_obj->dev_addr == 0) {
                driver_obj->dev_addr = event_msg->new_dev.address;
                //Open the device next
                driver_obj->actions |= ACTION_OPEN_DEV;
            }
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
                ESP_LOGI(TAG, "DEV_GONE 1");
            if (driver_obj->dev_hdl != NULL) {
                ESP_LOGI(TAG, "DEV_GONE 2");
                //Cancel any other actions and close the device next
                driver_obj->actions |= ACTION_CLOSE_DEV;
            }
            break;
        default:
            //Should never occur
            abort();
    }
}

static void action_open_dev(class_driver_t *driver_obj)
{
    assert(driver_obj->dev_addr != 0);
    ESP_LOGI(TAG, "Opening device at address %d", driver_obj->dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
    ESP_ERROR_CHECK(usb_host_interface_claim(driver_obj->client_hdl, driver_obj->dev_hdl, 0, 0));
    //Get the device's information next
    driver_obj->actions &= ~ACTION_OPEN_DEV;
    driver_obj->actions |= ACTION_DRIVER_ACTION_TRANSFER;
}

static void aciton_close_dev(class_driver_t *driver_obj)
{
    ESP_LOGI(TAG, "Close device");
    ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
    ESP_ERROR_CHECK(usb_host_interface_release(driver_obj->client_hdl, driver_obj->dev_hdl, 0));
    driver_obj->dev_hdl = NULL;
    driver_obj->dev_addr = 0;
    //We need to exit the event handler loop
    driver_obj->actions &= ~ACTION_CLOSE_DEV;
    driver_obj->actions |= ACTION_EXIT;
}

struct Keys {
    int key_a;
    int key_b;
    int key_c;
    int key_d;
} keys;

void InitKeys(uint8_t* input_array) {
    keys.key_a = 0x00;
    keys.key_b = 0x00;
    keys.key_c = 0x5a + 0x04;
    keys.key_d = 0x00;
    uint8_t data[18] = {0x7b, 0xff, 0x20, 0, 0x06, 0, (uint8_t)(keys.key_a),
                        (uint8_t)(keys.key_b), 0, 0, (uint8_t)(keys.key_c),
                        (uint8_t)(keys.key_d), 0, 0x08, 0x0c, 0, 0xf3, 0xf7};

    for (int i = 0; i < 18; ++i) {
        input_array[i] = data[i];
    }

    for (int i = 18; i < 64; ++i) {
        input_array[i] = 0x00;
    } 
}

void ProcessKeys(uint8_t* input_array) {
    keys.key_a += 0x04;
    keys.key_c -= 0x04;
    if (keys.key_a > 0xff) {
        keys.key_a -= 0xfe;
        keys.key_b += 0x01;
        if (keys.key_b > 0xff) {
            keys.key_b = 0x00;
        }
        keys.key_c -= 0x01;
    }
    else if (keys.key_c < 0x00) {
        keys.key_c += 0xff;
        keys.key_d -= 0x01;
        if (keys.key_d < 0x00) {
            keys.key_d = 0xff;
        }
    }

    uint8_t data[18] = {0x7b, 0xff, 0x20, 0, 0x06, 0, (uint8_t)(keys.key_a),
                        (uint8_t)(keys.key_b), 0, 0, (uint8_t)(keys.key_c),
                        (uint8_t)(keys.key_d), 0, 0x08, 0x0c, 0, 0xf3, 0xf7};

    for (int i = 0; i < 18; ++i) {
        input_array[i] = data[i];
    }
}

void usb_transfer_complete(usb_transfer_t *transfer) {
    static uint8_t compare_data[64];
    class_driver_t *class_driver_obj = (class_driver_t *)transfer->context;
    printf("Transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    ESP_LOGI(TAG, "Enter transfer callback");
    first_usb_use = 1;
    if (transfer->status == USB_TRANSFER_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "USB transfer completed successfully!");
        // Чтение ответных данных из буфера
        uint8_t* data = transfer->data_buffer;
        for (int i = 0; i < 64; ++i) {
            if (data[i] != compare_data[i]) {
                printf("!!! DIFFERENT ARRAYS !!! at pos %d\n", i);
            }
        }
        for (int i = 0; i < 64; ++i) {
            compare_data[i] = transfer->data_buffer[i];
            printf("%#02x ", data[i]);
        }
        // ESP_LOGI(TAG, "Response: %f", ((float)(data[20]) + (float)(data[21]) * 256
        //                                 + (float)(data[22]) * 256 * 256) / 100.0);
        ESP_LOGI(TAG, "Response: %f", (float)(data[11]) / 100.0);
    } else {
        ESP_LOGI(TAG, "USB transfer failed with error code %d", transfer->status);
    }
    // class_driver_obj->actions &= ~ACTION_DRIVER_ACTION_TRANSFER;
    // class_driver_obj->actions |= ACTION_CLOSE_DEV;
}

static void transfer_cb(usb_transfer_t *transfer)
{
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    class_driver_t *class_driver_obj = (class_driver_t *)transfer->context;
    printf("Transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    // class_driver_obj->actions &= ~ACTION_DRIVER_ACTION_TRANSFER;
    class_driver_obj->actions |= ACTION_CLOSE_DEV;
    first_send = 1;
}

void class_driver_task(void *arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
    class_driver_t driver_obj = {0};

    //Wait until daemon task has installed USB Host Library
    xSemaphoreTake(signaling_sem, portMAX_DELAY);

    ESP_LOGI(TAG, "Registering Client");
    usb_host_client_config_t client_config = {
        .is_synchronous = false,    //Synchronous clients currently not supported. Set this to false
        .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = client_event_cb,
            .callback_arg = &driver_obj,
        },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));

    usb_transfer_t *transfer;
    usb_host_transfer_alloc(64, 0, &transfer);

    uint8_t data[18] = {0x7b, 0xff, 0x20, 0, 0x06, 0, keys.key_a, keys.key_b,
                        0, 0, keys.key_c, keys.key_d, 0, 0x08, 0x0c, 0, 0xf3, 0xf7};
    // // memcpy(transfer->data_buffer, data, 18);
    // for (int i = 0; i < 18; ++i) {
    //     transfer->data_buffer[i] = data[i];
    // }
    
    InitKeys(transfer->data_buffer);

    while (1) {
        vTaskDelay(100);
        if (driver_obj.actions == 0) {
            usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
        } else {
            if (driver_obj.actions & ACTION_OPEN_DEV) {
                ESP_LOGI(TAG, "Enter opening");
                action_open_dev(&driver_obj);
            }
            if (driver_obj.actions & ACTION_DRIVER_ACTION_TRANSFER) {
                ESP_LOGI(TAG, "Enter transfer");
                printf("Key A %d, Key B %d, Key C %d, Key D %d\n", keys.key_a, keys.key_b, keys.key_c, keys.key_d);
                transfer->num_bytes = 64;
                transfer->callback = usb_transfer_complete;
                transfer->bEndpointAddress = 0x01;
                transfer->device_handle = driver_obj.dev_hdl;
                transfer->context = (void*)&driver_obj;
                transfer->actual_num_bytes = 18;
                if (first_usb_use) {
                    ESP_LOGI(TAG, "Status: %x", 0xffff & usb_host_transfer_submit(transfer));
                    // ESP_ERROR_CHECK(usb_host_transfer_submit(transfer));
                    first_usb_use = 0;
                    ProcessKeys(data);
                    for (int i = 0; i < 18; ++i) {
                        transfer->data_buffer[i] = data[i];
                    }
                }
                usb_host_client_handle_events(driver_obj.client_hdl, portMAX_DELAY);
            }
            if (driver_obj.actions & ACTION_CLOSE_DEV) {
                ESP_LOGI(TAG, "Enter ending");
                aciton_close_dev(&driver_obj);
                first_usb_use = 0;
            }
            if (driver_obj.actions & ACTION_EXIT) {
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Deregistering Client");
    ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj.client_hdl));
    usb_host_transfer_free(transfer);

    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}

