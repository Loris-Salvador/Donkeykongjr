#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE        		0
#define DKJR       			1
#define CROCO       		2
#define CORBEAU     		3
#define CLE 				4

#define AUCUN_EVENEMENT    	0

#define LIBRE_BAS			1
#define LIANE_BAS			2
#define DOUBLE_LIANE_BAS	3
#define LIBRE_HAUT			4
#define LIANE_HAUT			5

#define NB_VIES				3
#define VIE_SUP				50
#define ALARM				15
#define DELAI_MIN           2500

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
void respawn();

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
int  score;
bool MAJScore = false;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;
int nbEchecs;


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


int main(int argc, char* argv[])
{
	sigset_t mask;
    sigemptyset(&mask);

	struct sigaction sigAct;

//----------------------------------------

    sigAct.sa_handler = HandlerSIGQUIT;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGQUIT, &sigAct, NULL);

	sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGALRM;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGALRM, &sigAct, NULL);

	sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGUSR1;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGUSR1, &sigAct, NULL);

	sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGINT;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGINT, &sigAct, NULL);

	sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGUSR2;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGUSR2, &sigAct, NULL);

	sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGHUP;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGHUP, &sigAct, NULL);

	sigaddset(&mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	sigAct.sa_handler = HandlerSIGCHLD;
    sigemptyset(&sigAct.sa_mask);
	sigAct.sa_flags=0;
    sigaction(SIGCHLD, &sigAct, NULL);

	sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

//----------------------------------------

	
	ouvrirFenetreGraphique();


	//intialisation mutexs

	pthread_mutex_init(&mutexEvenement, NULL);  
	pthread_mutex_init(&mutexGrilleJeu, NULL);  
	pthread_mutex_init(&mutexDK, NULL);
	pthread_mutex_init(&mutexScore, NULL);  	
		
	//Initialisation condition

	pthread_cond_init(&condDK, NULL);
	pthread_cond_init(&condScore, NULL);


	//Initialisation Cle_Specifique

	pthread_key_create(&keySpec, DestructeurVS);

	//Thread cle
	printf("Initialisation Thread Cle\n");
	pthread_create(&threadCle, NULL, FctThreadCle, NULL);

	//Thread DkJr
	printf("Initialisation Thread DkJr\n");
	pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);

	//Thread Evenement
	printf("Initialisation Thread Evenement\n");
	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);

	//ThreadDK
	printf("Initialisation thread DK\n");
	pthread_create(&threadDK, NULL, FctThreadDK, NULL);

	//ThreadScore
	printf("Initialisation thread Score\n");
	pthread_create(&threadScore, NULL, FctThreadScore, NULL);

	//ThreadEnnemis
	printf("Initialisation thread Ennemis\n");
	pthread_create(&threadEnnemis, NULL, FctThreadEnnemis, NULL);

