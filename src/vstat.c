/**
* MoreStor SuperVault
* Copyright (c), 2008, Sierra Atlantic, Dream Team.
*
* vdaemon stat
*
* Author(s): wxlin  <weixuan.lin@sierraatlantic.com>
*
* $Id:vstat.h,v 1.4 2008-12-30 08:14:55 wxlin Exp $
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/utsname.h>
#include <linux/sockios.h>

#define HZ 1000
#define INT_MAX 2147483647 
#define PRG_LOCAL_ADDRESS "local_address"
#define PRG_INODE		  "inode"
#define PRG_SOCKET_PFX    "socket:["
#define PRG_SOCKET_PFXl   (strlen(PRG_SOCKET_PFX))
#define PRG_SOCKET_PFX2   "[0000]:"
#define PRG_SOCKET_PFX2l  (strlen(PRG_SOCKET_PFX2))
#define MALLOC(TYPE)	  calloc(1,sizeof(TYPE))

/* CMDs currently supported */
#define ETHTOOL_GSET		0x00000001 /* Get settings. */
#define ETHTOOL_SSET		0x00000002 /* Set settings. */
#define ETHTOOL_GDRVINFO	0x00000003 /* Get driver info. */
#define ETHTOOL_GREGS		0x00000004 /* Get NIC registers. */
#define ETHTOOL_GWOL		0x00000005 /* Get wake-on-lan options. */
#define ETHTOOL_SWOL		0x00000006 /* Set wake-on-lan options. */
#define ETHTOOL_GMSGLVL		0x00000007 /* Get driver message level */
#define ETHTOOL_SMSGLVL		0x00000008 /* Set driver msg level. */
#define ETHTOOL_NWAY_RST	0x00000009 /* Restart autonegotiation. */
#define ETHTOOL_GLINK		0x0000000a /* Get link status (ethtool_value) */
#define ETHTOOL_GEEPROM		0x0000000b /* Get EEPROM data */
#define ETHTOOL_SEEPROM		0x0000000c /* Set EEPROM data. */
#define ETHTOOL_GCOALESCE	0x0000000e /* Get coalesce config */
#define ETHTOOL_SCOALESCE	0x0000000f /* Set coalesce config. */
#define ETHTOOL_GRINGPARAM	0x00000010 /* Get ring parameters */
#define ETHTOOL_SRINGPARAM	0x00000011 /* Set ring parameters. */
#define ETHTOOL_GPAUSEPARAM	0x00000012 /* Get pause parameters */
#define ETHTOOL_SPAUSEPARAM	0x00000013 /* Set pause parameters. */
#define ETHTOOL_GRXCSUM		0x00000014 /* Get RX hw csum enable (ethtool_value) */
#define ETHTOOL_SRXCSUM		0x00000015 /* Set RX hw csum enable (ethtool_value) */
#define ETHTOOL_GTXCSUM		0x00000016 /* Get TX hw csum enable (ethtool_value) */
#define ETHTOOL_STXCSUM		0x00000017 /* Set TX hw csum enable (ethtool_value) */
#define ETHTOOL_GSG			0x00000018 /* Get scatter-gather enable
* (ethtool_value) */
#define ETHTOOL_SSG			0x00000019 /* Set scatter-gather enable
* (ethtool_value). */
#define ETHTOOL_TEST		0x0000001a /* execute NIC self-test. */
#define ETHTOOL_GSTRINGS	0x0000001b /* get specified string set */
#define ETHTOOL_PHYS_ID		0x0000001c /* identify the NIC */
#define ETHTOOL_GSTATS		0x0000001d /* get NIC-specific statistics */
#define ETHTOOL_GTSO		0x0000001e /* Get TSO enable (ethtool_value) */
#define ETHTOOL_STSO		0x0000001f /* Set TSO enable (ethtool_value) */
#define ETHTOOL_GPERMADDR	0x00000020 /* Get permanent hardware address */
#define ETHTOOL_GUFO		0x00000021 /* Get UFO enable (ethtool_value) */
#define ETHTOOL_SUFO		0x00000022 /* Set UFO enable (ethtool_value) */
#define ETHTOOL_GGSO		0x00000023 /* Get GSO enable (ethtool_value) */
#define ETHTOOL_SGSO		0x00000024 /* Set GSO enable (ethtool_value) */

/* compatibility with older code */
#define SPARC_ETH_GSET		ETHTOOL_GSET
#define SPARC_ETH_SSET		ETHTOOL_SSET

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half		(1 << 0)
#define SUPPORTED_10baseT_Full		(1 << 1)
#define SUPPORTED_100baseT_Half		(1 << 2)
#define SUPPORTED_100baseT_Full		(1 << 3)
#define SUPPORTED_1000baseT_Half	(1 << 4)
#define SUPPORTED_1000baseT_Full	(1 << 5)
#define SUPPORTED_Autoneg			(1 << 6)
#define SUPPORTED_TP				(1 << 7)
#define SUPPORTED_AUI				(1 << 8)
#define SUPPORTED_MII				(1 << 9)
#define SUPPORTED_FIBRE				(1 << 10)
#define SUPPORTED_BNC				(1 << 11)
#define SUPPORTED_10000baseT_Full	(1 << 12)
#define SUPPORTED_Pause				(1 << 13)
#define SUPPORTED_Asym_Pause		(1 << 14)
#define SUPPORTED_2500baseX_Full	(1 << 15)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half		(1 << 0)
#define ADVERTISED_10baseT_Full		(1 << 1)
#define ADVERTISED_100baseT_Half	(1 << 2)
#define ADVERTISED_100baseT_Full	(1 << 3)
#define ADVERTISED_1000baseT_Half	(1 << 4)
#define ADVERTISED_1000baseT_Full	(1 << 5)
#define ADVERTISED_Autoneg			(1 << 6)
#define ADVERTISED_TP				(1 << 7)
#define ADVERTISED_AUI				(1 << 8)
#define ADVERTISED_MII				(1 << 9)
#define ADVERTISED_FIBRE			(1 << 10)
#define ADVERTISED_BNC				(1 << 11)
#define ADVERTISED_10000baseT_Full	(1 << 12)
#define ADVERTISED_Pause			(1 << 13)
#define ADVERTISED_Asym_Pause		(1 << 14)
#define ADVERTISED_2500baseX_Full	(1 << 15)

/* The following are all involved in forcing a particular link
* mode for the device for setting things.  When getting the
* devices settings, these indicate the current mode and whether
* it was foced up into this mode or autonegotiated.
*/

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000
#define SPEED_2500		2500
#define SPEED_10000		10000

/* Duplex, half or full. */
#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01

/* Which connector port. */
#define PORT_TP			0x00
#define PORT_AUI		0x01
#define PORT_MII		0x02
#define PORT_FIBRE		0x03
#define PORT_BNC		0x04

/* Which transceiver to use. */
#define XCVR_INTERNAL		0x00
#define XCVR_EXTERNAL		0x01
#define XCVR_DUMMY1		0x02
#define XCVR_DUMMY2		0x03
#define XCVR_DUMMY3		0x04

