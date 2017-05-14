#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdk/XPLMDataAccess.h"
#include "sdk/XPLMProcessing.h"


typedef struct {
    char aircraft_icao[40];
    char aircraft_tailnum[40];
    double latitude;
    double longitude;
    double altitude;
    float track;
    float ground_speed;
    float air_speed;
    float vertical_speed;
} position_data_t;


static XPLMDataRef *acf_icao_dr = NULL;
static XPLMDataRef *acf_tailnum_dr = NULL;
static XPLMDataRef *latitude_dr = NULL;
static XPLMDataRef *longitude_dr = NULL;
static XPLMDataRef *elevation_dr = NULL;
static XPLMDataRef *mag_psi_dr = NULL;
static XPLMDataRef *groundspeed_dr = NULL;
static XPLMDataRef *true_airspeed_dr = NULL;
static XPLMDataRef *vh_ind_dr = NULL;


static float
FlightLoopCallback(float elapsedMe, float elapsedSim, int counter, void *refcon)
{
    position_data_t *data = malloc(sizeof(position_data_t));
    if (data == NULL)
        goto cleanup;

    XPLMGetDatab(acf_icao_dr, data->aircraft_icao, 0, 40);
    if (data->aircraft_icao == NULL)
        goto cleanup;

    XPLMGetDatab(acf_tailnum_dr, data->aircraft_tailnum, 0, 40);
    if (data->aircraft_tailnum == NULL)
        goto cleanup;

    data->latitude = XPLMGetDatad(latitude_dr);
    data->longitude = XPLMGetDatad(longitude_dr);
    data->altitude = XPLMGetDatad(elevation_dr);
    data->track = XPLMGetDataf(mag_psi_dr);
    data->ground_speed = XPLMGetDataf(groundspeed_dr);
    data->air_speed = XPLMGetDataf(true_airspeed_dr);
    data->vertical_speed = XPLMGetDataf(vh_ind_dr);

    FILE *fp = fopen("/tmp/lol.txt", "a");
    fprintf(fp, "aircraft_icao: %s\n", data->aircraft_icao);
    fprintf(fp, "aircraft_tailnum: %s\n", data->aircraft_tailnum);
    fprintf(fp, "latitude: %f\n", data->latitude);
    fprintf(fp, "longitude: %f\n", data->longitude);
    fprintf(fp, "altitude: %f\n", data->altitude);
    fprintf(fp, "track: %f\n", data->track);
    fprintf(fp, "ground_speed: %f\n", data->ground_speed);
    fprintf(fp, "air_speed: %f\n", data->air_speed);
    fprintf(fp, "vertical_speed: %f\n\n", data->vertical_speed);
    fclose(fp);

cleanup:
    free(data);
    return 5;  // 5 seconds
}


PLUGIN_API int
XPluginStart(char *outName, char * outSig, char *outDesc)
{
    strcpy(outName, "xplogd");
    strcpy(outSig, "io.rgm.xplogd");
    strcpy(outDesc, "A plugin that sends your flight data to a remote server.");

    acf_icao_dr = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
    if (acf_icao_dr == NULL)
        return 0;

    acf_tailnum_dr = XPLMFindDataRef("sim/aircraft/view/acf_tailnum");
    if (acf_tailnum_dr == NULL)
        return 0;

    latitude_dr = XPLMFindDataRef("sim/flightmodel/position/latitude");
    if (latitude_dr == NULL)
        return 0;

    longitude_dr = XPLMFindDataRef("sim/flightmodel/position/longitude");
    if (longitude_dr == NULL)
        return 0;

    elevation_dr = XPLMFindDataRef("sim/flightmodel/position/elevation");
    if (elevation_dr == NULL)
        return 0;

    mag_psi_dr = XPLMFindDataRef("sim/flightmodel/position/mag_psi");
    if (mag_psi_dr == NULL)
        return 0;

    groundspeed_dr = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
    if (groundspeed_dr == NULL)
        return 0;

    true_airspeed_dr = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");
    if (true_airspeed_dr == NULL)
        return 0;

    vh_ind_dr = XPLMFindDataRef("sim/flightmodel/position/vh_ind");
    if (vh_ind_dr == NULL)
        return 0;

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
