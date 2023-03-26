#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace boost::asio::ip;
using namespace boost::asio;

class session : public std::enable_shared_from_this<session>   
{
public:
    session(tcp::socket socket) : socket_(move(socket)){}
    void Start()
    {
        do_read();
    }

private:
    tcp::socket socket_;    //connected socket of client
    enum { maxLength = 1024 };
    char data_[maxLength];
    
    string requestMethod = "";
    string requestURI = "";
    string queryString = "";
    string serverProtocol = "";
    string httpHost = "";
    string serverAddr = "";
    string serverPort = "";
    string remoteAddr = "";
    string remotePort = "";
    string cgi = "";

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, maxLength),
            [this, self](boost::system::error_code ec, size_t length)
            {
                if (!ec)
                {
                    ParseRequest();
                    DoRequest();
                }
            });
    }
    void ParseRequest()
    {
        string data = string(data_);
        //
        // GET /console.cgi?h0=localhost&p0=7788&f0=t1.txt&h1=localhost&p1=7799&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4= HTTP/1.1
        // Host: localhost:12345
        // Connection: keep-alive
        // Cache-Control: max-age=0
        // sec-ch-ua: "Google Chrome";v="105", "Not)A;Brand";v="8", "Chromium";v="105"
        // sec-ch-ua-mobile: ?0
        // sec-ch-ua-platform: "Linux"
        // Upgrade-Insecure-Requests: 1
        // User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36
        // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
        // Sec-Fetch-Site: none
        // Sec-Fetch-Mode: navigate
        // Sec-Fetch-User: ?1
        // Sec-Fetch-Dest: document
        // Accept-Encoding: gzip, deflate, br
        // Accept-Language: zh-TW,zh;q=0.9,en-US;q=0.8,en;q=0.7

        vector<string> data_split;
        boost::split(data_split, data, boost::is_any_of(" \r\n"), boost::token_compress_on);

        requestMethod = data_split[0];      // GET
        requestURI = data_split[1];         // /console.cgi?h0=localhost&p0=7788&f0=t1.txt&h1=localhost&p1=7799&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
        serverProtocol = data_split[2];     // HTTP/1.1
        httpHost = data_split[4];           // localhost:12345

        if(strstr(requestURI.c_str(), "?") != NULL) //if there is parameter(?) in requestURI
        {
            vector<string> request_split;
            boost::split(request_split, requestURI, boost::is_any_of("?"), boost::token_compress_on);
            cgi = request_split[0];         // /console.cgi
            queryString = request_split[1]; // h0=localhost&p0=7788&f0=t1.txt&h1=localhost&p1=7799&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
        }
        else
            cgi = requestURI;

        serverAddr = socket_.local_endpoint().address().to_string();
        serverPort = to_string(socket_.local_endpoint().port());
        remoteAddr = socket_.remote_endpoint().address().to_string();
        remotePort = to_string(socket_.remote_endpoint().port());
        PrintEnv();
    }
    void PrintEnv()
    {
        cout<<"Environment"<<endl;
        cout<<"REQUEST_METHOD: "<<requestMethod<<endl;
        cout<<"REQUEST_URI: "<<requestURI<<endl;
        cout<<"QUERY_STRING: "<<queryString<<endl;
        cout<<"SERVER_PROTOCOL: "<<serverProtocol<<endl;
        cout<<"HTTP_HOST: "<<httpHost<<endl;
        cout<<"SERVER_ADDR: "<<serverAddr<<endl;
        cout<<"SERVER_PORT: "<<serverPort<<endl;
        cout<<"REMOTE_ADDR: "<<remoteAddr<<endl;
        cout<<"REMOTE_PORT: "<<remotePort<<endl;
        cout<<"CGI: "<<cgi<<endl<<endl;
    }
    void DoRequest()
    {
        pid_t pid_ = fork();
        if(pid_ == 0)   //child process
        {
            //set environment variable
            setenv("REQUEST_METHOD", requestMethod.c_str(), 1);
            setenv("REQUEST_URI", requestURI.c_str(), 1);
            setenv("QUERY_STRING", queryString.c_str(), 1);
            setenv("SERVER_PROTOCOL", serverProtocol.c_str(), 1);
            setenv("HTTP_HOST", httpHost.c_str(), 1);
            setenv("SERVER_ADDR", serverAddr.c_str(), 1);
            setenv("SERVER_PORT", serverPort.c_str(), 1);
            setenv("REMOTE_ADDR", remoteAddr.c_str(), 1);
            setenv("REMOTE_PORT", remotePort.c_str(), 1);
            //dup
            dup2(socket_.native_handle(), 0);
            dup2(socket_.native_handle(), 1);
            close(socket_.native_handle());
            //exec
            Exec();
            exit(0);
        }
        else            //parent process
        {
            // int status = 0;
            // waitpid(pid_, &status, 0);
            // socket_.close();
            return;
        }
    }
    void Exec()
    {
        cout << "HTTP/1.1 200 OK\r\n" ;
        cout << "Content-Type: text/html\r\n" ;
        fflush(stdout);

        string path = "." + cgi;
        char** argv = new char*[2];
        argv[0] = new char[cgi.size()+1];
        strcpy(argv[0], cgi.c_str());
        argv[1] = NULL;
        if(execv(path.c_str(), argv)<0)
        {
            cerr<<"Fail to exec cgi, errno: "<<errno<<endl;
            exit(1);
        }
    }
};
class server
{
public:
    server(io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }

private:
    tcp::acceptor acceptor_;

    void do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if(!ec)
                {
                    make_shared<session>(move(socket))->Start();
                }

                do_accept();
            });
    }
};

int main(int argc, char* const argv[])
{
    signal(SIGCHLD, SIG_IGN);
    try
    {
        if(argc != 2)
        {
            cout<<"Usage: ./http_server <port>\n";
            return 1;
        }
        
        io_context io_context;
        server s(io_context, atoi(argv[1]));    //port == atoi(argv[1])
        io_context.run();
    }
    catch(std::exception& e)
    {
        cerr<<"Exception: "<<e.what()<<"\n";
    }

    return 0;
}