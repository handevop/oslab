#include <pthread.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <ctime>

#define BR_CITACA 10
#define BR_PISACA 4
#define BR_BRISACA 1

using namespace std;

pthread_t citaci[BR_CITACA];
pthread_t pisaci[BR_PISACA];
pthread_t brisaci[BR_BRISACA];

pthread_mutex_t m;
pthread_cond_t red_citaca, red_pisaca, red_brisaca;

int br_citaca_ceka = 0;
int br_citaca_cita = 0;
int br_brisaca_brise = 0;
int br_brisaca_ceka = 0;
int br_pisaca_pise = 0;
int br_pisaca_ceka = 0;

struct CvorListe
{
	int el;
	CvorListe* sljed;
};

struct Lista
{
	int br_el;
	CvorListe* glava, * zadnji;
	// glava je dummy 
	Lista() : br_el(0), glava(nullptr), zadnji(nullptr)
	{
		glava = new CvorListe;
		zadnji = glava;
		glava->sljed = nullptr;
	}
	bool Prazna()
	{
		return zadnji == glava;
	}
	void dodaj(int el)
	{
		CvorListe* novi = new CvorListe;
		novi->el = el;
		novi->sljed = nullptr;
		zadnji->sljed = novi;
		zadnji = novi;
		br_el++;
	}
	void brisi(int mjesto)
	{
		// brisemo cvor na zadanom mjestu (>=1) iz liste
		CvorListe* pred = nullptr;
		int i;
		if (mjesto>=1 && mjesto <= br_el)
		{
			// odlazimo do mjesta
			for (i = 0, pred = glava; i < mjesto - 1; i++, pred = pred->sljed);
			// brisanje
			CvorListe* obrisani = pred->sljed;
			pred->sljed = obrisani->sljed;
			if (obrisani == zadnji)
				zadnji = pred;
			br_el--;
			delete obrisani;
			
		}
	}

	int citaj(int mjesto, int& podatak)
	{
		CvorListe* tmp = nullptr;
		if (mjesto >= 1 && mjesto <= br_el)
		{
			// odlazimo do mjesta
			tmp = glava;
			for (int i = 0; i < mjesto; i++, tmp = tmp->sljed);
			// citamo cvor
			podatak = tmp->el;
			return 1;
		}
		return 0;
	}

	void ispis()
	{
		printf("Lista: ");
		for (CvorListe* tmp = glava->sljed; tmp != nullptr; tmp = tmp->sljed)
			printf("%d ", tmp->el);
		printf("\n");
	}

};

Lista lista;

void ispisi_broj_aktivnih()
{
	printf("aktivnih: citaca=%d, pisaca=%d, brisaca=%d\n", br_citaca_cita, br_pisaca_pise, br_brisaca_brise);
}



