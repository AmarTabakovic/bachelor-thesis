#ifndef XYZTILEKEY_H
#define XYZTILEKEY_H

#include <string>

/**
 * @brief The XYZTileKey class
 */
class XYZTileKey
{
public:
    XYZTileKey(unsigned x, unsigned y, unsigned z);
    XYZTileKey(std::string str);
    std::string string();
    XYZTileKey topLeftChild();
    XYZTileKey topRightChild();
    XYZTileKey bottomLeftChild();
    XYZTileKey bottomRightChild();

    unsigned _x, _y, _z;
};

#endif // XYZTILEKEY_H
