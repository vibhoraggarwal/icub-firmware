/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

/* @file       hal_chip_st_l3g4200d.c
	@brief      This file implements internals of the temperature sensor module.
	@author     marco.accame@iit.it
    @date       10/25/2012
**/

// - modules to be built: contains the HAL_USE_* macros ---------------------------------------------------------------
#include "hal_brdcfg_modules.h"

#ifdef  HAL_USE_CHIP_ST_L3G4200D

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"
#include "hal_base_hid.h" 
#include "hal_brdcfg.h"


#include "stdio.h"




 
// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "hal_chip_st_l3g4200d.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "hal_chip_st_l3g4200d_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------

#define I2CADDRESS          0xD0

#define WHOAMI_ADR          0x0F
#define WHOAMI_VAL          0xD3

#define TEMP_ADR            0x26

#define ARXL_ADR            0x28
#define ARXH_ADR            0x29
#define ARYL_ADR            0x2a
#define ARYH_ADR            0x2b
#define ARZL_ADR            0x2c
#define ARZH_ADR            0x2d

#define CTR1_ADR            0x20
#define CTR2_ADR            0x21
#define CTR3_ADR            0x22
#define CTR4_ADR            0x23
#define CTR5_ADR            0x24


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------

extern const hal_chip_st_l3g4200d_cfg_t hal_chip_st_l3g4200d_cfg_default  = 
{ 
    .i2cid      = hal_i2c1,
    .range      = hal_chip_st_l3g4200d_range_250dps    
};

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------


typedef struct
{
    hal_chip_st_l3g4200d_cfg_t      config;
    uint32_t                        factor;
    uint8_t                         shift;
} hal_chip_st_l3g4200d_internal_item_t;

typedef struct
{
    hal_bool_t                                  initted;
    hal_chip_st_l3g4200d_internal_item_t*       items[1];   
} hal_chip_st_l3g4200d_theinternals_t;

// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static void s_hal_chip_st_l3g4200d_initted_set(void);
static hal_boolval_t s_hal_chip_st_l3g4200d_initted_is(void);

static hal_result_t s_hal_chip_st_l3g4200d_hw_init(const hal_chip_st_l3g4200d_cfg_t *cfg, hal_chip_st_l3g4200d_internal_item_t* intitem);

static int32_t s_hal_chip_st_l3g4200d_convert(int32_t v);


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static const variables
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static hal_chip_st_l3g4200d_theinternals_t s_hal_chip_st_l3g4200d_theinternals =
{
    .initted            = hal_false,
    .items              = { NULL }   
};





// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern hal_result_t hal_chip_st_l3g4200d_init(const hal_chip_st_l3g4200d_cfg_t *cfg)
{
    hal_chip_st_l3g4200d_internal_item_t *intitem = s_hal_chip_st_l3g4200d_theinternals.items[0];
     
    if(NULL == cfg)
    {
        cfg  = &hal_chip_st_l3g4200d_cfg_default;
    }
    
    if(hal_true == s_hal_chip_st_l3g4200d_initted_is())
    {
        return(hal_res_OK);
    }  

    // if it does not have ram yet, then attempt to allocate it.
    if(NULL == intitem)
    {
        intitem = s_hal_chip_st_l3g4200d_theinternals.items[0] = hal_heap_new(sizeof(hal_chip_st_l3g4200d_internal_item_t));
        // minimal initialisation of the internal item
        // nothing to init.      
    }  
    

    if(hal_res_OK != s_hal_chip_st_l3g4200d_hw_init(cfg, intitem))
    {
        return(hal_res_NOK_generic);
    }
    
    
    s_hal_chip_st_l3g4200d_initted_set();

    return(hal_res_OK);
}


extern hal_result_t hal_chip_st_l3g4200d_temp_get(int8_t* temp)
{
    hal_result_t res = hal_res_NOK_generic; 
    hal_chip_st_l3g4200d_internal_item_t *intitem = s_hal_chip_st_l3g4200d_theinternals.items[0];
    hal_i2c_t i2cid = intitem->config.i2cid;

    uint8_t data = 0;

    *temp = 0;
    
    if(hal_false == s_hal_chip_st_l3g4200d_initted_is())
    {
        return(hal_res_NOK_generic);
    }
    


    hal_i2c_regaddr_t regaddr = {.numofbytes = 1, .bytes.one = 0x00 }; 
    
    regaddr.bytes.one = TEMP_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &data, 1);
    *temp = (int8_t)data;   

    res = res;    
   
    return(hal_res_OK);
}

extern hal_result_t hal_chip_st_l3g4200d_angrate_get(int32_t* xar, int32_t* yar, int32_t* zar)
{
    hal_result_t res = hal_res_NOK_generic; 
    hal_chip_st_l3g4200d_internal_item_t *intitem = s_hal_chip_st_l3g4200d_theinternals.items[0];
    hal_i2c_t i2cid = intitem->config.i2cid;

    uint8_t datal = 0;
    uint8_t datah = 0;
    int32_t tmp;

    
    *xar = 0;
    *yar = 0;
    *zar = 0;
    
    if(hal_false == s_hal_chip_st_l3g4200d_initted_is())
    {
        return(hal_res_NOK_generic);
    }


    hal_i2c_regaddr_t regaddr = {.numofbytes = 1, .bytes.one = 0x00 }; 
    
    regaddr.bytes.one = ARXL_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datal, 1);
    regaddr.bytes.one = ARXH_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datah, 1);
    tmp = (int16_t)((datah << 8) | datal);    
    *xar = s_hal_chip_st_l3g4200d_convert(tmp);
    
    regaddr.bytes.one = ARYL_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datal, 1);
    regaddr.bytes.one = ARYH_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datah, 1);
    tmp = (int16_t)((datah << 8) | datal);    
    *yar = s_hal_chip_st_l3g4200d_convert(tmp);    

    regaddr.bytes.one = ARZL_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datal, 1);
    regaddr.bytes.one = ARZH_ADR;
    res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &datah, 1);
    tmp = (int16_t)((datah << 8) | datal);    
    *zar = s_hal_chip_st_l3g4200d_convert(tmp);
    
    res = res;
   
    return(hal_res_OK);
}



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// ---- isr of the module: begin ----
// empty-section
// ---- isr of the module: end ------



