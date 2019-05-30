#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <sstream>


using namespace std;
using namespace boost::asio;

string HTTPOK=" 200 OK\r\n";
string HTTPNotFound = " 404 Not Found\r\n";
io_service global_io_service;

struct Envalue {
  string METHOD_;
  string URI_;
  string Request_Version;
  string Query_String;
  string HOST_NAME;
};

void detstr (string,char,vector<string> &);

class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 15000 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { //do_read(); 
    //cout << "connect success" << endl;
    dup2(_socket.native_handle(),1);

    
    do_read();  
  }

 private:
 void np_setenv(struct Envalue &E){
  int posi=E.URI_.find('?',0);
  if (posi>=0 && posi<E.URI_.length()) {
    E.Query_String.assign(E.URI_,posi+1,E.URI_.length()-posi);
    E.URI_.assign(E.URI_,0,posi);
    setenv("QUERY_STRING",E.Query_String.c_str(),1);
    //cout << "QUERY_STRING=" <<E.Query_String << endl;
  }
  setenv("REQUEST_METHOD",E.METHOD_.c_str(),1);
  //cout << "REQUEST_METHOD="<<E.METHOD_ << endl;
  setenv("SERVER_PROTOCOL",E.Request_Version.c_str(),1);  
  //cout << "SERVER_PROTOCOL"<<E.Request_Version<< endl;
  E.URI_.assign(E.URI_,1,E.URI_.length()-1);
  E.URI_="./"+E.URI_;
  setenv("REQUEST_URI",E.URI_.c_str(),1);
  //cout << "REQUEST_URI"<<E.URI_<<endl;
  setenv("HTTP_HOST",E.HOST_NAME.c_str(),1);
  //cout <<"HTTP_HOST"<<E.HOST_NAME<<endl;
  setenv("SERVER_ADDR",_socket.local_endpoint().address().to_string().c_str(),1);
  //cout <<"SERVER_ADDR"<<_socket.local_endpoint().address().to_string()<<endl;
  //string num="";
  stringstream sstr;

  sstr << _socket.local_endpoint().port();
  setenv("SERVER_PORT",sstr.str().c_str(),1);
  //cout <<"SERVER_PORT"<<num.c_str()<<endl;
  setenv("REMOTE_ADDR",_socket.remote_endpoint().address().to_string().c_str(),1);
  stringstream sctr;
  sctr << _socket.remote_endpoint().port();
  setenv("REMOTE_PORT",sctr.str().c_str(),1);
};
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec)  {
            string tempbuf;
            tempbuf.assign(_data.data(),0,length); 
            vector<string> DetString;
            detstr (tempbuf,' ',DetString);
            Envalue HTTP_Env_Value;
            HTTP_Env_Value.METHOD_=DetString[0];            
            HTTP_Env_Value.URI_=DetString[1];
            DetString[2].assign(DetString[2],0,8);
            HTTP_Env_Value.Request_Version=DetString[2];
            DetString[4].assign(DetString[4],0,DetString[4].length()-2);
            HTTP_Env_Value.HOST_NAME=DetString[4];
            np_setenv(HTTP_Env_Value);
            int childpid;
            //prepare boost::asio::
            global_io_service.notify_fork(io_service::fork_prepare);
            childpid=fork();
            if (childpid==0) {   //child
              
              global_io_service.notify_fork(io_service::fork_child);
              //cout << HTTP_Env_Value.METHOD_<<HTTPOK;
              cout <<"HTTP/1.1 200 OK\r\n\r\n";
              if (execvp(getenv("REQUEST_URI"),NULL)<0) { ////+./
              /////child
                cout << HTTP_Env_Value.METHOD_<<HTTPNotFound;                
                //return 404;
              } else {//execvpsuccess
                
                                
              }
              exit(0);
            } else {           ///parents
              global_io_service.notify_fork(io_service::fork_parent);
              wait(NULL);
              usleep(500);
              do_read();
            }
          } else {
            _socket.close();
          }
        });
  }

  void do_write(string mes) {
    auto self(shared_from_this());
    _socket.async_send(
        buffer(mes, mes.length()),
        [this, self](boost::system::error_code ec, std::size_t /* length */) {
          if (!ec) {
            //do_read();            
          }
        });
  }
};

class EchoServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  EchoServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
      if (!ec) make_shared<EchoSession>(move(_socket))->start();

      do_accept();
    });
  }
};



int main(int argc, char* const argv[]) {
  if (argc != 2) {
    std::cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }

  setenv("PATH",".:test_case:/usr/bin/",1);
  try {
    unsigned short port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}



void detstr (string inp,char det,vector<string> &fin) 
{
    int s=0,e=0;
    string temp;
    while (1){
        e=inp.find(det,s);
        if (e<0|| e >=inp.length()) {           // for last space
            temp.assign(inp,s,inp.length()-s);
            fin.push_back(temp);
            temp.clear();   
            break;
        } else if (e==s) {                       //consis space
            s=s+1; 
        } else {                                 //cut
	    temp.assign(inp,s,e-s);
	    fin.push_back(temp);
	    temp.clear();
	    s=e+1;
        }
        if (s>inp.length()-1) { break;}
    } 
};
/*
  cout << "HTTP/1.1 200 OK\r\n";
  cout << "<html>\r\n";
  cout << "<head>\r\n";
  cout << "  <title>HELLO</title>\r\n";
  cout << "  </head>\r\n";
  cout << "  <body>\r\n";
  cout << "     HELLO World.\r\n";
  cout << "  </body>\r\n";
  cout << "</html>\r\n";

*/