/* Enable or disable autonegotiation.  If this is set to enable,
* the forced link modes above are completely ignored.
*/
#define AUTONEG_DISABLE		0x00
#define AUTONEG_ENABLE		0x01

/* Wake-On-Lan options. */
#define WAKE_PHY		(1 << 0)
#define WAKE_UCAST		(1 << 1)
#define WAKE_MCAST		(1 << 2)
#define WAKE_BCAST		(1 << 3)
#define WAKE_ARP		(1 << 4)
#define WAKE_MAGIC		(1 << 5)
#define WAKE_MAGICSECURE	(1 << 6) /* only meaningful if WAKE_MAGIC */

/* hack, so we may include kernel's ethtool.h */
typedef unsigned long long __u64;
typedef __uint32_t __u32;
typedef __uint16_t __u16;
typedef __uint8_t __u8;

/* historical: we used to use kernel-like types; remove these once cleaned */
typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t u8;

struct cpu_info
{
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long hardirq;
    unsigned long long softirq; 
    unsigned long long steal;
};

struct cpu_arch
{
    unsigned int procnum;
    char medel_name[512];
    unsigned int MHz;
    unsigned int freq;
};

struct mem_info 
{
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long buffers;
    unsigned long cached;
    unsigned long swap_cached;
    unsigned long swap_total;
    unsigned long swap_free;
    unsigned long free_mem;     /*no proc*/
    unsigned long used_mem;     /*no proc*/
};

struct pid_stat
{
    unsigned int euid;
    unsigned int egid;
    unsigned int pid;
    char comm[PATH_MAX];
    char exe[PATH_MAX];
    char state;
    pid_t ppid;
    pid_t pgid;
    pid_t sid;
    int tty_nr;
    int tty_pgrp;
    unsigned int flags;
    unsigned long min_flt;
    unsigned long cmin_flt;
    unsigned long maj_flt;
    unsigned long cmaj_flt;
    unsigned long utime;
    unsigned long stimev;
    unsigned long cutime;
    unsigned long cstime;
    long priority;
    long nice;
    int num_threads;
    int zero;
    unsigned long long start_time;
    unsigned long vsize;
    unsigned long rss;
    unsigned long rlim;
    unsigned long start_code;
    unsigned long end_code;
    unsigned long start_stack;
    unsigned long esp;
    unsigned long eip;
    unsigned long pendingsig;
    unsigned long block_sig;
    unsigned long sigign;
    unsigned long sigcatch;
    unsigned long wchan;
    unsigned long nswap;
    unsigned long cnswap;
    int exit_signal;
    unsigned int task_cpu;
    unsigned int task_rt_priority;
    unsigned int task_policy;
    unsigned int rflags;
};

struct pid_io
{
    unsigned long long rchar;
    unsigned long long wchar;
    unsigned long long syscr;
    unsigned long long syscw;
    unsigned long long read_bytes;
    unsigned long long write_bytes;
    unsigned long long cancelled_write_bytes;
};

struct if_stats
{
    char devname[10];
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_errors;
    unsigned long long tx_errors;
    unsigned long long rx_dropped;
    unsigned long long tx_dropped;
    unsigned long long rx_fifo;
    unsigned long long tx_fifo;
    unsigned long long rx_frame;
    unsigned long long tx_colls;
    unsigned long long rx_multicast;
    unsigned long long tx_carrier;
    unsigned long long rx_compressed;
    unsigned long long tx_compressed;
    struct if_stats* next;
};

struct iface_dev 
{
    char name[16];
    struct in_addr ip;
    struct in_addr netmask;
    struct iface_dev* next;
};

/* This should work for both 32 and 64 bit userland. */
struct ethtool_cmd 
{
    __u32	cmd;
    __u32	supported;	/* Features this interface supports */
    __u32	advertising;/* Features this interface advertises */
    __u16	speed;		/* The forced speed, 10Mb, 100Mb, gigabit */
    __u8	duplex;		/* Duplex, half or full */
    __u8	port;		/* Which connector port */
    __u8	phy_address;
    __u8	transceiver;/* Which transceiver to use */
    __u8	autoneg;	/* Enable or disable autonegotiation */
    __u32	maxtxpkt;	/* Tx pkts before generating tx int */
    __u32	maxrxpkt;	/* Rx pkts before generating rx int */
    __u32	reserved[4];
};

/* for passing single values */
struct ethtool_value 
{
    __u32	cmd;
    __u32	data;
};

union iaddr 
{
    unsigned u;
    unsigned char b[4];
};

struct fds_stat
{
    long db_ialloc;
    long sk_ialloc;
    long db_itotal;
    long sk_itotal;
    long *db_inode;
    long *sk_inode;
    char* db_path;
};

struct conn_stat
{
    union iaddr laddr;
    union iaddr raddr;
    unsigned lport; 
    unsigned rport; 
    unsigned state; 
    unsigned txq; 
    unsigned rxq; 
    unsigned num;
    unsigned tr; 
    unsigned when; 
    unsigned retrnsmt; 
    unsigned uid;
    unsigned timeout;
    unsigned inode;
    struct conn_stat* next;
};

struct sock_stat
{
    unsigned int fe_port;
    unsigned int be_port;
    unsigned int fe_total;
    unsigned int be_total;
    struct conn_stat sk_stat;
};

struct disk_stat
{
    unsigned int major;
    unsigned int minor; 
    char *dev_name;
    unsigned long rd_ios;
    unsigned long rd_merges;
    unsigned long long rd_sectors;
    unsigned long rd_ticks;
    unsigned long wr_ios;
    unsigned long wr_merges;
    unsigned long long wr_sectors;
    unsigned long wr_ticks;
    unsigned long ios_pgr;
    unsigned long tot_ticks;
    unsigned long rq_ticks;
    struct disk_stat* next;
};

struct mounted_stat
{
    const char *device;
    const char *mount_point;
    const char *filesystem;
    const char *flags;
    struct mounted_stat* next;
};

struct vstate
{
    pid_t pid;
    double cpu_usage;
    double mem_usage;
    struct cpu_arch st_arch;
    struct iface_dev st_ndev;
    struct cpu_info st_cpu;
    struct mem_info st_mem;
    struct pid_stat st_pid;
    struct pid_io st_pio;	
    struct if_stats st_ifs;
    struct fds_stat st_fds;
    struct sock_stat st_sock;
    struct disk_stat st_disk;
    struct mounted_stat st_mount;
};


int options = 0;
int intetval = 4;
char vsbuf[2046];
char *vsline = vsbuf;
FILE* logfile = NULL;

#define S_VALUE(m,n,p)	(((double) ((n) - (m))) / (p) * HZ)
/* new define to normalize to %; HZ is 1024 on IA64 and % should be normalized to 100 */
#define SP_VALUE(m,n,p)	(((double) ((n) - (m))) / (p) * 100)

