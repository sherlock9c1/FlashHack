#ifndef KERNEL_EE_H
#define KERNEL_EE_H

#include "binfile.h"
#include <QDebug>
#include <QByteArray>

// E-SIDE MODE6 CTRL FROM MEMORY
static unsigned char ESIDE_DEV_ROUTINE_A[] = {
                      0xe4,0xff,0x06,0x01,0x3e,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0x2e,0x17,0x8b,0x55,0x8d,0x29,0x18,0xa6,0x00,
                      0x8d,0x24,0x18,0x08,0x5a,0x26,0xf6,0x96,0x2e,0x40,0x8d,0x1a,0x1f,0x2e,0x40,
                      0xfc,0xb6,0x18,0x03,0x84,0xfb,0xb7,0x18,0x03,0x37,0xc6,0x0b,0x5a,0x26,0xfd,
                      0x33,0x1d,0x2d,0x08,0x18,0x38,0x32,0x39,0xbd,0x01,0x8b,0x1f,0x2e,0x80,0xf9,
                      0xa7,0x2f,0x9b,0x2e,0x97,0x2e,0x39,0x37,0xc6,0x55,0xf7,0x10,0x3a,0x53,0xf7,
                      0x10,0x3a,0xc6,0x50,0xf7,0x18,0x06,0xc6,0xa0,0xf7,0x18,0x06,0x33,0x39,0x3c,
                      0xce,0x10,0x02,0x1d,0x00,0x08,0x38,0x39,0x3c,0xce,0x10,0x02,0x1c,0x00,0x08,
                      0x38,0x39,0x36,0x20,0x03,0x36,0x86,0x0a,0x37,0x4d,0x27,0x0b,0xc6,0x4b,0xbd,
                      0x01,0x8b,0x5a,0x26,0xfa,0x4a,0x26,0xf5,0x33,0x32,0x39,0x37,0xfc,0x10,0x0e,
                      0xfd,0x10,0x16,0x33,0x7f,0x10,0x22,0x20,0x07,0xb6,0x10,0x23,0x84,0x80,0x27,
                      0x05,0x86,0x80,0xb7,0x10,0x23,0x39,0x56};

static unsigned char ESIDE_DEV_ROUTINE_B[] = {
                      0xe4,0xff,0x06,0x00,0xa7,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0x2e,0x81,0x06,0x26,0xd9,0x8d,0x3a,0x97,0x2c,
                      0x7a,0x00,0x31,0x8d,0x33,0x97,0x2d,0x7a,0x00,0x31,0x18,0xde,0x2c,0x8d,0x29,
                      0x18,0xa7,0x00,0x18,0x08,0x7a,0x00,0x31,0x26,0xf4,0x8d,0x1d,0x5d,0x26,0xb7,
                      0x18,0xde,0x2c,0x18,0xad,0x00,0x20,0xaf,0x8d,0x10,0x3c,0x30,0xcc,0x05,0xaa,
                      0xed,0x00,0xc6,0x02,0xbd,0x01,0x24,0x38,0x7e,0x00,0x95,0x18,0x3c,0x18,0xce,
                      0x05,0x75,0x7f,0x00,0x2f,0x7a,0x00,0x2f,0x26,0x04,0x18,0x09,0x27,0x07,0xbd,
                      0x01,0x8b,0x1f,0x2e,0x0e,0x05,0x38,0x38,0x7e,0x00,0x95,0x1f,0x2e,0x20,0xe7,
                      0xa6,0x2f,0x16,0xdb,0x2e,0xd7,0x2e,0x18,0x38,0x39,0x36,0x18,0x3c,0x3c,0x18,
                      0x38,0xce,0x10,0x00,0x86,0x08,0xa7,0x2d,0x37,0xc6,0x0b,0x5a,0x26,0xfd,0x33,
                      0xb6,0x18,0x03,0x8a,0x04,0xb7,0x18,0x03,0xb6,0x18,0x02,0x8a,0x04,0xb7,0x18,
                      0x02,0x4f,0x97,0x2e,0x86,0xe4,0x8d,0x0b};

