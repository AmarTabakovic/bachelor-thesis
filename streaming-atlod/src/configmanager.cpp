#include "configmanager.h"
#include <filesystem>
#include <fstream>
#include <iostream>

ConfigManager::ConfigManager()
{

}

bool ConfigManager::addSingleEntry(std::string key, std::string value)
{

    bool shouldExit = false;

    if (key == "diskcachepath") {
        _diskCachePath = value;
    }
    if (key == "heightdataserviceurl") {
        _heightDataServiceUrl = value;
    }
    if (key == "overlaydataserviceurl") {
        _overlayDataServiceUrl = value;
    }
    if (key == "heightdataservicekey") {
        _heightDataServiceKey = value;
    }
    if (key == "overlaydataservicekey") {
        _overlayDataServiceKey = value;
    }
    if (key == "datapath") {
        _dataPath = value;
    }
    if (key == "memorycachesize") {
        shouldExit |= tryParsingNumber(_memoryCacheSize, value, "Memory cache size must be an unsigned integer");
    }
    if (key == "diskacachesize") {
        shouldExit |= tryParsingNumber(_diskCacheSize, value, "Disk cache size must be an unsigned integer");
    }
    if (key == "lowmeshres") {
        shouldExit |= tryParsingNumber(_lowMeshRes, value, "Low resolution mesh size must be an unsigned integer");
    }
    if (key == "mediummeshres") {
        shouldExit |= tryParsingNumber(_mediumMeshRes, value, "Medium resolution mesh size  must be an unsigned integer");
    }
    if (key == "highmeshres") {
        shouldExit |= tryParsingNumber(_highMeshRes, value, "High resolution mesh size  must be an unsigned integer");
    }
    if (key == "numloadworkers") {
        shouldExit |= tryParsingNumber(_numLoadWorkers, value, "Number of load workers must be an unsigned integer");
    }
    if (key == "maxzoom") {
        shouldExit |= tryParsingNumber(_maxZoom, value, "Maximum zoom level must be an unsigned integer");
    }

    return shouldExit;
}

bool ConfigManager::tryParsingNumber(int& property, std::string value, std::string errorMessage)
{
    try {
        int parsedValue = std::stoul(value);
        property = parsedValue;
        return false;
    } catch (...) {
        std::cerr << errorMessage << std::endl;
        return true;
    }
}

int ConfigManager::maxZoom() const
{
    return _maxZoom;
}

int ConfigManager::numLoadWorkers() const
{
    return _numLoadWorkers;
}

int ConfigManager::highMeshRes() const
{
    return _highMeshRes;
}

int ConfigManager::mediumMeshRes() const
{
    return _mediumMeshRes;
}

int ConfigManager::lowMeshRes() const
{
    return _lowMeshRes;
}

int ConfigManager::diskCacheSize() const
{
    return _diskCacheSize;
}

int ConfigManager::memoryCacheSize() const
{
    return _memoryCacheSize;
}

std::string ConfigManager::dataPath() const
{
    return _dataPath;
}

std::string ConfigManager::overlayDataServiceKey() const
{
    return _overlayDataServiceKey;
}

std::string ConfigManager::overlayDataServiceUrl() const
{
    return _overlayDataServiceUrl;
}

std::string ConfigManager::heightDataServiceKey() const
{
    return _heightDataServiceKey;
}

std::string ConfigManager::heightDataServiceUrl() const
{
    return _heightDataServiceUrl;
}

void ConfigManager::loadConfig(std::string path)
{
    std::ifstream configFile(path);
    bool shouldExit = false;

    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file" << std::endl;
        std::exit(1);
    }

    std::string line;
    while (std::getline(configFile, line)) {
        auto delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            shouldExit |= addSingleEntry(key, value);
        } else {
            shouldExit = true;
        }
    }

    if (_diskCachePath == "") {
        std::cerr << "Cache path is missing." << std::endl;
        shouldExit = true;
    }

    if (_dataPath == "") {
        std::cerr << "Data folder path is missing." << std::endl;
        shouldExit = true;
    }

    if (_heightDataServiceUrl == "") {
        std::cerr << "Height data service URL is missing." << std::endl;
        shouldExit = true;
    }

    if (_overlayDataServiceUrl == "") {
        std::cerr << "Overlay data service URL is missing." << std::endl;
        shouldExit = true;
    }

    if (_heightDataServiceKey == "") {
        std::cerr << "Height data service key is missing." << std::endl;
        shouldExit = true;
    }

    if (_overlayDataServiceKey == "") {
        std::cerr << "Overlay data service key is missing." << std::endl;
        shouldExit = true;
    }

    if (_memoryCacheSize < 200 || _memoryCacheSize > 500) {
        std::cerr << "Memory cache size must be between 200 and 500" << std::endl;
        shouldExit = true;
    }

    if (_diskCacheSize < 400 || _diskCacheSize > 8000) {
        std::cerr << "Disk cache size must be between 400 and 8000" << std::endl;
        shouldExit = true;
    }

    if (_diskCacheSize < 4 * _memoryCacheSize) {
        std::cerr << "Disk cache must be at least larger than four times the memory cache" << std::endl;
        shouldExit = true;
    }

    if (_lowMeshRes < 8 || _lowMeshRes > 512) {
        std::cerr << "Low resolution mesh size must be between 8 and 512" << std::endl;
        shouldExit = true;
    }

    if (_mediumMeshRes < 8 || _mediumMeshRes > 512) {
        std::cerr << "Medium resolution mesh size must be between 8 and 512" << std::endl;
        shouldExit = true;
    }

    if (_highMeshRes < 8 || _highMeshRes > 512) {
        std::cerr << "High resolution mesh size must be between 8 and 512" << std::endl;
        shouldExit = true;
    }

    if (_numLoadWorkers < 1 || _numLoadWorkers > 8) {
        std::cerr << "Number of load workers must be between 1 and 8" << std::endl;
        shouldExit = true;
    }

    if (_maxZoom < 0 || _maxZoom >= 30) {
        std::cerr << "The maximum zoom level must be between 0 and 30" << std::endl;
        shouldExit = true;
    }

    if (shouldExit) {
        std::exit(1);
    }
}

std::string ConfigManager::diskCachePath() const
{
    return _diskCachePath;
}

ConfigManager* ConfigManager::_manager = nullptr;

ConfigManager* ConfigManager::getInstance()
{
    if (!_manager) {
        _manager = new ConfigManager();
    }
    return _manager;
}
