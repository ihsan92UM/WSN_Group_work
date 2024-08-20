#include <stdio.h>
#include <time.h>
#include "ztimer.h"
#include "phydat.h"
#include "saul_reg.h"
#include "board.h"
#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6/hdr.h"
#include "net/utils.h"
#include "net/netif.h"
#include "od.h"
#include "shell.h"
#include "thread.h"

#define SENSOR_FREQUENCY 10


/* Shell command handler for sending IPv6 packets */
static int send_packet(int argc, char **argv) {
    if (argc != 3) {
        puts("usage: send <IPv6 address> <message>");
        puts("Note: to send multiple words wrap the message in \"\"");
        return 1;
    }

    /* Parse IPv6 address */
    netif_t *netif;
    ipv6_addr_t addr;
    if (netutils_get_ipv6(&addr, &netif, argv[1]) < 0) {
        puts("Unable to parse IPv6 address\n");
        return 1;
    }

    /* Prepare payload */
    gnrc_pktsnip_t *payload = NULL, *ip = NULL;
    ipv6_hdr_t *ip_hdr;
    size_t payload_size = strlen(argv[2]);

    /* Start packet with payload */
    payload = gnrc_pktbuf_add(NULL, argv[2], payload_size, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Unable to copy message to packet buffer");
        return 1;
    }

    /* Add IPv6 header with payload as next header */
    ip = gnrc_ipv6_hdr_build(payload, NULL, &addr);
    if (ip == NULL) {
        puts("Unable to allocate IPv6 header");
        gnrc_pktbuf_release(payload);
        return 1;
    }

    /* Set IPv6 next header to experimental protocol number 253 */
    ip_hdr = ip->data;
    ip_hdr->nh = 253;

    if (netif != NULL) {
        
        gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        if (netif_hdr == NULL) {
            puts("Unable to allocate netif header");
            gnrc_pktbuf_release(ip);
            return 1;
        }
        /* Set GNRC specific network interface values */
        gnrc_netif_hdr_set_netif(netif_hdr->data,
                                 container_of(netif, gnrc_netif_t, netif));
        /* Prepend link layer header to IP packet */
        ip = gnrc_pkt_prepend(ip, netif_hdr);
    }

    /* Send the packet */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_IPV6,
                                   GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        printf("Unable to send IPv6 packet\n");
        gnrc_pktbuf_release(ip);
        return 1;
    }

    printf("IPv6 packet sent successfully\n");

    return 0;
}

int main(void)
{
    puts("Discovering Accelerometer and Gyroscope sensor!");

    saul_reg_t *accel_sensor = saul_reg_find_type(SAUL_SENSE_ACCEL);
    saul_reg_t *gyro_sensor = saul_reg_find_type(SAUL_SENSE_GYRO);
    
    if (!accel_sensor) {
        puts("Accelerometer sensor was not found");
    }
    if (!gyro_sensor) {
        puts("Gyroscope sensor was not found");
    }
    if (!accel_sensor || !gyro_sensor) {
        return 1;
    }

    /* record the starting time */
    ztimer_now_t last_wakeup = ztimer_now(ZTIMER_MSEC);

    int packet_number = 0;

    while (1) {
        /* read gyroscope values from the sensor */
        phydat_t gyro;
        int gyro_dimensions = saul_reg_read(gyro_sensor, &gyro);
        if (gyro_dimensions < 1) {
            puts("Error reading a value from the device");
            break;
        }

        /* read accelerometer values from the sensor */
        phydat_t acceleration;
        int acc_dimensions = saul_reg_read(accel_sensor, &acceleration);
        if (acc_dimensions < 1) {
            puts("Error reading a value from the device");
            break;
        }



        char formatted_data[128];  
        snprintf(formatted_data, sizeof(formatted_data), "%d,%d,%d,%d,%d,%d,%d",
                 packet_number,
                 acceleration.val[0], acceleration.val[1], acceleration.val[2],
                 gyro.val[0], gyro.val[1], gyro.val[2]);
        
        packet_number = packet_number + 1;

        printf("%s\n", formatted_data);

        /* send the formatted data using send_packet */
        if (send_packet(3, (char *[]){ "send", "fe80::dcc5:ea41:9918:4fcf", formatted_data }) != 0) {
            puts("Error sending packet");
            break;
        }

        /* wait for 1000 ms */
        ztimer_periodic_wakeup(ZTIMER_MSEC, &last_wakeup, 1000 / SENSOR_FREQUENCY);
    }

    return 0;
}

