#ifndef HEIGHTSERVICE_H
#define HEIGHTSERVICE_H

#include "xyztilekey.h"
#include <curl/curl.h>
#include <string>

class HeightService {
public:
    HeightService();

    std::string _baseUrl = "https://api.maptiler.com/tiles/terrain-rgb-v2/";

    void request(XYZTileKey tileKey);
};

#endif // HEIGHTSERVICE_H
