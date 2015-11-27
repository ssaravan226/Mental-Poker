#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <ctgmath>
#include <algorithm>
#include <cmath>

#define SERVER_PORT 11111
#define MAX_PENDING 5
#define MAX_LINE 256


using namespace std;

//calculates bob's inverse key
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

//calculate bob's key
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



int main(int argc, char *argv[])
{

   struct sockaddr_in client, server;
   struct hostent *hp;
   char buf[MAX_LINE],buff[MAX_LINE];
   int len, ret, n,ret1,slen;
   int s, new_s;

   bzero((char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons(0);

   s = socket(AF_INET, SOCK_DGRAM, 0);
   if (s < 0)
   {
        perror("simplex-talk: UDP_socket error");
        //exit(1);
   }
   printf("CLIENT\n");
   ret1= bind(new_s, (struct sockaddr *)&client, sizeof(client));
   if ((bind(s, (struct sockaddr *)&server, sizeof(server))) < 0)
   {
        perror("simplex-talk: UDP_bind error");
       // exit(1);
   }

   hp = gethostbyname( "localhost" );
   if( !hp )
   {
        fprintf(stderr, "Unknown host %s\n", "localhost");
        //exit(1);
   }

   bzero( (char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   bcopy( hp->h_addr, (char *)&server.sin_addr, hp->h_length );
   server.sin_port = htons(SERVER_PORT);
   slen=sizeof(server);


   //GAME PART

   //generate bob key

   int bobkey,randomprime=317;
   int m[52], encryptcards[52], alicecards[52],bobcards[52];
   int i,j=0;
   bobkey = gcd_calculate(randomprime);

   int invbobkey = keyinverse(bobkey,randomprime-1);

   printf("bob's key is %d\n",bobkey);
   for(i=48;i<=99;i++)
   {
       m[j]=i;
       j++;
   }

   //shuffle all 52 cards
  random_shuffle(&m[0],&m[51]);

   //encrypt all 52 cards
   for(j=0;j<52;j++)
   {

       encryptcards[j]=modPower(m[j],bobkey,randomprime);
       printf("%d\t",encryptcards[j]);
   }

   //send encrypted cards to alice
   ret = sendto(s,  (char*)encryptcards, sizeof(encryptcards), 0,(struct sockaddr *)&server, sizeof(server));


   //receive alice's 5 cards
   printf("\n alice cards\n");
   ret1 = recvfrom(s,alicecards ,sizeof(alicecards), 0, (struct sockaddr *)&server, (socklen_t*)&slen);
   for(int i=0;i<5;i++)
      printf("%d\t",alicecards[i]);

   //receive bob's 5 cards
   printf("\n bob cards");
   ret1 = recvfrom(s,bobcards ,sizeof(bobcards), 0, (struct sockaddr *)&server, (socklen_t*)&slen);
   for(int i=0;i<5;i++)
      printf("%d\t",bobcards[i]);

   //decrypt bob cards
   printf("\n decrypted bob cards\n");
   for(j=0;j<5;j++)
   {
       bobcards[j]=modPower(bobcards[j],invbobkey,randomprime);
       printf("%d\t",bobcards[j]);
   }
   // decrypt alice cards with bob's key
   printf("\n decrypt alice cards with bob key\n");
   for(j=0;j<5;j++)
   {
       alicecards[j]=modPower(alicecards[j],invbobkey,randomprime);
       printf("%d\t",alicecards[j]);
   }

   //send alice cards
   ret = sendto(s,  (char*)alicecards, sizeof(alicecards), 0,(struct sockaddr *)&server, sizeof(server));

   int acard1,bcard1,acard,bcard;
   int acounter=0, bcounter=0;
   int play=0, playedalice[5];
   for(i=0;i<5;i++)
   {
     bcard1=bobcards[i];
     bcard = htonl(bcard1);
     ret1 = recvfrom(s, &acard1, 4, 0, (struct sockaddr *)&client, (socklen_t*)&slen);
     acard = ntohl(acard1);
     playedalice[play]=acard;
     ret = sendto(s, (char*)&bcard, 4, 0,(struct sockaddr *)&client, sizeof(client));
     printf("\nbob card is %d\n",bcard1);
     printf("\nalice card is %d\n",acard);

     //calculate who won
     if(acard>bcard1)
     {
         printf("\n Bob won\n");
         acounter++;
     }
     else if(acard<bcard1)
     {
         printf("\n Alice won\n");
         bcounter++;
     }
     play++;
   }

   //send bob key and receive alice key
   int bobkey1,alicekey,alicekey1;
   bobkey1 = htonl(bobkey);
   ret1 = recvfrom(s, &alicekey1, 4, 0, (struct sockaddr *)&client, (socklen_t*)&slen);
   ret = sendto(s, (char*)&bobkey1, 4, 0,(struct sockaddr *)&client, sizeof(client));
   alicekey=ntohl(alicekey1);

   //find invalicekey and decrypt her cards and verify if cheating took place
   int invalicekey = keyinverse(alicekey,randomprime-1);
   for(j=0;j<5;j++)
   {
       alicecards[j]=modPower(alicecards[j],invalicekey,randomprime);
   }

   for(j=0;j<5;j++)
   {
        if(alicecards[j]!=playedalice[j])
        {
            printf("\n  Alice cheating!!!");
            break;
        }
    }
   if(j==5)
       printf("\n Bob's Verification on alice is done. There was no cheating\n");

   return 0;
}
