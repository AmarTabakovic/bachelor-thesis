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
    bool addSingleEntry(std::string key, std::string value);
    bool tryParsingNumber(int& property, std::string value, std::string errorMessage);

    static ConfigManager* _manager;

    std::string _diskCachePath = "";
    std::string _heightDataServiceUrl = "";
    std::string _heightDataServiceKey = "";
    std::string _overlayDataServiceUrl = "";
    std::string _overlayDataServiceKey = "";
    std::string _dataPath = "";
    int _memoryCacheSize = -1;
    int _diskCacheSize = -1;
    int _lowMeshRes = -1;
    int _mediumMeshRes = -1;
    int _highMeshRes = -1;
    int _numLoadWorkers = -1;
    int _maxZoom = -1;

public:
    ConfigManager(ConfigManager& other) = delete;
    void operator=(const ConfigManager&) = delete;

    static ConfigManager* getInstance();

    void loadConfig(std::string path);

    std::string diskCachePath() const;
    std::string heightDataServiceUrl() const;
    std::string heightDataServiceKey() const;
    std::string overlayDataServiceUrl() const;
    std::string overlayDataServiceKey() const;
    std::string dataPath() const;
    int memoryCacheSize() const;
    int diskCacheSize() const;
    int lowMeshRes() const;
    int mediumMeshRes() const;
    int highMeshRes() const;
    int numLoadWorkers() const;
    int maxZoom() const;
};

#endif // CONFIGMANAGER_H
