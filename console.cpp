#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;

struct HostArg
{
    string host;
    string port;
    string file;
    string id;
};
vector<HostArg> host_vec;

void HTML_Responce()
{
    cout << "Content-type: text/html\r\n\r\n";
    boost::format table("<!DOCTYPE html>"
                        "<html lang=\"en\">"
                        "  <head>"
                        "    <meta charset=\"UTF-8\" />"
                        "    <title>NP Project 3</title>"
                        "    <link"
                        "      rel=\"stylesheet\""
                        "      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\""
                        "      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\""
                        "      crossorigin=\"anonymous\""
                        "    />"
                        "    <link"
                        "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\""
                        "      rel=\"stylesheet\""
                        "    />"
                        "    <link"
                        "      rel=\"icon\""
                        "      type=\"image/png\""
                        "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\""
                        "    />"
                        "    <style>"
                        "      * {"
                        "        font-family: 'Source Code Pro', monospace;"
                        "        font-size: 1rem !important;"
                        "      }"
                        "      body {"
                        "        background-color: #212529;"
                        "      }"
                        "      pre {"
                        "        color: #cccccc;"
                        "      }"
                        "      b {"
                        "        color: #01b468;"
                        "      }"
                        "    </style>"
                        "  </head>"
                        "  <body>"
                        "    <table class=\"table table-dark table-bordered\">"
                        "      <thead>"
                        "        <tr>"
                        "          %1%"
                        "        </tr>"
                        "      </thead>"
                        "      <tbody>"
                        "        <tr>"
                        "          %2%"
                        "        </tr>"
                        "      </tbody>"
                        "    </table>"
                        "  </body>"
                        "</html>");

    string title;
    string body;
    for(int i = 0 ; i < host_vec.size() ; i++)
    {
        title += R"(<th scope="col">)" + host_vec[i].host + ":" + host_vec[i].port + R"(</th>)";
        body += R"(<td><pre id=")" + host_vec[i].id + R"(" class="mb-0"></pre></td>)";
    }
    cout << (table % title % body).str();
    cout.flush();
}
void ParseQuery()
{
    //QUERY_STRING example
    //h0=localhost&p0=7788&f0=t1.txt&h1=localhost&p1=7799&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
    vector<string> argV;
    boost::split(argV, getenv("QUERY_STRING"), boost::is_any_of("&"), boost::token_compress_on);
    for(int i=0 ; i<argV.size() ; i++)
    {
        vector<string> tempV;
        boost::split(tempV, argV[i], boost::is_any_of("="), boost::token_compress_on);
        if(tempV[1].length())
        {
            if(i%3==0)
            {
                HostArg tempHost;
                host_vec.push_back(tempHost);
                host_vec[i/3].id = "c" + to_string(i/3);
                cerr<<"--------------------"<<endl;
                cerr<<"host id: "<<host_vec[i/3].id<<endl;
                host_vec[i/3].host = tempV[1];
                cerr<<"h"<<i/3<<" "<<tempV[1]<<endl;
            }
            else if(i%3==1)
            {
                host_vec[i/3].port = tempV[1];
                cerr<<"p"<<i/3<<" "<<tempV[1]<<endl;
            }
            else if(i%3==2)
            {
                host_vec[i/3].file = tempV[1];
                cerr<<"f"<<i/3<<" "<<tempV[1]<<endl;
                cerr<<"--------------------"<<endl;
            }
        }
    }
}
class client : public enable_shared_from_this<client>
{
public:
    client(io_context& io_context, string id, tcp::resolver::query q, string file)
        : resolver_(io_context), socket_(io_context), id_(id), q_(move(q))
        {
            file_.open("test_case/" + file, ios::in);
        }
    void Start()
    {
        DoResolve();
    }
private:
    
    tcp::resolver resolver_;
    tcp::socket socket_;
    string id_;
    tcp::resolver::query q_;
    fstream file_;
    enum { maxLength = 15000 };
    char data_[maxLength];

    void Output(string data, bool isCmd)
    {
        boost::algorithm::replace_all(data, "&", "&amp;");
        boost::algorithm::replace_all(data, "\"", "&quot;");
        boost::algorithm::replace_all(data, "\'", "&apos;");
        boost::algorithm::replace_all(data, "<", "&lt;");
        boost::algorithm::replace_all(data, ">", "&gt;");
        boost::algorithm::replace_all(data, "\r\n", "\n");
        boost::algorithm::replace_all(data, "\n", "<br>");
        
        if(!isCmd)
        {
            boost::format output("<script>document.all('%1%').innerHTML += '%2%';</script>");
            cout<<output%id_%data;
        }
        else
        {
            boost::format output("<script>document.all('%1%').innerHTML += '<font color = \"green\">%2%</font>';</script>");
            cout<<output%id_%data;
        }
        cout.flush();
    }
    void ReceiveHandler(size_t length)
    {
        bool isCmd = false;
        string data(data_, data_ + length);
        // cerr<<"testing1: "<<data<<endl;
        Output(data, isCmd);
        if(data.find("% ") != string::npos)
        {
            isCmd = true;
            string cmd;
            getline(file_, cmd);
            cmd += "\n";
            // cerr<<"testing2: "<<cmd<<endl;
            Output(cmd, isCmd);
            socket_.write_some(buffer(cmd));
            // auto self = shared_from_this();
            // boost::asio::async_write(socket_, boost::asio::buffer(cmd), [self, cmd](beast::error_code, std::size_t)
            // {
            //     if(cmd == "exit\n")
            //     {
            //         self->socket_.close();
            //     }
            //     else
            //     {
            //         self->async_read();
            //     }
            // });
        }
    }
    void ConnectHandler()
    {
        auto self(shared_from_this());
        socket_.async_receive(buffer(data_, maxLength), 
            [this, self](boost::system::error_code ec, size_t length)
            {
                if(!ec) ReceiveHandler(length);
                ConnectHandler();
            });
    }
    void ResolveHandler(tcp::resolver::iterator it)
    {
        auto self(shared_from_this());
        socket_.async_connect(*it, 
            [this, self](boost::system::error_code ec)
            {
                if(!ec) ConnectHandler();
            });
    }
    void DoResolve()
    {
        auto self(shared_from_this());
        resolver_.async_resolve(q_, 
            [this, self](boost::system::error_code ec, tcp::resolver::iterator it)
            {
                if(!ec) ResolveHandler(it);
            });
    }
};

int main()
{
    ParseQuery();
    HTML_Responce();

    try
    {
        io_context io_context;
        for(int i=0; i<host_vec.size(); i++)
        {
            tcp::resolver::query q(host_vec[i].host, host_vec[i].port);
            make_shared<client>(io_context, host_vec[i].id, move(q), host_vec[i].file)->Start();
        }
        io_context.run();
    }
    catch(std::exception& e)
    {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}