

#include "EOtheLEDpulser.h"

#include "embot_app_theCANboardInfo.h"

#include "embot.h"

#include "stm32hal.h" // to see bsp_led_init etc

#include "embot_i2h.h"
#include "embot_hw.h"

#include "embot_hw_FlashStorage.h"
#include "embot_sys_theStorage.h"

#include "embot_app_theBootloader.h"

#include "embot_hw_FlashBurner.h"

struct ActivityParam
{
    uint32_t blinkingperiod;
};

static void bl_activity(void* param);

void periodic_activity(embot::sys::Task *tsk, void *param);

static ActivityParam activity_param = {0};

//static embot::hw::FlashBurner *fb = nullptr;


int main(void)
{ 
    embot::app::theBootloader & bootloader = embot::app::theBootloader::getInstance();

    std::uint32_t param = 0;    
    embot::sys::theJumper::Command cmd = bootloader.getcommand(param);
           
    embot::app::theBootloader::Config config;
    config.userdeflauncher.callback = bl_activity;
    config.userdeflauncher.arg = &activity_param;
    activity_param.blinkingperiod = 1*EOK_reltime100ms;
    config.countdown = 5*embot::common::time1second;
    
    switch(cmd)
    {
        case embot::sys::theJumper::Command::none:
        {   // we dont change what we want to do: we exec for at 5 seconds
        } break;
        
        case embot::sys::theJumper::Command::stay:
        {   // we have received a command from the previous running process to stay in here forever
            config.countdown = 0;
            activity_param.blinkingperiod = 5*EOK_reltime100ms;
        } break; 
        
        case embot::sys::theJumper::Command::jump:
        {   // we have received command to jump to an address
            bootloader.jump(param); 
            // if we cannot jump to the specified address ... we stay in here forever
            config.countdown = 0;
            activity_param.blinkingperiod = 5*EOK_reltime10ms;
        } break;          
    }

    
    bootloader.execute(config);
    
    for(;;);
    
}



static void bl_activity(void* param)
{
    
    ActivityParam* pp = (ActivityParam*) param;
    
    uint32_t period = 0;
    
    if(nullptr != pp)
    {
        period = pp->blinkingperiod;
    }
    
    eOledpulser_cfg_t ledconfig = {0};
    
    ledconfig.led_enable_mask   = (1 << eo_ledpulser_led_zero);
    ledconfig.led_init          = reinterpret_cast<eOint8_fp_uint8_cvoidp_t>(embot::hw::led::init_legacy);
    ledconfig.led_on            = reinterpret_cast<eOint8_fp_uint8_t>(embot::hw::led::on); 
    ledconfig.led_off           = reinterpret_cast<eOint8_fp_uint8_t>(embot::hw::led::off);
    ledconfig.led_toggle        = reinterpret_cast<eOint8_fp_uint8_t>(embot::hw::led::toggle);
    
    eo_ledpulser_Initialise(&ledconfig);    

    eo_ledpulser_Start(eo_ledpulser_GetHandle(), eo_ledpulser_led_zero, period, 0);  

    
    embot::hw::button::init(embot::hw::button::BTN::zero);

    // also start a periodic task which checks ....
    embot::sys::PeriodicTask *taskper = new embot::sys::PeriodicTask;        
    taskper->init(nullptr, periodic_activity, 1024, 30, 100*1000, nullptr);
    
    // init the storage object ....
    
    embot::app::theCANboardInfo &canbrdinfo = embot::app::theCANboardInfo::getInstance();
    
    canbrdinfo.synch(embot::app::theCANboardInfo::Type::mtb3, 1);
    
    uint8_t adr = canbrdinfo.getCANaddress();
    adr = adr;
    if(1 == adr)
    {
        canbrdinfo.setCANaddress(2);
    }
    
    //fb = new embot::hw::FlashBurner;
        
}



void periodic_activity(embot::sys::Task *tsk, void *param)
{
    static uint32_t x = 0;
    if(true == embot::hw::button::pressed(embot::hw::button::BTN::zero))
    {
        embot::app::theBootloader & bootloader = embot::app::theBootloader::getInstance();
        bootloader.stopcountdown();
        
        eo_ledpulser_Start(eo_ledpulser_GetHandle(), eo_ledpulser_led_zero, 10*EOK_reltime100ms, 0); 
        
        x++;
        // we stop countdown and we stay in bootloader w/ reduced frequency of ....
        x = x; 
#if 0        
        static std::uint8_t data[40] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
        static std::uint32_t size = 6;
        static std::uint32_t adr = embot::hw::sys::addressOfApplication;
        static std::uint8_t *dd = data;
        
        if(1 == x)
        {
            size = 6;
            fb->add(adr, size, dd);    
            adr += size;  
            dd += size;
        }
        else if(2 == x)
        {
            size = 6;  
            fb->add(adr, size, dd);    
            adr += size;  
            dd += size;            
        }
        else if(3 == x)
        {
            size = 4; 
            fb->add(adr, size, dd);    
            adr = embot::hw::sys::addressOfApplication+2048;  
            dd += size;                        
        }
        else if(4 == x)
        {
            size = 6;
            fb->add(adr, size, dd);    
            adr += size;  
            dd += size;
            size = 6;  
            fb->add(adr, size, dd);    
            adr += size;  
            dd += size;    
            size = 4; 
            fb->add(adr, size, dd); 
            adr = embot::hw::sys::addressOfApplication+2048;  
            dd += size;             
        }
        
#endif        
    }    
}


///


