#ifndef BOARDSERIALIZER_H
#define BOARDSERIALIZER_H

#include <QString>

class Board;

class BoardSerializer
{
public:
    static Board* load(const QString &filePath);
    static bool save(Board *board, const QString &filePath);
    
private:
    BoardSerializer() = default;
    
    static constexpr int FILE_VERSION = 1;
    static constexpr char FILE_MAGIC[] = "CREF";
};

#endif // BOARDSERIALIZER_H
