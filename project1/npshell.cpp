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
using namespace std;


class pipefd {
    public:
        int InPipe;
        int OutPipe;
        bool ifr;
        bool ife;
        int pipN;
        void setpipe(int InPipe,int OutPipe,bool ifr,bool ife) {
            this->InPipe=InPipe;
            this->OutPipe=OutPipe;
            this->ifr=ifr;
            this->ife=ife;
        };
};


void detstr (string inp,char det,vector<string> &fin);
void countPipe(vector <string> inps,int &pNum,int &ifpipeN);
void npPipe(vector<pipefd> &pipetoN,vector<string> inps,int pNum,int &pipN,vector<string> &cPath);
void exepro(int s,int e,vector<string> inps,vector<string> &cPath,int &br);
void setpipe(vector<pipefd> &pipetable,vector<pipefd>&pipetoN,int Ncount,int pNum,int flag);
void Buildpipe(vector<pipefd> &pipetable,int i,int lastfork);
void BuildtoNpipe(vector<pipefd> &pipetable,vector<pipefd> &pipetoN,int i,int Nposi,vector<int> &flag);
void closepipe(vector<pipefd> &pipetable,int i,int lastfork);
void contoN(vector<pipefd>&pipetoN,int status);
void CHandler(int Csig);
int Openfd(vector<string> inps,string cPath,int pipefd);
void nprintenv(string pathName);
void npsetenv(string pathName, string targetpath,vector<string>&pathtable);
void spath(vector<string> &pathtable);
int pipetosame(vector<pipefd> &pipetoN,int pipN);