static unsigned char ESIDE_DEV_ROUTINE_C[] = {
                      0xe4,0xff,0x06,0x00,0x10,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0x7e,0x01,0xb3,0x7e,0x01,0xb6,0x7e,0x01,0xd8,
                      0x7e,0x01,0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0xb6,0x18,0x02,0x84,0xfc,0xb7,0x18,0x02,
                      0x86,0x49,0x97,0x01,0x8e,0x00,0x4a,0x0f,0xb6,0x18,0x01,0x8a,0x7f,0xb7,0x18,
                      0x01,0xb6,0x18,0x03,0x84,0xfb,0xb7,0x18,0x03,0xb6,0x10,0x09,0x8a,0x01,0xb7,
                      0x10,0x09,0xb6,0x10,0x08,0x84,0xfd,0xb7,0x10,0x08,0xb6,0x10,0x03,0x8a,0x08,
                      0xb7,0x10,0x03,0xbd,0x01,0xa1,0x3c,0x30,0x13,0x32,0x01,0x07,0xcc,0x06,0x01,
                      0xed,0x00,0x20,0x08,0x14,0x32,0x01,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x02,0xbd,
                      0x01,0x24,0x38,0xce,0x10,0x00,0x86,0x04,0xa7,0x2d,0xec,0x2e,0x4f,0x97,0x2e,
                      0x1c,0x2d,0x02,0x8d,0x52,0x81,0xe4,0x26,0xeb,0x8d,0x4c,0x80,0x56,0x25,0xe5,
                      0x97,0x31,0x8d,0x44,0x81,0x05,0x27,0xdd};

static unsigned char ESIDE_DEV_ROUTINE_D[] = {
                      0xe4,0x78,0x06,0x00,0x00,0x20,0x3c,0x20,0x47,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0xff,0xff,0x7e,0x01,0x24,0x7e,0x01,
                      0x8b,0x7e,0x01,0xa1,0x7e,0x01,0xaa,0xc7};

// T-SIDE MODE6 CTRL FROM MEMORY
static unsigned char TSIDE_DEV_ROUTINE_A[] = {
                      0xf4,0xff,0x06,0x01,0x46,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0x2e,0x17,0x8b,0x55,0x8d,0x29,0x18,0xa6,0x00,
                      0x8d,0x24,0x18,0x08,0x5a,0x26,0xf6,0x96,0x2e,0x40,0x8d,0x1a,0x1f,0x2e,0x40,
                      0xfc,0xb6,0x18,0x03,0x84,0xbf,0xb7,0x18,0x03,0x37,0xc6,0x0b,0x5a,0x26,0xfd,
                      0x33,0x1d,0x2d,0x08,0x18,0x38,0x32,0x39,0xbd,0x01,0x93,0x1f,0x2e,0x80,0xf9,
                      0xa7,0x2f,0x9b,0x2e,0x97,0x2e,0x39,0x37,0xc6,0x55,0xf7,0x10,0x3a,0x53,0xf7,
                      0x10,0x3a,0xc6,0x50,0xf7,0x18,0x06,0xc6,0xa0,0xf7,0x18,0x06,0x33,0x39,0x3c,
                      0xce,0x10,0x02,0x1d,0x00,0x08,0x38,0x39,0x3c,0xce,0x10,0x02,0x1c,0x00,0x08,
                      0x38,0x39,0x36,0x20,0x03,0x36,0x86,0x0a,0x37,0x4d,0x27,0x0b,0xc6,0x4b,0xbd,
                      0x01,0x93,0x5a,0x26,0xfa,0x4a,0x26,0xf5,0x33,0x32,0x39,0x37,0xfc,0x10,0x0e,
                      0xfd,0x10,0x16,0x33,0x7f,0x10,0x22,0x20,0x07,0xb6,0x10,0x23,0x84,0x80,0x27,
                      0x05,0x86,0x80,0xb7,0x10,0x23,0x39,0x6a};

