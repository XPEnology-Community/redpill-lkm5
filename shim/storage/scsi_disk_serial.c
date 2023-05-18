#include<scsi/scsi_cmnd.h>
#include<scsi/scsi_device.h>
#include<scsi/scsi_host.h>

#include "../../common.h"

int rp_scsi_device_disk_name_match(struct device *dev, const void *data)
{
    struct Scsi_Host *shost;
    struct scsi_device *sdev;
    int found = 0;
    char * blk_name = *(char **)data;

    shost = class_to_shost(dev);
    shost_for_each_device(sdev, shost){
        if (strcmp(blk_name, sdev->syno_disk_name) == 0) {
            pr_loc_dbg("scsi host no: %d, device id: %d, name: %s, serial: %s",
                shost->host_no, sdev->id, sdev->syno_disk_name, sdev->syno_disk_serial);
            found = 1;
        }
    }

    return found == 1;
}

// refer from scsi_host_lookup
struct Scsi_Host * rp_search_scsi_host_by_blk_name(struct class * shost_class, char * blk_name)
{
    struct device *cdev;
    struct Scsi_Host *shost = NULL;

    cdev = class_find_device(shost_class, NULL, &blk_name, rp_scsi_device_disk_name_match);
    if (cdev) {
        shost = scsi_host_get(class_to_shost(cdev));
        put_device(cdev);
    }
    return shost;
}

char * rp_fetch_block_serial(char * blk_name) {
    struct Scsi_Host * shost;
    struct scsi_device * sdev;
    struct class * shost_class;

    char * serial = NULL;

    // find the first scsi host to get shost class
    shost = scsi_host_lookup(0);
    if (shost == NULL) {
        printk(KERN_ALERT "shost 0 not found\n");
        return serial;
    }

    shost_class = shost->shost_dev.class;
    scsi_host_put(shost);

    shost = rp_search_scsi_host_by_blk_name(shost_class, blk_name);
    if (shost == NULL) {
        printk(KERN_ALERT "shost not found by block name %s\n", blk_name);
        return serial;
    }

    shost_for_each_device(sdev, shost){
        if (strcmp(blk_name, sdev->syno_disk_name) == 0) {
            serial = sdev->syno_disk_serial;
        }
    }

    scsi_host_put(shost);
    return serial;
}
