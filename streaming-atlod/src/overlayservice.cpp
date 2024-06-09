#include "overlayservice.h"
#include <curl/curl.h>

OverlayService::OverlayService()
{
}

void OverlayService::request(XYZTileKey tileKey)
{
    /* Steps:
     * - Create HTTP request string with XYZ tile key
     * - Send HTTP request and receive response
     * - Check return code
     * - Write response into buffer (if successful)
     * - Done
     */
}