static unsigned char TSIDE_DEV_ROUTINE_B[] = {
                      0xf4,0xff,0x06,0x00,0xaf,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0x2e,0x81,0x06,0x26,0xd9,0x8d,0x3a,0x97,0x2c,
                      0x7a,0x00,0x31,0x8d,0x33,0x97,0x2d,0x7a,0x00,0x31,0x18,0xde,0x2c,0x8d,0x29,
                      0x18,0xa7,0x00,0x18,0x08,0x7a,0x00,0x31,0x26,0xf4,0x8d,0x1d,0x5d,0x26,0xb7,
                      0x18,0xde,0x2c,0x18,0xad,0x00,0x20,0xaf,0x8d,0x10,0x3c,0x30,0xcc,0x05,0xaa,
                      0xed,0x00,0xc6,0x02,0xbd,0x01,0x2c,0x38,0x7e,0x00,0x9d,0x18,0x3c,0x18,0xce,
                      0x05,0x75,0x7f,0x00,0x2f,0x7a,0x00,0x2f,0x26,0x04,0x18,0x09,0x27,0x07,0xbd,
                      0x01,0x93,0x1f,0x2e,0x0e,0x05,0x38,0x38,0x7e,0x00,0x9d,0x1f,0x2e,0x20,0xe7,
                      0xa6,0x2f,0x16,0xdb,0x2e,0xd7,0x2e,0x18,0x38,0x39,0x36,0x18,0x3c,0x3c,0x18,
                      0x38,0xce,0x10,0x00,0x86,0x08,0xa7,0x2d,0x37,0xc6,0x0b,0x5a,0x26,0xfd,0x33,
                      0xb6,0x18,0x03,0x8a,0x40,0xb7,0x18,0x03,0xb6,0x18,0x02,0x8a,0x40,0xb7,0x18,
                      0x02,0x4f,0x97,0x2e,0x86,0xf4,0x8d,0x4b};

static unsigned char TSIDE_DEV_ROUTINE_C[] = {
                      0xf4,0xff,0x06,0x00,0x18,0x86,0xaa,0x36,0x18,0x30,0x86,0x06,0xc6,0x01,0xfe,
                      0xff,0xbc,0xad,0x00,0x32,0x39,0xe0,0x7e,0x01,0xd3,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8e,0x00,0x4a,0x0f,
                      0xb6,0x18,0x01,0x8a,0x80,0xb7,0x18,0x01,0xb6,0x18,0x00,0x84,0x7f,0xb7,0x18,
                      0x00,0xb6,0x18,0x03,0x84,0xbf,0xb7,0x18,0x03,0xb6,0x10,0x09,0x8a,0x01,0xb7,
                      0x10,0x09,0xb6,0x10,0x08,0x84,0xfd,0xb7,0x10,0x08,0xb6,0x10,0x03,0x8a,0x08,
                      0xb7,0x10,0x03,0xbd,0x01,0xa9,0x3c,0x30,0x13,0x32,0x01,0x07,0xcc,0x06,0x01,
                      0xed,0x00,0x20,0x08,0x14,0x32,0x01,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x02,0xbd,
                      0x01,0x2c,0x38,0xce,0x10,0x00,0x86,0x04,0xa7,0x2d,0xec,0x2e,0x4f,0x97,0x2e,
                      0x1c,0x2d,0x02,0x8d,0x52,0x81,0xf4,0x26,0xeb,0x8d,0x4c,0x80,0x56,0x25,0xe5,
                      0x97,0x31,0x8d,0x44,0x81,0x05,0x27,0xad};

static unsigned char TSIDE_DEV_ROUTINE_D[] = {
                      0xf4,0x80,0x06,0x00,0x00,0x20,0x49,0x20,0x47,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0xff,0xff,0x7e,0x01,0x2c,0x7e,0x01,
                      0x93,0x7e,0x01,0xa9,0x7e,0x01,0xb2,0x7e,0x01,0xbb,0x7e,0x01,0xbe,0x7e,0x01,
                      0x8c};