double ll_sp_value(unsigned long long value1, unsigned long long value2,
                   unsigned long long itv)
{
    if ((value2 < value1) && (value1 <= 0xffffffff))
        /* Counter's type was unsigned long and has overflown */
        return ((double) ((value2 - value1) & 0xffffffff)) / itv * 100;
    else
        return SP_VALUE(value1, value2, itv);
}

double ll_s_value(unsigned long long value1, unsigned long long value2,
                  unsigned long long itv)
{
    if ((value2 < value1) && (value1 <= 0xffffffff))
        /* Counter's type was unsigned long and has overflown */
        return ((double) ((value2 - value1) & 0xffffffff)) / itv * HZ;
    else
        return S_VALUE(value1, value2, itv);
}

int read_proc_stat(struct cpu_info *cpu)
{
    FILE *fp;
    char line[8192];

    if ((fp = fopen("/proc/stat", "r")) == NULL) {
        printf("Can't open /proc/stat.\n");
        return -1;
    }

    /* Some fields are only present in 2.6 kernels */
    memset(cpu,0,sizeof(struct cpu_info));
    while (fgets(line, 8192, fp) != NULL) {

        if (!strncmp(line, "cpu ", 4)) {
            /*
            * Read the number of jiffies spent in the different modes,
            * and compute system uptime in jiffies (1/100ths of a second
            * if HZ=100).
            */
            sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu",
                &(cpu->user), &(cpu->nice), &(cpu->system), &(cpu->idle), &(cpu->iowait),
                &(cpu->hardirq), &(cpu->softirq), &(cpu->steal));
        }
        break;
    }

    fclose(fp);
    return 0;
}

int read_cpu_info(struct cpu_arch *cpu)
{
    FILE *fp;
    char line[1024];
    char str[512];	

    fp = fopen ("/proc/cpuinfo", "r");
    if (fp == NULL){
        printf("Can't open /proc/cpuinfo.\n");
        return -1;
    }

    memset(cpu,0,sizeof(struct cpu_arch));
    while(fgets(line, 1024, fp) != NULL){
        if(strstr(line, "processor") == str){ 
            cpu->procnum++; continue;
        }
        if(sscanf(line, "model name : %69[a-zA-Z0-9/+() ]", cpu->medel_name) == 1)
            continue;
        if(sscanf(line, "cpu MHz         : %u", &cpu->MHz) == 1) 
            continue;
    }

    fclose(fp);
    return 0;
}

int read_mem_info(struct mem_info *minfo)
{
    FILE *fp;
    static char line[128];

    fp = fopen ("/proc/meminfo", "r");
    if (fp == NULL){
        printf("Can't open /proc/meminfo.\n");
        return -1;
    }

    memset(minfo,0,sizeof(struct mem_info));
    while (fgets(line, 128, fp) != NULL) {
        if (!strncmp(line, "MemTotal:", 9))
            /* Read the total amount of memory in kB */
            sscanf(line + 9, "%lu", &(minfo->mem_total));
        else if (!strncmp(line, "MemFree:", 8))
            /* Read the amount of free memory in kB */
            sscanf(line + 8, "%lu", &(minfo->mem_free));

        else if (!strncmp(line, "Buffers:", 8))
            /* Read the amount of buffered memory in kB */
            sscanf(line + 8, "%lu", &(minfo->buffers));

        else if (!strncmp(line, "Cached:", 7))
            /* Read the amount of cached memory in kB */
            sscanf(line + 7, "%lu", &(minfo->cached));

        else if (!strncmp(line, "SwapCached:", 11))
            /* Read the amount of cached swap in kB */
            sscanf(line + 11, "%lu", &(minfo->swap_cached));

        else if (!strncmp(line, "SwapTotal:", 10))
            /* Read the total amount of swap memory in kB */
            sscanf(line + 10, "%lu", &(minfo->swap_total));

        else if (!strncmp(line, "SwapFree:", 9))
            /* Read the amount of free swap memory in kB */
            sscanf(line + 9, "%lu", &(minfo->swap_free));
    }

    fclose(fp);
    return 0;
}

char* get_vfs_name(long magic)
{
    static char fsname[20] = {0};

    switch(magic)
    {
    case 0xadf5: strcpy(fsname,"ADFS"); break;
    case 0xadff: strcpy(fsname,"AFFS"); break;
    case 0x5346414f: strcpy(fsname,"AFS"); break;
    case 0x0187: strcpy(fsname,"AUTOFS"); break;
    case 0x73757245: strcpy(fsname,"CODA"); break;
    case 0x414a53: strcpy(fsname,"EFS"); break;
    case 0xef53: strcpy(fsname,"EXT3"); break;
    case 0xf995e849: strcpy(fsname,"HPFS"); break;
    case 0x9660: strcpy(fsname,"ISOFS"); break;
    case 0x72b6: strcpy(fsname,"JFFS2"); break;
    case 0x19700426: strcpy(fsname,"KVMFS"); break;
    case 0x09041934: strcpy(fsname,"ANON"); break;
    case 0x137f: strcpy(fsname,"MINIX"); break;
    case 0x138f: strcpy(fsname,"MINIX"); break;
    case 0x2468: strcpy(fsname,"MINIX2"); break;
    case 0x2478: strcpy(fsname,"MINIX2"); break;
    case 0x4d5a: strcpy(fsname,"MINIX3"); break;
    case 0x4d44: strcpy(fsname,"MSDOS"); break;
    case 0x564c: strcpy(fsname,"NCP"); break;
    case 0x6969: strcpy(fsname,"NFS"); break;
    case 0x9fa1: strcpy(fsname,"OPENPROM"); break;
    case 0x9fa0: strcpy(fsname,"PROC"); break;
    case 0x002f: strcpy(fsname,"QNX4"); break;
    case 0x52654973: strcpy(fsname,"REISERFS"); break;
    case 0xf15f083d: strcpy(fsname,"UNIONFS");  break;
    case 0x517b: strcpy(fsname,"SMB"); break;
    case 0x9fa2: strcpy(fsname,"USB"); break;
    default:  strcpy(fsname,"UNKNOW");
    }
    return fsname;
}

int read_vfs_df(char *path) 
{
    struct statfs st;

    if (statfs(path, &st) < 0) {
        return -1;
    }else{
        printf(" DB Partition: %s %lldG total, %lldM used, %lldM free (block size %d)\n",
            get_vfs_name(st.f_type),
            ((long long)st.f_blocks * (long long)st.f_bsize) /1024/1024/1024,
            ((long long)(st.f_blocks - (long long)st.f_bfree) * st.f_bsize)/1024/1024,
            ((long long)st.f_bfree * (long long)st.f_bsize) / 1024/1024,
            (int) st.f_bsize);
    }
    return 0;
}

