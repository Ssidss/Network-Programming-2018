#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstring>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iterator>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#define MAXBUF 4096
#define MAX_CLIENT 30
#define EXIT_STR "exit"
using namespace std;

class envaild {
    public:
        string pathname;
        string pathvalue;
};
class Clientinf {
    public:
        int fdNum;
        int urid;
        string name;
        string ipaddr;
        string Mes;
        unsigned short portN;
        vector<envaild> PATHenv;
        void set (int fdNum,int urid,string name, string ipaddr,string Mes,unsigned short portN) {
            this->fdNum = fdNum;
            this->urid = urid;
            this->name = name;
            this->ipaddr = ipaddr;
            this->Mes = Mes;
            this->portN = portN;
        };
};


class pipefd {
    public:
        int InPipe;
        int OutPipe;
        bool ifr;
        bool ife;
        int pipN;
        int pipNower;          //////UID
        int pipeto;
        int pipefrom;
        string pipeMes;
        void setpipe(int InPipe,int OutPipe,bool ifr,bool ife) {
            this->InPipe=InPipe;
            this->OutPipe=OutPipe;
            this->ifr=ifr;
            this->ife=ife;
        };
};


fd_set afds;
int sockfd;
int nfds;
vector <Clientinf> clientable;
vector <pipefd> pipetoN;
vector <pipefd> pipetoC;
int UIDtable[MAX_CLIENT+1];

void clientlogout(vector<Clientinf>::iterator currentclient);
vector<Clientinf>::iterator getUIDbyUID(int UID);
bool ifccmd(vector<Clientinf>::iterator it , vector<string> prestr,int ifd,vector<string> pathtable,string inp);
int AssiUId();
void detstr (string inp,char det,vector<string> &fin);
void countPipe(vector <string> inps,int &pNum,int &ifpipeN);
void npPipe(vector<string> inps,int pNum,int &pipN,vector<string> &cPath,int UID,string input);
void exepro(int s,int e,vector<string> inps,vector<string> &cPath,int cfd);
void setpipe(vector<pipefd> &pipetable,int Ncount,int pNum,int flag,int UID);
void Buildpipe(vector<pipefd> &pipetable,int i,int lastfork);
void BuildtoNpipe(vector<pipefd> &pipetable,int i,int Nposi,vector<int> &flag,int UID);
void closepipe(vector<pipefd> &pipetable,int i,int lastfork);
void contoN(int status,int UID);
void CHandler(int Csig);
int Openfd(vector<string> inps,string cPath,int pipefd);
void nprintenv(vector<Clientinf>::iterator it,string pathname);
void npsetenv(string pathName, string pathvalue,vector<Clientinf>::iterator it);
void spath(vector<string> &pathtable);
int pipetosame(int pipN,int UID);
void welcome(int sock);
vector<Clientinf>::iterator getUIDbyfd(int ifd);
void pipNcount(int UID);
void Cbroadcast(int mode,vector<Clientinf>::iterator currentClient,int targetClient,string mes);
bool ifclientexist(int UID);

