#include <TI/tivx.h>

int app_multi_cam_main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    int status;

    tivxInit();
    status = app_openwater_main(argc, argv);
    tivxDeInit();

    return status;
}
