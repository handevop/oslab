#include <iostream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

using namespace std;

struct timespec t0; /* vrijeme pocetka programa */

string K_Z = "0000";
int T_P = 0;
stack<int> stog, pomoc;

/* postavlja trenutno vrijeme u t0 */
void postavi_pocetno_vrijeme()
{
	clock_gettime(CLOCK_REALTIME, &t0);
}

/* dohvaca vrijeme proteklo od pokretanja programa */
void vrijeme(void)
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);

	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}

	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

/* ispis kao i printf uz dodatak trenutnog vremena na pocetku */
#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)

/*
 * spava zadani broj sekundi
 * ako se prekine signalom, kasnije nastavlja spavati neprospavano
 */
void spavaj(time_t sekundi)
{
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR){}
		
}

void obradi_dogadjaj(int sig);
void obradi_dogadjaj1(int sig);
void obradi_sigterm(int sig);
void obradi_sigint(int sig);
void ispis();
void nastavak(int br);

int main()
{
    postavi_pocetno_vrijeme();

    struct sigaction act;
	/* 1. maskiranje signala SIGUSR1 */
    act.sa_handler = obradi_dogadjaj; /* kojom se funkcijom signal obrađuje */
    sigemptyset(&act.sa_mask);
    // sigaddset(&act.sa_mask, SIGTERM); /* blokirati i SIGTERM za vrijeme obrade */
    act.sa_flags = 0; /* naprednije mogućnosti preskočene */
    sigaction(SIGUSR1, &act, NULL); /* maskiranje signala preko sučelja OS-a */

    act.sa_handler = obradi_dogadjaj1; /* kojom se funkcijom signal obrađuje */
    sigemptyset(&act.sa_mask);
    // sigaddset(&act.sa_mask, SIGTERM); /* blokirati i SIGTERM za vrijeme obrade */
    act.sa_flags = 0; /* naprednije mogućnosti preskočene */
    sigaction(SIGUSR2, &act, NULL); /* maskiranje signala preko sučelja OS-a */

    /* 2. maskiranje signala SIGTERM */
    act.sa_handler = obradi_sigterm;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);

    /* 3. maskiranje signala SIGINT */
    act.sa_handler = obradi_sigint;
    sigaction(SIGINT, &act, NULL);

    long pid = (long) getpid();
    PRINTF("Program s PID=%ld krenuo s radom\n", pid);
    ispis();
    
    
	while(1){
        if (K_Z[3] == '1'){
            stog.push(T_P);
            T_P = 4;
            obradi_sigint(NULL);
        }
        else if (K_Z[2] == '1'){
            stog.push(T_P);
            T_P = 3;
            obradi_sigint(NULL);
        }
        else if(K_Z[1] == '1'){
            stog.push(T_P);
            T_P = 2;
            obradi_dogadjaj(NULL);
        }
        else if(K_Z[0] == '1'){
            stog.push(T_P);
            T_P = 1;
            obradi_sigterm(NULL);
        }

        sleep(15);
    }

	return 0;
}
// K_Z - maska, T_P - trenutni prioritet, stog - stog i stog pomoc za pomoc pri ispisu stanja na stogu

void obradi_sigterm(int sig) //sigterm
{
    if (T_P != 1){
        if (T_P < 1){
            PRINTF("Dogodio se prekid razine 1 i počinje obrada prekida\n");

            K_Z[0] = '1';
            stog.push(T_P);
            T_P = 1;
            ispis();
            K_Z[0] = '0';

            spavaj(15);
            PRINTF("Završila obrada razine 1\n\n");
            if (!stog.empty()){
                T_P = stog.top();
                stog.pop();
                nastavak(T_P);
                ispis();
            }
        }
        else{
            PRINTF("Dogodio se prekid razine 1 ali se on pamti i ne prosljeđuje procesoru\n");
            K_Z[0] = '1';
            ispis();
        }
    }
    if (T_P == 1){
        PRINTF("Počela obrada razine 1\n");
        K_Z[0] = '0';
        ispis();

        spavaj(15);
        PRINTF("Završila obrada razine 1\n\n");
        if (!stog.empty()){
            T_P = stog.top();
            stog.pop();
            nastavak(T_P);
            ispis();
        }
    }

}

