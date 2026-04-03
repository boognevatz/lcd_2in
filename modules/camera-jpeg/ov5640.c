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
#include "py/mpprint.h"

// Define the default register settings for the OV5640 sensor
const uint16_t sensor_default_regs[][2] = {
    {SYSTEM_CTROL0, 0x82},  // software reset
    {0XFFFF, 10}, // delay 10ms
    {SYSTEM_CTROL0, 0x42},  // power down

    //enable pll
    {0x3103, 0x13},

    //io direction
    {0x3017, 0xff},
    {0x3018, 0xff},

    {DRIVE_CAPABILITY, 0xc3},
    // PCLK ungated; PIO gates capture by HREF via jmp_pin.
    {CLOCK_POL_CONTROL, 0x21},

    {0x4713, 0x02},//jpg mode select

    {ISP_CONTROL_01, 0x83}, // turn color matrix, awb and SDE

    //sys reset
    {0x3000, 0x20}, // reset MCU
    {0XFFFF, 10}, // delay 10ms
    {0x3002, 0x1c},

    //clock enable
    {0x3004, 0xff},
    {0x3006, 0xc3},

    //isp control
    {0x5000, 0xa7},
    {ISP_CONTROL_01, 0xa3},//+scaling?
    {0x5003, 0x08},//special_effect

    //unknown
    {0x370c, 0x03},//analog bias (all ref drivers use 0x03, OV5640-1B fix)
    {0x3634, 0x40},//!!IMPORTANT

    //AEC/AGC
    {0x3a02, 0x03},
    {0x3a03, 0xd8},
    {0x3a08, 0x01},
    {0x3a09, 0x27},
    {0x3a0a, 0x00},
    {0x3a0b, 0xf6},
    {0x3a0d, 0x04},
    {0x3a0e, 0x03},
    {0x3a0f, 0x28},//ae_level high (tuned for highlight/detail balance)
    {0x3a10, 0x20},//ae_level low
    {0x3a11, 0x60},//ae_level
    {0x3a13, 0x43},
    {0x3a14, 0x03},
    {0x3a15, 0xd8},
    {0x3a18, 0x00},//gainceiling
    {0x3a19, 0xf8},//gainceiling
    {0x3a1b, 0x28},//ae_level fast-mode high
    {0x3a1e, 0x18},//ae_level fast-mode low
    {0x3a1f, 0x14},//ae_level

    //vcm debug
    {0x3600, 0x08},
    {0x3601, 0x33},

    //50/60Hz
    {0x3c01, 0xa4},
    {0x3c04, 0x28},
    {0x3c05, 0x98},
    {0x3c06, 0x00},
    {0x3c07, 0x08},
    {0x3c08, 0x00},
    {0x3c09, 0x1c},
    {0x3c0a, 0x9c},
    {0x3c0b, 0x40},

    {0x460c, 0x22},//disable jpeg footer

    //BLC
    {0x4001, 0x02},
    {0x4004, 0x02},
    {0x4005, 0x1a},//BLC always-update (OV5640-1B blue channel fix)

    //AWB — OV5640-1B manufacturer calibration
    {0x5180, 0xff},
    {0x5181, 0xf2},
    {0x5182, 0x00},
    {0x5183, 0x14},
    {0x5184, 0x25},
    {0x5185, 0x24},
    {0x5186, 0x10},
    {0x5187, 0x10},
    {0x5188, 0x10},
    {0x5189, 0x6d},
    {0x518a, 0x53},
    {0x518b, 0x90},
    {0x518c, 0x8c},
    {0x518d, 0x3b},
    {0x518e, 0x2c},
    {0x518f, 0x59},
    {0x5190, 0x42},
    {0x5191, 0xf8},
    {0x5192, 0x04},
    {0x5193, 0x70},
    {0x5194, 0xf0},
    {0x5195, 0xf0},
    {0x5196, 0x03},
    {0x5197, 0x01},
    {0x5198, 0x04},
    {0x5199, 0x00},
    {0x519a, 0x04},
    {0x519b, 0x13},
    {0x519c, 0x06},
    {0x519d, 0x9e},
    {0x519e, 0x38},

    //LENC — OV5640-1B manufacturer calibration (Register Setting Update)
    {0x5800, 0x2e},
    {0x5801, 0x1d},
    {0x5802, 0x15},
    {0x5803, 0x15},
    {0x5804, 0x1c},
    {0x5805, 0x32},
    {0x5806, 0x14},
    {0x5807, 0x0b},
    {0x5808, 0x07},
    {0x5809, 0x07},
    {0x580a, 0x0a},
    {0x580b, 0x12},
    {0x580c, 0x0c},
    {0x580d, 0x04},
    {0x580e, 0x00},
    {0x580f, 0x00},
    {0x5810, 0x03},
    {0x5811, 0x0c},
    {0x5812, 0x0c},
    {0x5813, 0x05},
    {0x5814, 0x00},
    {0x5815, 0x00},
    {0x5816, 0x04},
    {0x5817, 0x0c},
    {0x5818, 0x14},
    {0x5819, 0x0b},
    {0x581a, 0x07},
    {0x581b, 0x07},
    {0x581c, 0x0b},
    {0x581d, 0x14},
    {0x581e, 0x33},
    {0x581f, 0x21},
    {0x5820, 0x17},
    {0x5821, 0x17},
    {0x5822, 0x1e},
    {0x5823, 0x33},
    {0x5824, 0x22},
    {0x5825, 0x24},
    {0x5826, 0x06},
    {0x5827, 0x26},
    {0x5828, 0x22},
    {0x5829, 0x62},
    {0x582a, 0x24},
    {0x582b, 0x24},
    {0x582c, 0x24},
    {0x582d, 0x42},
    {0x582e, 0x60},
    {0x582f, 0x22},
    {0x5830, 0x20},
    {0x5831, 0x22},
    {0x5832, 0x62},
    {0x5833, 0x42},
    {0x5834, 0x24},
    {0x5835, 0x24},
    {0x5836, 0x24},
    {0x5837, 0x42},
    {0x5838, 0x02},
    {0x5839, 0x24},
    {0x583a, 0x06},
    {0x583b, 0x06},
    {0x583c, 0x24},
    {0x583d, 0xee},

    //color matrix (Saturation)
    {0x5381, 0x1e},
    {0x5382, 0x5b},
    {0x5383, 0x08},
    {0x5384, 0x0a},
    {0x5385, 0x7e},
    {0x5386, 0x88},
    {0x5387, 0x7c},
    {0x5388, 0x6c},
    {0x5389, 0x10},
    {0x538a, 0x01},
    {0x538b, 0x98},

    //CIP control (Sharpness)
    {0x5300, 0x20},//sharpness MT thresh1 (increased for detail)
    {0x5301, 0x20},//sharpness MT thresh2
    {0x5302, 0x10},//sharpness MT offset1 (lower=stronger)
    {0x5303, 0x10},//sharpness MT offset2
    {0x5304, 0x10},
    {0x5305, 0x10},
    {0x5306, 0x04},//denoise thresh (lower=more detail preserved)
    {0x5307, 0x16},
    {0x5308, 0x40},
    {0x5309, 0x10},//sharpness
    {0x530a, 0x10},//sharpness
    {0x530b, 0x04},//sharpness
    {0x530c, 0x06},//sharpness

    //GAMMA
    {0x5480, 0x01},
    {0x5481, 0x00},
    {0x5482, 0x1e},
    {0x5483, 0x3b},
    {0x5484, 0x58},
    {0x5485, 0x66},
    {0x5486, 0x71},
    {0x5487, 0x7d},
    {0x5488, 0x83},
    {0x5489, 0x8f},
    {0x548a, 0x98},
    {0x548b, 0xa6},
    {0x548c, 0xb8},
    {0x548d, 0xca},
    {0x548e, 0xd7},
    {0x548f, 0xe3},
    {0x5490, 0x1d},

    //Special Digital Effects (SDE) (UV adjust)
    {0x5580, 0x06},//enable brightness, contrast and saturation
    {0x5583, 0x40},//saturation Cb
    {0x5584, 0x40},//saturation Cr (must match Cb for neutral color)
    {0x5586, 0x20},//contrast
    {0x5587, 0x00},//brightness
    {0x5588, 0x01},//brightness
    {0x5589, 0x10},
    {0x558a, 0x00},
    {0x558b, 0xf8},
    {0x501d, 0x40},// enable manual offset of contrast

    //power on
    {0x3008, 0x02},

    //50Hz
    {0x3c00, 0x04},
    
    {0XFFFF, 300},
    {0x0000, 0x00}, // tail
};

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
    // sccb_init: no-custom-regs path
    
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
    mp_printf(MP_PYTHON_PRINTER, "OV5640 ID=0x%04x\n", reg);

    for(uint16_t i=0; i<sizeof(sensor_default_regs)/4; i++)
	{
		OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,sensor_default_regs[i][0],sensor_default_regs[i][1]);
	}
    sleep_ms(50);

    // === JPEG VGA 640x480 mode configuration ===

    // VGA 640x480 window/crop/output (resolution registers, PLL-independent)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,TIMING_TC_REG20, 0x41);  // Timing TC (subsample)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,TIMING_TC_REG21, 0x27);  // Timing TC (mirror + JPEG enable bit5)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_INCREMENT, 0x31);  // H subsample
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_INCREMENT, 0x31);  // V subsample

    // Input window
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3800, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3801, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3802, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3803, 0x04);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3804, 0x0a);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3805, 0x3f);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3806, 0x07);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3807, 0x9b);

    // Output size: 640x480
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OUTPUT_SIZE_H, 0x02);  // Width H (640)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OUTPUT_SIZE_L, 0x80);  // Width L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OUTPUT_SIZE_H, 0x01);  // Height H (480)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OUTPUT_SIZE_L, 0xe0);  // Height L

    // Timing
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_TOTAL_SIZE_H, 0x07);  // HTS H
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_TOTAL_SIZE_L, 0x68);  // HTS L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_TOTAL_SIZE_H, 0x04);  // VTS H
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_TOTAL_SIZE_L, 0x38);  // VTS L

    // ISP offsets
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OFFSET_H, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OFFSET_L, 0x10);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OFFSET_H, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OFFSET_L, 0x06);
    sleep_ms(50);

    // JPEG format configuration
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3002, 0x00);  // Release format/JPEG block resets
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3006, 0xff);  // Enable ALL clocks including JPEG
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,FORMAT_CTRL, 0x00);   // ISP format: YUV422
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,FORMAT_CTRL00, 0x30); // YUV422 YUYV byte order
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x4407, 0x20);  // JPEG quality scale (moderate)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x460b, 0x35);  // JPEG stream marker enable
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x471c, 0x50);  // DVP/JPEG path control (esp32-camera JPEG fmt)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x4713, 0x03);  // JPEG mode 3 (override default 0x02)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,ISP_CONTROL_01, 0xa3);  // ISP control with scaling
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,AEC_PK_MANUAL, 0x00);  // AEC/AGC auto

    // VFIFO size (match VGA output)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_X_SIZE_H, 0x02);  // HSIZE H (640)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_X_SIZE_L, 0x80);  // HSIZE L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_Y_SIZE_H, 0x01);  // VSIZE H (480)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_Y_SIZE_L, 0xe0);  // VSIZE L
    sleep_ms(50);

    // PLL configuration for 37MHz XCLK (scaled from manufacturer's 24MHz values)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_5, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_0, 0x1A);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_1, 0x11);  // sys_div=1 (was 2)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_2, 0x2D);  // mult=45 (VCO=555MHz, mult=70 too fast for PIO)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_3, 0x13);  // prediv (manufacturer value)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SYSTEM_ROOT_DIVIDER, 0x16);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,PCLK_RATIO, 0x02);       // ratio=2 (ratio=1 too fast for PIO)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_CTRL0C, 0x22);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SCCB_SYSTEM_CTRL1, 0x13);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x300e, 0x58);           // DVP control (was missing)

    // JPEG compression enable + PCLK polarity
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3821, 0x26);           // compression enable, bit0=0
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,CLOCK_POL_CONTROL, 0x21); // active-high PCLK, VSYNC inverted

    // Release all blocks from reset AFTER PLL config (PLL change can re-assert resets).
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3000, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3002, 0x00);

    sleep_ms(50);
    mp_printf(MP_PYTHON_PRINTER, "Camera OK\n");
}

