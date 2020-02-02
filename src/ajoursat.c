/************************************************************************
    Copyright (C) 1986-2000 by

    F6FBB - Jean-Paul ROUBELAT
    jpr@f6fbb.org

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of code have been taken from many other softwares.
    Thanks for the help.
************************************************************************
 * HISTORIQUE :
 * DATE			AUTEUR / AUTHOR : Bernard Pidoux, f6bvp@amsat.org
 * ajoursat 16/10/89	Bernard Pidoux F6BVP
 * 11/11/89		le répertoire n'est pas imposé * 20/05/00
 *			portage sous linux (J-P ROUBELAT - F6FBB)
 *
 * Version 1.72 12/12/90  avec vérifications renforcés du synchronisme
 * du format de lecture, sans bouclage infini.
 * 1.74	 3/03/91  Traite jusqu'à 768 Satellites
 * 26/04/91	 Test si place disponible en mémoire
 * 1.75 27/04/91  Option lecture format Nasa
 * nouveau renforcement controle intégrité des donnés
 * Exclusion des doublons
 * libéation mémoire alloué en sortie
 * extension fichier source .txt uniquement par défaut
 * 1.76 19/05/91  CRC en lecture format Nasa	
 * changement du format de lecture lignes NASA
 * 24/05/91	 déplacement message "Aucune mise  jour aprés else
 * 1.77 28/05/91  CRC en lecture format AMSAT
 * 1.78 31/10/93 fscanf remplacé par fgets dans AMSAT pour supprimer
 * plantage programme sur longue ligne sans RC
 * 1.80 20/11/93  prise en compte de la date_limite des donnés
 * 1.83 20/11/94  champ ELEMENT SET pour compatibilité
 * format AMSAT du service REQKEP
 * 1.84 :  15/02/98 suite plantage par ligne pseudo Satellite
 * agrandissement tableaux buf1 et buf2 
 * 1.85 : 12/01/2000 pour le bogue !
 *        dernières mises au point 9 juin 2000 sous Linux
 * 1.86 : septembre 2007
 * 1.87 : 26/09/2009
 * 1.88 Jan-26-2015 changed fgets() lines avoiding compiler warnings 
 * 1.89 Feb-03-2015 changed write() line avoiding compiler warning 
 *************************************************************************/

#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* #define DEBUG */
#define FRANCAIS
#define VERSION "1.89"
/* Tabulation a 4 espaces !! */

#ifdef DEBUG
#define d_printf printf
#else
void d_printf(const char * format, ...);

void d_printf(const char * format, ...) 
{
}
#endif

#define TRUE 1
#define FALSE 0
/*
 * Record du fichier SATEL 
 */

#define __a2__ __attribute__ ((packed, aligned(2)))

typedef struct typ_satel {
	char dd[18];		/* Nom du satellite	*/
	short y3;		/* Annee de reference	*/
	double d3 __a2__;	/* Jour de reference	*/
	short n3;		/* Mois de reference	*/
	short h3;		/* Heure de reference	*/
	short m3;		/* Minute de reference	*/
	short s3;		/* Seconde de reference */
	double i0 __a2__;	/* Inclinaison	*/
	double o0 __a2__;	/* R.A.A.N.		*/
	double e0 __a2__;	/* Excentricite */
	double w0 __a2__;	/* Argument de perigee	*/
	double m0 __a2__;	/* Anomalie moyenne	*/
	double a0 __a2__;	/* Contient 0.0 */
	double n0 __a2__;	/* Mouvement moyen	*/
	double q3 __a2__;	/* Derivee du mouv. moy.*/
	long k0 __a2__;		/* Orbite de reference	*/
	double f1 __a2__;	/* Frequence pour le calcul doppler */
	double v1 __a2__;	/* Contient	 0.0	*/
	short pas;		/* Pas des calculs*/
	long maj __a2__;	/* Date de derniere mise à jour*/
	long cat __a2__;	/* Catalog Number - anciennement vide	*/
	short set;		/* Element set NASA */
	short libre[3];
} satel;

#define MAXSAT 769
#define LINE 81

/********** Prototypes **************/

int Misajour(satel ** vieux, int vv, satel ** nouv, int nn, char option);
int Ecrit_Sat(char *filename, satel ** stru, int nombre);
void Amsat(char *filename);
void nasa(char *filename);
int Lire_Ancien(char *filename);
void Print_Sat(satel ** stru, int m);	/* Imprime fichier */
void Tri_Cat(satel ** stru, int nombre);
void Tri(satel ** stru, int nombre);	/* tri a bulle des noms de satellites */
void CopyStru(satel ** stru, int indice, satel * psat);
int doublons(int nombre);
void avorte(void);
int fraicheur(void);
/************************************/


char *filename, *str;		/* Nom fichier Source et copie */
struct tm *cejour;		/* date et time declares dans TIME.H */
char *aujour;			/* Date de derniere mise à jour */
int date_limite;		/* nombre de jours ancienneté limite pour les données */
satel *vieux[MAXSAT];
satel *nouveaux[MAXSAT];
satel *tampon[MAXSAT];		/* tampon pour mise à jour */
satel sat;			/* declaration structure sat de type satel */
int nsat;			/* nombre de satellites modifié */
int maxsat;			/* nombre de satellites gérable */
char *opt;
char *options[5];
char *option;
char *dmo;
char *dmold;
int good_crc;
int bad_crc;

