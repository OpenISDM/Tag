/*
 Copyright (c) 2016 Academia Sinica, Institute of Information Science

 License:

      GPL 3.0 : The content of this file is subject to the terms and
      conditions defined in file 'COPYING.txt', which is part of this source
      code package.

 Project Name:

      BeDIS

 File Description:

      This file contains the programs executed by location beacons to
      support indoor poositioning and object tracking functions.

 File Name:

      Tag.c

 Version:

       1.0,  20190429

 Abstract:

 Authors:

      Chun Yu Lai, chunyu1202@gmail.com

*/

#include "Tag.h"
#include "zlog.h"

#define Debugging

ErrorCode single_running_instance(char *file_name){
    int retry_time = 0;
    int lock_file = 0;
    struct flock fl;

    retry_time = FILE_OPEN_RETRY;
    while(retry_time--){
        lock_file = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);

        if(-1 != lock_file){
            break;
        }
    }

    if(-1 == lock_file){
        zlog_error(category_health_report,
            "Unable to open lock file");
#ifdef Debugging
        zlog_error(category_debug,
            "Unable to open lock file");
#endif
        return E_OPEN_FILE;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if(fcntl(lock_file, F_SETLK, &fl) == -1){
        zlog_error(category_health_report, "Unable to lock file");
#ifdef Debugging
        zlog_error(category_debug, "Unable to lock file");
#endif
        close(lock_file);
        return E_OPEN_FILE;
    }

    char pids[10];
    snprintf(pids, sizeof(pids), "%d\n", getpid());
    if((size_t)write(lock_file, pids, strlen(pids)) != strlen(pids)){

        zlog_error(category_health_report,
                   "Unable to write pid into lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to write pid into lock file");
#endif
        close(lock_file);

        return E_OPEN_FILE;
    }

    return WORK_SUCCESSFULLY;
}


