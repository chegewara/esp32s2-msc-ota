/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/**
 * Updated by:
 * chagewara
 */
#include "Arduino.h"
#include "tusb.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
extern "C" {
#include "boards.h"
}
#if CFG_TUD_MSC
const esp_partition_t *ota_0;

static bool idf_flash;
static uint32_t _lba = 0;
static long old_millis;
static uint32_t _offset = 0;

/**
 * Task used as workaround to detect when update has been finished
 */
static void ticker_task(void* p)
{
  while(1)
  {
    if (millis() - old_millis > 1000)
    {
      board_led_state(STATE_WRITING_FINISHED);
      esp_err_t err = esp_ota_set_boot_partition(ota_0);
      if(err)
        ESP_LOGE("", "BOOT ERR => %x [%d]", err, _offset);

      board_led_state(10);
      esp_restart();
    }
    delay(100);
  }
}

// Some MCU doesn't have enough 8KB SRAM to store the whole disk
// We will use Flash as read-only disk with board that has
// CFG_EXAMPLE_MSC_READONLY defined

#define README_CONTENTS \
  "This is tinyusb's MassStorage Class used to OTA update.\r\n\r\n\
  It's a part of esp32 S2 simple demo to show native USB functionality \r\n\
  used to OTA update by simple drag n drop to disk drive. \r\n"

enum
{
  DISK_BLOCK_NUM = 2 * 50, // 8KB is the smallest size that windows allow to mount
  DISK_BLOCK_SIZE = 512
};

#ifdef CFG_EXAMPLE_MSC_READONLY
const
#endif
uint8_t *msc_disk;
uint8_t _msc_disk[50][DISK_BLOCK_SIZE] =
    {
        //------------- Block0: Boot Sector -------------//
        // byte_per_sector    = DISK_BLOCK_SIZE; fat12_sector_num_16  = DISK_BLOCK_NUM;
        // sector_per_cluster = 1; reserved_sectors = 1;
        // fat_num            = 1; fat12_root_entry_num = 16;
        // sector_per_fat     = 1; sector_per_track = 1; head_num = 1; hidden_sectors = 0;
        // drive_number       = 0x80; media_type = 0xf8; extended_boot_signature = 0x29;
        // filesystem_type    = "FAT12   "; volume_serial_number = 0x1234; volume_label = "TinyUSB MSC";
        // FAT magic code at offset 510-511
        {
            0xEB, 0x3C, 0x90, 
            0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 
            0x00, 0x02, 
            0x10, 
            0x01, 0x00,
            0x01, 
            0x10, 0x00, 
            0x10, 0x00, 
            0xF8, 
            0x01, 0x00, 
            0x01, 0x00, 
            0x01, 0x00, 
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x29, 0x34, 0x12, 0x56, 0x78, 
            'E',  'S',  'P',  '3',  '2', 'S', '2', ' ', 'M', 'S', 'C', 
            0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x00, 0x00,

            // Zero up to 2 last bytes of FAT magic code
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA},

        //------------- Block1: FAT12 Table -------------//
        {
            0xF8, 0xFF, 0xFF, 0xFF, 0x0F // // first 2 entries must be F8FF, third entry is cluster end of readme file
        },

        //------------- Block2: Root Directory -------------//
        {
            // first entry is volume label
            'E', 'S', 'P', '3', '2', 'S', '2', ' ', 'M', 'S', 'C', 0x08, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x6D, 0x65, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // second entry is readme file
            'R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'T', 0x20,
            0x00, 0xC6, 0x52, 0x6D,
            0x65, 0x43, 0x65, 0x43, 0x00, 0x00,
            0x88, 0x6D, 0x65, 0x43,                       // date and time
            0x02, 0x00,                                   // sector
            sizeof(README_CONTENTS) - 1, 0x00, 0x00, 0x00 // readme's files size (4 Bytes)
        },

        //------------- Block3: Readme Content -------------//
        README_CONTENTS
    };


