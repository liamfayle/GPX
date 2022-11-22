
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <stdbool.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlschemastypes.h>
#include "LinkedListAPI.h"
#include "GPXParser.h"

/*createGPXdoc Function
 * The following numerated steps describe the basic logic followed for the function. (All nuances will be commented in-line)
 * 1. Decalres xml pointers and tests version.
 * 2. Attempts to open provided file and if it does not exist returns NULL
 * 3. Obtains root element and begins parsing
 * 4. Checks that first node obtained is the <gpx> node!
 * 5. Parses gpx attributes and verifies they are properly initialized, if not free memory & return null
 */
/** Function to create an GPX object based on the contents of an GPX file.
 *@pre File name cannot be an empty string or NULL.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid GPXdoc has been created and its address was returned
        or
        An error occurred, and NULL was returned
 *@return the pinter to the new struct or NULL
 *@param fileName - a string containing the name of the GPX file
**/
GPXdoc *createGPXdoc(char *fileName)
{

    //[XML] init
    xmlDoc *file = NULL;
    xmlNode *node = NULL;
    xmlNode *value2 = NULL;
    xmlNode *value3 = NULL;
    xmlNode *nodeI = NULL; // node Iterate
    xmlAttr *attr = NULL;
    xmlNode *value = NULL;
    LIBXML_TEST_VERSION

    if (fileName == NULL)
    {

        return NULL;
    }

    file = xmlReadFile(fileName, NULL, 0);
    if (file == NULL)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
        return NULL; // Returns NULL pointer if file provided is invalid and thus cannot be parsed
    }
    node = xmlDocGetRootElement(file);

    //[GPXdoc Struct] init
    GPXdoc *doc = malloc(sizeof(GPXdoc)); // MALLOC MUST BE FREED [0]
    if (doc == NULL)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
    }
    doc->version = -33.3; // impossible value to error check later
    doc->creator = NULL;  // null to check if initialized later
    doc->waypoints = NULL;
    doc->routes = NULL;
    doc->tracks = NULL;

    //[XML] Parse Gpx TAG ONLY & error check validity!
    if (strcmp("gpx", (const char *)node->name) != 0)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL; // GPX tag must be first for gpx format!
    }

    if (node->ns == NULL)
    { // namespace tag does not exist -> Invalid gpx
        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL;
    }
    strcpy(doc->namespace, (const char *)node->ns->href);

    for (attr = node->properties; attr != NULL; attr = attr->next)
    { // parse gpx attributes

        if (strcmp("version", (const char *)attr->name) == 0)
        { // get version

            value = attr->children;
            doc->version = atof((const char *)value->content);
        }
        else if (strcmp("creator", (const char *)attr->name) == 0)
        { // get creator

            value = attr->children;
            doc->creator = malloc((sizeof(char) * strlen((const char *)value->content)) + (1 * sizeof(char))); // MALLOC MUST BE FREED [0]

            if (doc->creator == NULL)
            { // malloc fail
                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }

            strcpy(doc->creator, (const char *)value->content);
        }
    }

    if (doc->creator == NULL || strlen((const char *)doc->creator) == 0 || doc->version == -33.3 || strlen((const char *)doc->namespace) == 0)
    { // error check gpxdoc values

        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL;
    }

    // get children of gpx tag (all other tags)
    if (node->children != NULL)
    {

        node = node->children;
    }

    // Node [init]
    Waypoint *wpt = NULL;
    GPXData *data = NULL;
    Route *rte = NULL;
    Track *trk = NULL;
    TrackSegment *trkSeg = NULL;

    // Lists [init]
    List *wptExtra = NULL; // extra wpt data
    List *rteExtra = NULL; // extra rte data
    List *rteWpts = NULL;  // list for points of route
    List *trkExtra = NULL;
    List *trkSegs = NULL;
    List *trkWpts = NULL;

    List *waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints); // must be freed
    doc->waypoints = waypoints;
    List *routes = initializeList(&routeToString, &deleteRoute, &compareRoutes); // must be freed
    doc->routes = routes;
    List *tracks = initializeList(&trackToString, &deleteTrack, &compareTracks); // must be freed
    doc->tracks = tracks;

    /*Begin parsing of data other than gpx type
     *waypoints [x]
     *routes    [x]
     *tracks    []
     */
    for (nodeI = node; nodeI != NULL; nodeI = nodeI->next)
    {

        // Resets
        attr = NULL;
        value = NULL;
        value2 = NULL;
        value3 = NULL;
        wpt = NULL;
        rte = NULL;
        data = NULL;
        wptExtra = NULL;
        rteExtra = NULL;
        rteWpts = NULL;
        trkExtra = NULL;
        trkSegs = NULL;
        trk = NULL;
        trkSeg = NULL;
        trkWpts = NULL;

        /*Extract <wpt> data
         *covers all tags listed at gpx namespace
         *lat & lon attributes
         *all other tags are children*/
        if (strcmp((const char *)nodeI->name, "wpt") == 0)
        { // tag found

            wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
            wpt->otherData = NULL;
            wpt->name = NULL;
            if (wpt == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            wpt->latitude = -999.999;  // impossible values
            wpt->longitude = -999.999; // impossible values

            for (attr = nodeI->properties; attr != NULL; attr = attr->next)
            { // get lon/lat

                if (strcmp("lat", (const char *)attr->name) == 0)
                {

                    value = attr->children;
                    wpt->latitude = atof((const char *)value->content);
                }
                else if (strcmp("lon", (const char *)attr->name) == 0)
                {

                    value = attr->children;
                    wpt->longitude = atof((const char *)value->content);
                }
            }

            if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
            { // invalid file

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteWaypoint(wpt);
                deleteGPXdoc(doc);
                return NULL;
            }

            // init extra data list
            wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            wpt->otherData = wptExtra;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && wpt->name == NULL)
                    {

                        wpt->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));

                        if (wpt->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(wpt->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "ele") == 0 || strcmp((const char *)value->name, "time") == 0 || strcmp((const char *)value->name, "magvar") == 0 || strcmp((const char *)value->name, "geoidheight") == 0 || strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "sym") == 0 || strcmp((const char *)value->name, "type") == 0 || strcmp((const char *)value->name, "fix") == 0 || strcmp((const char *)value->name, "sat") == 0 || strcmp((const char *)value->name, "hdop") == 0 || strcmp((const char *)value->name, "vdop") == 0 || strcmp((const char *)value->name, "pdop") == 0 || strcmp((const char *)value->name, "ageofdgpsdata") == 0 || strcmp((const char *)value->name, "dgpsid") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0...1] tags ?
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(wpt->otherData, (void *)data);
                    }
                }
            }

            if (wpt->name == NULL)
            {

                wpt->name = malloc(sizeof(char) * 2);
                if (wpt->name == NULL)
                {

                    xmlFreeDoc(file);
                    xmlCleanupParser();
                    deleteWaypoint(wpt);
                    deleteGPXdoc(doc);
                    return NULL;
                }
                strcpy(wpt->name, "");
            }

            insertBack(waypoints, wpt);
        }
        else if (strcmp((const char *)nodeI->name, "rte") == 0)
        {

            rte = malloc(sizeof(Route));
            if (rte == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            rte->name = NULL;
            rteExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            rte->otherData = rteExtra;
            rteWpts = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
            rte->waypoints = rteWpts;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && rte->name == NULL)
                    {

                        rte->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));
                        if (rte->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(rte->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "number") == 0 || strcmp((const char *)value->name, "type") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0-1]
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(rteExtra, (void *)data);
                    }
                }

                if (strcmp((const char *)value->name, "rtept") == 0)
                { // Type waypoint !

                    // wpt = NULL;
                    wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
                    if (wpt == NULL)
                    {

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteRoute(rte);
                        deleteGPXdoc(doc);
                        return NULL;
                    }
                    wpt->latitude = -999.999;  // impossible value2s
                    wpt->longitude = -999.999; // impossible value2s
                    wpt->name = NULL;
                    wpt->otherData = NULL;

                    for (attr = value->properties; attr != NULL; attr = attr->next)
                    { // get lon/lat

                        if (strcmp("lat", (const char *)attr->name) == 0)
                        {

                            value2 = attr->children;
                            wpt->latitude = atof((const char *)value2->content);
                        }
                        else if (strcmp("lon", (const char *)attr->name) == 0)
                        {

                            value2 = attr->children;
                            wpt->longitude = atof((const char *)value2->content);
                        }
                    }

                    if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
                    { // invalid file

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteWaypoint(wpt);
                        deleteRoute(rte);
                        deleteGPXdoc(doc);
                        return NULL;
                    }

                    // init extra data list
                    wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
                    wpt->otherData = wptExtra;

                    for (value2 = value->children; value2 != NULL; value2 = value2->next)
                    {

                        if (value2->children != NULL)
                        {

                            if (strcmp((const char *)value2->name, "name") == 0 && wpt->name == NULL)
                            {

                                wpt->name = malloc(sizeof(char) * strlen((const char *)value2->children->content) + (1 * sizeof(char)));
                                if (wpt->name == NULL)
                                {

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteRoute(rte);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }
                                strcpy(wpt->name, (const char *)value2->children->content);
                            }
                            else if (strcmp((const char *)value2->name, "ele") == 0 || strcmp((const char *)value2->name, "time") == 0 || strcmp((const char *)value2->name, "magvar") == 0 || strcmp((const char *)value2->name, "geoidheight") == 0 || strcmp((const char *)value2->name, "cmt") == 0 || strcmp((const char *)value2->name, "desc") == 0 || strcmp((const char *)value2->name, "src") == 0 || strcmp((const char *)value2->name, "sym") == 0 || strcmp((const char *)value2->name, "type") == 0 || strcmp((const char *)value2->name, "fix") == 0 || strcmp((const char *)value2->name, "sat") == 0 || strcmp((const char *)value2->name, "hdop") == 0 || strcmp((const char *)value2->name, "vdop") == 0 || strcmp((const char *)value2->name, "pdop") == 0 || strcmp((const char *)value2->name, "ageofdgpsdata") == 0 || strcmp((const char *)value2->name, "dgpsid") == 0)
                            {

                                if (value2->name == NULL || strlen((const char *)value2->name) == 0 || value2->children->content == NULL || strlen((const char *)value2->children->content) == 0)
                                { // check empty strings

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteRoute(rte);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }

                                data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value2->children->content)) + (sizeof(char) * 1));
                                strcpy(data->name, (const char *)value2->name); // consider [0-1]
                                strcpy(data->value, (const char *)value2->children->content);

                                insertBack(wptExtra, (void *)data);
                            }
                        }
                    }

                    if (wpt->name == NULL)
                    {

                        wpt->name = malloc(sizeof(char) * 2);
                        if (wpt->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(wpt->name, "");
                    }

                    insertBack(rte->waypoints, wpt);
                }
            }

            if (rte->name == NULL)
            {

                rte->name = malloc(sizeof(char) * 2);
                if (rte->name == NULL)
                {

                    xmlFreeDoc(file);
                    xmlCleanupParser();
                    deleteRoute(rte);
                    deleteGPXdoc(doc);
                    return NULL;
                }
                strcpy(rte->name, "");
            }

            insertBack(routes, rte);
        }
        else if (strcmp((const char *)nodeI->name, "trk") == 0)
        { // track

            trk = malloc(sizeof(Track));
            if (trk == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            trk->name = NULL;
            trkExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            trk->otherData = trkExtra;
            trkSegs = initializeList(&trackSegmentToString, &deleteTrackSegment, &compareTrackSegments);
            trk->segments = trkSegs;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && trk->name == NULL)
                    {

                        trk->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));
                        if (trk->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(trk->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "number") == 0 || strcmp((const char *)value->name, "type") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0-1]
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(trkExtra, (void *)data);
                    }

                    if (trk->name == NULL)
                    {

                        trk->name = malloc(sizeof(char) * 2);
                        if (trk->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(trk->name, "");
                    }
                }

                if (strcmp((const char *)value->name, "trkseg") == 0)
                {

                    trkSeg = malloc(sizeof(TrackSegment));
                    if (trkSeg == NULL)
                    {

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteTrack(trk);
                        deleteGPXdoc(doc);
                        return NULL;
                    }
                    trkWpts = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
                    trkSeg->waypoints = trkWpts;

                    for (value2 = value->children; value2 != NULL; value2 = value2->next)
                    {

                        if (strcmp((const char *)value2->name, "trkpt") == 0)
                        {

                            wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
                            if (wpt == NULL)
                            {

                                xmlFreeDoc(file);
                                xmlCleanupParser();
                                deleteTrackSegment(trkSeg);
                                deleteTrack(trk);
                                deleteGPXdoc(doc);
                                return NULL;
                            }
                            wpt->latitude = -999.999;  // impossible value2s
                            wpt->longitude = -999.999; // impossible value2s
                            wpt->name = NULL;
                            wpt->otherData = NULL;

                            for (attr = value2->properties; attr != NULL; attr = attr->next)
                            { // get lon/lat

                                if (strcmp("lat", (const char *)attr->name) == 0)
                                {

                                    value3 = attr->children;
                                    wpt->latitude = atof((const char *)value3->content);
                                }
                                else if (strcmp("lon", (const char *)attr->name) == 0)
                                {

                                    value3 = attr->children;
                                    wpt->longitude = atof((const char *)value3->content);
                                }
                            }

                            if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
                            { // invalid file

                                xmlFreeDoc(file);
                                xmlCleanupParser();
                                deleteWaypoint(wpt);
                                deleteTrackSegment(trkSeg);
                                deleteTrack(trk);
                                deleteGPXdoc(doc);
                                return NULL;
                            }

                            // init extra data list
                            wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
                            wpt->otherData = wptExtra;

                            // wpt extra
                            for (value3 = value2->children; value3 != NULL; value3 = value3->next)
                            {

                                if (value3->children != NULL)
                                {

                                    if (strcmp((const char *)value3->name, "name") == 0 && wpt->name == NULL)
                                    {

                                        wpt->name = malloc(sizeof(char) * strlen((const char *)value3->children->content) + (1 * sizeof(char)));
                                        if (wpt->name == NULL)
                                        {

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }
                                        strcpy(wpt->name, (const char *)value3->children->content);
                                    }
                                    else if (strcmp((const char *)value3->name, "ele") == 0 || strcmp((const char *)value3->name, "time") == 0 || strcmp((const char *)value3->name, "magvar") == 0 || strcmp((const char *)value3->name, "geoidheight") == 0 || strcmp((const char *)value3->name, "cmt") == 0 || strcmp((const char *)value3->name, "desc") == 0 || strcmp((const char *)value3->name, "src") == 0 || strcmp((const char *)value3->name, "sym") == 0 || strcmp((const char *)value3->name, "type") == 0 || strcmp((const char *)value3->name, "fix") == 0 || strcmp((const char *)value3->name, "sat") == 0 || strcmp((const char *)value3->name, "hdop") == 0 || strcmp((const char *)value3->name, "vdop") == 0 || strcmp((const char *)value3->name, "pdop") == 0 || strcmp((const char *)value3->name, "ageofdgpsdata") == 0 || strcmp((const char *)value3->name, "dgpsid") == 0)
                                    {

                                        if (value3->name == NULL || strlen((const char *)value3->name) == 0 || value3->children->content == NULL || strlen((const char *)value3->children->content) == 0)
                                        { // check empty strings

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }

                                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value3->children->content)) + (sizeof(char) * 1));
                                        if (data == NULL)
                                        {

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }
                                        strcpy(data->name, (const char *)value3->name); // consider [0-1]
                                        strcpy(data->value, (const char *)value3->children->content);

                                        insertBack(wptExtra, (void *)data);
                                    }
                                }
                            }

                            if (wpt->name == NULL)
                            {

                                wpt->name = malloc(sizeof(char) * 2);
                                if (wpt->name == NULL)
                                {

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteTrackSegment(trkSeg);
                                    deleteTrack(trk);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }
                                strcpy(wpt->name, "");
                            }

                            insertBack(trkSeg->waypoints, wpt);
                        }
                    }

                    insertBack(trk->segments, trkSeg);
                }
            }

            insertBack(tracks, trk);
        }
    }

    //[XML] free parser & file
    xmlFreeDoc(file);
    xmlCleanupParser();

    return doc;
}

