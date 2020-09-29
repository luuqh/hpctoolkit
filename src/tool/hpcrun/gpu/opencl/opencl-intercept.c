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

#include <inttypes.h>

//******************************************************************************
// local includes
//******************************************************************************

#include <hpcrun/gpu/instrumentation/gtpin-instrumentation.h>
#include <hpcrun/gpu/gpu-metrics.h>
#include <hpcrun/memory/hpcrun-malloc.h>
#include <hpcrun/messages/messages.h>
#include <hpcrun/files.h>
#include <hpcrun/sample-sources/libdl.h>
#include <lib/prof-lean/hpcrun-gotcha.h>
#include <lib/prof-lean/hpcrun-opencl.h>
#include <lib/prof-lean/stdatomic.h>
#include <lib/prof-lean/spinlock.h>
#include <lib/prof-lean/crypto-hash.h>

#include "opencl-intercept.h"


// TODO: This file is no longer needed. To be deleted

void
opencl_intercept_setup
(
 void
)
{
#ifndef HPCRUN_STATIC_LINK
  ETMSG(OPENCL, "setting up opencl intercepts");
  gpu_metrics_GPU_INST_enable();
  gtpin_enable_profiling();
#endif
}


void
opencl_intercept_teardown
(
 void
)
{
}