void sortie(int n)
{
	int i;

	for (i = 0; i < maxsat; i++) 
	{
		free(vieux[i]);
		free(nouveaux[i]);
		/*
		 * farfree(tampon[i]); 
		 */
		free(tampon[i]);
	}
	free(filename);
	free(str);
	free(dmo);
	free(dmold);
	exit(n);
}

void avorte(void)
/*
 * Nom:			  avorte() Description :  Sortie du programme en cas
 * d'erreur. Commentaire :	Efface le fichier provisoire et ferme les
 * fichiers de travail. 
 */
{
	remove(dmo);		/* efface le fichier provisoire .$$$ */
#ifdef FRANCAIS
	printf("\nDépassement du nombre de satellites autorisé!\n");
	printf(" ou pas assez de mémoire.\n");
#else
	printf("\nToo much satellites on file !\n");
	printf(" or not enough memory for the program.\n");
#endif
	sortie(1);
}

int main(int argc, char *argv[])
{
	int i, j, k, l, m, n = 0;
	/* long jour;		 secondes depuis 1er Janv 1970 0 heures GMT */
	/* char option1;	 option format fichier /A AMSAT, /N NASA */
	/* char option2='X';	 option de mise a jour fusion par défaut*/
	/* char option3='X';	 option de délai*/
	char optionA;
	char optionB;
	char optionC;
	time_t temps;

	for (i = 0; i < MAXSAT; i++) 
	{
		vieux[i] = (satel *) (malloc(sizeof(satel)));
		tampon[i] = (satel *) (malloc(sizeof(satel)));
		nouveaux[i] = (satel *) (malloc(sizeof(satel)));

		if (nouveaux[i] == NULL)
			avorte();
	}
	maxsat = i - 1;
	for (i = 0; i < 5; i++)
		options[i] = (char *) (calloc(40, sizeof(char)));
	option = (char *) (malloc(40 * sizeof(char)));
	dmo = (char *) (malloc(80 * sizeof(char)));
	dmold = (char *) (malloc(80 * sizeof(char)));
	opt =  (char *) (malloc(40 * sizeof(char)));
	str = (char *) (malloc(80 * sizeof(char)));
	filename = (char *) (malloc(80 * sizeof(char)));

	strcpy(dmold, "satel.dat");
	strcpy(dmo, "satel.$$$");

	*options[0] = 'A';		/* défaut format AMSAT */
	optionA = 'A';
	*options[1] = 'F';		/* fusion des donnés */
	optionB = 'F';
	date_limite = 100;		/* effacer donnés si supéieures é00 jours */
	optionC = 'D';

	/*
	 * printf("%d arguments\n",argc); for(i=0;i<argc;i++) printf("%s
	 * ",argv[i]); printf("\n"); 
	 */
	if (argc >= 2 && argc <= 5) 
	{
		strcpy(filename, argv[1]);

		for (i = 0; i < argc-2; i++) 
		{
			strcpy(option, argv[i + 2]);
			opt = strstr(option, "/");
			if (opt == NULL)
				break;
			opt++;
			*options[i] = toupper(*opt);
			if (*options[i] == 'D')
				date_limite = atoi(++opt);
		}
	}
#ifdef FRANCAIS
	printf("\nMise à jour des paramètres orbitaux de la banque de données des satellites pour BBS F6FBB\n");
	printf("Version %s - février 2015 - Bernard Pidoux, f6bvp@amsat.org\n",VERSION);
	if ((argc < 2) || (argc > 5)) 
	{
		printf("Emploi: ajoursat nom_fichier<.txt> </option> </option> </option>\n");
		printf("\nOption lecture :");
		printf("\n		/a format AMSAT (par défaut)");
		printf("\n		/n format NASA");
		printf("\nOption mise à jour :");
		printf("\n		/f fusion avec les anciennes données (par défaut)");
		printf("\n		/u uniquement des anciennes données");
		printf("\n		/s seulement les données réactualisées");
		printf("\nOption effacement	 :");
		printf("\n		/dxxx supprime les données de plus de xxx jours");
		printf("\n			  (par défaut 100 jours)\n");
		printf("\n<<maximum %d satellites>>\n", maxsat);
		sortie(0);
	}
#else
	printf("\nUpdate of F6FBB's BBS satellite data base orbital parameters\n");
	printf("Version %s - February 2015 - Bernard Pidoux, f6bvp@amsat.org\n",VERSION);
	if ((argc < 2) || (argc > 5)) 
	{
		printf("Usage: satupdat file_name<.txt> </option> </option> </option>\n");
		printf("\nReading option:");
		printf("\n		/a AMSAT format (default)");
		printf("\n		/n NASA format");
		printf("\nUpdate option:");
		printf("\n		/f merging new and old data (default)");
		printf("\n		/u update only satellites being in the data base");
		printf("\n		/s keep only satellites being in the input file");
		printf("\nDelete option:");
		printf("\n		/dxxx delete data older than xxx days");
		printf("\n			  (default 100 days)\n");
		printf("\n<<%d satellites maximum>>\n", maxsat);
		sortie(0);
	}
#endif	

	time(&temps);
	cejour = gmtime(&temps);

	for (i = 0; i < argc - 2; i++) 
	{
		if (*options[i] == 'A')
			optionA = 'A';
		else if (*options[i] == 'N')
			optionA = 'N';
		else if (*options[i] == 'S')
			optionB = 'S';
		else if (*options[i] == 'U')
			optionB = 'U';
		else if (*options[i] == 'F')
			optionB = 'F';
		else if (*options[i] == 'D')
			optionC = 'D';
	}

#ifdef FRANCAIS
	if (optionA == 'A')
		printf("Lecture format AMSAT\n");
	else if (optionA == 'N')
		printf("Lecture format NASA avec CRC-10\n");
	printf("Mise  jour ");
	if (optionB == 'S')
		printf("en gardant seulement les satellites de la nouvelle liste\n");
	else if (optionB == 'U')
		printf("uniquement des données des anciens satellites\n");
	else if (optionB == 'F')
		printf("en fusionnant la nouvelle avec l'ancienne liste\n");
	if (optionC == 'D')
		printf("avec suppression des données de plus de %d jours.\n", date_limite);
#else
	if (optionA == 'A')
		printf("Reading AMSAT format\n");
	else if (optionA == 'N')
		printf("Reading NASA format with CRC-10\n");
	printf("Update ");
	if (optionB == 'S')
		printf("keeping only satellites being in the input file\n");
	else if (optionB == 'U')
		printf("only satellites being already in the data base\n");
	else if (optionB == 'F')
		printf("merging new list with old list\n");
	if (optionC == 'D')
		printf("deleting data older than %d days.\n", date_limite);
#endif

	n = Lire_Ancien(dmold);
	if (n > 1) 
	{
		Tri_Cat(vieux, n);
	}

	if (n != -1)
	{
#ifdef FRANCAIS
		printf("\n%d données de moins de %d jours", n, date_limite);
#else
		printf("\n%d data less than %d days", n, date_limite);
#endif
	}
	
	if (strchr(filename, '.') == NULL) 
	{
		strtok(filename, ".");
		strcat(filename, ".txt");
	}

#ifdef FRANCAIS
	printf("\nLecture du fichier: %s\n\n", filename);
#else
	printf("\nReading file: %s\n\n", filename);
#endif
				 
	if (optionA == 'A') 
		Amsat(filename);
	else if (optionA == 'N') 
		nasa(filename);

	m = good_crc;	
	
	if (good_crc > 0) 
	{		
		/* Il existe au moins une donné nouvelle... */
		Tri_Cat(nouveaux, m);
	printf ("(%d)",m); 
		j = doublons(m);
	printf ("(%d)",j); 
		k = Misajour(vieux, n, nouveaux, j, optionB);
		Tri(nouveaux, k);	/* tri alphabetique pour liste */
	printf ("(%d)",k); 
		/* Print_Sat(nouveaux,j); */
		remove("satel.bak");
		rename(dmold, "satel.bak");
		if ((l = Ecrit_Sat(dmo, nouveaux, k)) != k) 
#ifdef FRANCAIS
			printf("\nErreur écriture fichier '%s'\n", dmo);
#else
			printf("\nError writing file '%s'\n", dmo);
#endif
				 
		rename(dmo, dmold);
	}

#ifdef FRANCAIS
	if (good_crc > 0) 
		printf("\nLes données de %d satellites sur %d ont été réactualisées", nsat, k);
	else if (good_crc == 0)
		printf("\nAucune nouvelle donnée");
	else if (good_crc == -1)
		printf("\nFichier %s non trouvé !", filename);
#else
	if (good_crc > 0) 
		printf("\nData of %d satellites over %d have been updated", nsat, k);
	else if (good_crc == 0)
		printf("\nNothing to update");
	else if (good_crc == -1)
		printf("\nFile %s not found !", filename);
#endif

#ifdef FRANCAIS
	printf(" le %02d/%02d/%02d ", cejour->tm_mday, cejour->tm_mon+1, cejour->tm_year%100);
	printf("a %02u:%02u:%02u\n", cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#else
	printf(" on %02d/%02d/%02d ", cejour->tm_mday, cejour->tm_mon+1, cejour->tm_year%100);
	printf("at %02u:%02u:%02u\n", cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#endif

	sortie(0);
	return 0;
}

/*
 * fonction validation du checksum NASA revue le 24/01/92 pour nouveau format 
 * NASA avec signe (+) valeur 0 
 */

int crcdix(char *ligne)
{
	char caract;
	char *pointeur;
	int i, crc10, crc, valide;

	crc10 = 0;
	pointeur = ligne;
	/*
	 * printf("\n%s\n",ligne);
	 */
	for (i = 0; i < 68; i++) 
	{
		caract = pointeur[i];
		if (caract == '-')
			crc = 1;
		else if (caract == '+')
			crc = 0;
		else
			crc = atoi(&caract);
		crc10 += crc;
		crc10 = crc10 % 10;
	}
	caract = pointeur[68];
	crc = atoi(&caract);
	if (crc == crc10)
		valide = 1;
	else
		valide = -1;
	return (valide);
}

/*************************** FORMAT AMSAT *************************/

int crc, crc10;
int checksum(char *ligne)
{
	int i;
	char caract;

	for (i = 0; i < strlen(ligne); i++) 
	{
		caract = ligne[i];
		if (caract == '-')
			crc += 1;
		else if (caract == '+')
			crc += 2;
		else
			crc += atoi(&caract);
	}
	/* printf("*%d*\n",crc); */
	return (crc);
}

void Amsat(char *filename)
{
	FILE *fd;
	char tampon[82];
	char ligne[81];
	char buf1[41] = "\0";
	char buf2[41] = "\0";
	char nom[18];
	long jour;			/* secondes depuis 1er Janv 1970 0 heures GMT */
	int k, n, m = 0;
	int valide;			/* validation enregistrement */

	fd = fopen(filename, "rt");
	if (fd != NULL) 
	{

		while (!feof(fd)) 
		{
			crc = valide = 0;
			do {
				if (fgets(tampon, LINE, fd) != NULL) {
				sscanf(tampon, "%s", ligne);
				d_printf("%s\n",ligne);
				}
			} while (strcmp("Satellite:", ligne) && !(feof(fd)));

			crc = checksum(tampon);

			k = sscanf(tampon, "%*s %s %s %s", nom, buf1, buf2);
			if (k > 1)
				strcat(nom, " "), strncat(nom, buf1, 9);
			else if (k > 2)
				strcat(nom, " "), strncat(nom, buf2, 9);

			strncpy(sat.dd, nom, 17);
			sat.dd[17] = '\0';

			if (!feof(fd)) 
			{

				k = fscanf(fd, "%80s", ligne);
				if (strncmp("Catalog", ligne, 7) == 0 && !feof(fd))
					valide = 1;
				for (n = 0; n < 2; n++)
					k = fscanf(fd, "%80s", ligne);
				sat.cat = atol(ligne);	/* Catalog number */
				crc = checksum(ligne);

				for (n = 0; n < 2; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
					d_printf("%s\n",ligne);
					d_printf("\n*%s*",ligne);
				}
				
				k = fscanf(fd, "%2d", (int *)&sat.y3);
				d_printf("%s\n", ligne);
				/* itoa(sat.y3,ligne,10); */
				sprintf(ligne, "%2d", sat.y3);
				crc = checksum(ligne);
				d_printf("%d ", sat.y3);

				k = fscanf(fd, "%lf", &sat.d3);
				d_printf("%s\n",ligne);
				sprintf(ligne, "%3.8lf", sat.d3);
				crc = checksum(ligne);
				d_printf("%lf\n",sat.d3);

				/* Nouvelle ligne élire */
				for (n = 0; n < 2; n++) 
				{
					k = fscanf(fd, "%80s", ligne);	/* Element Set */
					d_printf("%s\n",ligne);
					d_printf("\n*%s*",ligne);
				}
				k = fscanf(fd, "%4d", (int *)&sat.set);	/* element number */
				sprintf(ligne, "%4d", sat.set);
				crc = checksum(ligne);
				d_printf("%4d\n",sat.set);

				k = fscanf(fd, "%80s", ligne);
				d_printf("*%s*",ligne);
				k = fscanf(fd, "%lf", &sat.i0); /* inclinaison */
				sprintf(ligne, "%3.8lf", sat.i0);
				d_printf("%lf\n",sat.i0);
				crc = checksum(ligne);

				for (n = 0; n < 4; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
					crc = checksum(ligne);
				}
				k = fscanf(fd, "%lf", &sat.o0); /* R.A.A.N. */
				sprintf(ligne, "%3.8lf", sat.o0);
				crc = checksum(ligne);

				for (n = 0; n < 2; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
					crc = checksum(ligne);
				}
				k = fscanf(fd, "%lf", &sat.e0); /* excentricite */
				sprintf(ligne, "%3.8lf", sat.e0);
				crc = checksum(ligne);

				for (n = 0; n < 3; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
				}
				k = fscanf(fd, "%lf", &sat.w0); /* arg. perigee */
				sprintf(ligne, "%3.8lf", sat.w0);
				crc = checksum(ligne);

				for (n = 0; n < 3; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
				}
				k = fscanf(fd, "%lf", &sat.m0); /* anomal. moyenne */
				sprintf(ligne, "%3.8lf", sat.m0);
				crc = checksum(ligne);

				sat.a0 = 0.0;	/* * contient 0.0 */

				for (n = 0; n < 3; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
					crc = checksum(ligne);
				}
				k = fscanf(fd, "%lf", &sat.n0); /* mouvement moyen */
				sprintf(ligne, "%3.8lf", sat.n0);
				crc = checksum(ligne);

				for (n = 0; n < 3; n++) 
				{
					k = fscanf(fd, "%80s", ligne);
					crc = checksum(ligne);
				}
				k = fscanf(fd, "%lf", &sat.q3); /* deriv. mouvem. moy. */
				sprintf(ligne, "%2.7e", sat.q3);
				crc = checksum(ligne);
				
				k = fscanf(fd, "%80s", ligne);	/* day ^2 */
				crc = checksum(ligne);
				
				k = fscanf(fd, "%80s", ligne);	/* Epoch */
				crc = checksum(ligne);

				if (strcmp("Epoch", ligne) != 0)
					valide = 0;
				k = fscanf(fd, "%80s", ligne);	/* rev: */
				crc = checksum(ligne);
				
				k = fscanf(fd, "%ld", &sat.k0); /* num. orbite referen. */
				sprintf(ligne, "%ld", sat.k0);
				crc = checksum(ligne);

				k = fscanf(fd, "%80s", ligne);
				/*
				 * while(strcmp("Checksum:",ligne) && !(feof(fd))) ;
				 */
				 
				k = fscanf(fd, "%d", &crc10);
				d_printf("CRC calcule:%d et lu:%d\n",crc,crc10);
				sat.f1 = 0.0;
				sat.v1 = 0.0;
				sat.pas = 5;

				time(&jour);
				sat.maj = jour;

				if ((sat.cat != 0) && (valide != 0)) 
				{
					if ((crc == crc10) && (fraicheur())) 
						printf("%3d : %-18s",  m + 1, sat.dd);
					else
						printf("      %-18s", sat.dd);
					
					if (fraicheur()) 
					{
						if (crc == crc10) 
						{
							CopyStru(nouveaux, m, &sat);
							m++;	/* incrémente le compteur de satellites mis à jour */
							printf(" ok\n");
							if (m > maxsat)
								break;;
						} 
						else
						{
#ifdef FRANCAIS
							printf(" erreur de CRC !\n");
#else
							printf(" bad CRC !\n");
#endif
						}
					} 
					else
					{
#ifdef FRANCAIS
						printf(" données trop anciennes !\n");
#else
						printf(" data too old !\n");
#endif		
					}
				}
			}		/* tant que pas fin de fichier */
		}			/* tant que pas fin de fichier */
	fclose(fd);
	}				/* si pas NULL	*/
	else
		m = -1;
/*return (m);*/
	good_crc = m;
}

void CopyStru(satel ** stru, int indice, satel * psat)
{
	*stru[indice] = *psat;
}

/*
 * élimination des doublons de satellites
 * en conservant les donnés les plus récentes
 * retourne le nombre de satellites
 */

int doublons(int nombre)
{
	int i, j, k;

#ifdef FRANCAIS
	printf("\nrecherche de doublons");
#else
	printf("\nlooking for duplicate");
#endif				/*
				 * FRANCAIS 
				 */

	for (i = 0; i < nombre - 1; i++) 
	{
		printf(".");
		for (j = i + 1; j < nombre; j++) 
		{
			if (nouveaux[i]->cat == nouveaux[j]->cat) 
			{
				if (((nouveaux[i]->y3 == nouveaux[j]->y3)
					 && (nouveaux[i]->d3 > nouveaux[j]->d3))
					|| (nouveaux[i]->y3 > nouveaux[j]->y3)) 
				{
					nouveaux[j]->libre[0] = -1;
				} 
				else 
				{
					nouveaux[i]->libre[0] = -1;
				}
			}
		}
	}

	k = 0;
	for (i = 0; i < nombre; i++) 
	{
		if (nouveaux[i]->libre[0] != -1) 
		{
			CopyStru(tampon, k, nouveaux[i]);
			k++;
		}
	}
	for (i = 0; i < k; i++) 
	{
		CopyStru(nouveaux, i, tampon[i]);
	}
	return (i);
}

/*
 * Ecrit_Sat.c 
 */

int Ecrit_Sat(char *filename, satel **stru, int nombre)
{
	int fd2;
	int indice, n;
	char *str;

	str = (char *) (malloc(30 * sizeof(char)));
	strtok(filename, ".");
	strcat(filename, ".dat");
	sprintf(str, "%s", filename);
	strcpy(filename, str);
	n = nombre;

	fd2 = open(filename, O_CREAT | O_RDWR, S_IREAD | S_IWRITE | S_IEXEC);
	if (fd2 != -1) 
	{
		for (indice = 0; indice < nombre; indice++) 
		{
			if (stru[indice]->libre[0] != -1) 
			{
				if (write(fd2, stru[indice], sizeof(satel)) == 0)
#ifdef FRANCAIS
		printf("Impossible d'écrire dans le fichier %s !\n", filename);
#else
		printf("Could not write into file %s !\n", filename);
#endif
			} 
			else
				n--;
		}
	close(fd2);
	}

	free(str);
	return (n);
}

/* LIRVIEUX.C
 * Relecture de la base de donné des parametres képlériens
 * avec suppression des donnés qui dépassent la date_limite
 */

int fd2;
int openfile(char *filename)
/*
 * Nom:			  openfile() Description :	Ouvre le fichier source.
 * Commentaire :  Les fichiers doivent être dans le réertoire FICHIERS 
 */
{
	char *str;

	str = (char *) (malloc(30 * sizeof(char)));
	strtok(filename, ".");
	strcat(filename, ".dat");
	sprintf(str, "%s", filename);
	strcpy(filename, str);
	/*
	 * fd2 = open(filename,O_BINARY); 
	 */
	fd2 = open(filename, O_RDWR, S_IREAD | S_IWRITE | S_IEXEC);

	if (fd2 == -1) {
#ifdef FRANCAIS
	printf("fichier %s non trouvé - nouvelle banque créée !\n", filename);
#else
	printf("file %s not found - new data base created !\n", filename);
#endif
	fd2 = open(filename, O_CREAT | O_RDWR, S_IREAD | S_IWRITE | S_IEXEC);
	}
#ifdef FRANCAIS
	printf("lecture fichier %s\n", filename);
#else
	printf("reading file %s\n", filename);
#endif
	
	return (fd2);
}

int Lire_Ancien(char *filename)
{
	int n, m, lg;
	if ((n = openfile(filename)) != -1) 
	{
		m = 0;
		while (m < maxsat) 
		{
			lg = read(fd2, &sat, sizeof(sat));
			if (lg <= 0)
				break;

			if (fraicheur()) 
			{
				CopyStru(vieux, m, &sat);
				m++;
			}
		}
		if (m > maxsat)
			avorte();
		close(fd2);
		return (m);
	} 
	else
		return (n);
}

/*
 * MISAJOUR.C	version 28 Avril 1991
 * retourne le nombre de satellites dans la nouvelle liste
 * le compteur nsat est égal au nombre de satellites réactualisé
 * libère la mémoire tampon en sortie	
 */

int Misajour(satel ** vieux, int vv, satel ** nouv, int nn, char option)
{
	int i, j, nnn, vvv;
	long jour;	/* secondes depuis 1er Janv 1970 0 heures GMT */

	i = 0;
	nsat = 0;
	nnn = 0;
	vvv = 0;
	--nn;
	--vv;
	time(&jour);

	switch (option) 
	{
	/* Option de fusion anciennes & nouvelles données */
	case 'F':
		/* on compare anciennes et nouvelles données 
		 * en fonction du numéro de catalogue */
		while ((nnn <= nn) && (vvv <= vv)) 
		{
			if (nouv[nnn]->cat < vieux[vvv]->cat) 
			{
				CopyStru(tampon, i, nouv[nnn]);
				nnn++;
				nsat++;
			} 
			else if (nouv[nnn]->cat == vieux[vvv]->cat) 
			{
				/* Objet courant déja au catalogue */
				if (((nouv[nnn]->y3 == vieux[vvv]->y3)
						 && (nouv[nnn]->d3 > vieux[vvv]->d3))
						|| (nouv[nnn]->y3 > vieux[vvv]->y3)) 
				{
					CopyStru(tampon, i, nouv[nnn]);
					nouv[nnn]->maj = jour;
					memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
					memcpy(&tampon[i]->f1, &vieux[vvv]->f1, sizeof(sat.f1));
					memcpy(&tampon[i]->pas, &vieux[vvv]->pas, sizeof(sat.pas));
					nsat++;
				} 
				else 
				{
					CopyStru(tampon, i, vieux[vvv]);
				}
				nnn++;
				vvv++;
			} 
			else if (nouv[nnn]->cat > vieux[vvv]->cat) 
			{
				CopyStru(tampon, i, vieux[vvv]);
				vvv++;
			}
			i++;
		}

		if (nnn >= nn) 
		{
			while (vvv <= vv) 
			{
				CopyStru(tampon, i, vieux[vvv]);
				vvv++;
				i++;
			}
		}
		if (vvv >= vv) 
		{
			while (nnn <= nn) 
			{
				CopyStru(tampon, i, nouv[nnn]);
				nouv[nnn]->maj = jour;
				memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
				nnn++;
				i++;
				nsat++;
			}
		}

		break;

	case 'S':
		while ((nnn <= nn) && (vvv <= vv)) 
		{
			if (nouv[nnn]->cat < vieux[vvv]->cat) 
			{
				CopyStru(tampon, i, nouv[nnn]);
				nouv[nnn]->maj = jour;
				memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
				nnn++;
				i++;
				nsat++;
			} 
			else if (nouv[nnn]->cat == vieux[vvv]->cat) 
			{
				if (((nouv[nnn]->y3 == vieux[vvv]->y3)
						&& (nouv[nnn]->d3 > vieux[vvv]->d3))
						|| (nouv[nnn]->y3 > vieux[vvv]->y3)) 
				{
					CopyStru(tampon, i, nouv[nnn]);
					nouv[nnn]->maj = jour;
					memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
					/* On conserve la fréuence balise et le pas du calcul */
					memcpy(&tampon[i]->f1, &vieux[vvv]->f1, sizeof(sat.f1));
					memcpy(&tampon[i]->pas, &vieux[vvv]->pas, sizeof(sat.pas));
					nsat++;
				} 
				else 
				{
					CopyStru(tampon, i, vieux[vvv]);
				}
				nnn++;
				vvv++;
				i++;
			} 
			else if (nouv[nnn]->cat > vieux[vvv]->cat)
			{
				vvv++;
			}
		}

		if (vvv >= vv) 
		{
			while (nnn <= nn) 
			{
				CopyStru(tampon, i, nouv[nnn]);
				nouv[nnn]->maj = jour;
				memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
				nnn++;
				i++;
				nsat++;
			}
		}

		break;
		
	case 'U':
		/* mettre à jour seulement les données des satellites déja connus */
		while ((nnn <= nn) && (vvv <= vv)) 
		{
			if (nouv[nnn]->cat < vieux[vvv]->cat) 
			{
				nnn++;
			} 
			else if (nouv[nnn]->cat == vieux[vvv]->cat) 
			{
				if (((nouv[nnn]->y3 == vieux[vvv]->y3)
						 && (nouv[nnn]->d3 > vieux[vvv]->d3))
						|| (nouv[nnn]->y3 > vieux[vvv]->y3)) 
				{
					CopyStru(tampon, i, nouv[nnn]);
					nouv[nnn]->maj = jour;
					memcpy(&tampon[i]->maj, &nouv[nnn]->maj, sizeof(sat.maj));
					memcpy(&tampon[i]->f1, &vieux[vvv]->f1, sizeof(sat.f1));
					memcpy(&tampon[i]->pas, &vieux[vvv]->pas, sizeof(sat.pas));
					nsat++;
				} 
				else 
				{
					CopyStru(tampon, i, vieux[vvv]);
				}
				nnn++;
				vvv++;
				i++;
			} 
			else if (nouv[nnn]->cat > vieux[vvv]->cat) 
			{
				CopyStru(tampon, i, vieux[vvv]);
				vvv++;
				i++;
			}
		}

		if (nnn >= nn) 
		{
			while (vvv <= vv) 
			{
				CopyStru(tampon, i, vieux[vvv]);
				vvv++;
				i++;
			}
		}

		break;
	}

	for (j = 0; j < i; j++)
		CopyStru(nouv, j, tampon[j]);

	return i;

}

/********************************
 *	NASA.C			*
 *				*
 * Version 1.6 du 23/10/94
 * retourne -1 si fichier non trouvé
 *				*
 *	avec CRC10		*
 ********************************/

int crcdix(char *ligne);

int fraicheur()
{
	double anciennete;	/* ancienneté des prévisions en nombre de jours */
	double jours;		/* différence de jours */
	int annees;			/* nombre d'annés */

	jours = (double) cejour->tm_yday - sat.d3;
	annees = (cejour->tm_year - sat.y3) % 100;	/* nombre d'années */
		
	
	if (annees == 0) 
	{
		anciennete = jours;
		d_printf(" %d jours d'ancienneté ",(int)(jours + 0.5));
	} 
	else 
	{
/* jours éoulé */
		anciennete = (365.0 - sat.d3) + (double) (annees - 1) * 365.0 + (double) cejour->tm_yday;
		d_printf(" = %02d années écoulées; %d jours d'ancienneté",annees,(int)(anciennete+0.5));
	}
	if (anciennete < date_limite)
		return 1;
	else
		return 0;
}

char *epure(char *ligne)
{
	int len = strlen(ligne);
	char *ptr;
	
	while (len > 0)
	{
		ptr = ligne + len - 1;
		if (isgraph(*ptr))
			break;
		*ptr = '\0';
		--len;
	}
	
	return ligne;
}

void nasa(char *filename)
{
	int i, m, r;			/* m = number of sat entries - r number of rejected entries */
	int twolinemodel;		/* 2Line model */
	int ligne1valide, ligne2valide; /* enregistrement valide 1 sinon 0 */
	char *ligne;			/* tampon pour lire ligne du fichier */
	long jour;			/* secondes depuis 1er Janv 1970 0 heures GMT  */
	double i0;			/* Inclinaison */
	double o0;			/* R.A.A.N. */
	double e0;			/* Excentricite */
	double w0;			/* Argument de perigee */
	double m0;			/* Anomalie moyenne */
	double n0;			/* Mouvement moyen */
	long k0;			/* Orbite de reference */
	int set;			/* Element Number */
	long catalog, cat;		/* Catalog Number - anciennement vide */
	int y3;				/* Annee de reference */
	double d3;			/* Jour de reference */
	double q3;			/* Derivee du mouv. moy. */
	char caract[13];
	char *pointe, *pointeur;
	satel *satp;
	FILE *fd;

	satp = &sat;

	ligne = (char *) (malloc(82 * sizeof(char)));
	if (ligne == NULL)
		avorte();

	m = r = 0;

	fd = fopen(filename, "rt");

	if (fd != NULL) 
	{

		while (!feof(fd)) 
		{

			twolinemodel = FALSE;
			ligne1valide = TRUE;
			ligne2valide = TRUE;
			catalog = -1;
			strcpy(ligne, " ");
			pointe = strchr(ligne, '1');
			pointeur = strchr(ligne, 'U');

			while ((pointe - ligne != 0) || (pointeur - ligne != 7)) 
			{
				strncpy(sat.dd, ligne, 17);
				sat.dd[17] = '\0';
				if (strncmp (sat.dd, "DECODE 2-LINE", 13) == 0)
					twolinemodel = TRUE;
				if (fgets(ligne, LINE, fd) == NULL) 
					break;
				epure(ligne);
				pointe = strchr(ligne, '1');
				pointeur = strchr(ligne, 'U');
			}

			if (feof(fd))
				break;
				
			ligne1valide = crcdix(ligne);

			if (!feof(fd)) 
			{
				caract[5] = '\0';
				for (i = 2; i < 7; i++)
					caract[i - 2] = ligne[i];
				cat = atol(caract); /* numéro catalogue */

				caract[2] = '\0';
				caract[0] = ligne[18];
				caract[1] = ligne[19];
				y3 = atoi(caract);	/* année */

				caract[12] = '\0';
				for (i = 20; i < 32; i++)
					caract[i - 20] = ligne[i];
				d3 = atof(caract);	/* jour julien */

				caract[10] = '\0';
				for (i = 33; i < 43; i++)
					caract[i - 33] = ligne[i];
				q3 = atof(caract);	/* 1ère dérivée mouvement moyen */

				caract[4] = '\0';
				for (i = 64; i < 68; i++)
					caract[i - 64] = ligne[i];
				set = atoi(caract); /* Element Set */

				sat.set = set;	/* Element Number */
				sat.cat = cat;	/* numéro catalogue */
				sat.y3 = y3;	/* année  */
				sat.d3 = d3;	/* jour julien */
				sat.q3 = q3;	/* 1ère dérivée mouvement moyen */


				if (fgets(ligne, LINE, fd) != NULL)
				
				ligne2valide = crcdix(ligne);

				epure(ligne);

				if (ligne[0] != '2')
					ligne2valide = 0;	/* deuxième ligne */
				caract[5] = '\0';
				for (i = 2; i < 7; i++)
					caract[i - 2] = ligne[i];
				catalog = atol(caract); /* numéro catalogue */
				if (catalog != cat)
					ligne1valide = 0;
				caract[8] = '\0';
				for (i = 8; i < 16; i++)
					caract[i - 8] = ligne[i];
				i0 = atof(caract);	/* inclinaison */
				caract[8] = '\0';
				for (i = 17; i < 25; i++)
					caract[i - 17] = ligne[i];
				o0 = atof(caract);	/* R.A.A.N. */
				caract[7] = '\0';
				for (i = 26; i < 33; i++)
					caract[i - 26] = ligne[i];
				e0 = atof(caract);	/* excentricité*/
				e0 /= 10e+06;
				caract[8] = '\0';
				for (i = 34; i < 42; i++)
					caract[i - 34] = ligne[i];
				w0 = atof(caract);	/* arg. périgé */
				caract[8] = '\0';
				for (i = 43; i < 51; i++)
					caract[i - 43] = ligne[i];
				m0 = atof(caract);	/* anomal. moyenne */
				caract[11] = '\0';
				for (i = 52; i < 63; i++)
					caract[i - 52] = ligne[i];
				n0 = atof(caract);	/* mouvement moyen */
				caract[5] = '\0';
				for (i = 63; i < 68; i++)
					caract[i - 63] = ligne[i];
				k0 = atol(caract);	/* numéro orbite */

				sat.i0 = i0;	/* inclinaison */
				sat.o0 = o0;	/* R.A.A.N. */
				sat.e0 = e0;	/* excentricité*/
				sat.w0 = w0;	/* arg. périgé */
				sat.m0 = m0;	/* anomal. moyenne */
				sat.n0 = n0;	/* mouvement moyen */
				sat.k0 = k0;	/* numéro orbite */

				sat.a0 = 0.0;	/* contient 0.0 */
				sat.f1 = 0.0;
				sat.v1 = 0.0;

				sat.pas = 5;
				time(&jour);
				sat.maj = jour;

				if (twolinemodel == FALSE) 
				{
					if ((ligne1valide == -1) || (ligne2valide == -1) || !fraicheur()) 
						printf("      %-18s", sat.dd);
					else
						printf("%3d : %-18s", m + 1, sat.dd);
					if (ligne1valide == TRUE && ligne2valide == TRUE) 
					{
						if (fraicheur()) 
						{
							CopyStru(nouveaux, m, satp);
							m++;	/* incrémente le compteur de satellites mis à jour */
							printf(" ok\n");
							if (m > maxsat)
								break;
						}
						else
						{
#ifdef FRANCAIS
							printf(" données trop anciennes !\n");
#else
							printf(" data too old !\n");
#endif			
						}
					}
					else
					{
						r++;
#ifdef FRANCAIS
						printf(" erreur de CRC !\n");
#else
						printf(" CRC error !\n");
#endif

					}
				}
			ligne1valide = TRUE;
			ligne2valide = TRUE;
			}		/* tant que pas fin de fichier */
		}			/* tant que pas fin de fichier */
    fclose(fd);
	} 
	else
		m = -1;			/* si nul=fichier non trouvé*/
	free(ligne);
	good_crc = m;
	bad_crc = r;
}

/* tri a bulle des noms de satellites  */
void Tri(satel ** stru, int nombre)
{				
	int i, j;
	satel *tampon;		/* tampon pour comparaison */

#ifdef FRANCAIS
	printf("\ntri alphabétique");
#else
	printf("\nsorting by name");
#endif

	/* on trie les pointeurs */
	for (i = 0; i < nombre - 1; i++) 
	{
		printf(".");
		for (j = i + 1; j < nombre; j++) 
		{
			if (strcmp(stru[i]->dd, stru[j]->dd) > 0) 
			{
				tampon = stru[i];
				stru[i] = stru[j];
				stru[j] = tampon;
			}
		}
	}
}


/* tri à bulle des numéros de catalogue de satellites */
void Tri_Cat(satel ** stru, int nombre)
{
	int i, j;
	satel *tampon;		/* tampon pour comparaison	*/

#ifdef FRANCAIS
	printf("\ntri par numéro de catalogue");
#else
	printf("\nsorting by catalog number");
#endif 

	/* on trie les pointeurs */
	for (i = 0; i < nombre - 1; i++) 
	{
		printf(".");
		for (j = i + 1; j < nombre; j++) 
		{
			if (stru[i]->cat > stru[j]->cat) 
			{
				tampon = stru[i];
				stru[i] = stru[j];
				stru[j] = tampon;
			}
		}
	}
}