/** Function to create a string representation of an GPX object.
 *@pre GPX object exists, is not null, and is valid
 *@post GPX has not been modified in any way, and a string representing the GPX contents has been created
 *@return a string contaning a humanly readable representation of an GPX object
 *@param obj - a pointer to an GPX struct
 **/
char *GPXdocToString(GPXdoc *doc)
{

    // init
    if (doc == NULL)
    {
        return NULL;
    }

    if (doc->creator == NULL)
    {

        return NULL;
    }

    if (doc->waypoints == NULL)
    {

        return NULL;
    }

    char *wpts = toString(doc->waypoints); // must be freed
    char *rtes = toString(doc->routes);    // must be freed
    char *trks = toString(doc->tracks);

    int double_chars = 8;
    int buffer = 100;

    int len = buffer + strlen((const char *)doc->creator) + strlen((const char *)doc->namespace) + double_chars + strlen(wpts) + strlen(rtes) + strlen(trks);

    // format string
    char *toString = malloc(sizeof(char) * len);
    sprintf(toString, "<gpx   Creator: %s    Version: %.7g    NameSpace: %s/>\n%s\n%s\n%s</gpx>\n", doc->creator, doc->version, doc->namespace, wpts, rtes, trks);

    free(wpts);
    free(rtes);
    free(trks);
    return toString;
}

/*deleteGPXdoc Function
 * The following numerated steps describe the basic logic followed for the function. (All nuances will be commented in-line)
 * ***MUST CHECK IF VALUES HAVE ALREADY BEEN FREED || NULL TO PREVENT SEG FAULTS***
 */
/** Function to delete doc content and free all the memory.
 *@pre GPX object exists, is not null, and has not been freed
 *@post GPX object had been freed
 *@return none
 *@param obj - a pointer to an GPX struct
 **/
void deleteGPXdoc(GPXdoc *doc)
{

    if (doc == NULL)
    {

        return;
    }

    if (doc->creator != NULL)
    {

        free(doc->creator);
    }

    if (doc->waypoints != NULL)
    {

        freeList(doc->waypoints);
    }

    if (doc->routes != NULL)
    {

        freeList(doc->routes);
    }

    if (doc->tracks != NULL)
    {

        freeList(doc->tracks);
    }

    free(doc);
}

// GPX DATA HELPER FUNCTIONS
///////////////////////////
/*deleteGPXdata Function
 * free mem associated with gpx data struct
 */
void deleteGpxData(void *data)
{

    GPXData *gpx = (GPXData *)data;

    if (gpx == NULL)
    {

        return;
    }

    free(gpx);
}

/*gpxDataToString Function
 * Creates humanly readable string for gpxdata
 */
char *gpxDataToString(void *data)
{

    GPXData *gpx = (GPXData *)data;

    if (gpx == NULL)
    {

        return NULL;
    }

    if (gpx->name == NULL || gpx->value == NULL)
    {

        return NULL;
    }

    int buffer = 100; // 100 char buffer for newlines, spaces, and null chars

    int len = buffer + strlen(gpx->name) + strlen(gpx->value);

    char *toString = malloc(sizeof(char) * len);

    sprintf(toString, "\n        <%s>%s</%s>", gpx->name, gpx->value, gpx->name);

    return toString;
}

/*compareGpxData Function
 * Compares past pointers
 * returns 0 if one pointer null
 * returns 1 if first is larger
 * returns 2 if second is larger
 */
int compareGpxData(const void *first, const void *second)
{

    if (first == NULL)
    {

        return 0;
    }

    if (second == NULL)
    {

        return 0;
    }

    if (first > second)
    {

        return 1;
    }

    return 2;
}

// Waypoint Helper Functions
///////////////////////////
/*deleteWaypoint Function
 * Frees all data associated with <wpt> tag
 */
void deleteWaypoint(void *data)
{

    Waypoint *wpt = (Waypoint *)data;
    if (data == NULL)
    { // null pointer

        return;
    }

    if (wpt->otherData != NULL)
    { // check list is not null before freeing

        freeList(wpt->otherData);
    }

    if (wpt->name != NULL)
    {

        free(wpt->name);
    }

    free(wpt);
}

/*waypointToString Function
 * Turns <wpt> tag into humanly readabnle string
 */
char *waypointToString(void *data)
{

    Waypoint *wpt = (Waypoint *)data;
    if (wpt == NULL)
    {

        return NULL;
    }

    if (wpt->name == NULL)
    {

        return NULL;
    }

    if (wpt->otherData == NULL)
    {

        return NULL;
    }

    // must free these vars//////////////////////////
    char *listString = toString(wpt->otherData);
    ////////////////////////////////////////////////

    int double_chars = 8;
    int buffer = 100; // 100 char buffer for newlines, spaces, and null chars

    int len = buffer + strlen(wpt->name) + (double_chars * 2) + strlen(listString);

    char *toString = malloc(sizeof(char) * len);

    sprintf(toString, "    <wpt lat=%f lon=%f>\n        <name>%s</name>      %s\n    </wpt>\n", wpt->latitude, wpt->longitude, wpt->name, listString);

    free(listString);

    return toString;
}

/*compareWaypoints Function
 * Compares past pointers
 * returns 1 if first is larger
 * returns 2 if second larger
 */
int compareWaypoints(const void *first, const void *second)
{

    if (first == NULL)
    {

        return 0;
    }

    if (second == NULL)
    {

        return 0;
    }

    if (first > second)
    {

        return 1;
    }

    return 2;
}

// Route Helper Functions
////////////////////////
/*deleteRoute Function
 *
 *
 *
 *
 */
void deleteRoute(void *data)
{

    Route *rte = (Route *)data;
    if (rte == NULL)
    {

        return;
    }

    if (rte->name != NULL)
    {

        free(rte->name);
    }

    if (rte->waypoints != NULL)
    {

        freeList(rte->waypoints);
    }

    if (rte->otherData != NULL)
    {

        freeList(rte->otherData);
    }

    free(rte);
}

/*routeToString Function
 * Turns route to humanly readable string
 */
char *routeToString(void *data)
{

    Route *rte = (Route *)data;
    if (rte == NULL)
    {

        return NULL;
    }

    if (rte->name == NULL)
    {

        return NULL;
    }

    if (rte->otherData == NULL)
    {

        return NULL;
    }

    if (rte->waypoints == NULL)
    {

        return NULL;
    }

    // must free these vars//////////////////////////
    char *listString = toString(rte->otherData);
    char *wptString = toString(rte->waypoints);
    ////////////////////////////////////////////////

    int buffer = 100; // 100 char buffer for newlines, spaces, and null chars

    int len = buffer + strlen(rte->name) + strlen(listString) + strlen(wptString);

    char *toString = malloc(sizeof(char) * len);

    sprintf(toString, "    <rte>\n        <name>%s</name>      %s\n      %s    </rte>\n", rte->name, listString, wptString);
    free(listString);
    free(wptString);

    return toString;
}

/*compareRoutes Function
 * return 1 if first larger
 * return 2 if second larger
 */
int compareRoutes(const void *first, const void *second)
{

    if (first == NULL)
    {

        return 0;
    }

    if (second == NULL)
    {

        return 0;
    }

    if (first > second)
    {

        return 1;
    }

    return 2;
}

// TrackSegment Helper Functions
///////////////////////////////
/*deleteTrackSegment Function
 */
void deleteTrackSegment(void *data)
{

    TrackSegment *trkSeg = (TrackSegment *)data;

    if (trkSeg == NULL)
    {

        return;
    }

    if (trkSeg->waypoints != NULL)
    {

        freeList(trkSeg->waypoints);
    }

    free(trkSeg);
}

/*trackSegmentToString Function
 *
 *
 *
 *
 */
char *trackSegmentToString(void *data)
{

    TrackSegment *trkSeg = (TrackSegment *)data;

    if (trkSeg == NULL)
    {

        return NULL;
    }

    if (trkSeg->waypoints == NULL)
    {

        return NULL;
    }

    char *listString = toString(trkSeg->waypoints);
    char *toString = NULL;
    int buffer = 100;

    int len = buffer + strlen(listString);
    toString = malloc(sizeof(char) * len);

    sprintf(toString, "<trkseg>\n%s   </trkseg>\n", listString);

    free(listString);

    return toString;
}

/*compareTrackSegments Function
 *
 *
 *
 *
 */
int compareTrackSegments(const void *first, const void *second)
{

    if (first == NULL)
    {

        return 0;
    }

    if (second == NULL)
    {

        return 0;
    }

    if (first > second)
    {

        return 1;
    }

    return 2;
}

// Track Helper FUnctions
////////////////////////
/*deleteTrack Function
 *
 *
 *
 *
 */
void deleteTrack(void *data)
{

    Track *trk = (Track *)data;

    if (trk == NULL)
    {

        return;
    }

    if (trk->segments != NULL)
    {

        freeList(trk->segments);
    }

    if (trk->name != NULL)
    {

        free(trk->name);
    }

    if (trk->otherData != NULL)
    {

        freeList(trk->otherData);
    }

    free(trk);
}

/*trackToString Function
 *
 *
 *
 *
 */
