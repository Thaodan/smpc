import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

ApplicationWindow
{
    id: mainWindow

    title: qsTr("SMPC")
    width: 640
    height: 480
    visible: true

    // Signals for c++<->qml communication
    signal setHostname(string hostname);
    signal setPort(int port);
    signal setPassword(string password);
    signal setVolume(int volume);
    signal setCurrentArtist(string artist);
    signal connectToServer();

    // Signals for database requests to networkaccess
    //Variant in format [artistname,albumname]
    signal addAlbum(variant album);
    signal addArtist(string artist);
    signal addFiles(string files);
    signal addSong(string uri);
    signal addSongToSaved(variant data);
    signal removeSongFromSaved(variant data);
    signal addSongAfterCurrent(string uri);
    signal addPlaylist(string name);
    signal playPlaylist(string name);
    signal playAlbum(variant album);
    signal playArtist(string artist);
    signal playFiles(string uri);
    signal playPlaylistSongNext(int index);
    //appends song to playlist
    signal playSong(string uri);
    // Adds all tracks from last search result to playlist
    signal addlastsearch();
    signal requestSavedPlaylists();
    signal requestSavedPlaylist(string name);
    signal requestAlbums();
    signal requestAlbum(variant album);
    signal requestArtists();
    signal requestArtistAlbums(string artist);
    signal requestFilesPage(string files);
    signal requestFilesModel(string files);
    signal requestCurrentPlaylist();
    signal requestOutputs();
    signal requestSearch(variant request);
    // Request an MPD database update (remote mpd db)
    signal updateDB();


    // Signals controller that stack of filemodels can be wiped
    signal popfilemodelstack();
    signal cleanFileStack();

    // Controller model clear signals (memory cleanup)
    signal clearAlbumList();
    signal clearArtistList();
    signal clearTrackList();
    signal clearPlaylistTracks();
    signal clearPlaylists();
    signal clearSearchTracks();

    // MPD Output controls
    signal enableOutput(int nr);
    signal disableOutput(int nr);

    // Control signals
    signal play();
    signal next();
    signal prev();
    signal stop();
    signal seek(int position);
    signal setRepeat(bool rep);
    signal setShuffle(bool shfl);

    //Playlist signals
    signal savePlaylist(string name);
    signal deletePlaylist();
    signal deleteSavedPlaylist(string name);
    signal playPlaylistTrack(int index);
    signal deletePlaylistTrack(int index);

    // Changes server profile or creates new one
    signal changeProfile(variant profile);
    signal newProfile(variant profile);
    signal deleteProfile(int index);
    // Requests controller to connect to MPD host
    signal connectProfile(int index);

    // Database stuff (local-metadata DB)
    signal bulkDownloadArtists();
    signal bulkDownloadAlbums();
    signal cleanupBlacklisted();
    signal cleanupArtists();
    signal cleanupAlbums();
    signal cleanupDB();

    // Wiki/Biography information requests for sqlDB
    signal requestArtistInfo(string artist);
    signal requestAlbumInfo(variant album);

    // Signals change in download-size preference
    signal newDownloadSize(int size)

    // Signals changed settings key
    // [settingkey,value]
    signal newSettingKey(variant setting);

    signal wakeUpServer(int index);

    // Propertys of application
    property string hostname;

    property int port;
    property string password;
    property int listfontsize : 12;
    property int liststretch : 20;
    property int lastsongid : -1;
    property string playbuttoniconsource : "image://theme/icon-m-play";
    property string playbuttoniconsourcecover : "image://theme/icon-cover-play";
    property string volumebuttoniconsource;
    property string lastpath;
    property string artistname;
    property string albumname;
    property string playlistname;
    property string coverimageurl;
    property string artistimageurl;
    property string profilename;
    property bool repeat;
    property bool shuffle;
    property bool quitbtnenabled;
    property bool connected;
    property bool playing : false;
    property bool stopped : false;
    property bool fastscrollenabled : false;

    property int populateDuration: 500

    property bool volumeChanging:false

    // current song information
    property string mTitle
    property string mArtist
    property string mAlbum
    property int mVolume
    property int mLength:0.0
    property int mPosition:0
    property int mPlaylistlength : 0;
    property bool mRepeat
    property bool mShuffle
    property bool mDebugEnabled
    property bool mPositionSliderActive:false;
    property string mAudioProperties;
    property string mTrackNr;
    property string mBitrate;
    property string mUri;
    property string mLengthText;

    property bool jollaQuickscroll: false

    // JS-functions
    //TODO separation

    function slotConnected(profile)
    {
        profilename = profile;
        connected = true;
    }

    function slotDisconnected()
    {
        connected = false;
        profilename = "";
        playing = false;
    }

    function busy()
    {
        busyIndicator.running = true;
        inputBlock.enabled = true
    }

    function ready()
    {
        busyIndicator.running = false;
        inputBlock.enabled = false
    }

    function updateCurrentPlaying(list)
    {
        mTitle = list[0];
        mAlbum = list[1];
        mArtist = list[2];
        if(mPositionSliderActive==false) {
            mPosition = list[3];
        }
        mLength = list[4];
        //currentsongpage.lengthtextcurrent = formatLength(list[3]);
        mLengthText = list[4]==0 ? "": formatLength(list[4]);
        mBitrate = list[5]+"kbps";
        playbuttoniconsource = (list[6]=="playing") ? "image://theme/icon-m-pause" : "image://theme/icon-m-play";
        playbuttoniconsourcecover = (list[6]=="playing") ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play";
        playing = (list[6]=="playing") ? true : false;
        stopped = (list[6]=="stop") ? true : false;
        if(!volumeChanging){
            mVolume = list[7];
        }
        mRepeat = (list[8]=="0" ?  false:true);
        mShuffle = (list[9]=="0" ?  false:true);
        mTrackNr = (list[10]==0? "":list[10]);
        mUri = list[11];
        if(list[12]!=lastsongid)
        {
            lastsongid = list[12];
        }
        if(stopped) {
            coverimageurl = "";
            artistimageurl = "";
            mArtist=""
            mAlbum=""
            mTitle=""
        }
        mPlaylistlength = list[16];
        mAudioProperties = list[13]+ "Hz "+ list[14] + "Bits " + list[15]+ "Channels";
    }

    function savedPlaylistClicked(modelData)
    {
        playlistname = modelData;
        requestSavedPlaylist(modelData);
    }


    function filesClicked(path)
    {
        lastpath = path;
        requestFilesPage(path);
    }

    function updatePlaylist()
    {
        //  blockinteraction.enabled=false;
        console.debug("setting new playlist");
        // playlistModelVar = playlistModel;
        console.debug("received new playlist and set model");
    }


    function albumTrackClicked(title,album,artist,lengthformatted,uri,year,tracknr,trackmbid,artistmbid,albummbid)
    {
        pageStack.push(Qt.resolvedUrl("components/SongDialog.qml"),{title:title,album:album,artist:artist,filename:uri,lengthtext:lengthformatted,date:year,nr:tracknr,trackmbid:trackmbid,artistmbid:artistmbid,albummbid:albummbid});
    }

    function receiveFilesPage()
    {
        pageStack.push("pages/database/FileBrowserPage.qml",{listmodel: filesModel,filepath :lastpath});
        fastscrollenabled = true;
    }

    function popCleared() {
        console.debug("POP cleared");
        fastscrollenabled = true;
    }

    // Nicely formats length values to string
    function formatLength(length)
    {
        if ( length === 0 ) {
            return "0:00"
        }

        var temphours = Math.floor(length/3600);
        var min = 0;
        var sec = 0;
        var temp="";
        if(temphours>1)
        {
            min=(length-(3600*temphours))/60;
        }
        else{
            min=Math.floor(length/60);
        }
        sec = length-temphours*3600-min*60;
        if(temphours===0)
        {
            temp=((min<10?"0":"")+min)+":"+(sec<10?"0":"")+(sec);
        }
        else
        {
            temp=((temphours<10?"0":"")+temphours)+":"+((min<10?"0":"")+min)+":"+(sec<10?"0":"")+(sec);
        }
        return temp;
    }

    function albumClicked(artist,albumstring)
    {
        requestAlbum([artist,albumstring]);
        artistname = artist;
        albumname = albumstring;
    }

    function artistClicked(item)
    {
        mainWindow.artistname = item;
        requestArtistAlbums(item);
    }

    function coverArtReceiver(url)
    {
        coverimageurl = url;
    }

    function coverArtistArtReceiver(url)
    {
        artistimageurl = url;
    }

    function jumpNowPlaying()
    {
        pageStack.clear();
        pageStack.push(initialPage);
        pageStack.push();
        pageStack.push(Qt.resolvedUrl("pages/database/CurrentPlaylistPage.qml"));
        pageStack.push(Qt.resolvedUrl("pages/database/CurrentSong.qml"));
    }

    // Notifies user about ongoing action in netaccess
    BusyIndicator
    {
        id:busyIndicator
        anchors.centerIn: parent
    }

    // Prevents clicking if active request is running
    MouseArea
    {
        id: inputBlock
        anchors.fill: parent
        preventStealing: true
        enabled: false
    }

    Button {
        id: testBtn
        anchors.centerIn: parent
        width: parent.width
        height: 100
        text: "Test"
    }

    MessageDialog {
        id: messageDialog
        title: qsTr("May I have your attention, please?")

        function show(caption) {
            messageDialog.text = caption;
            messageDialog.open();
        }
    }

}
