#ifndef USER_H
#define USER_H

#include "database.h"

#include <string>
#include <vector>
#include <optional>
#include "Poco/JSON/Object.h"

namespace database
{
    class User{
        public:
            std::string login;
            std::string first_name;
            std::string last_name;
            int age;

            Poco::JSON::Object::Ptr toJSON() const;
            static User fromJSON(const std::string & str);

            static void init();
            static std::optional<User> read_by_login( std::string login);
            void send_to_queue();
            bool save_to_mysql();
    };
}

#endif