int read_mounted_stat(struct mounted_stat* mounts)
{
    FILE *fp;
    char line[2048];
    char device[64];
    char mount_point[64];
    char filesystem[64];
    char flags[128];
    int total = 0;

    fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        printf("Can't open /proc/mounts.\n");
        return -1;
    }

    while ((fgets(line, 4095, fp)) != NULL) {
        memset(device,0,64);
        memset(mount_point,0,64);
        memset(filesystem,0,64);
        memset(flags,0,128);

        sscanf(line, "%s %s %s %s",
            device, mount_point, filesystem, flags);

        if(total){
            if(mounts->next == NULL)
                mounts->next = MALLOC(struct mounted_stat);
            mounts = mounts->next;
        }

        mounts->device = strdup(device);
        mounts->mount_point = strdup(mount_point);
        mounts->filesystem = strdup(filesystem);
        mounts->flags = strdup(flags);
        total++;
    }

    fclose(fp);
    return 0;
}

struct mounted_stat* 
    get_path_mounted(struct mounted_stat* st_mounts,char* path)
{
    int len;
    char* ppath, *p;
    struct mounted_stat* mount;

    if(path == NULL || !strlen(path))
        return NULL;

    ppath = strdup(path);
    p = ppath + 1;

    while (p > ppath){
        p = strrchr(ppath, '/');
        if(p == NULL)
            break;

        len = p - ppath;
        if(p != ppath)
            *p = '\0';
        else
            len+=1;

        mount = st_mounts;        
        while (mount){
            if (!strncmp(mount->filesystem,"rootfs", 6)){
                mount = mount->next;
                continue;     
            }
            if(len == 1){ /*root*/
                if(strlen(mount->mount_point) == 1){
                    free(ppath);
                    return mount;
                }
            }else if(!strncmp(ppath, mount->mount_point, len)){
                free(ppath);
                return mount;
            }
            mount = mount->next;
        }
    }

    free(ppath);
    return NULL;
}

int read_vfs_stat() 
{
    FILE *fp;
    static char line[128];

    fp = fopen("/proc/mounts", "r");
    if(fp == NULL){
        printf("Can't open /proc/mounts.\n");
        return -1;
    }

    while (fgets(line, 2000, fp)) {
        char *c, *e = line;

        for (c = line; *c; c++) {
            if (*c == ' ') {
                e = c + 1;
                break;
            }
        }

        for (c = e; *c; c++) {
            if (*c == ' ') {
                *c = '\0';
                break;
            }
        }
        read_vfs_df(e);
    }

    fclose(fp);
    return 0;
}

int read_pid_stat(pid_t pid, struct pid_stat *pinfo)
{
    int ret;
    FILE *fp;
    char filelink[_POSIX_PATH_MAX];
    char filename[_POSIX_PATH_MAX];
    char line[2048],*s,*t;    
    struct stat st;

    if (pinfo == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(pinfo,0,sizeof(struct pid_stat));
    sprintf (filename, "/proc/%u/stat", (unsigned)pid);
    if (-1 == access (filename, R_OK)) {
        printf("please get root right run vstat.\n");
        return (pinfo->pid = -1);
    }

    if (stat (filename, &st) != -1) {
        pinfo->euid = st.st_uid;
        pinfo->egid = st.st_gid;
    }
    else {
        pinfo->euid = pinfo->egid = -1;
    }

    sprintf (filelink, "/proc/%u/exe", (unsigned)pid);
    ret = readlink(filelink,pinfo->exe,_POSIX_PATH_MAX);
    if(ret < 0 && ret <= PATH_MAX) {
        pinfo->exe[ret] = '\0';
    }

    if ((fp = fopen (filename, "r")) == NULL) {
        return (pinfo->pid = -1);
    }

    if ((s = fgets (line, 2048, fp)) == NULL) {
        fclose (fp);
        return (pinfo->pid = -1);
    }

    sscanf (line, "%u", &(pinfo->pid));
    s = strchr (line, '(') + 1;
    t = strchr (line, ')');
    strncpy (pinfo->comm, s, t - s);
    pinfo->comm [t - s] = ' ';
    sscanf (t + 2, "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %d %d %llu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %u %u",
        &(pinfo->state),
        &(pinfo->ppid),
        &(pinfo->pgid),
        &(pinfo->sid),
        &(pinfo->tty_nr),
        &(pinfo->tty_pgrp),
        &(pinfo->flags),
        &(pinfo->min_flt),
        &(pinfo->cmin_flt),
        &(pinfo->maj_flt),
        &(pinfo->cmaj_flt),
        &(pinfo->utime),
        &(pinfo->stimev),
        &(pinfo->cutime),
        &(pinfo->cstime),
        &(pinfo->priority),
        &(pinfo->nice),
        &(pinfo->num_threads),
        &(pinfo->zero),
        &(pinfo->start_time),
        &(pinfo->vsize),
        &(pinfo->rss),
        &(pinfo->rlim),
        &(pinfo->start_code),
        &(pinfo->end_code),
        &(pinfo->start_stack),
        &(pinfo->esp),
        &(pinfo->eip),
        &(pinfo->pendingsig),
        &(pinfo->block_sig),
        &(pinfo->sigign),
        &(pinfo->sigcatch),
        &(pinfo->wchan),
        &(pinfo->nswap),
        &(pinfo->cnswap),
        &(pinfo->exit_signal),
        &(pinfo->task_cpu),
        &(pinfo->task_rt_priority),
        &(pinfo->task_policy)
        );

    fclose(fp);
    return 0;
}

int read_pid_io(pid_t pid, struct pid_io* pio)
{
    FILE *fp;
    char filename[128], line[256];

    sprintf(filename, "/proc/%u/io", pid);
    if ((fp = fopen(filename, "r")) == NULL) {
        printf("process %d exited.\n",pid);
        /* No such process.*/
        return -1;
    }

    memset(pio,0,sizeof(struct pid_io));
    while (fgets(line, 256, fp) != NULL) {
        if (!strncmp(line, "read_bytes:", 11))
            sscanf(line + 12, "%llu", &(pio->read_bytes));
        else if (!strncmp(line, "write_bytes:", 12))
            sscanf(line + 13, "%llu", &(pio->write_bytes));
        else if (!strncmp(line, "cancelled_write_bytes:", 22))
            sscanf(line + 23, "%llu", &(pio->cancelled_write_bytes));
    }

    fclose(fp);
    return 0;
}

pid_t read_proc_piddir(char* comm,struct pid_stat *pinfo)
{
    DIR *dir;
    struct dirent *drp;
    pid_t pid = 0;
    uid_t uid = getuid();

    /* Open /proc directory */
    if ((dir = opendir("/proc")) == NULL) {
        printf("Can't open /proc.\n");
        return -1;
    }

    /* Get directory entries */
    while ((drp = readdir(dir)) != NULL) {
        if (isdigit(drp->d_name[0])) {            
            read_pid_stat(atoi(drp->d_name),pinfo);
            if (!strncmp(pinfo->comm, comm, 7)){
                if(uid == pinfo->euid){
                    pid = pinfo->pid;
                    break;
                }
            }			
        }
    }

    /* Close /proc directory */
    closedir(dir);
    return pid;
}

int get_if_mtu(const char *if_name)
{
    struct ifreq ifr;
    int fd, ret;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("Cannot get a control socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, if_name);

    ret = ioctl(fd, SIOCGIFMTU, &ifr);
    if (ret < 0) {
        perror("ioctl");
        return -1;
    }

    ret = close(fd);
    if (ret < 0) {
        return -1;
    }

    return ifr.ifr_mtu;
}

int read_ifstat_proc(struct if_stats *ifs)
{
    FILE *fp;
    char buf[2048];
    char *line;
    char *stat;
    char *name;	
    int total = 0;

    /* Open /proc/net/dev. */
    fp = fopen ("/proc/net/dev", "r");
    if (fp == NULL){
        printf("Can't open /proc/net/dev.\n");
        return -1;
    }

    /* Drop header lines. */
    fgets(buf, 2048, fp);
    fgets(buf, 2048, fp);

    /* Update each interface's statistics. */
    while (fgets (buf, 2048, fp) != NULL)
    {
        line = buf;
        /* Skip white space.*/
        while (*line == ' ')
            line++;
        name = line;
        /* Cut interface name. */
        stat = strrchr (line, ':');
        *stat++ = '\0';
        line = stat;

        /* Skip loop if*/
        if (!strncmp(name, "lo", 2)){
            continue;
        }
        
        if(total){
            if(ifs->next == NULL)
                ifs->next = MALLOC(struct if_stats);
            ifs = ifs->next;
        }

        /* Read interface stat*/
        sscanf(line, 
            "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            &ifs->rx_bytes,
            &ifs->rx_packets,
            &ifs->rx_errors,
            &ifs->rx_dropped,
            &ifs->rx_fifo,
            &ifs->rx_frame,
            &ifs->rx_compressed,
            &ifs->rx_multicast,

            &ifs->tx_bytes,
            &ifs->tx_packets,
            &ifs->tx_errors,
            &ifs->tx_dropped,
            &ifs->tx_fifo,
            &ifs->tx_colls,
            &ifs->tx_carrier,
            &ifs->tx_compressed);

        strcpy(ifs->devname,name);
        total++;
    }

    fclose(fp);
    return 0;
}

int get_interfaces(struct iface_dev *ifaces)
{
    struct ifconf ifc;
    char buff[8192];
    int fd, i, n;
    struct ifreq *ifr=NULL;    
    struct in_addr ipaddr;
    struct in_addr nmask;
    char *iname;
    int total = 0;

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

    memset(ifaces,0,sizeof(sizeof(struct iface_dev)));
    /* Loop through interfaces, looking for given IP address */    
    for (i=n-1;i>=0;i--) {
        if(total){
            if(ifaces->next == NULL)
                ifaces->next = MALLOC(struct iface_dev);
            ifaces = ifaces->next;
        }

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

        strncpy(ifaces->name, iname, sizeof(ifaces->name)-1);
        ifaces->name[sizeof(ifaces->name)-1] = 0;
        ifaces->ip = ipaddr;
        ifaces->netmask = nmask;
        total++;
    }

    close(fd);
    return total;
}

struct iface_dev* 
    get_interface_dev(struct iface_dev *ifaces,char* dev_name)
{
    struct iface_dev *ifa = ifaces;

    while (ifa){
        if (!strcmp(dev_name, dev_name)){
            return ifa;
        }
        ifa = ifa->next;
    }
    return NULL;
}

int print_ethx_info(struct ethtool_cmd *ep)
{
    printf(" Speed: ");
    switch (ep->speed) {
        case SPEED_10:
            printf("10Mb/s");
            break;
        case SPEED_100:
            printf("100Mb/s");
            break;
        case SPEED_1000:
            printf("1000Mb/s");
            break;
        case SPEED_2500:
            printf("2500Mb/s");
            break;
        case SPEED_10000:
            printf("10000Mb/s");
            break;
        default:
            printf("Unknown! (%i)", ep->speed);
            break;
    };

    printf(" Duplex: ");
    switch (ep->duplex) {
        case DUPLEX_HALF:
            printf("Half");
            break;
        case DUPLEX_FULL:
            printf("Full");
            break;
        default:
            printf("Unknown! (%i)", ep->duplex);
            break;
    };

    return 0;
}

int get_ethx_info(char* devname)
{
    int err;
    int fd;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;
    struct ethtool_value edata;
    int allfail = 1;

    /* Setup our control structures. */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, devname);

    /* Open control socket. */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }

    ecmd.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)&ecmd;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err == 0) {
        err = print_ethx_info(&ecmd);
        if (err)
            return err;
        allfail = 0;
    } else if (errno != EOPNOTSUPP) {
        return -1;
    }

    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&edata;
    err = ioctl(fd, SIOCETHTOOL, &ifr);
    if (err == 0) {
        printf(" Link: %s", edata.data ? "yes":"no");
        allfail = 0;
    } else if (errno != EOPNOTSUPP) {
        return -1;
    }

    if (allfail) {
        return -1;
    }
    return 0;
}

