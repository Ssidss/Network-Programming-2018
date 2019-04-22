
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdio>
using namespace std;

int main(void)
{
//    cout << getenv("PATH")<< endl;;
  cout << "HTTP/1.1 200 OK\r\n\r\n";
  cout << "Content-type: text/html\r\n\r\n";
  cout << "<html>\r\n";
  cout << "<head>\r\n";
  cout << "  <title>HELLO</title>\r\n";
  cout << "  </head>\r\n";
  cout << "  <body>\r\n";
  cout << "     HELLO World.\r\n";
  cout << "  </body>\r\n";
  cout << "</html>\r\n";
  return 0;
}
