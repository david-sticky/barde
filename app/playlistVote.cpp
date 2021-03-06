#include "playlistVote.h"

#include <cppcms/http_response.h>
#include <cppcms/json.h>
#include <cppcms/url_dispatcher.h>
#include <booster/log.h>

#include <app/playlist.h>


using namespace cppcms::http;

namespace app {
    PlaylistVote::PlaylistVote(cppcms::service& s) :
        app::Master(s), dbMapper_(new data::PlaylistVoteMapper(connectionString_))
    {
        dispatcher().assign("/ajax-vote/(\\d+)/(.+)", &PlaylistVote::ajaxVote, this, 1, 2);
        mapper().assign("/{1}/{2}");
    }

    void PlaylistVote::ajaxVote(std::string playlistId, std::string vote) {
        if (! checkAuth(data::User::CITIZEN)) {
            response().make_error_response(response::forbidden);
            BOOSTER_WARNING("ajaxVote") << "Forbid user "
                << page_.user.alias << " to vote for playlist";
            return;
        }

        bool result = dbMapper_->saveVote(
            page_.user.id,
            playlistId,
            data::PlaylistVote::stringToVote(vote)
        );
        dbMapper_->clear();

        std::string key = Playlist::getCacheKey(playlistId, page_.user);
        cache().rise(key);
        BOOSTER_DEBUG("ajaxVote") << "Clean cache for key=" << key;

        cppcms::json::value jsonOutput;
        jsonOutput["success"] = result;
        response().out() << jsonOutput;
    }

}   // namespace app
