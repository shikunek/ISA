#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

int schranka_m; // schranka pro multicast
int schranka_u; // schranka pro unicast
struct ip_mreq mlt; // struktura k ipv4
struct ipv6_mreq mlt6; // struktura k ipv6
ssize_t reading; // pro cteni
ssize_t sending; // pro zapis
struct addrinfo hint, *res = NULL; // pro funkci addrinfo
char protokol; // urceni protokolu pro skupinu
char membership; // urceni vztahu pro skupinu
int typ_m = 0; 
int typ_u = 0; 

void napoveda()
{
	printf("\nProjekt do predmetu ISA\n"
	        "Vypracoval: Petr Polanský , xpolan07\n"
		"Popis: Projekt slouzi pro preposilani dat\n"
			"       z multicastove adresy na unicastovou\n"
			"       adresu. Funguje i pro streamovani videa.\n\n"
			"       Program se spousti s parametry:\n"
			"       ./m2u -g multicastova adresa:port -h unicastova:port\n"
			"       - pokud se zadava ipv6 adresa, musi byt v hranatych\n" 
			"         zavorkach []\n"
			"       - jako multicast adresa lze zadat ipv4 nebo ipv6\n"
			"       - jako unicast adresu lze zadat ipv4, ipv6 nebo domenovou adresu\n"
			"       - pokud je zadan neplatny vstupni parametr, program vypise chybovou\n"
			"         hlasku a ukonci se\n"
			"       - pokud dojde k chybe po prihlaseni do multicastove skupiny, program\n"
			"         se odhlasi ze skupiny, vypise chybovou hlasku a ukonci se\n"
			"       - program se korektně ukonci volanim SIG_INT nebo SIG_TERM\n"
			"       - spustenim ./m2u --help se vypise tato napoveda\n");

	close(schranka_m);
        close(schranka_u);
        exit(1);

}

/* Zpracování parametrů z příkazové řádky */
int zprac_ip(int argc, const char **argv, char *mu , char *mp, int par)
{

	int cnt1 = 0; // pro prochazeni 
	int leva_zav = 0; // leva hranata zavorka
	int prava_zav = 0; // prava hranata zavorka
	int dvojtecka = 0; // oznaceni konce adresy a zacatku portu

	int port = 0; // pro ukladani portu
	int adresa = 0;

	while(argv[par][cnt1] != '\0') // cteni do konce prvniho parametru
    	{
        /** Pripad kdy je Ipv6 jako multicast **/

        if (argv[par][cnt1] == '[')
        {
            leva_zav = 1;
            cnt1++;
            continue;
        }
        
        if (argv[par][cnt1] == ']')
        {
            prava_zav = 1;
            leva_zav = 2; 
            cnt1+=2; // preskakujeme ']' a ':'
            continue;
        }

        /* Je to jeste adresa */

        if (argv[par][cnt1] == ':' && leva_zav == 1)
        {
            dvojtecka = 1;
        }

        /** Neni to adresa ale port **/

        if (argv[par][cnt1] == ':' && leva_zav == 2)
        {
            dvojtecka = 1;
            cnt1++;
            continue;
        }


        if (leva_zav == 1)
        {
            mu[adresa] = argv[par][cnt1];
            adresa++;
        }

        if (argv[par][cnt1] == ':' && leva_zav != 1)
        {

        	dvojtecka = 1;
        	cnt1++;
        	continue;
        }

        /** Pripad kdy je IPv4 jako multicast **/

        if (leva_zav == 0 && dvojtecka == 0)
        {
            mu[adresa] = argv[par][cnt1];
            adresa++;
        }

        

        /** Port multicastu **/

        if (dvojtecka == 1 && leva_zav != 1)
        {
            mp[port] = argv[par][cnt1];
            port++; 
        }

        cnt1++;
    }
    mu[adresa++] = '\0'; // ukoncovaci znak proti smeti


	return 1;
}

/* Rozlišení ip na ipv4 a ipv6 */
int urceni_ip(char *ip)
{
	
	int ret;

	memset(&hint, '\0', sizeof hint);

    hint.ai_family = PF_UNSPEC;
    hint.ai_flags = AI_NUMERICHOST;

    ret = getaddrinfo(ip, NULL, &hint, &res);
    if (ret) {
        
        return 2;
    }
    if(res->ai_family == AF_INET) { // je to ipv4
       	return 4;
    }
 
    else if (res->ai_family == AF_INET6) { // je to ipv6
    	  return 6;
    } 
    
    else {	
        return 2;  // je to domena
    }
	close(schranka_m);
        close(schranka_u);
        exit(1);


    return 0;
}
/* Převod domény na ip */
/* Tato funkce je prevzata viz manual.pdf */
int domena_na_ip(char * domena , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( domena ) ) == NULL)
    {
        // get the host info
        perror("Nepodarilo se ziskat ip z domeny.\n");
	close(schranka_m);
        close(schranka_u);
        exit(1);
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}