ErrorCode get_config(Config *config, char *file_name) {
    /* Return value is a struct containing all config information */
    int retry_time = 0;
    FILE *file = NULL;

    /* Create spaces for storing the string of the current line being read */
    char config_setting[CONFIG_BUFFER_SIZE];
    char *config_message = NULL;

    retry_time = FILE_OPEN_RETRY;
    while(retry_time--){
        file = fopen(file_name, "r");

        if(NULL != file){
            break;
        }
    }

    if (NULL == file) {
        zlog_error(category_health_report,
                   "Error openning file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning file");
#endif
        return E_OPEN_FILE;
    }

    /* Keep reading each line and store into the config struct */
    /* item 1 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_dongle_id = atoi(config_message);

    /* item 2 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_rssi_value = atoi(config_message);

    /* item 3 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->scan_rssi_coverage = atoi(config_message);

    /* item 4 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->scan_timeout = atoi(config_message);
    
    /* item 5 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->change_lbeacon_rssi_criteria = atoi(config_message);
    
    /* item 6 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->rssi_movement_per_second = atoi(config_message);

    fclose(file);

    return WORK_SUCCESSFULLY;
}

ErrorCode enable_advertising(int dongle_device_id,
                             int advertising_interval,
                             char *advertising_uuid,
                             int major_number,
                             int minor_number,
                             int rssi_value) {
#ifdef Debugging
    zlog_debug(category_debug, ">> enable_advertising ");
#endif
    int device_handle = 0;
    int retry_time = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    uint8_t segment_length = 1;
    unsigned int *uuid = NULL;
    int uuid_iterator;

#ifdef Debugging
    zlog_info(category_debug, "Using dongle id [%d] uuid [%s]\n", dongle_device_id, advertising_uuid);
#endif
    //dongle_device_id = hci_get_route(NULL);
    if (dongle_device_id < 0){
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_time = SOCKET_OPEN_RETRY;
    while(retry_time--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    le_set_advertising_parameters_cp advertising_parameters_copy;
    memset(&advertising_parameters_copy, 0,
           sizeof(advertising_parameters_copy));
    advertising_parameters_copy.min_interval = htobs(advertising_interval);
    advertising_parameters_copy.max_interval = htobs(advertising_interval);
    /* advertising non-connectable */
    advertising_parameters_copy.advtype = 3;
    /*set bitmap to 111 (i.e., circulate on channels 37,38,39) */
    advertising_parameters_copy.chan_map = 7; /* all three advertising
                                              channels*/

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    request.cparam = &advertising_parameters_copy;
    request.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertise_enable_cp advertisement_copy;
    memset(&advertisement_copy, 0, sizeof(advertisement_copy));
    advertisement_copy.enable = 0x01;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertising_data_cp advertisement_data_copy;
    memset(&advertisement_data_copy, 0, sizeof(advertisement_data_copy));

    /* The Advertising data consists of one or more Advertising Data (AD)
    elements. Each element is formatted as follows:

    1st byte: length of the element (excluding the length byte itself)
    2nd byte: AD type – specifies what data is included in the element
    AD data - one or more bytes - the meaning is defined by AD type
    */

    /* 1. Fill the EIR_FLAGS type (0x01 in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_FLAGS);
    segment_length++;

    /* FLAG information is carried in bits within the flag are as listed below,
    and we choose to use
    0x1A (i.e., 00011010) setting.
    bit 0: LE Limited Discoverable Mode
    bit 1: LE General Discoverable Mode
    bit 2: BR/EDR Not Supported
    bit 3: Simultaneous LE and BR/EDR to Same Device Capable (Controller)
    bit 4: Simultaneous LE and BR/EDR to Same Device Capable (Host)
    bit 5-7: Reserved
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x1A);
    segment_length++;

    /* Fill the length for EIR_FLAGS type (0x01 in Bluetooth AD type) */
    advertisement_data_copy
        .data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    /* 2. Fill the EIR_MANUFACTURE_SPECIFIC_DATA (0xFF in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_MANUFACTURE_SPECIFIC_DATA);
    segment_length++;

    /* The first two bytes of EIR_MANUFACTURE_SPECIFIC_DATA type is the company
    identifier
    https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers

    For Raspberry Pi, we should use 0x000F to specify the manufacturer as
    Broadcom Corporation.
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x0F);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x00);
    segment_length++;

    /* The next byte is Subtype. For beacon-like, we should use 0x02 for iBeacon
    type.
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x02);
    segment_length++;

    /* The next byte is the Subtype length of following beacon-like information.
    They are pre-defined and fixed as 0x15 = 21 bytes with following format:

    16 bytes: Proximity UUID
    2 bytes: Major version
    2 bytes: Minor version
    1 byte: Signal power
    */

    /* Subtype length is pre-defined and fixed as 0x15 for beacon-like
    information*/
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x15);
    segment_length++;

    /* 16 bytes: Proximity UUID */
    uuid = uuid_str_to_data(advertising_uuid);

    for (uuid_iterator = 0;
         uuid_iterator < strlen(advertising_uuid) / 2;
         uuid_iterator++) {

        advertisement_data_copy
            .data[advertisement_data_copy.length + segment_length] =
            htobs(uuid[uuid_iterator]);

        segment_length++;
    }

    /* 2 bytes: Major number */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(major_number >> 8 & 0x00FF);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(major_number & 0x00FF);
    segment_length++;

    /* 2 bytes: Minor number */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(minor_number >> 8 & 0x00FF);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(minor_number & 0x00FF);
    segment_length++;

    /* 1 byte: Signal power (also known as RSSI calibration) */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(twoc(rssi_value, 8));
    segment_length++;

    /* Fill the length for EIR_MANUFACTURE_SPECIFIC_DATA type
    (0xFF in Bluetooth AD type) */
    advertisement_data_copy.data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_DATA;
    request.cparam = &advertisement_data_copy;
    request.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise returned status %d", status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise returned status %d", status);
#endif
        return E_ADVERTISE_STATUS;
    }
#ifdef Debugging
    zlog_debug(category_debug, "<< enable_advertising ");
#endif
    return WORK_SUCCESSFULLY;
}


ErrorCode disable_advertising(int dongle_device_id) {
    int device_handle = 0;
    int retry_time = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    le_set_advertise_enable_cp advertisement_copy;

#ifdef Debugging
    zlog_debug(category_debug,
               ">> disable_advertising ");
#endif
    /* Open Bluetooth device */
    retry_time = DONGLE_GET_RETRY;
    //dongle_device_id = hci_get_route(NULL);
    if (dongle_device_id < 0) {
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_time = SOCKET_OPEN_RETRY;
    while(retry_time--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    memset(&advertisement_copy, 0, sizeof(advertisement_copy));

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#endif
        return E_ADVERTISE_MODE;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise enable on returned status %d",
                   status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise enable on returned status %d",
                   status);
#endif
        return E_ADVERTISE_STATUS;
    }
#ifdef Debugging
    zlog_debug(category_debug,
               "<< disable_advertising ");
#endif

    return WORK_SUCCESSFULLY;
}



