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
    sigaction(SIGQUIT, &sigAct, NULL);

	sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &mask, NULL);


	fflush(stdout);

	ouvrirFenetreGraphique();

	//intialisation mutex

	pthread_mutex_init(&mutexEvenement, NULL);  
	pthread_mutex_init(&mutexGrilleJeu, NULL);  
	pthread_mutex_init(&mutexDK, NULL);
	pthread_mutex_init(&mutexScore, NULL);  

	//Thread Evenement
	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);

	//Thread DkJr
	pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);

	//Thread cle
	pthread_create(&threadCle, NULL, FctThreadCle, NULL);

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
		pthread_mutex_lock(&mutexGrilleJeu);

		if(pos==1)
			grilleJeu[0][1].type=CLE;
		else
			grilleJeu[0][1].type=VIDE;

		afficherCle(pos);

		pthread_mutex_unlock(&mutexGrilleJeu);
			
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
		evt = lireEvenement();

		if(evt == SDL_QUIT)
			exit(0);

		pthread_mutex_lock(&mutexEvenement);
		evenement = evt;
		pthread_mutex_unlock(&mutexEvenement);

		pthread_kill(threadDKJr, SIGQUIT);
		nanosleep(&temps, NULL);

		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);
	}
}

void * FctThreadDKJr(void* arg)
{
	sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

	bool on = true;

	pthread_mutex_lock(&mutexGrilleJeu);
	setGrilleJeu(3, 1, DKJR); 
	afficherGrilleJeu();
	afficherDKJr(11, 9, 1); 
	etatDKJr = LIBRE_BAS; 
	positionDKJr = 1;	
	pthread_mutex_unlock(&mutexGrilleJeu);

	while (on)
	{
		pause();
		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);

		switch (etatDKJr)
		{
			case LIBRE_BAS:
				switch (evenement)
				{
					case SDLK_LEFT:
						if (positionDKJr > 1)
						{
							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr--;
							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();
							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						}
						break;
					case SDLK_RIGHT:
						setGrilleJeu(3, positionDKJr);//eneleve dk
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
						positionDKJr++;
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherGrilleJeu();
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					break;
					case SDLK_UP:
					
					break;
				}	
			case LIANE_BAS:
			
			break;
			case DOUBLE_LIANE_BAS:
			
			break;
			case LIBRE_HAUT:
			
			break;
			case LIANE_HAUT:

			break;

		}
		pthread_mutex_unlock(&mutexGrilleJeu);
		pthread_mutex_unlock(&mutexEvenement);
	}
	pthread_exit(0);
}


void HandlerSIGQUIT(int sig)
{
	printf("Evenement\n");
}