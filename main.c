#include <stdio.h>
#include <string.h>
#include "sdk/XPLMDataAccess.h"
#include "sdk/XPLMProcessing.h"


static float
FlightLoopCallback(float elapsedMe, float elapsedSim, int counter, void *refcon)
{
    FILE *fp = fopen("/tmp/lol.txt", "a");
    fprintf(fp, "lol\n");
    fclose(fp);
    return -1;
}


PLUGIN_API int
XPluginStart(char *outName, char * outSig, char *outDesc)
{
    strcpy(outName, "xplogd");
    strcpy(outSig, "io.rgm.xplogd");
    strcpy(outDesc, "A plugin that sends your flight data to a remote server.");

    XPLMRegisterFlightLoopCallback(FlightLoopCallback, -1, 0);

    return 1;
}

PLUGIN_API void
XPluginStop(void)
{
}

PLUGIN_API void
XPluginDisable(void)
{
}

PLUGIN_API int
XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void
XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam)
{
}