//-------------------------------------

	pthread_join(threadDKJr, NULL);
	nbEchecs++;
	afficherEchec(nbEchecs);


	while(nbEchecs < NB_VIES)
	{
		pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);
		pthread_join(threadDKJr, NULL);
		nbEchecs++;
		afficherEchec(nbEchecs);
	}


	pthread_exit(0);

}

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

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


		pthread_mutex_lock(&mutexGrilleJeu);

		if(pos<3)//2 carres à supprimer
			effacerCarres(3, 12 + (pos-1), 2);	
		else//4 carres à supprimer
			effacerCarres(3, 13, 2, 2);
			
		pthread_mutex_unlock(&mutexGrilleJeu);


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
	struct timespec temps;
	sigset_t mask;

    sigemptyset(&mask);

    sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGCHLD);

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
		temps = { 1, 400000000 };

		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);

		switch (etatDKJr)
		{
			case LIBRE_BAS:
				switch (evenement)
				{
					case SDLK_LEFT:

						if(positionDKJr > 1)
						{
							setGrilleJeu(3, positionDKJr, VIDE);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							if(grilleJeu[3][positionDKJr-1].type == CROCO)
							{
								pthread_kill(grilleJeu[3][positionDKJr-1].tid, SIGUSR2);
								on = false;
								break;
							}

							positionDKJr--;

							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						}
						break;

					case SDLK_RIGHT:

						if(positionDKJr < 7)
						{
							setGrilleJeu(3, positionDKJr, VIDE);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

							if(grilleJeu[3][positionDKJr+1].type == CROCO)
							{
								pthread_kill(grilleJeu[3][positionDKJr+1].tid, SIGUSR2);
								on = false;
								break;
							}

							positionDKJr++;

							setGrilleJeu(3, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
						}
						break;

					case SDLK_UP:

						setGrilleJeu(3, positionDKJr, VIDE);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

						if(grilleJeu[2][positionDKJr].type == CORBEAU)
						{
							pthread_kill(grilleJeu[2][positionDKJr].tid, SIGUSR1);
							on = false;
							break;
						}
				

						if(positionDKJr == 5 || positionDKJr == 1)
						{
							etatDKJr=LIANE_BAS;

							setGrilleJeu(2, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(10, (positionDKJr * 2) + 7, 7);

							break;
						}

						if(positionDKJr == 7)
						{
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


						setGrilleJeu(2, positionDKJr, VIDE);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);


						//Descente apres saut
						if(grilleJeu[3][positionDKJr].type == CROCO)
						{
							pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
							on = false;
							break;
						}


						setGrilleJeu(3, positionDKJr, DKJR);
						afficherGrilleJeu();

						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					
					break;	

				}
			break;

			case LIANE_BAS:

				if(evenement == SDLK_DOWN)
				{
					setGrilleJeu(2, positionDKJr,VIDE);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					if(grilleJeu[3][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
						on = false;
						break;
					}

					setGrilleJeu(3, positionDKJr, DKJR);
					afficherGrilleJeu();

					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					etatDKJr=LIBRE_BAS;
				}

			break;

			case DOUBLE_LIANE_BAS:

				if(evenement==SDLK_DOWN)
				{
					setGrilleJeu(2, positionDKJr, VIDE);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					if(grilleJeu[3][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[3][positionDKJr].tid, SIGUSR2);
						on = false;
						break;
					}

					setGrilleJeu(3, positionDKJr, 1);
					afficherGrilleJeu();

					afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

					etatDKJr=LIBRE_BAS;
				}

				else if(evenement==SDLK_UP)
				{
					setGrilleJeu(2, positionDKJr, VIDE);
					effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

					if(grilleJeu[1][positionDKJr].type == CROCO)
					{

						pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);
						on = false;
						break;
					}

					setGrilleJeu(1, positionDKJr, 1);
					afficherGrilleJeu();

					afficherDKJr(7, (positionDKJr * 2) + 7, 6);

					etatDKJr=LIBRE_HAUT;
				}
			
			break;

			case LIBRE_HAUT:

				switch (evenement)
				{
					case SDLK_LEFT:

						if(positionDKJr > 3)
						{
							setGrilleJeu(1, positionDKJr, VIDE);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							if(grilleJeu[1][positionDKJr-1].type == CROCO)
							{
								pthread_kill(grilleJeu[1][positionDKJr-1].tid, SIGUSR2);
								on = false;
								break;
							}

							positionDKJr--;

							setGrilleJeu(1, positionDKJr, DKJR);
							afficherGrilleJeu();

							afficherDKJr(7, (positionDKJr * 2) + 7, 7-positionDKJr);
						}
						else
						{
							setGrilleJeu(1, positionDKJr, VIDE);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							positionDKJr--;

							temps = { 0, 500000000 };

							if(grilleJeu[0][1].type == 4)
							{
								afficherDKJr(6, 11, 9);
								nanosleep(&temps, NULL);
								effacerCarres(5, 12, 3, 2);

								afficherDKJr(6, 11, 10);
								nanosleep(&temps, NULL);


								//effacer sans toucher au cadenas
								effacerPoints(11*40,3*40, 2*40,67);
								effacerPoints(11.7 * 40, 120+67, 50, 40);
								//--------------------------------

								afficherDKJr(0, 0, 11);

								pthread_mutex_lock(&mutexDK);
								MAJDK=true;
								pthread_mutex_unlock(&mutexDK);
								pthread_cond_signal(&condDK);

								pthread_mutex_lock(&mutexScore);
								score=score+10;
								MAJScore=true;
								pthread_mutex_unlock(&mutexScore);
								pthread_cond_signal(&condScore);


								nanosleep(&temps, NULL);
								effacerCarres(6, 10, 2, 3);

								respawn();

								setGrilleJeu(3, 1, DKJR); 
								afficherGrilleJeu();
								afficherDKJr(11, 9, 1); 
								etatDKJr = LIBRE_BAS; 
								positionDKJr = 1;	
							}
							else
							{
								afficherDKJr(6, 11, 9);

								
								//pas de lock/unlock sinn visuellement illogique
								nanosleep(&temps, NULL);
								

								setGrilleJeu(0, positionDKJr, VIDE);
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
								
								effacerCarres(11, 7, 2, 2);

								on = false;
							}
						}

					break;

					case SDLK_RIGHT:

					if(positionDKJr<7)
					{
						setGrilleJeu(1, positionDKJr, VIDE);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						if(grilleJeu[1][positionDKJr+1].type == CROCO)
						{
							pthread_kill(grilleJeu[1][positionDKJr+1].tid, SIGUSR2);
							on = false;
							break;
						}

						positionDKJr++;

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherGrilleJeu();

						if(positionDKJr<7)
							afficherDKJr(7, (positionDKJr * 2) + 7, 7-positionDKJr);
						else if(positionDKJr==7)
							afficherDKJr(7, (positionDKJr * 2) + 7, 6);
					}
					break;

					case SDLK_UP:

						if(positionDKJr == 7 || positionDKJr == 5)
							break;


						setGrilleJeu(1, positionDKJr, VIDE);
						effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

						if(positionDKJr == 6)
						{
							setGrilleJeu(0, positionDKJr, DKJR);
							afficherGrilleJeu();
							etatDKJr=LIANE_HAUT;

							afficherDKJr(6, (positionDKJr * 2) + 7, 7);
							break;
						}



						setGrilleJeu(0, positionDKJr, DKJR);
						afficherGrilleJeu();

						afficherDKJr(6, (positionDKJr * 2) + 7, 8);

						pthread_mutex_unlock(&mutexGrilleJeu);
						nanosleep(&temps, NULL);
						pthread_mutex_lock(&mutexGrilleJeu);

						
						setGrilleJeu(0, positionDKJr, VIDE);
						effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

						if(grilleJeu[1][positionDKJr].type == CROCO)
						{
							pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);
							on = false;
							break;
						}


						setGrilleJeu(1, positionDKJr, DKJR);
						afficherGrilleJeu();

						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
					break;	

					case SDLK_DOWN:
						if(positionDKJr==7)
						{
							setGrilleJeu(1, positionDKJr, VIDE);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							if(grilleJeu[2][positionDKJr].type == CORBEAU)
							{
								pthread_kill(grilleJeu[2][positionDKJr].tid, SIGUSR1);
								on = false;
								break;
							}

							setGrilleJeu(2, positionDKJr, 1);
							afficherGrilleJeu();
							etatDKJr=DOUBLE_LIANE_BAS;

							afficherDKJr(10, (positionDKJr * 2) + 7, 5);
						}						
					break;	
				}
			
			break;

			case LIANE_HAUT:

				if(evenement == SDLK_DOWN)
				{
					setGrilleJeu(0, positionDKJr,VIDE);
					effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

					if(grilleJeu[1][positionDKJr].type == CROCO)
					{
						pthread_kill(grilleJeu[1][positionDKJr].tid, SIGUSR2);
						on = false;
						break;
					}
					setGrilleJeu(1, positionDKJr, 1);
					afficherGrilleJeu();

					etatDKJr=LIBRE_HAUT;
					afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
				}

			break;
		}

		pthread_mutex_unlock(&mutexGrilleJeu);
		pthread_mutex_unlock(&mutexEvenement);

	}

	pthread_mutex_lock(&mutexGrilleJeu);
	respawn();
	pthread_mutex_unlock(&mutexGrilleJeu);
	
	pthread_exit(0);
}

