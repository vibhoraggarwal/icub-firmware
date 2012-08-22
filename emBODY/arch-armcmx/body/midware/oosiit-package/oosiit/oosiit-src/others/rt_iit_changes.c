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
 
/* @file       rt_iit_changes.c
	@brief      This file implements internal implementation of the IIT extension for some calls of RTX.
	@author     marco.accame@iit.it
    @date       07/30/2012
    @warning    this file contains parts of code taken from various files coming from the free distribution of the
                "CMSIS-RTOS RTX Implementation" version 4.20. the code has been modified to add support to 32 bit 
                timing and to dynamic retrieval of memory used by the rtos objects. the modification is done in the
                following way:
                - if a function of RTX needs a different behaviour but keeps its declation, we just redefine it in this
                  file. obviously in its original file the funtion was made __weak.
                - if a function of RTX needs also different parameters in its number of type, we just create a new
                  function in this file with name prefixed with "iitchanged_". the old function is left unchanged in 
                  its original file. 
                hence, we attach in here the copyrigth notice of KEIL.
**/



/*-----------------------------------------------------------------------------

    License for CMSIS-RTOS RTX Implementation

    Copyright (c) 1999-2009 KEIL, 2009-2011 ARM Germany GmbH
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
     - Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     - Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     - Neither the name of ARM  nor the names of its contributors may be used 
       to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
    
-----------------------------------------------------------------------------*/


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

// acemor: there are not include guards in the header files from keil. such a shame...


#include "stdlib.h"

#include "oosiit_hid.h"

#include "oosiit_storage_hid.h"


// - types
#include "rt_TypeDef.h"
#include "RTX_Config.h" 

// - rt_time
#include "rt_Task.h"
#include "rt_Time.h"

// - rt_mutex
#include "rt_List.h"
#include "rt_Mutex.h"

// - rt_event
#include "rt_System.h"
#include "rt_Event.h"


// - rt_mailbox
#include "rt_Mailbox.h"
#include "rt_HAL_CM.h"

// - rt_semaphore
#include "rt_Semaphore.h"


// - rt_robin
#include "rt_Robin.h"


#include "rt_iit_AdvTimer.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "rt_iit_changes.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------




// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------


#if defined(OOSIIT_DBG_ENABLE)
static uint8_t s_dbg_previous_id = ev_ID_idle;
#endif



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------



extern void rt_iit_params_init(void)
{
    uint16_t offset32 = 0;
    uint16_t offset64 = 0;
    uint16_t ram32size[9], ram64size[2];

    // cannot initialise if i dont know a configuration
    if(NULL == oosiit_params_cfg)
    {
        os_error(0x60000001);
    }

    // cannot initialise if i dont have ram
    if((NULL == oosiit_params_ram32data) || (NULL == oosiit_params_ram64data) || (NULL == oosiit_params_stdlib32data))
    {
        os_error(0x60000002);
    }

    oosiit_hid_params_get_ram32(oosiit_params_cfg, ram32size);

    oosiit_hid_params_get_ram64(oosiit_params_cfg, ram64size);

    os_maxtaskrun = oosiit_params_cfg->maxnumofusertasks;
//     os_stackinfo  = (oosiit_params_cfg->checkStack << 24) |
//                     (oosiit_params_cfg->numTaskWithUserProvidedStack << 16) |
//                     (oosiit_params_cfg->sizeStack*4);
    os_stackinfo  = (oosiit_params_cfg->checkStack << 24) |
                    (oosiit_params_cfg->maxnumofusertasks << 16) |
                    (0*4);    
    os_rrobin_use  = (0 == oosiit_params_cfg->roundRobin) ? (0) : (1);
    os_rrobin_tout = oosiit_params_cfg->roundRobinTimeout;

    os_trv        = ((uint32_t)(((double)oosiit_params_cfg->osClock*(double)oosiit_params_cfg->osTick)/1E6)-1);
    oosiit_num_units_of_systick = (os_trv+1);
    oosiit_ns_per_unit_of_systick = ((uint32_t)oosiit_params_cfg->osTick*1000)/(uint32_t)(oosiit_num_units_of_systick);
    os_flags      = oosiit_params_cfg->priviledgeMode; 

    os_clockrate  =  oosiit_params_cfg->osTick;
    //os_timernum   = (0 << 16) | oosiit_params_cfg->numTimer;    // in cm3, OS_TIMER is 0
    os_timernum   = (0 << 16) | 0;    // in cm3, OS_TIMER is 0
 
    
    if(0 != ram32size[0])
    {
        uint16_t sizeofadvtmr_ptrs = (sizeof(oosiit_arrayhead_t)-4) + 4*oosiit_params_cfg->numAdvTimer;
        // in here we place the buffer for temporary pointers, which we init with its capacity and sizeofitem
        oosiit_cfg_advtmr_ptrs = &oosiit_params_ram32data[offset32];
        ((oosiit_arrayhead_t*)oosiit_cfg_advtmr_ptrs)->capacity = oosiit_params_cfg->numAdvTimer;
        ((oosiit_arrayhead_t*)oosiit_cfg_advtmr_ptrs)->sizeofitem = 4;
        // after that we put the buffer for the timers
        oosiit_cfg_advtmr_size = sizeof(uint32_t)*ram32size[0] - sizeofadvtmr_ptrs;
        oosiit_cfg_advtmr = &oosiit_params_ram32data[offset32+(sizeofadvtmr_ptrs/4)];
        offset32 += ram32size[0];
    }
    else
    {
        oosiit_cfg_advtmr_ptrs   = NULL;
        oosiit_cfg_advtmr_size   = 0;
        oosiit_cfg_advtmr        = NULL;
    }

   
    if(0 != ram32size[1])
    {   
        oosiit_cfg_mutex_size = sizeof(uint32_t)*ram32size[1];
        oosiit_cfg_mutex = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[1];
    }
    else
    {
        oosiit_cfg_mutex_size    = 0;
        oosiit_cfg_mutex         = NULL;
    }

    
    if(0 != ram32size[2])
    {   
        oosiit_cfg_semaphore_size = sizeof(uint32_t)*ram32size[2];
        oosiit_cfg_semaphore = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[2];
    }
    else
    {
        oosiit_cfg_semaphore_size    = 0;
        oosiit_cfg_semaphore         = NULL;
    }
    
    if(0 != ram32size[3])
    {   
        oosiit_cfg_mbox_size = sizeof(uint32_t)*ram32size[3];
        oosiit_cfg_mbox = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[3];
    }

    if(0 != ram32size[4])
    {
        oosiit_cfg_mutex_memory = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[4];
    }
    else
    {
        oosiit_cfg_mutex_memory    = 0;
    }

    if(0 != ram32size[5])
    {   
        mp_tcb_size = sizeof(uint32_t)*ram32size[5];
        mp_tcb = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[5];
    }
    else
    {
        mp_tcb_size    = 0;
        mp_tcb         = NULL;
    }

    if(0 != ram32size[6])
    {   
        // error: os_fifo_size = sizeof(uint32_t)*ram32size[6];
        // os_fifo_size contains the number of items, not the size in bytes of teh memory
        os_fifo_size = oosiit_params_cfg->sizeISRFIFO;
        os_fifo = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[6];
    }
    else
    {
        os_fifo_size    = 0;
        os_fifo         = NULL;
    }


    if(0 != ram32size[7])
    {   
        os_active_TCB = (void**) &oosiit_params_ram32data[offset32];
        offset32 += ram32size[7];
    }
    else
    {
        os_active_TCB    = 0;
    }


    if(0 != ram32size[8])
    {   
        mp_tmr_size = sizeof(uint32_t)*ram32size[8];
        m_tmr = &oosiit_params_ram32data[offset32];
        offset32 += ram32size[8];
    }
    else
    {
        mp_tmr_size    = 0;
        m_tmr         = NULL;
    }


    if(0 != ram64size[0])
    {   
        oosiit_cfg_globstack_size = sizeof(uint64_t)*ram64size[0];
        oosiit_cfg_globstack = &oosiit_params_ram64data[offset64];
        offset64 += ram64size[0];
    }
    else
    {
        oosiit_cfg_globstack_size    = 0;
        oosiit_cfg_globstack         = NULL;
    }

    if(0 != ram64size[1])
    {   
        mp_stk_size = sizeof(uint64_t)*ram64size[1];
        mp_stk = &oosiit_params_ram64data[offset64];
        offset64 += ram64size[1];
    }
    else
    {
        mp_stk_size    = 0;
        mp_stk         = NULL;
    }


    std_libspace = (U32 (*)[24])rt_iit_libspace_init();
    if(NULL == std_libspace)
    {
        os_error(0x60000002);    
    }


    // initialisation to zero of os variables
    {
        extern BIT os_lock;             //IIT-EXT
        extern BIT os_psh_flag;         //IIT-EXT
        extern U8  pend_flags;          //IIT-EXT

        os_lock = 0;
        os_psh_flag = 0;
        pend_flags = 0;    

    }
    
        
}


