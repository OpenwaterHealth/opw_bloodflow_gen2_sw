/* Copyright (C) D3 Engineering - All Rights Reserved
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "inputStream.h"

//#define INPUT_STREAM_DEBUG

#ifdef INPUT_STREAM_DEBUG
#define DEBUG_PRINTF(f_, ...) printf((f_), ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(f_, ...)
#endif

static vx_status openCurFile(struct InputStreamStruct *self, int32_t channel);
static vx_status readDataFromFile(struct InputStreamStruct *self, int32_t channel, uint8_t *dataPtr, uint32_t numBytes, uint32_t *numBytesRead);
static vx_status closeCurFile(struct InputStreamStruct *self, int32_t channel);
static vx_status nextFile(struct InputStreamStruct *self);
static vx_status resetFile(struct InputStreamStruct *self);
static vx_status getCurFileIdx(struct InputStreamStruct *self, int32_t channel, uint32_t *curFileIdx);
static vx_status getNumFiles(struct InputStreamStruct *self, int32_t channel, uint32_t *streamLen);

/* Initialize the inputStream module for vision sensor with file  as physical medium. The function scans the content of each directory for files and store the names in self->_.file.fileList*/
vx_status inputStreamVisionFileInit(InputStream *self, InputStream_InitParams *initParams)
{
    // printf("<<<<VIDEO INPUT_STREAM>>>>: inputStreamVisionFileInit\n");
    vx_status status;
    status = VX_SUCCESS;
    char *path;

    assert(self->numChannels <= INPUT_STREAM_MAX_CHANNELS);

    self->open = openCurFile;
    self->close = closeCurFile;
    self->next = nextFile;
    self->reset = resetFile;
    self->read = readDataFromFile;
    self->getStreamLen = getNumFiles;
    self->getCurStreamPos = getCurFileIdx;

    if(self->numChannels > 0)
    {
        self->_.file.numFiles[0] = 1;
        path = &self->_.file.chanPathList[0][0];
        strcpy(path, initParams->_.file.path[0]);
        printf("<<<<VIDEO INPUT_STREAM>>>>: Initialized channel %d with path %s\n", 0, self->_.file.chanPathList[0]);
        self->_.file.curFileIdx[0] = 0;
        self->_.file.fileDesc[0] = NULL;
    }
    
    if(self->numChannels > 1)
    {
        self->_.file.numFiles[1] = 1;
        path = &self->_.file.chanPathList[1][0];
        strcpy(path, initParams->_.file.path[1]);
        printf("<<<<VIDEO INPUT_STREAM>>>>: Initialized channel %d with path %s\n", 1, self->_.file.chanPathList[1]);
        self->_.file.curFileIdx[1] = 0;
        self->_.file.fileDesc[1] = NULL;
    }

    return status;
}

vx_status inputStreamVisionFileDeInit(InputStream *self)
{
    // printf("<<<<VIDEO INPUT_STREAM>>>>: inputStreamVisionFileDeInit\n");
    vx_status status;

    status= VX_SUCCESS;
    
    if(self->numChannels > 0)
    {
        printf("<<<<VIDEO INPUT_STREAM>>>>: Release channel 0\n");
    }


    if(self->numChannels > 1)
    {
        printf("<<<<VIDEO INPUT_STREAM>>>>: Release channel 1\n");
    }

    return status;
}

static vx_status openCurFile(struct InputStreamStruct *self, int32_t channel)
{
    // printf("<<<<VIDEO INPUT_STREAM>>>>: openCurFile %d %s\n", channel, self->_.file.chanPathList[channel]);
    vx_status status;
    status = VX_SUCCESS;         
    if ((self->_.file.fileDesc[channel] = fopen(self->_.file.chanPathList[channel], "rb")) == NULL)
    {
        printf("<<<<VIDEO INPUT_STREAM ERROR>>>>: Unable to open channel %d file %s\n", channel, self->_.file.chanPathList[channel]);
        status = VX_FAILURE;
    }

    return status;
}

static vx_status readDataFromFile(struct InputStreamStruct *self, int32_t channel, uint8_t *dataPtr, uint32_t numBytes, uint32_t *numBytesRead)
{
    vx_status status;
    uint32_t actualNumBytes;
    status = VX_SUCCESS;
    if ((actualNumBytes=fread(dataPtr, 1, numBytes, self->_.file.fileDesc[channel])) != numBytes)
    {
        printf("<<<<VIDEO INPUT_STREAM ERROR>>>>: Unable to read channel %d, read %d bytes, expected %d bytes\n", channel, actualNumBytes, numBytes);
        status= VX_FAILURE;
    }

    *numBytesRead= actualNumBytes;
    return status;
};

static vx_status closeCurFile(struct InputStreamStruct *self, int32_t channel)
{
    vx_status status;
    status = VX_SUCCESS;
    fclose(self->_.file.fileDesc[channel]);
    // printf("<<<<VIDEO INPUT_STREAM>>>>: Closed channel %d file\n", channel);

    return status;
}

static vx_status nextFile(struct InputStreamStruct *self)
{
    vx_status status;
    status = VX_SUCCESS;
    
    //printf("<<<<VIDEO INPUT_STREAM>>>>: nextFile %d \n", 0);
    if(self->numChannels > 0)
    {
        self->_.file.curFileIdx[0] = 0;
    }
    
    if(self->numChannels > 1)
    {
        self->_.file.curFileIdx[1] = 0;
    }

    return status;
}

static vx_status resetFile(struct InputStreamStruct *self)
{
    vx_status status;
    status = VX_SUCCESS;
    
    // printf("<<<<VIDEO INPUT_STREAM>>>>: resetFile %d \n", 0);    
    if(self->numChannels > 0)
    {
        self->_.file.curFileIdx[0] = 0;
    }
    
    if(self->numChannels > 1)
    {
        self->_.file.curFileIdx[1] = 0;
    }
    
    return status;
}

static vx_status getCurFileIdx(struct InputStreamStruct *self, int32_t channel, uint32_t *curFileIdx)
{
    // printf("<<<<VIDEO INPUT_STREAM>>>>: getCurFileIdx for channel %d = %d \n", channel, 0);
    vx_status status = VX_SUCCESS;
    *curFileIdx = 0;
    return status;
}

static vx_status getNumFiles(struct InputStreamStruct *self, int32_t channel, uint32_t *streamLen)
{
    // printf("<<<<VIDEO INPUT_STREAM>>>>: getNumFiles for channel %d = %d\n", channel, 1);
    vx_status status= VX_SUCCESS;
    *streamLen= 1;
    return status;
}
