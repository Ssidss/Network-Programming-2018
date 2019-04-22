#include <cstdlib>
#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <cstdio>
#include <memory>
#include <functional>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;
using namespace boost::asio;
io_service main_Client;
io_service global_io_service;

string HTTPOK = " 200 OK\r\n\r\n";
string HTTPNotFound = " 404 Not Found\r\n\r\n";
int readtime=0;



struct  HostInfo
{
	string Hname;
	unsigned short Hport;
	string strport;
	string Hinput;
	bool ifHostexist = false;
	int Hlabel;
} HostTable[5];

void detstr(string, char, vector<string> &);
void ParseQuery(string);


class Client : public enable_shared_from_this<Client> {
public:
	Client(const shared_ptr<ip::tcp::socket>& http_socket_ptr, io_service& main_Client,
		const string& np_host, const string& np_port, const string& np_inputfile,const int& np_label);
	void start() {
		DoResolver();
	};
private:
	
	ip::tcp::socket socket_;
	ip::tcp::resolver resolver_;
	shared_ptr<ip::tcp::socket> _http_socket_ptr;

	string npCommand;
	enum { BUF_SIZE = 15000 };
	//char cin_buf_[BUF_SIZE];
	struct HostInfo np_hostinf;
	std::array<char, BUF_SIZE> buf_;
	char mess[BUF_SIZE];
	//string np_buffer;

	string ReplaceEscape(string mes) {
		string res = "";
		for (int i = 0; i<mes.length(); i++)
		{
			if (mes[i] == '\n') {
				res += "<br>";
			}
			else if (mes[i] == '<') {
				res += "&lt";
			}
			else if (mes[i] == '>') {
				res += "&gt";
			}
			else if (mes[i] == '\r'&&mes[i + 1] == '\n') {
				res += "<br>";
				i++;
			}
			else if (mes[i] == '"') {
				res += "&quot;";
			}
			else {
				res += mes[i];
			}
		}
		return res;
	};
	void DoResolver() {
		//cout << "Do resolve" << endl;
		auto np_this(shared_from_this());
		resolver_.async_resolve(ip::tcp::v4(), np_hostinf.Hname, np_hostinf.strport,  //////why
			bind(&Client::OnResolve, np_this,
				std::placeholders::_1,
				std::placeholders::_2));
	};
	void OnResolve(boost::system::error_code ec,              /////DNS
		ip::tcp::resolver::results_type endpoints) {
		if (ec) {
			cerr << "Resolve: " << ec.message() << endl;
		}
		else {
			//cout << "On Resolve " << endl;
			auto np_this(shared_from_this());
			boost::asio::async_connect(socket_, endpoints,
				bind(&Client::OnConnect, np_this,
					std::placeholders::_1,
					std::placeholders::_2));
		}
	};
	void OnConnect(boost::system::error_code ec, ip::tcp::endpoint endpoint) {
		if (ec) {
			cerr << "Connect faild: " << ec.message() << endl;
			socket_.close();
		}
		else {
			//cout << "connect success" << endl;
			cout << "Connect Success! \n";
			buf_.fill(0);
			DoRead();
			//DoSend("ls\n");
			//socket_.close();
		}

	};
	string Detcom() {

		string com;

		//take one line command to send
		//find \r\n or \r||\n to store one line
		int posi = npCommand.find("\n", 0);
		com.assign(npCommand, 0, posi);
		npCommand.assign(npCommand, posi + 1, npCommand.length() - posi);
		while (1)
		{
			if (npCommand.find('\n', 0) == 0) {
				npCommand.assign(npCommand, 1, npCommand.length() - 1);
			}
			else if (npCommand.find('\r', 0) == 0) {
				npCommand.assign(npCommand, 1, npCommand.length() - 1);
			}
			else {
				break;
			}
		}
		while (1)
		{
			posi = com.find('\r', 0);
			if (posi<com.length() && posi >= 0) {
				com.erase(posi);
			}
			else { break; }
		}
		while (1)
		{
			posi = com.find('\n', 0);
			if (posi<com.length() && posi >= 0) {
				com.erase(posi);
			}
			else { break; }
		}

		//system("puase");
		return com;
	};
	void Print_BUF(string data) {
		string send_mes;
		stringstream ss;
		ss << np_hostinf.Hlabel;
		send_mes = send_mes + "<script>document.getElementById(\"s" + ss.str() + "\").innerHTML += \"" + data + "\";</script>\r\n";
		strcpy_s(mess, send_mes.c_str());
		DoSendHTTP(send_mes.length());
	};
	void DoSend(size_t length) {
		auto self(shared_from_this());
		socket_.async_send(
			buffer(mess, length),
			[this, self](boost::system::error_code ec, std::size_t /* length */) {
			if (ec) {
				socket_.close();
			}
		});
	};
	void DoSendHTTP(size_t length) {
		auto self(shared_from_this());
		_http_socket_ptr->async_send(
			buffer(mess,length),
			[this, self](boost::system::error_code ec, std::size_t /* length */) {
			if (ec) {
				socket_.close();
			}
		});
	};
	void DoRead() {
		auto self(shared_from_this());
		socket_.async_read_some(
			buffer(buf_, BUF_SIZE),
			[this, self](boost::system::error_code ec, std::size_t length) {
			if (!ec) {   ////Analysis buffer   
				//cout << "Client Do Read" << endl;
				if (length == 0) { cout << "client exit"; socket_.close(); }

				string tempbuf="";
				cout << "length=" << length << endl;
				tempbuf.assign(buf_.data(), 0, length);
				//cout << endl << endl <<endl <<tempbuf << endl << endl << endl;
				tempbuf = ReplaceEscape(tempbuf);
				Print_BUF(tempbuf);
				if (tempbuf.find('%', 0) >= 0 ) {
					string OneLineCommand="";   //for find % -> need to send one line command
					//OneLineCommand.clear();  //and cout tempbuf to echo_server          
					OneLineCommand = Detcom();
					string SendCommand;
					//usleep(500);
					//DoSend("Hi iam ken\r\n");
					//cout << "sendcommamd" << endl;
					//cout << OneLineCommand<< endl;
					//cout << "command length=" << OneLineCommand.length() << endl;
					SendCommand = OneLineCommand + "\\n";
					SendCommand = ReplaceEscape(SendCommand);
					Print_BUF(SendCommand);
					SendCommand = OneLineCommand + "\n";
					strcpy_s(mess, SendCommand.c_str());
					DoSend(SendCommand.length());
				}
				//usleep(500);
				//cout << "Doread" << endl;
				buf_.fill(0);
				DoRead();
			}
			else {
				cout << "close" << endl;
				socket_.close();
			}
		});
	};
};