void addr2str(union iaddr addr, unsigned port, char *buf)
{
    if(port) {
        snprintf(buf, 64, "%d.%d.%d.%d:%d",
            addr.b[0], addr.b[1], addr.b[2], addr.b[3], port);
    } else {
        snprintf(buf, 64, "%d.%d.%d.%d:*",
            addr.b[0], addr.b[1], addr.b[2], addr.b[3]);
    }
}

const char *state2str(unsigned state)
{
    switch(state){
case 0x1: return "ESTABLISHED";
case 0x2: return "SYN_SENT";
case 0x3: return "SYN_RECV";
case 0x4: return "FIN_WAIT1";
case 0x5: return "FIN_WAIT2";
case 0x6: return "TIME_WAIT";
case 0x7: return "CLOSE";
case 0x8: return "CLOSE_WAIT";
case 0x9: return "LAST_ACK";
case 0xA: return "LISTEN";
case 0xB: return "CLOSING";
default: return "UNKNOWN";
    }
}

unsigned long find_inode(struct fds_stat* fds,unsigned long inode)
{
    int i;
    for (i=0;i<fds->sk_itotal;i++){
        if(fds->sk_inode[i] == inode)
            return inode;
    }
    return 0;
}

int read_proc_tcp(struct fds_stat* fds,struct sock_stat* st_sock)
{
    int n,total = 0;
    FILE *fp;
    char line[512];
    union iaddr laddr, raddr;
    unsigned port = 0;
    unsigned lport, rport, state, txq, rxq, num;
    unsigned tr, when, retrnsmt, uid,timeout,inode;
    struct conn_stat *st_conn = &st_sock->sk_stat;

    uid_t user_uid = getuid();
    fp = fopen("/proc/net/tcp", "r");
    if(fp == NULL) 
        return -1;

    fgets(line, 512, fp);
    memset(st_conn,0,sizeof(struct conn_stat));
    while(fgets(line, 512, fp) != NULL)
    {
        n = sscanf(line, " %d: %x:%x %x:%x %x %x:%x %x:%x %x %d %d %d",
            &num, &laddr.u, &lport, &raddr.u, &rport,
            &state, &txq, &rxq,&tr, &when, &retrnsmt, 
            &uid,&timeout, &inode);

        /* user connection/inode*/
        if(user_uid == uid && find_inode(fds,inode))
        {
            if(total){
                if(st_conn->next == NULL)
                    st_conn->next = MALLOC(struct conn_stat);
                st_conn = st_conn->next;
            }

            /*get sock listen port*/
            if(state == 0xA){
                if(lport > port)
                    port = lport;
            }

            st_conn->laddr.u =  laddr.u;
            st_conn->lport = lport;
            st_conn->raddr.u = raddr.u;
            st_conn->rport = rport;
            st_conn->state = state, 
            st_conn->txq = txq; 
            st_conn->rxq = rxq;
            st_conn->tr = tr; 
            st_conn->when = when; 
            st_conn->retrnsmt = retrnsmt;
            st_conn->uid = uid;
            st_conn->timeout = timeout; 
            st_conn->inode = inode;
            total++;
        }
    }

    st_sock->be_port = port;
    st_sock->fe_port = port-1;
    fclose(fp);
    return 0;
}

