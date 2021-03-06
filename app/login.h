#ifndef APP_LOGIN_H
#define APP_LOGIN_H

#include <app/master.h>
#include <data/login.h>
#include <data/loginMapper.h>

namespace app {
    class Login : public Master {

        data::PageLogin login_;
        std::unique_ptr<data::LoginMapper> dbMapper_;

    public:
        Login(cppcms::service& s);
        void display();

    };
}   // namespace app

#endif  // APP_LOGIN_H

