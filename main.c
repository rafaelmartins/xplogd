#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "sdk/XPLMDataAccess.h"
#include "sdk/XPLMMenus.h"
#include "sdk/XPLMProcessing.h"
#include "sdk/XPLMUtilities.h"


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

static char url[1024];
static bool sending = true;
static XPLMMenuID status_menu;
static int status_index = 0;


/* xplogd protocol, version 1
 *
 * This is the definition of our protocol. all the fields are separated with
 * a newline '\n' character.
 *
 * The server should return 202 to notify that accepted the data, and always
 * check if the client sent the correct content-type header
 * (application/vnd.xplogd.serialized).
 */
const char *body_format =
    "1\n"   // protocol version
    "%s\n"  // aircraft icao
    "%s\n"  // aircraft tailnum
    "%f\n"  // latitude in degrees
    "%f\n"  // longitude in degrees
    "%f\n"  // altitude in meters (not feets!)
    "%f\n"  // track in degrees
    "%f\n"  // ground speed in meters per second (not knots!)
    "%f\n"  // air speed in meters per second (not knots!)
    "%f\n"  // vertical speed meters per second (not feets per minute!)
    "";


static bool
SendPositionData(const char *url, position_data_t *data)
{
    if (url == NULL || data == NULL)
        return false;

    int body_len = snprintf(NULL, 0, body_format, data->aircraft_icao,
        data->aircraft_tailnum, data->latitude, data->longitude, data->altitude,
        data->track, data->ground_speed, data->air_speed, data->vertical_speed);

    if (body_len <= 0)
        return false;

    char *body = malloc(body_len + 1);
    if (body == NULL)
        return false;

    snprintf(body, body_len + 1, body_format, data->aircraft_icao,
        data->aircraft_tailnum, data->latitude, data->longitude, data->altitude,
        data->track, data->ground_speed, data->air_speed, data->vertical_speed);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers,
        "Content-Type: application/vnd.xplogd.serialized");

    CURL *hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_URL, url);
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, PACKAGE_NAME "/" PACKAGE_VERSION);
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, body);

    bool rv = curl_easy_perform(hnd) == CURLE_OK;
    if (rv) {
        long http_code = 0;
        curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &http_code);
        rv = http_code == 202;
    }

    free(body);
    curl_slist_free_all(headers);
    curl_easy_cleanup(hnd);

    return rv;
}


static char*
str_lstrip(char *str)
{
    if (str == NULL)
        return NULL;
    int i;
    size_t str_len = strlen(str);
    for (i = 0; i < str_len; i++) {
        if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\n') &&
            (str[i] != '\r') && (str[i] != '\t') && (str[i] != '\f') &&
            (str[i] != '\v'))
        {
            str += i;
            break;
        }
        if (i == str_len - 1) {
            str += str_len;
            break;
        }
    }
    return str;
}


static char*
str_rstrip(char *str)
{
    if (str == NULL)
        return NULL;
    int i;
    size_t str_len = strlen(str);
    for (i = str_len - 1; i >= 0; i--) {
        if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\n') &&
            (str[i] != '\r') && (str[i] != '\t') && (str[i] != '\f') &&
            (str[i] != '\v'))
        {
            str[i + 1] = '\0';
            break;
        }
        if (i == 0) {
            str[0] = '\0';
            break;
        }
    }
    return str;
}


char*
str_strip(char *str)
{
    return str_lstrip(str_rstrip(str));
}


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

    // FIXME: remove this
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

    // FIXME: make this global, and increment with retries
    float time = 5;  // in seconds

    if (!SendPositionData(str_strip(url), data)) {
        // delay next cycle in 10s, to give the server a bit more time to
        // recover
        time += 10;
        if (sending) {
            XPLMSetMenuItemName(status_menu, status_index, "Status: Failed", 1);
            sending = false;
        }
    }
    else if (!sending) {
        XPLMSetMenuItemName(status_menu, status_index, "Status: Ok", 1);
        sending = true;
    }

cleanup:
    free(data);
    return time;
}


PLUGIN_API int
XPluginStart(char *outName, char * outSig, char *outDesc)
{
    strcpy(outName, "xplogd");
    strcpy(outSig, "io.rgm.xplogd");
    strcpy(outDesc, "A plugin that sends your flight data to a remote server.");

    char config_file[1024];
    XPLMGetSystemPath(config_file);
    strcat(config_file, "Resources");
    strcat(config_file, XPLMGetDirectorySeparator());
    strcat(config_file, "plugins");
    strcat(config_file, XPLMGetDirectorySeparator());
    strcat(config_file, "xplogd.txt");

    FILE *fp = fopen(config_file, "r");
    if (fp == NULL)
        return 0;

    // FIXME: read in loop?
    fread(url, sizeof(char), 1024, fp);
    fclose(fp);

    if (url == NULL)
        return 0;

    int submenu = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "xplogd", 0, 1);
    status_menu = XPLMCreateMenu("xplogd", XPLMFindPluginsMenu(),
        submenu, NULL, 0);

    sending = true;
    status_index = XPLMAppendMenuItem(status_menu, "Status: Ok", NULL, 1);
    XPLMEnableMenuItem(status_menu, status_index, 0);

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