void extract_type_1_socket_inode(const char lname[], long * inode_p) {

    /* If lname is of the form "socket:[12345]", extract the "12345"
    as *inode_p.  Otherwise, return -1 as *inode_p.
    */

    if (strlen(lname) < PRG_SOCKET_PFXl+3) *inode_p = -1;
    else if (memcmp(lname, PRG_SOCKET_PFX, PRG_SOCKET_PFXl)) *inode_p = -1;
    else if (lname[strlen(lname)-1] != ']') *inode_p = -1;
    else {
        char inode_str[strlen(lname + 1)];  /* e.g. "12345" */
        const int inode_str_len = strlen(lname) - PRG_SOCKET_PFXl - 1;
        char *serr;

        strncpy(inode_str, lname+PRG_SOCKET_PFXl, inode_str_len);
        inode_str[inode_str_len] = '\0';
        *inode_p = strtol(inode_str,&serr,0);
        if (!serr || *serr || *inode_p < 0 || *inode_p >= INT_MAX) 
            *inode_p = -1;
    }
}

void extract_type_2_socket_inode(const char lname[], long * inode_p) {

    /* If lname is of the form "[0000]:12345", extract the "12345"
    as *inode_p.  Otherwise, return -1 as *inode_p.
    */

    if (strlen(lname) < PRG_SOCKET_PFX2l+1) *inode_p = -1;
    else if (memcmp(lname, PRG_SOCKET_PFX2, PRG_SOCKET_PFX2l)) *inode_p = -1;
    else {
        char *serr;

        *inode_p=strtol(lname + PRG_SOCKET_PFX2l,&serr,0);
        if (!serr || *serr || *inode_p < 0 || *inode_p >= INT_MAX) 
            *inode_p = -1;
    }
}

int read_proc_fds(pid_t pid,struct fds_stat* fds)
{
    DIR *dir;    
    struct dirent *drp;
    char path[_POSIX_PATH_MAX];
    char fdname[_POSIX_PATH_MAX];    
    char filelink[_POSIX_PATH_MAX];
    char *filename,*suffix;
    int lnamelen;
    long inode;

    /* Open /proc directory */
    sprintf(path,"/proc/%d/fd",pid);
    if ((dir = opendir(path)) == NULL) {
        if (errno==EACCES)
            printf("Permission denied: %s\n",path);
        return -1;
    }

    /* alloc inode struct */
    if(fds->sk_inode == NULL){
        fds->sk_ialloc = 1000;
        fds->sk_itotal = 0;
        fds->sk_inode = malloc(fds->sk_ialloc*sizeof(long));
    }

    /* Get directory entries */
    while ((drp = readdir(dir)) != NULL) {
        if (isdigit(drp->d_name[0])) {
            sprintf(fdname,"/proc/%d/fd/%s",pid,drp->d_name);
            lnamelen = readlink(fdname,filelink,_POSIX_PATH_MAX);
            if(lnamelen > 0 && lnamelen <= _POSIX_PATH_MAX) {
                filelink[lnamelen] = '\0';
            }

            extract_type_1_socket_inode(filelink, &inode);
            if (inode < 0) 
                extract_type_2_socket_inode(filelink, &inode);

            /* non socket fd,get db path*/
            if (inode < 0){ 
                if ((filename = strrchr(filelink, '/'))){
                    if((suffix = strrchr(filelink, '.'))){
                        if(strncmp(suffix+1,"db",2)){
                            fds->db_itotal++;
                            if((lnamelen-(filename-filelink)) > 40){
                                if(!fds->db_path){
                                    *filename = '\0';
                                    fds->db_path = strdup(filelink);
                                    *filename = '/';
                                }
                            }
                        }
                    }
                }
                continue;
            }

            /* socket fd,max hold 1000*/
            if(fds->sk_itotal < fds->sk_ialloc)
                fds->sk_inode[fds->sk_itotal++] = inode;
        }
    }

    /* Close /proc directory */
    closedir(dir);
    return 0;
}

int read_diskstats_stat(struct disk_stat* st_disk)
{	
    FILE *fp;
    int n,total=0;
    char line[256];
    char dev_name[256];

    unsigned int major, minor;
    unsigned long rd_ios,rd_merges, rd_ticks,wr_ios, wr_merges;
    unsigned long long rd_sectors ,wr_sectors;
    unsigned long wr_ticks, ios_pgr, tot_ticks, rq_ticks;

    fp = fopen("/proc/diskstats", "r");
    if(fp == NULL)
        return -1;

    while (fgets(line, 256, fp) != NULL) {
        memset(st_disk,0,sizeof(struct disk_stat));
        /* major minor name rio rmerge rsect ruse wio wmerge wsect wuse running use aveq */
        n = sscanf(line, "%u %u %s %lu %lu %llu %lu %lu %lu %llu %lu %lu %lu %lu",
            &major, 
            &minor, 
            dev_name,
            &rd_ios, 
            &rd_merges, 
            &rd_sectors, 
            &rd_ticks,
            &wr_ios, 
            &wr_merges, 
            &wr_sectors, 
            &wr_ticks, 
            &ios_pgr, 
            &tot_ticks, 
            &rq_ticks);

        /* It's a device and not a partition */
        if (!rd_ios && !wr_ios)
            /* Unused device: ignore it */
            continue;

        /* Skip loop ram/loop/dm*/
        if (!strncmp(dev_name, "ram", 3)){
            continue;
        }
        else if (!strncmp(dev_name, "loop", 4)){
            continue;
        }
        else if (!strncmp(dev_name, "dm", 4)){
            continue;
        }

        if(total){
            if(st_disk->next == NULL)
                st_disk->next = MALLOC(struct disk_stat);
            st_disk = st_disk->next;
        }

        st_disk->major = major;
        st_disk->minor = minor;
        st_disk->dev_name = strdup(dev_name);
        st_disk->rd_ios = rd_ios;
        st_disk->rd_merges = rd_merges;
        st_disk->rd_sectors = rd_sectors;
        st_disk->rd_ticks = rd_ticks;
        st_disk->wr_ios = wr_ios;
        st_disk->wr_merges = wr_merges;
        st_disk->wr_sectors = wr_sectors;
        st_disk->wr_ticks = wr_ticks;
        st_disk->ios_pgr = ios_pgr;
        st_disk->tot_ticks = tot_ticks;
        st_disk->rq_ticks = rq_ticks;
        total++;
    }

    fclose(fp);
    return 0;
}