Client::Client(const shared_ptr<ip::tcp::socket>&http_socket_ptr, io_service& main_Client,
	const string& np_host, const string& np_port, const string& np_inputfile,const int& np_label)
	:socket_(main_Client),
	resolver_(main_Client),
	_http_socket_ptr(http_socket_ptr) {
	//cout << "fstream" << endl;
	string filepath = "test_case\\" ;
	filepath = filepath + np_inputfile;
	ifstream fin(filepath.c_str());
	
	npCommand.assign(istreambuf_iterator<char>(fin), istreambuf_iterator<char>());
	//cout <<  "my npCommand= " << endl;
	if (npCommand.length() == 0) { cout << "command=empty()" << endl;; npCommand = "exit\n"; };
	//cout << npCommand << endl;
	np_hostinf.Hname = np_host;
	np_hostinf.strport = np_port;
	np_hostinf.Hlabel = np_label;
	//cout << "np_host" << np_host << " np_port=" << np_port << " np_label=" << np_label << endl;
	//cout << "init Done!" << endl;
}


class EchoSession : public enable_shared_from_this<EchoSession> {
private:
	enum { max_length = 15000 };
	ip::tcp::socket _socket;
	array<char, max_length> _data;
	char mess[max_length];
	struct Envalue {
		string METHOD_;
		string URI_;
		string Request_Version;
		string Query_String;
		string HOST_NAME;

		string QUERY_STRING;
		string REQUEST_METHOD;
		string REQUEST_URI;
		string SERVER_PROTOCOL;
		string HTTP_HOST;
		string SERVER_ADDR;
		string SERVER_PORT;
		string REMOTE_ADDR;
		string REMOTE_PORT;
	}E;


public:
	EchoSession(ip::tcp::socket socket) : _socket(move(socket)) {}

	void start() { //do_read(); 
				   //cout << "connect success" << endl;
		//dup2(_socket.native_handle(), 1);
		_data.fill(0);
		do_read();
		
	}

private:
	