char *trackToString(void *data)
{

    Track *trk = (Track *)data;

    if (trk == NULL)
    {

        return NULL;
    }

    if (trk->segments == NULL)
    {

        return NULL;
    }

    if (trk->name == NULL)
    {

        return NULL;
    }

    if (trk->otherData == NULL)
    {

        return NULL;
    }

    int buffer = 100;

    char *other = toString(trk->otherData);
    char *segs = toString(trk->segments);

    int len = buffer + strlen(other) + strlen(segs) + strlen(trk->name);

    char *toString = NULL;
    toString = malloc(sizeof(char) * len);

    sprintf(toString, "     <trk>\n        <name>%s</name>        %s\n        %s\n     </trk>\n", trk->name, other, segs);

    free(segs);
    free(other);

    return toString;
}

/*compareTracks Function
 */
int compareTracks(const void *first, const void *second)
{

    if (first == NULL)
    {

        return 0;
    }

    if (second == NULL)
    {

        return 0;
    }

    if (first > second)
    {

        return 1;
    }

    return 2;
}

// MODULE 2
int getNumWaypoints(const GPXdoc *doc)
{

    if (doc == NULL)
    {

        return 0;
    }

    if (doc->waypoints == NULL)
    {

        return 0;
    }

    int numWpts = getLength(doc->waypoints);

    if (numWpts == -1)
    {

        return 0;
    }

    return numWpts;
}

int getNumRoutes(const GPXdoc *doc)
{

    if (doc == NULL)
    {

        return 0;
    }

    if (doc->routes == NULL)
    {

        return 0;
    }

    int numRtes = getLength(doc->routes);

    if (numRtes == -1)
    {

        return 0;
    }

    return numRtes;
}

int getNumTracks(const GPXdoc *doc)
{

    if (doc == NULL)
    {

        return 0;
    }

    if (doc->tracks == NULL)
    {

        return 0;
    }

    int numTrks = getLength(doc->tracks);

    if (numTrks == -1)
    {

        return 0;
    }

    return numTrks;
}

int getNumSegments(const GPXdoc *doc)
{

    if (doc == NULL)
    {

        return 0;
    }

    if (doc->tracks == NULL)
    {

        return 0;
    }

    int numSegs = 0;
    int tmp = 0;

    ListIterator iter = createIterator(doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);

    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (trk->segments != NULL)
        {

            tmp = getLength(trk->segments);

            if (tmp != -1)
            {

                numSegs += tmp;
            }
        }

        el = nextElement(&iter);
    }

    return numSegs;
}

int getNumGPXData(const GPXdoc *doc)
{

    if (doc == NULL)
    {

        return 0;
    }

    int numGpx = 0;
    int tmp = 0;

    // waypoints
    if (doc->waypoints != NULL)
    {

        ListIterator iter = createIterator(doc->waypoints);
        void *el = NULL;
        el = nextElement(&iter);

        while (el != NULL)
        {

            Waypoint *wpt = (Waypoint *)el;

            if (wpt->otherData != NULL)
            {

                tmp = getLength(wpt->otherData);

                if (tmp != -1)
                {

                    numGpx += tmp;
                }
            }

            if (wpt->name != NULL)
            {

                if (strlen(wpt->name) > 0)
                { // has name tag non empty

                    numGpx += 1;
                }
            }

            el = nextElement(&iter);
        }
    }

    // routes
    if (doc->routes != NULL)
    {

        ListIterator iter = createIterator(doc->routes);
        void *el = NULL;
        el = nextElement(&iter);

        while (el != NULL)
        {

            Route *rte = (Route *)el;

            if (rte->name != NULL)
            {

                if (strlen(rte->name) > 0)
                {

                    numGpx += 1;
                }
            }

            if (rte->otherData != NULL)
            {

                tmp = getLength(rte->otherData);

                if (tmp != -1)
                {

                    numGpx += tmp;
                }
            }

            if (rte->waypoints != NULL)
            {

                ListIterator iter2 = createIterator(rte->waypoints);
                void *el2 = NULL;
                el2 = nextElement(&iter2);

                while (el2 != NULL)
                {

                    Waypoint *rteWpt = (Waypoint *)el2;

                    if (rteWpt->name != NULL)
                    {

                        if (strlen(rteWpt->name) > 0)
                        {

                            numGpx += 1;
                        }
                    }

                    if (rteWpt->otherData != NULL)
                    {

                        tmp = getLength(rteWpt->otherData);

                        if (tmp != -1)
                        {

                            numGpx += tmp;
                        }
                    }

                    el2 = nextElement(&iter2);
                }
            }

            el = nextElement(&iter);
        }
    }

    // tracks
    if (doc->tracks != NULL)
    {

        ListIterator iter = createIterator(doc->tracks);
        void *el = NULL;
        el = nextElement(&iter);

        while (el != NULL)
        {

            Track *trk = (Track *)el;

            if (trk->name != NULL)
            {

                if (strlen(trk->name) > 0)
                {

                    numGpx += 1;
                }
            }

            if (trk->otherData != NULL)
            {

                tmp = getLength(trk->otherData);

                if (tmp != -1)
                {

                    numGpx += tmp;
                }
            }

            if (trk->segments != NULL)
            {

                ListIterator iter2 = createIterator(trk->segments);
                void *el2 = NULL;
                el2 = nextElement(&iter2);

                while (el2 != NULL)
                {

                    TrackSegment *trkSeg = (TrackSegment *)el2;

                    if (trkSeg->waypoints != NULL)
                    {

                        ListIterator iter3 = createIterator(trkSeg->waypoints);
                        void *el3 = NULL;
                        el3 = nextElement(&iter3);

                        while (el3 != NULL)
                        {

                            Waypoint *trkPt = (Waypoint *)el3;

                            if (trkPt->name != NULL)
                            {

                                if (strlen(trkPt->name) > 0)
                                {

                                    numGpx += 1;
                                }
                            }

                            if (trkPt->otherData != NULL)
                            {

                                tmp = getLength(trkPt->otherData);

                                if (tmp != -1)
                                {

                                    numGpx += tmp;
                                }
                            }

                            el3 = nextElement(&iter3);
                        }
                    }

                    el2 = nextElement(&iter2);
                }
            }

            el = nextElement(&iter);
        }
    }

    return numGpx;
}

// getWaypoint
Waypoint *getWaypoint(const GPXdoc *doc, char *name)
{

    if (doc == NULL)
    {

        return NULL;
    }

    if (name == NULL)
    {

        return NULL;
    }

    if (doc->waypoints == NULL)
    {

        return NULL;
    }

    ListIterator iter = createIterator(doc->waypoints);
    void *el = NULL;
    el = nextElement(&iter);

    while (el != NULL)
    {

        Waypoint *wpt = (Waypoint *)el;

        if (wpt->name != NULL)
        {

            if (strcmp(name, wpt->name) == 0)
            {

                return wpt;
            }
        }

        el = nextElement(&iter);
    }

    return NULL;
}

// getTrack
Track *getTrack(const GPXdoc *doc, char *name)
{

    if (doc == NULL)
    {

        return NULL;
    }

    if (name == NULL)
    {

        return NULL;
    }

    if (doc->tracks == NULL)
    {

        return NULL;
    }

    ListIterator iter = createIterator(doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);

    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (trk->name != NULL)
        {

            if (strcmp(name, trk->name) == 0)
            {

                return trk;
            }
        }

        el = nextElement(&iter);
    }

    return NULL;
}

// getRoute
// getTrack
Route *getRoute(const GPXdoc *doc, char *name)
{

    if (doc == NULL)
    {

        return NULL;
    }

    if (name == NULL)
    {

        return NULL;
    }

    if (doc->routes == NULL)
    {

        return NULL;
    }

    ListIterator iter = createIterator(doc->routes);
    void *el = NULL;
    el = nextElement(&iter);

    while (el != NULL)
    {

        Route *rte = (Route *)el;

        if (rte->name != NULL)
        {

            if (strcmp(name, rte->name) == 0)
            {

                return rte;
            }
        }

        el = nextElement(&iter);
    }

    return NULL;
}

