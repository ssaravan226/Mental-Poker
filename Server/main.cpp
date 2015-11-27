
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include<iostream>
#include <ctgmath>
#include <algorithm>
#include <cmath>


#define SERVER_PORT 11111
#define MAX_LINE 256
using namespace std;

//calculate alice's inverse key
int keyinverse(int a, int m)
{
    a %= m;
    for(int x = 1; x < m; x++) {
        if((a*x) % m == 1) return x;
    }
}


int modPower(int x,int n,int p)
{
    int u,y;
    y=1;

    u= fmod(x,p);
    while(n!=0)
    {
        if(fmod(n,2) == 1)
            y = fmod(y*u,p);
        n=floor(n/2);
        u=fmod(u*u,p);
    }
    return y;
}

//calculate alice key
int gcd_calculate(int random_number)
{
    int gcd_value,random;
    srand(time(NULL));
    while(1)
    {
    random = rand() % 200 + 100;
    gcd_value = std::__gcd(random,random_number-1);
    if (gcd_value == 1)
        return random;
    else
        continue;
    }
}


int main (int argc, char * argv[])
{
   struct hostent *hp;
   struct sockaddr_in server, client;
   char buf[MAX_LINE],buff1[MAX_LINE];
   int s, ret,ret1;
   int length=0,len=0,n;

   hp = gethostbyname("localhost");
   if (!hp)
   {
        fprintf(stderr,"simplex-talk:Unknown host: %s\n",hp);
        //exit(1);
   }
   printf("SERVER\n");
   bzero((char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   bcopy(hp->h_addr, (char *)&server.sin_addr,hp->h_length);
   server.sin_port = htons(SERVER_PORT);

   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
        perror("simplex_talk: socket error");
        //exit(1);
   }

   ret = bind(s, (struct sockaddr *)&server, sizeof(server));
   if( ret < 0)
   {
        fprintf( stderr, "Bind Error: can't bind local address");
        //exit(1);
   }

   length = sizeof(client);


//GAME PART

//generate alicekey
   int alicekey,randomprime=317;
   int encryptcards[52];
   int i,j;
   int alicecards[52],bobcards[52];
   j=0;
   alicekey = gcd_calculate(randomprime);

   int invalicekey = keyinverse(alicekey,randomprime-1);
   cout<<"invalice"<<invalicekey;
   printf("alice's key is %d\n",alicekey);

   //receive encrypted cards
   ret1 = recvfrom(s, encryptcards, sizeof(encryptcards), 0, (struct sockaddr *)&client, (socklen_t*)&length);

   srand(time(NULL));
   pick:int ai=rand()%52;
   int af=ai+5;
   int bf=rand()%52;
   int bi=bf-5;
   int flag=0;
   if(ai!=bi )
       flag=1;
   if(flag!=1)
       goto pick;
   if(flag==1)
   {
       //pick 5 cards for alice

       for(i=ai;i<af;i++)
       {
            alicecards[j]=encryptcards[i];
            j++;
       }

       //pick 5 cards for bob
       j=0;
       for(i=bi;i<bf;i++)
       {
            bobcards[j]=encryptcards[i];
            j++;
       }
   }



   printf("\n picked alice cards\n");
   for(i=0;i<5;i++)
      printf("%d\t",alicecards[i]);


   printf("\n picked bob cards\n");
   for(i=0;i<5;i++)
      printf("%d\t",bobcards[i]);

//encrypt alicecards
   for(j=0;j<5;j++)
   {
   alicecards[j]=modPower(alicecards[j],alicekey,randomprime);
   }

   printf("\n encrypted alice cards\n");
   for(i=0;i<5;i++)
      printf("%d\t",alicecards[i]);



   //send alice cards
   ret = sendto(s, (char*)alicecards, sizeof(alicecards), 0,(struct sockaddr *)&client, sizeof(client));

   //send bob cards
    ret = sendto(s, (char*)bobcards, sizeof(bobcards), 0,(struct sockaddr *)&client, sizeof(client));

    //receive alice cards decrypted by bob's key
    printf("\n received alice cards\n");
    ret1 = recvfrom(s, alicecards, sizeof(alicecards), 0, (struct sockaddr *)&client, (socklen_t*)&length);
    for(int i=0;i<5;i++)
          printf("%d\t",alicecards[i]);

    //decrypt alice cards with alice key
    printf("\n final decrypted alice cards");
    for(j=0;j<5;j++)
    {
        alicecards[j]=modPower(alicecards[j],invalicekey,randomprime);
        printf("%d\t",alicecards[j]);
    }
    int acard1,bcard1,acard,bcard;
    int acounter=0, bcounter=0;
    int play=0;
    int playedbob[5];
    for(i=0;i<5;i++)
    {
      acard1=alicecards[i];
      acard = htonl(acard1);
      ret = sendto(s, (char*)&acard, 4, 0,(struct sockaddr *)&client, sizeof(client));
      ret1 = recvfrom(s, &bcard1, 4, 0, (struct sockaddr *)&client, (socklen_t*)&length);
      bcard = ntohl(bcard1);
      playedbob[play]=bcard;
      printf("\nbob card is %d\n",bcard);
      printf("\nalice card is %d\n",acard1);

      if(acard1>bcard)
      {
          printf("\n Bob won\n");
          acounter++;
      }
      else if(acard1<bcard)
      {
          printf("\n Alice won\n");
          bcounter++;
      }
      play++;
    }

    //send alice key and receive bob key
    int alicekey1=htonl(alicekey);
    int bobkey1,bobkey;
    ret = sendto(s, (char*)&alicekey1, 4, 0,(struct sockaddr *)&client, sizeof(client));
    ret1 = recvfrom(s, &bobkey1, 4, 0, (struct sockaddr *)&client, (socklen_t*)&length);
    bobkey = ntohl(bobkey1);

    //finding bobinvkey and decrypting to verify if cheating took place
    int invbobkey = keyinverse(bobkey,randomprime-1);
    for(j=0;j<5;j++)
    {
        bobcards[j]=modPower(bobcards[j],invbobkey,randomprime);
        printf("%d\t",bobcards[j]);
    }

    for(j=0;j<5;j++)
    {
         if(bobcards[j]!=playedbob[j])
         {
             printf("\n  BOB cheating!!!");
             break;
         }
     }
    if(j==5)
        printf("\n ALice's Verification on bob is done. There was no cheating\n");


   return 0;
}
