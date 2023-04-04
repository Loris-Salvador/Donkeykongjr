#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE        		0
#define DKJR       		1
#define CROCO       		2
#define CORBEAU     		3
#define CLE 			4

#define AUCUN_EVENEMENT    	0

#define LIBRE_BAS		1
#define LIANE_BAS		2
#define DOUBLE_LIANE_BAS	3
#define LIBRE_HAUT		4
#define LIANE_HAUT		5

void* FctThreadEvenements(void *);
void* FctThreadCle(void *);
void* FctThreadDK(void *);
void* FctThreadDKJr(void *);
void* FctThreadScore(void *);
void* FctThreadEnnemis(void *);
void* FctThreadCorbeau(void *);
void* FctThreadCroco(void *);

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

void DestructeurVS(void *p);

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;

pthread_key_t keySpec;

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
  bool haut;
  int position;
} S_CROCO;

// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	sigset_t mask;
    sigemptyset(&mask);

	struct sigaction sigAct;

    sigAct.sa_handler = HandlerSIGQUIT;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGQUIT, &sigAct, NULL);

	sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

	ouvrirFenetreGraphique();

	//intialisation mutex

	pthread_mutex_init(&mutexEvenement, NULL);  
	pthread_mutex_init(&mutexGrilleJeu, NULL);  
	pthread_mutex_init(&mutexDK, NULL);
	pthread_mutex_init(&mutexScore, NULL);  	
		

	//Thread cle

	printf("Initialisation Thread Cle\n");
	pthread_create(&threadCle, NULL, FctThreadCle, NULL);

	//Thread DkJr
	printf("Initialisation Thread DkJr\n");

	pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);

	//Thread Evenement
	printf("Initialisation Thread Evenement\n");

	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);


	pthread_join(threadDKJr, NULL);


	pthread_exit(0);

}

// -------------------------------------

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{   
   for(int j = 0; j < 4; j++)
   {
       for(int k = 0; k < 8; k++)
          printf("%d  ", grilleJeu[j][k].type);
       printf("\n");
   }

   printf("\n");   
}
void *FctThreadCle(void* arg)
{
	struct timespec temps = { 0, 700000000 };

	int pos = 1, sens = -1;

	while(1)
	{	
		//printf("Avant Lock cle\n");
		pthread_mutex_lock(&mutexGrilleJeu);
		//printf("Lock cle\n");

		if(pos==1)
			grilleJeu[0][1].type=CLE;
		else
			grilleJeu[0][1].type=VIDE;

		afficherCle(pos);

		pthread_mutex_unlock(&mutexGrilleJeu);

		//printf("APRES Lock cle\n");
			
		nanosleep(&temps, NULL);

		if(pos<3)//2 carres à supprimer
			effacerCarres(3, 12 + (pos-1), 2);	

		else//4 carres à supprimer
			effacerCarres(3, 13, 2, 2);

		if(pos==1 || pos ==4)
			sens = sens * -1;

		pos = pos + sens;
	}
}

void *FctThreadEvenements(void* arg)
{
	int evt;
	struct timespec temps = { 0, 100000000 };

	while (1)
	{
		printf("Lecture evenement\n");
		evt = lireEvenement();

		if(evt == SDL_QUIT)
			exit(0);

		printf("Avant Lock Evenement\n");
		pthread_mutex_lock(&mutexEvenement);
		evenement = evt;
		pthread_mutex_unlock(&mutexEvenement);
		printf("Apres Lock Evenement\n");

		printf("Envoie signal a dkjr\n");
		pthread_kill(threadDKJr, SIGQUIT);
		printf("wesh\n");
		nanosleep(&temps, NULL);
		printf("Avant Lock Evenement 2\n");
		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);
		printf("APRES Lock Evenement 2\n");
	}
}

