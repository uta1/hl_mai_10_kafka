#include "user.h"
#include "../config/config.h"

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SessionFactory.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/LogStream.h>
#include <Poco/Logger.h>
#include <cppkafka/cppkafka.h>

#include <sstream>
#include <exception>

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;
using Poco::Logger;
using Poco::LogStream;

namespace database
{

    Poco::JSON::Object::Ptr User::toJSON() const
    {
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("login", login);
        root->set("first_name", first_name);
        root->set("last_name", last_name);
        root->set("age", age);

        return root;
    }

    User User::fromJSON(const std::string &str)
    {
        User user;
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

        user.login = object->getValue<std::string>("login");
        user.first_name = object->getValue<std::string>("first_name");
        user.last_name = object->getValue<std::string>("last_name");
        user.age = object->getValue<int>("age");

        return user;
    }

    void User::init()
    {
        try
        {

            Poco::Data::Session session = database::Database::get().create_session_write();
            //*
            Statement drop_stmt(session);
            drop_stmt << "DROP TABLE IF EXISTS users", now;
            //*/

            // (re)create table
            Statement create_stmt(session);
            create_stmt << "CREATE TABLE IF NOT EXISTS users ("
                           "   login CHAR(50) NOT NULL PRIMARY KEY,"
                           "   first_name CHAR(50) NOT NULL,"
                           "   last_name CHAR(50) NOT NULL,"
                           "   age INT NOT NULL"
                           " );",
                           now;
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
    }

    std::optional<User> User::read_by_login(std::string login)
    {
        try
        {
            auto db = database::Database::get();
            Poco::Data::Session session = db.create_session_read();
            Poco::Data::Statement select(session);

            std::string select_str = "SELECT login, first_name, last_name, age FROM users WHERE login=? ";

            User a;
            select << select_str,
                    into(a.login),
                    into(a.first_name),
                    into(a.last_name),
                    into(a.age),
                    use(login),
                    range(0, 1); //  iterate over result set one row at a time

            if (!select.done())
            {
                if (!select.execute()) return std::nullopt;
            }

            return a;
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
    }

    void User::send_to_queue()
    {
        cppkafka::Configuration config = {
            {"metadata.broker.list", Config::get().get_queue_host()}};

        cppkafka::Producer producer(config);
        std::stringstream ss;
        Poco::JSON::Stringifier::stringify(toJSON(), ss);
        std::string message = ss.str();
        producer.produce(cppkafka::MessageBuilder(Config::get().get_queue_topic()).partition(0).payload(message));
        producer.flush();
    }

    bool User::save_to_mysql()
    {

        try
        {
            auto db = database::Database::get();
            Poco::Data::Session session = db.create_session_write();
            Poco::Data::Statement insert(session);

            std::string select_str = "INSERT INTO users (login, first_name, last_name, age) VALUES(?, ?, ?, ?) ";
            insert << select_str,
                use(login),
                use(first_name),
                use(last_name),
                use(age);

            insert.execute();

            return true;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
        }

        return false;
    }
}
