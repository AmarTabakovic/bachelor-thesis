#ifndef OVERLAYSERVICE_H
#define OVERLAYSERVICE_H

#include "xyztilekey.h"
#include <curl/curl.h>
#include <string>

class OverlayService {
public:
    OverlayService();

    std::string _baseUrl = "https://api.maptiler.com/tiles/satellite-v2/";

    void request(XYZTileKey tileKey);
};

#endif // OVERLAYSERVICE_H