static unsigned char TSIDE_READ_ROUTINE[] = {
                                    0xf4,0xde,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                    0x02,0x9d,0x14,0x38,0x39,0x36,0x37,0x3c,0x18,0x3c,0xce,0x10,0x00,
                                    0x86,0x08,0xa7,0x2d,0x86,0x0b,0x4a,0x26,0xfd,0xb6,0x18,0x03,0x8a,
                                    0x40,0xb7,0x18,0x03,0x4f,0xb7,0x20,0x00,0x97,0x2e,0x86,0xf4,0xbd,
                                    0x01,0x85,0x86,0xd9,0xbd,0x01,0x85,0x86,0x06,0xbd,0x01,0x85,0x86,
                                    0xaa,0xbd,0x01,0x85,0xb6,0x02,0x02,0xbd,0x01,0x85,0xb6,0x02,0x03,
                                    0xbd,0x01,0x85,0x5f,0x18,0xfe,0x02,0x02,0x18,0x3a,0x18,0xa6,0x00,
                                    0xbd,0x01,0x85,0x5c,0xc1,0x80,0x26,0xef,0x96,0x2e,0x40,0xbd,0x01,
                                    0x85,0x18,0x08,0x18,0xff,0x02,0x02,0x1f,0x2e,0x40,0xfc,0xb6,0x18,
                                    0x03,0x84,0xbf,0xb7,0x18,0x03,0x37,0xc6,0x0b,0x5a,0x26,0xfd,0x33,
                                    0x1d,0x2d,0x08,0x18,0x38,0x38,0x33,0x32,0x39,0x99};

static unsigned char ESIDE_READ_ROUTINE[] = {
                                    0xe4,0xde,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                    0x02,0x9d,0x14,0x38,0x39,0x36,0x37,0x3c,0x18,0x3c,0xce,0x10,0x00,
                                    0x86,0x08,0xa7,0x2d,0x86,0x0b,0x4a,0x26,0xfd,0xb6,0x18,0x03,0x8a,
                                    0x04,0xb7,0x18,0x03,0x4f,0xb7,0x20,0x00,0x97,0x2e,0x86,0xe4,0xbd,
                                    0x01,0x7d,0x86,0xd9,0xbd,0x01,0x7d,0x86,0x06,0xbd,0x01,0x7d,0x86,
                                    0xaa,0xbd,0x01,0x7d,0xb6,0x02,0x02,0xbd,0x01,0x7d,0xb6,0x02,0x03,
                                    0xbd,0x01,0x7d,0x5f,0x18,0xfe,0x02,0x02,0x18,0x3a,0x18,0xa6,0x00,
                                    0xbd,0x01,0x7d,0x5c,0xc1,0x80,0x26,0xef,0x96,0x2e,0x40,0xbd,0x01,
                                    0x7d,0x18,0x08,0x18,0xff,0x02,0x02,0x1f,0x2e,0x40,0xfc,0xb6,0x18,
                                    0x03,0x84,0xfb,0xb7,0x18,0x03,0x37,0xc6,0x0b,0x5a,0x26,0xfd,0x33,
                                    0x1d,0x2d,0x08,0x18,0x38,0x38,0x33,0x32,0x39,0xf9};

// reset routines/cksum

static unsigned char TSIDE_VPP_REMOVE[] = {
                                   0xf4,0x91,0x06,0x02,0x00,0xb6,0x18,0x00,0x84,0x7f,0xb7,0x18,0x00,
                                   0x86,0xc8,0xbd,0x01,0xbb,0xb6,0x10,0x02,0x84,0xf8,0x8a,0x05,0xb7,
                                   0x10,0x02,0x86,0x07,0xb7,0x10,0x30,0x05,0x3d,0x3d,0x3d,0xb6,0x10,
                                   0x31,0x81,0x5a,0x23,0x05,0xcc,0x06,0x04,0x20,0x03,0xcc,0x06,0xaa,
                                   0x3c,0x30,0xed,0x00,0xc6,0x02,0x9d,0x14,0x38,0x39,0x7e};