void calculate_disk_stat(unsigned long long itv,int fctr,
struct disk_stat *new,struct disk_stat *old)
{
    unsigned long long rd_sec, wr_sec;
    double tput, util, await, svctm, arqsz, nr_ios;

    nr_ios = (new->rd_ios - old->rd_ios) + (new->wr_ios - old->wr_ios);
    tput = ((double) nr_ios) * HZ / itv;
    util = S_VALUE(old->tot_ticks, new->tot_ticks, itv);
    svctm = tput ? util / tput : 0.0;

    await = nr_ios ?
        ((new->rd_ticks - old->rd_ticks) + (new->wr_ticks - old->wr_ticks)) /
nr_ios : 0.0;

    rd_sec = new->rd_sectors - old->rd_sectors;
    if ((new->rd_sectors < old->rd_sectors) && (old->rd_sectors <= 0xffffffff))
        rd_sec &= 0xffffffff;
    wr_sec = new->wr_sectors - old->wr_sectors;
    if ((new->wr_sectors < old->wr_sectors) && (old->wr_sectors <= 0xffffffff))
        wr_sec &= 0xffffffff;

    arqsz = nr_ios ? (rd_sec + wr_sec) / nr_ios : 0.0;

    /*      DEV   rrq/s wrq/s   r/s   w/s  rsec  wsec  rqsz  qusz await svctm %util */
    printf(" %8.2f %8.2f %7.2f %7.2f %8.2f %8.2f %8.2f %8.2f %7.2f %6.2f %6.2f\n",
        S_VALUE(old->rd_merges, new->rd_merges, itv),
        S_VALUE(old->wr_merges, new->wr_merges, itv),
        S_VALUE(old->rd_ios, new->rd_ios, itv),
        S_VALUE(old->wr_ios, new->wr_ios, itv),
        ll_s_value(old->rd_sectors, new->rd_sectors, itv) / fctr,
        ll_s_value(old->wr_sectors, new->wr_sectors, itv) / fctr,
        arqsz,
        S_VALUE(old->rq_ticks, new->rq_ticks, itv) / 1000.0,
        await,
        /* The ticks output is biased to output 1000 ticks per second */
        svctm,
        /* Again: Ticks in milliseconds */
        util / 10.0);
}

void calculate_disk_basic_stat(unsigned long long itv, int flags, int fctr,
struct disk_stat *new,struct disk_stat *old)
{
    unsigned long long rd_sec, wr_sec;

    /* Print stats coming from /sys or /proc/{diskstats,partitions} */
    rd_sec = new->rd_sectors - old->rd_sectors;
    if ((new->rd_sectors < old->rd_sectors) && (old->rd_sectors <= 0xffffffff))
        rd_sec &= 0xffffffff;
    wr_sec = new->wr_sectors - old->wr_sectors;
    if ((new->wr_sectors < old->wr_sectors) && (old->wr_sectors <= 0xffffffff))
        wr_sec &= 0xffffffff;

    printf(" %8.2f %12.2f %12.2f %10llu %10llu\n",
        S_VALUE(old->rd_ios + old->wr_ios, new->rd_ios + new->wr_ios, itv),
        ll_s_value(old->rd_sectors, new->rd_sectors, itv) / fctr,
        ll_s_value(old->wr_sectors, new->wr_sectors, itv) / fctr,
        (unsigned long long) rd_sec / fctr,
        (unsigned long long) wr_sec / fctr);

    /* Print stats coming from /proc/stat 
    printf(" %8.2f %12.2f %12.2f %10lu %10lu\n",
    S_VALUE(old->dk_drive, new->dk_drive, itv),
    S_VALUE(old->dk_drive_rblk, new->dk_drive_rblk, itv) / fctr,
    S_VALUE(old->dk_drive_wblk, new->dk_drive_wblk, itv) / fctr,
    (new->dk_drive_rblk - old->dk_drive_rblk) / fctr,
    (new->dk_drive_wblk - old->dk_drive_wblk) / fctr);
    */
}

void calculate_cpu_usage(struct vstate *vst,struct cpu_info *new)
{
    struct cpu_info *old = &vst->st_cpu;
    unsigned long long total,user,nice,system,idle,iowait,hardirq,softirq;

    user = new->user - old->user; 
    nice = new->nice - old->nice;
    system = new->system - old->system;
    idle = new->idle - old->idle;
    iowait = new->iowait - old->iowait;
    hardirq = new->hardirq - old->hardirq;
    softirq = new->softirq - old->softirq;

    total = user+nice+system+idle+iowait+hardirq+softirq;
    vst->cpu_usage = (double)idle/total;
    *old = *new;

    int n = sprintf(vsline,"  %2.3f%%",vst->cpu_usage);
    vsline += n;
}

void calculate_mem_usage(struct vstate *vst)
{
    struct mem_info *m = &vst->st_mem;
    m->free_mem = m->mem_free + m->buffers + m->cached;
    m->used_mem = m->mem_total - m->free_mem;
    vst->mem_usage = (double)m->used_mem/m->mem_total;

    int n = sprintf(vsline," %2.3f%% %4luM",
        vst->mem_usage,
        vst->st_mem.used_mem >> 10);
    vsline += n;
}

void calculate_pio_rate(struct vstate *vst,struct pid_io* new)
{
    struct pid_io* old = &vst->st_pio;
    double read_bytes,write_bytes;

    read_bytes = new->read_bytes - old->read_bytes;
    write_bytes = new->write_bytes - old->write_bytes;
    *old = *new;

    read_bytes /= (1024*intetval); 
    write_bytes /= (1024*intetval);
    int n = sprintf(vsline," %7.1fk %7.1fk",
        read_bytes,write_bytes);
    vsline += n;
}

void swap_ifstat(struct if_stats *if1,struct if_stats *if2)
{
    struct if_stats *next;
    next = if1->next;
    *if1 = *if2;
    if2->next = next;
}

void calculate_if_rate(struct if_stats* old,struct if_stats* new)
{
    double rx_bytes,tx_bytes;

    //printf("old rx %llu tx %llu\n",old->rx_bytes,old->tx_bytes);
    //printf("new rx %llu tx %llu\n",new->rx_bytes,new->tx_bytes);

    rx_bytes = new->rx_bytes - old->rx_bytes;
    tx_bytes = new->tx_bytes - old->tx_bytes;
    //*old = *new;

    rx_bytes /= (1024*intetval); 
    tx_bytes /= (1024*intetval);
    int n = sprintf(vsline," %7.1fk %7.1fk",
        rx_bytes,tx_bytes);
    vsline += n;
}

void calculate_ifs_rate(struct vstate *vst,struct if_stats* new)
{
    struct if_stats* old = &vst->st_ifs;
    while (old && new){
        calculate_if_rate(old,new);
        old = old->next;
        new = new->next;
    }    
}