void* FctThreadDK(void * param)
{
    struct timespec temps = {0, 700000000};
    int cage = 1;

	for(int i=1; i<=4;i++)
		afficherCage(i);

    while(1)
    {
		pthread_mutex_lock(&mutexDK);
        while(MAJDK==false)
        {
            pthread_cond_wait(&condDK, &mutexDK);
        }
        MAJDK=false;
        pthread_mutex_unlock(&mutexDK);

		switch(cage)
		{
			case 1:
				effacerCarres(2,7,2,2);
				cage++;
				break;
			
			case 2:
				effacerCarres(2,9,2,2); 
				cage++;
				break;

			case 3:
				effacerCarres(4,7,2,2); 
				cage++;
				break;

			case 4:
				effacerCarres(4,9,2,3);
				afficherRireDK();
				nanosleep(&temps, NULL);
				effacerCarres(3,8,2,2);
				for(int i=1; i<=4;i++)
					afficherCage(i);
				cage = 1;
				pthread_mutex_lock(&mutexScore);
				score = score + 10;
				MAJScore=true;
				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
				break;
		}   


    }
}

void * FctThreadScore(void * param)
{
	int tmp;

	afficherScore(score);

	while(1)
	{
		tmp = score;
		pthread_mutex_lock(&mutexScore);
        while(MAJScore == false)
        {
            pthread_cond_wait(&condScore, &mutexScore);
        }
		if(score>tmp)
		{
			afficherScore(score);
			if((score % VIE_SUP == 0) &&  (nbEchecs > 0))
			{
				nbEchecs--;
				effacerCarres(7, nbEchecs+27, 1, 1);
			}
		}

		MAJScore=false;	
        pthread_mutex_unlock(&mutexScore);
	}
}

