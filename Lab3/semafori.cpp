#include <cstdio>
#include <pthread.h>
#include <semaphore.h>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cstring>


#define BUD_MAX 32
#define BRD_MAX 32
#define BID_MAX 32
#define MAX_vel_medjuspremnika 20

using namespace std;

void ispis_reda(char* red, int ulaz, int izlaz);
void ispis_redova1(char redovi[][MAX_vel_medjuspremnika], int ulazi[], int izlazi[], int br_redova);
void ispis_redova();
void spremi_u_red(char red[][MAX_vel_medjuspremnika], int64_t br_reda, int ulaz[], int izlaz[], char x);
char dohvati_ulaz(int64_t id_dretve);
int64_t obradi_ulaz(int64_t id_dretve, char U);
void* ulazna_dretva(void* par);
void obradi(int J, char P, char& r, int64_t& t);
void* radna_dretva(void* par);
void* izlazna_dretva(void* par);



int BUD, BRD, BID;
int vel_medjuspremnika;
char UMS[BRD_MAX][MAX_vel_medjuspremnika];
char IMS[BID_MAX][MAX_vel_medjuspremnika];
pthread_t ul_dretve[BUD_MAX];
pthread_t rad_dretve[BRD_MAX];
pthread_t izl_dretve[BID_MAX];
int UMS_ulaz[BRD_MAX], UMS_izlaz[BRD_MAX];
int IMS_ulaz[BID_MAX], IMS_izlaz[BID_MAX];
sem_t semafori_UMS[BRD_MAX]; // po jedan semafor za svaki UMS red
sem_t semafori_IMS[BID_MAX]; // po jedan semafor za svaki IMS red
sem_t broj_semafori_UMS[BRD_MAX]; // po jedan brojeci (opci) semafor za svaki UMS red


void zakljucaj_za_ispis()
{
	for (int i = 0; i != BRD; i++)
	{
		sem_wait(&semafori_UMS[i]);
	}
	for (int i = 0; i != BID; i++)
	{
		sem_wait(&semafori_IMS[i]);
	}
}

void odkljucaj_za_ispis()
{
	for (int i = 0; i != BRD; i++)
	{
		sem_post(&semafori_UMS[i]);
	}
	for (int i = 0; i != BID; i++)
	{
		sem_post(&semafori_IMS[i]);
	}
}

void ispis_redova1(char redovi[][MAX_vel_medjuspremnika], int ulazi[], int izlazi[], int br_redova)
{

	for (int i = 0; i != br_redova; i++)
	{
		ispis_reda(&redovi[i][0], ulazi[i], izlazi[i]);
	}
	printf("\n");
}

void ispis_reda(char* red, int ulaz, int izlaz)
{
	for (int i = 0; i != vel_medjuspremnika; i++)
	{
		if (izlaz < ulaz)
		{
			if (i >= izlaz && i < ulaz)
				printf("%c",red[i]);
			else
				printf("-");
		}
		else
		{
			if (izlaz > ulaz)
			{
				if (i < ulaz || i >= izlaz)
					printf("%c",red[i]);
				else
					printf("-");
			}
			else
			{
				printf("-");
			}
		}
	}
	printf(" ");
}


void ispis_redova()
{
	zakljucaj_za_ispis();
	printf("UMS[]: ");
	ispis_redova1(UMS, UMS_ulaz, UMS_izlaz, BRD);
	printf("IMS[]: ");
	ispis_redova1(IMS, IMS_ulaz, IMS_izlaz, BID);
	printf("\n");
	odkljucaj_za_ispis();
}

void spremi_u_red(char red[][MAX_vel_medjuspremnika], int64_t br_reda, int ulaz[], int izlaz[], char x)
{
	int& U = ulaz[br_reda];
	int& I = izlaz[br_reda];
	char* red1  = &red[br_reda][0];
	printf("%c => UMS[%ld]\n", x, br_reda);
	if (((U + 1) % vel_medjuspremnika) == I)
	{
		// red je pun
		I = (I + 1) % vel_medjuspremnika;
	}
	red1[U] = x;
	U = (U + 1) % vel_medjuspremnika;
}

char dohvati_ulaz(int64_t id_dretve)
{
	char U = 'A' + (rand() % 26);
	printf("U%ld: dohvati_ulaz(%ld)=>%c; ", id_dretve, id_dretve, U);
	return U;
}

int64_t obradi_ulaz(int64_t id_dretve, char U)
{
	int64_t broj_reda = rand() % BRD;
	printf("obradi_ulaz(%c)=>%ld; ", U, broj_reda);
	return broj_reda;
}

