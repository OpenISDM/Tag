/*
Copyright (c) 2016 Academia Sinica, Institute of Information Science

License:

    GPL 3.0 : The content of this file is subject to the terms and
    conditions defined in file 'COPYING.txt', which is part of this source
    code package.

Project Name:

    BeDIS

File Description:

    This header file contains declarations of variables, structs and
    functions and definitions of global variables used in the LBeacon.c file.

File Name:

    Tag.h

Version:

    1.0,  20190429

Abstract:

Authors:

    Chun Yu Lai, chunyu1202@gmail.com

*/

#ifndef TAG_H
#define TAG_H

/*
* INCLUDES
*/

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <netinet/in.h>
#include <obexftp/client.h>
#include "BeDIS.h"
#include "Version.h"

/*
  CONSTANTS
*/

/* Command opcode pack/unpack from HCI library. ogf and ocf stand for Opcode
   group field and Opcofe command field, respectively. See Bluetooth
   specification - core version 4.0, vol.2, part E Chapter 5.4 for details.
*/
//#define cmd_opcode_pack(ogf, ocf) (uint16_t)((ocf &amp; 0x03ff) | \
//                                                        (ogf &lt;&lt; 10))
/* File path of the config file of the Tag */
#define CONFIG_FILE_NAME "/home/pi/Tag/config/config.conf"

/* File path of the logging file*/
#define LOG_FILE_NAME "/home/pi/Tag/config/zlog.conf"

/* The category defined of log file used for health report */
#define LOG_CATEGORY_HEALTH_REPORT "Health_Report"

/* The category defined for the printf during debugging */
#define LOG_CATEGORY_DEBUG "Tag_Debug"

/* The lock file for Tag  */
#define TAG_LOCK_FILE "/home/pi/Tag/bin/Tag.pid"

/* For following EIR_ constants, please refer to Bluetooth specifications for
the defined values.
https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
*/
/* BlueZ bluetooth extended inquiry response protocol: flags */
#define EIR_FLAGS 0X01

/* BlueZ bluetooth extended inquiry response protocol: short local name */
#define EIR_NAME_SHORT 0x08

/* BlueZ bluetooth extended inquiry response protocol: complete local name */
#define EIR_NAME_COMPLETE 0x09

/* BlueZ bluetooth extended inquiry response protocol: Manufacturer Specific
   Data */
#define EIR_MANUFACTURE_SPECIFIC_DATA 0xFF

/* Timeout in milliseconds of hci_send_req funtion */
#define HCI_SEND_REQUEST_TIMEOUT_IN_MS 1000

/* Time interval in milliseconds between advertising by a LBeacon */
#define INTERVAL_ADVERTISING_IN_MS 500

/* Number of characters in the name of a Bluetooth device */
#define LENGTH_OF_DEVICE_NAME 30

/* Number of characters in the uuid of a Bluetooth device */
#define LENGTH_OF_UUID 33

/* Number of characters in a Bluetooth MAC address */
#define LENGTH_OF_MAC_ADDRESS 18

/* The maximum index of LBeacon struct for comparison signal strength */
#define MAX_INDEX_OF_LBEACON_STRUCT 50

/*
  TYPEDEF STRUCTS
*/
typedef struct LBeacon_data {
   char uuid[LENGTH_OF_UUID];
   int avg_rssi;
   int count;
} LBeacon_data;

LBeacon_data LBeacon[MAX_INDEX_OF_LBEACON_STRUCT];
int index_LBeacon = -1;

/* The configuration file structure */

typedef struct Config {
    int advertise_dongle_id;

    /* The rssi value used to advertise */
    int advertise_rssi_value;

    /* The required signal strength */
    int scan_rssi_coverage;

    /* The time window */
    int scan_timeout;

} Config;



/*
  EXTERN STRUCTS
*/

/* In sys/poll.h, the struct for controlling the events. */
//extern struct pollfd;

/* In hci_sock.h, the struct for callback event from the socket. */
//extern struct hci_filter;

/*
  EXTERNAL GLOBAL VARIABLES
*/

extern int errno;

/*
  GLOBAL VARIABLES
*/

/* Struct for storing config information from the input file */
Config g_config;

char lbeacon_uuid[40];

int is_uuid_changed;
//int prevent_bounce_count;

/*
  FUNCTIONS
*/

/*
  single_running_instance:

      This function write a file lock to ensure that system has only one
      instance of running LBeacon.

  Parameters:
      file_name - the name of the lock file that specifies PID of running
                  LBeacon

  Return value:
      ErrorCode - indicate the result of execution, the expected return code
                  is WORK_SUCCESSFULLY

*/

ErrorCode single_running_instance(char *file_name);

/*
  generate_uuid:

      This function generates the UUID of this LBeacon according to the 3D
      coordinates read from configuration file.

  Parameters:
      config - Config struct including file path, coordinates, etc.

  Return value:
      ErrorCode - indicate the result of execution, the expected return code
                  is WORK_SUCCESSFULLY

*/

ErrorCode generate_uuid(Config *config);

