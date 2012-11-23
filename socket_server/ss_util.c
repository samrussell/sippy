#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ss_util.h"

static uint32_t crc32_tab[] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*
 * Random table for Pearson hashing function below. Each 8-bit value only
 * appears once in the table and those values are expected to be randomly
 * distributed.
 */
static const uint8_t hash_tab[256] = {
    175, 235,  43, 181, 108, 210, 253,  28,   1,  41, 173,  78, 218,  61,  32,
     17,  31, 158, 147, 186, 206, 254, 239,  39,  85, 152, 150,  92, 196,  11,
    164,  72, 222,   8, 183, 129, 180, 234,  29, 121,  79, 116, 212,  16, 117,
    193, 189, 226, 199,  50,   9, 131,  14,  47,  80, 203, 124,  53, 162, 238,
     83, 157, 231,  88, 110, 109,  62, 245, 114, 105,  73, 242,  22,  56,  84,
     42,   0,  38, 144,  94, 249,  77, 137, 224,  74, 168,  75,  59,  71, 248,
    208,  44, 176, 112, 184, 182, 190, 246,  10, 192, 115, 146, 195, 160,  15,
     48, 145, 191, 107, 100,  58, 104, 217, 102,  76, 170, 149, 111, 148, 132,
    142,  66, 237,  69,  27,  89, 126,  82,  87, 227, 171, 232, 185,  49, 216,
    135, 243, 251, 103, 141, 236, 153, 228,  64,  93,  25,  21,  20, 127, 155,
     98,  23,  67,  51,  40,  99, 201, 241, 214,  34,  60,  18, 159,  45, 166,
     55, 205,  86, 134, 219, 169, 204, 26,  244, 174, 143, 198,  65, 140, 122,
    161, 165, 223, 207, 187, 178, 118, 202,  13, 177, 230, 197,  54, 194, 247,
    119, 113, 136, 221, 133,  57,  12, 167,  81,  70, 172,  30, 255,   5,  37,
    213, 101, 215,  90,   6,  52,  33,  97, 240, 123, 233,   7,  63, 125, 120,
      2, 188, 128, 179,  96, 154,  95,  24, 209, 151,  35, 252,  19,  46, 106,
    163, 225, 138, 211, 130, 220, 156, 229,  91, 200,  36,   3, 250,  68,   4,
    139
};

uint32_t
ss_crc32(const void *buf, size_t size)
{
    const uint8_t *p = buf;
    uint32_t crc;

    crc = ~0U;
    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}

/* Variant of Pearson hashing function */
uint32_t
hash_string(str *buf, int rounds)
{
    uint32_t res;
    uint8_t ires, *bp, *ep;
    int i;

    res = 0;
    ep = (uint8_t *)(buf->s + buf->len);
    for (i = 0; i < rounds; i++) {
        ires = hash_tab[i];
        for (bp = (uint8_t *)buf->s; bp != ep; bp++) {
            ires = hash_tab[ires ^ bp[0]];
        }
        res <<= 8;
        res |= ires;
    }
    return res;
}

/*
 * Portable daemon(3) implementation, borrowed from FreeBSD. For license
 * and other information see:
 *
 * $FreeBSD: src/lib/libc/gen/daemon.c,v 1.8 2007/01/09 00:27:53 imp Exp $
 */
int
ss_daemon(int nochdir, int redirect_fd)
{
    struct sigaction osa, sa;
    pid_t newgrp;
    int oerrno;
    int osa_ok;

    /* A SIGHUP may be thrown when the parent exits below. */
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    osa_ok = sigaction(SIGHUP, &sa, &osa);

    switch (fork()) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(0);
    }

    newgrp = setsid();
    oerrno = errno;
    if (osa_ok != -1)
        sigaction(SIGHUP, &osa, NULL);

    if (newgrp == -1) {
        errno = oerrno;
        return (-1);
    }

    if (!nochdir)
        (void)chdir("/");

    if (redirect_fd != -1) {
        (void)dup2(redirect_fd, STDOUT_FILENO);
        (void)dup2(redirect_fd, STDERR_FILENO);
        if (redirect_fd > 2)
            (void)close(redirect_fd);
        redirect_fd = open("/dev/null", O_RDWR, 0);
        if (redirect_fd != -1) {
            (void)dup2(redirect_fd, STDIN_FILENO);
            if (redirect_fd > 2)
                (void)close(redirect_fd);
        }
    }
    return (0);
}