void * FctThreadDKJr(void* arg)
{
	sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

	struct timespec temps = { 1, 400000000 };

	bool on = true;

	printf("Avant Lock DKJR\n");

	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(3, 1, DKJR); 
	afficherGrilleJeu();
	afficherDKJr(11, 9, 1); 
	etatDKJr = LIBRE_BAS; 
	positionDKJr = 1;	
	pthread_mutex_unlock(&mutexGrilleJeu);
	printf("Apres Lock Evenement\n");

	while (on)
	{
		printf("Attente...\n");
		pause();
		printf("Sortie de pause()\n");
		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);
		printf("EtatDK = %d\n", etatDKJr);

		switch (etatDKJr)
		{
			case LIBRE_BAS:
				switch (evenement)
				{
					case SDLK_LEFT:
						if (positionDKJr > 1)
						{
							setGrilleJeu(3, positionDKJr, 0);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							positionDKJr--;

							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						}
						break;
					case SDLK_RIGHT:
						if(positionDKJr < 7)
						{
							setGrilleJeu(3, positionDKJr, 0);//enleve dk
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							positionDKJr++;

							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						}

					break;
					case SDLK_UP:
						setGrilleJeu(3, positionDKJr, 0);//enleve dk
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						if(positionDKJr == 5 || positionDKJr == 1)
						{
							printf("IF\n");
							etatDKJr=LIANE_BAS;
							setGrilleJeu(2, positionDKJr, DKJR);
							afficherGrilleJeu();
							afficherDKJr(10, (positionDKJr * 2) + 7, 7);
							break;
						}
						else if(positionDKJr == 7)
						{
							printf("ELSE IF\n");
							etatDKJr=DOUBLE_LIANE_BAS;
							setGrilleJeu(2, positionDKJr, DKJR);
							afficherGrilleJeu();
							afficherDKJr(10, (positionDKJr * 2) + 7, 5);
							break;
						}
							

						setGrilleJeu(2, positionDKJr, DKJR);
						afficherGrilleJeu();
						afficherDKJr(10, (positionDKJr * 2) + 7, 8);

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						
						setGrilleJeu(2, positionDKJr);//enleve dk
						setGrilleJeu(3, positionDKJr, 1);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					
						break;	
				}
			break;
			case LIANE_BAS:
				printf("LIANE_BAS\n");
				if(evenement==SDLK_DOWN)
				{
					setGrilleJeu(2, positionDKJr,0);//enleve dk
					setGrilleJeu(3, positionDKJr, 1);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					etatDKJr=LIBRE_BAS;
				}
			break;
			case DOUBLE_LIANE_BAS:
				printf("DOUBLE LIANE\n");
				if(evenement==SDLK_DOWN)
				{
					setGrilleJeu(2, positionDKJr);//enleve dk
					setGrilleJeu(3, positionDKJr, 1);
					afficherGrilleJeu();
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					etatDKJr=LIBRE_BAS;
				}
				else if(evenement==SDLK_UP)
				{
					setGrilleJeu(2, positionDKJr);//enleve dk
					setGrilleJeu(1, positionDKJr, 1);
					afficherGrilleJeu();
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);
					afficherDKJr(7, (positionDKJr * 2) + 7, 6);
					etatDKJr=LIBRE_HAUT;
				}
			
			break;
			case LIBRE_HAUT:
				switch (evenement)
				{
					case SDLK_LEFT:
					if(positionDKJr>3)
					{
						setGrilleJeu(1, positionDKJr, 0);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						positionDKJr--;

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherGrilleJeu();

						afficherDKJr(7, (positionDKJr * 2) + 7, 7-positionDKJr);
					}
					else
					{
						setGrilleJeu(1, positionDKJr, 0);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						positionDKJr--;

						setGrilleJeu(0, positionDKJr, DKJR);
						afficherGrilleJeu();

						afficherDKJr(6, 11, 9);

						temps = { 0, 500000000 };
						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						setGrilleJeu(0, positionDKJr, 0);
						effacerCarres(5, 12, 3, 2);

						afficherDKJr(0, 0, 12);

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						effacerCarres(6, 11, 2, 2);
						afficherDKJr(0, 0, 13);

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);
						
						positionDKJr=1;
						effacerCarres(11, 7, 2, 2);


						setGrilleJeu(3, 1, DKJR); 
						afficherGrilleJeu();
						afficherDKJr(11, 9, 1); 

						etatDKJr=LIBRE_BAS;

					}


					break;
					case SDLK_RIGHT:

					if(positionDKJr<7)
					{
						setGrilleJeu(1, positionDKJr, 0);//enleve dk
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						positionDKJr++;

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherGrilleJeu();
						if(positionDKJr!=7)
							afficherDKJr(7, (positionDKJr * 2) + 7, 7-positionDKJr);
						else
							afficherDKJr(7, (positionDKJr * 2) + 7, 8);
					}
					break;
					case SDLK_UP:

						if(positionDKJr == 7 || positionDKJr == 5)
							break;


						setGrilleJeu(1, positionDKJr, 0);//enleve dk
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						if(positionDKJr == 6)
						{
							printf("IF\n");
							etatDKJr=LIANE_HAUT;
							setGrilleJeu(0, positionDKJr, DKJR);
							afficherGrilleJeu();
							afficherDKJr(6, (positionDKJr * 2) + 7, 7);
							break;
						}



						setGrilleJeu(0, positionDKJr, DKJR);
						afficherGrilleJeu();
						afficherDKJr(6, (positionDKJr * 2) + 7, 8);

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						
						setGrilleJeu(0, positionDKJr);//enleve dk
						setGrilleJeu(3, positionDKJr, 1);
						effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					break;	
					case SDLK_DOWN:
					if(positionDKJr==7)
					{
						etatDKJr=DOUBLE_LIANE_BAS;
						setGrilleJeu(1, positionDKJr);//enleve dk
						setGrilleJeu(2, positionDKJr, 1);
						afficherGrilleJeu();
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
						afficherDKJr(10, (positionDKJr * 2) + 7, 5);
					}						
					break;	
				}
			
			break;
			case LIANE_HAUT:
				printf("LIANE_BAS\n");
				if(evenement==SDLK_DOWN)
				{
					setGrilleJeu(0, positionDKJr,0);//enleve dk
					setGrilleJeu(1, positionDKJr, 1);
					effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);
					afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					etatDKJr=LIBRE_HAUT;
				}
			break;

		}
		pthread_mutex_unlock(&mutexGrilleJeu);
		pthread_mutex_unlock(&mutexEvenement);
	}
	pthread_exit(0);
}


void HandlerSIGQUIT(int sig)
{
	//sprintf("\nEvenement\n");
}