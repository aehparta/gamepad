// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>
#include <string.h>
#include "esp_hidd_prf_api.h"
#include "hidd_le_prf_int.h"
#include "esp_log.h"
#include "hid_dev.h"


esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks)
{
	esp_err_t hidd_status;

	if (callbacks != NULL) {
		hidd_le_env.hidd_cb = callbacks;
	} else {
		return ESP_FAIL;
	}

	if ((hidd_status = hidd_register_cb()) != ESP_OK) {
		return hidd_status;
	}

	esp_ble_gatts_app_register(BATTRAY_APP_ID);

	if ((hidd_status = esp_ble_gatts_app_register(HIDD_APP_ID)) != ESP_OK) {
		return hidd_status;
	}

	return hidd_status;
}

esp_err_t esp_hidd_profile_init(void)
{
	if (hidd_le_env.enabled) {
		ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
		return ESP_FAIL;
	}
	// Reset the hid device target environment
	memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
	hidd_le_env.enabled = true;
	return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
	uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
	if (!hidd_le_env.enabled) {
		ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
		return ESP_OK;
	}

	if (hidd_svc_hdl != 0) {
		esp_ble_gatts_stop_service(hidd_svc_hdl);
		esp_ble_gatts_delete_service(hidd_svc_hdl);
	} else {
		return ESP_FAIL;
	}

	/* register the HID device profile to the BTA_GATTS module */
	esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);

	return ESP_OK;
}

void esp_hidd_send_joystick_value(uint16_t conn_id, uint16_t joystick_buttons, uint8_t joystick_x, uint8_t joystick_y, uint8_t joystick_z, uint8_t joystick_rx)
{
	uint8_t buffer[6];
	ESP_LOGD(HID_LE_PRF_TAG, "the buttons value = %d js1 = %d, %d js2 = %d, %d", joystick_buttons, joystick_x, joystick_y, joystick_z, joystick_rx);

	buffer[0] = joystick_buttons & 0xff;
	buffer[1] = joystick_buttons >> 8;
	buffer[2] = joystick_x ^ 0x80;
	buffer[3] = joystick_y ^ 0x80;
	buffer[4] = joystick_z ^ 0x80;
	buffer[5] = joystick_rx ^ 0x80;

	hid_dev_send_report(hidd_le_env.gatt_if, conn_id, HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, sizeof(buffer), buffer);
}