// Create valid gpx doc function
GPXdoc *createValidGPXdoc(char *fileName, char *gpxSchemaFile)
{

    if (gpxSchemaFile == NULL)
    {

        return NULL;
    }

    if (fileName == NULL)
    {

        return NULL;
    }

    int doNothing()
    {
        return 1;
    }

    //[XML] init
    xmlDoc *file = NULL;
    xmlNode *node = NULL;
    xmlNode *value2 = NULL;
    xmlNode *value3 = NULL;
    xmlNode *nodeI = NULL; // node Iterate
    xmlAttr *attr = NULL;
    xmlNode *value = NULL;
    LIBXML_TEST_VERSION

    // schema validation
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt = NULL;
    xmlLineNumbersDefault(1);
    ctxt = xmlSchemaNewParserCtxt(gpxSchemaFile);
    xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)doNothing, (xmlSchemaValidityWarningFunc)doNothing, NULL);
    schema = xmlSchemaParse(ctxt);
    xmlSchemaFreeParserCtxt(ctxt);

    if (schema == NULL)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
        return NULL;
    }

    file = xmlReadFile(fileName, NULL, 0);
    if (file == NULL)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
        return NULL; // Returns NULL pointer if file provided is invalid and thus cannot be parsed
    }

    // validate
    xmlSchemaValidCtxtPtr xtxt;
    int ret;
    xtxt = xmlSchemaNewValidCtxt(schema);
    xmlSchemaSetValidErrors(xtxt, (xmlSchemaValidityErrorFunc)doNothing, (xmlSchemaValidityWarningFunc)doNothing, NULL);
    ret = xmlSchemaValidateDoc(xtxt, file);

    if (schema != NULL)
    {

        xmlSchemaFree(schema);
    }

    xmlSchemaFreeValidCtxt(xtxt);
    xmlSchemaCleanupTypes();
    xmlMemoryDump();

    if (ret != 0)
    { // invalid gpx

        xmlFreeDoc(file);
        xmlCleanupParser();
        return NULL;
    }

    node = xmlDocGetRootElement(file);

    //[GPXdoc Struct] init
    GPXdoc *doc = malloc(sizeof(GPXdoc)); // MALLOC MUST BE FREED [0]
    if (doc == NULL)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
    }
    doc->version = -33.3; // impossible value to error check later
    doc->creator = NULL;  // null to check if initialized later
    doc->waypoints = NULL;
    doc->routes = NULL;
    doc->tracks = NULL;

    //[XML] Parse Gpx TAG ONLY & error check validity!
    if (strcmp("gpx", (const char *)node->name) != 0)
    {

        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL; // GPX tag must be first for gpx format!
    }

    if (node->ns == NULL)
    { // namespace tag does not exist -> Invalid gpx
        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL;
    }
    strcpy(doc->namespace, (const char *)node->ns->href);

    for (attr = node->properties; attr != NULL; attr = attr->next)
    { // parse gpx attributes

        if (strcmp("version", (const char *)attr->name) == 0)
        { // get version

            value = attr->children;
            doc->version = atof((const char *)value->content);
        }
        else if (strcmp("creator", (const char *)attr->name) == 0)
        { // get creator

            value = attr->children;
            doc->creator = malloc((sizeof(char) * strlen((const char *)value->content)) + (1 * sizeof(char))); // MALLOC MUST BE FREED [0]

            if (doc->creator == NULL)
            { // malloc fail
                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }

            strcpy(doc->creator, (const char *)value->content);
        }
    }

    if (doc->creator == NULL || strlen((const char *)doc->creator) == 0 || doc->version == -33.3 || strlen((const char *)doc->namespace) == 0)
    { // error check gpxdoc values

        xmlFreeDoc(file);
        xmlCleanupParser();
        deleteGPXdoc(doc);
        return NULL;
    }

    // get children of gpx tag (all other tags)
    if (node->children != NULL)
    {

        node = node->children;
    }

    // Node [init]
    Waypoint *wpt = NULL;
    GPXData *data = NULL;
    Route *rte = NULL;
    Track *trk = NULL;
    TrackSegment *trkSeg = NULL;

    // Lists [init]
    List *wptExtra = NULL; // extra wpt data
    List *rteExtra = NULL; // extra rte data
    List *rteWpts = NULL;  // list for points of route
    List *trkExtra = NULL;
    List *trkSegs = NULL;
    List *trkWpts = NULL;

    List *waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints); // must be freed
    doc->waypoints = waypoints;
    List *routes = initializeList(&routeToString, &deleteRoute, &compareRoutes); // must be freed
    doc->routes = routes;
    List *tracks = initializeList(&trackToString, &deleteTrack, &compareTracks); // must be freed
    doc->tracks = tracks;

    /*Begin parsing of data other than gpx type
     *waypoints [x]
     *routes    [x]
     *tracks    []
     */
    for (nodeI = node; nodeI != NULL; nodeI = nodeI->next)
    {

        // Resets
        attr = NULL;
        value = NULL;
        value2 = NULL;
        value3 = NULL;
        wpt = NULL;
        rte = NULL;
        data = NULL;
        wptExtra = NULL;
        rteExtra = NULL;
        rteWpts = NULL;
        trkExtra = NULL;
        trkSegs = NULL;
        trk = NULL;
        trkSeg = NULL;
        trkWpts = NULL;

        /*Extract <wpt> data
         *covers all tags listed at gpx namespace
         *lat & lon attributes
         *all other tags are children*/
        if (strcmp((const char *)nodeI->name, "wpt") == 0)
        { // tag found

            wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
            wpt->otherData = NULL;
            wpt->name = NULL;
            if (wpt == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            wpt->latitude = -999.999;  // impossible values
            wpt->longitude = -999.999; // impossible values

            for (attr = nodeI->properties; attr != NULL; attr = attr->next)
            { // get lon/lat

                if (strcmp("lat", (const char *)attr->name) == 0)
                {

                    value = attr->children;
                    wpt->latitude = atof((const char *)value->content);
                }
                else if (strcmp("lon", (const char *)attr->name) == 0)
                {

                    value = attr->children;
                    wpt->longitude = atof((const char *)value->content);
                }
            }

            if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
            { // invalid file

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteWaypoint(wpt);
                deleteGPXdoc(doc);
                return NULL;
            }

            // init extra data list
            wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            wpt->otherData = wptExtra;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && wpt->name == NULL)
                    {

                        wpt->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));

                        if (wpt->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(wpt->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "ele") == 0 || strcmp((const char *)value->name, "time") == 0 || strcmp((const char *)value->name, "magvar") == 0 || strcmp((const char *)value->name, "geoidheight") == 0 || strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "sym") == 0 || strcmp((const char *)value->name, "type") == 0 || strcmp((const char *)value->name, "fix") == 0 || strcmp((const char *)value->name, "sat") == 0 || strcmp((const char *)value->name, "hdop") == 0 || strcmp((const char *)value->name, "vdop") == 0 || strcmp((const char *)value->name, "pdop") == 0 || strcmp((const char *)value->name, "ageofdgpsdata") == 0 || strcmp((const char *)value->name, "dgpsid") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0...1] tags ?
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(wpt->otherData, (void *)data);
                    }
                }
            }

            if (wpt->name == NULL)
            {

                wpt->name = malloc(sizeof(char) * 2);
                if (wpt->name == NULL)
                {

                    xmlFreeDoc(file);
                    xmlCleanupParser();
                    deleteWaypoint(wpt);
                    deleteGPXdoc(doc);
                    return NULL;
                }
                strcpy(wpt->name, "");
            }

            insertBack(waypoints, wpt);
        }
        else if (strcmp((const char *)nodeI->name, "rte") == 0)
        {

            rte = malloc(sizeof(Route));
            if (rte == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            rte->name = NULL;
            rteExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            rte->otherData = rteExtra;
            rteWpts = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
            rte->waypoints = rteWpts;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && rte->name == NULL)
                    {

                        rte->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));
                        if (rte->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(rte->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "number") == 0 || strcmp((const char *)value->name, "type") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0-1]
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(rteExtra, (void *)data);
                    }
                }

                if (strcmp((const char *)value->name, "rtept") == 0)
                { // Type waypoint !

                    // wpt = NULL;
                    wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
                    if (wpt == NULL)
                    {

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteRoute(rte);
                        deleteGPXdoc(doc);
                        return NULL;
                    }
                    wpt->latitude = -999.999;  // impossible value2s
                    wpt->longitude = -999.999; // impossible value2s
                    wpt->name = NULL;
                    wpt->otherData = NULL;

                    for (attr = value->properties; attr != NULL; attr = attr->next)
                    { // get lon/lat

                        if (strcmp("lat", (const char *)attr->name) == 0)
                        {

                            value2 = attr->children;
                            wpt->latitude = atof((const char *)value2->content);
                        }
                        else if (strcmp("lon", (const char *)attr->name) == 0)
                        {

                            value2 = attr->children;
                            wpt->longitude = atof((const char *)value2->content);
                        }
                    }

                    if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
                    { // invalid file

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteWaypoint(wpt);
                        deleteRoute(rte);
                        deleteGPXdoc(doc);
                        return NULL;
                    }

                    // init extra data list
                    wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
                    wpt->otherData = wptExtra;

                    for (value2 = value->children; value2 != NULL; value2 = value2->next)
                    {

                        if (value2->children != NULL)
                        {

                            if (strcmp((const char *)value2->name, "name") == 0 && wpt->name == NULL)
                            {

                                wpt->name = malloc(sizeof(char) * strlen((const char *)value2->children->content) + (1 * sizeof(char)));
                                if (wpt->name == NULL)
                                {

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteRoute(rte);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }
                                strcpy(wpt->name, (const char *)value2->children->content);
                            }
                            else if (strcmp((const char *)value2->name, "ele") == 0 || strcmp((const char *)value2->name, "time") == 0 || strcmp((const char *)value2->name, "magvar") == 0 || strcmp((const char *)value2->name, "geoidheight") == 0 || strcmp((const char *)value2->name, "cmt") == 0 || strcmp((const char *)value2->name, "desc") == 0 || strcmp((const char *)value2->name, "src") == 0 || strcmp((const char *)value2->name, "sym") == 0 || strcmp((const char *)value2->name, "type") == 0 || strcmp((const char *)value2->name, "fix") == 0 || strcmp((const char *)value2->name, "sat") == 0 || strcmp((const char *)value2->name, "hdop") == 0 || strcmp((const char *)value2->name, "vdop") == 0 || strcmp((const char *)value2->name, "pdop") == 0 || strcmp((const char *)value2->name, "ageofdgpsdata") == 0 || strcmp((const char *)value2->name, "dgpsid") == 0)
                            {

                                if (value2->name == NULL || strlen((const char *)value2->name) == 0 || value2->children->content == NULL || strlen((const char *)value2->children->content) == 0)
                                { // check empty strings

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteRoute(rte);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }

                                data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value2->children->content)) + (sizeof(char) * 1));
                                strcpy(data->name, (const char *)value2->name); // consider [0-1]
                                strcpy(data->value, (const char *)value2->children->content);

                                insertBack(wptExtra, (void *)data);
                            }
                        }
                    }

                    if (wpt->name == NULL)
                    {

                        wpt->name = malloc(sizeof(char) * 2);
                        if (wpt->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteWaypoint(wpt);
                            deleteRoute(rte);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(wpt->name, "");
                    }

                    insertBack(rte->waypoints, wpt);
                }
            }

            if (rte->name == NULL)
            {

                rte->name = malloc(sizeof(char) * 2);
                if (rte->name == NULL)
                {

                    xmlFreeDoc(file);
                    xmlCleanupParser();
                    deleteRoute(rte);
                    deleteGPXdoc(doc);
                    return NULL;
                }
                strcpy(rte->name, "");
            }

            insertBack(routes, rte);
        }
        else if (strcmp((const char *)nodeI->name, "trk") == 0)
        { // track

            trk = malloc(sizeof(Track));
            if (trk == NULL)
            {

                xmlFreeDoc(file);
                xmlCleanupParser();
                deleteGPXdoc(doc);
                return NULL;
            }
            trk->name = NULL;
            trkExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
            trk->otherData = trkExtra;
            trkSegs = initializeList(&trackSegmentToString, &deleteTrackSegment, &compareTrackSegments);
            trk->segments = trkSegs;

            for (value = nodeI->children; value != NULL; value = value->next)
            {

                if (value->children != NULL)
                {

                    if (strcmp((const char *)value->name, "name") == 0 && trk->name == NULL)
                    {

                        trk->name = malloc((sizeof(char) * strlen((const char *)value->children->content)) + (1 * sizeof(char)));
                        if (trk->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(trk->name, (const char *)value->children->content);
                    }
                    else if (strcmp((const char *)value->name, "cmt") == 0 || strcmp((const char *)value->name, "desc") == 0 || strcmp((const char *)value->name, "src") == 0 || strcmp((const char *)value->name, "number") == 0 || strcmp((const char *)value->name, "type") == 0)
                    {

                        if (value->name == NULL || strlen((const char *)value->name) == 0 || value->children->content == NULL || strlen((const char *)value->children->content) == 0)
                        { // check empty strings

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }

                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value->children->content)) + (sizeof(char) * 1));
                        if (data == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(data->name, (const char *)value->name); // consider [0-1]
                        strcpy(data->value, (const char *)value->children->content);

                        insertBack(trkExtra, (void *)data);
                    }

                    if (trk->name == NULL)
                    {

                        trk->name = malloc(sizeof(char) * 2);
                        if (trk->name == NULL)
                        {

                            xmlFreeDoc(file);
                            xmlCleanupParser();
                            deleteTrack(trk);
                            deleteGPXdoc(doc);
                            return NULL;
                        }
                        strcpy(trk->name, "");
                    }
                }

                if (strcmp((const char *)value->name, "trkseg") == 0)
                {

                    trkSeg = malloc(sizeof(TrackSegment));
                    if (trkSeg == NULL)
                    {

                        xmlFreeDoc(file);
                        xmlCleanupParser();
                        deleteTrack(trk);
                        deleteGPXdoc(doc);
                        return NULL;
                    }
                    trkWpts = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
                    trkSeg->waypoints = trkWpts;

                    for (value2 = value->children; value2 != NULL; value2 = value2->next)
                    {

                        if (strcmp((const char *)value2->name, "trkpt") == 0)
                        {

                            wpt = malloc(sizeof(Waypoint)); // malloc must be freed.
                            if (wpt == NULL)
                            {

                                xmlFreeDoc(file);
                                xmlCleanupParser();
                                deleteTrackSegment(trkSeg);
                                deleteTrack(trk);
                                deleteGPXdoc(doc);
                                return NULL;
                            }
                            wpt->latitude = -999.999;  // impossible value2s
                            wpt->longitude = -999.999; // impossible value2s
                            wpt->name = NULL;
                            wpt->otherData = NULL;

                            for (attr = value2->properties; attr != NULL; attr = attr->next)
                            { // get lon/lat

                                if (strcmp("lat", (const char *)attr->name) == 0)
                                {

                                    value3 = attr->children;
                                    wpt->latitude = atof((const char *)value3->content);
                                }
                                else if (strcmp("lon", (const char *)attr->name) == 0)
                                {

                                    value3 = attr->children;
                                    wpt->longitude = atof((const char *)value3->content);
                                }
                            }

                            if (wpt->latitude == -999.999 || wpt->longitude == -999.999)
                            { // invalid file

                                xmlFreeDoc(file);
                                xmlCleanupParser();
                                deleteWaypoint(wpt);
                                deleteTrackSegment(trkSeg);
                                deleteTrack(trk);
                                deleteGPXdoc(doc);
                                return NULL;
                            }

                            // init extra data list
                            wptExtra = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);
                            wpt->otherData = wptExtra;

                            // wpt extra
                            for (value3 = value2->children; value3 != NULL; value3 = value3->next)
                            {

                                if (value3->children != NULL)
                                {

                                    if (strcmp((const char *)value3->name, "name") == 0 && wpt->name == NULL)
                                    {

                                        wpt->name = malloc(sizeof(char) * strlen((const char *)value3->children->content) + (1 * sizeof(char)));
                                        if (wpt->name == NULL)
                                        {

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }
                                        strcpy(wpt->name, (const char *)value3->children->content);
                                    }
                                    else if (strcmp((const char *)value3->name, "ele") == 0 || strcmp((const char *)value3->name, "time") == 0 || strcmp((const char *)value3->name, "magvar") == 0 || strcmp((const char *)value3->name, "geoidheight") == 0 || strcmp((const char *)value3->name, "cmt") == 0 || strcmp((const char *)value3->name, "desc") == 0 || strcmp((const char *)value3->name, "src") == 0 || strcmp((const char *)value3->name, "sym") == 0 || strcmp((const char *)value3->name, "type") == 0 || strcmp((const char *)value3->name, "fix") == 0 || strcmp((const char *)value3->name, "sat") == 0 || strcmp((const char *)value3->name, "hdop") == 0 || strcmp((const char *)value3->name, "vdop") == 0 || strcmp((const char *)value3->name, "pdop") == 0 || strcmp((const char *)value3->name, "ageofdgpsdata") == 0 || strcmp((const char *)value3->name, "dgpsid") == 0)
                                    {

                                        if (value3->name == NULL || strlen((const char *)value3->name) == 0 || value3->children->content == NULL || strlen((const char *)value3->children->content) == 0)
                                        { // check empty strings

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }

                                        data = malloc(sizeof(GPXData) + (sizeof(char) * strlen((const char *)value3->children->content)) + (sizeof(char) * 1));
                                        if (data == NULL)
                                        {

                                            xmlFreeDoc(file);
                                            xmlCleanupParser();
                                            deleteWaypoint(wpt);
                                            deleteTrackSegment(trkSeg);
                                            deleteTrack(trk);
                                            deleteGPXdoc(doc);
                                            return NULL;
                                        }
                                        strcpy(data->name, (const char *)value3->name); // consider [0-1]
                                        strcpy(data->value, (const char *)value3->children->content);

                                        insertBack(wptExtra, (void *)data);
                                    }
                                }
                            }

                            if (wpt->name == NULL)
                            {

                                wpt->name = malloc(sizeof(char) * 2);
                                if (wpt->name == NULL)
                                {

                                    xmlFreeDoc(file);
                                    xmlCleanupParser();
                                    deleteWaypoint(wpt);
                                    deleteTrackSegment(trkSeg);
                                    deleteTrack(trk);
                                    deleteGPXdoc(doc);
                                    return NULL;
                                }
                                strcpy(wpt->name, "");
                            }

                            insertBack(trkSeg->waypoints, wpt);
                        }
                    }

                    insertBack(trk->segments, trkSeg);
                }
            }

            insertBack(tracks, trk);
        }
    }

    //[XML] free parser & file
    xmlFreeDoc(file);
    xmlCleanupParser();

    return doc;
}

