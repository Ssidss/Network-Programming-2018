#include <cstdlib>
#include <boost/asio.hpp>
#include <iostream>
#include <cstdio>
#include <memory>
#include <functional>
#include <fstream>

using namespace std;
using boost::asio::ip::tcp;
boost::asio::io_context main_Client;
vector<string> np_label={"s1","s2","s3","s4","s5"};


struct  HostInfo
{
    string Hname;
    unsigned short Hport;
    string strport;
    string Hinput;
    bool ifHostexist=false;
    string Hlabel;
} HostTable[5];

class Client : public enable_shared_from_this<Client> {
    public:
        Client(boost::asio::io_context& main_Client,
               const string& np_host, const string& np_port,const string& np_inputfile,const string& np_label);  
        void start (){
            DoResolver();
        };             
    private:
        tcp::socket socket_;
        tcp::resolver resolver_;
        string npCommand;
        enum{ BUF_SIZE = 15000};
        //char cin_buf_[BUF_SIZE];
        struct HostInfo np_hostinf;       
        std::array<char,BUF_SIZE> buf_;
        //string np_buffer;
        string ReplaceEscape(string mes) {
            string res="";
            for (int i=0;i<mes.length();i++) 
            {
                if (mes[i]=='\n') {
                    res+="<br>";
                } else if (mes[i]=='<') {
                    res+="&lt";
                } else if (mes[i]=='>') {
                    res+="&gt";
                } else if (mes[i]=='\r'&&mes[i+1]=='\n'){
                    res+="<br>";
                    i++;
                } else if (mes[i]=='"') {
                    res+="&quot;";                    
                } else {
                    res+=mes[i];
                }                
            }
            return res;
        };
        void DoResolver(){
            auto np_this(shared_from_this());
            resolver_.async_resolve(tcp::v4(),np_hostinf.Hname,np_hostinf.strport,
            bind(&Client::OnResolve,np_this, 
            std::placeholders::_1,
            std::placeholders::_2));
        };
        void OnResolve(boost::system::error_code ec,              /////DNS
                       tcp::resolver::results_type endpoints) {
                        if (ec) {
                            cerr << "Resolve: " << ec.message() << endl;
                        } else {
                            auto np_this(shared_from_this());
                            boost::asio::async_connect(socket_,endpoints,
                                                    bind(&Client::OnConnect,np_this,
                                                    placeholders::_1,
                                                    placeholders::_2));
                        }
        };
        void OnConnect(boost::system::error_code ec,tcp::endpoint endpoint){
            if (ec) {
                cout << "Connect faild: " << ec.message() << endl;
                socket_.close();
            } else {
                //cout << "connect success" << endl;
                DoRead();
                //DoSend("ls\n");
                //socket_.close();
            }

        };
        string Detcom(){
            
            string com;
                      
                         //take one line command to send
                        //find \r\n or \r||\n to store one line
            int posi=npCommand.find("\n",0);
            com.assign(npCommand,0,posi);
            npCommand.assign(npCommand,posi+1,npCommand.length()-posi);
            while (1)
            {
                if (npCommand.find('\n',0)==0) {
                    npCommand.assign(npCommand,1,npCommand.length()-1);
                } else if (npCommand.find('\r',0)==0) {
                    npCommand.assign(npCommand,1,npCommand.length()-1);
                } else {
                    break;
                }
            }
            while (1)
            {
                posi=com.find('\r',0);
                if (posi<com.length()&&posi>=0){
                    com.erase(posi);
                } else {break;}
            }
            while (1)
            {
                posi=com.find('\n',0);
                if (posi<com.length()&&posi>=0){
                    com.erase(posi);
                } else {break;}
            }            
            
            //system("puase");
            return com;                        
        };
        void Print_BUF(string data) {
            cout <<"<script>document.getElementById(\"s"<<np_hostinf.Hlabel<<"\").innerHTML += \""<<data<<"\";</script>\r\n";            
        };
        void DoSend(string np_mesg){
            auto self(shared_from_this());
            socket_.async_send(
            boost::asio::buffer(np_mesg,np_mesg.length()),
                [this, self](boost::system::error_code ec, std::size_t /* length */) {                    
                });
        };
        void DoRead(){
            auto self(shared_from_this());
            socket_.async_read_some(
            boost::asio::buffer(buf_, BUF_SIZE),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {   ////Analysis buffer   
  
                    string tempbuf;
                    tempbuf.assign(buf_.data(),0,length); 
                    tempbuf=ReplaceEscape(tempbuf);                   
                    Print_BUF(tempbuf);
                    if (tempbuf.find('%',0)>=0&&tempbuf.find('%',0)<tempbuf.length()){                          
                        string OneLineCommand;   //for find % -> need to send one line command
                        OneLineCommand.clear();  //and cout tempbuf to echo_server          
                        OneLineCommand=Detcom();                        
                        string SendCommand;
                        usleep(500);
                        SendCommand=OneLineCommand+"\\n";
                        Print_BUF(SendCommand);
                        SendCommand=OneLineCommand+"\n";
                        DoSend(SendCommand);
                    }   
                    usleep(500);                                                      
                    DoRead();
                } else {
                    socket_.close();
                }
            });
        };        
};

