/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
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


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"

#include "EoCommon.h"
#include "EoError.h"
#include "EOtheErrorManager.h"


#include "EOtheCANservice.h"
#include "EOtheCANprotocol.h"

#include "EOaction.h"

#include "EOMtheEMSconfigurator.h"

#include "EOtheEntities.h"

#include "EOMtheEMSappl.h"

#include "EOVtheCallbackManager.h"

// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheCANdiscovery2.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheCANdiscovery2_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables. deprecated: better using _get(), _set() on static variables 
// --------------------------------------------------------------------------------------------------------------------

const eOcandiscovery_cfg_t eo_candiscovery_default_cfg = 
{ 
    .period     = 100*1000, 
    .timeout    = 3*1000*1000 
};


// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static void s_eo_candiscovery2_resetTarget(void);

static void s_eo_candiscovery2_resetDetection(void);

static void s_eo_candiscovery2_sendResultToHost(eObool_t allboardsfound, eObool_t allboardsareok); 

static void s_eo_candiscovery2_resetSearchStatus(void);

static eObool_t s_eo_candiscovery2_AllBoardsAreFound(EOtheCANdiscovery2 *p);

static eObool_t s_eo_candiscovery2_IsDetectionOK(EOtheCANdiscovery2 *p, eOcanmap_location_t loc, eObool_t match, eObrd_typeandversions_t *detected);


static void s_eo_candiscovery2_on_timer_expiry(void *arg);

static eObool_t s_eo_candiscovery2_search(void);

static eOresult_t s_eo_candiscovery2_getFWversion(eObrd_cantype_t board, eOcanmap_location_t location, eObrd_version_t requiredprotocolversion);



// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static EOtheCANdiscovery2 s_eo_thecandiscovery2 = 
{
    .initted                            = eobool_false,
    .config                             = {0},
    .discoverytimer                     = NULL,

    .discoverymaxretries                = 10,
    
    .searchstatus                       = {0},
    .target                             = {0},
    .detection                          =
    {
        .allhavereplied                 = eobool_false,        
        .atleastonereplyisincompatible  = eobool_false,
        .replies                        = {0},
        .incompatibilities              = {0},
        .boards                         = {0}
    }
};

static const char s_eobj_ownname[] = "EOtheCANdiscovery2";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheCANdiscovery2* eo_candiscovery2_Initialise(const eOcandiscovery_cfg_t* cfg)
{
    if(eobool_true == s_eo_thecandiscovery2.initted)
    {
        return(&s_eo_thecandiscovery2);
    }
    
    if(NULL == cfg)
    {
        cfg = &eo_candiscovery_default_cfg;
    }
    
    memcpy(&s_eo_thecandiscovery2.config, cfg, sizeof(eOcandiscovery_cfg_t));
    
    if(eok_reltimeINFINITE == s_eo_thecandiscovery2.config.timeout)
    {
        s_eo_thecandiscovery2.discoverymaxretries = eok_reltimeINFINITE;
    }
    else
    {
        s_eo_thecandiscovery2.discoverymaxretries = s_eo_thecandiscovery2.config.timeout / s_eo_thecandiscovery2.config.period;
    }

    s_eo_thecandiscovery2.discoverytimer = eo_timer_New();

 
    
    s_eo_candiscovery2_resetTarget();
    s_eo_candiscovery2_resetDetection();
    s_eo_candiscovery2_resetSearchStatus();


    s_eo_thecandiscovery2.initted = eobool_true;
    
    return(&s_eo_thecandiscovery2);   
}


extern EOtheCANdiscovery2* eo_candiscovery2_GetHandle(void)
{
    if(s_eo_thecandiscovery2.initted == eobool_false)
    {
        return NULL;
    }
    
    return(&s_eo_thecandiscovery2);
}