int main (int argc,char **argv)
{

    int newsockfd,childpid;
    struct sockaddr_in cli_addr,serv_addr;
    socklen_t clilen;
    fd_set rfds;
    int PORT = atoi(argv[1]);
    if (( sockfd = socket(AF_INET, SOCK_STREAM,0))<0)
    {   cerr<<"server erro!" << endl; }
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    envaild initenv;
    initenv.pathname = "PATH";
    initenv.pathvalue = "bin:.";

    vector<string> pathtable;
    setenv("PATH","bin:.",1);
    string cPath = getenv("PATH");//"/home/ssidss/np/p1/bin/";
    detstr(cPath,':',pathtable);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0)
    { cerr<< "bind error" << endl;}
    signal(SIGCHLD,CHandler);
    listen(sockfd,5);
    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(sockfd,&afds);
    while(1)                      ///////////////////////
    {
        //listen ( sockfd, 5);
        memcpy(&rfds,&afds,sizeof(rfds));
        int ssock,ifd,se;
        se=select(nfds,&rfds,(fd_set *)0,(fd_set*)0,(struct timeval *)0);
        if (se<0){ 
            if (errno==EINTR){
                continue;
            } else {
                perror("select"); 
            }
        }          /////////////select
        for (ifd=0;ifd<nfds;ifd++)
        {
            if (FD_ISSET(ifd,&rfds)) {
                if (ifd == sockfd) { 
                    clilen = sizeof(cli_addr);
                    newsockfd = accept (sockfd,(struct sockaddr*)&cli_addr, & clilen); 
                                  
                    //cout << "acept" << endl;
                    if (newsockfd < 0) {
                        if (errno == EINTR){
                            continue; 
                        } else {
                            perror("accept");
                        }                    
                    } else {
                        Clientinf temp;
                        temp.set(newsockfd,AssiUId(),"(no name)",inet_ntoa(cli_addr.sin_addr),"",ntohs(cli_addr.sin_port));
                        temp.PATHenv.push_back(initenv);
                        clientable.push_back(temp);
                        FD_SET(newsockfd,&afds);
                        //welcome(newsockfd); 
                        vector<Clientinf>::iterator it = getUIDbyUID(temp.urid);
                        //Cbroadcast(1,it,-1,"");
                        send(newsockfd,"% ",2,0);
                    }
                } else {            
        /////////////////////////*** shell ***//////////               
                    string inp;
                    int pNum,pipN;
                    vector<string> prestr;    
                    int UID;
                    char s1[15000];
                    vector<Clientinf>::iterator currentclient = getUIDbyfd(ifd);
                    UID=(*currentclient).urid;
                    //while (1) 
                    prestr.clear();
                    for(int pathi=0;pathi<(*currentclient).PATHenv.size();pathi++) {
                        setenv((*currentclient).PATHenv[pathi].pathname.c_str(),(*currentclient).PATHenv[pathi].pathvalue.c_str(),1);
                    }
                    dup2(ifd,1);
                    dup2(ifd,2);
                    if(recv(ifd,s1,15000,0)<0){                        
                        clientlogout(currentclient); 
                        //Cbroadcast(3,currentclient,-1,"");
                        close(ifd);
                        FD_CLR(ifd,&afds);
                        UIDtable[(*currentclient).urid]=0;
                        clientable.erase(currentclient);
                    } else {
                        inp.assign(s1);
                        int rpnp;
                        while (1)
                        {
                            rpnp=inp.find('\r',0);        
                            if (rpnp<inp.size()&&rpnp>=0){
                                inp.erase(rpnp);
                            } else {break;}
                        }
                        while (1)
                        {
                            rpnp=inp.find('\n',0);        
                            if (rpnp<inp.size()&&rpnp>=0){
                                inp.erase(rpnp);
                            } else {break;}
                        }
                        detstr(inp,' ',prestr);         
                        pipN=0;
                        if (inp.empty()){
                            cout << "% ";
                            fflush(stdout);
                            continue;
                        }
                                               
                        if (prestr.size()==0) {
                            send((*currentclient).fdNum,"% ",2,0);
                            continue;                            
                        }
                        if(inp.find("exit")==0) { 
                            clientlogout(currentclient); 
                            //Cbroadcast(3,currentclient,-1,"");
                            close(ifd);
                            close(1);
                            close(2);
                            dup2(0,1);
                            dup2(0,2);
                            FD_CLR(ifd,&afds);
                            UIDtable[(*currentclient).urid]=0;
                            clientable.erase(currentclient);
                                                       
                            //exit(0);
                            continue;
                        }
                        
                        
                        if (ifccmd(currentclient,prestr,ifd,pathtable,inp)) {
                            continue;
                        }

                        //cout << pipN << endl; 
                        if (pipN==0) { pipN-=1; }
                        countPipe(prestr,pNum,pipN);
                        npPipe(prestr,pNum,pipN,pathtable,UID,inp);
                        pipNcount(UID); 
                        usleep(1000);
                        send((*currentclient).fdNum,"% ",2,0);
                                       
                        for(int pathi=0;pathi<(*currentclient).PATHenv.size();pathi++) {
                            unsetenv((*currentclient).PATHenv[pathi].pathname.c_str());
                        }                    
                    }
                    //}
    //////////////////////////////////////
                }
            }
                
            
        }
    }
    cout << "return " << endl;
    return 0;
}

