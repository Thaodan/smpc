#include "networkaccess.h"

/** Constructor for NetworkAccess object. Handles all the network stuff
  */


NetworkAccess::NetworkAccess(QObject *parent) :
    QThread(parent)
{
    updateinterval = 5000;
    mPlaylistversion = -1;
    //create socket later used for communication
    tcpsocket = new QTcpSocket(this);
    statusupdater = new QTimer(this);
    connect(tcpsocket,SIGNAL(connected()),this,SLOT(connectedtoServer()));
    connect(tcpsocket,SIGNAL(disconnected()),this,SIGNAL(disconnected()));
    connect(tcpsocket,SIGNAL(disconnected()),this,SLOT(disconnectedfromServer()));
    connect(statusupdater,SIGNAL(timeout()),this,SLOT(updateStatusInternal()));
    connect(tcpsocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(errorHandle()));
}


/** connects to host and return true if successful, false if not. Takes an string as hostname and int as port */
bool NetworkAccess::connectToHost(QString hostname, quint16 port,QString password)
{
    emit busy();
    tcpsocket->connectToHost(hostname ,port,QIODevice::ReadWrite);
    connect(tcpsocket,SIGNAL(connected()),SLOT(socketConnected()));
    this->password = password;
    bool success = tcpsocket->waitForConnected(10000);
    if (!success)
    {
        emit ready();
        emit disconnected();
        return false;
    }
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        //Do host authentication
        tcpsocket->waitForReadyRead(READYREAD);
        QString response;
        while (tcpsocket->canReadLine())
        {
            response += tcpsocket->readLine();
        }
        QString teststring = response;
        teststring.truncate(6);
        if (teststring==QString("OK MPD"))
        {

        }
        authenticate(password);
        emit ready();
        emit connectionestablished();
        return true;

    }
    emit ready();
    emit disconnected();
    return false;
}

void NetworkAccess::disconnect()
{
    emit busy();
    tcpsocket->disconnectFromHost();
    emit ready();
}


/** return all albums currently availible from connected MPD as MpdAlbum objects,
  * empty list if not connected or no albums are availible */
void NetworkAccess::getAlbums()
{
    emit busy();
    QList<MpdAlbum*> *albums = new QList<MpdAlbum*>();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        //Start getting list from mpd
        //Send request
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");

        outstream << "list album" << endl;

        //Read all albums until OK send from mpd
        QString response ="";
        MpdAlbum *tempalbum;
        QString name;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(7)==QString("Album: "))
                {
                    name = response.right(response.length()-7);
                    name.chop(1);
                    tempalbum = new MpdAlbum(this,name);
                    albums->append(tempalbum);
                }
            }
        }
    }

    //Get album tracks
    qSort(albums->begin(),albums->end(),MpdAlbum::lessThan);
    emit ready();
    emit albumsReady((QList<QObject*>*)albums);
}

void NetworkAccess::getArtists()
{
    emit busy();
    emit artistsReady((QList<QObject*>*)getArtists_prv());
    emit ready();

}

QList<MpdArtist*> *NetworkAccess::getArtists_prv()
{
    QList<MpdArtist*> *artists = new QList<MpdArtist*>();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        //Start getting list from mpd
        //Send request
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");

        outstream << "list artist" << endl;

        //Read all albums until OK send from mpd
        QString response ="";
        MpdArtist *tempartist=NULL;
        QString name;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

                if (response.left(8)==QString("Artist: "))
                {
                    name = response.right(response.length()-8);
                    name.chop(1);
                    tempartist = new MpdArtist(NULL,name);
                    artists->append(tempartist);
                }

            }
        }
    }
    qSort(artists->begin(),artists->end(),MpdArtist::lessThan);
    return artists;
}


bool NetworkAccess::authenticate(QString password)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {

        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "password " << password << "\n";
        outstream.flush();
        tcpsocket->waitForReadyRead(READYREAD);
        //Check Response
        QString response;
        while (tcpsocket->canReadLine())
        {
            response += tcpsocket->readLine();
        }
        QString teststring = response;
        teststring.truncate(2);
        if (teststring==QString("OK"))
        {
            return true;
        }
        else {
            return false;
        }
        return false;
    }
    return false;
}

void NetworkAccess::socketConnected()
{
    //emit connectionestablished();
    //updateStatusInternal();
}

void NetworkAccess::getArtistsAlbums(QString artist)
{
    emit busy();
    emit(artistAlbumsReady((QList<QObject*>*)getArtistsAlbums_prv(artist)));
    emit ready();
}