void * FctThreadEnnemis(void * param)
{
	int ennemi, result, sec, nanosec;
	pthread_t threadCorbeau, threadCroco;
	struct timespec temps;

	sigset_t mask;

	sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

	alarm(ALARM);

	while(1)
	{
		sec = delaiEnnemis /1000;
		nanosec = (delaiEnnemis % sec) * 1000000;
		temps = { sec, nanosec};

		srand(time(0));
		ennemi = (rand()% 2) + 0;

		if(ennemi ==  0)
			pthread_create(&threadCorbeau, NULL, FctThreadCorbeau, NULL);
		else
			pthread_create(&threadCroco, NULL, FctThreadCroco, NULL);
		
		do 
		{
			result = nanosleep(&temps, &temps);
		} 
		while (result == -1);
	}
}
void * FctThreadCorbeau(void * param)
{
	struct timespec temps = { 0, 700000000 };

	sigset_t mask;

	sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);


	int* position = (int*) malloc(sizeof(int));
	if (position == NULL)
	{
		printf("Allocation CORBEAU a échoué");
		exit(1);
	}

	pthread_setspecific(keySpec, (const void*)position);

	*position=0;

	pthread_mutex_lock(&mutexGrilleJeu);

	while((*position) < 8)
	{
		if(grilleJeu[2][(*position)].type == DKJR)
		{
			grilleJeu[2][(*position)].type = 0;
			pthread_kill(threadDKJr, SIGINT);
			pthread_mutex_unlock(&mutexGrilleJeu);
			pthread_exit(0);
		}
	

		setGrilleJeu(2, *position, CORBEAU, pthread_self());
		afficherGrilleJeu();
		afficherCorbeau((*position) * 2 + 8, (*position)%2+1);

		pthread_mutex_unlock(&mutexGrilleJeu);

		nanosleep(&temps, NULL);

		pthread_mutex_lock(&mutexGrilleJeu);
		setGrilleJeu(2, *position);
		effacerCarres(9, (*position) * 2 + 8,2,1);
		(*position)++;
	}

	pthread_mutex_unlock(&mutexGrilleJeu);


	pthread_exit(0);

}

