/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Copyright (c) 2020 MaxLinear
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ieee802_3_link_metrics_collector.h"

#include <bcl/network/network_utils.h>

#include <easylogging++.h>

#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// SPEED values
#include <linux/ethtool.h>

#define IFLIST_REPLY_BUFFER 8192

namespace beerocks {

/**
 * @brief Netlink request type.
 */
struct nl_req_t {
    /**
     * Netlink message
     */
    struct nlmsghdr hdr;

    /**
     * "general form of address family dependent" message, i.e. how to tell which AF we are
     * interested in. */
    struct rtgenmsg gen;
};

static bool get_link_metrics(const struct nlmsghdr *h, const std::string &local_interface_name,
                             sLinkMetrics &link_metrics)
{
    bool result = false;

    struct ifinfomsg *iface = static_cast<ifinfomsg *>(NLMSG_DATA(h));
    int length              = h->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

    /**
     * Loop over all attributes of the NEWLINK message
     */
    for (struct rtattr *attribute = IFLA_RTA(iface); RTA_OK(attribute, length);
         attribute                = RTA_NEXT(attribute, length)) {
        switch (attribute->rta_type) {
        case IFLA_IFNAME:
            /**
             * This message contains the stats for the interface we are interested in
             */
            if (0 == strcmp(local_interface_name.c_str(), (char *)RTA_DATA(attribute))) {
                result = true;
            }
            break;
        case IFLA_STATS:
            if (result) {
                struct rtnl_link_stats *stats = (struct rtnl_link_stats *)RTA_DATA(attribute);

                /**
                 * Get interface speed and set PHY rate accordingly.
                 * Speed values other than 100Mbps and 1Gbps are ignored.
                 */
                uint16_t phy_rate = UINT16_MAX;
                uint32_t speed    = 0;
                if (beerocks::net::network_utils::linux_iface_get_speed(local_interface_name,
                                                                        speed)) {
                    if (SPEED_100 == speed) {
                        phy_rate = 100;
                    } else if (SPEED_1000 == speed) {
                        phy_rate = 1000;
                    }
                }

                link_metrics.transmitter.packet_errors       = stats->tx_errors;
                link_metrics.transmitter.transmitted_packets = stats->tx_packets;
                /**
                 * Note: The MAC throughput capacity is a function of the physical data rate and
                 * of the MAC overhead. We could somehow compute such overhead or, for simplicity,
                 * set the MAC throughput as a percentage of the physical data rate. To make
                 * things even simpler, we will set the MAC throughput capacity with the same
                 * value as the physical data rate, as if there was no overhead at all.
                 */
                link_metrics.transmitter.mac_throughput_capacity = phy_rate;
                // Note: For simplicity, link availability is set to "100% of the time"
                link_metrics.transmitter.link_availability = 100;
                link_metrics.transmitter.phy_rate          = phy_rate;

                link_metrics.receiver.packet_errors    = stats->rx_errors;
                link_metrics.receiver.packets_received = stats->rx_packets;
                link_metrics.receiver.rssi             = UINT8_MAX;
            }
            break;
        }
    }

    return result;
}

static bool get_link_metrics(int fd, const std::string &local_interface_name,
                             sLinkMetrics &link_metrics)
{
    bool result = false;

    struct sockaddr_nl kernel; /* the remote (kernel space) side of the communication */

    struct msghdr rtnl_msg; /* generic msghdr struct for use with sendmsg */
    struct iovec io;        /* IO vector for sendmsg */
    struct nl_req_t req;    /* structure that describes the Netlink packet itself */

    /**
     * Netlink socket is ready for use, prepare and send request
     */
    memset(&rtnl_msg, 0, sizeof(rtnl_msg));
    memset(&kernel, 0, sizeof(kernel));
    memset(&req, 0, sizeof(req));

    kernel.nl_family = AF_NETLINK; /* fill-in kernel address (destination of our message) */

    req.hdr.nlmsg_len    = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    req.hdr.nlmsg_type   = RTM_GETLINK;
    req.hdr.nlmsg_flags  = NLM_F_REQUEST | NLM_F_DUMP;
    req.hdr.nlmsg_seq    = 1;
    req.hdr.nlmsg_pid    = 0;
    req.gen.rtgen_family = AF_PACKET; /*  no preferred AF, we will get *all* interfaces */

    io.iov_base          = &req;
    io.iov_len           = req.hdr.nlmsg_len;
    rtnl_msg.msg_iov     = &io;
    rtnl_msg.msg_iovlen  = 1;
    rtnl_msg.msg_name    = &kernel;
    rtnl_msg.msg_namelen = sizeof(kernel);

    if (sendmsg(fd, (struct msghdr *)&rtnl_msg, 0) < 0) {
        LOG(ERROR) << "Unable to send message through Netlink socket: " << strerror(errno);
    } else {
        int msg_done = 0; /* flag to end loop parsing */

        /**
         * Parse reply until message is done
         */
        while (!msg_done) {
            int length;
            struct nlmsghdr *msg_ptr; /* pointer to current message part */

            struct msghdr rtnl_reply; /* generic msghdr structure for use with recvmsg */
            struct iovec io_reply;

            /* a large buffer to receive lots of link information */
            char reply[IFLIST_REPLY_BUFFER];

            memset(&io_reply, 0, sizeof(io_reply));
            memset(&rtnl_reply, 0, sizeof(rtnl_reply));

            io.iov_base            = reply;
            io.iov_len             = IFLIST_REPLY_BUFFER;
            rtnl_reply.msg_iov     = &io;
            rtnl_reply.msg_iovlen  = 1;
            rtnl_reply.msg_name    = &kernel;
            rtnl_reply.msg_namelen = sizeof(kernel);

            /**
             * Read as much data as fits in the receive buffer
             */
            if ((length = recvmsg(fd, &rtnl_reply, 0)) != 0) {
                for (msg_ptr = (struct nlmsghdr *)reply; NLMSG_OK(msg_ptr, length);
                     msg_ptr = NLMSG_NEXT(msg_ptr, length)) {
                    switch (msg_ptr->nlmsg_type) {
                    case NLMSG_DONE:
                        /**
                         * This is the special meaning NLMSG_DONE message we asked for by using NLM_F_DUMP flag
                         */
                        msg_done = 1;
                        break;
                    case RTM_NEWLINK:
                        /**
                         * This is a RTM_NEWLINK message, which contains lots of information about a link
                         */
                        if (get_link_metrics(msg_ptr, local_interface_name, link_metrics)) {
                            msg_done = 1;
                            result   = true;
                        }
                        break;
                    }
                }
            }
        }
    }

    return result;
}

static bool get_link_metrics(const std::string &local_interface_name, sLinkMetrics &link_metrics)
{
    bool result = false;

    /**
     * Create Netlink socket for kernel/user-space communication.
     * No need to call bind() as packets are sent only between the kernel and the originating
     * process (no multicasting).
     */
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        LOG(ERROR) << "Failed creating Netlink socket: " << strerror(errno);
    } else {
        /**
         * Get link metrics using Netlink socket
         */
        result = get_link_metrics(fd, local_interface_name, link_metrics);

        /**
         * Clean up and finish properly
         */
        close(fd);
    }

    return result;
}

ieee802_3_link_metrics_collector::~ieee802_3_link_metrics_collector() {}

bool ieee802_3_link_metrics_collector::get_link_metrics(const std::string &local_interface_name,
                                                        const sMacAddr &neighbor_interface_address,
                                                        sLinkMetrics &link_metrics)
{
    (void)neighbor_interface_address;

    return beerocks::get_link_metrics(local_interface_name, link_metrics);
}

} // namespace beerocks