QList<MpdAlbum*> *NetworkAccess::getArtistsAlbums_prv(QString artist)
{
    QList<MpdAlbum*> *albums = new QList<MpdAlbum*>();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        //Start getting list from mpd
        //Send request
        artist = artist.replace('\"',"\\\"");
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream.setAutoDetectUnicode(false);
        outstream.setCodec("UTF-8");
        outstream << "list album \"" <<artist<<"\"" << endl;

        //Read all albums until OK send from mpd
        QString response ="";
        MpdAlbum *tempalbum;
        QString name;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(7)==QString("Album: "))
                {
                    name = response.right(response.length()-7);
                    name.chop(1);
                    tempalbum = new MpdAlbum(NULL,name,artist);
                    tempalbum->moveToThread(mQmlThread);
                    QQmlEngine::setObjectOwnership(tempalbum, QQmlEngine::CppOwnership);
                    albums->append(tempalbum);
                }
            }
        }
    }

    //Get album tracks
    qSort(albums->begin(),albums->end(),MpdAlbum::lessThan);
    return albums;

}

void NetworkAccess::getAlbumTracks(QString album)
{
    emit busy();
    emit albumTracksReady((QList<QObject*>*)getAlbumTracks_prv(album));
    emit ready();
}

QList<MpdTrack*>* NetworkAccess::getAlbumTracks_prv(QString album)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        album.replace(QString("\""),QString("\\\""));

        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "find album \"" << album << "\""<< endl;
    }
    return parseMPDTracks("");
}

void  NetworkAccess::getAlbumTracks(QString album, QString cartist)
{
    emit busy();
    emit albumTracksReady((QList<QObject*>*)getAlbumTracks_prv(album,cartist));
    emit ready();
}

void  NetworkAccess::getAlbumTracks(QVariant albuminfo)
{
    emit busy();
    QStringList strings = albuminfo.toStringList();
    emit albumTracksReady((QList<QObject*>*)getAlbumTracks_prv(strings[1],strings[0]));
    emit ready();
}

QList<MpdTrack*>*  NetworkAccess::getAlbumTracks_prv(QString album, QString cartist)
{
    if (cartist=="")
    {

        return getAlbumTracks_prv(album);
    }
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        album.replace(QString("\""),QString("\\\""));
        outstream << "find album \"" << album << "\""<< endl;

    }
    return parseMPDTracks(cartist);
}

void NetworkAccess::getTracks()
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "listallinfo" << endl;
    }
    emit parseMPDTracks("");
    emit ready();
}



void NetworkAccess::getCurrentPlaylistTracks()
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        emit startupdateplaylist();
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "playlistinfo " << endl;
    }
    emit currentPlayListReady((QList<QObject*>*)parseMPDTracks(""));
    emit ready();
}


void NetworkAccess::getPlaylistTracks(QString name)
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "listplaylistinfo \"" << name << "\"" << endl;
    }
    emit savedplaylistTracksReady((QList<QObject*>*)parseMPDTracks(""));
    emit ready();
}

void NetworkAccess::updateStatusInternal()
{
    emit statusUpdate(getStatus());
}

