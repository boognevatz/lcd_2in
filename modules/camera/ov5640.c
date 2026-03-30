/*****************************************************************************
* | File      	:   ov5640.c
* | Author      :   Waveshare team
* | Function    :   OV5640 configuration function interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-03-13
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "ov5640.h"

// Static variables to hold I2C instance and camera address after sccb_init
static i2c_inst_t *ov5640_i2c = NULL;
static uint8_t ov5640_cam_addr = 0x3c;

/********************************************************************************
 * Function Definitions
 */
/********************************************************************************
function:   Camera register initialization
parameter:
********************************************************************************/
void sccb_init(const uint32_t sda_pin, const uint32_t scl_pin)
{
    ov5640_cam_addr = 0x3c; // default: OV5640
    ov5640_i2c = i2c1;

    // Initialize I2C port at 100 kHz
    i2c_init(ov5640_i2c, 100 * 1000);

    // Initialize I2C pins
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    uint16_t reg;
    reg=OV5640_RD_Reg(ov5640_i2c,ov5640_cam_addr,0X300A);
	reg<<=8;
	reg|=OV5640_RD_Reg(ov5640_i2c,ov5640_cam_addr,0X300B);
    printf("ID: %d \r\n",reg);
    sleep_ms(50);
}

/**
 * @brief Write arbitrary register value to OV5640
 *        Must be called after sccb_init().
 * @param reg: 16-bit register address
 * @param value: 8-bit value to write
 * @return: 0 on success, -1 if not initialized
 */
int ov5640_write_register(uint16_t reg, uint8_t value) {
    if (!ov5640_i2c) return -1;
    OV5640_WR_Reg(ov5640_i2c, ov5640_cam_addr, reg, value);
    return 0;
}

/**
 * @brief Read arbitrary register value from OV5640
 *        Must be called after sccb_init().
 * @param reg: 16-bit register address
 * @return: register value (0-255), or -1 if not initialized
 */
int ov5640_read_register(uint16_t reg) {
    if (!ov5640_i2c) {
        printf("ov5640_read_register: error - not initialized\n");
        return -1;
    }
    uint8_t val = OV5640_RD_Reg(ov5640_i2c, ov5640_cam_addr, reg);
    return val;
}

/**
 * @brief Set the OV5640 DATA ORDER register (0x4745)
 *        Must be called after sccb_init().
 * @param reverse: true for reverse output data bit order, false for normal
 */
void ov5640_set_data_order(bool reverse) {
    if (!ov5640_i2c) return; // Not initialized
    uint8_t val = OV5640_RD_Reg(ov5640_i2c, ov5640_cam_addr, OV5640_REG_DATA_ORDER);
    if (reverse) {
        val |= 0x01; // Set bit 0 for reverse
    } else {
        val &= ~0x01; // Clear bit 0 for normal
    }
    OV5640_WR_Reg(ov5640_i2c, ov5640_cam_addr, OV5640_REG_DATA_ORDER, val);
}

/********************************************************************************
function:   Read 1 byte from the specified register
parameter:
********************************************************************************/
uint8_t OV5640_RD_Reg(i2c_inst_t *i2c,
                  const uint8_t addr,
                  uint16_t reg)
{
    uint8_t val = 0;
    uint8_t reg_high = reg >> 8;
    uint8_t reg_low = reg & 0xFF;

    uint8_t reg_data[2] = {reg_high, reg_low};
    i2c_write_blocking(i2c, addr, reg_data, 2, true);
    i2c_read_blocking(i2c, addr, &val, 1, false);  
    return val;
}

/********************************************************************************
function:   Write 1 byte to the specified register
parameter:
********************************************************************************/
uint8_t OV5640_WR_Reg(i2c_inst_t *i2c,
                  const uint8_t addr,
                  uint16_t reg,
                  uint8_t data)
{
    uint8_t msg[3];
    msg[0] = reg >> 8;
    msg[1] = reg & 0xFF;
    msg[2] = data;

    uint8_t ret=0;
    ret = i2c_write_blocking(i2c, addr, msg, 3, false);
    (void)ret;
    return ret;
}

/********************************************************************************
function:   Write 4 byte to the specified register
parameter:
********************************************************************************/
void OV5640_WR_Reg_2(i2c_inst_t *i2c,
                  const uint8_t addr,
                  uint16_t reg,
                  uint16_t data1,
                  uint16_t data2)
{
    uint8_t msg[3];
    msg[0] = reg >> 8;
    msg[1] = reg & 0xFF;
    msg[2] = data1 >> 8;
    uint8_t ret=0;
    ret = i2c_write_blocking(i2c, addr, msg, 3, false);

    reg+=1;
    msg[0] = reg >> 8;
    msg[1] = reg & 0xFF;
    msg[2] = data1 & 0xFF;
    ret = i2c_write_blocking(i2c, addr, msg, 3, false);

    reg+=1;
    msg[0] = reg >> 8;
    msg[1] = reg & 0xFF;
    msg[2] = data2 >> 8;
    ret=i2c_write_blocking(i2c, addr, msg, 3, false);

    reg+=1;
    msg[0] = reg >> 8;
    msg[1] = reg & 0xFF;
    msg[2] = data2 & 0xFF;
    ret = i2c_write_blocking(i2c, addr, msg, 3, false);

    (void)ret;
}
