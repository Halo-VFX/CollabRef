#include "BoardSerializer.h"
#include "Board.h"

#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Board* BoardSerializer::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }
    
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);
    
    // Read and verify magic number
    char magic[5] = {0};
    stream.readRawData(magic, 4);
    if (strcmp(magic, FILE_MAGIC) != 0) {
        return nullptr;
    }
    
    // Read version
    qint32 version;
    stream >> version;
    
    if (version > FILE_VERSION) {
        // Future version, try to read anyway
    }
    
    // Read board metadata as JSON
    QByteArray metaJson;
    stream >> metaJson;
    
    QJsonDocument metaDoc = QJsonDocument::fromJson(metaJson);
    QJsonObject meta = metaDoc.object();
    
    Board *board = new Board();
    board->setName(meta["name"].toString());
    board->setBackgroundColor(QColor(meta["backgroundColor"].toString()));
    
    // Read image count
    qint32 imageCount;
    stream >> imageCount;
    
    // Read each image
    for (int i = 0; i < imageCount; ++i) {
        // Image metadata as JSON
        QByteArray imgMetaJson;
        stream >> imgMetaJson;
        
        QJsonDocument imgMetaDoc = QJsonDocument::fromJson(imgMetaJson);
        QJsonObject imgMeta = imgMetaDoc.object();
        
        // Image data
        QByteArray imageData;
        stream >> imageData;
        
        QImage image;
        image.loadFromData(imageData, "PNG");
        
        if (image.isNull()) {
            continue;
        }
        
        BoardImage boardImg;
        boardImg.id = imgMeta["id"].toString();
        boardImg.image = image;
        boardImg.position = QPointF(imgMeta["x"].toDouble(), imgMeta["y"].toDouble());
        boardImg.rotation = imgMeta["rotation"].toDouble();
        boardImg.scale = imgMeta["scale"].toDouble(1.0);
        boardImg.zIndex = imgMeta["zIndex"].toDouble();
        boardImg.sourcePath = imgMeta["sourcePath"].toString();
        boardImg.flippedH = imgMeta["flippedH"].toBool();
        boardImg.flippedV = imgMeta["flippedV"].toBool();
        
        if (imgMeta.contains("cropRect")) {
            QJsonObject crop = imgMeta["cropRect"].toObject();
            boardImg.cropRect = QRectF(crop["x"].toDouble(), crop["y"].toDouble(),
                                       crop["width"].toDouble(), crop["height"].toDouble());
        }
        
        board->addImage(boardImg);
    }
    
    board->setModified(false);
    return board;
}

bool BoardSerializer::save(Board *board, const QString &filePath)
{
    if (!board) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);
    
    // Write magic number
    stream.writeRawData(FILE_MAGIC, 4);
    
    // Write version
    stream << static_cast<qint32>(FILE_VERSION);
    
    // Write board metadata as JSON
    QJsonObject meta;
    meta["name"] = board->name();
    meta["backgroundColor"] = board->backgroundColor().name();
    
    QByteArray metaJson = QJsonDocument(meta).toJson(QJsonDocument::Compact);
    stream << metaJson;
    
    // Write image count
    QList<QString> imageIds = board->imageIds();
    stream << static_cast<qint32>(imageIds.size());
    
    // Write each image
    for (const QString &id : imageIds) {
        BoardImage img = board->image(id);
        
        // Image metadata as JSON
        QJsonObject imgMeta;
        imgMeta["id"] = img.id;
        imgMeta["x"] = img.position.x();
        imgMeta["y"] = img.position.y();
        imgMeta["rotation"] = img.rotation;
        imgMeta["scale"] = img.scale;
        imgMeta["zIndex"] = img.zIndex;
        imgMeta["sourcePath"] = img.sourcePath;
        imgMeta["flippedH"] = img.flippedH;
        imgMeta["flippedV"] = img.flippedV;
        
        if (img.cropRect.isValid()) {
            QJsonObject crop;
            crop["x"] = img.cropRect.x();
            crop["y"] = img.cropRect.y();
            crop["width"] = img.cropRect.width();
            crop["height"] = img.cropRect.height();
            imgMeta["cropRect"] = crop;
        }
        
        QByteArray imgMetaJson = QJsonDocument(imgMeta).toJson(QJsonDocument::Compact);
        stream << imgMetaJson;
        
        // Image data as PNG
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        img.image.save(&buffer, "PNG");
        
        stream << imageData;
    }
    
    file.close();
    return true;
}