status_struct NetworkAccess::getStatus()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        QString response ="";
        QString title;
        QString album;
        QString artist;
        QString bitrate = "";

        quint32 playlistid=-1;
        QString playlistidstring="-1";
        quint32 playlistversion = 0;
        quint32 playlistlength = 0;
        int elapsed =0;
        int runtime =0;
        QString tracknrstring="";
        int tracknr = 0;
        int albumtrackcount = 0;

        QString timestring;
        QString elapstr,runstr;
        QString volumestring;
        QString fileuri;
        quint8 playing=NetworkAccess::STOP;
        bool repeat=false;
        bool random=false;
        quint8 volume = 0;
        quint8 bitdepth = 0;
        quint16 samplerate = 0;
        quint8 channelcount = 0;

        outstream << "status" << endl;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(9)==QString("bitrate: ")) {
                    bitrate = response.right(response.length()-9);
                    bitrate.chop(1);
                }
                if (response.left(6)==QString("time: ")) {
                    timestring = response.right(response.length()-6);
                    timestring.chop(1);
                    elapstr = timestring.split(":").at(0);
                    runstr = timestring.split(":").at(1);
                    elapsed = elapstr.toInt();
                    runtime = runstr.toInt();
                }
                if (response.left(6)==QString("song: ")) {
                    playlistidstring = response.right(response.length()-6);
                    playlistidstring.chop(1);
                    playlistid = playlistidstring.toUInt();
                }
                if (response.left(8)==QString("volume: ")) {
                    volumestring = response.right(response.length()-8);
                    volumestring.chop(1);
                    volume = volumestring.toUInt();
                }
                if (response.left(10)==QString("playlist: ")) {
                    volumestring = response.right(response.length()-10);
                    volumestring.chop(1);
                    playlistversion = volumestring.toUInt();
                }
                if (response.left(16)==QString("playlistlength: ")) {
                    volumestring = response.right(response.length()-16);
                    volumestring.chop(1);
                    playlistlength = volumestring.toUInt();
                }
                if (response.left(7)==QString("state: ")) {
                    {
                        volumestring = response.right(response.length()-7);
                        volumestring.chop(1);
                        if (volumestring == "play")
                        {
                            playing = NetworkAccess::PLAYING;
                        }
                        else if (volumestring == "pause") {
                            playing = NetworkAccess::PAUSE;
                        }
                        else if (volumestring == "stop") {
                            playing = NetworkAccess::STOP;
                        }
                    }
                }
                if (response.left(8)==QString("repeat: ")) {
                    {
                        volumestring = response.right(response.length()-8);
                        volumestring.chop(1);
                        if (volumestring == "1")
                        {
                            repeat = true;
                        }
                        else {
                            repeat = false;
                        }
                    }
                }
                if (response.left(8)==QString("random: ")) {
                    {
                        volumestring = response.right(response.length()-8);
                        volumestring.chop(1);
                        if (volumestring == "1")
                        {
                            random = true;
                        }
                        else {
                            random = false;
                        }
                    }
                }
                if(response.left(7)==QString("audio: ")) {
                    QStringList templist = response.right(response.length()-7).split(":");
                    if(templist.length()==3){
                        samplerate=templist.at(0).toUInt();
                        channelcount = templist.at(2).toUInt();
                        bitdepth = templist.at(1).toUInt();
                    }
                }

            }
        }

        response = "";
        outstream << "currentsong" << endl;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(7)==QString("Title: ")) {
                    title = response.right(response.length()-7);
                    title.chop(1);
                }
                else if (response.left(8)==QString("Artist: ")) {
                    artist = response.right(response.length()-8);
                    artist.chop(1);
                }
                else if (response.left(7)==QString("Album: ")) {
                    album = response.right(response.length()-7);
                    album.chop(1);
                }
                else if (response.left(6)==QString("file: ")) {
                    fileuri = response.right(response.length()-6);
                    fileuri.chop(1);
                }
                else if (response.left(7)==QString("Track: "))
                {
                    tracknrstring = response.right(response.length()-7);
                    tracknrstring.chop(1);
                    //tracknr = tracknrstring.toInt();
                    QStringList tempstrs = tracknrstring.split("/");
                    if(tempstrs.length()==2)
                    {
                        tracknr = tempstrs.first().toUInt();
                        albumtrackcount = tempstrs.at(1).toUInt();

                    }
                    if(tempstrs.length()==1)
                    {
                        tracknr = tracknrstring.toUInt();
                    }
                }
            }
        }

        status_struct tempstat;
        if ((runtime!=0)&&(elapsed!=0)) {
            tempstat.percent = ((elapsed*100)/runtime);
        }
        else {
            tempstat.percent=0;
        }

        //TODO Samplerate,channel count, bit depth
        tempstat.info = "Bitrate: "+bitrate.toUtf8();
        tempstat.album = album;
        tempstat.title = title;
        tempstat.artist = artist;
        tempstat.id = playlistid;
        tempstat.volume = volume;
        tempstat.playing = playing;
        tempstat.playlistversion = playlistversion;
        tempstat.playlistlength = playlistlength;
        tempstat.repeat = repeat;
        tempstat.shuffle = random;
        tempstat.currentpositiontime=elapsed;
        tempstat.length = runtime;
        tempstat.fileuri=fileuri;
        tempstat.tracknr = tracknr;
        tempstat.bitrate = bitrate.toUInt();
        tempstat.albumtrackcount = albumtrackcount;
        tempstat.bitdepth = bitdepth;
        tempstat.samplerate = samplerate;
        tempstat.channelcount = channelcount;
        if(mPlaylistversion!=tempstat.playlistversion)
        {
            getCurrentPlaylistTracks();
        }
        mPlaylistversion=tempstat.playlistversion;
        return tempstat;
    }
    status_struct tempstat;
    tempstat.repeat=false;
    tempstat.shuffle=false;
    tempstat.id = -1;
    tempstat.percent = 0;
    tempstat.album="";
    tempstat.info="";
    tempstat.title="";
    tempstat.artist="";
    tempstat.volume=0;
    tempstat.playlistversion = 0;
    tempstat.playing = NetworkAccess::STOP;
    tempstat.currentpositiontime=0;
    tempstat.length=0;
    tempstat.fileuri="";
    tempstat.tracknr = 0;
    tempstat.bitrate = 0;
    tempstat.albumtrackcount = 0;
    tempstat.playlistlength = 0;
    tempstat.bitdepth = 0;
    tempstat.samplerate = 0;
    tempstat.channelcount = 0;
    if(mPlaylistversion!=tempstat.playlistversion)
    {
        getCurrentPlaylistTracks();
    }
    mPlaylistversion=tempstat.playlistversion;
    return tempstat;
}