void nprintenv(vector<Clientinf>::iterator it,string pathname)
{
    for (int i=0;i<(*it).PATHenv.size();i++)
    {
        if ((*it).PATHenv[i].pathname.compare(pathname)==0){
            cout << (*it).PATHenv[i].pathvalue << endl;
        }
    } 
    return ;
};
void npsetenv(string pathName, string pathvalue,vector<Clientinf>::iterator it)
{
    for (int i=0;i<(*it).PATHenv.size();i++)
    {
        if (pathName==(*it).PATHenv[i].pathname) {
            (*it).PATHenv[i].pathvalue=pathvalue;
            return ;
        }
    }
    envaild temp;
    temp.pathname=pathName;
    temp.pathvalue=pathvalue;
    (*it).PATHenv.push_back(temp);
};

void clientlogout(vector<Clientinf>::iterator currentclient)
{
    for (int i=0;i<pipetoN.size();i++)
    {
        if (pipetoN[i].pipNower==(*currentclient).urid&&pipetoN[i].pipN>=0){
            close(pipetoN[i].InPipe);
            close(pipetoN[i].OutPipe);
            //pipetoN.erase(i)
        }
    }
    vector<pipefd>::iterator it=pipetoN.begin();
    while(it!=pipetoN.end())
    {
        if((*it).pipNower==(*currentclient).urid) {
            pipetoN.erase(it);  
            continue;          
        }
        it++;
    }
    vector<pipefd>::iterator cit=pipetoC.begin();
    while(cit!=pipetoC.end())
    {
        if((*cit).pipefrom==(*currentclient).urid || (*cit).pipeto==(*currentclient).urid) {
            close((*cit).InPipe);
            close((*cit).OutPipe);
            pipetoC.erase(cit);  
            continue;          
        }
        it++;
    }

};

bool ifccmd(vector<Clientinf>::iterator currentclient , vector<string> prestr,int ifd,vector<string> pathtable,string inp)
{
    if(prestr[0].compare("setenv\0")==0) {
        npsetenv(prestr[1],prestr[2],currentclient);
        //cout << "% ";
        //fflush(stdout);
        send((*currentclient).fdNum,"% ",2,0);
        return true;
        //continue;
    } else if(prestr[0].compare("printenv\0")==0) {
        if (prestr.size()==1) {
            string argerr="too few arg\n";            
            send(ifd,argerr.c_str(),argerr.length(),0);
            //cout << "% ";
            //fflush(stdout);
            send((*currentclient).fdNum,"% ",2,0);
            return true;
        }
        nprintenv(currentclient,prestr[1]);
        //cout << "% ";
        //fflush(stdout);
        send((*currentclient).fdNum,"% ",2,0);
        return true;
        //continue;
    } else if(prestr[0].compare("name\0")==0) {
        vector<Clientinf>::iterator it = clientable.begin();
        string names;
        if (inp.length()>5) {
            names.assign(inp,5,inp.length()-5);
        }
        while (it!=clientable.end())
        {
            if ((*it).name.compare(names)==0) {
                string namerr="*** User '"+names+"' already exists. ***\n";
                send(ifd,namerr.c_str(),namerr.length(),0);
                //cout << "% ";
                //fflush(stdout);
                send((*currentclient).fdNum,"% ",2,0);
                return true;
            }
            it ++;
        }
        (*currentclient).name.assign(names);
        //Cbroadcast(2,currentclient,-1,(*currentclient).name);
        //cout << "% ";
        //fflush(stdout);
        send((*currentclient).fdNum,"% ",2,0);
        return true;
        //continue;
    } else if (prestr[0].compare("tell")==0 ){  
        string mesg;      
        int tarUID=atoi(prestr[1].c_str());
        if(!ifclientexist(tarUID)) {
            mesg = "*** Error: user #"+prestr[1]+" does not exist yet. ***\n";
            send(ifd,mesg.c_str(),mesg.length(),0);
            //cout << "% ";
            //fflush(stdout);
            send((*currentclient).fdNum,"% ",2,0);
            return true;
        }
        int sp1=inp.find(' ',0);
        int sp2=inp.find(' ',sp1+1);
        if (inp.length()>sp2) {
            mesg.assign(inp,sp2+1,inp.length()-sp2-1);
        }
        mesg="*** "+(*currentclient).name+" told you ***: "+mesg+"\n";
        send((*getUIDbyUID(tarUID)).fdNum,mesg.c_str(),mesg.length(),0);
        send((*currentclient).fdNum,"% ",2,0);
        //cout << "% ";
        //fflush(stdout);
        return true;
    } else if (prestr[0].compare("yell")==0) {
        string mesg;
        if (inp.length()>5) {
            mesg.assign(inp,5,inp.length()-5);
            //Cbroadcast(4,currentclient,-1,mesg);
        }
        send((*currentclient).fdNum,"% ",2,0);
        //cout << "% ";
        //fflush(stdout);
        return true;
    } else if (prestr[0].compare("who\0")==0) {
        cout << "<ID>\t<nickname>\t<IP/port>\t<indicate me>"<< endl;
        for (int i=0;i<=MAX_CLIENT;i++)
        {
            if (UIDtable[i]!=0){
                vector<Clientinf>::iterator it = getUIDbyUID(i);
                if (i==(*currentclient).urid){
                    cout << (*it).urid<<"\t"<<(*it).name<<"\t"<<"CGILAB"<<"/"<<511<<"\t"<<"<-me"<<endl;
//                    cout << (*it).urid<<"\t"<<(*it).name<<"\t"<<(*it).ipaddr<<"/"<<to_string((*it).portN)<<"\t"<<"<-me"<<endl;
                } else{
                    cout << (*it).urid<<"\t"<<(*it).name<<"\t"<<"CGILAB"<<"/"<<511<<endl;
//                    cout << (*it).urid<<"\t"<<(*it).name<<"\t"<<(*it).ipaddr<<"/"<<to_string((*it).portN)<<endl;
                }                
            }
        }
        send((*currentclient).fdNum,"% ",2,0);
        //cout << "% ";
        //fflush(stdout);
        return true;        
    }
    return false;

};