extern U32* rt_iit_libspace_init(void)
{
    uint16_t sizes[2];

    if(NULL == oosiit_params_cfg)
    {
        return(NULL);
    }



    oosiit_hid_params_get_stdlib32(oosiit_params_cfg, sizes);

    if(0 != sizes[0])
    {   
        return(&oosiit_params_stdlib32data[0]);
    }
    else
    {
        return(NULL);
    }     

}


extern void rt_iit_time_set(U32 low, U32 high) 
{
    U64 target = 0; 
    U64 current = 0;



    // retrigger the systick
    (*((volatile U32 *)0xE000E018)) = 0;

    target = ((U64)high << 32) + (U64)low;
    current = oosiit_time;

    if(target == current)
    {
	    
        return;
    }
    
    // change the time
    oosiit_time = target;

    // adjust the absolute-time data-structure: advanced timers
    rt_iit_advtmr_synchronise(current);
	
}



// - system task management routines: rt_System.c ---------------------------------------------------------------------

#include "rt_Timer.h"

#define IIT_EXEC_ONPENDSV_IN_SYSTICK //IIT-EXT

//IIT-EXT: made them global
extern BIT os_lock;
extern BIT os_psh_flag;
extern U8  pend_flags;

void rt_tsk_lock (void) {
  /* Prevent task switching by locking out scheduler */
  // IIT-EXT: we need to read the register in order to clear bit 16, the COUNTFLAG
  volatile U32 nvicstctrl = NVIC_ST_CTRL;
  
  nvicstctrl = nvicstctrl;
  OS_LOCK();
  os_lock = __TRUE;
  OS_UNPEND (&pend_flags);
  ;
}

void rt_tsk_unlock (void) {
  /* Unlock scheduler and re-enable task switching */
  // IIT-EXT: added the execution of teh systick if the timer was expired in the meantime.
  // warning: the systick executes only one even if the lock lastd for more than one period
  volatile U32 nvicstctrl = NVIC_ST_CTRL;
  					 
  if(0x00010000 == (nvicstctrl & 0x00010000))
  { // bit 16 is the COUNTFLAG: reads as 1 if counter reaches 0 since this is the last time this register
    // is read. clear to 0 automatically when read or when current counter value is cleared.
	// in our case, OS_LOCK() cleared it.
  	pend_flags |= 1; // in position 1 there is the systick flag
  }

  OS_UNLOCK();
  os_lock = __FALSE;
  OS_PEND (pend_flags, os_psh_flag);
  os_psh_flag = __FALSE;
  
}

#ifdef IIT_EXEC_ONPENDSV_IN_SYSTICK
void rt_iit_pop_req_base (void) {
  struct OS_XCB *p_CB;
  U32  idx;

  idx = os_psq->last;
  while (os_psq->count) {
    p_CB = os_psq->q[idx].id;
    if (p_CB->cb_type == TCB) {
      /* Is of TCB type */
      // rt_evt_psh ((P_TCB)p_CB, (U16)os_psq->q[idx].arg);     //IIT-EXT
      iitchanged_rt_evt_psh ((U32)p_CB, (U32)os_psq->q[idx].arg);    //IIT-EXT
    }
    else if (p_CB->cb_type == MCB) {
      /* Is of MCB type */
      rt_mbx_psh ((P_MCB)p_CB, (void *)os_psq->q[idx].arg);
      // if we want send2front: rt_iit_mbx_psh((P_MCB)p_CB, (void *)os_psq->q[idx].arg, os_psq->q[idx].flags); //IIT-EXT
    }
    else if (p_CB->cb_type == ATCB) { //IIT-EXT
      rt_advtmr_psh ((OS_ID)p_CB, (U32)os_psq->q[idx].arg); //IIT-EXT
    } //IIT-EXT
    else if (p_CB->cb_type == SCB) { //IIT-EXT
      /* Must be of SCB type */
      //rt_sem_psh ((P_SCB)p_CB); //IIT-EXT
      iitchanged_rt_sem_psh((P_SCB)p_CB);
    }
    if (++idx == os_psq->size) idx = 0;
    rt_dec (&os_psq->count);
  }
  os_psq->last = idx;

}
#endif 

#ifdef IIT_EXEC_ONPENDSV_IN_SYSTICK
void rt_iit_pop_req_inside_systick (void) {

  	if((1<<28) == ((1<<28) & (NVIC_INT_CTRL)))
  	{
		rt_iit_pop_req_base();
  		// clear
  		NVIC_INT_CTRL  |= (1<<27);
	}

}
#endif

void rt_pop_req (void) {
  /* Process an ISR post service requests. */
#ifdef IIT_EXEC_ONPENDSV_IN_SYSTICK
  
  P_TCB next;

  rt_iit_dbg_pendsv_enter(); // IIT-EXT

  os_tsk.run->state = READY;
  rt_put_rdy_first (os_tsk.run);

  rt_iit_pop_req_base();

  next = rt_get_first (&os_rdy);
  rt_switch_req (next);

  rt_iit_dbg_pendsv_exit(); // IIT-EXT

#else

  struct OS_XCB *p_CB;
  P_TCB next;
  U32  idx;

  rt_iit_dbg_pendsv_enter(); // IIT-EXT

  os_tsk.run->state = READY;
  rt_put_rdy_first (os_tsk.run);

  idx = os_psq->last;
  while (os_psq->count) {
    p_CB = os_psq->q[idx].id;
    if (p_CB->cb_type == TCB) {
      /* Is of TCB type */
      // rt_evt_psh ((P_TCB)p_CB, (U16)os_psq->q[idx].arg);     //IIT-EXT: removed
      rt_iit_evt_psh ((U32)p_CB, (U32)os_psq->q[idx].arg);    //IIT-EXT: added
    }
    else if (p_CB->cb_type == MCB) {
      /* Is of MCB type */
      rt_mbx_psh ((P_MCB)p_CB, (void *)os_psq->q[idx].arg);
      // if we want send2front: rt_iit_mbx_psh((P_MCB)p_CB, (void *)os_psq->q[idx].arg, os_psq->q[idx].flags); //IIT-EXT
    }
    else if (p_CB->cb_type == ATCB) { //IIT-EXT
      rt_advtmr_psh ((OS_ID)p_CB, (U32)os_psq->q[idx].arg); //IIT-EXT
    } //IIT-EXT
    else {
      /* Must be of SCB type */
      //rt_sem_psh ((P_SCB)p_CB); //IIT-EXT
      rt_iit_sem_psh((P_SCB)p_CB);
    }
    if (++idx == os_psq->size) idx = 0;
    rt_dec (&os_psq->count);
  }
  os_psq->last = idx;

  next = rt_get_first (&os_rdy);
  rt_switch_req (next);

  rrt_iit_dbg_pendsv_exit(); // IIT-EXT

#endif
}

#ifdef __CMSIS_RTOS
extern void sysTimerTick(void);
#endif

void rt_systick (void) {
  /* Check for system clock update, suspend running task. */
  P_TCB next;

  rt_iit_dbg_systick_enter(); // IIT-EXT

  os_tsk.run->state = READY;
  rt_put_rdy_first (os_tsk.run);

  if(0 == os_tsk.run->prio)		//IIT-EXT
  {								//IIT-EXT
        oosiit_idletime++; 	//IIT-EXT
  }								//IIT-EXT

//  os_tsk.run->total_run_time++; //IIT-EXT

  /* Check Round Robin timeout. */
  iitchanged_rt_chk_robin ();			//IIT-EXT

  /* Update delays. */
  os_time++;
  oosiit_time++;				//IIT-EXT
  iitchanged_rt_dec_dly ();			//IIT-EXT


  /* Check the user timers. */
#ifdef __CMSIS_RTOS
  sysTimerTick();
  rt_iit_advtmr_tick();			//IIT-EXT
#else
  rt_tmr_tick ();
  rt_iit_advtmr_tick();			//IIT-EXT
#endif

#ifdef IIT_EXEC_ONPENDSV_IN_SYSTICK	
  rt_iit_pop_req_inside_systick(); //IIT-EXT
#endif

  /* Switch back to highest ready task */
  next = rt_get_first (&os_rdy);
  rt_switch_req (next);


  rt_iit_dbg_systick_exit();	// IIT-EXT
}