int main (void)
{
    //int Csig;
    signal(SIGCHLD,CHandler);
    string inp;
    int pNum,pipN;
    vector<string> prestr;    
    vector<pipefd> pipetoN;
    vector<string> pathtable;
    setenv("PATH","bin:.",1);
    string cPath = getenv("PATH");//"/home/ssidss/np/p1/bin/";
    detstr(cPath,':',pathtable);
    while (1) {
        prestr.clear();
        cout << "\% ";
        pipN=0;
        getline(cin,inp);
	if (inp.empty()){continue;}
        if(inp.compare("exit\0")==0) { exit(0);}
        detstr(inp,' ',prestr);
        if(prestr[0].compare("setenv\0")==0) {
           npsetenv(prestr[1],prestr[2],pathtable);
           continue;
        }
        if(prestr[0].compare("printenv\0")==0) {
           nprintenv(prestr[1]);
           continue;
        }
        //cout << pipN << endl; 
        if (pipN==0) { pipN-=1; }
        countPipe(prestr,pNum,pipN);
        npPipe(pipetoN,prestr,pNum,pipN,pathtable);

        if (pipetoN.size()>=1){
            for (int i=0;i<pipetoN.size();i++) {
                pipetoN[i].pipN=pipetoN[i].pipN-1;
            }
        }
        int ncount=0;
        for (int i=0;i<pipetoN.size();i++) {
            if (pipetoN[i].pipN<0) {
                ncount+=1;
            }
        }
        if (ncount==pipetoN.size()) {
            pipetoN.clear();
        }
    }
    return 0;
}
void CHandler(int Csig)
{
    int status;
    while (waitpid(-1,&status,WNOHANG)>0) {
    }

};
void nprintenv(string pathName)
{
    if (getenv(pathName.c_str())!=NULL){
        cout << getenv(pathName.c_str()) << endl; 
    } else { 
        cout << "getenv err" << endl;
    }
};
void npsetenv(string pathName, string targetpath,vector<string> &pathtable)
{
    if(setenv(pathName.c_str(),targetpath.c_str(),1)==-1) {
        cout << "setenv err!" << endl;
    } else {
        string path=getenv("PATH");
        pathtable.clear();
        detstr(path,':',pathtable);
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
void npPipe (vector<pipefd> &pipetoN,vector<string> inps,int pNum,int &pipN,vector<string> &cPath)
{  
    int br=0;
    vector<pipefd> pipetab;
    int Cpipecount=0;
    int s=0,e=-1;
    int lstf,status;
    int sameflag=pipetosame(pipetoN,pipN);
    vector<int> flag;
    flag.clear();
    for (int ii=0;ii<pipetoN.size();ii++) 
    {    
        if (pipetoN[ii].pipN==pipN) {
           flag.push_back(ii);
        }
    }

    //cout <<"piptoNsize=" << pipetoN.size() << endl;
    //if (pipetoN.size()>0){
    //cout << "pipN=" << pipetoN[0].pipN << endl;
    //}
    for (int i=0;i<inps.size(); i++)
    { 
        if (inps[i].find('|',0)==0){            //det ! or | 
           if (i==inps.size()-1) {                    //pipetoN
               setpipe(pipetoN,pipetoN,pipN,pNum,sameflag);
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0) {
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,0);
                   }                   
                   int Nposi=pipetoN.size()-1;                      //for count=0        
                   BuildtoNpipe(pipetoN,pipetab,Cpipecount,Nposi,flag);                   
                   exepro(s,e,inps,cPath,br);
                   exit(0);
               } else {       
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,1);
                   }
                   if (s>0){      
                      closepipe(pipetab,Cpipecount-1,1);                   
                   } 
               }
           } else {                                         //for pipe ot next comd
               setpipe(pipetab,pipetoN,0,pNum,sameflag);
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0){    
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,0);
                   }
                   Buildpipe(pipetab,Cpipecount,0);
                   exepro(s,e,inps,cPath,br);
                   exit(0);
               } else {
                   if (pipetoN.size()>0){
                       contoN(pipetoN,1);
                   }
                   closepipe(pipetab,Cpipecount,0);
                   e=e+1;            //counter +1
                   s=e+1;
                   Cpipecount+=1;
               }
           }
        } else if (inps[i].find('!',0)==0){        ////////////////!!!!
           if (i==inps.size()-1) {                    //pipetoN
               setpipe(pipetoN,pipetoN,pipN,pNum,sameflag);
               while((lstf=fork())<0){usleep(1000);}
               if (lstf==0) {
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,0);
                   }
                   int Nposi=pipetoN.size()-1;                      //for count=0        
                   //dup2(pipetoN[Nposi].InPipe,2)
                   if (flag.size()==0){
                        dup2(pipetoN[Nposi].InPipe,2);
                   } else if (flag.size()>0) {
                      for (int ff=0;ff<flag.size();ff++) {
                          dup2(pipetoN[flag[ff]].InPipe,2);
                       }
                   }
                   BuildtoNpipe(pipetoN,pipetab,Cpipecount,Nposi,flag);             
                   exepro(s,e,inps,cPath,br);
                   exit(0);
               } else {
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,1);
                   }
                   if (s>0){
                      closepipe(pipetab,Cpipecount-1,1);
                   }
               }
           } else {                                         //for pipe ot next comd
               setpipe(pipetab,pipetoN,0,pNum,sameflag);
               while((lstf=fork()),0){usleep(1000);}
               if (lstf==0){
                   if (pipetoN.size()>0) {
                       contoN(pipetoN,0);
                   }
                   dup2(pipetab[Cpipecount].InPipe,2);
                   Buildpipe(pipetab,Cpipecount,0);
                   exepro(s,e,inps,cPath,br);
                   exit(0);
               } else {
                   if (pipetoN.size()>0){
                       contoN(pipetoN,1);
                   }
                   closepipe(pipetab,Cpipecount,0);
                   e=e+1;            //counter +1
                   s=e+1;
                   Cpipecount+=1;
               }
           }
        } else if (inps[i].find('>',0)==0) {       ///////////>>>>>>>>>>>>>>> 
            while((lstf=fork())<0){usleep(1000);}
            if (lstf==0) {
                if(pipetoN.size()>0) {
                   contoN(pipetoN,0);
                }
                if (s>0){dup2(pipetab[Cpipecount-1].OutPipe,0);}
                int fd;
                //for (int i=0;i<cPath.size();i++){
                    string ofd =/*cPath[i]+"/"+*/inps[inps.size()-1];
                    fd=open(ofd.c_str(),O_RDWR | O_CREAT | O_TRUNC,0666);
                  //  if (fd>=0) { break;}
                //}
                dup2(fd,1);
		//dup2(fd,2);
                exepro(s,e,inps,cPath,br);
                if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}
                close(fd);
            } else {
                if (pipetoN.size()>0) {
                    contoN(pipetoN,1);
                }                              
                if (s>0) {close(pipetab[Cpipecount-1].OutPipe);}             
                //break;
		waitpid(lstf,&status,0);
		break;
            }                             /////////////?>>>>>>>>>>>>>>>>>>>
        } else {                                   ///////////////////
            e+=1;
            if (i==inps.size()-1) {
               if (s==0){
                   while((lstf=fork())<0){usleep(1000);}
                   if (lstf==0){
                      if (pipetoN.size()>0) {
                         contoN(pipetoN,0);
                       }
                       exepro(s,e,inps,cPath,br);
                       exit(0);
                   } else { 
		       //waitpid(lstf,&status,0);
                       if (pipetoN.size()>0) {
                          contoN(pipetoN,1);
                       }
		       waitpid(lstf,&status,0);
                   }
               } else {
                   while((lstf=fork())<0){usleep(1000);}
                   if (lstf==0){
                       Buildpipe(pipetab,Cpipecount-1,1);   ///  dont need to set just red
                       exepro(s,e,inps,cPath,br);
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
    return ;
};

int pipetosame(vector<pipefd> &pipetoN,int pipN)
{
    int count=0;
    if (pipetoN.size()==0) { return 0;}
    for (int sfd=0;sfd<pipetoN.size();sfd++)
    {   if (pipetoN[sfd].pipN==pipN) {
           count+=1;
        }
    }
    if (count >=1) { 
       return 1;
    } else { return 0;}
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

void contoN(vector<pipefd>&pipetoN,int status)         ////0for dup   1for close 
{
    for (int nfd=0;nfd<pipetoN.size();nfd++) {
        //cout << "pipeton" << pipetoN[nfd].pipN << endl;
        if (pipetoN[nfd].pipN==0) {
           if (status==0){
               dup2(pipetoN[nfd].OutPipe,0);
           }
           close(pipetoN[nfd].OutPipe);
           close(pipetoN[nfd].InPipe);
           break;
        }
    }
};
void setpipe(vector<pipefd>& pipetable,vector<pipefd> &pipetoN,int Ncount,int pNum,int sameflag)
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
    if (Ncount>0) { 
        if (sameflag==0){
            pipetoN.push_back(temp); 
        }
    } else if (Ncount==0){
        pipetable.push_back(temp);
    }
};
void BuildtoNpipe(vector<pipefd> &pipetoN,vector<pipefd> &pipetable,int i,int Nposi,vector<int> &flag)
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
void exepro(int s,int e,vector<string> inps,vector<string> & cPath,int &br) // for write
{ 
    int po=0;
    char* cIns[e-s+2];
    for (int jj=s;jj<e+1;jj++) {
        cIns[po] = new char[inps[jj].length()];
        strcpy(cIns[po],inps[jj].c_str());
        po+=1;
    }
    string path;
    int errcount=0;
    cIns[e-s+1]=NULL;
        if (execvp(cIns[0],cIns)<0) {
                cerr << "Unknown command: ["<<cIns[0]<< "]." << endl;
	}
    exit(0) ;
};





