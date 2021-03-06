
/*
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
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

// - include guard ----------------------------------------------------------------------------------------------------

#ifndef _EMBOT_HW_I2C_H_
#define _EMBOT_HW_I2C_H_

#include "embot_common.h"
#include "embot_hw.h"



// see also https://www.i2c-bus.org/

namespace embot { namespace hw { namespace i2c {
    
    enum class Bus { one = 0, two = 1, none = 32, maxnumberof = 2 };
    
    struct Config
    {   
        std::uint32_t   speed; 
        Config(std::uint32_t s) : speed(s) {}        
        Config() : speed(400000) {}
    };
    
    struct Descriptor
    {
        Bus     bus;
        Config  config;
        Descriptor() : bus(Bus::none), config(400000) {}
        Descriptor(Bus b, std::uint32_t s) : bus(b), config(s) {}
    };
    
    
    bool supported(Bus b);
    
    bool initialised(Bus b);
    
    result_t init(Bus b, const Config &config);
    
    // if a transaction is ongoing the bus cannot be used.
    bool isbusy(Bus b);
    // we also have a version w/ timeout. use it carefully!
    bool isbusy(Bus b, embot::common::relTime timeout, embot::common::relTime &remaining);
    
    // check is the device is present.
    // it internally calls isbusy(timeout, remaining)
    bool ping(Bus b, std::uint8_t adr, embot::common::relTime timeout = 3*embot::common::time1millisec);
        
    // not blocking read. we read from register reg a total of destination.size bytes
    // at the end of transaction, data is copied into destination.pointer and oncompletion.callback() is called (if non nullptr). 
    result_t read(Bus b, std::uint8_t adr, std::uint8_t reg, embot::common::Data &destination, const embot::common::Callback &oncompletion);
    
    // blocking read. we read from register reg a total of destination.size bytes and we wait until a timeout. 
    // if result is resOK, destination.pointer contains the data; if resNOKtimeout, the timeout expired. if resNOK the operation was not even started
    // the functions internally waits until not busy for the timeout ... however, please check isbusy() outside. 
    result_t read(Bus b, std::uint8_t adr, std::uint8_t reg, embot::common::Data &destination, embot::common::relTime timeout);
        
    // not blocking write. we write in register reg the content.size byte pointed by content.pointer.
    // when the write is done, the function oncompletion.callback() is called to alert the user.
    result_t write(Bus b, std::uint8_t adr, std::uint8_t reg, const embot::common::Data &content, const embot::common::Callback &oncompletion = embot::common::Callback(nullptr, nullptr));
    
    // blocking write. we write in register reg thethe content.size byte pointed by content.pointer and we wait until a timeout.
    // if result is resOK, the operation is successful. if resNOKtimeout, the timeout expired. if resNOK the operation was not even started
    // the functions internally waits until not busy for the timeout ... however, please check isbusy() outside.
    result_t write(Bus b, std::uint8_t adr, std::uint8_t reg, const embot::common::Data &content, embot::common::relTime timeout);    

}}} // namespace embot { namespace hw { namespace i2c {
    
    

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------