static unsigned char ESIDE_RESET_MSG[] = {
                                   0xe4,0x70,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                   0x02,0x9d,0x14,0x38,0xce,0x01,0xff,0x6f,0x00,0x09,0x26,0xfb,0x6f,
                                   0x00,0x20,0xfe,0x2a};

static unsigned char TSIDE_RESET_MSG[] = {
                                   0xf4,0x70,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                   0x02,0x9d,0x14,0x38,0xce,0x01,0xff,0x6f,0x00,0x09,0x26,0xfb,0x6f,
                                   0x00,0x20,0xfe,0x1a};

// write routines
static unsigned char TSIDE_VPP_APPLY[] = {
                                   0xf4,0xc1,0x06,0x02,0x00,0xb6,0x10,0x02,0x84,0xf8,0xb7,0x10,0x02,
                                   0x86,0x07,0xb7,0x10,0x30,0x05,0x3d,0x3d,0x3d,0xb6,0x10,0x31,0x81,
                                   0x66,0x25,0x04,0x81,0xa1,0x23,0x05,0xcc,0x06,0x02,0x20,0x3e,0xb6,
                                   0x18,0x00,0x8a,0x80,0xb7,0x18,0x00,0x9d,0x17,0x9d,0x23,0x9d,0x17,
                                   0x9d,0x23,0xb6,0x10,0x02,0x84,0xf8,0x8a,0x05,0xb7,0x10,0x02,0x86,
                                   0x07,0xb7,0x10,0x30,0x05,0x3d,0x3d,0x3d,0xb6,0x10,0x31,0x81,0xbc,
                                   0x25,0x04,0x81,0xe9,0x23,0x0d,0xb6,0x18,0x00,0x84,0x7f,0xb7,0x18,
                                   0x00,0xcc,0x06,0x03,0x20,0x03,0xcc,0x06,0xaa,0x3c,0x30,0xed,0x00,
                                   0xc6,0x02,0x9d,0x14,0x38,0x39,0xba};

static unsigned char TSIDE_PREP_A[] = {
                                   0xf4,0x7f,0x06,0x02,0x00,0x9d,0x1d,0x86,0xaa,0xb7,0x55,0x55,0x86,
                                   0x55,0xb7,0xaa,0xaa,0x86,0x90,0xb7,0x55,0x55,0x9d,0x1a,0x3c,0x3c,
                                   0x30,0xfc,0x20,0x00,0xed,0x02,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x04,
                                   0x9d,0x14,0x38,0x38,0x39,0xe6};

static unsigned char TSIDE_ERASE_A[] = {
                               0xf4,0xd8,0x06,0x02,0x49,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x02,
                               0x9d,0x14,0x38,0x39,0xcc,0x06,0xaa,0x20,0x03,0xcc,0x06,0x07,0x3c,0x30,
                               0xed,0x00,0xc6,0x02,0x9d,0x14,0x38,0x9d,0x1a,0x39,0x36,0x37,0x3c,0x3c,
                               0x30,0xcc,0x06,0x55,0xed,0x00,0xc6,0x02,0x9d,0x14,0x38,0x38,0x33,0x32,
                               0x39,0x9d,0x29,0x4f,0xb7,0x02,0xc8,0xde,0x12,0x08,0x09,0x4f,0x36,0x9d,
                               0x26,0x32,0x27,0x0f,0x7c,0x02,0xc8,0xf6,0x02,0xc8,0xc1,0x05,0x25,0x05,
                               0x8d,0xd0,0x7f,0x02,0xc8,0x9d,0x1d,0xc6,0x40,0xe7,0x00,0xc6,0x00,0xe7,
                               0x00,0x9d,0x17,0xc6,0xc0,0xe7,0x00,0x9d,0x1a,0xe6,0x00,0x27,0x07,0x4c,
                               0x81,0x19,0x23,0xe5,0x20,0x09,0x9c,0x10,0x26,0xc8,0xcc,0x00,0x00,0x20,
                               0x03,0xcc,0x06,0x06,0x39,0x00,0x00,0x2e};