void NetworkAccess::pause()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        status_struct tempstat = getStatus();
        if(tempstat.playing!=NetworkAccess::STOP) {
            QTextStream outstream(tcpsocket);
            outstream << "pause" << endl;
            QString response ="";
            while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
            {
                tcpsocket->waitForReadyRead(READYREAD);
                while (tcpsocket->canReadLine())
                {
                    response = QString::fromUtf8(tcpsocket->readLine());

                }
            }
            updateStatusInternal();
        }
        else {
            playTrackByNumber(tempstat.id);
        }
    }
    
}

void NetworkAccess::stop()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "stop" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::enableOutput(int nr)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "enableoutput " << nr << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
}

void NetworkAccess::disableOutput(int nr)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "disableoutput " << nr << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
}
void NetworkAccess::updateDB()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "update" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::next()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "next" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::previous()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "previous" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::addAlbumToPlaylist(QString album)
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QList<MpdTrack*> *temptracks = new QList<MpdTrack*>();
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        QString response ="";

        temptracks = getAlbumTracks_prv(album);
        //Add Tracks to Playlist
        outstream.setCodec("UTF-8");
        outstream << "command_list_begin" << endl;
        for (int i=0;i<temptracks->length();i++)
        {
            outstream << "add \"" << temptracks->at(i)->getFileUri() << "\""<< endl;
        }
        outstream << "command_list_end" << endl;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    emit ready();
    //   updateStatusInternal();
}

void NetworkAccess::addArtistAlbumToPlaylist(QString artist, QString album)
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QList<MpdTrack*> *temptracks = new QList<MpdTrack*>();
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        //album.replace(QString("\""),QString("\\\""));
        QString response ="";
        temptracks = getAlbumTracks_prv(album,artist);

        //Add Tracks to Playlist
        outstream.setCodec("UTF-8");
        outstream << "command_list_begin" << endl;
        for (int i=0;i<temptracks->length();i++)
        {
            outstream << "add \"" << temptracks->at(i)->getFileUri() << "\""<< endl;
        }
        outstream << "command_list_end" << endl;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    emit ready();
    //     updateStatusInternal();
}



void NetworkAccess::addArtistAlbumToPlaylist(QVariant albuminfo)
{
    QStringList strings = albuminfo.toStringList();
    addArtistAlbumToPlaylist(strings[0],strings[1]);
}

void NetworkAccess::playArtistAlbum(QVariant albuminfo)
{
    clearPlaylist();
    addArtistAlbumToPlaylist(albuminfo);
    playTrackByNumber(0);
    setRandom(false);
    setRepeat(false);
}

void NetworkAccess::playArtistAlbum(QString artist, QString album)
{
    clearPlaylist();
    addArtistAlbumToPlaylist(artist,album);
    playTrackByNumber(0);
    setRandom(false);
    setRepeat(false);
}

void NetworkAccess::playAlbum(QString album)
{
    clearPlaylist();
    addAlbumToPlaylist(album);
    playTrackByNumber(0);
    setRandom(false);
    setRepeat(false);
}


void NetworkAccess::addTrackToPlaylist(QString fileuri)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "add \"" << fileuri << "\"" << endl;
        QString response ="";
        //Clear read buffer
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    updateStatusInternal();
}

void NetworkAccess::playTrackNext(int index)
{
    qWarning() << "PlaynextTrack";
    int currentPosition = getStatus().id;
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        if ( index < currentPosition) {
            outstream << "move " << QString::number(index) << " " << QString::number(currentPosition) << endl;
        } else {
            outstream << "move " << QString::number(index) << " " << QString::number(currentPosition + 1) << endl;
        }
        QString response ="";
        //Clear read buffer
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    updateStatusInternal();
}

