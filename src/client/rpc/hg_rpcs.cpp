/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <hermes.hpp>
#include <client/rpc/hg_rpcs.hpp>

namespace hermes { namespace detail {

//==============================================================================
// register request types so that they can be used by users and the engine
//
void
register_user_request_types() {
    (void) registered_requests().add<gkfs::rpc::fs_config>();
    (void) registered_requests().add<gkfs::rpc::create>();
}

}} // namespace hermes::detail
