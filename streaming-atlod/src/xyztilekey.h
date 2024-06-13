#ifndef XYZTILEKEY_H
#define XYZTILEKEY_H

#include <functional>
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
    bool operator==(const XYZTileKey& other) const;
    bool operator!=(const XYZTileKey& other) const;
    XYZTileKey topLeftChild() const;
    XYZTileKey topRightChild() const;
    XYZTileKey bottomLeftChild() const;
    XYZTileKey bottomRightChild() const;

    unsigned x() const;
    unsigned y() const;
    unsigned z() const;

private:
    unsigned _x, _y, _z;
};

/**
 * Hash function injection
 *
 * Based on:
 * - https://en.cppreference.com/w/cpp/utility/hash
 * - https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
 * - http://stackoverflow.com/a/1646913/126995
 */
template <>
struct std::hash<XYZTileKey> {
    std::size_t operator()(const XYZTileKey& k) const
    {
        std::size_t res = 17;
        res = res * 31 + hash<unsigned>()(k.x());
        res = res * 31 + hash<unsigned>()(k.y());
        res = res * 31 + hash<unsigned>()(k.z());
        return res;
    }
};

#endif // XYZTILEKEY_H
