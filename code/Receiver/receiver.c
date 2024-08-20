/*
 * Copyright (C) 2015 Inria
 * Copyright (C) 2023 TU Dresden
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Oliver Hahm <oliver.hahm@inria.fr>
 * @author  Martine Lenders <martine.lenders@tu-dresden.de>
 */

#include <stdio.h>

#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6/hdr.h"
#include "net/gnrc/netif.h"
#include "net/utils.h"
#include "net/netif.h"
#include "od.h"
#include "shell.h"
#include "thread.h"


#define MSG_QUEUE_SIZE  8

extern void gnrc_netif_hdr_print(gnrc_netif_hdr_t *hdr);


int main(void)
{
   /* [TASK 1: include message queue initialization here] */
    msg_t msg_queue[MSG_QUEUE_SIZE];
    msg_init_queue(msg_queue, MSG_QUEUE_SIZE);

    puts("Starting the Server\n");

   /* gets the ipv6 address of the base server */
    gnrc_netif_t *netif = NULL;
    while ((netif = gnrc_netif_iter(netif))) {
        printf("Iface %d:\n", (int)netif->pid);

        ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        int res = gnrc_netapi_get(netif->pid, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                                  sizeof(ipv6_addrs));

        if (res < 0) {
            puts(" {}");
            continue;
        }
        for (unsigned i = 0; i < (unsigned)(res / sizeof(ipv6_addr_t)); i++) {
            char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];

            ipv6_addr_to_str(ipv6_addr, &ipv6_addrs[i], IPV6_ADDR_MAX_STR_LEN);
            printf("- %s\n", ipv6_addr);
        }
    }



    /* [TASK 1: register for IPv6 protocol number 253 here] */
    gnrc_netreg_entry_t server = GNRC_NETREG_ENTRY_INIT_PID(253, thread_getpid());
    gnrc_netreg_register(GNRC_NETTYPE_IPV6, &server);
    
    
    /* [TASK 1: receive packet here] */
            while (1) {
            msg_t msg;
        
            msg_receive(&msg);
            if (msg.type == GNRC_NETAPI_MSG_TYPE_RCV) {
                gnrc_pktsnip_t *pkt = msg.content.ptr;
                
            if (pkt->next) {
                if (pkt->next->next) {
                    puts("=== Link layer header ===");
                    gnrc_netif_hdr_print(pkt->next->next->data);
                    
                }
                
            }
        
                printf("Packet Size : %d, Data : %s \n", pkt->size, (char *)pkt->data);
                gnrc_pktbuf_release(pkt);
            }
        }

    /* [TASK 1: unregister from IPv6 protocol number 253 here] */
    gnrc_netreg_unregister(GNRC_NETTYPE_IPV6, &server);
    
    return 0;
}