extern eOresult_t eo_candiscovery2_Start(EOtheCANdiscovery2 *p, const eOcandiscovery_target_t *set)
{
    if((NULL == p) || (NULL == set))
    {
        return(eores_NOK_nullpointer);
    }  
    
    
    // if a search is active, i must call the stop
    if(eobool_true == s_eo_thecandiscovery2.searchstatus.searching)
    {
        eo_candiscovery2_Stop(p);
    }
    
    
    // reset target of previous search
    s_eo_candiscovery2_resetTarget();
    s_eo_candiscovery2_resetDetection();
    
    // reset search status
    s_eo_candiscovery2_resetSearchStatus();
    
    
    
    
    // copy the new set of boards
    memcpy(&s_eo_thecandiscovery2.target, set, sizeof(eOcandiscovery_target_t));    
    
    // now start the procedure
    s_eo_thecandiscovery2.searchstatus.searching = eobool_true;
    s_eo_thecandiscovery2.searchstatus.tickingenabled = eobool_false;
    s_eo_thecandiscovery2.searchstatus.forcestop = eobool_false;
    s_eo_thecandiscovery2.detection.atleastonereplyisincompatible = eobool_false;
    s_eo_thecandiscovery2.detection.allhavereplied = eobool_false;
    
    // 1. i send to all the can boards the first request. all subsequent requests are managed by the _Tick() function which is triggered by the timer
    s_eo_candiscovery2_search();
    
    EOaction_strg astg = {0};
    EOaction *action = (EOaction*)&astg;
       
    // the action depends on the state of the application. 
    // if we are in config mode, then we send an event to the config task.  
    // else if we are in run mode we just set an enable flag for teh control-loop to exec _Tick().
    
    eo_action_SetCallback(action, s_eo_candiscovery2_on_timer_expiry, p, eov_callbackman_GetTask(eov_callbackman_GetHandle())); // eom_callbackman_GetTask(eom_callbackman_GetHandle()));
    eo_timer_Start(s_eo_thecandiscovery2.discoverytimer, eok_abstimeNOW, s_eo_thecandiscovery2.config.period, eo_tmrmode_FOREVER, action);         

    return(eores_OK);
}



extern eOresult_t eo_candiscovery2_Tick(EOtheCANdiscovery2 *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }  
    
    // i execute only if the tick is enabled    
    if(eobool_false == s_eo_thecandiscovery2.searchstatus.tickingenabled)
    {
        return(eores_OK);
    }

    // i execute only if the search is enabled    
    if(eobool_false == s_eo_thecandiscovery2.searchstatus.searching)
    {
        return(eores_OK);
    } 

    // i check if there is a forced stop, which means that we have reached the max number of attempts
    if(eobool_true == s_eo_thecandiscovery2.searchstatus.forcestop)
    {   //  ... stop everything. the stop operation also sends up diagnostics or ack/nak
        return(eo_candiscovery2_Stop(p));
    }
    
    // otherwise i trigger a further search    
    s_eo_candiscovery2_search();
      

    return(eores_OK);
}



extern eOresult_t eo_candiscovery2_OneBoardIsFound(EOtheCANdiscovery2 *p, eOcanmap_location_t loc, eObool_t match, eObrd_typeandversions_t *detected)
{   
    if((NULL == p) || (NULL == detected))
    {
        return(eores_NOK_nullpointer);
    }
    
    // use the information inside loc to mark that a can board has replied. 
    eo_common_hlfword_bitset(&s_eo_thecandiscovery2.detection.replies[loc.port], loc.addr);
    // put inside detected what the board has told
    memcpy(&s_eo_thecandiscovery2.detection.boards[loc.port][loc.addr], detected, sizeof(eObrd_typeandversions_t)); 
    // now mark success or failure of the query. the success depends on what we want to do with board-type, prot-version, appl-version.
    // so far we use the strict rule: 
    // - the board must be the same, 
    // - the prot-version must be the same (if the expected is not 0.0), 
    // - the appl-version must be the same (if the expected is not 0.0).
    eObool_t itisok = s_eo_candiscovery2_IsDetectionOK(p, loc, match, detected);
    // mark a failure
    if(eobool_false == itisok)
    {
        s_eo_thecandiscovery2.detection.atleastonereplyisincompatible = eobool_true;
        eo_common_hlfword_bitset(&s_eo_thecandiscovery2.detection.incompatibilities[loc.port], loc.addr);
    }
    
    // now i verify if every board i was looking for has replied. if so, i can stop procedure.    
    s_eo_thecandiscovery2.detection.allhavereplied = s_eo_candiscovery2_AllBoardsAreFound(p);
    
    if(eobool_true == s_eo_thecandiscovery2.detection.allhavereplied)
    {
        eo_candiscovery2_Stop(p);
    }

    return(eores_OK);
}


extern eObool_t eo_candiscovery2_IsSearchOK(EOtheCANdiscovery2 *p)
{
    if(NULL == p)
    {
        return(eobool_false);
    }   

    eObool_t allboardsareok = (eobool_false == s_eo_thecandiscovery2.detection.incompatibilities) ? (eobool_true) : (eobool_false);
    
    if((eobool_true == s_eo_thecandiscovery2.detection.allhavereplied) && (eobool_true == allboardsareok))
    {   
        return(eobool_true);
    }
    return(eobool_false);
}


