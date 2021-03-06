#include "song.h"

#include <cppcms/url_dispatcher.h>
#include <cppcms/json.h>
#include <booster/log.h>

#include <data/song.h>
#include <app/playlist.h>
#include <app/validators/songValidator.h>

#include <sstream>


using namespace cppcms::http;

namespace app {
    Song::Song(cppcms::service& s) :
        app::Master(s),
        dbSongMapper_(new data::SongMapper(connectionString_)),
        dbArtistMapper_(new data::ArtistMapper(connectionString_)),
        dbPlaylistMapper_(new data::PlaylistMapper(connectionString_))
    {
        dispatcher().assign("/edit/(\\d+)", &Song::displayEdit, this, 1);
        mapper().assign("/{1}");

        dispatcher().assign("/proposed", &Song::displayProposed, this);
        mapper().assign("");

        dispatcher().assign("/ajax-new", &Song::ajaxNew, this);
        mapper().assign("");

        dispatcher().assign("/ajax-set-playlist/(\\d+)/(\\d+)", &Song::ajaxSetPlaylist, this, 1, 2);
        mapper().assign("/{1}/{2}");
    }

    void Song::displayProposed() {
        BOOSTER_DEBUG("displayProposed");

        if (! checkAuth(data::User::ADMINISTRATOR)) {
            response().make_error_response(response::forbidden);
            BOOSTER_WARNING("displayProposed") << "Forbid user "
                << page_.user.alias << " to view proposed songs";
            return;
        }

        data::AdminPageSong pageSong;
        pageSong.resetFrom(page_);
        pageSong.pageTitle = "Proposed songs";

        dbSongMapper_->loadPendingSongs(pageSong);
        dbPlaylistMapper_->loadComingPlaylists(pageSong);

        dbPlaylistMapper_->clear();
        dbSongMapper_->clear();

        render("proposedSongs", pageSong);
    }

    void Song::displayEdit(std::string songId) {
        BOOSTER_DEBUG("displayEdit");

        if (! checkAuth(data::User::ADMINISTRATOR)) {
            response().make_error_response(response::forbidden);
            BOOSTER_WARNING("displayEdit") << "Forbid user "
                << page_.user.alias << " to edit song";
            return;
        }

        data::Song song;

        data::PageEditSong pageSong;
        if (request().request_method() == "POST") {
            pageSong.input.load(context());
            if(! pageSong.input.validate()) {
                pageSong.alerts.errors.push_back("Invalid or missing fields!");
            } else {
                song.id = std::stoi(songId);
                song.title = pageSong.input.title.value();
                song.artist = pageSong.input.artist.value();
                song.file = pageSong.input.file.value();
                song.url = pageSong.input.url.value();
                song.showVideo = pageSong.input.showVideo.value();
                song.position = pageSong.input.position.value();
                if (! update(song)) {
                    pageSong.alerts.errors.push_back("Could not modify the song, please retry later!");
                } else {
                    std::ostringstream message;
                    message << "Successfully modified \"" << song.title << "\".";
                    pageSong.alerts.success.push_back(message.str());
                    BOOSTER_INFO("displayEdit") << message.str();
                }
            }
        } else if (!songId.empty() && dbSongMapper_->loadSong(song, songId)) {
            BOOSTER_DEBUG("displayEdit") << "Loading fields for song " << songId;
            pageSong.input.artist.value(song.artist);
            pageSong.input.title.value(song.title);
            pageSong.input.file.value(song.file);
            pageSong.input.url.value(song.url);
            pageSong.input.showVideo.value(song.showVideo);
            pageSong.input.position.value(song.position);
        }


        pageSong.resetFrom(page_);
        pageSong.pageTitle = "Modify a song";

        dbSongMapper_->loadUserProposedSongs(page_.user);
        dbSongMapper_->clear();

        render("editSong", pageSong);
    }

    void Song::ajaxNew() {
        if (! checkAuth(data::User::CITIZEN)) {
            response().make_error_response(response::forbidden);
            BOOSTER_WARNING("ajaxNew") << "Forbid user "
                << page_.user.alias << " to add new song";
            return;
        }

        BOOSTER_DEBUG("ajaxNew");

        data::Song song;
        song.title = request().post("title");
        song.artist = request().post("artist");
        song.url = request().post("url");

        validator::SongValidator validator(song);
        bool success = validator.validate();
        std::string message = validator.lastMessage();

        if (success) {
            success = insert(song);
            std::ostringstream oss;
            if (success) {
                oss << "Successfully proposed \"" << song.title << "\". Thanks for your contribution to the playlist ^_^";
            } else {
                oss << "Could not submit the song, please retry later!";
            }
            message = oss.str();
        }

        if (success) {
            BOOSTER_INFO("ajaxNew") << message;
        } else {
            BOOSTER_ERROR("ajaxNew") << message;
        }

        cppcms::json::value jsonOutput;
        jsonOutput["success"] = success;
        jsonOutput["message"] = message;
        response().out() << jsonOutput;
    }

    void Song::ajaxSetPlaylist(std::string songId, std::string playlistId) {
        if (! checkAuth(data::User::ADMINISTRATOR)) {
            response().make_error_response(response::forbidden);
            BOOSTER_WARNING("ajaxSetPlaylist") << "Forbid user "
                << page_.user.alias << " to set song playlist";
            return;
        }

        BOOSTER_DEBUG("ajaxSetPlaylist");

        bool result = dbSongMapper_->setSongPlaylist(
            std::stoi(songId),
            std::stoi(playlistId)
        );
        dbSongMapper_->clear();

        std::string key = Playlist::getCacheKey(playlistId, page_.user);
        cache().rise(key);
        BOOSTER_DEBUG("ajaxSetPlaylist") << "Clean cache for key=" << key;

        cppcms::json::value jsonOutput;
        jsonOutput["success"] = result;
        response().out() << jsonOutput;
    }

    bool Song::insert(const data::Song& song) {
        bool success = false;

        unsigned int artistId = dbArtistMapper_->getBySimilarName(song.artist);
        if (artistId == 0) {
            artistId = dbArtistMapper_->insert(song.artist);
        }

        if (artistId == 0) {
            BOOSTER_DEBUG("addNew") << "Could not create artist " << song.artist;
        } else {
            success = dbSongMapper_->insert(page_.user, artistId, song);
        }

        dbArtistMapper_->clear();
        dbSongMapper_->clear();
        return success;
    }

    bool Song::update(const data::Song& song) {
        bool success = false;

        unsigned int artistId = dbArtistMapper_->getByName(song.artist);
        if (artistId == 0) {
            artistId = dbArtistMapper_->insert(song.artist);
        }

        if (artistId == 0) {
            BOOSTER_DEBUG("update") << "Could not create artist " << song.artist;
        } else {
            success = dbSongMapper_->update(artistId, song);
        }

        dbArtistMapper_->clear();
        dbSongMapper_->clear();
        return success;
    }

}   // namespace app

