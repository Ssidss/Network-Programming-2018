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
#include <sstream>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iterator>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <errno.h>
#define MAXBUF 4096
#define MAX_CLIENT 30
#define SHMKEY ((key_t)713)
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
        int pid;
        char name[20];
        char ipaddr[20];
        char Mes[1500];
        unsigned short portN; 
        int sentTo[30];
        int recvFrom[30];       
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


//fd_set afds;
Clientinf* shmFirstAddr;
Clientinf* currentClient;
int shmid;
//int nfds;
//vector <Clientinf> clientable;
vector <pipefd> pipetoN;
//vector <pipefd> pipetoC;
//int UIDtable[MAX_CLIENT+1];

void clientlogout(vector<Clientinf>::iterator currentclient);
//vector<Clientinf>::iterator getUIDbyUID(int UID);
bool ifccmd(vector<string> prestr,vector<string> pathtable,string inp);
//int AssiUId();
void detstr (string inp,char det,vector<string> &fin);
void countPipe(vector <string> inps,int &pNum,int &ifpipeN);
void npPipe(vector<string> inps,int pNum,int &pipN,vector<string> &cPath,string input,int cfd);
void exepro(int s,int e,vector<string> inps,vector<string> &cPath,int cfd);
void setpipe(vector<pipefd> &pipetable,int Ncount,int pNum,int flag);
void Buildpipe(vector<pipefd> &pipetable,int i,int lastfork);
void BuildtoNpipe(vector<pipefd> &pipetable,int i,int Nposi,vector<int> &flag);
void closepipe(vector<pipefd> &pipetable,int i,int lastfork);
void contoN(int status);
void CHandler(int Csig);
int Openfd(vector<string> inps,string cPath,int pipefd);
void nprintenv(string pathname);
void npsetenv(string pathName, string pathvalue);
void spath(vector<string> &pathtable);
int pipetosame(int pipN);
void welcome(int sock);
void pipNcount();
void initialclientable(void);
int initialshm(void);
void ClientSig(int signo);
void exeshell(int newsockfd);
Clientinf* getcurrentClient(void);
bool ifclient(int uid);


int main (int argc,char **argv)
{
    int sockfd;

    int newsockfd,childpid,status;
    struct sockaddr_in cli_addr,serv_addr;
    socklen_t clilen;
    int PORT = atoi(argv[1]);
    if (( sockfd = socket(AF_INET, SOCK_STREAM,0))<0)
    {   cerr<<"server erro!" << endl; }
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    //vector<string> pathtable;
    //setenv("PATH","bin:.",1);
    //string cPath = getenv("PATH");//"/home/ssidss/np/p1/bin/";
    //detstr(cPath,':',pathtable);
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0)     
    { cerr<< "bind error" << endl;}
    signal(SIGCHLD,CHandler);
    signal(SIGUSR1, ClientSig);
    listen(sockfd,5);
    if (initialshm() ==-1){
        perror("shm err");
    }
    initialclientable();
    while(1)                      ///////////////////////
    {

        clilen = sizeof(cli_addr);
        newsockfd = accept (sockfd,(struct sockaddr*)&cli_addr, & clilen );           
        

        childpid=fork();

        if (childpid<0) {
            perror("fork err");            
        } else if (childpid==0) {      
            dup2(newsockfd,1);
            dup2(newsockfd,2);   
            welcome(newsockfd);

            //close(sockfd);
            cout << "init" << endl;
            currentClient=getcurrentClient();
            currentClient->fdNum=newsockfd;
            strcpy(currentClient->name,"(no name)");
            currentClient->pid=getpid();        
            strcpy(currentClient->ipaddr,"CGILAB");
            //currentClient.portN=ntohs(cli_addr.sin_port);
            currentClient->portN=511;
            cout << "exeshell" << endl;
            
            exeshell(newsockfd);
            

            
            
        } else {
            //waitpid(childpid,&status,0);
            //close(newsockfd);

        }

    }
    //cout << "return " << endl;
    return 0;
}
Clientinf* getcurrentClient(void)
{
    int i=0;
    for (i=0;i<30;i++)
    {
        if ((shmFirstAddr+i)->pid==-1) {
            (shmFirstAddr+i)->pid=i+1;
            return (shmFirstAddr+i);
        }        
    }
    return NULL;
};
bool ifclient(int uid)
{
    for (int i=0;i<30;i++)
    {
        if (shmFirstAddr[i].urid==uid)
        {  return true; }
    }
    return false;
};
void exeshell(int newsockfd)
{
    //string cPath;
    string input;
    vector <string> inps;
    char s1[15000];
    int pNum;
    vector<string> pathtable;
    setenv("PATH","bin:.",1);
    string cPath = getenv("PATH");//"/home/ssidss/np/p1/bin/";
    detstr(cPath,':',pathtable);
    while(1)
    {
        fflush(stdout);
        cout << "% " ;
        fflush(stdout);
        //send(newsockfd,"% ",2,0);
        recv(newsockfd,s1,15000,0);
        input.assign(s1);
        int rpnp;
        while (1)
        {
            rpnp=input.find('\r',0);        
            if (rpnp<input.size()&&rpnp>=0){
                input.erase(rpnp);
            } else {break;}
        }
        while (1)
        {
            rpnp=input.find('\n',0);        
            if (rpnp<input.size()&&rpnp>=0){
                input.erase(rpnp);
            } else {break;}
        }
      
        detstr(input,' ',inps);

        int pipN=-1;
        countPipe(inps,pNum,pipN);
        npPipe(inps,pNum,pipN,pathtable,input,newsockfd);
        pipNcount(); 
        inps.clear();
        input.clear();
    }
};
void ClientSig(int signo)
{
    cout << currentClient->Mes << endl;
    strcpy(currentClient->Mes,"");
};
void initialclientable(void)
{
    for(int i=0;i<30;i++)
    {
        (shmFirstAddr +i)->pid = -1;
        (shmFirstAddr +i)->fdNum  = -1;
        (shmFirstAddr +i)->urid = -1;


    }
};
int initialshm(void)
{
    shmid=shmget(SHMKEY,sizeof(Clientinf)*30,IPC_CREAT | 0600 );
    if (shmid <0)
    {
        return -1;
    }
    if ((shmFirstAddr = (Clientinf*) shmat(shmid,NULL,0))<0) {
        return -1;
    }
    return 0;
};
/*
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
*/
/*
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
*/