extern const eOcandiscovery_target_t* eo_candiscovery2_GetTarget(EOtheCANdiscovery2 *p)
{
    if(NULL == p)
    {
        return(NULL);
    }   

    return(&s_eo_thecandiscovery2.target);    
}

extern const eOcandiscovery_detection_t* eo_candiscovery2_GetDetection(EOtheCANdiscovery2 *p)
{
    if(NULL == p)
    {
        return(NULL);
    }   

    return(&s_eo_thecandiscovery2.detection);    
}


extern eOresult_t eo_candiscovery2_Stop(EOtheCANdiscovery2 *p)
{   
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    // there are four possible situations:
    // 1. all boards are found and they all have correct fw version: OK
    //    if we have this situation we are not in timeout expiry. 
    //    i must: stop the timer, send up a confirmation OK, reset status. 
    // 2. all boards are found but at least one has an incorrect fw version: NOK_allfound_fwerror
    //    if we have this situation we are not in timeout expiry. 
    //    i must: stop the timer, send up a confirmation NOK_allfound_fwerror, send up diagnostics with list of error in fw version, reset status.
    // 3. at least one board is not found. the ones found have correct fw version: NOK_notfound_fwok
    //    if we have this situation we are in timeout expiry.
    //    i must: stop the timer, send up a confirmation NOK_notfound_fwok, send up diagnostics with list of missing boards, reset status.
    // 4. at least one board is not found. at least one found have wrong fw version: NOK_notfound_fwko
    //    if we have this situation we are in timeout expiry.
    //    i must: stop the timer, send up a confirmation NOK_notfound_fwko, send up diagnostics with list of missing boards, send up diagnostics with list of error in fw version, reset status.    
    
    
//    uint8_t mode = 0; // 0 is ok, 1 is NOK_allfound_fwerror, 2 is NOK_notfound_fwok, 3 is NOK_notfound_fwko
    eObool_t allboardsareok = (eobool_false == s_eo_thecandiscovery2.detection.atleastonereplyisincompatible) ? (eobool_true) : (eobool_false);
    
    if((eobool_true == s_eo_thecandiscovery2.detection.allhavereplied) && (eobool_true == allboardsareok))
    {
        eo_timer_Stop(s_eo_thecandiscovery2.discoverytimer);
        s_eo_candiscovery2_sendResultToHost(eobool_true, eobool_true); 
        s_eo_candiscovery2_resetSearchStatus();
    }
    else
    {
        eo_timer_Stop(s_eo_thecandiscovery2.discoverytimer);
        s_eo_candiscovery2_sendResultToHost(s_eo_thecandiscovery2.detection.allhavereplied, allboardsareok);
        s_eo_candiscovery2_resetSearchStatus();        
    }
        
    return(eores_OK); 
}




// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

