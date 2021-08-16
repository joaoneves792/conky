#include "kraken.h"
#include "conky.h"
#include "logging.h"
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libudev.h>
struct kraken_state kraken_current_state;
int kraken_initialized = 0;

int find_kraken_device(struct udev* udev, char* device, int c){
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_subsystem(enumerate, KRAKEN_SUBSYSTEM);
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry* entry;

  udev_list_entry_foreach(entry, devices) {
      const char* nodepath;
      const char* path = udev_list_entry_get_name(entry);
      struct udev_device* dev = udev_device_new_from_syspath(udev, path);
      if (dev) {
        path = udev_device_get_devnode(dev);
        if (path){
           dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
           if(!dev){
             udev_device_unref(dev);
             continue;
           }
           const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
           const  char* product = udev_device_get_sysattr_value(dev, "idProduct");
           if(!strncmp(vendor, KRAKEN_NZXT_X_SERIES_VENDOR, 4) && !strncmp(product, KRAKEN_NZXT_X_SERIES_PRODUCT, 4)){
              strncpy(device, path, c);
              udev_device_unref(dev);
              udev_enumerate_unref(enumerate);
              return 1;
           }
        }
        udev_device_unref(dev);
      }
  }
  udev_enumerate_unref(enumerate);
  return 0;
}

int init_kraken(){
	int i, res, desc_size = 0;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;
	char device[KRAKEN_MAX_DEV_PATH_CHARS];

  kraken_current_state.fd = -1;

  struct udev* udev = udev_new();
  if (!udev) {
    NORM_ERR("%s: udev_new() failed", __func__);
    return 1;
  }

  if(!find_kraken_device(udev, device, KRAKEN_MAX_DEV_PATH_CHARS)){
    udev_unref(udev);
    NORM_ERR("%s: No Kraken device found", __func__);
    return 1;
  }
  udev_unref(udev);

  kraken_current_state.fd = open(device, O_RDWR | O_NONBLOCK);

	if (kraken_current_state.fd < 0) {
    NORM_ERR("%s: Unable to open device", __func__);
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));
  
  /*Initialize device*/
  buf[0]  = 0x10;
  buf[1] = 0x01;
  res = write(kraken_current_state.fd, buf, 2);
	if (res < 0) {
    NORM_ERR("%s: Failed to init  Kraken device", __func__);
    return 1;
	}
  buf[0]  = 0x20;
  buf[1] = 0x03;
  res = write(kraken_current_state.fd, buf, 2);
	if (res < 0) {
    NORM_ERR("%s: Failed to init  Kraken device", __func__);
    return 1;
	}
  buf[0]  = 0x70;
  buf[1] = 0x02;
  buf[2]  = 0x01;
  buf[3] = 0xb8;
  buf[4] = 0x01;
  res = write(kraken_current_state.fd, buf, 5);
	if (res < 0) {
    NORM_ERR("%s: Failed to init  Kraken device", __func__);
    return 1;
	}
  buf[0]  = 0x70;
  buf[1] = 0x01;
  res = write(kraken_current_state.fd, buf, 2);
	if (res < 0) {
    NORM_ERR("%s: Failed to init  Kraken device", __func__);
    return 1;
	}
	
  res = read(kraken_current_state.fd, buf, KRAKEN_REPORT_SIZE);
	if (res < 0) {
    NORM_ERR("%s: Failed to read report from Kraken device", __func__);
    return 1;
	}
  
  kraken_current_state.rpm = 0;
  kraken_current_state.liquid_temp = 0;
  kraken_current_state.duty = 0;
  return 0;
}

int update_kraken(){
  char buf[KRAKEN_REPORT_SIZE];
  if(!kraken_initialized){
    init_kraken();
    kraken_initialized = 1;
  }
  if(kraken_current_state.fd < 0){
    return 1;
  }
  
  int res;
  //read through any queued stale reports
  do{
    res = read(kraken_current_state.fd, buf, KRAKEN_REPORT_SIZE);
  }while(res > 0); 
  //Get the latest report
  do{
    res = read(kraken_current_state.fd, buf, KRAKEN_REPORT_SIZE);
  }while((res < 0 && !usleep(1000)) || buf[0] != 0x75); 

  kraken_current_state.liquid_temp = buf[15]+buf[16]/10.0;
  kraken_current_state.rpm = (buf[18] << 8) | buf[17];
  kraken_current_state.duty = buf[19];
  return 0;
}

void print_kraken_rpm(struct text_object *obj, char *p, unsigned int p_max_size){
    snprintf(p, p_max_size, "%d", kraken_current_state.rpm);
}
void print_kraken_liquid_temp(struct text_object *obj, char *p, unsigned int p_max_size){
    snprintf(p, p_max_size, "%.1f", kraken_current_state.liquid_temp);
}
void print_kraken_duty(struct text_object *obj, char *p, unsigned int p_max_size){
    snprintf(p, p_max_size, "%d", kraken_current_state.duty);
}
void free_kraken(struct text_object * obj){
  (void)obj;
  close(kraken_current_state.fd);
  kraken_initialized = 0;
  kraken_current_state.fd = -1;
}