// - time management routines: rt_Time.c ------------------------------------------------------------------------------

void iitchanged_rt_dly_wait (TIME_t delay_time) {

   /* Delay task by "delay_time" */
   iitchanged_rt_block (delay_time, WAIT_DLY, NOTIMEOUT);   
} 

void iitchanged_rt_itv_set (TIME_t interval_time) {

   
   /* Set interval length and define start of first interval */
   os_tsk.run->interval_time = interval_time;
   os_tsk.run->delta_time = interval_time + oosiit_time;
   
} 

void iitchanged_rt_itv_wait (void) {
   /* Wait for interval end and define start of next one */
   TIME_t delta;

   delta = os_tsk.run->delta_time - oosiit_time;
   os_tsk.run->delta_time += os_tsk.run->interval_time;

   if ((delta & MSB_TIME_t) == 0) {
      iitchanged_rt_block (delta, WAIT_ITV, NOTIMEOUT);
   }
   
} 


// - mutex management routines: rt_Mutex.c ----------------------------------------------------------------------------

OS_RESULT iitchanged_rt_mut_wait (OS_ID mutex, TIME_t timeout) {
   /* Wait for a mutex, continue when mutex is free. */
   P_MUCB p_MCB = mutex;
   	
   if (p_MCB->level == 0) {
      p_MCB->owner = os_tsk.run;
      p_MCB->prio  = os_tsk.run->prio;
      goto inc;
   }
   if (p_MCB->owner == os_tsk.run) {
      /* OK, running task is the owner of this mutex. */
inc: p_MCB->level++;
      
      return (OS_R_OK);
   }
   /* Mutex owned by another task, wait until released. */
   if (timeout == 0) {
      
      return (OS_R_TMO);
   }
   /* Raise the owner task priority if lower than current priority. */
   /* This priority inversion is called priority inheritance.       */
   if (p_MCB->prio < os_tsk.run->prio) {
      p_MCB->owner->prio = os_tsk.run->prio;
      rt_resort_prio (p_MCB->owner);
   }
   if (p_MCB->p_lnk != NULL) {
      rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
   }
   else {
      p_MCB->p_lnk = os_tsk.run;
      os_tsk.run->p_lnk  = NULL;
      os_tsk.run->p_rlnk = (P_TCB)p_MCB;
   }
   iitchanged_rt_block(timeout, WAIT_MUT, NOTIMEOUT);
   
   return (OS_R_TMO);
} 


// - event flag management routines: rt_Event.c -----------------------------------------------------------------------

OS_RESULT iitchanged_rt_evt_wait (EVENT_t wait_flags, TIME_t timeout, BOOL and_wait) {
   /* Wait for one or more event flags with optional time-out.               */
   /* "wait_flags" identifies the flags to wait for.                         */
   /* "timeout" is the time-out limit in system ticks (0xffff-ffff if no time-out)*/
   /* "and_wait" specifies the AND-ing of "wait_flags" as condition to be met*/
   /* to complete the wait. (OR-ing if set to 0).                            */
   U32 block_state;

   if (and_wait) {
      /* Check for AND-connected events */
      if ((os_tsk.run->events & wait_flags) == wait_flags) {
         os_tsk.run->events &= ~wait_flags;
		 
         return (OS_R_EVT);
      }
      block_state = WAIT_AND;
   }
   else {
      /* Check for OR-connected events */
      if (os_tsk.run->events & wait_flags) {
         os_tsk.run->waits = os_tsk.run->events & wait_flags;
         os_tsk.run->events &= ~wait_flags;
		 
         return (OS_R_EVT);
      }
      block_state = WAIT_OR;
   }
   /* Task has to wait */
   os_tsk.run->waits = wait_flags;
   iitchanged_rt_block (timeout, (U8)block_state, NOTIMEOUT);
   
   return (OS_R_TMO);
} 


void iitchanged_rt_evt_set (EVENT_t event_flags, OS_TID task_id) {
  /* Set one or more event flags of a selectable task. */
  P_TCB p_tcb;

  p_tcb = os_active_TCB[task_id-1];
  if (p_tcb == NULL) {
    
    return;
  }
  p_tcb->events |= event_flags;
  event_flags    = p_tcb->waits;
  /* If the task is not waiting for an event, it should not be put */
  /* to ready state. */
  if (p_tcb->state == WAIT_AND) {
    /* Check for AND-connected events */
    if ((p_tcb->events & event_flags) == event_flags) {
      goto wkup;
    }
  }
  if (p_tcb->state == WAIT_OR) {
    /* Check for OR-connected events */
    if (p_tcb->events & event_flags) {
      p_tcb->waits  &= p_tcb->events;
wkup: p_tcb->events &= ~event_flags;
      rt_rmv_dly (p_tcb);
      p_tcb->state   = READY;
#ifdef __CMSIS_RTOS
      rt_ret_val2(p_tcb, 0x08/*osEventSignal*/, p_tcb->waits); 
#else
      rt_ret_val (p_tcb, OS_R_EVT);
#endif
      rt_dispatch (p_tcb);
    }
  }

  
}

void iitchanged_rt_evt_clr (EVENT_t clear_flags, OS_TID task_id) {
  /* Clear one or more event flags (identified by "clear_flags") of a */
  /* selectable task (identified by "task"). */
  P_TCB task = os_active_TCB[task_id-1];

  

  if (task == NULL) {
    
    return;
  }
  task->events &= ~clear_flags;

  
}

void iitchanged_rt_evt_clr_runningtask(EVENT_t flags)
{
    os_tsk.run->events &= ~flags;
}


void iitchanged_isr_evt_set (EVENT_t event_flags, OS_TID task_id) {
  /* Same function as "os_evt_set", but to be called by ISRs. */
  P_TCB p_tcb = os_active_TCB[task_id-1];

  if (p_tcb == NULL) {
    return;
  }
  rt_psq_enq (p_tcb, event_flags);
  rt_psh_req ();
}

EVENT_t iitchanged_rt_evt_get (void) {
  
  /* Get events of a running task after waiting for OR connected events. */
  return (os_tsk.run->waits);
}

void iitchanged_rt_evt_psh (U32 arg_u32, U32 set_flags) {
  /* Check if task has to be waken up */
  U32 event_flags;
  P_TCB p_CB = (P_TCB)arg_u32; 

  p_CB->events |= set_flags;
  event_flags = p_CB->waits;
  if (p_CB->state == WAIT_AND) {
    /* Check for AND-connected events */
    if ((p_CB->events & event_flags) == event_flags) {
      goto rdy;
    }
  }
  if (p_CB->state == WAIT_OR) {
    /* Check for OR-connected events */
    if (p_CB->events & event_flags) {
      p_CB->waits  &= p_CB->events;
rdy:  p_CB->events &= ~event_flags;
      rt_rmv_dly (p_CB);
      p_CB->state   = READY;
#ifdef __CMSIS_RTOS
      rt_ret_val2(p_CB, 0x08/*osEventSignal*/, p_CB->waits); 
#else
      rt_ret_val (p_CB, OS_R_EVT);
#endif
      rt_put_prio (&os_rdy, p_CB);
    }
  }
}

// - mailbox management routines: rt_Mailbox.c ------------------------------------------------------------------------