void NetworkAccess::addTrackAfterCurrent(QString fileuri)
{
    int currentPosition = getStatus().id;
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "addid \"" << fileuri << "\" " << QString::number(currentPosition+1) << endl;
        QString response ="";
        //Clear read buffer
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    updateStatusInternal();
}

//Replace song with uri and plays it back
void NetworkAccess::playFiles(QString fileuri)
{
    clearPlaylist();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "add \"" << fileuri << "\"" << endl;
        QString response ="";
        //Clear read buffer
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        //Get song id in playlist


        playTrackByNumber(0);
    }
    updateStatusInternal();
}


// Append track to playlist and plays it.
void NetworkAccess::playTrack(QString fileuri)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "add \"" << fileuri << "\"" << endl;
        QString response ="";
        //Clear read buffer
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        //Get song id in playlist

        
        playTrackByNumber(getStatus().playlistlength-1);
    }
    updateStatusInternal();
}

void NetworkAccess::playTrackByNumber(int nr)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "play " << QString::number(nr).toUtf8() << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::deleteTrackByNumer(int nr)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "delete " << QString::number(nr).toUtf8() << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::seekPosition(int id, int pos)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "seek " << QString::number(id).toUtf8() <<" " <<  QString::number(pos).toUtf8() << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::seek(int pos)
{
    seekPosition(getStatus().id,pos);
}



void NetworkAccess::setRepeat(bool repeat)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "repeat " << (repeat ? "1":"0") << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
}

void NetworkAccess::setRandom(bool random)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "random " << (random ? "1":"0") << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();

    }

}

void NetworkAccess::setVolume(int volume)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "setvol " << QString::number(volume).toUtf8() << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
}

void NetworkAccess::savePlaylist(QString name)
{
    emit ready();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "save \"" << name << "\"" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        if (response.left(2)=="OK")
        {
            emit ready();
            return;
        }
        else {
            emit ready();
            return;
        }

    }
    emit ready();
    return;
}

void NetworkAccess::deletePlaylist(QString name)
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "rm \"" << name << "\"" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        if (response.left(2)=="OK")
        {
            return;
        }
        else {
            return;
        }

    }
    return;
}

void NetworkAccess::getSavedPlaylists()
{
    emit busy();
    QStringList *tempplaylists  = new QStringList();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "listplaylists" <<endl;
        QString response ="";
        QString name;

        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(10)==QString("playlist: ")) {
                    name = response.right(response.length()-10);
                    name.chop(1);
                    tempplaylists->append(name);
                }
            }
        }

    }
    emit ready();
    emit savedPlaylistsReady(tempplaylists);
}


void NetworkAccess::addPlaylist(QString name)
{
    emit busy();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "load \"" << name << "\"" <<endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    emit ready();
}

void NetworkAccess::playPlaylist(QString name)
{
    emit busy();
    clearPlaylist();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "load \"" << name << "\"" <<endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
    }
    playTrackByNumber(0);
    emit ready();
}

void NetworkAccess::clearPlaylist()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream << "clear" << endl;
        QString response ="";
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());

            }
        }
        updateStatusInternal();
    }
}

void NetworkAccess::disconnectedfromServer()
{
    if (statusupdater->isActive())
    {
        statusupdater->stop();
    }
    emit ready();
}

void NetworkAccess::connectedtoServer() {
    statusupdater->start(updateinterval);
}

quint32 NetworkAccess::getPlayListVersion()
{
    quint32 playlistversion = 0;
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "status" << endl;
        QString response ="";
        QString versionstring;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(10)==QString("playlist: ")) {
                    versionstring = response.right(response.length()-10);
                    versionstring.chop(1);
                    playlistversion = versionstring.toUInt();
                }
            }
        }
    }
    return playlistversion;
}