void printf_ifs_value(struct if_stats* ifs)
{
    while (ifs){
        printf("%s rx %llu tx %llu\n",ifs->devname,ifs->rx_bytes,ifs->tx_bytes);
        ifs = ifs->next;
    }    
}

void calculate_tcp_connection(struct sock_stat* skt)
{

    struct conn_stat *st_conn = &skt->sk_stat;

    while (st_conn)
    {
        if(st_conn->lport == skt->be_port && st_conn->state != 0xA)
            skt->be_total++;
        if(st_conn->lport == skt->be_port-1 && st_conn->state != 0xA)
            skt->fe_total++;
        st_conn = st_conn->next;
    }
}

char* get_uidname()
{
    static char uidname[20] = {0};
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    if (pw) {
        sprintf(uidname,"uid=%d(%s)'s", uid, pw->pw_name);
    } else {
        sprintf(uidname,"uid=%d's",uid);
    }
    return uidname;
}

char *time_to_string(time_t t)
{
    int len;
    static char time_buf[30];
    struct tm *tm = localtime(&t);

    len = strftime(time_buf, sizeof time_buf - 1, "%H:%M:%S", tm);
    time_buf[len] = '\0';
    return time_buf;
}

void stat_print(struct vstate *vst)
{
    *vsline = '\0';
    printf("%s\n",vsbuf);
    if(vsbuf != vsline && logfile){
        fprintf(logfile,"%s %s\n",
            time_to_string(time(0)),vsbuf);
        fflush(logfile);        
    }
    memset(vsbuf,0,2046);
    vsline = vsbuf;
}

void printf_tcp_connection(struct sock_stat* skt)
{
    char lip[64],rip[64];
    struct conn_stat *st_conn = &skt->sk_stat;

    while (st_conn)
    {
        addr2str(st_conn->laddr, st_conn->lport, lip);
        addr2str(st_conn->raddr, st_conn->rport, rip);
        printf("   TCP %-22s %-17s %s\n",
            lip, rip,state2str(st_conn->state));  
        st_conn = st_conn->next;
    }
    printf("\n");
}

void printf_interfaces(struct iface_dev* ifs)
{
    int mtu;
    struct iface_dev* ifa = ifs;
    while (ifa){
        if (!strncmp(ifa->name, "lo",2)){
            ifa = ifa->next;
            continue;
        }else{    						
            printf("    Interface: %s %s",
                ifa->name,inet_ntoa(ifa->ip));
            mtu = get_if_mtu(ifa->name);
            printf(" MTU: %d",mtu);
            get_ethx_info(ifa->name);
            printf("\n");
        }
        ifa = ifa->next;
    }
    printf("\n");
}

void print_headline(struct vstate *vst)
{
    struct mounted_stat* mount = NULL;

    //iface = get_interface_dev(&vst->st_ndev,"eth0");
    read_mounted_stat(&vst->st_mount);
    read_proc_fds(vst->pid,&vst->st_fds);
    read_proc_tcp(&vst->st_fds,&vst->st_sock);  
    calculate_tcp_connection(&vst->st_sock);

    printf("    Processor: %s MHz(%d)\n",
        vst->st_arch.medel_name,
        vst->st_arch.MHz);

    printf("      Vdaemon: %s\n",
        vst->st_pid.exe);

    printf("     BDB path: %s \n",
        vst->st_fds.db_path);

    read_vfs_df(vst->st_fds.db_path);
    mount = get_path_mounted(&vst->st_mount,vst->st_fds.db_path);

    if(mount != NULL)
        printf("  Mounted dev: %s(%s) %s\n",
        mount->device,
        mount->mount_point,
        mount->filesystem);

    printf("   PID(%5d): socket opened(%ld) DB Files(%ld) FE(%d) BE(%d)\n",
        vst->st_pid.pid,
        vst->st_fds.sk_itotal,
        vst->st_fds.db_itotal,
        vst->st_sock.fe_total,
        vst->st_sock.be_total);

    printf_interfaces(&vst->st_ndev);

    printf_tcp_connection(&vst->st_sock);
}

void print_stat_headline(struct vstate *vst)
{
    int i,count = 0;
    struct if_stats *ifs = &vst->st_ifs;

    /* print the interface device name*/
    printf("                                            ");
    while (ifs){
        printf("|%s            ",ifs->devname);
        ifs = ifs->next;
        count++;
    }
    printf("\n");

    /* print the system stat head line*/
    printf("  | CPU%%  UMEM%%  UMEM      HDR      HDW      RCV      SND");

    /* print the redundant interface stat*/
    for(i=0;i<count-1;i++){
        printf("      RCV      SND");
    }
}

void print_errors(struct vstate *vst)
{
    printf(" VaultUSA SuperVault - WWW.VaultUSA.COM\n");
    printf(" SuperValt(vdaemon) stat: not found %s daemon\n",get_uidname());
}

int stat_loop(struct vstate *vst)
{
    static int start = 0;    
    struct cpu_info cpu;
    struct pid_io pio;
    static struct if_stats ifs = {{0},0};

    /* get cpu usage info*/
    if(read_proc_stat(&cpu) == -1)
        return -1;
    if(read_pid_io(vst->pid,&pio) == -1)
        return -1;
    if(read_ifstat_proc(&ifs) == -1)
        return -1;

    /* first time startup*/
    if(start == 0){
        /* clone stat value*/
        vst->st_cpu = cpu;
        vst->st_pio = pio;
        swap_ifstat(&vst->st_ifs,&ifs);
        start = 1;
        print_stat_headline(vst);
        return 0;
    }

    /* calculate stat usage */
    calculate_cpu_usage(vst,&cpu);
    read_mem_info(&vst->st_mem);
    calculate_mem_usage(vst);
    calculate_pio_rate(vst,&pio);
    calculate_ifs_rate(vst,&ifs);
    swap_ifstat(&vst->st_ifs,&ifs);
    return 0;
}

int creat_vstat_log(char* name)
{
    logfile = fopen("vstat.log", "a");
    if(logfile == NULL)
        return -1;

    return 0;
}

int main(void) 
{
    int running = 1;    
    struct vstate vst;
    vsline = vsbuf;    

    /* init vst struct*/
    memset(&vst,0,sizeof(struct vstate));
    creat_vstat_log("vstat.log");

    /*get vs daemon pid*/    
    vst.pid = read_proc_piddir("vdaemon",&vst.st_pid);
    if(vst.pid == 0){
        print_errors(&vst);
        return 0;
    }

    /* get cpu arch info*/
    read_cpu_info(&vst.st_arch);

    /* get net interfaces & ip*/
    get_interfaces(&vst.st_ndev);

    /* output the headline*/
    print_headline(&vst);

    /* loop get pid stat*/
    while(running) {
        /* loop read proc info*/
        if(stat_loop(&vst) == -1)
            break;
        /* print proc stat info*/
        stat_print(&vst);
        usleep(1000000*intetval);
    }

    fclose(logfile);
    return EXIT_SUCCESS;
}
