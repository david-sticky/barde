#ifndef DATA_USER_H
#define DATA_USER_H

#include <cppcms/view.h>

#include <string>
#include <vector>

namespace data {

    struct User : public cppcms::base_content {
        struct ProposedSong {
            struct Playlist {
                std::string id;
                std::string name;
                bool enabled;
            };

            unsigned int id;
            std::string title;
            std::string artist;
            std::string url;

            Playlist playlist;
        };

        enum Level {
            ANONYMOUS = 0,  // No authentification
            GUEST = 1,
            CITIZEN = 10,
            ADMINISTRATOR = 200,
            ROOT = 255
        };

        unsigned int id;
        std::string alias;
        unsigned int level;
        bool isAuthenticated;
        bool isAllowed;

        // Songs the user proposed
        std::vector<ProposedSong> proposedSongs;


        User() {
            clear();
        }

        void clear() {
            id = 0;
            alias = "Anonymous";
            level = data::User::ANONYMOUS;
            isAuthenticated = false;
            isAllowed = true;
        }
    };

}   // namespace data


#endif  // DATA_USER_H;