OS_RESULT iitchanged_rt_mbx_send (OS_ID mailbox, void *p_msg, TIME_t timeout) {
   /* Send message to a mailbox */
   P_MCB p_MCB = mailbox;
   P_TCB p_TCB;

   

  if (p_MCB->state == 1) {
    /* A task is waiting for message */
    p_TCB = rt_get_first ((P_XCB)p_MCB);
    if (p_MCB->p_lnk == NULL) {
      p_MCB->state = 0;
    }
#ifdef __CMSIS_RTOS
    rt_ret_val2(p_TCB, 0x10/*osEventMessage*/, (U32)p_msg);
#else
    *p_TCB->msg = p_msg;
    rt_ret_val (p_TCB, OS_R_MBX);
#endif
    rt_rmv_dly (p_TCB);
    rt_dispatch (p_TCB);
  }
   else {
      /* Store message in mailbox queue */
      if (p_MCB->count == p_MCB->size) {
         /* No free message entry, wait for one. If message queue is full, */
         /* then no task is waiting for message. The 'p_MCB->p_lnk' list   */
         /* pointer can now be reused for send message waits task list.    */
         if (timeout == 0) {
		    
            return (OS_R_TMO);
         }
         if (p_MCB->p_lnk != NULL) {
            rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
         }
         else {
            p_MCB->p_lnk = os_tsk.run;
            os_tsk.run->p_lnk  = NULL;
            os_tsk.run->p_rlnk = (P_TCB)p_MCB;
        /* Task is waiting to send a message */      
        p_MCB->state = 2;
         }
         os_tsk.run->msg = p_msg;
         iitchanged_rt_block (timeout, WAIT_MBX, NOTIMEOUT);
		 
         return (OS_R_TMO);
      }
      /* Yes, there is a free entry in a mailbox. */
      p_MCB->msg[p_MCB->first] = p_msg;
      rt_inc (&p_MCB->count);
      if (++p_MCB->first == p_MCB->size) {
         p_MCB->first = 0;
      }
   }
   
   return (OS_R_OK);
} 



OS_RESULT iitchanged_rt_mbx_wait (OS_ID mailbox, void **message, TIME_t timeout) {
   /* Receive a message; possibly wait for it */
   P_MCB p_MCB = mailbox;
   P_TCB p_TCB;
   

   /* If a message is available in the fifo buffer */
   /* remove it from the fifo buffer and return. */
   if (p_MCB->count) {
      *message = p_MCB->msg[p_MCB->last];
      if (++p_MCB->last == p_MCB->size) {
         p_MCB->last = 0;
      }
    if (p_MCB->state == 2) {
      /* A task is waiting to send message */
      p_TCB = rt_get_first ((P_XCB)p_MCB);
      if (p_MCB->p_lnk == NULL) {
        p_MCB->state = 0;
      }
#ifdef __CMSIS_RTOS
      rt_ret_val(p_TCB, 0/*osOK*/);
#else
      rt_ret_val(p_TCB, OS_R_OK);
#endif
         p_MCB->msg[p_MCB->first] = p_TCB->msg;
         if (++p_MCB->first == p_MCB->size) {
            p_MCB->first = 0;
         }
         rt_rmv_dly (p_TCB);
         rt_dispatch (p_TCB);
      }
      else {
         rt_dec (&p_MCB->count);
      }
	  
      return (OS_R_OK);
   }
   /* No message available: wait for one */
   if (timeout == 0) {
      
      return (OS_R_TMO);
   }
   if (p_MCB->p_lnk != NULL) {
      rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
   }
   else {
      p_MCB->p_lnk = os_tsk.run;
      os_tsk.run->p_lnk = NULL;
      os_tsk.run->p_rlnk = (P_TCB)p_MCB;
    /* Task is waiting to receive a message */      
    p_MCB->state = 1;       
   }
   iitchanged_rt_block(timeout, WAIT_MBX, NOTIMEOUT);
#ifndef __CMSIS_RTOS
  os_tsk.run->msg = message;
#endif
  
   return (OS_R_TMO);
} 

// not present in distribution of CMSIS-RTOS RTX
OS_RESULT rt_iit_mbx_count(OS_ID mailbox) {

  /* Gives the used space in a mailbox. Returns the number of messages     */
  /* currently stored in the mailbox. It returns 0 when mailbox is empty.   */
  P_MCB p_MCB = mailbox;
  return (p_MCB->count);
}

// not present in distribution of CMSIS-RTOS RTX
OS_RESULT rt_iit_isr_mbx_count(OS_ID mailbox) {

  /* Gives the used space in a mailbox. Returns the number of messages     */
  /* currently stored in the mailbox. It returns 0 when mailbox is empty.   */
  P_MCB p_MCB = mailbox;
  return (p_MCB->count);
}