/* A static struct function that returns specific bluetooth BLE request. */
const struct hci_request ble_hci_request(uint16_t ocf,
                                         int clen,
                                         void * status,
                                         void * cparam){
    struct hci_request rq;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = ocf;
    rq.cparam = cparam;
    rq.clen = clen;
    rq.rparam = status;
    rq.rlen = 1; /* length of request.rparam */

    return rq;
}

/* A static function to prase the name from the BLE device. */
static ErrorCode eir_parse_uuid(uint8_t *eir,
                           size_t eir_len,
                           char *buf,
                           size_t buf_len){
    size_t offset;
    uint8_t field_len;
    size_t uuid_len;
    int index;
    int i;
	bool has_uuid = false;

    offset = 0;

    while (offset < eir_len) {
        field_len = eir[0];

        /* Check for the end of EIR */
        if (field_len == 0)
            break;

        if (offset + field_len > eir_len)
            goto failed;

        switch (eir[1]) {
            case EIR_MANUFACTURE_SPECIFIC_DATA:
                uuid_len = field_len - 1;
/*
                printf("parse result:\n");
                for(int i = 0 ; i < uuid_len ; i++)
                   printf("%d\n", eir[2+i]);
*/
                if (uuid_len > buf_len)
                       goto failed;

                index = 0;
                // Ensure the Beacon is our LBeacon
                if(eir[2] == 15 && eir[3] == 0 && eir[4] == 2 && eir[5] == 21){

                    if(eir[6] ==0 && eir[7] == 0 && eir[8] ==0){
                        for(i = 6 ; i < 22 ; i++)
                        {
                            buf[index] = eir[i] / 16 + '0';
                            buf[index+1] = eir[i] % 16 + '0';
                            index=index+2;
                        }
                        buf[index] = '\0';
						has_uuid = true;
					}
                }
				if(has_uuid){
					return WORK_SUCCESSFULLY;
				}
				return E_PARSE_UUID;

        }


        offset += field_len + 1;
        eir += field_len + 1;
    }

failed:
    snprintf(buf, buf_len, NULL);

    return E_PARSE_UUID;
}