void check_running_partition(void)
{
  const esp_partition_t* _part_ota0 = esp_ota_get_running_partition();

  ESP_LOGI("", "TYPE => %d", _part_ota0->type);
  ESP_LOGI("", "SUBTYPE => %02x", _part_ota0->subtype);
  ESP_LOGI("", "ADDRESS => %x", _part_ota0->address);
  ESP_LOGI("", "SIZE => %x", _part_ota0->size);
}

bool init_disk()
{
  board_init();
  check_running_partition();
  ota_0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_0");
  msc_disk = (uint8_t*)heap_caps_calloc(1, DISK_BLOCK_SIZE * DISK_BLOCK_NUM, MALLOC_CAP_32BIT);
  if(msc_disk == NULL) return false;
  memcpy(msc_disk, _msc_disk, sizeof(_msc_disk));
  msc_disk[20] = (uint8_t)(ota_0->size / DISK_BLOCK_SIZE >> 8);
  msc_disk[19] = (uint8_t)(ota_0->size / DISK_BLOCK_SIZE & 0xff);
  board_led_state(STATE_BOOTLOADER_STARTED);
  return true;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void)lun;

  const char vid[] = "Espressif";
  const char pid[] = "Mass Storage";
  const char rev[] = "1.0";

  memcpy(vendor_id, vid, strlen(vid));
  memcpy(product_id, pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void)lun;

  return true; // RAM disk is always ready
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
  board_led_state(STATE_USB_MOUNTED);

  (void)lun;
  if (ota_0 == NULL)
  {
    *block_count = DISK_BLOCK_NUM;
    *block_size = DISK_BLOCK_SIZE;
  }
  else
  {
    *block_count = ota_0->size / DISK_BLOCK_SIZE;
    *block_size = DISK_BLOCK_SIZE;
  }
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void)lun;
  (void)power_condition;

  if (load_eject)
  {
    if (start)
    {
      // load disk storage
    }
    else
    {
      // unload disk storage
    }
  }

  return true;
}
// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
  (void)lun;
  if (ota_0 == NULL || lba < 50) // first 50 lba is RAM disk, 50+ its ota partition
  {
    uint8_t *addr = &msc_disk[lba * 512] + offset;
    memcpy(buffer, addr, bufsize);
  }
  else
  {
    esp_partition_read(ota_0, (lba * 512) + offset - 512, buffer, bufsize);
  }
  return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
  esp_err_t err = 0;
  (void)lun;
  if(buffer[0] == 0xe9 && !idf_flash) { // we presume that we are having beginning of esp32 binary file when we see magic number at beginning of buffer
    ESP_LOGI("", "start flash");
    board_led_state(STATE_WRITING_STARTED);

    idf_flash = true;
    _lba = lba;
    esp_partition_erase_range(ota_0, 0x0, ota_0->size);
    old_millis = millis();
    xTaskCreate(ticker_task, "tT", 3*1024, NULL, 1, NULL);
  }

  if ( !idf_flash)
  {
    uint8_t *addr = &msc_disk[lba * 512] + offset;
    memcpy(addr, buffer, bufsize);
  }
  else
  {
    if(lba < _lba) {
      // ignore LBA that is lower than start update LBA, it is most likely FAT update
      return bufsize;
    }
    err = esp_partition_write(ota_0, _offset, buffer, bufsize);
    _offset += bufsize;
  }
  old_millis = millis();

  return bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const *response = NULL;
  uint16_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
  case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
    // Host is about to read/write etc ... better not to disconnect disk
    resplen = 0;
    break;

  default:
    // Set Sense = Invalid Command Operation
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

    // negative means error -> tinyusb could stall and/or response with failed status
    resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if (resplen > bufsize)
    resplen = bufsize;

  if (response && (resplen > 0))
  {
    if (in_xfer)
    {
      memcpy(buffer, response, resplen);
    }
    else
    {
      // SCSI output
    }
  }

  return resplen;
}

#endif
