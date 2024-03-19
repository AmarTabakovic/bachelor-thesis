#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>

class ConfigManager
{
public:
    ConfigManager();

    void loadConfig(std::string path);

    std::string _cachePath;
    std::string _cacheSizeInBytes;
    std::string _tileSize;
    std::string _xyzTileServiceUrl; /* Including API key */
};

#endif // CONFIGMANAGER_H