Client::Client(boost::asio::io_context& main_Client,
               const string& np_host,const string& np_port,const string& np_inputfile,const string& np_label) 
               :socket_(main_Client),
                resolver_(main_Client) {       
                        string filepath="test_case/";                        
                        filepath=filepath+np_inputfile;
                        ifstream fin(filepath.c_str());

                        npCommand.assign(istreambuf_iterator<char>(fin),istreambuf_iterator<char>()); 
                        np_hostinf.Hname=np_host;
                        np_hostinf.strport=np_port;
                        np_hostinf.Hlabel=np_label;                                                                             
}




void PrintHeader();
void detstr (string,char,vector<string> &);
void ParseQuery(string);

int main (void) 
{
    string _query;
    _query=getenv("QUERY_STRING");
    //_query="h0=nplinux4.cs.nctu.edu.tw&p0=7002&f0=t1.txt&h1=nplinux2.cs.nctu.edu.tw&p1=7124&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=";
    ParseQuery(_query);
    PrintHeader();
    for (int i=0;i<5;i++) {
        if (HostTable[i].ifHostexist) {
            make_shared<Client> (main_Client,HostTable[i].Hname,HostTable[i].strport,HostTable[i].Hinput,HostTable[i].Hlabel)->start();

        }

    }
    main_Client.run();    

    return 0;
}

//cout <<"<script>document.getElementById(\"s"<<i<<"\").innerHTML += \""<<mes<<"\";</script>\r\n";
void ParseQuery(string _query)
{
    vector <string> Dquery;
    string temp;
    detstr(_query,'&',Dquery);
    int posi=0;
    for(int i=0;i<5;i++)
    {
        for (int j=0;j<3;j++)
        {            
            if (Dquery[posi].length()<=3) {
                posi++;
                //HostTable[i].ifHostexist=false;
                continue;
            }
            temp.assign(Dquery[posi],3,Dquery[posi].length()-3);
            if (j==0) {
                HostTable[i].Hname=temp;
                HostTable[i].ifHostexist=true;
                HostTable[i].Hlabel=np_label[i];
            } else if (j==1) {
                HostTable[i].Hport=atoi(temp.c_str());
                HostTable[i].strport=temp;
            } else if (j==2) {
                HostTable[i].Hinput=temp;
            }
            posi++;
        }
        
    }
};
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
void PrintHeader()
{
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<!DOCTYPE html>\r\n";
    cout << "<html lang=\"en\">\r\n";
    cout << "  <head>\r\n";
    cout << "    <meta charset=\"UTF-8\" />\r\n";
    cout << "    <title>NP Project 3 Console</title>\r\n";
    cout << "    <link\r\n";
    cout << "      rel=\"stylesheet\"\r\n";
    cout << "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\r\n";
    cout << "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\r\n";
    cout << "      crossorigin=\"anonymous\"\r\n";
    cout << "    />\r\n";
    cout << "    <link\r\n";
    cout << "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\r\n";
    cout << "      rel=\"stylesheet\"\r\n";
    cout << "    />\r\n";
    cout << "    <link\r\n";
    cout << "      rel=\"icon\"\r\n";
    cout << "      type=\"image/png\"\r\n";
    cout << "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\r\n";
    cout << "    />\r\n";
    cout << "    <style>\r\n";
    cout << "      * {\r\n";
    cout << "        font-family: 'Source Code Pro', monospace;\r\n";
    cout << "        font-size: 1rem !important;\r\n";
    cout << "      }\r\n";
    cout << "      body {\r\n";
    cout << "        background-color: #212529;\r\n";
    cout << "      }\r\n";
    cout << "      pre {\r\n";
    cout << "        color: #cccccc;\r\n";
    cout << "      }\r\n";
    cout << "      b {\r\n";
    cout << "        color: #ffffff;\r\n";
    cout << "      }\r\n";
    cout << "    </style>\r\n";
    cout << "  </head>\r\n";
    cout << "  <body>\r\n";
    cout << "    <table class=\"table table-dark table-bordered\">\r\n";
    cout << "      <thead>\r\n";
    cout << "        <tr>\r\n";
    for (int i=0;i<5;i++){
        if (HostTable[i].ifHostexist) {
            cout << "          <th scope=\"col\">"<<HostTable[i].Hname<<".cs.nctu.edu.tw:"<<HostTable[i].Hport<<"</th>\r\n";
        } 
    }
    cout << "        </tr>\r\n";
    cout << "      </thead>\r\n";
    cout << "      <tbody>\r\n";
    cout << "        <tr>\r\n";
    for (int i=0;i<5;i++){
        if (HostTable[i].ifHostexist) {
            cout << "          <td><pre id=\"s"<<HostTable[i].Hlabel<<"\" class=\"mb-0\"></pre></td>\r\n";
        }
    }
    cout << "        </tr>\r\n";
    cout << "      </tbody>\r\n";
    cout << "    </table>\r\n";
    cout << "  </body>\r\n";
    cout << "</html>\r\n";
};