static unsigned char TSIDE_ERASE_B[] = {
                               0xf4,0xae,0x06,0x02,0x00,0xbd,0x02,0x7d,0x1a,0x83,0x00,0x00,0x27,0x03,
                               0x7e,0x02,0x5e,0x4f,0xb7,0x02,0xc7,0xde,0x10,0x9d,0x1d,0xc6,0x20,0xe7,
                               0x00,0xe7,0x00,0x9d,0x23,0x09,0x08,0xc6,0xa0,0xe7,0x00,0x9d,0x17,0x9d,
                               0x1a,0x36,0x9d,0x26,0x32,0x27,0x0f,0x7c,0x02,0xc8,0xf6,0x02,0xc8,0xc1,
                               0x05,0x25,0x05,0x8d,0x32,0x7f,0x02,0xc8,0xc6,0xff,0xe1,0x00,0x27,0x0f,
                               0x4c,0x26,0xce,0x7c,0x02,0xc7,0xf6,0x02,0xc7,0xc1,0x0c,0x23,0xc4,0x20,
                               0x0b,0x9d,0x1d,0x9c,0x12,0x26,0xc7,0x0e};

static unsigned char ESIDE_PREP_A[] = {
                              0xe4,0x7f,0x06,0x02,0x00,0x9d,0x1d,0x86,0xaa,0xb7,0x55,0x55,0x86,0x55,
                              0xb7,0xaa,0xaa,0x86,0x90,0xb7,0x55,0x55,0x9d,0x1a,0x3c,0x3c,0x30,0xfc,
                              0x20,0x00,0xed,0x02,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x04,0x9d,0x14,0x38,
                              0x38,0x39,0xf6};

static unsigned char ESIDE_ERASE_A[] = {
                               0xe4,0xd8,0x06,0x02,0x49,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,0x02,
                               0x9d,0x14,0x38,0x39,0xcc,0x06,0xaa,0x20,0x03,0xcc,0x06,0x07,0x3c,0x30,
                               0xed,0x00,0xc6,0x02,0x9d,0x14,0x38,0x9d,0x1a,0x39,0x36,0x37,0x3c,0x3c,
                               0x30,0xcc,0x06,0x55,0xed,0x00,0xc6,0x02,0x9d,0x14,0x38,0x38,0x33,0x32,
                               0x39,0x9d,0x29,0x4f,0xb7,0x02,0xc8,0xde,0x12,0x08,0x09,0x4f,0x36,0x9d,
                               0x26,0x32,0x27,0x0f,0x7c,0x02,0xc8,0xf6,0x02,0xc8,0xc1,0x05,0x25,0x05,
                               0x8d,0xd0,0x7f,0x02,0xc8,0x9d,0x1d,0xc6,0x40,0xe7,0x00,0xc6,0x00,0xe7,
                               0x00,0x9d,0x17,0xc6,0xc0,0xe7,0x00,0x9d,0x1a,0xe6,0x00,0x27,0x07,0x4c,
                               0x81,0x19,0x23,0xe5,0x20,0x09,0x9c,0x10,0x26,0xc8,0xcc,0x00,0x00,0x20,
                               0x03,0xcc,0x06,0x06,0x39,0x00,0x00,0x3e};