ErrorCode *start_ble_scanning(void *param){
    /* A buffer for the callback event */
    uint8_t ble_buffer[HCI_MAX_EVENT_SIZE];
    int socket = 0; /* socket number */
    int dongle_device_id = 0; /* dongle id */
    int ret, status;
    struct hci_filter new_filter; /* Filter for controlling the events*/
    evt_le_meta_event *meta;
    le_advertising_info *info;
    le_set_event_mask_cp event_mask_cp;
    int retry_time = 0;
    struct hci_request scan_params_rq;
    struct hci_request set_mask_rq;
    /* Time interval is 0.625ms */
    uint16_t interval = htobs(0x0010); /* 16*0.625ms = 10ms */
    uint16_t window = htobs(0x0010); /* 16*0.625ms = 10ms */
    int i=0;
    char address[LENGTH_OF_MAC_ADDRESS];
    char name[LENGTH_OF_DEVICE_NAME];
    int rssi;
    char uuid[LENGTH_OF_UUID];
    int time_start = get_system_time();
    int lbeacon_index;
    int best_index;
    int best_rssi;
    int num_LBeacons = -1;
    int associated_index = -1;

    memset(LBeacon, 0, sizeof(LBeacon));

#ifdef Debugging
    zlog_debug(category_debug, ">> start_ble_scanning... ");
#endif

    while(true == ready_to_work){
        retry_time = DONGLE_GET_RETRY;
        while(retry_time--){
            dongle_device_id = hci_get_route(NULL);

            if(dongle_device_id >= 0){
                break;
            }
        }

        if (dongle_device_id < 0) {
            zlog_error(category_health_report,
                       "Error openning the device");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning the device");
#endif
            return E_OPEN_DEVICE;
        }

        retry_time = SOCKET_OPEN_RETRY;
        while(retry_time--){
            socket = hci_open_dev(dongle_device_id);

            if(socket >= 0){
                break;
            }
        }
        if (socket < 0) {
            zlog_error(category_health_report,
                       "Error openning socket");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning socket");
#endif
             return E_OPEN_SOCKET;
        }

        if( 0> hci_le_set_scan_parameters(socket, 0x01, interval,
                                          window, 0x00, 0x00,
                                          HCI_SEND_REQUEST_TIMEOUT_IN_MS)){

            zlog_error(category_health_report,
                       "Error setting parameters of BLE scanning");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error setting parameters of BLE scanning");
#endif

        }

        if( 0> hci_le_set_scan_enable(socket, 1, 0,
                                      HCI_SEND_REQUEST_TIMEOUT_IN_MS)){

            zlog_error(category_health_report,
                       "Error enabling BLE scanning");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error enabling BLE scanning");
#endif
        }

        memset(&event_mask_cp, 0, sizeof(le_set_event_mask_cp));

        for (i = 0 ; i < 8 ; i++ ){
            event_mask_cp.mask[i] = 0xFF;
        }

        set_mask_rq = ble_hci_request(OCF_LE_SET_EVENT_MASK,
                                      LE_SET_EVENT_MASK_CP_SIZE,
                                      &status, &event_mask_cp);

        ret = hci_send_req(socket, &set_mask_rq,
                           HCI_SEND_REQUEST_TIMEOUT_IN_MS);

        if ( ret < 0 ) {
            hci_close_dev(socket);
            return E_SCAN_SET_EVENT_MASK;
        }

        hci_filter_clear(&new_filter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
        hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);

        if (0 > setsockopt(socket, SOL_HCI, HCI_FILTER, &new_filter,
                           sizeof(new_filter)) ) {
            hci_close_dev(socket);

            zlog_error(category_health_report,
                       "Error setting HCI filter");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error setting HCI filter");
#endif
        }

        while(true == ready_to_work &&
              HCI_EVENT_HDR_SIZE <=
              read(socket, ble_buffer, sizeof(ble_buffer))){

            meta = (evt_le_meta_event*)
                (ble_buffer + HCI_EVENT_HDR_SIZE + 1);

            if(EVT_LE_ADVERTISING_REPORT != meta->subevent){
                continue;
            }

            info = (le_advertising_info *)(meta->data + 1);

            rssi = (signed char)info->data[info->length];

            if(rssi > g_config.scan_rssi_coverage){
                ba2str(&info->bdaddr, address);
                strcat(address, "\0");

                memset(uuid, 0, sizeof(uuid));

                if(0 != strncmp(address, "C1:", 3) &&
                   WORK_SUCCESSFULLY == eir_parse_uuid(info->data,
                                                       info->length,
                                                       uuid,
                                                       sizeof(uuid) - 1)){

                    if(0 == strncmp(uuid, "000000", 6)){
#ifdef Debugging
                        zlog_debug(category_debug,
                                   "Detected LBeacon  %s, uuid=[%s], rssi=%d",
                                   address, uuid, rssi);
#endif
                        lbeacon_index = -1;
                        for(int i = 0 ; i <= num_LBeacons ; i++){
                            if(0 == strncmp(LBeacon[i].uuid, uuid,
                                            LENGTH_OF_UUID)){
                                lbeacon_index = i;
                                LBeacon[i].avg_rssi =
                                    (LBeacon[i].avg_rssi *
                                     LBeacon[i].count + rssi) /
                                    (LBeacon[i].count + 1);
                                LBeacon[i].count++;
                                break;
                            }
                        }

                        if(lbeacon_index == -1 &&
                           num_LBeacons++ < MAX_INDEX_OF_LBEACON_STRUCT){

                            lbeacon_index = num_LBeacons;
                            memcpy(LBeacon[lbeacon_index].uuid, uuid,
                                   LENGTH_OF_UUID);
                            LBeacon[lbeacon_index].avg_rssi = rssi;
                            LBeacon[lbeacon_index].count = 1;
                        }

                        if(associated_index == -1 && 
                            0 == strncmp(LBeacon[lbeacon_index].uuid,
                                         lbeacon_uuid, LENGTH_OF_UUID)){

                            associated_index = lbeacon_index;
                        }
                    } // end of if lbeacon
                } // end of if "C1" prefix
            } // end of if rssi is higher than threshold

            if(get_system_time() - time_start >= g_config.scan_timeout){
                if(num_LBeacons != -1){
                    best_index = -1;
                    best_rssi = -100;

                    if(associated_index != -1 && 
                       (LBeacon[associated_index].avg_rssi <= 
                        previous_associated_avg_rssi || 
                        LBeacon[associated_index].avg_rssi - 
                        previous_associated_avg_rssi < 
                        g_config.scan_timeout * 
                        g_config.rssi_movement_per_second)){
                        previous_associated_avg_rssi =
                            LBeacon[associated_index].avg_rssi;
#ifdef Debugging
                        zlog_debug(category_debug,
                                   "Scan timeout:  keep association=[%s] rssi=%d",
                                   lbeacon_uuid, LBeacon[associated_index].avg_rssi);
#endif
                    }else{
                        for(int i = 0 ; i <= num_LBeacons ; i++){
                            if(LBeacon[i].avg_rssi > best_rssi){
                                best_rssi = LBeacon[i].avg_rssi;
                                best_index = i;
                            }

#ifdef Debugging
                            zlog_debug(category_debug,
                                       "Scan timeout:  index=[%d], " \
                                       "lbeacon_uuid=[%s], avg_rssi=%d, "\
                                       "count=%d",
                                       i, LBeacon[i].uuid,
                                       LBeacon[i].avg_rssi,
                                       LBeacon[i].count);
#endif
                        }

                        if(associated_index == -1 || 
                           (associated_index != -1 && 
                            best_index != associated_index && 
                            LBeacon[best_index].avg_rssi - 
                            LBeacon[associated_index].avg_rssi > 
                            g_config.change_lbeacon_rssi_criteria)){
#ifdef Debugging
                            zlog_debug(category_debug,
                                       "Scan timeout:  change " \
                                       "best uuid=[%s], " \
                                       "avg_rssi=%d, count=%d",
                                       LBeacon[best_index].uuid,
                                       LBeacon[best_index].avg_rssi,
                                       LBeacon[best_index].count);
#endif
                            memcpy(lbeacon_uuid, LBeacon[best_index].uuid,
                                   LENGTH_OF_UUID);

                            is_lbeacon_changed = true;
                            previous_associated_avg_rssi =
                                LBeacon[best_index].avg_rssi;

                            break;
                        }
                    } // end of else
                } // end of if
                time_start = get_system_time();
                memset(LBeacon, 0, sizeof(LBeacon));
                num_LBeacons = -1;
                associated_index = -1;
            }

        } // end while (HCI_EVENT_HDR_SIZE)

        if( 0> hci_le_set_scan_enable(socket, 0, 0,
                                      HCI_SEND_REQUEST_TIMEOUT_IN_MS)){

            zlog_error(category_health_report,
                       "Error disabling BLE scanning");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error disabling BLE scanning");
#endif
        }

        hci_close_dev(socket);

        if(is_lbeacon_changed){
            break;
        }
    }

#ifdef Debugging
    zlog_debug(category_debug, "<< start_ble_scanning... ");
#endif
    return WORK_SUCCESSFULLY;
}