// writegpxdoc function
bool writeGPXdoc(GPXdoc *doc, char *fileName)
{

    if (doc == NULL)
    {

        return false;
    }

    if (fileName == NULL)
    {

        return false;
    }

    xmlDocPtr outputDoc = NULL;
    xmlNodePtr root, node1, node2, node3 = NULL;
    int i = 0;
    char buf[256];

    LIBXML_TEST_VERSION;

    // begin tree creation
    snprintf(buf, 50, "%f", doc->version);
    bool passed = false;
    for (i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == '.')
        {
            passed = true;
        }
        if (passed == true && buf[i] == '0')
        {

            buf[i] = '\0';
            break;
        }
    }

    // setup xml root
    outputDoc = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST "gpx");
    // xmlNewProp(root, BAD_CAST "xmlns", BAD_CAST doc->namespace);
    xmlNsPtr ns = xmlNewNs(root, BAD_CAST doc->namespace, NULL);
    xmlSetNs(root, ns);
    xmlNewProp(root, BAD_CAST "version", BAD_CAST buf);
    xmlNewProp(root, BAD_CAST "creator", BAD_CAST doc->creator);
    xmlDocSetRootElement(outputDoc, root);

    // set child gpx elements
    // wpts
    ListIterator iter = createIterator(doc->waypoints);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Waypoint *wpt = (Waypoint *)el;

        node1 = xmlNewChild(root, NULL, BAD_CAST "wpt", NULL);
        snprintf(buf, 50, "%f", wpt->latitude);
        xmlNewProp(node1, BAD_CAST "lat", BAD_CAST buf);
        snprintf(buf, 50, "%f", wpt->longitude);
        xmlNewProp(node1, BAD_CAST "lon", BAD_CAST buf);

        if (strlen(wpt->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST wpt->name);
        }

        // get gpx data stuff
        ListIterator iter2 = createIterator(wpt->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // next wpt
        el = nextElement(&iter);
    }

    // routes
    iter = createIterator(doc->routes);
    el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        node1 = xmlNewChild(root, NULL, BAD_CAST "rte", NULL);

        if (strlen(rte->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST rte->name);
        }

        // other data list
        ListIterator iter2 = createIterator(rte->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // wpts
        ListIterator iter3 = createIterator(rte->waypoints);
        void *wpts = NULL;
        wpts = nextElement(&iter3);
        while (wpts != NULL)
        {

            Waypoint *wpt = (Waypoint *)wpts;

            node2 = xmlNewChild(node1, NULL, BAD_CAST "rtept", NULL);
            snprintf(buf, 50, "%f", wpt->latitude);
            xmlNewProp(node2, BAD_CAST "lat", BAD_CAST buf);
            snprintf(buf, 50, "%f", wpt->longitude);
            xmlNewProp(node2, BAD_CAST "lon", BAD_CAST buf);

            if (strlen(wpt->name) > 0)
            {
                xmlNewChild(node2, NULL, BAD_CAST "name", BAD_CAST wpt->name);
            }

            ListIterator iter4 = createIterator(wpt->otherData);
            void *data = NULL;
            data = nextElement(&iter4);
            while (data != NULL)
            {

                GPXData *gpxData = (GPXData *)data;

                xmlNewChild(node2, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

                data = nextElement(&iter4);
            }

            wpts = nextElement(&iter3);
        }

        el = nextElement(&iter);
    }

    // tracks
    iter = createIterator(doc->tracks);
    el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        node1 = xmlNewChild(root, NULL, BAD_CAST "trk", NULL);

        if (strlen(trk->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST trk->name);
        }

        // gpx data
        ListIterator iter2 = createIterator(trk->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // segments
        ListIterator iter3 = createIterator(trk->segments);
        void *segments = NULL;
        segments = nextElement(&iter3);
        while (segments != NULL)
        {

            TrackSegment *seg = (TrackSegment *)segments;

            node2 = xmlNewChild(node1, NULL, BAD_CAST "trkseg", NULL);

            // wpts
            ListIterator iter4 = createIterator(seg->waypoints);
            void *wpts = NULL;
            wpts = nextElement(&iter4);
            while (wpts != NULL)
            {

                Waypoint *wpt = (Waypoint *)wpts;

                node3 = xmlNewChild(node2, NULL, BAD_CAST "trkpt", NULL);
                snprintf(buf, 50, "%f", wpt->latitude);
                xmlNewProp(node3, BAD_CAST "lat", BAD_CAST buf);
                snprintf(buf, 50, "%f", wpt->longitude);
                xmlNewProp(node3, BAD_CAST "lon", BAD_CAST buf);

                if (strlen(wpt->name) > 0)
                {
                    xmlNewChild(node3, NULL, BAD_CAST "name", BAD_CAST wpt->name);
                }

                ListIterator iter5 = createIterator(wpt->otherData);
                void *xdata = NULL;
                xdata = nextElement(&iter5);
                while (xdata != NULL)
                {

                    GPXData *xtra = (GPXData *)xdata;

                    xmlNewChild(node3, NULL, BAD_CAST xtra->name, BAD_CAST xtra->value);

                    xdata = nextElement(&iter5);
                }

                wpts = nextElement(&iter4);
            }

            segments = nextElement(&iter3);
        }

        el = nextElement(&iter);
    }

    if (xmlSaveFormatFileEnc(fileName, outputDoc, "UTF-8", 1) == -1)
    {

        xmlFreeDoc(outputDoc);
        xmlCleanupParser();
        xmlMemoryDump();
        return false;
    }

    xmlFreeDoc(outputDoc);
    xmlCleanupParser();
    xmlMemoryDump();

    return true;
}

// validateGpxDoc function
bool validateGPXDoc(GPXdoc *gpxDoc, char *gpxSchemaFile)
{

    if (gpxDoc == NULL)
    {

        return false;
    }

    if (gpxSchemaFile == NULL)
    {

        return false;
    }

    xmlDocPtr outputDoc = NULL;
    xmlNodePtr root, node1, node2, node3 = NULL;
    int i = 0;
    char buf[256];

    LIBXML_TEST_VERSION;

    // begin tree creation
    // check valid gpxdoc struct
    if (gpxDoc->version < 0 || strlen(gpxDoc->namespace) == 0 || gpxDoc->creator == NULL || gpxDoc->waypoints == NULL || gpxDoc->routes == NULL || gpxDoc->tracks == NULL)
    {

        return false;
    }
    snprintf(buf, 50, "%f", gpxDoc->version);
    bool passed = false;
    for (i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == '.')
        {
            passed = true;
        }
        if (passed == true && buf[i] == '0')
        {

            buf[i] = '\0';
            break;
        }
    }

    // setup xml root
    outputDoc = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST "gpx");
    // xmlNewProp(root, BAD_CAST "xmlns", BAD_CAST doc->namespace);
    xmlNewProp(root, BAD_CAST "version", BAD_CAST buf);
    xmlNewProp(root, BAD_CAST "creator", BAD_CAST gpxDoc->creator);
    xmlNsPtr ns = xmlNewNs(root, BAD_CAST gpxDoc->namespace, NULL);
    xmlSetNs(root, ns);
    xmlDocSetRootElement(outputDoc, root);

    // set child gpx elements
    // wpts
    ListIterator iter = createIterator(gpxDoc->waypoints);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Waypoint *wpt = (Waypoint *)el;

        if (wpt->latitude < -90 || wpt->latitude > 90 || wpt->longitude < -180 || wpt->longitude > 180 || wpt->name == NULL)
        {

            xmlFreeDoc(outputDoc);
            xmlCleanupParser();
            xmlMemoryDump();
            return false;
        }

        node1 = xmlNewChild(root, NULL, BAD_CAST "wpt", NULL);
        snprintf(buf, 50, "%f", wpt->latitude);
        xmlNewProp(node1, BAD_CAST "lat", BAD_CAST buf);
        snprintf(buf, 50, "%f", wpt->longitude);
        xmlNewProp(node1, BAD_CAST "lon", BAD_CAST buf);

        if (strlen(wpt->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST wpt->name);
        }

        if (wpt->otherData == NULL)
        {

            xmlFreeDoc(outputDoc);
            xmlCleanupParser();
            xmlMemoryDump();
            return false;
        }

        // get gpx data stuff
        ListIterator iter2 = createIterator(wpt->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            if (strlen(gpxData->name) == 0 || strlen(gpxData->value) == 0)
            {

                xmlFreeDoc(outputDoc);
                xmlCleanupParser();
                xmlMemoryDump();
                return false;
            }

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // next wpt
        el = nextElement(&iter);
    }

    // routes
    iter = createIterator(gpxDoc->routes);
    el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        if (rte->name == NULL || rte->waypoints == NULL || rte->otherData == NULL)
        {

            xmlFreeDoc(outputDoc);
            xmlCleanupParser();
            xmlMemoryDump();
            return false;
        }

        node1 = xmlNewChild(root, NULL, BAD_CAST "rte", NULL);

        if (strlen(rte->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST rte->name);
        }

        // other data list
        ListIterator iter2 = createIterator(rte->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            if (strlen(gpxData->value) == 0 || strlen(gpxData->name) == 0)
            {

                xmlFreeDoc(outputDoc);
                xmlCleanupParser();
                xmlMemoryDump();
                return false;
            }

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // wpts
        ListIterator iter3 = createIterator(rte->waypoints);
        void *wpts = NULL;
        wpts = nextElement(&iter3);
        while (wpts != NULL)
        {

            Waypoint *wpt = (Waypoint *)wpts;

            if (wpt->latitude < -90 || wpt->latitude > 90 || wpt->longitude < -180 || wpt->longitude > 180 || wpt->name == NULL || wpt->otherData == NULL)
            {

                xmlFreeDoc(outputDoc);
                xmlCleanupParser();
                xmlMemoryDump();
                return false;
            }

            node2 = xmlNewChild(node1, NULL, BAD_CAST "rtept", NULL);
            snprintf(buf, 50, "%f", wpt->latitude);
            xmlNewProp(node2, BAD_CAST "lat", BAD_CAST buf);
            snprintf(buf, 50, "%f", wpt->longitude);
            xmlNewProp(node2, BAD_CAST "lon", BAD_CAST buf);

            if (strlen(wpt->name) > 0)
            {
                xmlNewChild(node2, NULL, BAD_CAST "name", BAD_CAST wpt->name);
            }

            ListIterator iter4 = createIterator(wpt->otherData);
            void *data = NULL;
            data = nextElement(&iter4);
            while (data != NULL)
            {

                GPXData *gpxData = (GPXData *)data;

                if (strlen(gpxData->value) == 0 || strlen(gpxData->name) == 0)
                {

                    xmlFreeDoc(outputDoc);
                    xmlCleanupParser();
                    xmlMemoryDump();
                    return false;
                }

                xmlNewChild(node2, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

                data = nextElement(&iter4);
            }

            wpts = nextElement(&iter3);
        }

        el = nextElement(&iter);
    }

    // tracks
    iter = createIterator(gpxDoc->tracks);
    el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (trk->name == NULL || trk->segments == NULL || trk->otherData == NULL)
        {

            xmlFreeDoc(outputDoc);
            xmlCleanupParser();
            xmlMemoryDump();
            return false;
        }

        node1 = xmlNewChild(root, NULL, BAD_CAST "trk", NULL);

        if (strlen(trk->name) > 0)
        {
            xmlNewChild(node1, NULL, BAD_CAST "name", BAD_CAST trk->name);
        }

        // gpx data
        ListIterator iter2 = createIterator(trk->otherData);
        void *data = NULL;
        data = nextElement(&iter2);
        while (data != NULL)
        {

            GPXData *gpxData = (GPXData *)data;

            if (strlen(gpxData->name) == 0 || strlen(gpxData->value) == 0)
            {

                xmlFreeDoc(outputDoc);
                xmlCleanupParser();
                xmlMemoryDump();
                return false;
            }

            xmlNewChild(node1, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);

            data = nextElement(&iter2);
        }

        // segments
        ListIterator iter3 = createIterator(trk->segments);
        void *segments = NULL;
        segments = nextElement(&iter3);
        while (segments != NULL)
        {

            TrackSegment *seg = (TrackSegment *)segments;

            node2 = xmlNewChild(node1, NULL, BAD_CAST "trkseg", NULL);

            if (seg->waypoints == NULL)
            {

                xmlFreeDoc(outputDoc);
                xmlCleanupParser();
                xmlMemoryDump();
                return false;
            }

            // wpts
            ListIterator iter4 = createIterator(seg->waypoints);
            void *wpts = NULL;
            wpts = nextElement(&iter4);
            while (wpts != NULL)
            {

                Waypoint *wpt = (Waypoint *)wpts;

                if (wpt->latitude < -90 || wpt->latitude > 90 || wpt->longitude < -180 || wpt->longitude > 180 || wpt->name == NULL || wpt->otherData == NULL)
                {

                    xmlFreeDoc(outputDoc);
                    xmlCleanupParser();
                    xmlMemoryDump();
                    return false;
                }

                node3 = xmlNewChild(node2, NULL, BAD_CAST "trkpt", NULL);
                snprintf(buf, 50, "%f", wpt->latitude);
                xmlNewProp(node3, BAD_CAST "lat", BAD_CAST buf);
                snprintf(buf, 50, "%f", wpt->longitude);
                xmlNewProp(node3, BAD_CAST "lon", BAD_CAST buf);

                if (strlen(wpt->name) > 0)
                {
                    xmlNewChild(node3, NULL, BAD_CAST "name", BAD_CAST wpt->name);
                }

                ListIterator iter5 = createIterator(wpt->otherData);
                void *xdata = NULL;
                xdata = nextElement(&iter5);
                while (xdata != NULL)
                {

                    GPXData *xtra = (GPXData *)xdata;

                    if (strlen(xtra->name) == 0 || strlen(xtra->value) == 0)
                    {

                        xmlFreeDoc(outputDoc);
                        xmlCleanupParser();
                        xmlMemoryDump();
                        return false;
                    }

                    xmlNewChild(node3, NULL, BAD_CAST xtra->name, BAD_CAST xtra->value);

                    xdata = nextElement(&iter5);
                }

                wpts = nextElement(&iter4);
            }

            segments = nextElement(&iter3);
        }

        el = nextElement(&iter);
    }

    // validate tree
    // schema validation

    int doNothing()
    {
        return 1;
    }
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt = NULL;
    xmlLineNumbersDefault(1);
    ctxt = xmlSchemaNewParserCtxt(gpxSchemaFile);
    xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)doNothing, (xmlSchemaValidityWarningFunc)doNothing, NULL);
    schema = xmlSchemaParse(ctxt);
    xmlSchemaFreeParserCtxt(ctxt);

    if (schema == NULL)
    {

        xmlFreeDoc(outputDoc);
        xmlCleanupParser();
        xmlMemoryDump();
        return false;
    }

    if (outputDoc == NULL)
    {

        xmlFreeDoc(outputDoc);
        xmlCleanupParser();
        xmlMemoryDump();
        return false; // Returns NULL pointer if file provided is invalid and thus cannot be parsed
    }

    // validate
    xmlSchemaValidCtxtPtr xtxt;
    int ret;
    xtxt = xmlSchemaNewValidCtxt(schema);
    xmlSchemaSetValidErrors(xtxt, (xmlSchemaValidityErrorFunc)doNothing, (xmlSchemaValidityWarningFunc)doNothing, NULL);
    ret = xmlSchemaValidateDoc(xtxt, outputDoc);

    if (schema != NULL)
    {

        xmlSchemaFree(schema);
    }

    xmlSchemaFreeValidCtxt(xtxt);
    xmlSchemaCleanupTypes();

    if (ret != 0)
    { // invalid gpx

        xmlFreeDoc(outputDoc);
        xmlCleanupParser();
        xmlMemoryDump();
        return false;
    }

    xmlFreeDoc(outputDoc);
    xmlCleanupParser();
    xmlMemoryDump();
    return true;
}