void NetworkAccess::getDirectory(QString path)
{
    emit busy();
    QList<MpdFileEntry*> *tempfiles = new QList<MpdFileEntry*>();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        path.replace(QString("\""),QString("\\\""));

        QTextStream outstream(tcpsocket);

        outstream.setCodec("UTF-8");

        outstream << "lsinfo \"" << path << "\"" << endl;
        QString response ="";

        MpdTrack *temptrack=NULL;
        MpdFileEntry *tempfile=NULL;
        QString title="";
        QString artist="";
        QString album="";
        QString albumstring="";
        QString datestring = "";
        int nr=0;
        int albumnrs=0;
        QString file="";
        QString filename="";
        QString prepath="";
        QStringList tempsplitter;
        quint32 length=0;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                //New file: so new track begins in mpds output
                if (response.left(6)==QString("file: ")) {
                    if (file!=""&&length!=0)
                    {
                        tempsplitter = file.split("/");
                        if (tempsplitter.length()>0)
                        {
                            temptrack = new MpdTrack(NULL,file,title,artist,album,length);
                            temptrack->setYear(datestring);
                            temptrack->setTrackNr(nr);
                            temptrack->setAlbumTracks(albumnrs);
                            prepath ="";
                            for (int j=0;j<tempsplitter.length()-1;j++)
                            {
                                prepath += tempsplitter.at(j);
                                if (j!=tempsplitter.length()-2)
                                {
                                    prepath += "/";
                                }

                            }
                            tempfile = new MpdFileEntry(prepath,tempsplitter.last(),MpdFileEntry::MpdFileType_File,temptrack,NULL);
                            tempfiles->append(tempfile);
                            temptrack->moveToThread(mQmlThread);
                            tempfile->moveToThread(mQmlThread);
                            QQmlEngine::setObjectOwnership(temptrack, QQmlEngine::CppOwnership);
                            QQmlEngine::setObjectOwnership(tempfile, QQmlEngine::CppOwnership);
                            tempsplitter.clear();
                        }
                        artist= "";
                        albumstring="";
                        length=0;
                        album="";
                        title="";
                        filename="";
                        nr=0;
                        datestring="";
                        albumnrs=0;
                    }
                    file = response.right(response.length()-6);
                    file.chop(1);
                }
                if (response.left(7)==QString("Title: ")) {
                    title = response.right(response.length()-7);
                    title.chop(1);
                }
                if (response.left(8)==QString("Artist: ")) {
                    artist = response.right(response.length()-8);
                    artist.chop(1);
                }
                if (response.left(7)==QString("Album: ")) {
                    albumstring = response.right(response.length()-7);
                    albumstring.chop(1);
                    album = albumstring;
                }
                if (response.left(6)==QString("Time: ")) {
                    albumstring = response.right(response.length()-6);
                    albumstring.chop(1);
                    length = albumstring.toUInt();
                }
                if (response.left(6)==QString("Date: ")) {
                    datestring = response.right(response.length()-6);
                    datestring.chop(1);
                }
                if (response.left(7)==QString("Track: ")) {
                    albumstring = response.right(response.length()-7);
                    albumstring.chop(1);
                    QStringList tracknrs;
                    tracknrs = albumstring.split('/');
                    if(tracknrs.length()>0)
                    {
                        nr = tracknrs.at(0).toInt();
                        if(tracknrs.length()>1)
                            albumnrs = tracknrs.at(1).toInt();
                    }
                }
                //Directory found. WORKS
                if (response.left(11)==QString("directory: "))
                {
                    filename = response.right(response.length()-11);
                    filename.chop(1);
                    tempsplitter = filename.split("/");
                    if (tempsplitter.length()>0)
                    {
                        prepath ="";
                        for (int j=0;j<tempsplitter.length()-1;j++)
                        {
                            prepath += tempsplitter.at(j);
                            if (j!=tempsplitter.length()-2)
                            {
                                prepath += "/";
                            }

                        }
                        tempfile = new MpdFileEntry(path,tempsplitter.last(),1,NULL,NULL);
                        tempfiles->append(tempfile);
                        tempfile->moveToThread(mQmlThread);
                        QQmlEngine::setObjectOwnership(tempfile, QQmlEngine::CppOwnership);
                        filename = "";
                        tempsplitter.clear();
                    }

                }
                if (response.left(10)==QString("playlist: "))
                {
                    filename = response.right(response.length()-10);
                    filename.chop(1);
                    tempsplitter = filename.split("/");
                    if (tempsplitter.length()>0)
                    {
                        prepath ="";
                        for (int j=0;j<tempsplitter.length()-1;j++)
                        {
                            prepath += tempsplitter.at(j);
                            if (j!=tempsplitter.length()-2)
                            {
                                prepath += "/";
                            }

                        }
                        tempfile = new MpdFileEntry(path,tempsplitter.last(),MpdFileEntry::MpdFileType_Playlist,NULL,NULL);
                        tempfiles->append(tempfile);
                        tempfile->moveToThread(mQmlThread);
                        filename = "";
                        tempsplitter.clear();
                    }

                }
            }
        }
        //LAST FILE ADD
        if (file!=""&&title!=""&&length!=0)
        {
            tempsplitter = file.split("/");
            if (tempsplitter.length()>0)
            {
                temptrack = new MpdTrack(NULL,file,title,artist,album,length);
                temptrack->setTrackNr(nr);
                temptrack->setAlbumTracks(albumnrs);
                temptrack->setYear(datestring);
                prepath ="";
                for (int j=0;j<tempsplitter.length()-1;j++)
                {
                    prepath += tempsplitter.at(j);
                    if (j!=tempsplitter.length()-2)
                    {
                        prepath += "/";
                    }

                }
                qDebug() << "Last album: " << albumstring;
                tempfile = new MpdFileEntry(prepath,tempsplitter.last(),MpdFileEntry::MpdFileType_File,temptrack,NULL);
                tempfiles->append(tempfile);
                temptrack->moveToThread(mQmlThread);
                tempfile->moveToThread(mQmlThread);
                QQmlEngine::setObjectOwnership(temptrack, QQmlEngine::CppOwnership);
                QQmlEngine::setObjectOwnership(tempfile, QQmlEngine::CppOwnership);
                tempsplitter.clear();
            }
        }
    }
    qSort(tempfiles->begin(),tempfiles->end(),MpdFileEntry::lessThan);
    emit ready();
    emit filesReady((QList<QObject*>*)tempfiles);
    //    return tempfiles;
}

