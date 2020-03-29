// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2020, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *


//******************************************************************************
// system includes
//******************************************************************************

#include <assert.h>



//******************************************************************************
// macros
//******************************************************************************

#define UNIT_TEST 0

#define DEBUG 1

#include "gpu-print.h"



//******************************************************************************
// local includes
//******************************************************************************

#include "gpu-activity.h"
#include "gpu-channel-item-allocator.h"



//******************************************************************************
// interface functions
//******************************************************************************

void
gpu_context_activity_dump
(
 gpu_activity_t *activity,
 const char *context
)
{
  PRINT("context %s gpu activity %p kind = %d\n", context, activity, activity->kind);
}


void
gpu_activity_dump
(
 gpu_activity_t *activity
)
{
  gpu_context_activity_dump(activity, "DEBUGGER");
}


void
gpu_activity_consume
(
 gpu_activity_t *activity,
 gpu_activity_attribute_fn_t aa_fn
)
{
  //gpu_context_activity_dump(activity, "CONSUME");
  aa_fn(activity);
}


gpu_activity_t *
gpu_activity_alloc
(
 gpu_activity_channel_t *channel
)
{
  return channel_item_alloc(channel, gpu_activity_t);
}


void
gpu_activity_free
(
 gpu_activity_channel_t *channel, 
 gpu_activity_t *a
)
{
  channel_item_free(channel, a);
}

void
set_gpu_instruction
(
  gpu_instruction_t* insn, 
  ip_normalized_t pc
)
{
  insn->pc = pc;
}

void
set_gpu_interval
(
  gpu_interval_t* interval,
  uint64_t start,
  uint64_t end
)
{
  interval->start = start;
  interval->end = end;
}