float round10(float len)
{

    char buf[256];
    int i = 0;
    snprintf(buf, 50, "%f", len);

    for (i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == '.')
        {
            buf[i] = '\0';
            break;
        }
    }

    i--;

    long round = (long)strtol(buf, (char **)NULL, 10);

    if (buf[i] == '0')
    {

        return round;
    }

    if (buf[i] == '1')
    {

        round -= 1;
    }
    else if (buf[i] == '2')
    {

        round -= 2;
    }
    else if (buf[i] == '3')
    {

        round -= 3;
    }
    else if (buf[i] == '4')
    {

        round -= 4;
    }
    else if (buf[i] == '5')
    {

        round += 5;
    }
    else if (buf[i] == '6')
    {

        round += 4;
    }
    else if (buf[i] == '7')
    {

        round += 3;
    }
    else if (buf[i] == '8')
    {

        round += 2;
    }
    else if (buf[i] == '9')
    {

        round += 1;
    }

    return 1.0 * round;
}

float getRouteLen(const Route *rt)
{

    if (rt == NULL)
    {

        return 0.0;
    }

    if (rt->waypoints == NULL)
    {

        return 0.0;
    }

    Waypoint *temp = NULL;

    ListIterator iter = createIterator(rt->waypoints);
    void *el = NULL;
    el = nextElement(&iter);
    float harvesine = 0;
    while (el != NULL)
    {

        Waypoint *curr = (Waypoint *)el;

        if (temp == NULL)
        {

            temp = curr;
            el = nextElement(&iter);
            continue;
        }

        float lat = (curr->latitude - temp->latitude) * (M_PI / 180.0);
        float lon = (curr->longitude - temp->longitude) * (M_PI / 180.0);

        harvesine += 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(temp->latitude * M_PI / 180.0) * cos(curr->latitude * M_PI / 180.0))));

        temp = curr;

        el = nextElement(&iter);
    }

    return harvesine;
}

float getTrackLen(const Track *tr)
{

    if (tr == NULL)
    {

        return 0.0;
    }
    if (tr->segments == NULL)
    {

        return 0.0;
    }

    float harvesine = 0;

    TrackSegment *SegLast = NULL;

    ListIterator iter = createIterator(tr->segments);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        TrackSegment *currSeg = (TrackSegment *)el;

        if (currSeg->waypoints == NULL)
        {

            return 0.0;
        }

        Waypoint *tempWpt = NULL;

        ListIterator iter2 = createIterator(currSeg->waypoints);
        void *el2 = NULL;
        el2 = nextElement(&iter2);
        while (el2 != NULL)
        {

            Waypoint *currWpt = (Waypoint *)el2;

            if (tempWpt == NULL)
            {

                tempWpt = currWpt;
                el2 = nextElement(&iter2);
                continue;
            }

            float lat = (currWpt->latitude - tempWpt->latitude) * (M_PI / 180.0);
            float lon = (currWpt->longitude - tempWpt->longitude) * (M_PI / 180.0);

            harvesine += 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(tempWpt->latitude * M_PI / 180.0) * cos(currWpt->latitude * M_PI / 180.0))));

            tempWpt = currWpt;

            el2 = nextElement(&iter2);
        }

        if (SegLast == NULL)
        {

            SegLast = currSeg;
            el = nextElement(&iter);
            continue;
        }

        Waypoint *wpt1 = (Waypoint *)getFromBack(SegLast->waypoints);
        Waypoint *wpt2 = (Waypoint *)getFromFront(currSeg->waypoints);

        float lat = (wpt2->latitude - wpt1->latitude) * (M_PI / 180.0);
        float lon = (wpt2->longitude - wpt1->longitude) * (M_PI / 180.0);

        harvesine += 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(wpt1->latitude * M_PI / 180.0) * cos(wpt2->latitude * M_PI / 180.0))));

        SegLast = currSeg;

        el = nextElement(&iter);
    }

    return harvesine;
}

// numRoutesWithlength
int numRoutesWithLength(const GPXdoc *doc, float len, float delta)
{

    if (doc == NULL)
    {

        return 0;
    }
    if (len < 0.0 || delta < 0.0)
    {

        return 0;
    }
    if (doc->routes == NULL)
    {

        return 0;
    }

    int numRoutes = 0;

    ListIterator iter = createIterator(doc->routes);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        float comparLen = getRouteLen(rte);

        if (comparLen == 0.0)
        {

            el = nextElement(&iter);
            continue;
        }

        float diff = abs(comparLen - len);

        if (diff <= delta)
        {

            numRoutes++;
        }

        el = nextElement(&iter);
    }

    return numRoutes;
}

// numtrackswithlen
int numTracksWithLength(const GPXdoc *doc, float len, float delta)
{

    if (doc == NULL)
    {

        return 0;
    }
    if (len < 0.0 || delta < 0.0)
    {

        return 0;
    }
    if (doc->tracks == NULL)
    {

        return 0;
    }

    int numTracks = 0;

    ListIterator iter = createIterator(doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        float comparLen = getTrackLen(trk);

        if (comparLen == 0.0)
        {

            el = nextElement(&iter);
            continue;
        }

        float diff = abs(comparLen - len);

        if (diff <= delta)
        {

            numTracks++;
        }

        el = nextElement(&iter);
    }

    return numTracks;
}

// islooproute
bool isLoopRoute(const Route *rt, float delta)
{

    if (rt == NULL)
    {

        return false;
    }
    if (delta < 0.0)
    {

        return false;
    }
    if (rt->waypoints == NULL)
    {

        return false;
    }

    if (getLength(rt->waypoints) < 4)
    {

        return false;
    }

    Waypoint *first = (Waypoint *)getFromFront(rt->waypoints);
    Waypoint *last = (Waypoint *)getFromBack(rt->waypoints);

    float lat = (last->latitude - first->latitude) * (M_PI / 180.0);
    float lon = (last->longitude - first->longitude) * (M_PI / 180.0);

    float dist = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(last->latitude * M_PI / 180.0))));

    if (dist <= delta)
    {

        return true;
    }

    return false;
}

// isLooptrack func
bool isLoopTrack(const Track *tr, float delta)
{

    if (tr == NULL)
    {

        return false;
    }
    if (delta < 0.0)
    {

        return false;
    }
    if (tr->segments == NULL)
    {

        return 0;
    }

    int totalWpt = 0;

    if (getLength(tr->segments) == 0)
    {

        return false;
    }

    ListIterator iter = createIterator(tr->segments);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        TrackSegment *seg = (TrackSegment *)el;

        if (seg->waypoints == NULL)
        {

            return false;
        }

        totalWpt += getLength(seg->waypoints);

        el = nextElement(&iter);
    }

    if (totalWpt < 4)
    {

        return false;
    }

    Waypoint *first = getFromFront(((TrackSegment *)getFromFront(tr->segments))->waypoints);
    Waypoint *last = getFromBack(((TrackSegment *)getFromBack(tr->segments))->waypoints);

    float lat = (last->latitude - first->latitude) * (M_PI / 180.0);
    float lon = (last->longitude - first->longitude) * (M_PI / 180.0);

    float dist = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(last->latitude * M_PI / 180.0))));

    if (dist <= delta)
    {

        return true;
    }

    return false;
}

void doNothing(void *rt)
{
    return;
}

// getRoutesbetween func
List *getRoutesBetween(const GPXdoc *doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta)
{

    if (doc == NULL)
    {

        return NULL;
    }
    if (doc->routes == NULL)
    {

        return NULL;
    }
    if (delta < 0.0)
    {

        return NULL;
    }

    float lat, lon, dist1, dist2;
    bool check = false;

    List *routes = initializeList(&routeToString, &doNothing, &compareRoutes);

    ListIterator iter = createIterator(doc->routes);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        if (rte->waypoints == NULL)
        {

            /*freeList(routes);
            return NULL;*/
            el = nextElement(&iter);
            continue;
        }

        if (getLength(rte->waypoints) == 0)
        {
            el = nextElement(&iter);
            continue;
        }

        Waypoint *first = getFromFront(rte->waypoints);
        Waypoint *last = getFromBack(rte->waypoints);

        lat = (sourceLat - first->latitude) * (M_PI / 180.0);
        lon = (sourceLong - first->longitude) * (M_PI / 180.0);
        dist1 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(sourceLat * M_PI / 180.0))));

        lat = (sourceLat - last->latitude) * (M_PI / 180.0);
        lon = (sourceLong - last->longitude) * (M_PI / 180.0);
        dist2 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(last->latitude * M_PI / 180.0) * cos(sourceLat * M_PI / 180.0))));

        if (dist1 <= delta || dist2 <= delta)
        {
        }
        else
        {

            el = nextElement(&iter);
            continue;
        }

        lat = (destLat - first->latitude) * (M_PI / 180.0);
        lon = (destLong - first->longitude) * (M_PI / 180.0);
        dist1 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(destLat * M_PI / 180.0))));

        lat = (destLat - last->latitude) * (M_PI / 180.0);
        lon = (destLong - last->longitude) * (M_PI / 180.0);
        dist2 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(last->latitude * M_PI / 180.0) * cos(destLat * M_PI / 180.0))));

        if (dist1 <= delta || dist2 <= delta)
        {
        }
        else
        {

            el = nextElement(&iter);
            continue;
        }

        insertBack(routes, rte);
        check = true;

        el = nextElement(&iter);
    }

    if (check == true)
    {

        return routes;
    }

    freeList(routes);
    return NULL;
}

