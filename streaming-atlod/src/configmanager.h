#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <unordered_map>

/**
 * @brief The ConfigManager class
 *
 * ATTENTION: The config manager is currently designed to only load the config
 *            once upon start up and not modify them for the remainder of the
 *            runtime! This class is not thread-safe and the values should not
 *            be mutated from multiple threads at any time.
 */
class ConfigManager {
protected:
    ConfigManager();
    static ConfigManager* _manager;
    bool addSingleEntry(std::string key, std::string value);
    bool tryParsingNumber(int& property, std::string value, std::string errorMessage);

public:
    ConfigManager(ConfigManager& other) = delete;
    void operator=(const ConfigManager&) = delete;

    static ConfigManager* getInstance();

    void loadConfig(std::string path);

    std::string _diskCachePath = "";
    std::string _heightDataServiceUrl = "";
    std::string _heightDataServiceKey = "";
    std::string _overlayDataServiceUrl = "";
    std::string _overlayDataServiceKey = "";
    int _memoryCacheSize = -1;
    int _diskCacheSize = -1;
    int _lowMeshRes = -1;
    int _mediumMeshRes = -1;
    int _highMeshRes = -1;
    int _numLoadWorkers = -1;
};

#endif // CONFIGMANAGER_H
