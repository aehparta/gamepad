// Copyright 2019 Mark Wolfe.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     https://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "hid_dev.h"

#include "driver/adc.h"


#define DEVICE_NAME         "Old gamepad/joystick adapter"
#define LOG_TAG             "joystick"

#define NES_CLOCK           32
#define NES_LATCH           33
#define NES_DATA            27

#define BUTTON              26

#define LED_R               13
#define LED_G               14
#define LED_B               12


static uint16_t hid_conn_id = 0;
static bool sec_conn = false;

static bool connected = false;

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

static uint8_t hidd_service_uuid128[] = {
	/* LSB <--------------------------------------------------------------------------------> MSB */
	//first uuid, 16bit, [12],[13] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
	.set_scan_rsp = false,
	.include_name = true,
	.include_txpower = true,
	.min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
	.max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
	.appearance = 0x03c4, /* gamepad */
	.manufacturer_len = 0,
	.p_manufacturer_data =  NULL,
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(hidd_service_uuid128),
	.p_service_uuid = hidd_service_uuid128,
	.flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
	.adv_int_min        = 0x20,
	.adv_int_max        = 0x30,
	.adv_type           = ADV_TYPE_IND,
	.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
	//.peer_addr            =
	//.peer_addr_type       =
	.channel_map        = ADV_CHNL_ALL,
	.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
	switch (event) {
	case ESP_HIDD_EVENT_REG_FINISH: {
		if (param->init_finish.state == ESP_HIDD_INIT_OK) {
			//esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
			esp_ble_gap_set_device_name(DEVICE_NAME);
			esp_ble_gap_config_adv_data(&hidd_adv_data);
		}
		break;
	}
	case ESP_BAT_EVENT_REG: {
		break;
	}
	case ESP_HIDD_EVENT_DEINIT_FINISH:
		break;
	case ESP_HIDD_EVENT_BLE_CONNECT: {
		ESP_LOGI(LOG_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
		connected = true;
		hid_conn_id = param->connect.conn_id;
		break;
	}
	case ESP_HIDD_EVENT_BLE_DISCONNECT: {
		sec_conn = false;
		connected = false;
		ESP_LOGI(LOG_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
		esp_ble_gap_start_advertising(&hidd_adv_params);
		break;
	}
	case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
		ESP_LOGI(LOG_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
		ESP_LOG_BUFFER_HEX(LOG_TAG, param->vendor_write.data, param->vendor_write.length);
	}
	default:
		break;
	}
	return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch (event) {
	case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
		esp_ble_gap_start_advertising(&hidd_adv_params);
		break;
	case ESP_GAP_BLE_SEC_REQ_EVT:
		for (int i = 0; i < ESP_BD_ADDR_LEN; i++) {
			ESP_LOGD(LOG_TAG, "%x:", param->ble_security.ble_req.bd_addr[i]);
		}
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	case ESP_GAP_BLE_AUTH_CMPL_EVT:
		sec_conn = true;
		esp_bd_addr_t bd_addr;
		memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
		ESP_LOGI(LOG_TAG, "remote BD_ADDR: %08x%04x", \
		         (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
		         (bd_addr[4] << 8) + bd_addr[5]);
		ESP_LOGI(LOG_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
		ESP_LOGI(LOG_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
		if (!param->ble_security.auth_cmpl.success) {
			ESP_LOGE(LOG_TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
		}
		break;
	default:
		break;
	}
}

int p_init(void)
{
	esp_err_t ret;

	// Initialize NVS.
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	return 0;
}

int bt_init(void)
{
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	if (esp_bt_controller_init(&bt_cfg)) {
		ESP_LOGE(LOG_TAG, "initialize controller failed");
		return -1;
	}

	if (esp_bt_controller_enable(ESP_BT_MODE_BLE)) {
		ESP_LOGE(LOG_TAG, "enable controller failed");
		return -1;
	}

	if (esp_bluedroid_init()) {
		ESP_LOGE(LOG_TAG, "init bluedroid failed");
		return -1;
	}

	return 0;
}

void bt_disconnect(void)
{
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
	esp_bt_mem_release(ESP_BT_MODE_BTDM);
}

int bt_connect(void)
{
	if (esp_bluedroid_enable()) {
		ESP_LOGE(LOG_TAG, "init bluedroid failed");
		return -1;
	}

	if (esp_hidd_profile_init() != ESP_OK) {
		ESP_LOGE(LOG_TAG, "init bluedroid failed");
		return -1;
	}

	///register the callback function to the gap module
	esp_ble_gap_register_callback(gap_event_handler);
	esp_hidd_register_callbacks(hidd_event_callback);

	/* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
	esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
	uint8_t key_size = 16;      //the key size should be 7~16 bytes
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
	/* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
	and the response key means which key you can distribute to the Master;
	If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
	and the init key means which key you can distribute to the slave. */
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

	return 0;
}

void app_main(void)
{
	if (p_init()) {
		ESP_LOGE(LOG_TAG, "app initialization failed");
		return;
	}
	if (bt_init()) {
		ESP_LOGE(LOG_TAG, "bt init failed");
		return;
	}
	if (bt_connect()) {
		ESP_LOGE(LOG_TAG, "bt connect failed");
		return;
	}

	/* rgb-led */
	gpio_reset_pin(LED_R);
	gpio_reset_pin(LED_G);
	gpio_reset_pin(LED_B);
	gpio_set_pull_mode(LED_R, GPIO_FLOATING);
	gpio_set_pull_mode(LED_G, GPIO_FLOATING);
	gpio_set_pull_mode(LED_B, GPIO_FLOATING);
	gpio_set_direction(LED_R, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);
	gpio_set_level(LED_R, 0);
	gpio_set_level(LED_G, 0);
	gpio_set_level(LED_B, 0);

	/* flash each for to indicate boot */
	gpio_set_level(LED_R, 1);
	vTaskDelay(300 / portTICK_PERIOD_MS);
	gpio_set_level(LED_R, 0);
	gpio_set_level(LED_G, 1);
	vTaskDelay(300 / portTICK_PERIOD_MS);
	gpio_set_level(LED_G, 0);
	gpio_set_level(LED_B, 1);
	vTaskDelay(300 / portTICK_PERIOD_MS);
	gpio_set_level(LED_B, 0);


	/* single button */
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);

	/* nes/snes controller gpio */
	gpio_set_pull_mode(NES_CLOCK, GPIO_FLOATING);
	gpio_set_pull_mode(NES_LATCH, GPIO_FLOATING);
	gpio_set_direction(NES_CLOCK, GPIO_MODE_OUTPUT);
	gpio_set_level(NES_CLOCK, 0);
	gpio_set_direction(NES_LATCH, GPIO_MODE_OUTPUT);
	gpio_set_level(NES_LATCH, 0);
	gpio_set_direction(NES_DATA, GPIO_MODE_INPUT);
	gpio_set_pull_mode(NES_DATA, GPIO_PULLUP_ONLY);


	while (true) {
		static uint16_t btns_last = 0;
		static uint32_t js_last = 0;
		static int send_count = 0, timering = 0;
		uint16_t btns = 0;
		uint8_t js1x = 0x80, js1y = 0x80, js2x = 0x80, js2y = 0x80;

		/* read nes */
		gpio_set_level(NES_LATCH, 1);
		ets_delay_us(10);
		gpio_set_level(NES_LATCH, 0);
		ets_delay_us(5);
		for (int i = 0; i < 8; i++) {
			/* read button state */
			btns |= gpio_get_level(NES_DATA) << i;
			/* clock pulse */
			gpio_set_level(NES_CLOCK, 1);
			ets_delay_us(5);
			gpio_set_level(NES_CLOCK, 0);
			ets_delay_us(5);
		}
		btns = ~btns;

		/* convert upper buttons to joystick values */
		if (btns & 0x10) {
			js1x = 0xff;
		} else if (btns & 0x20) {
			js1x = 0x00;
		} else {
			js1x = 0x80;
		}
		if (btns & 0x80) {
			js1y = 0xff;
		} else if (btns & 0x40) {
			js1y = 0x00;
		} else {
			js1y = 0x80;
		}

		/* clear btns to show only buttons */
		btns &= 0x000f;


		/* only transmit if something changed */
		if (btns != btns_last || js_last != ((js2x << 24) | (js2y << 16) | (js1x << 8) | js1y)) {
			send_count = 3;
		}
		if (send_count > 0) {
			ESP_LOGI(LOG_TAG, "send buttons %d JS1 X=%d Y=%d JS2 X=%d Y=%d", btns, js1x, js1y, js2x, js2y);
			esp_hidd_send_joystick_value(hid_conn_id, btns, js1x, js1y, js2x, js2y);
			send_count--;
		}

		/* used to detect state changes which trigger sending a packet */
		btns_last = btns;
		js_last = ((js2x << 24) | (js2y << 16) | (js1x << 8) | js1y);

		/* very simple dummy timer thingie */
		if (timering > 250) {
			static bool toggle = 0;
			static int btn_down = 0;

			/* blue led on when connected, blinking when not */
			if (connected) {
				gpio_set_level(LED_B, 1);
			} else {
				gpio_set_level(LED_B, toggle);
				toggle = !toggle;
			}

			/* if button is down long enough, disconnect if connected or trying to connect */
			if (btn_down > 8 && connected) {
				/* TODO */
				// ESP_LOGI(LOG_TAG, "disconnecting by user request");
			} else if (!gpio_get_level(BUTTON)) {
				btn_down++;
			} else {
				btn_down = 0;
			}

			timering = 0;
		}
		timering++;

		vTaskDelay(1);
	}
}