void* funkcija_citaca(void* par)
{
	int64_t id = (int64_t)par;
	srand(id);
	int y = 0;
	int x = 0;
	while (1)
	{
		pthread_mutex_lock(&m);
		if (lista.br_el > 0)
			x = 1 + (rand() % lista.br_el);
		else x = 0;
		printf("Citac %ld zeli citati element %d liste\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();
		br_citaca_ceka++;
		while (br_brisaca_brise + br_brisaca_ceka > 0)
		{
			pthread_cond_wait(&red_citaca, &m);
		}
		// K.O.
		br_citaca_cita++;
		br_citaca_ceka--;
		int rez = lista.citaj(x, y);
		if (rez)
			printf("Citac %ld cita element %d liste (vrijednosti %d)\n", id, x, y);
		else
			printf("Citac %ld: ne postoji element %d liste\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();
		pthread_mutex_unlock(&m);
		sleep(5+rand()%5);
		pthread_mutex_lock(&m);
		br_citaca_cita--;
		printf("Citac %ld vise ne koristi listu\n", id);
		ispisi_broj_aktivnih();
		lista.ispis();

		if (br_citaca_cita == 0 && br_pisaca_pise==0 && br_brisaca_ceka > 0)
		{
			pthread_cond_signal(&red_brisaca);
		}
		pthread_mutex_unlock(&m);
		sleep(5 + rand() % 5);
	}
	return nullptr;
}

void* funkcija_pisaca(void* par)
{
	int64_t id = (int64_t)par;
	srand((unsigned)time(NULL));
	while (1)
	{
		int x = rand() % 100;
		pthread_mutex_lock(&m);
		printf("Pisac %ld zeli dodati element %d u listu\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();

		br_pisaca_ceka++;
		while (br_pisaca_pise + br_brisaca_brise + br_brisaca_ceka > 0)
		{
			pthread_cond_wait(&red_pisaca, &m);
		}
		// K.O.
		br_pisaca_pise++;
		br_pisaca_ceka--;
		printf("Pisac %ld zapocinje dodavanje vrijednosti %d na kraj liste\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();
		lista.dodaj(x);
		printf("Pisac %ld dodao vrijednost %d na kraj liste\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();
		br_pisaca_pise--;
		printf("Pisac %ld vise ne koristi listu\n", id);
		ispisi_broj_aktivnih();
		lista.ispis();
		if (br_brisaca_ceka > 0 && br_citaca_cita==0)
		{
			pthread_cond_signal(&red_brisaca);
		}
		else
		{
			if (br_pisaca_ceka > 0)
			{
				pthread_cond_signal(&red_pisaca);
			}
		}
		pthread_mutex_unlock(&m);
		sleep(5);
	}
	return nullptr;
}

void* funkcija_brisaca(void* par)
{
	int64_t id = (int64_t)par;
	srand((unsigned)time(NULL));
	int x = 0;
	while (1)
	{
		pthread_mutex_lock(&m);
		if (lista.br_el > 0)
			x = 1 + (rand() % lista.br_el);
		else x = 0;
		printf("Brisac %ld zeli brisati element %d liste\n", id, x);
		ispisi_broj_aktivnih();
		lista.ispis();
		br_brisaca_ceka++;
		while (br_pisaca_pise + br_citaca_cita + br_brisaca_brise > 0)
		{
			pthread_cond_wait(&red_brisaca, &m);
		}
		br_brisaca_brise++;
		br_brisaca_ceka--;
		int y;
		int rez = lista.citaj(x, y);
		if (rez)
		{
			printf("Brisac %ld zapocinje s brisanjem elementa %d liste (vrijednost = %d)\n", id, x, y);
			ispisi_broj_aktivnih();
			lista.ispis();
			lista.brisi(x);
			printf("Brisac %ld obrisao element liste %d (vrijednost = %d)\n", id, x, y);
			
		}
		else
		{
			printf("Brisac %ld ne moze obrisati nepostojeci element %d liste\n", id, x);
		}
		ispisi_broj_aktivnih();
		lista.ispis();
		br_brisaca_brise--;
		printf("Brisac %ld vise ne koristi listu\n", id);
		ispisi_broj_aktivnih();
		lista.ispis();
		if (br_pisaca_ceka > 0)
		{
			pthread_cond_signal(&red_pisaca);
			if (br_citaca_ceka > 0)
			{
				pthread_cond_broadcast(&red_citaca);
			}
		}
		else if (br_brisaca_ceka > 0 && lista.br_el>0)
		{
			pthread_cond_signal(&red_brisaca);
		}
		else
		{
			if (br_citaca_ceka > 0)
			{
				pthread_cond_broadcast(&red_citaca);
			}
		}
		pthread_mutex_unlock(&m);
		sleep(5 + rand() % 5);
	}
	return nullptr;
}

int main(void)
{
	// inicijalizacija monitora
	pthread_mutex_init(&m, NULL);
	pthread_cond_init(&red_citaca, NULL);
	pthread_cond_init(&red_pisaca, NULL);
	pthread_cond_init(&red_brisaca, NULL);

	for (int64_t i = 0; i != BR_PISACA; i++)
	{
		pthread_create(&pisaci[i], NULL, funkcija_pisaca, (void*)i);
		sleep(1);
	}
	sleep(10);
	for (int64_t i = 0; i != BR_CITACA; i++)
	{
		pthread_create(&citaci[i], NULL, funkcija_citaca, (void*)i);
		sleep(1);
	}
	sleep(10);
	for (int64_t i = 0; i != BR_BRISACA; i++)
	{
		pthread_create(&brisaci[i], NULL, funkcija_brisaca, (void*)i);
		sleep(1);
	}

	for (int64_t i = 0; i != BR_BRISACA; i++)
	{
		pthread_join(brisaci[i], NULL);
	}
	for (int64_t i = 0; i != BR_CITACA; i++)
	{
		pthread_join(citaci[i], NULL);
	}
	for (int64_t i = 0; i != BR_PISACA; i++)
	{
		pthread_join(pisaci[i], NULL);
	}

	return 0;
}