void Cbroadcast(int mode,vector<Clientinf>::iterator currentClient,int targetClient,string mes)
{
    vector<Clientinf>::iterator TC;
    if (targetClient>=0) {
        TC=getUIDbyfd(targetClient);
    }
    string temp;
    for (int ifd=0;ifd<nfds;ifd++)
    {
        temp.clear();
        if (!FD_ISSET(ifd,&afds)) {continue;}
        if (ifd==sockfd){continue;}
        
        switch(mode) {
            case 1: /////////////cas 1 for who
                //temp="*** User '"+(*currentClient).name+"' entered from "+(*currentClient).ipaddr+"/"+to_string((*currentClient).portN)+". ***\n";
                temp="*** User '"+(*currentClient).name+"' entered from "+"CGILAB"+"/"+"511"+". ***\n";                
                //temp.erase(temp.find('\0',0),);
                break;
            case 2: ////case 2 for name
                //temp="*** User from "+(*currentClient).ipaddr+"/"+to_string((*currentClient).portN)+" is named '"+(*currentClient).name+"'. ***\n";
                temp="*** User from CGILAB/511 is named '"+(*currentClient).name+"'. ***\n";
                break;
            case 3:  ////for left
                temp="*** User '"+(*currentClient).name+"' left. ***\n";
                break;
            case 4: /////for yell
                temp="*** "+(*currentClient).name+" yelled ***: "+mes+"\n";
                break;
            case 5:   /////for pipe to Client 
                temp="*** "+(*currentClient).name+" (#"+to_string((*currentClient).urid)+") just piped '"+mes+"' to "+(*getUIDbyUID(targetClient)).name+" (#"+to_string((*getUIDbyUID(targetClient)).urid)+") ***\n";
                break;
            //case 6:  ///for pipe to Client doesn't exists
            //    temp="*** Error: the pipe #"+to_string(targetClient)+"->#"+to_string((*currentClient).urid)+" does not exist yet. ***\n";
            //   break;
            case 7: ////for recv pipeto Client sucess
                temp="*** "+(*currentClient).name+" (#"+to_string((*currentClient).urid)+") just received from "+(*getUIDbyUID(targetClient)).name+" (#"+to_string((*getUIDbyUID(targetClient)).urid)+") by '"+mes+"' ***\n";;
                break;
            default:
                break;

        }
        //if (send(ifd,temp.c_str(),temp.length(),0)<0) {
        //    perror("send");
       // }
        write(ifd,temp.c_str(),temp.length());
    }
};

int AssiUId()
{
    for (int i=1;i<MAX_CLIENT+1;i++)
    {
        if (UIDtable[i]==0){
            UIDtable[i]=1;
            return i;
        } 
    }
    return 0;

};