// experimental support for 2front and 2back - REMOVED because 
// bla
// bla
// bla
//#define MSG2NONE    0
//#define MSG2BACK    1
//#define MSG2FRONT   2
//
//OS_RESULT iitchanged_rt_mbx_send (OS_ID mailbox, void *p_msg, TIME_t timeout) {
//   /* Send message to a mailbox */
//   P_MCB p_MCB = mailbox;
//   P_TCB p_TCB;
//
//   if (p_MCB->p_lnk != NULL && p_MCB->count == 0) {
//      /* A task is waiting for message */
//      p_TCB = rt_get_first ((P_XCB)p_MCB);
//      *p_TCB->msg    = p_msg;
//      p_TCB->ret_val = OS_R_MBX;
//      rt_rmv_dly (p_TCB);
//      rt_dispatch (p_TCB);
//      os_tsk.run->ret_val = OS_R_OK;
//   }
//   else {
//      /* Store message in mailbox queue */
//      if (p_MCB->count == p_MCB->size) {
//         /* No free message entry, wait for one. If message queue is full, */
//         /* then no task is waiting for message. The 'p_MCB->p_lnk' list   */
//         /* pointer can now be reused for send message waits task list.    */
//         if (timeout == 0) {
//            return (OS_R_TMO);
//         }
//         if (p_MCB->p_lnk != NULL) {
//            rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
//         }
//         else {
//            p_MCB->p_lnk = os_tsk.run;
//            os_tsk.run->p_lnk  = NULL;
//            os_tsk.run->p_rlnk = (P_TCB)p_MCB;
//            /* Signal the 'isr_mbx_receive ()' that the task is waiting */
//            /* to send a message */
//            p_MCB->isr_st = 1;
//         }
//         os_tsk.run->msgsendmode = MSG2BACK; //IIT-EXT
//         os_tsk.run->msg = p_msg;
//         iitchanged_rt_block (timeout, WAIT_MBX, NOTIMEOUT);
//         return (OS_R_TMO);
//      }
//      /* Yes, there is a free entry in a mailbox. */
//      p_MCB->msg[p_MCB->first] = p_msg;
//      rt_inc (&p_MCB->count);
//      if (++p_MCB->first == p_MCB->size) {
//         p_MCB->first = 0;
//      }
//   }
//   return (OS_R_OK);
//} 
//
//
//OS_RESULT iitchanged_rt_mbx_send2front (OS_ID mailbox, void *p_msg, TIME_t timeout) {
//   /* Send message to a mailbox */
//   P_MCB p_MCB = mailbox;
//   P_TCB p_TCB;
//
//   if (p_MCB->p_lnk != NULL && p_MCB->count == 0) {
//      /* A task is waiting for message */
//      // acemor: ok, i dont even put in mbox ...
//      p_TCB = rt_get_first ((P_XCB)p_MCB);
//      *p_TCB->msg    = p_msg;
//      p_TCB->ret_val = OS_R_MBX;
//      rt_rmv_dly (p_TCB);
//      rt_dispatch (p_TCB);
//      os_tsk.run->ret_val = OS_R_OK;
//   }
//   else {
//      /* Store message in mailbox queue */
//      if (p_MCB->count == p_MCB->size) {
//         /* No free message entry, wait for one. If message queue is full, */
//         /* then no task is waiting for message. The 'p_MCB->p_lnk' list   */
//         /* pointer can now be reused for send message waits task list.    */
//         if (timeout == 0) { // acemor: ok, i dont put
//            return (OS_R_TMO);
//         }
//         if (p_MCB->p_lnk != NULL) { // acemor: facenda
//            rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
//         }
//         else { // acemor: no task is waiting on this mbx, thus i put this task in block mode.
//                //         i tell the ??? that the msg is to be put in front or back by the value of msgsendmode 
//            p_MCB->p_lnk = os_tsk.run;
//            os_tsk.run->p_lnk  = NULL;
//            os_tsk.run->p_rlnk = (P_TCB)p_MCB;
//            /* Signal the 'isr_mbx_receive ()' that the task is waiting */
//            /* to send a message */
//            p_MCB->isr_st = 1;
//         }
//         os_tsk.run->msgsendmode = MSG2FRONT; //IIT-EXT
//         os_tsk.run->msg = p_msg;
//         iitchanged_rt_block (timeout, WAIT_MBX, NOTIMEOUT);
//         return (OS_R_TMO);
//      }
//      /* Yes, there is a free entry in a mailbox. */
//      // acemor: add in last, but before i decrement it
//      if (0 == p_MCB->last) {
//         p_MCB->last = (p_MCB->size - 1);
//      }
//      else {
//         p_MCB->last--;
//      }
//      p_MCB->msg[p_MCB->last] = p_msg;
//      rt_inc (&p_MCB->count); 
//
//   }
//   return (OS_R_OK);
//}
//
//OS_RESULT iitchanged_rt_mbx_wait (OS_ID mailbox, void **message, TIME_t timeout) {
//   /* Receive a message; possibly wait for it */
//   P_MCB p_MCB = mailbox;
//   P_TCB p_TCB;
//
//   /* If a message is available in the fifo buffer */
//   /* remove it from the fifo buffer and return. */
//   if (p_MCB->count) {
//      *message = p_MCB->msg[p_MCB->last];
//      if (++p_MCB->last == p_MCB->size) {
//         p_MCB->last = 0;
//      }
//      if (p_MCB->p_lnk != NULL) {
//         /* A task is waiting to send message */
//         // acemor: normally in back (0) or in front (1)
//         p_TCB = rt_get_first ((P_XCB)p_MCB);
//         p_TCB->ret_val = OS_R_OK;
//         if(MSG2FRONT ==  p_TCB->msgsendmode)
//         {  // in front
//            if (0 == p_MCB->last) {
//               p_MCB->last = (p_MCB->size - 1);
//            }
//            else {
//               p_MCB->last--;
//            }
//            p_MCB->msg[p_MCB->last] = p_TCB->msg;
//         }
//         else //if(MSG2BACK == p_TCB->msgsendmode)
//         {  // in back
//            p_MCB->msg[p_MCB->first] = p_TCB->msg;
//            if (++p_MCB->first == p_MCB->size) {
//               p_MCB->first = 0;
//            }
//         }
//         p_TCB->msgsendmode = MSG2NONE;
//         rt_rmv_dly (p_TCB);
//         rt_dispatch (p_TCB);
//         os_tsk.run->ret_val = OS_R_OK;
//      }
//      else {
//         rt_dec (&p_MCB->count);
//      }
//      return (OS_R_OK);
//   }
//   /* No message available: wait for one */
//   if (timeout == 0) {
//      return (OS_R_TMO);
//   }
//   if (p_MCB->p_lnk != NULL) {
//      rt_put_prio ((P_XCB)p_MCB, os_tsk.run);
//   }
//   else {
//      p_MCB->p_lnk = os_tsk.run;
//      os_tsk.run->p_lnk = NULL;
//      os_tsk.run->p_rlnk = (P_TCB)p_MCB;
//   }
//   iitchanged_rt_block(timeout, WAIT_MBX, NOTIMEOUT);
//   os_tsk.run->msg = message;
//   return (OS_R_TMO);
//} 
//
//
//void isr_iit_mbx_send (OS_ID mailbox, void *p_msg) {
//  /* Same function as "os_mbx_send", but to be called by ISRs. */
//  P_MCB p_MCB = mailbox;
//
//  rt_iit_psq_enq (p_MCB, (U32)p_msg, MSG2BACK);
//  rt_psh_req ();
//}
//
//
//void isr_iit_mbx_send2front (OS_ID mailbox, void *p_msg) {
//  /* Same function as "os_mbx_send", but to be called by ISRs. */
//  P_MCB p_MCB = mailbox;
//
//  rt_iit_psq_enq (p_MCB, (U32)p_msg, MSG2FRONT);
//  rt_psh_req ();
//}
//
//void rt_iit_mbx_psh (void *p, void *p_msg, U32 flags) {
//
//  P_MCB p_CB = (P_MCB) p;
//  /* Store the message to the mailbox queue or pass it to task directly. */
//  P_TCB p_TCB;
//
//  /* Check if this was an 'isr_mbx_receive ()' post service request.   */
//  if (p_CB->p_lnk != NULL && p_CB->isr_st == 2) {
//    /* A task is waiting to send message, remove it from the waiting list. */
//    p_CB->isr_st = 0;
//    p_TCB = rt_get_first ((P_XCB)p_CB);
//    p_TCB->ret_val = OS_R_OK;
//    /* Store the message to the mailbox queue. */
//    // acemor: it can be put in front or in back. it was a call to iitchanged_rt_mbx_send or iitchanged_rt_mbx_send2front
//    //         which generated a wait of the task. the isr does not wait.
//
//     if(MSG2FRONT ==  p_TCB->msgsendmode)
//     {  // in front
//        if (0 == p_CB->last) {
//           p_CB->last = (p_CB->size - 1);
//        }
//        else {
//           p_CB->last--;
//        }
//        p_CB->msg[p_CB->last] = p_TCB->msg;
//        rt_inc (&p_CB->count);
//     }
//     else //if(MSG2BACK == p_TCB->msgsendmode)
//     {
//        p_CB->msg[p_CB->first] = p_TCB->msg;
//        rt_inc (&p_CB->count);
//        if (++p_CB->first == p_CB->size) {
//          p_CB->first = 0;
//        }
//     }
////    p_CB->msg[p_CB->first] = p_TCB->msg;
////    rt_inc (&p_CB->count);
////    if (++p_CB->first == p_CB->size) {
////      p_CB->first = 0;
////    }
//    goto rdy_mbx;
//  }
//
//  // acemor: in here it is just the isr_mbx_send() whcih posted the request to send a given mbx to put a msg.
//  //         we just accept that the send from an isr is always in the back
//  /* A task is waiting for message, pass the message to task directly. */
//  if (p_CB->p_lnk != NULL && p_CB->count == 0) { 
//    // acemor: the isr has posted a msg and we already have a task which waits for it, thus we give it directly
//    //         this code stays the same also if we post in front and in back
//    p_TCB = rt_get_first ((P_XCB)p_CB);
//    *p_TCB->msg = p_msg;
//    p_TCB->ret_val = OS_R_MBX;
//rdy_mbx: p_TCB->state = READY;
//    rt_rmv_dly (p_TCB);
//    rt_put_prio (&os_rdy, p_TCB);
//  }
//  else {
//    /* No task is waiting for message, store the message to the mailbox queue.*/
//    // the isr has posted a msg but we dont have a task whcih waits for it, thus we put it in the queue
//    // only way to put it in front / back is to have another argument of this function which tells front/back,
//    // thus adding a third field in rt_psq_enq() ... i dont do it for now.
//    if (p_CB->count < p_CB->size) {
//
//      if(MSG2FRONT == flags)
//      {
//        if (0 == p_CB->last) {
//           p_CB->last = (p_CB->size - 1);
//        }
//        else {
//           p_CB->last--;
//        }
//        p_CB->msg[p_CB->last] = p_msg;
//        rt_inc (&p_CB->count);
//      }
//      else
//      {
//          p_CB->msg[p_CB->first] = p_msg;
//          rt_inc (&p_CB->count);
//          if (++p_CB->first == p_CB->size) {
//            p_CB->first = 0;
//          }
//      }
//
//    }
//    else {
//      // acemor: are u crazY? u should do something different ...
//      os_error (OS_ERR_MBX_OVF);
//    }
//  }
//}
//
//
//OS_RESULT rt_iit_mbx_count(OS_ID mailbox) {
//  /* Gives the used space in a mailbox. Returns the number of messages     */
//  /* currently stored in the mailbox. It returns 0 when mailbox is empty.   */
//  P_MCB p_MCB = mailbox;
//
//  return (p_MCB->count);
//}

// - semaphore management routines: rt_Semaphore.c --------------------------------------------------------------------


void iitchanged_rt_sem_init (OS_ID semaphore, U8 token_max, U8 token_count) {
  /* Initialize a semaphore */
  P_SCB p_SCB = semaphore;


  if(token_count > token_max)
  {
    token_count = token_max;
  }

  p_SCB->cb_type = SCB;
  p_SCB->p_lnk  = NULL;
  p_SCB->maxtokens = token_max;
  p_SCB->tokens = token_count;
  
}