static unsigned char ESIDE_ERASE_B[] = {
                               0xe4,0xae,0x06,0x02,0x00,0xbd,0x02,0x7d,0x1a,0x83,0x00,0x00,0x27,0x03,
                               0x7e,0x02,0x5e,0x4f,0xb7,0x02,0xc7,0xde,0x10,0x9d,0x1d,0xc6,0x20,0xe7,
                               0x00,0xe7,0x00,0x9d,0x23,0x09,0x08,0xc6,0xa0,0xe7,0x00,0x9d,0x17,0x9d,
                               0x1a,0x36,0x9d,0x26,0x32,0x27,0x0f,0x7c,0x02,0xc8,0xf6,0x02,0xc8,0xc1,
                               0x05,0x25,0x05,0x8d,0x32,0x7f,0x02,0xc8,0xc6,0xff,0xe1,0x00,0x27,0x0f,
                               0x4c,0x26,0xce,0x7c,0x02,0xc7,0xf6,0x02,0xc7,0xc1,0x0c,0x23,0xc4,0x20,
                               0x0b,0x9d,0x1d,0x9c,0x12,0x26,0xc7,0x1e};

static unsigned char ESIDE_WRITE_ROUTINE[] = {
                                   0xe4,0xc0,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                   0x02,0x9d,0x14,0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                   0x00,0x00,0x00,0x00,0x00,0x18,0x38,0x18,0xa6,0x02,0xb7,0x02,0x67,
                                   0xcd,0xee,0x00,0x18,0x08,0x18,0x08,0x18,0x08,0x4f,0x9d,0x1d,0xc6,
                                   0x40,0xe7,0x00,0x18,0xe6,0x00,0xe7,0x00,0x9d,0x17,0xc6,0xc0,0xe7,
                                   0x00,0x9d,0x1a,0x18,0xe6,0x00,0xe1,0x00,0x27,0x07,0x4c,0x81,0x19,
                                   0x23,0xe1,0x20,0x0d,0x08,0x18,0x08,0x7a,0x02,0x67,0x26,0xd6,0xcc,
                                   0x06,0xaa,0x20,0x03,0xcc,0x06,0x08,0x3c,0x30,0xed,0x00,0xc6,0x02,
                                   0x9d,0x14,0x38,0x39,0x00,0xb0};

static unsigned char TSIDE_WRITE_ROUTINE[] = {
                                   0xf4,0xc0,0x06,0x02,0x00,0x3c,0x30,0xcc,0x06,0xaa,0xed,0x00,0xc6,
                                   0x02,0x9d,0x14,0x38,0x39,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                   0x00,0x00,0x00,0x00,0x00,0x18,0x38,0x18,0xa6,0x02,0xb7,0x02,0x67,
                                   0xcd,0xee,0x00,0x18,0x08,0x18,0x08,0x18,0x08,0x4f,0x9d,0x1d,0xc6,
                                   0x40,0xe7,0x00,0x18,0xe6,0x00,0xe7,0x00,0x9d,0x17,0xc6,0xc0,0xe7,
                                   0x00,0x9d,0x1a,0x18,0xe6,0x00,0xe1,0x00,0x27,0x07,0x4c,0x81,0x19,
                                   0x23,0xe1,0x20,0x0d,0x08,0x18,0x08,0x7a,0x02,0x67,0x26,0xd6,0xcc,
                                   0x06,0xaa,0x20,0x03,0xcc,0x06,0x08,0x3c,0x30,0xed,0x00,0xc6,0x02,
                                   0x9d,0x14,0x38,0x39,0x00,0xa0};

static unsigned char WRITE_PACKET_PRIMER[] = {0x00,0x00,0x06,0x03,0x00,0xBD,0x02,0x1a};

void export_old(const unsigned char* data, QByteArray &out) {
  quint16 offset = 0x0000;
  offset |= ( ( data[3] << 8 ) & 0xFF00);
  offset |= (data[4] & 0xFF);
  int s = data[1] - 0x52 - 1; // normal size less checksum.
  QByteArray d((char*)data,s);
  d.remove(0,5);
  out.replace(offset,d.size(),d);
  for(int x=0;x<offset;x++) out[x] = 0x00;
}

#endif // KERNEL_EE_H
