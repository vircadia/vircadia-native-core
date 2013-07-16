//
//  inv_tty.h
//  interface
//
//  Created by Andrzej Kapolka on 7/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__inv_tty__
#define __interface__inv_tty__

void tty_set_file_descriptor(int file_descriptor);

int tty_i2c_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char const *data);

int tty_i2c_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data);

void tty_delay_ms(unsigned long num_ms);

void tty_get_ms(unsigned long *count);

#endif /* defined(__interface__inv_tty__) */