/* Zachycení signálů */
/* Tato funkce je prevzata viz. manual.pdf */
void sig_handler(int sig) 
{
    if(typ_m == 4) // pokud je multicast ipv4
	{
       		if(setsockopt(schranka_m, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mlt, sizeof(mlt)) == 0 )
		{
           	
                	close(schranka_m);
                	close(schranka_u);
			freeaddrinfo(res);

                	exit(1);
			
	        }

           	else
            	{
                	perror("Nepodarilo se odhlasit ze skupiny.\n");
                	close(schranka_m);
                	close(schranka_u);
			freeaddrinfo(res);

                	exit(1);

            	}
    	}

 	if(typ_m = 6) // pokud je multicast ipv6
	{
		if(setsockopt(schranka_m, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mlt6, sizeof(mlt6)) == 0 )
		{	
                	close(schranka_m);
                	close(schranka_u);
			freeaddrinfo(res);

                	exit(1);
			
	        }

           	else
            	{
                	perror("Nepodarilo se odhlasit ze skupiny.\n");
                	close(schranka_m);
                	close(schranka_u);
			freeaddrinfo(res);

                	exit(1);

            	}

	
	}

}

int main(int argc, char const *argv[])
{
	
	struct sockaddr_in addr_m;
	struct sockaddr_in addr_u;
	struct sockaddr_in6 addr_m6;
	struct sockaddr_in6 addr_u6;

    
	int On = 1;
	char ipecko_m;
	char ipecko_u;
	
	unsigned char ttl = 1;

	int delka;
	int len;

	fd_set rset;
	struct timeval wait; // casovani pro select
	char msg[4096]; // pole pro přenos dat

	char mult_ip[100]; // ulozeni multicast adresy
	char mult_port[20]; // ulozeni multicast portu

	int port_m;
	int port_u;

	char uni_ip[100]; // ulozeni unicast adresy
	char uni_port[20]; // ulozeni unicast portu


/* Vypis napovedy */	
    if(argc == 2 && (strcmp(argv[1], "--help") == 0))
    {
	napoveda();
    }


/* Parametry jsou chybne */

    if(argc != 5)
    {
        perror("Malo parametru!");
        exit(1);
    }


    if (strcmp(argv[1], "-g") != 0)
    {
        perror("Druhy parametr neni '-g'. ");
	exit(1);
    }



    if (strcmp(argv[3], "-h") != 0)
    {
        perror("Ctvrty parametr neni '-h'. ");
	exit(1);
    }   
    




	zprac_ip(argc, argv, mult_ip, mult_port, 2); // zavolani zpracovaní pro multicast
	zprac_ip(argc, argv, uni_ip, uni_port, 4); // zavolani zpracovani pro unicast



    /* Pokud je multicast ipv4 */
    if(urceni_ip(mult_ip)== 4) 
    {
    	ipecko_m = AF_INET; 
	protokol = IPPROTO_IP;
	membership = IP_ADD_MEMBERSHIP;
	typ_m = 4;
	memset(&addr_m, 0 , sizeof(addr_m)); // vynulovani struktury

	port_m = atoi(mult_port);

    	addr_m.sin_family = ipecko_m; 
	addr_m.sin_port = htons(port_m);
	addr_m.sin_addr.s_addr = htonl(INADDR_ANY);

	mlt.imr_multiaddr.s_addr = inet_addr(mult_ip);
	mlt.imr_interface.s_addr = htonl(INADDR_ANY);

	if((schranka_m = socket(ipecko_m, SOCK_DGRAM, 0)) == -1) // nastaveni schranky 
    	{
    		perror("Chyba funkce socket.\n");
   		exit(1);
    	}

	if(setsockopt(schranka_m, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On)) < 0)
    	{
		perror("Nastaveni SO_REUSEADDR se nezdarilo.\n");
   		close(schranka_m);
    		close(schranka_u);
    		exit(1);
    	}

	if(bind(schranka_m,(struct sockaddr*) &addr_m, sizeof(addr_m)) == -1) // bind pro ipv4
    	{
    		perror("Nedarilo se bindout.\n");
    		close(schranka_m);
    		close(schranka_u);
    		exit(1);
    	}
	
	/* Prihlaseni se to skupiny */
	if(setsockopt(schranka_m, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mlt, sizeof(mlt)) < 0 ) 
	{
		perror("Nepodarilo se prihlasit do skupiny.\n");
		close(schranka_m);
    		close(schranka_u);
    		exit(1);

	}

	
    }

    /* Pokud je multicast ipv6 */

    if (urceni_ip(mult_ip)== 6)
    {
    	ipecko_m = AF_INET6;
	protokol = IPPROTO_IPV6;
	membership = IPV6_ADD_MEMBERSHIP;
	typ_m = 6;
	memset(&addr_m6, 0 , sizeof(addr_m6)); // vynulovani struktury

	port_m = atoi(mult_port); // prevod portu
	addr_m6.sin6_scope_id = if_nametoindex("eth0"); // specifikace rozhrani
    	addr_m6.sin6_family = ipecko_m; 
	addr_m6.sin6_port = htons(port_m);
	addr_m6.sin6_flowinfo = 0;
	inet_pton(AF_INET6, mult_ip, &(addr_m6.sin6_addr)); // vlozeni adresy


	mlt6.ipv6mr_multiaddr = addr_m6.sin6_addr;
	mlt6.ipv6mr_interface = if_nametoindex("eth0");
	
	if((schranka_m = socket(ipecko_m, SOCK_DGRAM, 0)) == -1) 
    	{
    		perror("Chyba funkce socket.\n");
   		exit(1);
    	}
	/* Pro korektni prubeh bez chyb */

	if(setsockopt(schranka_m, SOL_SOCKET, SO_REUSEADDR, (char *)&On, sizeof(On)) < 0)
    	{
		perror("Nastaveni SO_REUSEADDR se nezdarilo.\n");
   		close(schranka_m);
    		close(schranka_u);
    		exit(1);
	}

	if(bind(schranka_m,(struct sockaddr*) &addr_m6, sizeof(addr_m6)) == -1)
    	{
    		perror("Nedarilo se bindout.\n");
    		close(schranka_m);
    		close(schranka_u);
    		exit(1);
    	}

	if(setsockopt(schranka_m, protokol, membership, (char *)&mlt6, sizeof(mlt6)) < 0 )
	{
		perror("Nepodarilo se prihlasit do skupiny.\n");
		close(schranka_m);
    		close(schranka_u);
    		exit(1);

	}



    }

    /* Pokud je unicast ipv4 */

    if(urceni_ip(uni_ip)== 4) 
    {
    	ipecko_u = AF_INET; 
	typ_u = 4;
	memset(&addr_u, 0 , sizeof(addr_u)); // vynulovani struktury

	port_u = atoi(uni_port);

	addr_u.sin_family = ipecko_u; 
	addr_u.sin_port = htons(port_u);
	addr_u.sin_addr.s_addr = inet_addr(uni_ip);

    }


    /* Pokud je unicast ipv6 */

    if (urceni_ip(uni_ip)== 6)
    {
	typ_u = 6;
    	ipecko_u = AF_INET6;
	memset(&addr_u6, 0 , sizeof(addr_u6)); // vynulovani struktury

	port_u = atoi(uni_port);

	addr_u6.sin6_scope_id = if_nametoindex("eth0");
    	addr_u6.sin6_family = ipecko_u; 
	addr_u6.sin6_port = htons(port_u);
	inet_pton(AF_INET6, uni_ip, &(addr_u6.sin6_addr));
	addr_m6.sin6_flowinfo = 0;



	
    }
    /* Pokud je unicast doménová adresa */

    if(urceni_ip(uni_ip) == 2)
    {
    	domena_na_ip(uni_ip, uni_ip); // prevod domény na IP
	ipecko_u = AF_INET;
	typ_u = 4;
	memset(&addr_u, 0 , sizeof(addr_u)); // vynulovani struktury

	port_u = atoi(uni_port);

	addr_u.sin_family = ipecko_u; 
	addr_u.sin_port = htons(port_u);
	addr_u.sin_addr.s_addr = inet_addr(uni_ip);

	
	
    }
   
  
	/* Nastaveni schranky pro unicast */

	if((schranka_u = socket(ipecko_u, SOCK_DGRAM, 0)) == -1) 
    	{
    		perror("Chyba funkce socket.\n");
    		exit(1);
    	}
	

	   
	while(1)
	{	
		if(typ_m == 4)
		{
			len = sizeof(addr_m);
				}
		if(typ_u == 4)
		{
			delka = sizeof(addr_u);

		}



		if(typ_m == 6)
		{
			len = sizeof(addr_m6);
		}

		if(typ_u == 6)		
		{
			delka = sizeof(addr_u6);
		}		


			 wait.tv_sec = 0.5;
   			 wait.tv_usec = 0.5;

			
			FD_ZERO(&rset); // nulovani 
			
			
			FD_SET(schranka_m, &rset); // nastaveni schranek pro select
			FD_SET(schranka_u, &rset);	
			select(schranka_m + 1, &rset, NULL, NULL, &wait);

		if(FD_ISSET(schranka_m, &rset)!= 0) // neblokující čtení z multicastové skupiny	
		{
			if(typ_m == 4) 
			{
				reading=recvfrom(schranka_m, msg, sizeof(msg), 0, (struct sockaddr *)&addr_m,&len);					
			}			

			if(typ_u == 4)
			{
				sending = sendto(schranka_u, msg, reading, 0, (struct sockaddr *)&addr_u, delka);
			}

			if(typ_m == 6)
			{
				reading=recvfrom(schranka_m, msg, sizeof(msg), 0, (struct sockaddr *)&addr_m6, &len);
			}
			if(typ_u == 6)
			{
				sending = sendto(schranka_u, msg, reading, 0, (struct sockaddr *)&addr_u6, delka);
			}

		}		
						
			signal(SIGINT, sig_handler); // zachyceni SIG_INT
			signal(SIGTERM, sig_handler); // zachyceni SIG_TERM

	
	}

    return 0;

}