void welcome(int sock)
{
    dup2(sock,1);
    cout << "****************************************" << endl;
    cout << "** Welcome to the information server. **" << endl;
    cout << "****************************************" << endl;
    //send(sock,"****************************************\n** Welcome to the information server. **\n****************************************\n",124,0);
};
void npPipe (vector<string> inps,int pNum,int &pipN,vector<string> &cPath,int UID,string input)
{  
    ////if recv pipe erase <
    vector<Clientinf>::iterator currentClient = getUIDbyUID(UID);

    vector<string>::iterator it = inps.begin();
    int ifpipeNandC=0;
    while (it!=inps.end())
    {
        if ((*it).find('<',0)==0&&(*it).length()<=3){
            string toCN;
            toCN.assign((*it),1,(*it).length()-1);
            int CN=atoi(toCN.c_str());
            vector<pipefd>::iterator pit=pipetoC.begin();
            if (UIDtable[CN]==0) {
                string mesg = "*** Error: user #"+toCN+" does not exist yet. ***\n";
                send((*getUIDbyUID(UID)).fdNum,mesg.c_str(),mesg.length(),0);
                return;
            }
            while (pit!=pipetoC.end())
            {
                if ((*pit).pipefrom==CN&&(*pit).pipeto==UID){
                    for (int i=0;i<pipetoN.size();i++) {///build pipe
                        if (pipetoN[i].pipNower==UID&&pipetoN[i].pipN==0) {
                            dup2(pipetoN[i].InPipe,(*pit).OutPipe);                            
                            //Cbroadcast(7,getUIDbyUID(UID),CN,input);
                            close((*pit).InPipe);
                            close((*pit).OutPipe);
                            pipetoC.erase(pit);
                            ifpipeNandC=1;
                            break;
                        }
                    }
                    if(ifpipeNandC==1) {break;}
                    ifpipeNandC=1;
                    dup2((*pit).OutPipe,0);
                    //Cbroadcast(7,getUIDbyUID(UID),CN,input);
                    close((*pit).InPipe);
                    close((*pit).OutPipe);
                    pipetoC.erase(pit);
                    break;
                }
                pit++;
            }
            inps.erase(it);
            if(ifpipeNandC==1) {break;}
            string mes="*** Error: the pipe #"+toCN+"->#"+to_string((*currentClient).urid)+" does not exist yet. ***\n";

            send((*getUIDbyUID(UID)).fdNum,mes.c_str(),mes.length(),0);
            //Cbroadcast(6,getUIDbyUID(UID),CN,""); 
            //send((*getUIDbyUID(UID)).fdNum,"% ",2,0);
            //cout << "% ";
            //fflush(stdout);
            return ;           
            //break;
        }
        it ++;
    }
    

    int br=0;
    vector<pipefd> pipetab;
    int Cpipecount=0;
    int s=0,e=-1;
    int lstf,status;
    int sameflag=pipetosame(pipN,UID);
    //vector<int> flag;
    //flag.clear();
    
/*    for (int ii=0;ii<pipetoN.size();ii++) 
    {    
        if (pipetoN[ii].pipN==pipN&&pipetoN[ii].pipNower==UID) {
           flag.push_back(ii);
           break;
        }
    }  */
     //cout << "pipN=" << pipN << endl;


    for (int i=0;i<inps.size(); i++)
    { 
        usleep(100);
        if (inps[i].find('|',0)==0||inps[i].find('!',0)==0){            //det ! or | 
            if (i==inps.size()-1) {                    //pipetoN
               //setpipe(pipetab,pipN,pNum,sameflag,UID);
                //vector<pipefd>::iterator pipit=pipetoN.begin();                
                if (sameflag==0) {
                    pipefd toN;
                    int pcfd[2];
                    pipe(pcfd);
                    toN.pipNower=UID;
                    toN.OutPipe=pcfd[0];
                    toN.InPipe =pcfd[1];                
                    toN.pipN=pipN;
                    pipetoN.push_back(toN);
                }                                
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0) {

                    if (s>0){
                        dup2(pipetab[Cpipecount-1].OutPipe,0);
                    } else if (s==0){
                        contoN(0,UID);    
                    }
                    int piptonfd;
                    for (int pnfd=0;pnfd<pipetoN.size();pnfd++) {
                        if (pipetoN[pnfd].pipNower==UID && pipetoN[pnfd].pipN==pipN) {
                            piptonfd = pipetoN[pnfd].InPipe;
                            break;
                        }
                    }
                    if (inps[i].find('|',0)==0) {
                        dup2(piptonfd,1);
                    } else if (inps[i].find('!',0)==0){
                        dup2(piptonfd,2);
                        dup2(piptonfd,1);
                    }
                   exepro(s,e,inps,cPath,(*currentClient).fdNum);
                   if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                   exit(0);
               } else {       
                   //if (pipetoN.size()>0) {
                   if (s==0){ 
                       contoN(1,UID); 
                    }else if (s>0) {
                        close(pipetab[Cpipecount-1].OutPipe);
                    }                   
                   //}
                   //if (s>0){      
                   //   closepipe(pipetab,Cpipecount-1,1);                   
                   //} 
               }
            } else {                                         //for pipe ot next comd
               setpipe(pipetab,-100,pNum,sameflag,UID);
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0){    
                   //if (pipetoN.size()>0) {  

                   if(s==0)contoN(0,UID);
                   //}
                   if (inps[i].find('!',0)==0){
                       dup2(pipetab[Cpipecount].InPipe,2);
                   }
                   Buildpipe(pipetab,Cpipecount,0);
                   
                   exepro(s,e,inps,cPath,(*currentClient).fdNum);
                   exit(0);
               } else {
                   //if (pipetoN.size()>0){
                   if (s==0)contoN(1,UID);
                   //}
                   closepipe(pipetab,Cpipecount,0);
                   e=e+1;            //counter +1
                   s=e+1;
                   Cpipecount+=1;
               }
           }
        } /*else if (inps[i].find('!',0)==0){        ////////////////!!!!
        }*/ else if (inps[i].find('>',0)==0) {       ///////////>>>>>>>>>>>>>>> 
            if (inps[i].length()==1) {    ////for pipeto file
                while((lstf=fork())<0){usleep(1000);}
                if (lstf==0) {
                    //if(pipetoN.size()>0) {
                    contoN(0,UID);
                    //}
                    if (s>0){dup2(pipetab[Cpipecount-1].OutPipe,0);} 
                    if (s==0) {contoN(0,UID);}
                    int fd;
                    string ofd =/*cPath[i]+"/"+*/inps[inps.size()-1];
                    fd=open(ofd.c_str(),O_RDWR | O_CREAT | O_TRUNC,0666);
                    dup2(fd,1);
            //dup2(fd,2);
                    exepro(s,e,inps,cPath,(*currentClient).fdNum);
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                    close(fd);
                } else {
                    //if (pipetoN.size()>0) {
                    if (s==0) contoN(1,UID);
                    //}                              
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}             
                    //break;
                    waitpid(lstf,&status,0);
                    break;
                }
            } else if (inps[i].length()>1) {   ///for pipe to clien 
                string tonum;
                int towhoUID= atoi(tonum.assign(inps[i],1,inps[i].length()-1).c_str());
                if (UIDtable[towhoUID]==0) {
                    string mes;
                    mes="*** Error: user #"+tonum+" does not exist yet. ***\n";
                    send((*getUIDbyUID(UID)).fdNum,mes.c_str(),mes.length(),0);
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                    //send((*currentClient).fdNum,"% ",2,0);
                    return ;
                }
                vector<pipefd>::iterator pipit=pipetoC.begin();
                while (pipit!=pipetoC.end())
                {
                    if ((*pipit).pipefrom==UID&&(*pipit).pipeto==towhoUID) {
                        string mes;
                        mes="*** Error: the pipe #"+to_string(UID)+"->#"+tonum+" already exists. ***\n";
                        send((*getUIDbyUID(UID)).fdNum,mes.c_str(),mes.length(),0);
                        if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                        //send((*currentClient).fdNum,"% ",2,0);
                        //cout << "% ";
                        //fflush(stdout);
                        return ;
                    }
                    pipit++;
                }
                pipefd toC;
                int pcfd[2];
                pipe(pcfd);
                toC.OutPipe=pcfd[0];
                toC.InPipe =pcfd[1];                
                toC.pipeto = towhoUID;
                toC.pipefrom = UID;
                toC.pipeMes = input;
                pipetoC.push_back(toC);
                //Cbroadcast(5,getUIDbyUID(UID),towhoUID,input);
                while((lstf=fork())<0){usleep(1000);}
                if (lstf==0){

                    //contoN(0,UID);
                    dup2(toC.InPipe,1);    
                    if (s>0){dup2(pipetab[Cpipecount-1].OutPipe,0);}     
                    if (s==0){contoN(0,UID);}                               
                    exepro(s,e,inps,cPath,(*currentClient).fdNum);
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                }else {
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                    if (s==0){contoN(1,UID);}  
                    //contoN(1,UID);
                    waitpid(lstf,&status,0);
                    break;
                }
            } 
                                           /////////////?>>>>>>>>>>>>>>>>>>>
        } else {                                   ///////////////////
            e+=1;
            if (i==inps.size()-1) {
               if (s==0){
                   while((lstf=fork())<0){usleep(1000);}
                   if (lstf==0){
                      //if (pipetoN.size()>0) {
                       contoN(0,UID);
                       //}                      
                       exepro(s,e,inps,cPath,(*currentClient).fdNum);
                       exit(0);
                   } else { 
		       //waitpid(lstf,&status,0);
                       //if (pipetoN.size()>0) {
                       contoN(1,UID);
                       //}
		       waitpid(lstf,&status,0);
                   }
               } else {
                   while((lstf=fork())<0){usleep(1000);}
                   if (lstf==0){
                       
                       Buildpipe(pipetab,Cpipecount-1,1);   ///  dont need to set just red
                       exepro(s,e,inps,cPath,(*currentClient).fdNum);
                       exit(0);
                   }  else {
		       //waitpid(lstf,&status,0);
                       closepipe(pipetab,Cpipecount-1,1);
		               waitpid(lstf,&status,0);
                   }
               }
            }
        }
        //if (br==1) { break;} 
    }
    //int status;
    //waitpid(lstf,&status,0);
    pipetab.clear();
    //send((*currentClient).fdNum,"% ",2,0);
    //cout << "% ";
    //fflush(stdout);
    return ;
};