// not present in distribution of CMSIS-RTOS RTX
OS_RESULT rt_iit_sem_set (OS_ID semaphore, U8 ntokens) {
  /* Return some tokens to semaphore */
  P_SCB p_SCB = semaphore;
  P_TCB p_TCB;
  U16 tkns = p_SCB->tokens;


  if(ntokens > p_SCB->maxtokens)
  { // cannot change ...
    return (OS_R_NOK);
  }

  if(ntokens == tkns)
  { // nothing changes, thus ... no action to do
    return (OS_R_OK);
  }

  if(p_SCB->p_lnk == NULL)
  { // no task is waiting, thus we can change the number without problems ...
    p_SCB->tokens = ntokens;
    return (OS_R_OK);
  }

  // we change the number of tokens and one or more tasks are waiting ...
  // in this case for sure p_SCB->tokens is zero. what we have to do is to
  // signal m waiting tasks (where m <= ntokens) that they could get the semaphore
  // and then set p_SCB->tokens to (ntokens - m)

  while((p_SCB->p_lnk == NULL) && (ntokens > 0))
  {
    p_TCB = rt_get_first ((P_XCB)p_SCB);
#ifdef __CMSIS_RTOS
    rt_ret_val(p_TCB, 1);
#else
    rt_ret_val(p_TCB, OS_R_SEM);
#endif
    rt_rmv_dly (p_TCB);
    rt_dispatch (p_TCB);
    //os_tsk.run->ret_val = OS_R_OK;
    ntokens --;
  }

  p_SCB->tokens = ntokens;
 
  return (OS_R_OK);
}


OS_RESULT iitchanged_rt_sem_send (OS_ID semaphore) {
  /* Return a token to semaphore */
  P_SCB p_SCB = semaphore;
  P_TCB p_TCB;
  
  
  if (p_SCB->p_lnk != NULL) {
    /* A task is waiting for token */
    p_TCB = rt_get_first ((P_XCB)p_SCB);
#ifdef __CMSIS_RTOS
    rt_ret_val(p_TCB, 1);
#else
    rt_ret_val(p_TCB, OS_R_SEM);
#endif
    rt_rmv_dly (p_TCB);
    rt_dispatch (p_TCB);
  }
  else {
    /* Store token. */
    if(p_SCB->tokens == p_SCB->maxtokens)
    {
	    
        return(OS_R_NOK);
    }
    else
    {
	    
        p_SCB->tokens++;
    }
  }
  return (OS_R_OK);
}

OS_RESULT iitchanged_rt_sem_wait (OS_ID semaphore, TIME_t timeout) {
   /* Obtain a token; possibly wait for it */
   P_SCB p_SCB = semaphore;


   if (p_SCB->tokens) {
      p_SCB->tokens--;	  
      return (OS_R_OK);
   }
   /* No token available: wait for one */
   if (timeout == 0) {      
      return (OS_R_TMO);
   }
   if (p_SCB->p_lnk != NULL) {
      rt_put_prio ((P_XCB)p_SCB, os_tsk.run);
   }
   else {
      p_SCB->p_lnk = os_tsk.run;
      os_tsk.run->p_lnk = NULL;
      os_tsk.run->p_rlnk = (P_TCB)p_SCB;
   }
   iitchanged_rt_block(timeout, WAIT_SEM, NOTIMEOUT);
   
   return (OS_R_TMO);
} 


void iitchanged_isr_sem_send (OS_ID semaphore) {
  /* Same function as "os_sem"send", but to be called by ISRs */
  P_SCB p_SCB = semaphore;

  rt_psq_enq (p_SCB, 0);
  rt_psh_req ();
}


void iitchanged_rt_sem_psh (void *p) {
  P_SCB p_CB = (P_SCB) p;
  /* Check if task has to be waken up */
  P_TCB p_TCB;

  if (p_CB->p_lnk != NULL) {
    /* A task is waiting for token */
    p_TCB = rt_get_first ((P_XCB)p_CB);
    rt_rmv_dly (p_TCB);
    p_TCB->state   = READY;
#ifdef __CMSIS_RTOS
    rt_ret_val(p_TCB, 1);
#else
    rt_ret_val(p_TCB, OS_R_SEM);
#endif
    rt_put_prio (&os_rdy, p_TCB);
  }
  else {
    /* Store token */
    if(p_CB->tokens < p_CB->maxtokens)
    {
        p_CB->tokens++;
    }
  }
}



// - from rt_task: rt_Task.c ------------------------------------------------------------------------------------------

#include "rt_MemBox.h"
#include "rt_iit_memory.h"

extern void rt_init_context (P_TCB p_TCB, U8 priority, FUNCP task_body);

extern BIT os_lock;             //IIT-EXT

#warning --> durante lo scheduling suspend la osal potrebbe fare un context swith se il task manda un evt (non bloccante) ad altro task + prioritario

void iitchanged_rt_block (TIME_t timeout, U8 block_state, const TIME_t notimeout) {
   /* Block running task and choose next ready task.                         */
   /* "timeout" sets a time-out value or is 0xffff-ffff (=no time-out).           */
   /* "block_state" defines the appropriate task state */
   P_TCB next_TCB;

   if (timeout) {
      if (timeout < notimeout) {
         iitchanged_rt_put_dly (os_tsk.run, timeout);
      }
      // marco accame added:
      // in case a task t_one locks the scheduler and before unlocking it, it nastily calls any blocking function, 
      // then there is a context switch to the next highest priority task etc. this is an erroneous behaviour.
      // the task t_one should never make such blocking calls when it has disabled scheduling.
      // solution 1: enter in error mode
      // solution 2: reenable scheduling.

      if(os_lock == __TRUE)
      {
        os_error(4); // invalid call
      }


      os_tsk.run->state = block_state;
      next_TCB = rt_get_first (&os_rdy);
      rt_switch_req (next_TCB);
   }
} 



extern U16 os_time;     //IIT-EXT
extern struct OS_XTMR os_tmr; //IIT-EXT

static U8 osiit_init_task_started = 0; // IIT-EXT: allows to give tid 1 only to init_task

OS_TID rt_get_TID (void) {
  U32 tid;
//  for (tid = 1; tid <= os_maxtaskrun; tid++) {                               // IIT-EXT: removed
  for (tid = (1+osiit_init_task_started); tid <= os_maxtaskrun; tid++) {       // IIT-EXT: added
    if (os_active_TCB[tid-1] == NULL) {
      return ((OS_TID)tid);
    }
  }
  return (0);
}


        

void rt_iit_sys_start(oosiit_task_properties_t* inittsk, oosiit_task_properties_t* idletsk) 
{ 
    uint32_t i;
  
    os_time = 0;                      //IIT-EXT
    os_tsk.run = NULL;                //IIT-EXT
    os_tsk.new = NULL;                //IIT-EXT
    osiit_init_task_started = 0;      //IIT-EXT
    oosiit_time = 0;                  //IIT-EXT
    oosiit_idletime = 0;              //IIT-EXT
//  rt_iit_params_init();             //IIT-EXT
//  rt_iit_memory_init();             //IIT-EXT


#ifdef OOSIIT_DBG_ENABLE  
    rt_iit_dbg_init();              
#endif
    
#ifdef OOSIIT_DBG_SYSTICK
    rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_SYSTICK);		
#endif
#ifdef OOSIIT_DBG_PENDSV
    rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_PENDSV);		
#endif
#ifdef OOSIIT_DBG_SVC
    rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_SVC);			