	string PrintPanel()
	{
		string panel_string = "Content-Type:text/html\r\n\r\n<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>NP Project 3 Panel</title>\r\n<link\r\nrel=\"stylesheet\"\r\nhref=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\r\nintegrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\r\ncrossorigin=\"anonymous\"\r\n/>\r\n<link\r\nhref=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\r\nrel=\"stylesheet\"\r\n/>\r\n<link\r\nrel=\"icon\"\r\ntype=\"image/png\"\r\nhref=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\r\n/>\r\n<style>\r\n* {\r\nfont-family: 'Source Code Pro', monospace;\r\n}\r\n</style>\r\n</head>\r\n<body class=\"bg-secondary pt-5\">\r\n<form action=\"console.cgi\" method=\"GET\">\r\n<table class=\"table mx-auto bg-light\" style=\"width: inherit\">\r\n<thead class=\"thead-dark\">\r\n<tr>\r\n<th scope=\"col\">#</th>\r\n<th scope=\"col\">Host</th>\r\n<th scope=\"col\">Port</th>\r\n<th scope=\"col\">Input File</th>\r\n</tr>\r\n</thead>\r\n<tbody>\r\n<tr>\r\n<th scope=\"row\" class=\"align-middle\">Session 1</th>\r\n<td>\r\n<div class=\"input-group\">\r\n<select name=\"h0\" class=\"custom-select\">\r\n<option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\r\n</select>\r\n<div class=\"input-group-append\">\r\n<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\r\n</div>\r\n</div>\r\n</td>\r\n<td>\r\n<input name=\"p0\" type=\"text\" class=\"form-control\" size=\"5\" />\r\n</td>\r\n<td>\r\n<select name=\"f0\" class=\"custom-select\">\r\n<option></option>\r\n<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option value=\"t5.txt\">t5.txt</option>\r\n<option value=\"t6.txt\">t6.txt</option>\r\n<option value=\"t7.txt\">t7.txt</option>\r\n<option value=\"t8.txt\">t8.txt</option>\r\n<option value=\"t9.txt\">t9.txt</option>\r\n<option value=\"t10.txt\">t10.txt</option>\r\n</select>\r\n</td>\r\n</tr>\r\n<tr>\r\n<th scope=\"row\" class=\"align-middle\">Session 2</th>\r\n<td>\r\n<div class=\"input-group\">\r\n<select name=\"h1\" class=\"custom-select\">\r\n<option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\r\n</select>\r\n<div class=\"input-group-append\">\r\n<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\r\n</div>\r\n</div>\r\n</td>\r\n<td>\r\n<input name=\"p1\" type=\"text\" class=\"form-control\" size=\"5\" />\r\n</td>\r\n<td>\r\n<select name=\"f1\" class=\"custom-select\">\r\n<option></option>\r\n<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option value=\"t5.txt\">t5.txt</option>\r\n</select>\r\n</td>\r\n</tr>\r\n<tr>\r\n<th scope=\"row\" class=\"align-middle\">Session 3</th>\r\n<td>\r\n<div class=\"input-group\">\r\n<select name=\"h2\" class=\"custom-select\">\r\n<option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\r\n</select>\r\n<div class=\"input-group-append\">\r\n<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\r\n</div>\r\n</div>\r\n</td>\r\n<td>\r\n<input name=\"p2\" type=\"text\" class=\"form-control\" size=\"5\" />\r\n</td>\r\n<td>\r\n<select name=\"f2\" class=\"custom-select\">\r\n<option></option>\r\n<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option value=\"t5.txt\">t5.txt</option>\r\n</select>\r\n</td>\r\n</tr>\r\n<tr>\r\n<th scope=\"row\" class=\"align-middle\">Session 4</th>\r\n<td>\r\n<div class=\"input-group\">\r\n<select name=\"h3\" class=\"custom-select\">\r\n<option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\r\n</select>\r\n<div class=\"input-group-append\">\r\n<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\r\n</div>\r\n</div>\r\n</td>\r\n<td>\r\n<input name=\"p3\" type=\"text\" class=\"form-control\" size=\"5\" />\r\n</td>\r\n<td>\r\n<select name=\"f3\" class=\"custom-select\">\r\n<option></option>\r\n<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option value=\"t5.txt\">t5.txt</option>\r\n</select>\r\n</td>\r\n</tr>\r\n<tr>\r\n<th scope=\"row\" class=\"align-middle\">Session 5</th>\r\n<td>\r\n<div class=\"input-group\">\r\n<select name=\"h4\" class=\"custom-select\">\r\n<option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\r\n</select>\r\n<div class=\"input-group-append\">\r\n<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\r\n</div>\r\n</div>\r\n</td>\r\n<td>\r\n<input name=\"p4\" type=\"text\" class=\"form-control\" size=\"5\" />\r\n</td>\r\n<td>\r\n<select name=\"f4\" class=\"custom-select\">\r\n<option></option>\r\n<option value=\"t1.txt\">t1.txt</option><option value=\"t2.txt\">t2.txt</option><option value=\"t3.txt\">t3.txt</option><option value=\"t4.txt\">t4.txt</option><option value=\"t5.txt\">t5.txt</option>\r\n</select>\r\n</td>\r\n</tr>\r\n<tr>\r\n<td colspan=\"3\"></td>\r\n<td>\r\n<button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\r\n</td>\r\n</tr>\r\n</tbody>\r\n</table>\r\n</form>\r\n</body>\r\n</html>\r\n";
		return panel_string;
	};