int pipetosame(int pipN,int UID)    ////0 need to create pipe
{

    int count=0;
    if (pipetoN.size()==0) { return 0;}
    for (int sfd=0;sfd<pipetoN.size();sfd++)
    {   if (pipetoN[sfd].pipN==pipN&&pipetoN[sfd].pipNower==UID) {
           return 1;
        }
    }
    return 0;
};

int Openfd(vector<string> inps,string cPath,int pipefd)
{
    int er=inps.size()-1;
    cPath=cPath+inps[er]; 
    int fd=open(inps[er].c_str(),O_RDWR | O_CREAT | O_TRUNC,0666);
    if (fd<0){cout << "openfalse" << endl;}
    dup2(fd,pipefd);
    return fd;
};

void contoN(int status,int UID)         ////0for dup   1for close 
{
    if (pipetoN.size()<=0) { return ;}
    for (int nf=0;nf<pipetoN.size();nf++) {
        //cout << "pipeton" << pipetoN[nfd].pipN << endl;
        if (pipetoN[nf].pipNower!=UID) {
            continue;
        }
        if (pipetoN[nf].pipN==0) {
           if (status==0){
               dup2(pipetoN[nf].OutPipe,0);
           }
           close(pipetoN[nf].OutPipe);
           close(pipetoN[nf].InPipe);
           break;
        }
    }
};
void setpipe(vector<pipefd>& pipetable,int Ncount,int pNum,int sameflag,int UID)
{
    pipefd temp;
    int pipe_id[2];
    //pipe(pipe_id);
    bool fi=false;
    bool end=false;
    //if (Cpipecount==0) { fi=true;}
    pipe(pipe_id);
    temp.pipN=Ncount;
    temp.setpipe(pipe_id[1],pipe_id[0],fi,end);
    temp.pipNower=UID;
    if (Ncount>0) { 
        if (sameflag==0){
            pipetoN.push_back(temp); 
        }
    } else if (Ncount==-100){
        pipetable.push_back(temp);
    } else { 
        close(pipe_id[1]);
        close(pipe_id[0]);
    }
};
void BuildtoNpipe(vector<pipefd> &pipetable,int i,int Nposi,vector<int> &flag,int UID)
{
    if (i>0){
        dup2(pipetable[i-1].OutPipe,0);
        close(pipetable[i-1].OutPipe);
    }
    if (flag.size()==0){
        dup2(pipetoN[Nposi].InPipe,1);
        close(pipetoN[Nposi].InPipe);
        close(pipetoN[Nposi].OutPipe);
    } else if (flag.size()>0) {
        for (int ff=0;ff<flag.size();ff++) {
            dup2(pipetoN[flag[ff]].InPipe,1);
            close(pipetoN[flag[ff]].InPipe);
            close(pipetoN[flag[ff]].OutPipe);
            break;
        }
    }
};

