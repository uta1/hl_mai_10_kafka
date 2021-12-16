#ifndef USERHANDLER_H
#define USERHANDLER_H

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include <iostream>
#include <iostream>
#include <fstream>

using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::Dynamic::Var;
using Poco::ThreadPool;
using Poco::Timestamp;
using Poco::Net::HTMLForm;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::NameValueCollection;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;
using Poco::JSON::Object;
using Poco::JSON::Parser;

#include "../../database/user.h"

void MakeResponseBody(HTTPServerResponse &response, const std::string& body) {
    std::ostream &ostr = response.send();
    ostr << body;
}

void MakeResponse400(HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
	response.setContentLength(0);
	response.send();
}

void MakeResponse404(HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
	response.setContentLength(0);
	response.send();
}

void MakeResponse409(HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPResponse::HTTP_CONFLICT);
	response.setContentLength(0);
	response.send();
}

void MakeResponse413(HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPResponse::HTTP_REQUEST_ENTITY_TOO_LARGE);
	response.setContentLength(0);
	response.send();
}

void MakeResponse500(HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
	response.setContentLength(0);
	response.send();
}

class UserHandler : public HTTPRequestHandler
{
private:

    void GetByLogin(HTMLForm &form, HTTPServerResponse &response) const {
        try
        {
            std::optional<database::User> result = database::User::read_by_login(form.get("login").c_str());
            if (result) {
                std::ostream &ostr = response.send();
                Poco::JSON::Stringifier::stringify(result->toJSON(), ostr);
            } else {
                MakeResponse404(response);
            }
        }
        catch (std::exception& s)
        {
            std::cout << s.what() << std::endl;
            MakeResponse500(response);
        }
    }

    void CreateUser(HTTPServerRequest &request, HTTPServerResponse &response) const {
        std::istream& client_body = request.stream();
		int length_msg = request.getContentLength();
		if (length_msg > 1024) {
		    MakeResponse413(response);
		    return;
		}
		char buffer[1025];
		client_body.read(buffer, length_msg);
		buffer[length_msg] = '\n';
		std::cout << buffer << std::endl;
        Parser parser;
        Var result;
        try {
            result = parser.parse(buffer);
        } catch (std::exception& exc) {
            MakeResponse400(response);
            return;
        }

        Object::Ptr object = result.extract<Object::Ptr>();
        database::User user;
        try {
            user.login = object->get("login").toString();
            user.first_name = object->get("first_name").toString();
            user.last_name = object->get("last_name").toString();
            user.age = std::atoi(object->get("age").toString().c_str());
        } catch (...) {
            MakeResponse400(response);
            return;
        }

        try {
            user.send_to_queue();
        } catch (...) {
            MakeResponse500(response);
            return;
        }
        MakeResponseBody(response, "{\"queued\":true}");
    }

public:
    UserHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest(HTTPServerRequest &request,
                       HTTPServerResponse &response)
    {
        HTMLForm form(request);

        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");

        if (request.getMethod() == "GET") {
            if (form.has("login")) {
                GetByLogin(form, response);
                return;
            }
        }

        if (
            request.getMethod() == "POST"
        ) {
            CreateUser(request, response);
            return;
        }

        MakeResponse400(response);
    }

private:
    std::string _format;
};
#endif // !USERHANDLER_H