// getTrackBetween
List *getTracksBetween(const GPXdoc *doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta)
{

    if (doc == NULL)
    {

        return NULL;
    }
    if (delta < 0.0)
    {

        return NULL;
    }
    if (doc->tracks == NULL)
    {

        return NULL;
    }

    float lat, lon, dist1, dist2;
    bool check = false;

    List *tracks = initializeList(&trackToString, &doNothing, &compareTracks);

    ListIterator iter = createIterator(doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (trk->segments == NULL)
        {

            /*freeList(tracks);
            return NULL;*/
            el = nextElement(&iter);
            continue;
        }

        if (getLength(trk->segments) == 0)
        {
            el = nextElement(&iter);
            continue;
        }

        if (((TrackSegment *)getFromFront(trk->segments))->waypoints == NULL)
        {
            el = nextElement(&iter);
            continue;
        }

        Waypoint *first = getFromFront(((TrackSegment *)getFromFront(trk->segments))->waypoints);
        Waypoint *last = getFromBack(((TrackSegment *)getFromBack(trk->segments))->waypoints);

        lat = (sourceLat - first->latitude) * (M_PI / 180.0);
        lon = (sourceLong - first->longitude) * (M_PI / 180.0);
        dist1 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(sourceLat * M_PI / 180.0))));

        lat = (sourceLat - last->latitude) * (M_PI / 180.0);
        lon = (sourceLong - last->longitude) * (M_PI / 180.0);
        dist2 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(last->latitude * M_PI / 180.0) * cos(sourceLat * M_PI / 180.0))));

        if (dist1 <= delta || dist2 <= delta)
        {
        }
        else
        {

            el = nextElement(&iter);
            continue;
        }

        lat = (destLat - first->latitude) * (M_PI / 180.0);
        lon = (destLong - first->longitude) * (M_PI / 180.0);
        dist1 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(first->latitude * M_PI / 180.0) * cos(destLat * M_PI / 180.0))));

        lat = (destLat - last->latitude) * (M_PI / 180.0);
        lon = (destLong - last->longitude) * (M_PI / 180.0);
        dist2 = 2 * 6371000 * asin(sqrt(pow(sin(lat / 2), 2) + (pow(sin(lon / 2), 2) * cos(last->latitude * M_PI / 180.0) * cos(destLat * M_PI / 180.0))));

        if (dist1 <= delta || dist2 <= delta)
        {
        }
        else
        {

            el = nextElement(&iter);
            continue;
        }

        insertBack(tracks, trk);
        check = true;

        el = nextElement(&iter);
    }

    if (check == true)
    {

        return tracks;
    }

    freeList(tracks);
    return NULL;
}

// route to json
char *routeToJSON(const Route *rt)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "{}");

    if (rt == NULL)
    {

        return nullString;
    }
    if (rt->name == NULL)
    {

        return nullString;
    }
    if (rt->waypoints == NULL)
    {

        return nullString;
    }

    int buffer = 150;

    char *toJSON = malloc(sizeof(char) * (buffer + strlen(rt->name)));

    if (strlen(rt->name) == 0)
    {

        sprintf(toJSON, "{\"name\":\"None\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", getLength(rt->waypoints), round10(getRouteLen(rt)), isLoopRoute(rt, 10) == 1 ? "true" : "false");
    }
    else
    {

        sprintf(toJSON, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", rt->name, getLength(rt->waypoints), round10(getRouteLen(rt)), isLoopRoute(rt, 10) == 1 ? "true" : "false");
    }

    free(nullString);
    return toJSON;
}

// tracktoJson
char *trackToJSON(const Track *tr)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "{}");

    if (tr == NULL)
    {

        return nullString;
    }
    if (tr->segments == NULL)
    {

        return nullString;
    }
    if (tr->name == NULL)
    {

        return nullString;
    }

    // CODE ADDED TO GET NUMPOINTS
    int numpoints = 0;
    ListIterator iter = createIterator((List *)tr->segments);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        TrackSegment *seg = (TrackSegment *)el;

        ListIterator iter2 = createIterator((List *)seg->waypoints);
        void *el2 = NULL;
        el2 = nextElement(&iter2);
        while (el2 != NULL)
        {

            numpoints++;

            el2 = nextElement(&iter2);
        }

        el = nextElement(&iter);
    }

    int buffer = 150;
    char *toJSON = malloc(sizeof(char) * (buffer + strlen(tr->name)));

    if (strlen(tr->name) == 0)
    {

        sprintf(toJSON, "{\"name\":\"None\",\"points\":%d,\"len\":%.1f,\"loop\":%s}", numpoints, round10(getTrackLen(tr)), isLoopTrack(tr, 10) == 1 ? "true" : "false");
    }
    else
    {

        sprintf(toJSON, "{\"name\":\"%s\",\"points\":%d,\"len\":%.1f,\"loop\":%s}", tr->name, numpoints, round10(getTrackLen(tr)), isLoopTrack(tr, 10) == 1 ? "true" : "false");
    }

    free(nullString);
    return toJSON;
}

// routelisttostring
char *routeListToJSON(const List *routeList)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "[]");

    if (routeList == NULL)
    {

        return nullString;
    }
    if (getLength((List *)routeList) == 0)
    {

        return nullString;
    }

    bool first = true;

    char *toJSON = malloc(sizeof(char) * 2);
    strcpy(toJSON, "[");
    int len = 2;

    ListIterator iter = createIterator((List *)routeList);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        if (first != true)
        {

            strcat(toJSON, ",");
        }

        first = false;

        char *append = routeToJSON(rte);
        len += (strlen(append) + 1);
        toJSON = realloc(toJSON, sizeof(char) * len);
        strcat(toJSON, append);
        free(append);

        el = nextElement(&iter);
    }

    len++;
    toJSON = realloc(toJSON, sizeof(char) * len);
    strcat(toJSON, "]");

    free(nullString);

    return toJSON;
}

// tracklisttoJSON
char *trackListToJSON(const List *trackList)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "[]");

    if (trackList == NULL)
    {

        return nullString;
    }
    if (getLength((List *)trackList) == 0)
    {

        return nullString;
    }

    bool first = true;

    char *toJSON = malloc(sizeof(char) * 2);
    strcpy(toJSON, "[");
    int len = 2;

    ListIterator iter = createIterator((List *)trackList);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (first != true)
        {

            strcat(toJSON, ",");
        }

        first = false;

        char *append = trackToJSON(trk);
        len += (strlen(append) + 1);
        toJSON = realloc(toJSON, sizeof(char) * len);
        strcat(toJSON, append);
        free(append);

        el = nextElement(&iter);
    }

    len++;
    toJSON = realloc(toJSON, sizeof(char) * len);
    strcat(toJSON, "]");

    free(nullString);

    return toJSON;
}

// gpxtojson
char *GPXtoJSON(const GPXdoc *gpx)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "{}");

    if (gpx == NULL)
    {

        return nullString;
    }
    if (strlen(gpx->namespace) == 0)
    {

        return nullString;
    }
    if (gpx->creator == NULL || strlen(gpx->creator) == 0)
    {

        return nullString;
    }
    if (gpx->waypoints == NULL)
    {

        return nullString;
    }
    if (gpx->routes == NULL)
    {

        return nullString;
    }
    if (gpx->tracks == NULL)
    {

        return nullString;
    }

    int buffer = 120;
    char *toJSON = malloc(sizeof(char) * (buffer + strlen(gpx->creator) + strlen(gpx->namespace)));

    sprintf(toJSON, "{\"version\":%.1f,\"creator\":\"%s\",\"numWaypoints\":%d,\"numRoutes\":%d,\"numTracks\":%d}", gpx->version, gpx->creator, getLength(gpx->waypoints), getLength(gpx->routes), getLength(gpx->tracks));

    free(nullString);
    return toJSON;
}

/*BONUS FUNCS
 *
 *
 *
 */
void addWaypoint(Route *rt, Waypoint *pt)
{
}

void addRoute(GPXdoc *doc, Route *rt)
{
}

GPXdoc *JSONtoGPX(const char *gpxString)
{

    return NULL;
}

Waypoint *JSONtoWaypoint(const char *gpxString)
{

    return NULL;
}

Route *JSONtoRoute(const char *gpxString)
{

    return NULL;
}

char *GPXtoJSONwithName(const GPXdoc *gpx, char *filename)
{

    char *nullString = malloc(sizeof(char) * 7);
    strcpy(nullString, "failed");

    if (gpx == NULL)
    {

        return nullString;
    }
    if (strlen(gpx->namespace) == 0)
    {

        return nullString;
    }
    if (gpx->creator == NULL || strlen(gpx->creator) == 0)
    {

        return nullString;
    }
    if (gpx->waypoints == NULL)
    {

        return nullString;
    }
    if (gpx->routes == NULL)
    {

        return nullString;
    }
    if (gpx->tracks == NULL)
    {

        return nullString;
    }
    if (filename == NULL)
    {

        return nullString;
    }

    int buffer = 120;
    char *toJSON = malloc(sizeof(char) * (buffer + strlen(gpx->creator) + strlen(gpx->namespace) + strlen(filename)));

    sprintf(toJSON, "{\"filename\":\"%s\",\"version\":%.1f,\"creator\":\"%s\",\"numWaypoints\":%d,\"numRoutes\":%d,\"numTracks\":%d}", filename, gpx->version, gpx->creator, getLength(gpx->waypoints), getLength(gpx->routes), getLength(gpx->tracks));

    free(nullString);
    return toJSON;
}

char *getValidLogData(char *file, char *xsd)
{

    char *failed = malloc(sizeof(char) * 7);
    strcpy(failed, "failed");

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    free(failed);

    char *data = GPXtoJSONwithName(doc, file);
    deleteGPXdoc(doc);

    return data;
}

char *getValidViewData(char *file, char *xsd)
{

    char *failed = malloc(sizeof(char) * 7);
    strcpy(failed, "failed");

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    free(failed);

    char *rts = routeListToJSON(doc->routes);
    char *trs = trackListToJSON(doc->tracks);

    char *data = malloc(sizeof(char) * (20 + strlen(rts) + strlen(trs)));
    strcpy(data, "{");

    strcat(data, rts);
    strcat(data, ",");

    strcat(data, trs);
    strcat(data, "}");

    free(rts);
    free(trs);

    deleteGPXdoc(doc);

    return data;
}

char *getValidRouteOtherData(char *file, char *xsd, int num)
{

    char *failed = malloc(sizeof(char) * 3);
    strcpy(failed, "[]");

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    // free(failed);

    int len = 3;

    char *data = malloc(sizeof(char) * 3);
    strcpy(data, "[");

    // get route other data
    int count = 0;
    ListIterator iter = createIterator((List *)doc->routes);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        count++;

        if (count == num)
        {

            bool first = true;
            ListIterator iter2 = createIterator((List *)rte->otherData);
            void *el2 = NULL;
            el2 = nextElement(&iter2);
            while (el2 != NULL)
            {

                if (first != true)
                {
                    strcat(data, ",");
                }

                char buffer[500] = {0};

                GPXData *d = (GPXData *)el2;

                snprintf(buffer, sizeof(buffer), "{\"name\":\"%s\",\"data\":\"%s\"}", d->name, d->value);

                len += strlen(buffer) + 1;

                data = realloc(data, sizeof(char) * len);

                if (data == NULL)
                {
                    return failed;
                }

                strcat(data, buffer);

                first = false;

                el2 = nextElement(&iter2);
            }
        }

        el = nextElement(&iter);
    }

    strcat(data, "]");

    deleteGPXdoc(doc);
    free(failed);

    return data;
}