void Buildpipe(vector<pipefd> &pipetable,int i,int lastfork)
{
    if (i==0&&lastfork==0) {
        close(pipetable[i].OutPipe);
        dup2(pipetable[i].InPipe,1);
        close(pipetable[i].InPipe);
    } else if (i!=0 && lastfork==0){
        dup2(pipetable[i].InPipe,1);
        dup2(pipetable[i-1].OutPipe,0);
        close(pipetable[i].OutPipe);
        close(pipetable[i].InPipe);
        close(pipetable[i-1].OutPipe);
    } else if (lastfork==1) {
        dup2(pipetable[i].OutPipe,0);
        close(pipetable[i].OutPipe);
    }
};
void closepipe(vector<pipefd> &pipetable,int i,int lastfork)
{
     if (i==0&&lastfork==0) {
         close(pipetable[i].InPipe);
     } else if (i!=0&&lastfork==0) {
         close(pipetable[i].InPipe);
         close(pipetable[i-1].OutPipe);
     } else if (i!=0&&lastfork==1){
         close(pipetable[i].OutPipe);
     }
};
void exepro(int s,int e,vector<string> inps,vector<string> & cPath,int cfd) // for write
{ 
    int po=0;
    char* cIns[e-s+2];
    for (int jj=s;jj<e+1;jj++) {
        cIns[po] = new char[inps[jj].length()];
        strcpy(cIns[po],inps[jj].c_str());
        po+=1;
    }
    string path;
    cIns[e-s+1]=NULL;
        if (execvp(cIns[0],cIns)<0) {
            fflush(stdin);
            fflush(stdout);
            fflush(stdout);
            //close(0);
            //close(1);
            
            dup2(cfd,1);
            cout << "Unknown command: ["<<cIns[0]<< "]." << endl;
            fflush(stdin);
            fflush(stdout);
            fflush(stdout);
            
            close(cfd);
            exit(0);
	}
    //return true;
    exit(0) ;
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
    //return fin;

};
void countPipe(vector <string> inps,int &pNum,int &ifpipeN)
{
    int pipeNum=0;
    string ken;
    for (int i=0;i<inps.size();i++) 
    {
        if (inps[i].find('|',0)==0) {
            if (inps[i].length()==1){
                pipeNum+=1;
            } else { 
                ken.assign(inps[i],1,inps[i].length()-1);
                ifpipeN=atoi(ken.c_str());
            }
        }  else if (inps[i].find('!',0)==0) { 
            if (inps[i].length()==1){
                pipeNum+=1;
            } else { 
                ken.assign(inps[i],1,inps[i].length()-1);
                ifpipeN=atoi(ken.c_str());
            }
        }  else if (inps[i].find('>',0)==0) {
            if (inps[i].length()==1){
                pipeNum+=1;
            }
        }
    }
    pNum=pipeNum;
    ken.clear();
};