/*
  get_config:

      This function reads the specified config file line by line until the
      end of file and copies the data in the lines into the Config struct
      global variable.

  Parameters:
      config - Pointer to config struct including file path, coordinates, etc.
      file_name - the name of the config file that stores all the beacon data

  Return value:

      ErrorCode - indicate the result of execution, the expected return code
                  is WORK_SUCCESSFULLY
*/

ErrorCode get_config(Config *config, char *file_name);

/*
  enable_advertising:

      This function enables the LBeacon to start advertising, sets the time
      interval for advertising, and calibrates the RSSI value.

  Parameters:

      dongle_device_id - the bluetooth dongle device which the LBeacon uses
                         to advertise
      advertising_interval - the time interval during which the LBeacon can
                         advertise
      advertising_uuid - universally unique identifier of advertiser
      major_number - major version number of LBeacon
      minor_number - minor version number of LBeacon
      rssi_value - RSSI value of the bluetooth device

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode enable_advertising(int dongle_device_id,
                             int advertising_interval,
                             char *advertising_uuid,
                             int major_number,
                             int minor_number,
                             int rssi_value);

/*
  disable_advertising:

      This function disables advertising of the beacon.

  Parameters:

      dongle_device_id - the bluetooth dongle device which the LBeacon needs to
                         disable advertising function

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode disable_advertising(int dongle_device_id);

/*
  ble_hci_request:

      This function prepares the bluetooh BLE request specified by the input
      parameters

  Parameters:

      ocf - an argument used by bluetooth BLE request
      clen - an argument used by bluetooth BLE request
      status - an argument used by bluetooth BLE request
      cparam - an argument used by bluetooth BLE request

  Return value:

      struct hci_request - the bluetooth BLE request specified by the input
                           parameters
*/

const struct hci_request ble_hci_request(uint16_t ocf,
                                         int clen,
                                         void * status,
                                         void * cparam);

/*
  eir_parse_uuid:

      This function parses the uuid from bluetooth BLE device

  Parameters:

      eir - the data member of the advertising information result
            from bluetooth BLE scan result
      eir_len - the length in number of bytes of the eir argument
      buf - the output buffer to receive the parsing result
      buf_len - the length in number of bytes of the buf argument

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

static ErrorCode eir_parse_uuid(uint8_t *eir,
                                size_t eir_len,
                                char *buf,
                                size_t buf_len);

/*
  start_ble_scanning:

      This function scans continuously for BLE bluetooth devices under the
      coverage of the beacon until scanning is cancelled. To reduce the
      traffic among BeDIS system, this function only tracks the tags with
      our specific name. When a tag with specific name is found, this
      function calls send_to_push_dongle to either add a new ScannedDevice
      struct of the device to ble_object_list or update the final scan time
      of a struct in the list.
      [N.B. This function is executed by the main thread. ]

  Parameters:

      param - not used. This parameter is defined to meet the definition of
              pthread_create() function

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode *start_ble_scanning(void *param);

/*
  EXTERNAL FUNCTIONS
*/

/*
  opendir:

      This function is called to open a specified directory.

  Parameters:

      dirname - the name of the directory to be opened.

  Return value:

      dirp - a pointer to the directory stream.
*/

extern DIR *opendir(const char *dirname);

/*
  memset:

      This function is called to fill a block of memory.

  Parameters:

      ptr - the pointer points to the memory area
      value - the int value passed to the function which fills the blocks of
              memory using unsinged char convension of this value
      number - number of bytes in the memory area starting from ptr to be
               filled

  Return value:

      dst - a pointer to the memory area
*/

extern void * memset(void * ptr, int value, size_t number);

/*
  hci_open_dev:

      This function is called to open a Bluetooth socket with the specified
      resource number.

  Parameters:

      dev_id - the id of the Bluetooth socket device

  Return value:

      dd - device descriptor of the Bluetooth socket
*/

extern int hci_open_dev(int dev_id);

/*
  hci_filter_clear:

      This function is called to clear a specified filter.

  Parameters:

      f - the filter to be cleared

  Return value:

      None
*/

extern void hci_filter_clear(struct hci_filter *f);

/*
  hci_filter_set_ptype:

      This function is called to let filter set ptype.

  Parameters:

      t - the type
      f - the filter to be set

  Return value:

      None
*/

extern void hci_filter_set_ptype(int t, struct hci_filter *f);

/*
  hci_filter_set_event:

      This function is called to let filter set event

  Parameters:

      e - the event
      f - the filter to be set

  Return value:

      None
*/

extern void hci_filter_set_event(int e, struct hci_filter *f);

/*
  hci_write_inquiry_mode:

      This function is called to configure inquiry mode

  Parameters:

      dd - device descriptor of the open HCI socket
      mode - new inquiry mode
      to -

  Return value:

      None
*/

extern int hci_write_inquiry_mode(int dd, uint8_t mode, int to);

/*
  hci_send_cmd:

      This function is called to send cmd

  Parameters:

      dd - device descriptor of the open HCI socket
      ogf - opcode group field
      ocf - opcode command field
      plen - the length of the command parameters
      param - the parameters that function runs with

  Return value:

      0 for success. error number for error.
*/

extern int  hci_send_cmd(int dd, uint16_t ogf, uint16_t ocf, uint8_t plen,
                         void *param);

#endif
