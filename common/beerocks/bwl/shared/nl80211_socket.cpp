/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Copyright (c) 2020 MaxLinear
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "nl80211_socket.h"

#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>

#include <easylogging++.h>

#define NETLINK_BUFFER_SIZE (8192)

#ifndef NL_AUTO_PORT
#define NL_AUTO_PORT 0
#endif

namespace bwl {

nl80211_socket::nl80211_socket() : nl_genl_socket()
{
    if (m_nl_socket) {
        // Increase the socket's internal buffer size
        nl_socket_set_buffer_size(m_nl_socket.get(), NETLINK_BUFFER_SIZE, NETLINK_BUFFER_SIZE);
    }
}

nl80211_socket::~nl80211_socket() {}

bool nl80211_socket::connect()
{
    // Connect the socket
    bool result = netlink_socket::connect();

    // Resolve the generic nl80211 family id
    if (result) {
        const char *family_name = "nl80211";

        m_family_id = genl_ctrl_resolve(m_nl_socket.get(), family_name);
        if (0 > m_family_id) {
            LOG(ERROR) << "'" << family_name << "' family not found!";
            result = false;

            close();
        }
    }

    return result;
}

bool nl80211_socket::send_receive_msg(int command, int flags,
                                      std::function<bool(struct nl_msg *msg)> msg_create,
                                      std::function<bool(struct nl_msg *msg)> msg_handle)
{
    return netlink_socket::send_receive_msg(
        [&](struct nl_msg *msg) -> bool {
            // Initialize the netlink message
            if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, m_family_id, 0, flags, command, 0)) {
                LOG(ERROR) << "Failed initializing the netlink message!";
                return false;
            }

            // Call the user's message create function
            return msg_create(msg);
        },
        msg_handle);
}

} // namespace bwl
