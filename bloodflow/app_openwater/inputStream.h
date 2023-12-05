/* Copyright (C) D3 Engineering - All Rights Reserved
 */

/* The  streaming module opens a data stream from a pre-defined source: file, camera, network and
 * fetches a frame of data every time its member function ReadFrameFunc() is called 
*/ 
#ifndef _INPUT_STREAM_H
#define _INPUT_STREAM_H

#include <TI/tivx.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <sys/stat.h>
#include <float.h>
#include <math.h>
#include <dirent.h>

#define INPUT_STREAM_MAX_CHANNELS 16
#define INPUT_STREAM_MAX_FILE_PATH 256

//#define INPUT_STREAM_DEBUG

struct InputStreamStruct;

typedef vx_status (*ReadFunc)(struct InputStreamStruct *self, int32_t channel, uint8_t *dataPtr, uint32_t numBytes, uint32_t *numBytesRead);
typedef vx_status (*OpenFunc)(struct InputStreamStruct *self, int32_t channel);
typedef vx_status (*CloseFunc)(struct InputStreamStruct *self, int32_t channel);
typedef vx_status (*NextFunc)(struct InputStreamStruct *self);
typedef vx_status (*ResetFunc)(struct InputStreamStruct *self);
typedef vx_status (*GetStreamLenFunc)(struct InputStreamStruct *self, int32_t channel, uint32_t* streamLen);
typedef vx_status (*GetCurStreamPosFunc)(struct InputStreamStruct *self, int32_t channel, uint32_t *curStreamPos);

enum InputStreamSensingId {INPUT_STREAM_VISION=0, INPUT_STREAM_RADAR= 1};
enum InputStreamType {INPUT_STREAM_FILE= 0, INPUT_STREAM_SERDES= 1, INPUT_STREAM_NETWORK= 2, INPUT_STREAM_SERIAL= 3, INPUT_STREAM_CAN= 4, INPUT_STREAM_MAX_NUM_TYPES};
enum InputStreamMode {INPUT_STREAM_STOP_AT_END= 0, INPUT_STREAM_INFINITE_LOOP= 1};

typedef struct InputStreamStruct
{
    uint32_t sensingId; /* InputStreamSensingId */
    uint32_t typeId; /* InputStreamType Id*/
    uint32_t numChannels;
    uint32_t mode;

    union
    {
        struct
        {
            char chanPathList[INPUT_STREAM_MAX_CHANNELS][INPUT_STREAM_MAX_FILE_PATH];
            /* Private variables */
            struct dirent **fileList[INPUT_STREAM_MAX_CHANNELS];
            uint32_t curFileIdx[INPUT_STREAM_MAX_CHANNELS];
            uint32_t numFiles[INPUT_STREAM_MAX_CHANNELS];
            FILE *fileDesc[INPUT_STREAM_MAX_CHANNELS];

        } file;

        struct
        {
            uint32_t dummy;
        } network;

        struct
        {
            char chanPathList[2*INPUT_STREAM_MAX_CHANNELS][INPUT_STREAM_MAX_FILE_PATH]; /* In Linux, serial ports are represented by files. Need 2 serial ports per radar, one for control and one for data  */
            int32_t ctlSerPortDesc[INPUT_STREAM_MAX_CHANNELS];
            int32_t dataSerPortDesc[INPUT_STREAM_MAX_CHANNELS];
            struct termios ctlTty[INPUT_STREAM_MAX_CHANNELS];
            struct termios dataTty[INPUT_STREAM_MAX_CHANNELS];
            char *cliPrompt[INPUT_STREAM_MAX_CHANNELS];
        } ser; /* serial port connection */

    } _; /* Coding Style Convention: "_" indicates union */

    ReadFunc read;
    OpenFunc open;
    CloseFunc close;
    NextFunc next;
    ResetFunc reset;
    GetStreamLenFunc getStreamLen;
    GetCurStreamPosFunc getCurStreamPos;
} InputStream;

typedef struct
{
    uint32_t sensingId; /* InputStreamSensingId */
    uint32_t typeId; /* InputStreamType Id*/
    uint32_t numChannels;
    uint32_t mode;
    union
    {
        struct
        {
            vx_char path[INPUT_STREAM_MAX_CHANNELS][INPUT_STREAM_MAX_FILE_PATH];
        } file;

        struct
        {
            uint32_t dummy;
        } network;

        struct
        {
            vx_char path[2*INPUT_STREAM_MAX_CHANNELS][INPUT_STREAM_MAX_FILE_PATH]; /* In Linux, serial ports are represented by files. Need 2 serial ports per radar, one for control and one for data  */
        } ser;

    } _; /* Coding Style Convention: "_" indicates union */

} InputStream_InitParams;


vx_status inputStreamInit(InputStream *self, InputStream_InitParams *initParams);
vx_status inputStreamDeInit(InputStream *self);

#endif
