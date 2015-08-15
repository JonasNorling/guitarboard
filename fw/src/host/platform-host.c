#include "platform.h"
#include "jackclient.h"

void platformInit(void)
{
    jackClientInit();
}

void platformMainloop(void)
{
    jackClientRun();
}
