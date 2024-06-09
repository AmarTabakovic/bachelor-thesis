#include "xyztilekey.h"
#include <iostream>
#include <sstream>

XYZTileKey::XYZTileKey(unsigned x, unsigned y, unsigned z)
{
    _x = x;
    _y = y;
    _z = z;
}

XYZTileKey::XYZTileKey(std::string str)
{
    std::istringstream iss(str);
    char delim;

    if (!(iss >> _x >> delim >> _y >> delim >> _z) || delim != '/') {
        std::cerr << "Invalid." << std::endl;
    }
}

std::string XYZTileKey::string()
{
    return std::to_string(_x) + "/" + std::to_string(_y) + "/" + std::to_string(_z);
}

XYZTileKey XYZTileKey::topLeftChild()
{
    return XYZTileKey(_x * 2, _y * 2, _z + 1);
}

XYZTileKey XYZTileKey::topRightChild()
{
    return XYZTileKey(_x * 2 + 1, _y * 2, _z + 1);
}

XYZTileKey XYZTileKey::bottomLeftChild()
{
    return XYZTileKey(_x * 2, _y * 2 + 1, _z + 1);
}

XYZTileKey XYZTileKey::bottomRightChild()
{
    return XYZTileKey(_x * 2 + 1, _y * 2 + 1, _z + 1);
}