/********************************************************************************
function:   Camera register initialization with custom register array
parameter:  sda_pin, scl_pin, custom_regs array, num_regs
********************************************************************************/
void sccb_init_with_registers(const uint32_t sda_pin, const uint32_t scl_pin, const uint16_t custom_regs[][2], uint16_t num_regs)
{
    ov5640_cam_addr = 0x3c;
    ov5640_i2c = i2c1;

    i2c_init(ov5640_i2c, 100 * 1000);

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    uint16_t reg;
    reg=OV5640_RD_Reg(ov5640_i2c,ov5640_cam_addr,0X300A);
    reg<<=8;
    reg|=OV5640_RD_Reg(ov5640_i2c,ov5640_cam_addr,0X300B);
    mp_printf(MP_PYTHON_PRINTER, "OV5640 ID=0x%04x\n", reg);

    for(uint16_t i=0; i<sizeof(sensor_default_regs)/4; i++)
    {
        OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,sensor_default_regs[i][0],sensor_default_regs[i][1]);
    }
    sleep_ms(50);

    // === JPEG VGA 640x480 mode configuration ===

    // VGA 640x480 window/crop/output (resolution registers, PLL-independent)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,TIMING_TC_REG20, 0x41);  // Timing TC (subsample)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,TIMING_TC_REG21, 0x27);  // Timing TC (mirror + JPEG enable bit5)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_INCREMENT, 0x31);  // H subsample
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_INCREMENT, 0x31);  // V subsample

    // Input window
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3800, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3801, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3802, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3803, 0x04);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3804, 0x0a);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3805, 0x3f);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3806, 0x07);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3807, 0x9b);

    // Output size: 640x480
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OUTPUT_SIZE_H, 0x02);  // Width H (640)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OUTPUT_SIZE_L, 0x80);  // Width L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OUTPUT_SIZE_H, 0x01);  // Height H (480)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OUTPUT_SIZE_L, 0xe0);  // Height L

    // Timing
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_TOTAL_SIZE_H, 0x07);  // HTS H
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_TOTAL_SIZE_L, 0x68);  // HTS L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_TOTAL_SIZE_H, 0x04);  // VTS H
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_TOTAL_SIZE_L, 0x38);  // VTS L

    // ISP offsets
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OFFSET_H, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,X_OFFSET_L, 0x10);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OFFSET_H, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,Y_OFFSET_L, 0x06);
    sleep_ms(50);

    // JPEG format configuration
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3002, 0x00);  // Release format/JPEG block resets
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3006, 0xff);  // Enable ALL clocks including JPEG
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,FORMAT_CTRL, 0x00);   // ISP format: YUV422
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,FORMAT_CTRL00, 0x30); // YUV422 YUYV byte order
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x4407, 0x20);  // JPEG quality scale (moderate)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x460b, 0x35);  // JPEG stream marker enable
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x471c, 0x50);  // DVP/JPEG path control (esp32-camera JPEG fmt)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x4713, 0x03);  // JPEG mode 3 (override default 0x02)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,ISP_CONTROL_01, 0xa3);  // ISP control with scaling
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,AEC_PK_MANUAL, 0x00);  // AEC/AGC auto

    // VFIFO size (match VGA output)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_X_SIZE_H, 0x02);  // HSIZE H (640)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_X_SIZE_L, 0x80);  // HSIZE L
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_Y_SIZE_H, 0x01);  // VSIZE H (480)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_Y_SIZE_L, 0xe0);  // VSIZE L
    sleep_ms(50);

    // PLL configuration for 37MHz XCLK (scaled from manufacturer's 24MHz values)
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_5, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_0, 0x1A);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_1, 0x21);  // sys_div=2
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_2, 0x2D);  // mult=45
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SC_PLL_CONTRL_3, 0x13);  // prediv
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SYSTEM_ROOT_DIVIDER, 0x16);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,PCLK_RATIO, 0x10);       // ratio=16
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,VFIFO_CTRL0C, 0x22);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,SCCB_SYSTEM_CTRL1, 0x13);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x300e, 0x58);           // DVP control

    // JPEG compression enable + PCLK polarity
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3821, 0x26);           // compression enable, bit0=0
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,CLOCK_POL_CONTROL, 0x21); // active-high PCLK, VSYNC inverted

    // Release all blocks from reset AFTER PLL config (PLL change can re-assert resets).
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3000, 0x00);
    OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,0x3002, 0x00);

    sleep_ms(50);

    // Apply custom register overrides from Python
    for(uint16_t i=0; i<num_regs; i++)
    {
        OV5640_WR_Reg(ov5640_i2c,ov5640_cam_addr,custom_regs[i][0],(uint8_t)custom_regs[i][1]);
    }
    sleep_ms(50);
    mp_printf(MP_PYTHON_PRINTER, "Camera OK\n");
}

/**
 * @brief Set the OV5640 DATA ORDER register (0x4745)
 *        Must be called after sccb_init().
 * @param reverse: true for reverse output data bit order, false for normal
 */
void ov5640_set_data_order(bool reverse) {
    if (!ov5640_i2c) return;
    uint8_t val = OV5640_RD_Reg(ov5640_i2c, ov5640_cam_addr, OV5640_REG_DATA_ORDER);
    if (reverse) {
        val |= 0x01; // Set bit 0 for reverse
    } else {
        val &= ~0x01; // Clear bit 0 for normal
    }
    OV5640_WR_Reg(ov5640_i2c, ov5640_cam_addr, OV5640_REG_DATA_ORDER, val);
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
        mp_printf(MP_PYTHON_PRINTER, "ov5640_read_register: error - not initialized\n");
        return -1;
    }
    uint8_t val = OV5640_RD_Reg(ov5640_i2c, ov5640_cam_addr, reg);
    return val;
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