char *getValidTrackOtherData(char *file, char *xsd, int num)
{

    char *failed = malloc(sizeof(char) * 7);
    strcpy(failed, "[]");

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    int len = 3;

    char *data = malloc(sizeof(char) * 3);
    strcpy(data, "[");

    // get route other data
    int count = 0;
    ListIterator iter = createIterator((List *)doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        count++;

        if (count == num)
        {

            bool first = true;
            ListIterator iter2 = createIterator((List *)trk->otherData);
            void *el2 = NULL;
            el2 = nextElement(&iter2);
            while (el2 != NULL)
            {

                if (first != true)
                {
                    strcat(data, ",");
                }

                char buffer[500] = {0};

                GPXData *d = (GPXData *)el2;

                snprintf(buffer, sizeof(buffer), "{\"name\":\"%s\",\"data\":\"%s\"}", d->name, d->value);

                len += strlen(buffer) + 1;

                data = realloc(data, sizeof(char) * len);

                if (data == NULL)
                {
                    return failed;
                }

                strcat(data, buffer);

                first = false;

                el2 = nextElement(&iter2);
            }
        }

        el = nextElement(&iter);
    }

    strcat(data, "]");

    deleteGPXdoc(doc);
    free(failed);

    return data;
}

char *updateRouteName(char *file, char *xsd, char *newName, int num)
{

    char *failed = malloc(sizeof(char) * 3);
    strcpy(failed, "-1");

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    if (newName == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    // get route other data
    int count = 0;
    ListIterator iter = createIterator((List *)doc->routes);
    void *el = NULL;
    char *oldName;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        count++;

        if (count == num)
        {

            oldName = malloc(sizeof(char) * (strlen(rte->name) + 1));
            strcpy(oldName, rte->name);
            free(rte->name);
            rte->name = malloc(sizeof(char) * (strlen(newName) + 1));
            strcpy(rte->name, newName);
        }

        el = nextElement(&iter);
    }

    writeGPXdoc(doc, file);
    deleteGPXdoc(doc);

    return oldName;
}

int updateTrackName(char *file, char *xsd, char *newName, int num)
{

    int failed = -1;

    if (file == NULL)
    {

        return failed;
    }

    if (xsd == NULL)
    {

        return failed;
    }

    if (newName == NULL)
    {

        return failed;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {

        deleteGPXdoc(doc);
        return failed;
    }

    // get route other data
    int count = 0;
    ListIterator iter = createIterator((List *)doc->tracks);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        count++;

        if (count == num)
        {

            free(trk->name);
            trk->name = malloc(sizeof(char) * (strlen(newName) + 1));
            strcpy(trk->name, newName);
        }

        el = nextElement(&iter);
    }

    writeGPXdoc(doc, file);
    deleteGPXdoc(doc);

    return 1;
}

bool isValidDoc(char *file, char *xsd)
{

    if (file == NULL)
    {
        return false;
    }
    if (xsd == NULL)
    {
        return false;
    }

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {
        return false;
    }

    deleteGPXdoc(doc);
    return true;
}

bool createDocForm(char *file, double version, char *creator)
{

    if (file == NULL)
    {
        return false;
    }

    if (creator == NULL)
    {
        return false;
    }

    char namespace[256] = "http://www.topografix.com/GPX/1/1";

    GPXdoc *doc = malloc(sizeof(GPXdoc));

    if (doc == NULL)
    {

        return false;
    }

    strcpy(doc->namespace, namespace);

    doc->version = version;

    doc->creator = malloc(sizeof(char) * (strlen(creator) + 1));

    strcpy(doc->creator, creator);

    doc->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    doc->routes = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    doc->tracks = initializeList(&trackToString, &deleteTrack, &compareTracks);

    bool val = validateGPXDoc(doc, "parser/gpx.xsd");

    if (val == true)
    {
        writeGPXdoc(doc, file);
    }

    deleteGPXdoc(doc);

    return val;
}

bool addRouteForm(char *file, char *xsd, char *name)
{

    if (file == NULL)
    {
        return false;
    }
    if (xsd == NULL)
    {
        return false;
    }

    if (name == NULL)
    {
        return false;
    }

    Route *rte = malloc(sizeof(Route));

    rte->name = malloc(sizeof(char) * (strlen(name) + 1));

    strcpy(rte->name, name);

    rte->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    rte->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {
        deleteRoute(rte);
        return false;
    }

    insertBack(doc->routes, rte);

    bool check = validateGPXDoc(doc, xsd);

    if (check == false)
    {
        deleteGPXdoc(doc);
        return false;
    }

    writeGPXdoc(doc, file);

    deleteGPXdoc(doc);

    return true;
}

bool addWptRoute(char *file, char *xsd, char *name, double lat, double lon)
{

    if (file == NULL)
    {
        return false;
    }
    if (xsd == NULL)
    {
        return false;
    }
    if (name == NULL)
    {
        return false;
    }

    Waypoint *wpt = malloc(sizeof(Waypoint));

    wpt->name = malloc(sizeof(char) * (strlen(name) + 1));

    strcpy(wpt->name, name);

    wpt->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    wpt->latitude = lat;
    wpt->longitude = lon;

    GPXdoc *doc = createValidGPXdoc(file, xsd);

    if (doc == NULL)
    {
        deleteWaypoint(wpt);
        return false;
    }

    Route *rte = getFromBack(doc->routes);

    insertBack(rte->waypoints, wpt);

    bool check = validateGPXDoc(doc, xsd);

    if (check == false)
    {
        deleteGPXdoc(doc);
        return false;
    }

    writeGPXdoc(doc, file);

    deleteGPXdoc(doc);

    return true;
}

char *routeToPath(const Route *rt)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "");

    if (rt == NULL)
    {

        return nullString;
    }
    if (rt->name == NULL)
    {

        return nullString;
    }
    if (rt->waypoints == NULL)
    {

        return nullString;
    }

    int buffer = 200;

    char *toJSON = malloc(sizeof(char) * (buffer + strlen(rt->name)));

    if (strlen(rt->name) == 0)
    {

        sprintf(toJSON, "{\"type\":\"R\",\"name\":\"None\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", getLength(rt->waypoints), round10(getRouteLen(rt)), isLoopRoute(rt, 10) == 1 ? "true" : "false");
    }
    else
    {

        sprintf(toJSON, "{\"type\":\"R\",\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", rt->name, getLength(rt->waypoints), round10(getRouteLen(rt)), isLoopRoute(rt, 10) == 1 ? "true" : "false");
    }

    free(nullString);
    return toJSON;
}

// tracktoJson
char *trackToPath(const Track *tr)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "");

    if (tr == NULL)
    {

        return nullString;
    }
    if (tr->segments == NULL)
    {

        return nullString;
    }
    if (tr->name == NULL)
    {

        return nullString;
    }

    // CODE ADDED TO GET NUMPOINTS
    int numpoints = 0;
    ListIterator iter = createIterator((List *)tr->segments);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        TrackSegment *seg = (TrackSegment *)el;

        ListIterator iter2 = createIterator((List *)seg->waypoints);
        void *el2 = NULL;
        el2 = nextElement(&iter2);
        while (el2 != NULL)
        {

            numpoints++;

            el2 = nextElement(&iter2);
        }

        el = nextElement(&iter);
    }

    int buffer = 200;
    char *toJSON = malloc(sizeof(char) * (buffer + strlen(tr->name)));

    if (strlen(tr->name) == 0)
    {

        sprintf(toJSON, "{\"type\":\"T\",\"name\":\"None\",\"points\":%d,\"len\":%.1f,\"loop\":%s}", numpoints, round10(getTrackLen(tr)), isLoopTrack(tr, 10) == 1 ? "true" : "false");
    }
    else
    {

        sprintf(toJSON, "{\"type\":\"T\",\"name\":\"%s\",\"points\":%d,\"len\":%.1f,\"loop\":%s}", tr->name, numpoints, round10(getTrackLen(tr)), isLoopTrack(tr, 10) == 1 ? "true" : "false");
    }

    free(nullString);
    return toJSON;
}

char *routeListToPath(const List *routeList)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "*");

    if (routeList == NULL)
    {

        return nullString;
    }
    if (getLength((List *)routeList) == 0)
    {

        return nullString;
    }

    bool first = true;

    char *toJSON = malloc(sizeof(char) * 2);
    int len = 2;

    ListIterator iter = createIterator((List *)routeList);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Route *rte = (Route *)el;

        if (first != true)
        {

            strcat(toJSON, ",");
        }

        char *append = routeToPath(rte);
        len += (strlen(append) + 1);
        toJSON = realloc(toJSON, sizeof(char) * len);
        if (first != true)
        {
            strcat(toJSON, append);
        }
        else
        {
            strcpy(toJSON, append);
        }
        free(append);

        first = false;

        el = nextElement(&iter);
    }

    // len++;
    // toJSON = realloc(toJSON, sizeof(char)*len);

    free(nullString);

    return toJSON;
}

// tracklisttoJSON
char *trackListToPath(const List *trackList)
{

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "*");

    if (trackList == NULL)
    {

        return nullString;
    }
    if (getLength((List *)trackList) == 0)
    {

        return nullString;
    }

    bool first = true;

    char *toJSON = malloc(sizeof(char) * 2);
    int len = 2;

    ListIterator iter = createIterator((List *)trackList);
    void *el = NULL;
    el = nextElement(&iter);
    while (el != NULL)
    {

        Track *trk = (Track *)el;

        if (first != true)
        {

            strcat(toJSON, ",");
        }

        char *append = trackToPath(trk);
        len += (strlen(append) + 1);
        toJSON = realloc(toJSON, sizeof(char) * len);
        if (first != true)
        {
            strcat(toJSON, append);
        }
        else
        {
            strcpy(toJSON, append);
        }
        free(append);

        first = false;

        el = nextElement(&iter);
    }

    // len++;
    // toJSON = realloc(toJSON, sizeof(char)*len);

    free(nullString);

    return toJSON;
}

char *getPathBetween(char *file, double sLat, double sLon, double eLat, double eLon, double tol)
{

    char *null = malloc(sizeof(char) * 3);
    strcpy(null, "*");
    if (file == NULL)
    {
        return null;
    }

    GPXdoc *doc = createValidGPXdoc(file, "parser/gpx.xsd");

    if (doc == NULL)
    {
        return null;
    }

    bool check = false;

    List *rtes = getRoutesBetween(doc, (float)sLat, (float)sLon, (float)eLat, (float)eLon, (float)tol);
    List *trks = getTracksBetween(doc, (float)sLat, (float)sLon, (float)eLat, (float)eLon, (float)tol);

    char *rtString = routeListToPath(rtes);
    char *trString = trackListToPath(trks);

    char *returned = malloc(sizeof(char) * (strlen(rtString) + strlen(trString) + 2));

    strcpy(returned, "");

    if (strcmp("*", rtString) != 0)
    {
        check = true;
        strcat(returned, rtString);
    }
    if (strcmp("*", trString) != 0)
    {
        if (check == true)
        {
            strcat(returned, ",");
        }
        check = true;
        strcat(returned, trString);
    }

    freeList(rtes);
    freeList(trks);
    deleteGPXdoc(doc);
    free(rtString);
    free(trString);

    if (check == false)
    {
        return null;
    }

    return returned;
}

char *jsonRoutes(char *file, char *xsd)
{

    GPXdoc *doc = createValidGPXdoc(file, "parser/gpx.xsd");

    char *string = routeListToJSON(doc->routes);

    return string;
}

char *routeToWaypoint(char *file, char *name, int len1, int index)
{

    GPXdoc *doc = createValidGPXdoc(file, "parser/gpx.xsd");

    ListIterator iter1 = createIterator((List *)doc->routes);
    void *el1 = NULL;
    Route *rte = NULL;
    int i = 0;
    el1 = nextElement(&iter1);
    while (el1 != NULL)
    {

        Route *rte1 = (Route *)el1;

        float val = abs((float)len1 - getRouteLen(rte1));

        if (val < 100 && strcmp(name, rte1->name) == 0 && index == i)
        {
            rte = rte1;
            break;
        }

        if (index == -1)
        {
            rte = rte1;
        }

        i++;
        el1 = nextElement(&iter1);
    }

    char *nullString = malloc(sizeof(char) * 3);
    strcpy(nullString, "[]");

    if (rte == NULL)
    {

        return nullString;
    }

    bool first = true;

    char *toJSON = malloc(sizeof(char) * 2);
    strcpy(toJSON, "[");
    int len = 2;

    ListIterator iter = createIterator((List *)rte->waypoints);
    void *el = NULL;
    el = nextElement(&iter);
    i = 0;
    while (el != NULL)
    {

        Waypoint *wpt = (Waypoint *)el;

        if (first != true)
        {

            strcat(toJSON, ",");
        }

        first = false;

        char append[512];
        snprintf(append, 512, "{\"index\":%d,\"lat\":%f,\"lon\":%f,\"name\":\"%s\"}", i, wpt->latitude, wpt->longitude, wpt->name);
        len += (strlen(append) + 1);
        toJSON = realloc(toJSON, sizeof(char) * len);
        strcat(toJSON, append);
        // free(append);

        i++;
        el = nextElement(&iter);
    }

    len++;
    toJSON = realloc(toJSON, sizeof(char) * len);
    strcat(toJSON, "]");

    free(nullString);

    return toJSON;
}