int main(int argc, char **argv) {
    ErrorCode return_value = WORK_SUCCESSFULLY;
    struct sigaction sigint_handler;

    /*Initialize the global flag */
    ready_to_work = true;
    memset(lbeacon_uuid, 0, sizeof(lbeacon_uuid));
    previous_associated_avg_rssi = -100;

    /* Initialize the application log */
    if (zlog_init("../config/zlog.conf") == 0) {

        category_health_report =
            zlog_get_category(LOG_CATEGORY_HEALTH_REPORT);

        if (!category_health_report) {
            zlog_fini();
        }

#ifdef Debugging
    	 category_debug =
           zlog_get_category(LOG_CATEGORY_DEBUG);

       if (!category_debug) {
           zlog_fini();
       }
#endif
    }

    /* Ensure there is only single running instance */
    return_value = single_running_instance(TAG_LOCK_FILE);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning lock file");
#endif
        return E_OPEN_FILE;
    }

    zlog_info(category_health_report,
              "Tag process is launched...");
#ifdef Debugging
    zlog_info(category_debug,
              "Tag process is launched...");
#endif

    /* Load config struct */
    return_value = get_config(&g_config, CONFIG_FILE_NAME);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning config file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning config file");
#endif
        return E_OPEN_FILE;
    }

    /* Register handler function for SIGINT signal */
    sigint_handler.sa_handler = ctrlc_handler;
    sigemptyset(&sigint_handler.sa_mask);
    sigint_handler.sa_flags = 0;

    if (-1 == sigaction(SIGINT, &sigint_handler, NULL)) {
        zlog_error(category_health_report,
                   "Error registering signal handler for SIGINT");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error registering signal handler for SIGINT");
#endif
    }

    while(true == ready_to_work){
        is_lbeacon_changed = false;

        return_value = enable_advertising(g_config.advertise_dongle_id,
                                          INTERVAL_ADVERTISING_IN_MS,
                                          lbeacon_uuid,
                                          LBEACON_MAJOR_VER,
                                          LBEACON_MINOR_VER,
                                          g_config.advertise_rssi_value);
        void *param;
        start_ble_scanning(param);

        disable_advertising(g_config.advertise_dongle_id);
    }

    return WORK_SUCCESSFULLY;
}