void* ulazna_dretva(void* par)
{
	int64_t id = (int64_t)par;
	srand(id);
	while (1)
	{
		sleep(5);
		char U = dohvati_ulaz(id);
		int64_t T = obradi_ulaz(id, U);
		sem_wait(&semafori_UMS[T]);
		spremi_u_red(UMS, T, UMS_ulaz, UMS_izlaz, U);
		sem_post(&semafori_UMS[T]);
		sem_post(&broj_semafori_UMS[T]);
		ispis_redova();
	}
	return NULL;
}

void obradi(int J, char P, char& r, int64_t& t)
{
	printf("R%d: uzimam iz UMS[%d]=>'%c' i obradjujem\n", J, J, P);
	r = P;
	t = rand() % BID;
	sleep(3);
}

void* radna_dretva(void* par)
{
	int64_t id = (int64_t)par;
	srand((unsigned)time(NULL));
	char r;
	int64_t t;
	while (1)
	{
		sem_wait(&broj_semafori_UMS[id]);
		sem_wait(&semafori_UMS[id]);
		char P = UMS[id][UMS_izlaz[id]];
		UMS_izlaz[id]=(UMS_izlaz[id]+1)%vel_medjuspremnika;
		sem_post(&semafori_UMS[id]);
		obradi(id, P, r, t);
		sem_wait(&semafori_IMS[t]);
		spremi_u_red(IMS, t, IMS_ulaz, IMS_izlaz, r);
		sem_post(&semafori_IMS[t]);
		ispis_redova();
	}
	return NULL;
}

void* izlazna_dretva(void* par)
{
	int64_t id = (int64_t)par;
	char c = '0';
	while (1)
	{
		sleep(3);
		sem_wait(&semafori_IMS[id]);
		if (IMS_ulaz[id] != IMS_izlaz[id])
		{
			// red nije prazan
			c = IMS[id][IMS_izlaz[id]];
			IMS_izlaz[id] = (IMS_izlaz[id] + 1) % vel_medjuspremnika;
		}
		sem_post(&semafori_IMS[id]);
		printf("Izlaz %ld: %c\n", id, c);
		ispis_redova();
	}
	return NULL;
}


int main(int argc, char* argv[])
{
	// ulazni parametri programa: BUD, BRD, BID, velicina medjuspremnika
	int64_t i;
	/*argc = 5;
	argv = new char* [5];
	for (int j = 0; j != 5; j++)
	{
		argv[j] = new char[10];
	}
	strcpy(argv[1], "4");
	strcpy(argv[2], "6");
	strcpy(argv[3], "3");
	strcpy(argv[4], "5");*/
	if (argc == 5)
	{
		BUD = atoi(argv[1]);
		assert(BUD <= BUD_MAX);
		BRD = atoi(argv[2]);
		assert(BRD <= BRD_MAX);
		BID = atoi(argv[3]);
		assert(BID <= BID_MAX);
		vel_medjuspremnika = atoi(argv[4]);
		assert(vel_medjuspremnika <= MAX_vel_medjuspremnika);

		// inicijalizacije
		for (i = 0; i != BRD; i++)
		{
			UMS_ulaz[i] = 0;
			UMS_izlaz[i] = 0;
			sem_init(&semafori_UMS[i], 0, 1); // zajednicka memorija, pocetna vrijednost 1
			sem_init(&broj_semafori_UMS[i], 0, 0); // zajednicka memorije, pocetna vrijednost 0
		}

		for (i = 0; i != BID; i++)
		{
			IMS_ulaz[i] = 0;
			IMS_izlaz[i] = 0;
			sem_init(&semafori_IMS[i], 0, 1); // zajednicka memorija, pocetna vrijednost 1
		}

		for (i = 0; i != BUD; i++)
		{
			pthread_create(&ul_dretve[i], NULL, ulazna_dretva, (void*)i);
			sleep(1);
		}
		sleep(10);
		for (i = 0; i != BRD; i++)
		{
			pthread_create(&rad_dretve[i], NULL, radna_dretva, (void*)i);
			sleep(1);
		}
		sleep(10);
		for (i = 0; i != BID; i++)
		{
			pthread_create(&izl_dretve[i], NULL, izlazna_dretva, (void*)i);
			sleep(1);
		}
		// cekamo dretve
		for (i = 0; i != BUD; i++)
		{
			pthread_join(ul_dretve[i], NULL);
		}
		for (i = 0; i != BRD; i++)
		{
			pthread_join(rad_dretve[i], NULL);
		}
		for (i = 0; i != BID; i++)
		{
			pthread_join(izl_dretve[i], NULL);
		}
		// unistimo semafore
		for (i = 0; i != BRD; i++)
		{
			sem_destroy(&semafori_UMS[i]);
			sem_destroy(&broj_semafori_UMS[i]);
		}
		for (i = 0; i != BID; i++)
		{
			sem_destroy(&semafori_IMS[i]);
		}
	}
	else
	{
		printf("Pogresan broj parametara u naredbenom redku\n");
	}
	return 0;
	
	
}
