#ifndef IMAGEDATABASE_H
#define IMAGEDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>

#include "commondebug.h"
#include "mpdalbum.h"
#include "mpdartist.h"
#include "albuminformation.h"
#include "lastfmalbumprovider.h"
#include "databasefilljob.h"


class ImageDatabase : public QObject
{
    Q_OBJECT
public:
    explicit ImageDatabase(QObject *parent = 0);
    ~ImageDatabase();



    bool syncAlbums(QList<MpdAlbum*> *albums,MpdArtist *artist);
    bool syncArtists(QList<MpdAlbum*> &artists);

public slots:
    void albumReady(AlbumInformation *albumInformation);
    void fillDatabase(QMap<MpdArtist*, QList<MpdAlbum*>* > *map);

private:
    QSqlDatabase *mDB;

    bool mAlbumSyncRunning;
    int mAlbumNo;
    LastFMAlbumProvider *mCurrentAlbumProvider;
    QList<MpdAlbum*>* mAlbums;
    MpdArtist *mAlbumArtist;
    DatabaseFillJob *mFiller;

signals:

public slots:

};

#endif // IMAGEDATABASE_H