#endif


    // Initialize dynamic memory and task TCB pointers to NULL. 
    for(i=0; i<os_maxtaskrun; i++) 
    {
        os_active_TCB[i] = NULL;
    }
    rt_init_box(mp_tcb, mp_tcb_size, sizeof(struct OS_TCB));                 //IIT-EXT
    rt_init_box(mp_stk, mp_stk_size, BOX_ALIGN_8 | (U16)(os_stackinfo));     //IIT-EXT
    rt_init_box((U32 *)m_tmr, mp_tmr_size, sizeof(struct OS_TMR));
    os_tmr.next = NULL;                                                       //IIT-EXT
    os_tmr.tcnt = 0;                                                          //IIT-EXT

    rt_iit_advtmr_init();			//IIT-EXT
  


    // idle task
    os_idle_TCB.task_id         = 255;
    os_idle_TCB.stack           = (uint32_t*)idletsk->stackdata; 
    os_idle_TCB.priv_stack      = idletsk->stacksize;
    os_idle_TCB.msg             = idletsk->param;
    rt_init_context (&os_idle_TCB, 0, (FUNCP)idletsk->function);
 
    
    /* Set up ready list: initially empty */
    os_rdy.cb_type = HCB;
    os_rdy.p_lnk   = NULL;
    /* Set up delay list: initially empty */
    os_dly.cb_type = HCB;
    os_dly.p_dlnk  = NULL;
    os_dly.p_blnk  = NULL;
    os_dly.delta_time = 0;


    /* Fix SP and systemvariables to assume idle task is running  */
    /* Transform main program into idle task by assuming idle TCB */
    rt_set_PSP (os_idle_TCB.tsk_stack+32);
    os_tsk.run = &os_idle_TCB;
    os_tsk.run->state = RUNNING;


    /* Initialize ps queue */
    os_psq->first = 0;
    os_psq->last  = 0;
    os_psq->size  = os_fifo_size;

    /* Intitialize system clock timer */
    rt_tmr_init ();
    // rt_init_robin ();          //IIT-EXT
    iitchanged_rt_init_robin ();  //IIT-EXT

    rt_psh_req();					//IIT-EXT


    /* Start up first user task before entering the endless loop */
    //rt_tsk_create(first_task, prio_stksz, stk, NULL);
    uint32_t stacksize24priority08 = ((((uint32_t)(inittsk->stacksize)) << 8)&0xffffff00) | (inittsk->priority&0xff);
    rt_tsk_create((FUNCP)inittsk->function, stacksize24priority08, (uint32_t*)inittsk->stackdata, inittsk->param);     
}



// void rt_sys_init (FUNCP first_task, U32 prio_stksz, void *stk) {
//   /* Initialize system and start up task declared with "first_task". */
//   U32 i;

//   //DBG_INIT(); //IIT-EXT: substituted with the iit version
// #ifdef OOSIIT_DBG_ENABLE  
//   rt_iit_dbg_init();             //IIT-EXT
// #endif
//   
//   os_time = 0;                      //IIT-EXT
//   os_tsk.run = NULL;                //IIT-EXT
//   os_tsk.new = NULL;                //IIT-EXT
//   osiit_init_task_started = 0;      //IIT-EXT
//   oosiit_time = 0;                  //IIT-EXT
//   oosiit_idletime = 0;              //IIT-EXT
// //  rt_iit_params_init();             //IIT-EXT
// //  rt_iit_memory_init();             //IIT-EXT

// #ifdef OOSIIT_DBG_SYSTICK
//   rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_SYSTICK);		//IIT-EXT
// #endif
// #ifdef OOSIIT_DBG_PENDSV
//   rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_PENDSV);		//IIT-EXT
// #endif
// #ifdef OOSIIT_DBG_SVC
//   rt_iit_dbg_syscall_register(RT_IIT_SYSCALL_ID_SVC);			//IIT-EXT
// #endif


//   /* Initialize dynamic memory and task TCB pointers to NULL. */
//   for (i = 0; i < os_maxtaskrun; i++) {
//     os_active_TCB[i] = NULL;
//   }
//   rt_init_box (mp_tcb, mp_tcb_size, sizeof(struct OS_TCB));                 //IIT-EXT
//   rt_init_box (mp_stk, mp_stk_size, BOX_ALIGN_8 | (U16)(os_stackinfo));     //IIT-EXT
//   rt_init_box ((U32 *)m_tmr, mp_tmr_size, sizeof(struct OS_TMR));
//   os_tmr.next = NULL;                                                       //IIT-EXT
//   os_tmr.tcnt = 0;                                                          //IIT-EXT

//   rt_iit_advtmr_init();			//IIT-EXT
//   

//   /* Set up TCB of idle demon */
//   os_idle_TCB.task_id    = 255;
//   os_idle_TCB.stack = NULL; //IIT-EXT
//   os_idle_TCB.priv_stack = 0;
//   rt_init_context (&os_idle_TCB, 0, os_idle_demon);

//   /* Set up ready list: initially empty */
//   os_rdy.cb_type = HCB;
//   os_rdy.p_lnk   = NULL;
//   /* Set up delay list: initially empty */
//   os_dly.cb_type = HCB;
//   os_dly.p_dlnk  = NULL;
//   os_dly.p_blnk  = NULL;
//   os_dly.delta_time = 0;

//   /* Fix SP and systemvariables to assume idle task is running  */
//   /* Transform main program into idle task by assuming idle TCB */
//   rt_set_PSP (os_idle_TCB.tsk_stack+32);
//   os_tsk.run = &os_idle_TCB;
//   os_tsk.run->state = RUNNING;


//   /* Initialize ps queue */
//   os_psq->first = 0;
//   os_psq->last  = 0;
//   os_psq->size  = os_fifo_size;

//   /* Intitialize system clock timer */
//   rt_tmr_init ();
//   // rt_init_robin ();          //IIT-EXT
//   iitchanged_rt_init_robin ();  //IIT-EXT

//   rt_psh_req();					//IIT-EXT

//   /* Start up first user task before entering the endless loop */
//   rt_tsk_create (first_task, prio_stksz, stk, NULL);
// }

// - from rt_list: rt_List.c ------------------------------------------------------------------------------------------

void iitchanged_rt_put_dly (P_TCB_Opaque p_task, TIME_t delay) {
   /* Put a task identified with "p_task" into chained delay wait list using */
   /* a delay value of "delay".                                              */
   P_TCB p;
   WIDETIME_t delta,idelay = delay;

   p = (P_TCB)&os_dly;
   if (p->p_dlnk == NULL) {
      /* Delay list empty */
      delta = 0;
      goto last;
   }
   delta = os_dly.delta_time;
   while (delta < idelay) {
      if (p->p_dlnk == NULL) {
         /* End of list found */
last:    p_task->p_dlnk = NULL;
         p->p_dlnk = p_task;
         p_task->p_blnk = p;
         p->delta_time = (TIME_t)(idelay - delta);
         p_task->delta_time = 0;
         return;
      }
      p = p->p_dlnk;
      delta += p->delta_time;
   }
   /* Right place found */
   p_task->p_dlnk = p->p_dlnk;
   p->p_dlnk = p_task;
   p_task->p_blnk = p;
   if (p_task->p_dlnk != NULL) {
      p_task->p_dlnk->p_blnk = p_task;
   }
   p_task->delta_time = (TIME_t)(delta - idelay);
   p->delta_time -= p_task->delta_time;
} 


void iitchanged_rt_dec_dly (void) {
   /* Decrement delta time of list head: remove tasks having a value of zero.*/
   P_TCB p_rdy;

   if (os_dly.p_dlnk == NULL) {
      return;
   }
   os_dly.delta_time--;
   while ((os_dly.delta_time == 0) && (os_dly.p_dlnk != NULL)) {
      p_rdy = os_dly.p_dlnk;
      if (p_rdy->p_rlnk != NULL) {
         /* Task is really enqueued, remove task from semaphore/mailbox */
         /* timeout waiting list. */
         p_rdy->p_rlnk->p_lnk = p_rdy->p_lnk;
         if (p_rdy->p_lnk != NULL) {
            p_rdy->p_lnk->p_rlnk = p_rdy->p_rlnk;
            p_rdy->p_lnk = NULL;
         }
         p_rdy->p_rlnk = NULL;
      }
      rt_put_prio (&os_rdy, p_rdy);
      os_dly.delta_time = p_rdy->delta_time;
      if (p_rdy->state == WAIT_ITV) {
         /* Calculate the next time for interval wait. */
         p_rdy->delta_time = p_rdy->interval_time + oosiit_time;
      }
      p_rdy->state   = READY;
      os_dly.p_dlnk = p_rdy->p_dlnk;
      if (p_rdy->p_dlnk != NULL) {
         p_rdy->p_dlnk->p_blnk =  (P_TCB)&os_dly;
         p_rdy->p_dlnk = NULL;
      }
      p_rdy->p_blnk = NULL;
   }
} 

