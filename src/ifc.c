/**
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* network device interfaces
*
* $Id: ifc.c,v 1.7 2009-11-16 07:19:38 wxlin Exp $
*
* Author: wxlin
*
*/

#include "global.h"
#include "log.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>

struct iface_struct {
    char name[16];
    struct in_addr ip;
    struct in_addr netmask;
};

struct iface_conf {
	int dht_iface;
	uint32 dht_network;
	uint32 dht_subnet;
};

#define MAX_INTERFACES 128
struct iface_struct _ifaces[MAX_INTERFACES];
unsigned int _total_ifaces = 0;
int _dht_iface = -1;
uint32 _dht_network = 0;
uint32 _dht_subnet = 0;

/* 
* get the netmask address for a local interface
*/
static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
    struct ifconf ifc;
    char buff[8192];
    int fd, i, n;
    struct ifreq *ifr=NULL;
    int total = 0;
    struct in_addr ipaddr;
    struct in_addr nmask;
    char *iname;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return -1;
    }

    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;

    if (ioctl(fd, SIOCGIFCONF, &ifc) != 0) {
        close(fd);
        return -1;
    }

    ifr = ifc.ifc_req;

    n = ifc.ifc_len / sizeof(struct ifreq);

    /* Loop through interfaces, looking for given IP address */
    for (i=n-1;i>=0 && total < max_interfaces;i--) {
        if (ioctl(fd, SIOCGIFADDR, &ifr[i]) != 0) {
            continue;
        }

        iname = ifr[i].ifr_name;
        ipaddr = (*(struct sockaddr_in *)&ifr[i].ifr_addr).sin_addr;

        if (ioctl(fd, SIOCGIFFLAGS, &ifr[i]) != 0) {
            continue;
        }

        if (!(ifr[i].ifr_flags & IFF_UP)) {
            continue;
        }

        if (ioctl(fd, SIOCGIFNETMASK, &ifr[i]) != 0) {
            continue;
        }

        nmask = ((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr;

        strncpy(ifaces[total].name, iname, sizeof(ifaces[total].name)-1);
        ifaces[total].name[sizeof(ifaces[total].name)-1] = 0;
        ifaces[total].ip = ipaddr;
        ifaces[total].netmask = nmask;
        total++;
    }

    close(fd);
    return total;
}

static int iface_comp(struct iface_struct *i1, struct iface_struct *i2)
{
    int r;
    r = strcmp(i1->name, i2->name);
    if (r) return r;
    r = ntohl(i1->ip.s_addr) - ntohl(i2->ip.s_addr);
    if (r) return r;
    r = ntohl(i1->netmask.s_addr) - ntohl(i2->netmask.s_addr);
    return r;
}

int get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
    int total, i, j;

    total = _get_interfaces(ifaces, max_interfaces);
    if (total <= 0) return total;

    /* now we need to remove duplicates */
    qsort(ifaces, total, sizeof(ifaces[0]), (__compar_fn_t)iface_comp);
    for (i=1;i<total;) {
        if (iface_comp(&ifaces[i-1], &ifaces[i]) == 0) {
            for (j=i-1;j<total-1;j++) {
                ifaces[j] = ifaces[j+1];
            }
            total--;
        } else {
            i++;
        }
    }

    return total;
}

/*
* Checks whether the given char* is an ip address
*/
int isipaddress(char *address)
{
    int a, i;

    if (address == NULL) {
        return -1;
    } else {
        if (index(address, ':') == NULL) {
            // IPv4?
            if (strlen(address) < 7 || strlen(address) > 15) {
                return -1;
            } else {
                for (i = 0; i < 4; i++) {
                    a = atoi(address);
                    // if atoi fails a becomes 0
                    if (a == 0 && *address != '0') {
                        return -1;
                    }
                    if (a < 0 || a > 255) {
                        return -1;
                    }
                    address = index(address, '.')+1;
                    // if what?
                }
                return 0;
            }
        } else {
            // IPv6?
            for (i=0; i<strlen(address); i++) {
                if (!(isalnum(address[i]) || address[i] == ':')) {
                    return -1;
                }
            }
            return 0;
        }
    }
}

unsigned int getsubnet(unsigned int netmask)
{
    int n,v;
    unsigned int subnet = ntohl(netmask);
    for(n=0;n<sizeof(unsigned int);n++){
        v = (0xff << n*8);
        if((subnet & v) == v)
            subnet &= ~v;
    }
    if(subnet){
        for(n=0;(subnet & (0x01<<n))==0;n++);
        subnet >>= n;
    }
    return subnet;
}

unsigned int getmaskbits(unsigned int netmask)
{
    int n = 0;
    unsigned int subnet = ntohl(netmask);
    if(subnet){
        for(;(subnet & (0x01<<n))==0;n++);
    }
    return 32-n;
}

int _set_interface()
{
    int i;
    unsigned int ip = 0;
    unsigned int network;
    unsigned int subnet;

    if(_total_ifaces == 0){
        _total_ifaces = get_interfaces(_ifaces, MAX_INTERFACES);
        //LOG("Daemon: got %d network interfaces.\n", _total_ifaces);
    }

    for (i=0;i<_total_ifaces;i++) {
        ip = ntohl(_ifaces[i].ip.s_addr);
        if (ip == LOOPBACK){
            ip = 0;
            continue;
        }
        //subnet = getsubnet(_ifaces[i].netmask.s_addr);
        subnet = getmaskbits(_ifaces[i].netmask.s_addr);
        network = _ifaces[i].ip.s_addr & _ifaces[i].netmask.s_addr;        
        if(network == _dht_network && subnet == _dht_subnet){
            _dht_iface = i;
            break;
        }
    }

    struct in_addr addr;
    addr.s_addr = _dht_network;
    if(ip == 0){
        WARN("Daemon: not found (vault.conf) %s/%d interface.\n", 
            inet_ntoa(addr),_dht_subnet);
    }else{
        LOG("Daemon: set dht network to %s/%d interface.\n", 
            inet_ntoa(addr),_dht_subnet);
    }

    return ip ? 0 : -1;
}

int set_interface(const char* netnumber)
{
    char* p = NULL;
    char subnetwork[30];
    unsigned int network = 0;
    unsigned int subnet = 0;    

    strcpy(subnetwork,netnumber);
    p = strchr(subnetwork, '\\');
    if (p == NULL) {
        p = strchr(subnetwork, '/');
        if (p == NULL)
            return -1;
    }

    if(p != NULL){
        subnet = atoi(p+1);
        *p = '\0';
    }

    network = inet_addr(subnetwork);
    if(network == INADDR_NONE)
        return -1;

    _dht_network = network;
    _dht_subnet = subnet;
    return _set_interface();
}

char* ip_htoa(uint32 ip, char* strbuf, size_t bufsize)
{
    snprintf(strbuf,
        bufsize,
        "%u.%u.%u.%u",
        *(((byte *) &ip) + 3),
        *(((byte *) &ip) + 2),
        *(((byte *) &ip) + 1),
        *((byte *) &ip));
    return strbuf;
}

uint32 getlocaluip()
{
    unsigned int ip = 0;
    if(_dht_iface != -1)
        ip = ntohl(_ifaces[_dht_iface].ip.s_addr);
    return ip;
}
