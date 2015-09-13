#include "platform.h"
#include "jackclient.h"

void platformInit(void)
{
    jackClientInit();
}

void platformRegisterIdleCallback(void(*cb)(void))
{
    (void)cb;
}

void platformMainloop(void)
{
    jackClientRun();
}
