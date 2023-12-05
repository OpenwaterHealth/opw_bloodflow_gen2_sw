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

vx_status inputStreamVisionFileInit(InputStream *self, InputStream_InitParams *initParams);
vx_status inputStreamVisionFileDeInit(InputStream *self);

vx_status inputStreamInit(InputStream *self, InputStream_InitParams *initParams)
{
    vx_status status;

    status= VX_SUCCESS;

    self->sensingId= initParams->sensingId;
    self->typeId= initParams->typeId;
    self->numChannels= initParams->numChannels;
    self->mode= initParams->mode;

    if ((self->sensingId == INPUT_STREAM_VISION) && (self->typeId == INPUT_STREAM_FILE))
    {
        status= inputStreamVisionFileInit(self, initParams);
    }
    else
    {
        status = VX_FAILURE;
    }

    return status;

}

vx_status inputStreamDeInit(InputStream *self)
{
    vx_status status;

    status= VX_SUCCESS;

    if ((self->sensingId == INPUT_STREAM_VISION) && (self->typeId == INPUT_STREAM_FILE))
    {
        status= inputStreamVisionFileDeInit(self);
    }
    else
    {
        status = VX_FAILURE;
    }

    return status;
}