bool ifccmd(vector<string> prestr,vector<string> pathtable,string inp)
{
    if(prestr[0].compare("setenv\0")==0) {
        //npsetenv(prestr[1],prestr[2]);
        return true;
        //continue;
    } else if(prestr[0].compare("printenv\0")==0) {
        if (prestr.size()==1) {
            return true;
        }
        //nprintenv(prestr[1]);
        return true;
    } else if(prestr[0].compare("name\0")==0) {
        string names;
        //Cbroadcast(2,currentclient,-1,(*currentclient).name);
        return true;
        //continue;
    } else if (prestr[0].compare("tell")==0 ){  
        string mesg;        
        return true;
    } else if (prestr[0].compare("yell")==0) {
        return true;
    } else if (prestr[0].compare("who\0")==0) {
        cout << "<ID>\t<nickname>\t<IP/port>\t<indicate me>"<< endl;
        for (int i=0;i<=MAX_CLIENT;i++)
        {
        }
        return true;   

    }
    return false;

};
/*
void Cbroadcast(int mode,vector<Clientinf>::iterator currentClient,int targetClient,string mes)
{
    vector<Clientinf>::iterator TC;
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
*/

void welcome(int sock)
{
    cout << "****************************************" << endl;
    cout << "** Welcome to the information server. **" << endl;
    cout << "****************************************" << endl;
};
void npPipe (vector<string> inps,int pNum,int &pipN,vector<string> &cPath,string input,int cfd)
{  

    int ifpipeNandC=0;
    vector<string>::iterator it=inps.begin();
    while (it!=inps.end())
    {
        if ((*it).find('<',0)==0&&(*it).length()<=3){
            string toCN;
            toCN.assign((*it),1,(*it).length()-1);
            int CN=atoi(toCN.c_str());

            if (!ifclient(CN)) {
                string mesg = "*** Error: user #"+toCN+" does not exist yet. ***\n";
                contoN(1);
                //send((cfd,mesg.c_str(),mesg.length(),0);
                return;
            }
            //if pipetoC exists


            inps.erase(it);
            if(ifpipeNandC==1) {break;}
            string mes="*** Error: the pipe #"+toCN+"->#"+to_string((*currentClient).urid)+" does not exist yet. ***\n";
            contoN(1);
            send(cfd,mes.c_str(),mes.length(),0);
            return ;           
        }
        it ++;
    }
    
    
    

    int br=0;
    vector<pipefd> pipetab;
    int Cpipecount=0;
    int s=0,e=-1;
    int lstf,status;
    int sameflag=pipetosame(pipN);


    for (int i=0;i<inps.size(); i++)
    { 
        usleep(100);
        if (inps[i].find('|',0)==0||inps[i].find('!',0)==0){            //det ! or | 
            if (i==inps.size()-1) {                    //pipetoN            
                if (sameflag==0) {
                    pipefd toN;
                    int pcfd[2];
                    pipe(pcfd);
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
                        contoN(0);    
                    }
                    int piptonfd;
                    for (int pnfd=0;pnfd<pipetoN.size();pnfd++) {
                        if (pipetoN[pnfd].pipN==pipN) {
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
                   exepro(s,e,inps,cPath,cfd);
                   if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                   exit(0);
               } else {       
                   //if (pipetoN.size()>0) {
                   if (s==0){ 
                       contoN(1); 
                    }else if (s>0) {
                        close(pipetab[Cpipecount-1].OutPipe);
                    }                   
               }
            } else {                                         //for pipe ot next comd
               setpipe(pipetab,-100,pNum,sameflag);
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0){    

                   if(s==0)contoN(0);
                   if (inps[i].find('!',0)==0){
                       dup2(pipetab[Cpipecount].InPipe,2);
                   }
                   Buildpipe(pipetab,Cpipecount,0);
                   
                   exepro(s,e,inps,cPath,cfd);
                   exit(0);
               } else {
                   if (s==0)contoN(1);
                   closepipe(pipetab,Cpipecount,0);
                   e=e+1;            //counter +1
                   s=e+1;
                   Cpipecount+=1;
               }
           }
        } else if (inps[i].find('>',0)==0) {       ///////////>>>>>>>>>>>>>>> 
            if (inps[i].length()==1) {    ////for pipeto file
                while((lstf=fork())<0){usleep(1000);}
                if (lstf==0) {
                    if (s==0)contoN(0);
                    if (s>0){dup2(pipetab[Cpipecount-1].OutPipe,0);}
                    int fd;
                    string ofd =inps[inps.size()-1];
                    fd=open(ofd.c_str(),O_RDWR | O_CREAT | O_TRUNC,0666);
                    dup2(fd,1);
                    exepro(s,e,inps,cPath,cfd);
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                    close(fd);
                } else {
                    if (s==0)contoN(1);
                    //}                              
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}             
                    //break;
                    waitpid(lstf,&status,0);
                    break;
                }
            } else if (inps[i].length()>1) {   ///for pipe to clien 
                string tonum;
                int towhoUID= atoi(tonum.assign(inps[i],1,inps[i].length()-1).c_str());
                if (!ifclient(towhoUID)) {
                    string mes;
                    mes="*** Error: user #"+tonum+" does not exist yet. ***\n";
                    send(cfd,mes.c_str(),mes.length(),0);
                    if (s>0) {
                        close(pipetab[Cpipecount-1].OutPipe);
                    } else if (s==0){
                        contoN(1);
                    }
                    return ;
                }
                /*
                while (if pipe already exists )
                {
                    if ((*pipit).pipefrom==UID&&(*pipit).pipeto==towhoUID) {
                        string mes;
                        mes="*** Error: the pipe #"+to_string(UID)+"->#"+tonum+" already exists. ***\n";
                        send((*getUIDbyUID(UID)).fdNum,mes.c_str(),mes.length(),0);
                        if (s>0) {
                            close(pipetab[Cpipecount-1].OutPipe);
                        } else if (s==0) {
                            contoN(1);
                        }

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
                */
                //Cbroadcast(5,getUIDbyUID(UID),towhoUID,input);
                while((lstf=fork())<0){usleep(1000);}
                if (lstf==0){

                    //contoN(0,UID);
                    //dup2(toC.InPipe,1);    
                    if (s>0){
                        dup2(pipetab[Cpipecount-1].OutPipe,0);
                    } else if (s==0) {
                        contoN(0);
                    }
                    //open();                            
                    exepro(s,e,inps,cPath,(*currentClient).fdNum);
                    if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                }else {
                    if (s>0) {
                        close(pipetab[Cpipecount-1].OutPipe);
                    }else if (s==0) {
                        contoN(1);
                    }
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

                        contoN(0);
                       //}                      
                        exepro(s,e,inps,cPath,cfd);
                        exit(0);
                    } else { 
                        contoN(1);
                       //}
		                waitpid(lstf,&status,0);
                   }
                } else {
                    while((lstf=fork())<0){usleep(1000);}
                    if (lstf==0){
                       
                       Buildpipe(pipetab,Cpipecount-1,1);   ///  dont need to set just red
                       exepro(s,e,inps,cPath,cfd);
                       exit(0);
                    }  else {
                       closepipe(pipetab,Cpipecount-1,1);
		                waitpid(lstf,&status,0);
                   }
               }
            }
        }
        //if (br==1) { break;} 
    }

    pipetab.clear();

    return ;
};