void NetworkAccess::resumeUpdates()
{
    if (tcpsocket->state()==QTcpSocket::ConnectedState) {
        updateStatusInternal();
        statusupdater->start(updateinterval);
    }
}

void NetworkAccess::suspendUpdates()
{
    if (statusupdater->isActive())
    {
        statusupdater->stop();
    }
}

void NetworkAccess::setUpdateInterval(int ms)
{
    updateinterval = ms;
    if (statusupdater->isActive())
    {
        statusupdater->stop();
        statusupdater->start(updateinterval);
    }
    if(ms==0){
        statusupdater->stop();
    }
}

bool NetworkAccess::connected()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState)
    {
        return true;
    } else {
        return false;
    }
}

void NetworkAccess::errorHandle()
{
    tcpsocket->disconnectFromHost();
}

void NetworkAccess::addArtist(QString artist)
{
    QList<MpdAlbum*> *albums = getArtistsAlbums_prv(artist);
    for(int i=0;i<albums->length();i++)
    {

        addArtistAlbumToPlaylist(artist,albums->at(i)->getTitle());
    }
}

void NetworkAccess::playArtist(QString artist)
{
    clearPlaylist();
    addArtist(artist);
    playTrackByNumber(0);
}

void NetworkAccess::setConnectParameters(QString hostname, int port, QString password)
{
    this->hostname = hostname;
    this->password = password;
    this->port = port;
}

void NetworkAccess::connectToHost()
{
    connectToHost(hostname,port,password);
}

QList<MpdTrack*>* NetworkAccess::parseMPDTracks(QString cartist)
{
    QList<MpdTrack*> *temptracks = new QList<MpdTrack*>();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        QString response ="";

        MpdTrack *temptrack=NULL;
        QString title;
        QString artist;
        QString albumartist;
        QString albumstring;
        QString datestring;
        int nr,albumnrs;
        nr = albumnrs = 0;
        QString file;
        quint32 length=0;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            if (!tcpsocket->waitForReadyRead(READYREAD))
            {
            }

            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(6)==QString("file: ")) {
                    if (temptrack!=NULL)
                    {
                        if (artist==cartist||cartist=="") {
                            temptrack->setPlaying(false);
                            temptracks->append(temptrack);
                            temptrack->moveToThread(mQmlThread);
                            QQmlEngine::setObjectOwnership(temptrack, QQmlEngine::CppOwnership);
                        } else {
                            delete(temptrack);
                        }
                        temptrack=NULL;
                    }
                    if (temptrack==NULL)
                    {
                        temptrack = new MpdTrack(NULL);
                    }
                    file = response.right(response.length()-6);
                    file.chop(1);
                    temptrack->setFileUri(file);
                }
                if (response.left(7)==QString("Title: ")) {
                    title = response.right(response.length()-7);
                    title.chop(1);
                    temptrack->setTitle(title);
                }
                if (response.left(8)==QString("Artist: ")) {
                    artist = response.right(response.length()-8);
                    artist.chop(1);
                    temptrack->setArtist(artist);
                }
                if (response.left(13)==QString("AlbumArtist: ")) {
                    albumartist = response.right(response.length()-13);
                    albumartist.chop(1);
                    temptrack->setAlbumArtist(albumartist);
                }
                if (response.left(7)==QString("Album: ")) {
                    albumstring = response.right(response.length()-7);
                    albumstring.chop(1);
                    temptrack->setAlbum(albumstring);
                }

                if (response.left(6)==QString("Time: ")) {
                    albumstring = response.right(response.length()-6);
                    albumstring.chop(1);
                    length = albumstring.toUInt();
                    temptrack->setLength(length);
                }
                if (response.left(6)==QString("Date: ")) {
                    datestring = response.right(response.length()-6);
                    datestring.chop(1);
                    temptrack->setYear(datestring);
                }
                if (response.left(7)==QString("Track: ")) {
                    albumstring = response.right(response.length()-7);
                    albumstring.chop(1);
                    QStringList tracknrs;
                    tracknrs = albumstring.split('/');
                    if(tracknrs.length()>0)
                    {
                        nr = tracknrs.at(0).toInt();
                        if(tracknrs.length()>1) {
                            albumnrs = tracknrs.at(1).toInt();
                            temptrack->setAlbumTracks(albumnrs);
                        }
                    }
                    temptrack->setTrackNr(nr);
                }
            }

        }
        if (temptrack!=NULL)
        {
            if (artist==cartist||cartist=="") {
                temptrack->setPlaying(false);
                temptracks->append(temptrack);
                temptrack->moveToThread(mQmlThread);
                QQmlEngine::setObjectOwnership(temptrack, QQmlEngine::CppOwnership);
            }
            else {
                delete(temptrack);
                temptrack = NULL;
            }
        }
    }
    return temptracks;
}