// experimental support for a post-service queue with a third argument: flags REMOVED as it is not necessary now
// explanation: it is used only for 2BACK and 2FRONT in message box
//void rt_iit_psq_enq (OS_ID entry, U32 arg, U32 flags) {
//  /* Insert post service request "entry" into ps-queue. */
//  U32 idx;
//
//  idx = rt_inc_qi (os_psq->size, &os_psq->count, &os_psq->first);
//  if (idx < os_psq->size) {
//    os_psq->q[idx].id  = entry;
//    os_psq->q[idx].arg = arg;
//    os_psq->q[idx].flags = flags;
//  }
//  else {
//    os_error (OS_ERR_FIFO_OVF);
//  }
//}



// - from rt_robin: rt_Robin.c ----------------------------------------------------------------------------------------

void iitchanged_rt_init_robin (void) {
  /* Initialize Round Robin variables. */
  os_robin.task = NULL;
  os_robin.tout = (TIME_t)os_rrobin_tout;
}

void iitchanged_rt_chk_robin (void) {
  /* Check if Round Robin timeout expired and switch to the next ready task.*/
  P_TCB p_new;

  if(0 == os_rrobin_use)
  {
    return;
  }

  if (os_robin.task != os_rdy.p_lnk) {
    /* New task was suspended, reset Round Robin timeout. */
    os_robin.task = os_rdy.p_lnk;
    os_robin.time = oosiit_time + os_robin.tout - 1;
  }
  if (os_robin.time == oosiit_time) {
    /* Round Robin timeout has expired, swap Robin tasks. */
    os_robin.task = NULL;
    p_new = rt_get_first (&os_rdy);
    rt_put_prio ((P_XCB)&os_rdy, p_new);
  }
}


// - extra functions --------------------------------------------------------------------------------------------------

void os_tmr_call(uint16_t info) 
{
  /* This function is called when the user timer has expired. Parameter   */
  /* 'info' holds the value, defined when the timer was created.          */
  info = info;
}

void os_idle_demon (void) {
  /* The idle demon is a system thread, running when no other thread is      */
  /* ready to run.                                                           */

  for (;;) {
  /* HERE: include optional user code to be executed when no thread runs.*/
  }
}



// - management of system calls with itm ------------------------------------------------------------------------------

// added funtions to signal to itm manager the execution of sys calls: systick, pendsv, svc

#ifdef OOSIIT_DBG_ENABLE

extern void oosiit_systick(void)
{
	volatile uint8_t aa = 0;
	aa = aa;
}

extern void oosiit_pendsv(void)
{
	volatile uint8_t aa = 0;
	aa = aa;
}

extern void oosiit_svc(void)
{
	volatile uint8_t aa = 0;
	aa = aa;
}

static uint8_t oosiit_dbg_initted = 0;

extern void rt_iit_dbg_init(void)
{
  if ((DEMCR & DEMCR_TRCENA)     && 
      (ITM_CONTROL & ITM_ITMENA) &&
      (ITM_ENABLE & (1UL << 31))) {
    oosiit_dbg_initted = 1;
  } 
  else {
    oosiit_dbg_initted = 0;
  }   
  oosiit_dbg_initted = oosiit_dbg_initted; 
}




extern void rt_iit_dbg_task_notify(void* ptcb, BOOL create)
{
    P_TCB p_tcb = (P_TCB) ptcb;
    if(0 == oosiit_dbg_initted)
    {
        return;
    }
    
    if(1 == create)
    {
        eventviewer_load(ev_ID_first_ostask+p_tcb->task_id, p_tcb->ptask);
    }
    else
    {
        eventviewer_unload(ev_ID_first_ostask+p_tcb->task_id, p_tcb->ptask);
    }
}

extern void rt_iit_dbg_task_switch(U32 task_id)
{
    U8 id = (255==task_id) ? (ev_ID_idle) : (ev_ID_first_ostask+(U8)task_id);
    
    if((0 == oosiit_dbg_initted) || (os_tsk.new == os_tsk.run))
    {
        return;
    }
    
    eventviewer_switch_to(id);
}

extern void rt_iit_dbg_syscall_register(U8 id) 
{
  FUNCP fptr = NULL;
  uint8_t idev = 0;
  switch(id) {
    case RT_IIT_SYSCALL_ID_SYSTICK:   fptr = oosiit_systick;    idev = ev_ID_systick;   break;
	case RT_IIT_SYSCALL_ID_PENDSV:    fptr = oosiit_pendsv;     idev = ev_ID_pendsv;    break;
	case RT_IIT_SYSCALL_ID_SVC:       fptr = oosiit_svc;        idev = ev_ID_svc;       break;
	default: 				          fptr = NULL;       	    idev = 0;               break;
  };
  eventviewer_load(idev, fptr);
}


//extern void rt_iit_dbg_syscall_enter (U8 id) {
//  while (ITM_PORT31_U32 == 0);
//  ITM_PORT31_U8 = id;
//}
//
extern void rt_iit_dbg_syscall_exit (void)
{
//  if(os_tsk.new==os_tsk.run)
//    dbg_task_switch(os_tsk.run->task_id);
//  else
//  ;
  if(os_tsk.new==os_tsk.run)
  {
    //uint8_t id = (255 == os_tsk.run->task_id) ? (ev_ID_idle) : (ev_ID_first_osaltask+os_tsk.run->task_id); 
    //eventviewer_switch_to(id); 
    eventviewer_switch_to(s_dbg_previous_id);
  }
  else
  {
    //s_dbg_previous_id;
  }
  //eventviewer_switch_to
}

#ifdef OOSIIT_DBG_SYSTICK
extern void rt_iit_dbg_systick_enter(void)
{
	//rt_iit_dbg_syscall_enter(RT_IIT_SYSCALL_ID_SYSTICK);
    s_dbg_previous_id = eventviewer_switch_to(ev_ID_systick);
}
extern void rt_iit_dbg_systick_exit(void)
{
 	rt_iit_dbg_syscall_exit();
    //eventviewer_switch_to(s_dbg_previous_id);
}
#else//OOSIIT_DBG_SYSTICK
extern void rt_iit_dbg_systick_enter(void){;}
extern void rt_iit_dbg_systick_exit(void){;}
#endif//OOSIIT_DBG_SYSTICK


#ifdef OOSIIT_DBG_PENDSV
extern void rt_iit_dbg_pendsv_enter(void)
{
	//rt_iit_dbg_syscall_enter(RT_IIT_SYSCALL_ID_PENDSV);
    s_dbg_previous_id = eventviewer_switch_to(ev_ID_pendsv);
}
extern void rt_iit_dbg_pendsv_exit(void)
{
 	rt_iit_dbg_syscall_exit();
    //eventviewer_switch_to(s_dbg_previous_id);
}
#else//OOSIIT_DBG_PENDSV
extern void rt_iit_dbg_pendsv_enter(void){;}
extern void rt_iit_dbg_pendsv_exit(void){;}
#endif//OOSIIT_DBG_PENDSV


#ifdef OOSIIT_DBG_SVC
extern void rt_iit_dbg_svc_enter(void)
{
	//rt_iit_dbg_syscall_enter(RT_IIT_SYSCALL_ID_SVC);
    s_dbg_previous_id = eventviewer_switch_to(ev_ID_svc);
}
extern void rt_iit_dbg_svc_exit(void)
{
 	rt_iit_dbg_syscall_exit();
    //eventviewer_switch_to(s_dbg_previous_id);
}
#else
extern void rt_iit_dbg_svc_enter(void){;}
extern void rt_iit_dbg_svc_exit(void){;}
#endif//OOSIIT_DBG_SVC


#else//OOSIIT_DBG_ENABLE is not defined

extern void rt_iit_dbg_init(void){;}

extern void rt_iit_dbg_task_notify(void* ptcb, BOOL create){;}
extern void rt_iit_dbg_task_switch(U32 task_id){;}
    
extern void rt_iit_dbg_syscall_register (U8 id){;}
//extern void rt_iit_dbg_syscall_enter (U8 id){;}
extern void rt_iit_dbg_syscall_exit (void){;}

extern void rt_iit_dbg_systick_enter(void){;}
extern void rt_iit_dbg_systick_exit(void){;}
extern void rt_iit_dbg_pendsv_enter(void){;}
extern void rt_iit_dbg_pendsv_exit(void){;}
extern void rt_iit_dbg_svc_enter(void){;}
extern void rt_iit_dbg_svc_exit(void){;}

#endif





// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------




// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------