int pipetosame(int pipN)    ////0 need to create pipe
{

    int count=0;
    if (pipetoN.size()==0) { return 0;}
    for (int sfd=0;sfd<pipetoN.size();sfd++)
    {   if (pipetoN[sfd].pipN==pipN) {
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

void contoN(int status)         ////0for dup   1for close 
{
    if (pipetoN.size()<=0) { return ;}
    for (int nf=0;nf<pipetoN.size();nf++) {
        //cout << "pipeton" << pipetoN[nfd].pipN << endl;

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
void setpipe(vector<pipefd>& pipetable,int Ncount,int pNum,int sameflag)
{
    pipefd temp;
    int pipe_id[2];
    bool fi=false;
    bool end=false;
    //if (Cpipecount==0) { fi=true;}
    pipe(pipe_id);
    temp.pipN=Ncount;
    temp.setpipe(pipe_id[1],pipe_id[0],fi,end);
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
/*void BuildtoNpipe(vector<pipefd> &pipetable,int i,int Nposi,vector<int> &flag)
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
};*/

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

void pipNcount()
{

    if (pipetoN.size()>=1){
        for (int i=0;i<pipetoN.size();i++) {
            
                pipetoN[i].pipN=pipetoN[i].pipN-1;
            
        }
    }
    vector<pipefd>::iterator it=pipetoN.begin();
    while(it!=pipetoN.end())
    {
        if ((*it).pipN<0) {
            pipetoN.erase(it);
            continue;
        }
        it ++;
    }

    
};