extern hal_result_t hal_chip_st_l3g4200d_hid_static_memory_init(void)
{
    memset(&s_hal_chip_st_l3g4200d_theinternals, 0, sizeof(s_hal_chip_st_l3g4200d_theinternals));
    return(hal_res_OK);  
}

// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


static void s_hal_chip_st_l3g4200d_initted_set(void)
{
    s_hal_chip_st_l3g4200d_theinternals.initted = hal_true;
}

static hal_bool_t s_hal_chip_st_l3g4200d_initted_is(void)
{
    return(s_hal_chip_st_l3g4200d_theinternals.initted);
}


// caso 2000 dps mappato su 32k -> 32*1024/2000 = 16.384. quindi ... ogni valore letto uguale a 16 io ho 1 dps.
// se moltiplico 16.384 per 2000 ottengo 32k. 
// 1 bit vale .... 1dps/16.384 = 0.06103515625 
// infatti ... se moltiplico per 32k ottengo 2000.
// se voglio milli-dps ... 1 bit vale 1000*0.06103515625 = 61.03515625
// la prova e' che 32*1024*61.03515625 = 200mila.
// scopongo 61.03515625 come 62500 / 1024 -> factor 62500 shift 10.

// caso 500.
// il fattore e' 61.03515625 / 4 = 15.2587890625
// scompongo come 15625 / 1024 

// caso 250 
// il fattore e' 15.2587890625 /2 = 7.62939453125
// scompongo come 15625 / 2048



static hal_result_t s_hal_chip_st_l3g4200d_hw_init(const hal_chip_st_l3g4200d_cfg_t *cfg, hal_chip_st_l3g4200d_internal_item_t *intitem)
{
    hal_result_t res = hal_res_NOK_generic;   
    uint8_t data;
    hal_i2c_t i2cid = cfg->i2cid;
    
 
    // init i2c    
    if(hal_res_OK != (res = hal_i2c_init(i2cid, NULL)))
    {
        return(res);
    }
    
    if(hal_res_OK != hal_i2c_ping(i2cid, I2CADDRESS))
    {
        return(hal_res_NOK_generic);
    }
    
   
    hal_i2c_regaddr_t regaddr = {.numofbytes = 1, .bytes.one = 0x00 }; 
    
    // whoami: value must be WHOAMI_VAL
    regaddr.bytes.one = WHOAMI_ADR;
    if(hal_res_OK != (res = hal_i2c_read(i2cid, I2CADDRESS, regaddr, &data, 1)))
    {
        return(res);
    }
    if((hal_res_OK != res) || (WHOAMI_VAL != data))
    {
        return(hal_res_NOK_generic);
    }
    
    // config now ...
    
    regaddr.bytes.one = CTR1_ADR;       
    data = 0x0F; // enable x, y, z and power down normal mode (so that i have temperature readings). bandwidth is 100 hx w/ 12.5 cutoff
    if(hal_res_OK != (res = hal_i2c_write(i2cid, I2CADDRESS, regaddr, &data, 1)))
    {
        return(res);
    }
    hal_i2c_standby(i2cid, I2CADDRESS);

    regaddr.bytes.one = CTR4_ADR;       
    data = 0x00;        // continuos update of data + lbs @lower address + full scalse is 250 dps + self test disable + spi disabled
    if(hal_chip_st_l3g4200d_range_250dps == cfg->range)
    {
        data |= 0x00;
        intitem->factor = 15625;
        intitem->shift  = 11; 
    }
    else if(hal_chip_st_l3g4200d_range_500dps == cfg->range)
    {
        data |= 0x10;
        intitem->factor = 15625;
        intitem->shift  = 10;         
    }
    else if(hal_chip_st_l3g4200d_range_2000dps == cfg->range)
    {
        data |= 0x20;
        intitem->factor = 62500;
        intitem->shift  = 10;         
    }
    if(hal_res_OK != (res = hal_i2c_write(i2cid, I2CADDRESS, regaddr, &data, 1)))
    {
        return(res);
    }  
    hal_i2c_standby(i2cid, I2CADDRESS);  
 
    regaddr.bytes.one = CTR5_ADR;       
    data = 0x40;        // enable fifo
    if(hal_res_OK != (res = hal_i2c_write(i2cid, I2CADDRESS, regaddr, &data, 1)))
    {
        return(res);
    }
    hal_i2c_standby(i2cid, I2CADDRESS);    
    
    
    memcpy(&intitem->config, cfg, sizeof(hal_chip_st_l3g4200d_cfg_t));
        
    
    // store the i2caddress and the register address.
    return(hal_res_OK);
}


static int32_t s_hal_chip_st_l3g4200d_convert(int32_t v)
{
    hal_chip_st_l3g4200d_internal_item_t *intitem = s_hal_chip_st_l3g4200d_theinternals.items[0];

    int32_t factor =   (int32_t)intitem->factor; // 
    uint8_t  shift =            intitem->shift; //  

    uint8_t neg = (v < 0) ? (1) : (0);
    int32_t r = (0 == neg) ? (factor*v) : (factor*(-v));
    // now r is positive
    r >>= shift;
    r = (0 == neg) ? (r) : (-r);
    
    return(r);  
}


#endif//HAL_USE_CHIP_ST_L3G4200D


// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



