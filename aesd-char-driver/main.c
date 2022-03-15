/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Ruchit Naik"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	//Getting pointer to the container structure
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

	//Saving the pointer in private_data of the passed file pointer
	filp->private_data = dev;
	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	//return 0 as there is no hardware to shutdown in character driver
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
	struct aesd_buffer_entry *pPos = NULL;		//Pointer to the position returned by function
	ssize_t entry_offset;						//stores the offset value from where command is to be read
	ssize_t read_count;							//Stores the count of number of characters read
	ssize_t unread_count;						//Stores the number of bytes left to be read, if any
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */
	//Error handling
	if(!count){
		return 0;
	}

	if(filp == NULL || buf == NULL || f_pos == NULL){
		//Passing NULL pointer
		return -EFAULT;
	}

	//acquire interruptible mutex
	if(mutex_lock_interruptible(&aesd_device.charMutex) != 0){
		PDEBUG("Failed to acquire mutex");
		return -ERESTARTSYS;
	}

	//find the position to read from
	pPos = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buf, *f_pos, &entry_offset);
	if(pPos == NULL){
		//In case if no matching entry found
		goto out;								//Error case, jump to cleanup section
	}

	//Calculate number of bytes read
	read_count = pPos->size - entry_offset;
	if(count > read_count){
		//limit the count if it exceed the read count
		count = read_count;
	}

	//Copy from kernel space into user space
	unread_count = copy_to_user(buf, (pPos->buffptr+entry_offset), read_count);
	retval = count - unread_count;				//amount of data is read successfully
	*f_pos += retval;							//update the pointer to point to next command in the queue
out:
	mutex_unlock(&aesd_device.charMutex);			//Unlock the mutex on each exit
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	ssize_t write_count = 0;
	const char *write_entry = NULL;
	struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	//Error handling
	if(count == 0){
		return 0;
	}
	if(filp == NULL || buf == NULL || f_pos == NULL){
		//Passing NULL pointer
		return -EFAULT;
	}

		//acquire interruptible mutex
	if(mutex_lock_interruptible(&aesd_device.charMutex) != 0){
		PDEBUG("Failed to acquire mutex");
		return -ERESTARTSYS;
	}

	if(dev->entry.size == 0){
		//Allocate the buffer for the first time
		dev->entry.buffptr = kmalloc((sizeof(char)*count), GFP_KERNEL);

		if(dev->entry.buffptr == NULL){
			PDEBUG("Error in initial allocation");
			retval = -ENOMEM;
			goto clear;
		}
	}
	else{
		//if already allocated
		dev->entry.buffptr = krealloc(dev->entry.buffptr, (dev->entry.size + count)*sizeof(char), GFP_KERNEL);
		if(dev->entry.buffptr == NULL){
			PDEBUG("Error while reallocation");
			goto clear;
		}
	}

	//copy from user space to kernel space
	write_count = copy_from_user((void *)(&dev->entry.buffptr[dev->entry.size]), buf, count);
	//Check the number of bytes actually writtern
	retval = count - write_count;
	//Increment the size by the actual number of bytes writtern
	dev->entry.size += retval;

	if(memchr(dev->entry.buffptr, '\n', dev->entry.size)){
		//newline character spotted. Enqueue only when '\n' received
		write_entry = aesd_circular_buffer_add_entry(&dev->buf, &dev->entry);		//Enqueue the recevied commands
		if(write_entry){
			//Free returned memory if queue is full
			kfree(write_entry);
		}
		dev->entry.buffptr = NULL;
		dev->entry.size = 0;
	}

clear:
	mutex_unlock(&aesd_device.charMutex);			//Unlock the mutex on each exit
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	//Initialize mutex
	mutex_init(&aesd_device.charMutex);
	//Initialize circular buffer
	aesd_circular_buffer_init(&aesd_device.buf);

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	//free circular buffer entries
	struct aesd_buffer_entry *entry = NULL;
	uint8_t index = 0;

	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	//Free all the entries allocated in the buffer
	AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buf, index){
		if(entry->buffptr != NULL){
			//Free if the pointer to the entry exists
			kfree(entry->buffptr);
		}
	}

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