void CHandler(int Csig)
{
    int status;
    while (waitpid(-1,&status,WNOHANG)>0) {
    }

};

void pipNcount(int UID)
{

    if (pipetoN.size()>=1){
        for (int i=0;i<pipetoN.size();i++) {
            if (pipetoN[i].pipNower==UID) {
                pipetoN[i].pipN=pipetoN[i].pipN-1;
            }
        }
    }
    vector<pipefd>::iterator it=pipetoN.begin();
    while(it!=pipetoN.end())
    {
        if ((*it).pipN<0&&(*it).pipNower==UID) {
            pipetoN.erase(it);
            continue;
        }
        it ++;
    }

    
};

bool ifclientexist(int UID)
{
    if (UIDtable[UID]==1) {
        return true;
    } else {
        return false;
    }
};
vector<Clientinf>::iterator getUIDbyfd(int ifd)
{
    vector<Clientinf>::iterator it = clientable.begin();
    while (it != clientable.end())
    {
        if ((*it).fdNum==ifd) {
            return it;
        }
        it ++;
    }
    return it;
};
vector<Clientinf>::iterator getUIDbyUID(int UID)
{
    vector<Clientinf>::iterator it = clientable.begin();
    while (it != clientable.end())
    {
        if ((*it).urid==UID) {
            return it;
        }
        it ++;
    }
    return it;
};
