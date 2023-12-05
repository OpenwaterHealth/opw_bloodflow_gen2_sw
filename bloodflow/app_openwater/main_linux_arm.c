
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <TI/tivx.h>
#include <app_init.h>


int32_t appInit()
{
    int32_t status = 0;

    status = appCommonInit();

    if(status==0)
    {
        tivxInit();
        tivxHostInit();
    }
    return status;
}

int32_t appDeInit()
{
    int32_t status = 0;

    tivxHostDeInit();
    tivxDeInit();
    appCommonDeInit();

    return status;
}

int main(int argc, char *argv[])
{
    int status = 0;

    status = appInit();

    if(status==0)
    {
        int app_openwater_main(int argc, char* argv[]);

        status = app_openwater_main(argc, argv);
        appDeInit();
    }

    return status;
}