static void s_eo_candiscovery2_sendResultToHost(eObool_t allboardsfound, eObool_t allboardsareok)
{
    if(NULL != s_eo_thecandiscovery2.target.onStop)
    {  
        eObool_t allisok = allboardsfound && allboardsareok;
        s_eo_thecandiscovery2.target.onStop(&s_eo_thecandiscovery2, allisok);
    }
    
    if((eobool_true == allboardsfound) && (eobool_true == allboardsareok))
    {

        // send reply with OK value
        #warning TODO: we must define a netvar which is signalled to pc104 with a OK result
        // values are: OK-presenceOK-compatibilityOK (00b), KO-presenceOK-compatibilityKO (01b), KO-presenceKO-compatibilityOK (10b), KO-presenceKO-compatibilityKO (11b).
        // where the problem is described by a bit mask.
    }
    else
    {
        // send reply with KO value
        #warning TODO: send up a KO value
        
        
        
        eOerrmanDescriptor_t errdes = {0};
        
        if(eobool_false == allboardsfound)
        {
            // "CFG: CANdiscovery cannot find some boards. In p16: board type in 0xff00 and number of missing in 0x00ff. In p64: mask of missing addresses in 0x000000000000ffff"
            
            uint8_t i = 0;
            for(i=eOcanport1; i<eOcanports_number; i++)
            {
                uint16_t maskofmissing = (~s_eo_thecandiscovery2.detection.replies[i]) & s_eo_thecandiscovery2.target.canmap[i];  
                
                if(0 != maskofmissing)
                { 
                    uint8_t numofmissing = eo_common_hlfword_bitsetcount(maskofmissing);      
                    
                    errdes.code             = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_candiscovery_boardsmissing);
                    errdes.sourcedevice     = (eOcanport1 == i) ? (eo_errman_sourcedevice_canbus1) : (eo_errman_sourcedevice_canbus2);
                    errdes.sourceaddress    = 0;
                    errdes.par16            = (s_eo_thecandiscovery2.target.type << 8) | (numofmissing);
                    errdes.par64            = maskofmissing;
                    eo_errman_Error(eo_errman_GetHandle(), eo_errortype_error, NULL, s_eobj_ownname, &errdes);
                }
            }
            
        }
        
        if(eobool_false == allboardsareok)
        {
            // "CFG: CANdiscovery detected invalid boards. In p16: target board type in 0xff00 and number of invalid in 0x00ff. In p64: each nibble contains 0x0 if ok, mask 0x1 if wrong type, mask 0x2 if wrong fw, mask 0x4 if wrong prot"
 
            uint8_t i = 0;
            for(i=eOcanport1; i<eOcanports_number; i++)
            {
                uint16_t maskofinvalid = (s_eo_thecandiscovery2.detection.incompatibilities[i]) & s_eo_thecandiscovery2.target.canmap[i];  
                     
                if(0 != maskofinvalid)
                { 
                    uint8_t numofinvalid = eo_common_hlfword_bitsetcount(maskofinvalid);     
                    uint64_t nibbles64 = 0;
                    uint8_t n = 0;
                    for(n=0; n<15; n++)
                    {
                        if(eobool_true == eo_common_hlfword_bitcheck(maskofinvalid, n))
                        {
                            // ok, we have an invalid detected board. let us see what nibble to put
                            uint8_t nib = 0x0;
                            if(s_eo_thecandiscovery2.target.type != s_eo_thecandiscovery2.detection.boards[i][n].boardtype)
                            {
                                nib |= 0x1;
                            }
                            if(0 != memcmp(&s_eo_thecandiscovery2.target.firmwareversion, &s_eo_thecandiscovery2.detection.boards[i][n].firmwareversion, sizeof(eObrd_version_t)))
                            {
                                nib |= 0x2;
                            }
                            if(0 != memcmp(&s_eo_thecandiscovery2.target.protocolversion, &s_eo_thecandiscovery2.detection.boards[i][n].protocolversion, sizeof(eObrd_version_t)))
                            {
                                nib |= 0x4;
                            }
                            nibbles64 |= (nib<<(4*n));
                        }
                    }
                    
                    errdes.code             = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_candiscovery_boardsinvalid);
                    errdes.sourcedevice     = (eOcanport1 == i) ? (eo_errman_sourcedevice_canbus1) : (eo_errman_sourcedevice_canbus2);
                    errdes.sourceaddress    = 0;
                    errdes.par16            = (s_eo_thecandiscovery2.target.type << 8) | (numofinvalid);
                    errdes.par64            = nibbles64;
                    eo_errman_Error(eo_errman_GetHandle(), eo_errortype_error, NULL, s_eobj_ownname, &errdes);
                }
            }
            
        }        
        
    }
   
}

static void s_eo_candiscovery2_resetTarget(void)
{
    // reset results of previous detections
    memset(&s_eo_thecandiscovery2.target, 0, sizeof(s_eo_thecandiscovery2.target));     
}

static void s_eo_candiscovery2_resetDetection(void)
{
    // reset results of previous detections
    memset(&s_eo_thecandiscovery2.detection, 0, sizeof(s_eo_thecandiscovery2.detection));     
}

static void s_eo_candiscovery2_resetSearchStatus(void)
{
    memset(&s_eo_thecandiscovery2.searchstatus, 0, sizeof(s_eo_thecandiscovery2.searchstatus));
}



static eObool_t s_eo_candiscovery2_AllBoardsAreFound(EOtheCANdiscovery2 *p)
{
    // it is enough to compare the s_eo_thecandiscovery2.target.canmap with the  s_eo_thecandiscovery2.detection.replies
    if((s_eo_thecandiscovery2.target.canmap[0] == s_eo_thecandiscovery2.detection.replies[0]) && (s_eo_thecandiscovery2.target.canmap[1] == s_eo_thecandiscovery2.detection.replies[1]))
    {
        return(eobool_true);
    }
    else
    {
        return(eobool_false);
    }      
}