void obradi_dogadjaj(int sig) //sigusr
{
    if (T_P != 2){
        if (T_P < 2){
            PRINTF("Dogodio se prekid razine 2 i počinje obrada prekida\n");

            K_Z[1] = '1';
            stog.push(T_P);
            T_P = 2;
            ispis();
            K_Z[1] = '0';

            spavaj(15);
            PRINTF("Završila obrada razine 2\n\n");
            if (!stog.empty()){
                T_P = stog.top();
                stog.pop();
                nastavak(T_P);
                ispis();
            }
        }
        else{
            PRINTF("Dogodio se prekid razine 2 ali se on pamti i ne prosljeđuje procesoru\n");
            K_Z[1] = '1';
            ispis();
        }
    }
    if(T_P == 2){
        PRINTF("Počela obrada razine 2\n");
        K_Z[1] = '0';
        ispis();

        spavaj(15);
        PRINTF("Završila obrada razine 2\n\n");
        if (!stog.empty()){
            T_P = stog.top();
            stog.pop();
            nastavak(T_P);
            ispis();
        }
    }

}

void obradi_dogadjaj1(int sig) //sigusr
{
    if (T_P != 4){
        if (T_P < 4){
            PRINTF("Dogodio se prekid razine 4 i počinje obrada prekida\n");

            K_Z[3] = '1';
            stog.push(T_P);
            T_P = 4;
            ispis();
            K_Z[3] = '0';

            spavaj(15);
            PRINTF("Završila obrada razine 4\n\n");
            if (!stog.empty()){
                T_P = stog.top();
                stog.pop();
                nastavak(T_P);
                ispis();
            }
        }
        else{
            PRINTF("Dogodio se prekid razine 4, ali se on pamti i ne prosljeđuje procesoru\n");
            K_Z[3] = '1';
            ispis();
        }
    }
    if(T_P == 4){
        PRINTF("Počela obrada razine 4\n");
        K_Z[3] = '0';
        ispis();

        spavaj(15);
        PRINTF("Završila obrada razine 4\n\n");
        if (!stog.empty()){
            T_P = stog.top();
            stog.pop();
            nastavak(T_P);
            ispis();
        }
    }

}

void obradi_sigint(int sig) //sigint
{
    if (T_P != 3){
        if (T_P < 3){
            PRINTF("Dogodio se prekid razine 3 i počinje obrada prekida\n");

            K_Z[2] = '1';
            stog.push(T_P);
            T_P = 3;
            ispis();
            K_Z[2] = '0';

            spavaj(15);
            PRINTF("Završila obrada razine 3\n\n");
            if (!stog.empty()){
                T_P = stog.top();
                stog.pop();
                nastavak(T_P);
                ispis();
            }
        }
        else{
            PRINTF("Dogodio se prekid razine 3 ali se on pamti i ne prosljeđuje procesoru\n");
            K_Z[2] = '1';
            ispis();
        }
    }
    if(T_P == 3){
        PRINTF("Počela obrada razine 3\n");
        K_Z[2] = '0';
        ispis();

        spavaj(15);
        PRINTF("Završila obrada razine 3\n\n");
        if (!stog.empty()){
            T_P = stog.top();
            stog.pop();
            nastavak(T_P);
            ispis();
        }
    }

}

void ispis(){
    PRINTF("K_Z = %c%c%c%c, T_P = %d, stog: ", K_Z[0], K_Z[1], K_Z[2], K_Z[3], T_P);

    if (stog.empty()) printf("__");

    while(!stog.empty()){
        int vrh = stog.top();
        pomoc.push(vrh);
        stog.pop();

        printf("%d, reg[%d]; ",vrh, vrh);
    }

    while(!pomoc.empty()){
        int vrh = pomoc.top();
        stog.push(vrh);
        pomoc.pop();
    }

    printf("\n\n");
}

void nastavak(int br){
    if (br == 0){
        PRINTF("Nastavlja se izvodenje glavnog programa\n\n");
    }
    if (br == 1){
        PRINTF("Nastavlja se obrada prekida razine 1\n\n");
    }
    if (br == 2){
        PRINTF("Nastavlja se obrada prekida razine 2\n\n");
    }
    if (br == 3){
        PRINTF("Nastavlja se obrada prekida razine 3\n\n");
    }
    if (br == 4){
        PRINTF("Nastavlja se obrada prekida razine 4\n\n");
    }

}