void NetworkAccess::exitRequest()
{
    this->disconnect();
    emit requestExit();
}

void NetworkAccess::getOutputs()
{
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {
        emit busy();
        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        QString response ="";
        QString tempstring;
        QList<MPDOutput*> *outputlist = new QList<MPDOutput*>();
        QString outputname;
        int outputid;
        bool outputenabled;


        outstream << "outputs" << endl;
        while ((tcpsocket->state()==QTcpSocket::ConnectedState)&&((response.left(2)!=QString("OK")))&&((response.left(3)!=QString("ACK"))))
        {
            tcpsocket->waitForReadyRead(READYREAD);
            while (tcpsocket->canReadLine())
            {
                response = QString::fromUtf8(tcpsocket->readLine());
                if (response.left(12)==QString("outputname: ")) {
                    tempstring = response.right(response.length()-12);
                    tempstring.chop(1);
                    outputname = tempstring;
                }
                if (response.left(10)==QString("outputid: ")) {
                    tempstring = response.right(response.length()-10);
                    tempstring.chop(1);
                    outputid = tempstring.toInt();
                }
                if (response.left(15)==QString("outputenabled: ")) {
                    tempstring = response.right(response.length()-15);
                    tempstring.chop(1);
                    outputenabled = ( tempstring=="1" ? true:false);
                    MPDOutput *tmpOutput = new MPDOutput(outputname,outputenabled,outputid);
                    outputlist->append(tmpOutput);
                    tmpOutput->moveToThread(mQmlThread);
                    QQmlEngine::setObjectOwnership(tmpOutput, QQmlEngine::CppOwnership);
                }


            }
        }
        emit outputsReady((QList<QObject*>*)outputlist);
        emit ready();
    }

}


void NetworkAccess::searchTracks(QVariant request)
{
    emit busy();
    QStringList searchrequest = request.toStringList();
    if (tcpsocket->state() == QAbstractSocket::ConnectedState) {

        QTextStream outstream(tcpsocket);
        outstream.setCodec("UTF-8");
        outstream << "search " << searchrequest.at(0) << " \"" << searchrequest.at(1) << "\"" << endl;
    }
    emit searchedTracksReady((QList<QObject*>*)parseMPDTracks(""));
    emit ready();
    //return parseMPDTracks("");
}

void NetworkAccess::setQmlThread(QThread *thread)
{
    if(thread)
    {
        mQmlThread = thread;
    }
}

void NetworkAccess::getArtistAlbumMap()
{
    emit busy();
    emit artistsAlbumsMapReady(getArtistsAlbumsMap_prv());
    emit ready();
}

QMap<MpdArtist*, QList<MpdAlbum*>* > *NetworkAccess::getArtistsAlbumsMap_prv()
{
    QMap<MpdArtist*, QList<MpdAlbum*>* > *resMap = new QMap<MpdArtist*, QList<MpdAlbum*>* >();
    QList<MpdArtist*> *artists = getArtists_prv();
    for ( int i = 0; i < artists->length(); i++ ) {
        QList<MpdAlbum*> *albums = getArtistsAlbums_prv(artists->at(i)->getName());
        MpdArtist *tmpArtist = artists->at(i);
        (*resMap)[tmpArtist] = albums;
    }
    return resMap;
}