	string PrintConsole_Header()
	{
		string send_message = "Content-Type:text/html\r\n\r\n<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<meta charset=\"UTF-8\" />\r\n<title>NP Project 3 Console</title>\r\n<link\r\nrel=\"stylesheet\"\r\nhref=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\r\nintegrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\r\ncrossorigin=\"anonymous\"\r\n/>\r\n<link\r\nhref=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\r\nrel=\"stylesheet\"\r\n/>\r\n<link\r\nrel=\"icon\"\r\ntype=\"image/png\"\r\nhref=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\r\n/>\r\n<style>\r\n* {\r\nfont-family: 'Source Code Pro', monospace;\r\nfont-size: 1rem !important;\r\n}\r\nbody {\r\nbackground-color: #212529;\r\n}\r\npre {\r\ncolor: #cccccc;\r\n}\r\nb {\r\ncolor: #ffffff;\r\n}\r\n</style>\r\n</head>\r\n<body>\r\n<table class=\"table table-dark table-bordered\">\r\n<thead>\r\n<tr>\r\n";
		for (int i = 0; i < 5; i++) 
		{
			if (HostTable[i].ifHostexist) {
				send_message = send_message + "<th scope=\"col\">" + HostTable[i].Hname + ".cs.nctu.edu.tw:" + HostTable[i].strport + "</th>\r\n";
			}			
		}
		send_message = send_message + "</tr>\r\n</thead>\r\n<tbody>\r\n<tr>\r\n";
		for (int i = 0; i < 5; i++) 
		{
			if (HostTable[i].ifHostexist) {
				stringstream ss;
				ss << HostTable[i].Hlabel;
				//cout << ss.str() << endl;
				//cout << "HI I " << i << "??"<< endl;
				send_message = send_message + "<td><pre id=\"s" + ss.str() + "\" class=\"mb-0\"></pre></td>\r\n";
				//cout << "Hostable" << i << " label=" << HostTable[i].Hlabel << endl;
			}							
		}
	
		send_message = send_message + "</tr>\r\n</tbody>\r\n</table>\r\n</body>\r\n</html>\r\n";
		return send_message;

	};
	void np_setenv() {
		int posi = E.URI_.find('?', 0);
		if (posi >= 0 && posi<E.URI_.length()) {
			E.Query_String.assign(E.URI_, posi + 1, E.URI_.length() - posi);
			E.URI_.assign(E.URI_, 0, posi);
			E.QUERY_STRING = E.Query_String;
		}
		E.REQUEST_METHOD = E.METHOD_;
		E.SERVER_PROTOCOL = E.Request_Version;
		E.URI_.assign(E.URI_, 1, E.URI_.length() - 1);  ////remove /
		E.REQUEST_URI= E.URI_;
		//cout << "EREQUET"<<E.REQUEST_URI << endl;
		E.HTTP_HOST = E.HOST_NAME;
		E.SERVER_ADDR = _socket.local_endpoint().address().to_string();
		E.REMOTE_ADDR = _socket.remote_endpoint().address().to_string();
	};
	void do_read() {
		auto self(shared_from_this());
		_socket.async_read_some(
			buffer(_data, max_length),
			[this, self](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				string tempbuf;
				tempbuf.clear();
				tempbuf.assign(_data.data(), 0, length);
				vector<string> DetString;
				DetString.clear();
				//cout << tempbuf << endl;
				detstr(tempbuf, ' ', DetString);
				//Envalue HTTP_Env_Value;
				E.METHOD_ = DetString[0];
				E.URI_ = DetString[1];
				DetString[2].assign(DetString[2], 0, 8);
				E.Request_Version = DetString[2];
				DetString[4].assign(DetString[4], 0, DetString[4].length() - 2);
				E.HOST_NAME = DetString[4];
				np_setenv();

				if (E.REQUEST_URI=="panel.cgi") {
					cout << "panel" << endl;
					string Ack;
					Ack = E.SERVER_PROTOCOL + HTTPOK;
					strcpy_s(mess, Ack.c_str());
					do_write(Ack.length());
					string tempstr;
					tempstr = PrintPanel();
					strcpy_s(mess, tempstr.c_str());
					do_write(tempstr.length());
				}
				else if (E.REQUEST_URI=="console.cgi") {
					cout << "console" << endl;
					string Ack;
					Ack = E.SERVER_PROTOCOL + HTTPOK;
					strcpy_s(mess, Ack.c_str());
					do_write(Ack.length());
					ParseQuery(E.QUERY_STRING);		
					//cout << E.QUERY_STRING << endl;
					string tempstr= PrintConsole_Header();
					strcpy_s(mess,tempstr.c_str());
					do_write(tempstr.length());
					//system("pause");
					//main_Client.stop();
					auto socket_ptr = make_shared<ip::tcp::socket>(move(_socket));
					for (int i = 0; i < 5; i++)
					{						
						if (HostTable[i].ifHostexist) {
							//cout << " Client start" << endl;
							//cout << "Hname" << i << "=" << HostTable[i].Hname << " Hprot=" << HostTable[i].strport << " testfile=" << HostTable[i].Hinput <<" label=" <<HostTable[i].Hlabel << endl;
							make_shared<Client>(socket_ptr, main_Client, HostTable[i].Hname, HostTable[i].strport, HostTable[i].Hinput, HostTable[i].Hlabel)->start();
						}
					}
					main_Client.run();
					//cout <<"run "<< endl;
				}
				else {
					string Ack;
					Ack = E.SERVER_PROTOCOL + HTTPNotFound;
					strcpy_s(mess, Ack.c_str());
					do_write(Ack.length());
				}
				cout << "Do Read " << endl;
				_data.fill(0);
				do_read();
				//start();
			}
			else {
				_socket.close();
			}
		});
	}
	
	void do_write(size_t length) {
		auto self(shared_from_this());
		_socket.async_send(
			boost::asio::buffer(mess,length),
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




int main(int argc, char* const argv[])
{
	if (argc != 2) {
        std::cerr << "Usage:" << argv[0] << " [port]" << endl;
        return 1;
    }
    try {
		unsigned short port = atoi(argv[1]);
		//short port = 2018;
		EchoServer server(port);
		global_io_service.run();
  	} catch (exception& e) {
		cerr << "Exception: " << e.what() << "\n";
  	}
	//short port = 2018;
	//EchoServer server(port);
	//global_io_service.run();

	return 0;
}

//cout <<"<script>document.getElementById(\"s"<<i<<"\").innerHTML += \""<<mes<<"\";</script>\r\n";

void detstr(string inp, char det, vector<string> &fin)
{
	int s = 0, e = 0;
	string temp;
	while (1) {
		e = inp.find(det, s);
		if (e<0 || e >= inp.length()) {           // for last space
			temp.assign(inp, s, inp.length() - s);
			fin.push_back(temp);
			temp.clear();
			break;
		}
		else if (e == s) {                       //consis space
			s = s + 1;
		}
		else {                                 //cut
			temp.assign(inp, s, e - s);
			fin.push_back(temp);
			temp.clear();
			s = e + 1;
		}
		if (s>inp.length() - 1) { break; }
	}
};


void ParseQuery(string _query)
{
	vector <string> Dquery;
	string temp;
	detstr(_query, '&', Dquery);
	int posi = 0;
	
	for (int i = 0; i<5; i++)
	{
		HostTable[i].ifHostexist = false;
		HostTable[i].strport = "";
		HostTable[i].Hport = 0;
		HostTable[i].Hinput = "";
		HostTable[i].Hname="";
		for (int j = 0; j<3; j++)
		{
			if (Dquery[posi].length() <= 3) {
				posi++;
				//HostTable[i].ifHostexist=false;
				continue;
			}
			temp.assign(Dquery[posi], 3, Dquery[posi].length() - 3);
			//cout << "temp=" << temp << endl;
			if (j == 0) {
				HostTable[i].Hname = temp;
				HostTable[i].ifHostexist = true;
				HostTable[i].Hlabel = i;
			}
			else if (j == 1) {
				HostTable[i].Hport = atoi(temp.c_str());
				HostTable[i].strport = temp;
			}
			else if (j == 2) {
				HostTable[i].Hinput = temp;
			}
			posi++;
		}

	}
};
