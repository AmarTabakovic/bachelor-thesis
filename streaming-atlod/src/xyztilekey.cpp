#include "xyztilekey.h"
#include <iostream>
#include <sstream>

XYZTileKey::XYZTileKey(unsigned x, unsigned y, unsigned z)
    : _x(x)
    , _y(y)
    , _z(z)
{
}

XYZTileKey::XYZTileKey(std::string str)
{
    std::istringstream iss(str);
    char delim;

    if (!(iss >> _x >> delim >> _y >> delim >> _z) || delim != '/') {
        std::cerr << "Invalid tile key." << std::endl;
    }
}

bool XYZTileKey::operator==(const XYZTileKey& other) const
{
    return _x == other._x && _y == other._y && _z == other._z;
}

bool XYZTileKey::operator!=(const XYZTileKey& other) const
{
    return !(*this == other);
}

std::string XYZTileKey::string()
{
    return std::to_string(_x) + "/" + std::to_string(_y) + "/" + std::to_string(_z);
}

XYZTileKey XYZTileKey::topLeftChild() const
{
    return XYZTileKey(_x * 2, _y * 2, _z + 1);
}

XYZTileKey XYZTileKey::topRightChild() const
{
    return XYZTileKey(_x * 2 + 1, _y * 2, _z + 1);
}

XYZTileKey XYZTileKey::bottomLeftChild() const
{
    return XYZTileKey(_x * 2, _y * 2 + 1, _z + 1);
}

XYZTileKey XYZTileKey::bottomRightChild() const
{
    return XYZTileKey(_x * 2 + 1, _y * 2 + 1, _z + 1);
}

unsigned XYZTileKey::x() const
{
    return _x;
}

unsigned XYZTileKey::y() const
{
    return _y;
}

unsigned XYZTileKey::z() const
{
    return _z;
}
