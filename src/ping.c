#include   <winsock2.h>  
  #include   <stdio.h>  
  #include   <stdlib.h>  
  #include   <iostream>  
  using   namespace   std;  
  #define   MAX_PING_PACKET_SIZE   1024  
  #define   ICMP_CEHO_REQUEST   8  
  #define   PACKET_SIZE   32  
  #define   xmalloc(s)   HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))  
  typedef   struct   ICMP_HEADER  
  {  
  BYTE   i_type;  
  BYTE   i_code;  
  USHORT   i_cksum;  
  USHORT   i_id;  
  USHORT   i_seq;  
  ULONG   timestamp;  
  }   ICMPHeader;  
  typedef   struct   IP_HEADER  
  {  
  unsigned   int   h_len:4;                     //   length   of   the   header  
  unsigned   int   version:4;                 //   Version   of   IP  
  unsigned   char   tos;                           //   Type   of   service  
  unsigned   short   total_len;             //   total   length   of   the   packet  
  unsigned   short   ident;                     //   unique   identifier  
  unsigned   short   frag_and_flags;   //   flags  
  unsigned   char     ttl;    
  unsigned   char   proto;                       //   protocol   (TCP,   UDP   etc)  
  unsigned   short   checksum;               //   IP   checksum  
   
  unsigned   int   sourceIP;  
  unsigned   int   destIP;  
  }IPHeader;  
  USHORT   checksum(USHORT   *buffer,   int   size);  
  int   inisocket()  
  {  
          WORD   wVR=MAKEWORD(2,1);  
  WSADATA   wsaDATA;  
  if(WSAStartup(wVR,&wsaDATA)!=0)//³õÊ¼»¯Ì×½Ó×Ö  
  {  
  cerr<<"Error"<<WSAGetLastError()<<"when   ini"<<endl;  
  return   1;  
  }  
  return   0;  
  }  
  int   allocate_buffers(ICMPHeader*&   send_buf,IPHeader*&   recv_buf,int   packet_size)  
  {  
  send_buf=(ICMPHeader*)new   char[packet_size];  
  if(send_buf==0)  
  {  
  cerr<<"Failed   to   allocate   buffer."<<endl;  
  return   -1;  
  }  
  recv_buf=(IPHeader*)   new   char[MAX_PING_PACKET_SIZE];  
  if(recv_buf==0)  
  {  
  cerr<<"Failed   to   allocate   output   buffer."<<endl;  
  return   -1;  
  }  
  return   0;  
  }  
  int   setup_for_ping(char*   host,int   ttl,   SOCKET&   sd,sockaddr_in&   dest)  
  {  
  sd=WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,0,0,0);  
  if(sd   ==INVALID_SOCKET)  
  {  
  cerr<<"Failed   to   create   raw   socket:"<<WSAGetLastError()<<endl;  
  return   -1;  
  }  
  if(setsockopt(sd,IPPROTO_IP,SO_SNDTIMEO,(const   char*)&ttl,sizeof(ttl))==SOCKET_ERROR)  
  {  
  cerr<<"TTL   setsockopt   failed:"<<WSAGetLastError()<<endl;  
  return   -1;  
  }  
  memset(&dest,0,sizeof(dest));  
  unsigned   int   addr=inet_addr(host);  
  if(addr!=INADDR_NONE)  
  {  
  dest.sin_addr.s_addr=addr;  
  dest.sin_family=AF_INET;  
  }  
  else  
  {  
  hostent*   hp=gethostbyname(host);  
  if(hp!=0)  
  {  
  memcpy(&(dest.sin_addr),hp->h_addr_list,hp->h_length);  
  dest.sin_family=hp->h_addrtype;  
  }  
  else  
  {  
  cerr<<"Failed   to   resolve"<<host<<endl;  
  return   -1;  
  }  
  }  
  return   0;  
  }  
  void   init_ping_packet(ICMPHeader*   icmp_hdr,int   packet_size,int   seq_no)  
  {  
  icmp_hdr->i_type=ICMP_CEHO_REQUEST;  
  icmp_hdr->i_code=0;  
  icmp_hdr->i_cksum=0;  
  icmp_hdr->i_id=(USHORT)GetCurrentProcessId();  
  icmp_hdr->timestamp=GetTickCount();  
  const   unsigned   long   int   deadmeat=0xDEADBEEF;  
  char*   datapart=(char*)icmp_hdr+sizeof(ICMPHeader);  
  int   bytes_left=packet_size-sizeof(ICMPHeader);  
  while(bytes_left>0)  
  {  
  memcpy(datapart,&deadmeat,min(int(sizeof(deadmeat)),bytes_left));  
  bytes_left-=sizeof(deadmeat);  
  datapart+=sizeof(deadmeat);  
  }  
  icmp_hdr->i_cksum=checksum((USHORT*)icmp_hdr,packet_size);  
  }  
  int   send_ping(SOCKET   sd,const   sockaddr_in&   dest,char*   send_buf,int   packet_size)  
  {  
  cout<<"Sending"<<packet_size<<"bytes   to"<<  
  inet_ntoa(dest.sin_addr)<<"..."<<flush;  
  int   bwrote=sendto(sd,send_buf,packet_size,0,  
  (sockaddr*)&dest,sizeof(dest));  
  if(bwrote==SOCKET_ERROR)  
  {  
  cerr<<"send   failed:"<<WSAGetLastError()<<endl;  
  return   -1;  
  }  
  else   if(bwrote<packet_size)  
  {  
  cout<<"sent"<<bwrote<<"bytes..."<<flush;  
  }  
  return   0;  
  }  
  int   recv_ping(SOCKET   sd,sockaddr_in&   source,char   *recv_buf,int   packet_size)  
  {  
  int   fromlen=sizeof(source);  
  int   bread=recvfrom(sd,recv_buf,packet_size+sizeof(IPHeader),  
  0,(sockaddr*)   &source,&fromlen);  
  if(bread==SOCKET_ERROR)  
  {  
  cerr<<"read   failed:";  
  if(WSAGetLastError()==WSAEMSGSIZE)  
  {  
  cerr<<"buffer   too   small"<<endl;  
  }  
  else  
  {  
  cerr<<"error   #"<<WSAGetLastError()<<endl;  
  }  
  return   -1;  
  }  
  return   0;  
  }  
  USHORT   checksum(USHORT   *buffer,   int   size)   {  
   
      unsigned   long   cksum=0;  
   
      while(size   >1)   {  
  cksum+=*buffer++;  
  size   -=sizeof(USHORT);  
      }  
       
      if(size   )   {  
  cksum   +=   *(UCHAR*)buffer;  
      }  
   
      cksum   =   (cksum   >>   16)   +   (cksum   &   0xffff);  
      cksum   +=   (cksum   >>16);  
      return   (USHORT)(~cksum);  
  }  
  void   main()  
  {  
  inisocket();  
  SOCKET   sd;  
  sockaddr_in   dest,source;  
  char   *argv;  
          char   *recv_buf;  
  char   *send_buf;  
  int   ttl=1000;  
  cout<<"Please   input   host:"<<endl;  
  cin>>argv;  
  if(setup_for_ping(argv,ttl,sd,dest)<0)  
          recv_buf   =   (char*)xmalloc(MAX_PING_PACKET_SIZE);  
          send_buf   =   (char*)xmalloc(MAX_PING_PACKET_SIZE);  
   
  init_ping_packet((ICMPHeader*)   send_buf,PACKET_SIZE,0);  
  if(send_ping(sd,dest,send_buf,PACKET_SIZE)>=0)  
  {  
  while(1)  
  if(recv_ping(sd,source,recv_buf,MAX_PING_PACKET_SIZE)<0)  
  {  
  unsigned   short   header_len=((IPHeader*)recv_buf)->h_len*4;  
  ICMPHeader   *icmphdr=(ICMPHeader*)(recv_buf+header_len);  
  if(icmphdr->i_seq!=0)  
  {  
  cerr<<"bad   sequence   number!"<<endl;  
  continue;  
  }  
  else  
  {  
  break;  
  }  
  }  
  }  
  WSACleanup();  
  }