void * FctThreadCroco(void * param)
{
	struct timespec temps = { 0, 700000000 };

	sigset_t mask;

	sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);


	S_CROCO* croco = (S_CROCO*) malloc(sizeof(S_CROCO));
	if (croco == NULL)
	{
		printf("Allocation CROCO a échoué");
		exit(1);
	}

	pthread_setspecific(keySpec, (const void*)croco);

	(*croco).position = 2;
	(*croco).haut = true;

	pthread_mutex_lock(&mutexGrilleJeu);


	while(croco->position < 8)
	{
		if(grilleJeu[1][croco->position].type == DKJR)
		{
			grilleJeu[1][croco->position].type = 0;
			pthread_kill(threadDKJr, SIGHUP);
			pthread_mutex_unlock(&mutexGrilleJeu);
			pthread_exit(0);
		}
	
		setGrilleJeu(1, croco->position, CROCO, pthread_self());
		afficherGrilleJeu();
		afficherCroco(croco->position * 2 + 7, (croco->position-1)%2+1);

		pthread_mutex_unlock(&mutexGrilleJeu);
		nanosleep(&temps, NULL);

		pthread_mutex_lock(&mutexGrilleJeu);

		setGrilleJeu(1, croco->position);
		effacerCarres(8, croco->position * 2 + 7,1,1);

		(*croco).position++;
	}



	afficherCroco(0, 3);
	pthread_mutex_unlock(&mutexGrilleJeu);

	nanosleep(&temps, NULL);
	effacerCarres(9, 23, 1,1);

	(*croco).haut = false;
	(*croco).position--;

	pthread_mutex_lock(&mutexGrilleJeu);


	while(croco->position > 0)
	{

		if(grilleJeu[3][croco->position].type == DKJR)
		{
			grilleJeu[3][croco->position].type = 0;
			pthread_kill(threadDKJr, SIGCHLD);
			pthread_mutex_unlock(&mutexGrilleJeu);
			pthread_exit(0);
		}
	
		setGrilleJeu(3, croco->position, CROCO, pthread_self());
		afficherGrilleJeu();

		if(croco->position % 2 == 1)
			afficherCroco(croco->position * 2 + 8, 5);
		else
			afficherCroco(croco->position * 2 + 8, 4);

		pthread_mutex_unlock(&mutexGrilleJeu);

		nanosleep(&temps, NULL);

		pthread_mutex_lock(&mutexGrilleJeu);
		setGrilleJeu(3, croco->position);
		effacerCarres(12, croco->position * 2 + 8,1,1);

		(*croco).position--;
	}

	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);
}

void respawn()//attention faut avoir lock mutex grille jeu
{
	int i;
	struct timespec temps = {0, 100000000};

	for(i = 1 ; i<4; i++)
	{
		if(grilleJeu[3][i].type== CROCO)
		{
			pthread_kill(grilleJeu[3][i].tid, SIGUSR2);
		}

	}

	for(i= 0 ; i<3 ; i++)
	{
		if(grilleJeu[2][i].type == CORBEAU)
		{
			pthread_kill(grilleJeu[2][i].tid, SIGUSR1);
		}		
	}
	
	nanosleep(&temps, NULL);
}

void HandlerSIGQUIT(int sig)
{
	//printf("\nEvenement !\n");
}

void HandlerSIGALRM(int sig)
{
	printf("SIGALRM\n");
	delaiEnnemis = delaiEnnemis - 250;

	if(delaiEnnemis > DELAI_MIN)
		alarm(ALARM);
}

void HandlerSIGUSR1(int sig)
{
	int *var = (int*)pthread_getspecific(keySpec);

	printf("SIGUSR1\n");

	pthread_mutex_lock(&mutexGrilleJeu);

	setGrilleJeu(2, *var);
	effacerCarres(9, (*var) * 2 + 8,2,1);

	pthread_mutex_unlock(&mutexGrilleJeu);
	

	pthread_exit(0);

}
void HandlerSIGUSR2(int sig)
{
	S_CROCO *croco = (S_CROCO*)pthread_getspecific(keySpec);

	printf("SIGUSR2\n");

	pthread_mutex_lock(&mutexGrilleJeu);

	if((*croco).haut == true)
	{
		setGrilleJeu(1, croco->position, VIDE);
		effacerCarres(8, croco->position * 2 + 7,1,1);
	}
	else
	{
		setGrilleJeu(3, croco->position, VIDE);
		effacerCarres(12, croco->position * 2 + 8, 1, 1);
	}

	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);

}
void HandlerSIGINT(int sig)
{
	printf("SIGINT\n");

	if(etatDKJr == LIBRE_BAS)
		pthread_mutex_unlock(&mutexEvenement);

	effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_lock(&mutexGrilleJeu);
	respawn();
	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);

}

void HandlerSIGHUP(int sig)
{
	printf("SIGHUP\n");

	effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_lock(&mutexGrilleJeu);
	respawn();
	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);
}

void HandlerSIGCHLD(int sig)
{
	printf("SIGCHLD\n");

	effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

	pthread_mutex_lock(&mutexGrilleJeu);
	respawn();
	pthread_mutex_unlock(&mutexGrilleJeu);

	pthread_exit(0);

}

void DestructeurVS(void* p)
{
	printf("Destructeur variable specifique\n");

	fflush(stdout);
	free(p);

}