static eObool_t s_eo_candiscovery2_IsDetectionOK(EOtheCANdiscovery2 *p, eOcanmap_location_t loc, eObool_t match, eObrd_typeandversions_t *detected)  
{    
    if(s_eo_thecandiscovery2.target.type != detected->boardtype)
    {   // the board must be the same type
        return(eobool_false);
    }

    // if((required->major == detected->major) && (required->minor <= detected->minor)): formerly the rule for correct version was this one.
    if ((0 == s_eo_thecandiscovery2.target.protocolversion.major == 0) && (0 == s_eo_thecandiscovery2.target.protocolversion.minor))
    {   // if the required protocol version is 0.0, i don't care about the answer
    }
    else if((detected->protocolversion.major != s_eo_thecandiscovery2.target.protocolversion.major) || (detected->protocolversion.minor != s_eo_thecandiscovery2.target.protocolversion.minor))
    {   // there must be the very same protocol version. if not i must return false
        return(eobool_false);
    }
    
    
    if ((0 == s_eo_thecandiscovery2.target.firmwareversion.major == 0) && (0 == s_eo_thecandiscovery2.target.firmwareversion.minor))
    {   // if the required firmware version is 0.0, i don't care about the answer
    }
    else if((detected->firmwareversion.major != s_eo_thecandiscovery2.target.firmwareversion.major) || (detected->firmwareversion.minor != s_eo_thecandiscovery2.target.firmwareversion.minor))
    {   // there must be the very same application firmware version. if not i must return false
        return(eobool_false);
    }
    
    // all tests are ok
    return(eobool_true);    
}




static void s_eo_candiscovery2_on_timer_expiry(void *arg)
{
    EOtheCANdiscovery2* p = (EOtheCANdiscovery2*)arg;
    
    p->searchstatus.tickingenabled = eobool_true;
    
    p->searchstatus.discoverynumretries ++;
    
    if(p->searchstatus.discoverynumretries >= p->discoverymaxretries)
    {   // make it stop ...
        p->searchstatus.forcestop = eobool_true;        
    }
        
    // ok, ... if we are in run i dont do anything else because i regularly call _Tick() every 1 ms.    
    // however, if in config mode i must alert the processing task to execute a _Tick().
    eOsmStatesEMSappl_t state = eo_sm_emsappl_STerr;
    eom_emsappl_GetCurrentState(eom_emsappl_GetHandle(), &state);
    if(eo_sm_emsappl_STcfg == state)
    {   // must send an event to the handling task
        eom_task_SetEvent(eom_emsconfigurator_GetTask(eom_emsconfigurator_GetHandle()), emsconfigurator_evt_userdef01);
    }    
}


static eObool_t s_eo_candiscovery2_search(void)
{
    eObool_t allFound = eobool_true;
    
    uint8_t i = 0;
    uint8_t j = 0;
      
    for(i=eOcanport1; i<eOcanports_number; i++)
    {
        for(j=1; j<15; j++)
        {   // valid addresses are [1, 14]
            if((eobool_true == eo_common_hlfword_bitcheck(s_eo_thecandiscovery2.target.canmap[i], j)) && (eobool_false == eo_common_hlfword_bitcheck(s_eo_thecandiscovery2.detection.replies[i], j)))
            {
                eOcanmap_location_t location = {.port = i, .addr = j, .insideindex = eocanmap_insideindex_none};
                s_eo_candiscovery2_getFWversion(s_eo_thecandiscovery2.target.type, location, s_eo_thecandiscovery2.target.protocolversion);
                allFound = eobool_false;
            }        
        }
    }
    
    s_eo_thecandiscovery2.detection.allhavereplied = allFound;

    return(s_eo_thecandiscovery2.detection.allhavereplied);
}


static eOresult_t s_eo_candiscovery2_getFWversion(eObrd_cantype_t board, eOcanmap_location_t location, eObrd_version_t requiredprotocolversion)
{  
    eOcanprot_command_t command = {0};
    command.value = (void*)&requiredprotocolversion;
    
    eObool_t found = eobool_false;
    
    
    switch(board)
    {
        case eobrd_cantype_mc4:
        case eobrd_cantype_1foc:
        {
            found = eobool_true;
            command.class = eocanprot_msgclass_pollingMotorControl;
            command.type  = ICUBCANPROTO_POL_MC_CMD__GET_FIRMWARE_VERSION;            
        } break;
        
        case eobrd_cantype_mais:
        case eobrd_cantype_strain:
        {
            found = eobool_true;
            command.class = eocanprot_msgclass_pollingAnalogSensor;
            command.type  = ICUBCANPROTO_POL_AS_CMD__GET_FW_VERSION;         
        } break;
        
        case eobrd_cantype_mtb:
        default:
        {
            found = eobool_false;            
        } break;               
    }
    
    if(eobool_false == found)
    {
        return(eores_NOK_generic);
        
    }
    
    
    return(eo_canserv_SendCommandToLocation(eo_canserv_GetHandle(), &command, location));     
    //return(eores_OK);
}





// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



