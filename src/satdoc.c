/* SATDOC.C by Bernard PIDOUX, F6BVP [ f6bvp@amsat.org ]

 Updates F6FBB BBS satellite documentation files from informations
 extracted from AMSAT bulletins AMSAT NEWS SERVICE 
 version 2.8 : option fichiers inverses avec noms des satellites
 sur 8 caracteres maximum XXXXXXXX.SAT
 version 2.8.1 INTERNATIONAL SPACE STATION ---> ISS
               corrige bogue noms < 8 caracteres
 version 2.8.2  supprime la redefinition de strncasecmp
		modification du compteur de fichiers documentaires
		option de compilation ACCENTS 
 version 2.8.3 for GCC compliance
 version 2.8.4 In special cases satellite names iare like satellit-1
               doc filename is translated as satellit1.sat
	       increased TAILLE from 80 to 122 to avoid buffer overflow
 version 2.8.5 displayed a wrong number of created files. Corrected
 version 2.8.6 the format for number of lines processed changed
 version 2.8.7 ANS BID changed from $ANS to $WSR. Corrected
 version 2.8.8 Both $ANS and $WSR detected are valid.
 version 2.8.9 strncmp() for Windows needed small code change.
*/
/********************************************************************
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston,
MA  02111-1307  USA
*********************************************************************/
/* A faire :
traitement special pour le cas de satellites doubles comme RS-12 / RS-13 ayant le meme numero NASA*/

/* #define FRANCAIS */
#define _LINUX_ 
#define ACCENTS
#define _DOS_
#define VERSION "2.8.9"

#ifdef _DOS_
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#ifdef _LINUX_
#include <sys/time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif
/*
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#else
#define strncasecmp strncmp
*/
#define strncasecmp strncmp
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#endif

#define LINE 256
#define TAILLE 256

/********** Prototypes **************/
int  lecture(int fd2);
int  ecriture(int fd1);
int  repereANS(char *infos);
int  repereOBJ(char *buf2, char *buf3, char *infos);
void cataloguer(FILE *fptr, int lignes, char *infos, char *buf2, short inverse);
int  lire_option(int argc, char *argv[]);
void usage(void);

/* SEMAPHORES & VARIABLES GLOBAUX */
 int valable = 0;		// ce drapeau indique que la 1re ligne est valable
 int info_satellite = 0;	// drapeau chapitre info satellite
 int info_ans = 0;		// drapeau BULLETIN ANS
 int info_frequence = 0;	// drapeau FREQUENCE BALISE
 int info_generale = 0;		// drapeau INFORMATIONS GENERALES par defaut
/* structure & variables globales */
struct tm *cejour;		// date et time declares dans TIME.H
int bulletin_ANS;		// nombre de bulletins trouvees
int doc_satellites;		// nombre de doc satellites traitees
long lignes;			// nombre de lignes parcourues
char ans[TAILLE] = "\0";	// BID ANS $
char nom_objet[TAILLE] = "\0";	// Nom de l'objet
char catalogue[TAILLE] = "\0";	// Nom de l'objet


void usage()
 {
#ifdef FRANCAIS
        printf("\nEmploi: SATDOC [-i] <nomfichier> (voir documentation)\n");
#else
        printf("\nUsage: SATDOC [-i] <filename>  (see doc file)\n");
#endif
exit(1);
}

int lire_option(int argc, char *argv[])
{
/* Lecture de l'option
   retourne -1 si erreur
   retourne  2 si normale
   retourne  1 si inverse
*/
char option[15];
char *opt;
char val;

 if(argc > 1) {
	strcpy(option,argv[1]);
			opt=strstr(option,"-");
 			if(opt != NULL) {
         	opt++;
				val=toupper(*opt);
 				if(val == 'I') {
            	if(argc == 3)
            		return(2);
               if(argc == 2)
            		return(-1);
            }
         }
         else
         		return(1);
  }
return(-1);
}

/* detection d'un bulletin AMSAT Weekly Satellite Report WSR*/
int repereANS(char *infos)
{
	if ((strncasecmp(infos,"BID: $ANS-",10) == 0) || (strncasecmp(infos,"BID: $WSR-",10) == 0)) {
		info_ans = 1;
		strcpy(ans, infos);
		printf("\n%s\n",ans);
		bulletin_ANS++ ;
		return 1;
	}
	else
		return 0;
 }

/* detection debut information OBJET NASA *
 * retourne 0 si non calage
 * retourne 1 si numéro d'objet présent
 * retourne -1 si "Catalog:" sans numéro */

int repereOBJ(char *buf2, char *buf3, char *infos)
{
 int i, catalog;
 valable = 0;

 if ((strncasecmp(infos, "Catalog number:",15) == 0) ) {
/* Controle renforce de la validite du numero d'objet  [ Version 1.3 ] */
	if (strlen (buf2) != 0)
		sscanf (buf3,"%[0-9]",buf2);
/* Tous les caracteres du champ numerique de catalog number doivent etre des chiffres */
	catalog = atoi(buf2);
/*	if (strlen(buf3) == strlen(buf2) && catalog != 0) { */
	if (catalog == 0)
		valable = -1;
	else
		valable = 1;
	info_satellite = 1;
	sprintf(buf2,"%d", catalog);
/* 	} */
 }
 i = valable;
return (i);
}

void cataloguer(FILE *fptr, int lignes, char *infos, char *buf2, short inverse)
{
 FILE *fiche;
 char tampon[LINE]="\0";	// tampon pour chaine de caracteres
 char buffer[LINE];
 char *nom;
 int fin,k;
/* Option fichiers au nom des numeros de catalogues */
 if (inverse == 1)
        strcpy(tampon,buf2);
        else 
	{
/* Option fichiers au nom des satellites */
        strcpy(tampon,nom_objet);
        nom = strchr(tampon,(int)' ');
        if (nom != NULL)
                *nom = '\0';
	if (tampon[7] == '-') 
		tampon[7] = tampon[8];
       	if (strlen(tampon) > 8 ) 
                tampon[8] = '\0';
	nom = strchr(tampon,(int)'\r');
	if (nom != NULL)
	*nom = '\0';	
        nom = strchr(tampon,(int)'\n');
        if (nom != NULL)
        *nom = '\0';
	}  
	if (strncasecmp(tampon,"INTERNAT",8)== 0) strcpy(tampon,"ARISS");
        strcat(tampon,".sat");
#ifdef FRANCAIS
        printf("Ecriture du fichier document : %s\n",tampon);
#else
        printf("Creating document file : %s\n",tampon);
#endif

/* Ouvrir le fichier doc correspondant
 creation - ouverture nouveau fichier */
		fiche = fopen(tampon,"wt+") ;
		if (fiche == NULL) {
#ifdef FRANCAIS
#ifdef _LINUX_
#ifdef ACCENTS			
                printf("Erreur: impossible de crÃ©er le fichier :'%s' ",tampon);
		printf("Ã  la ligne %d\n",lignes);
#else
                printf("Erreur: impossible de créer le fichier :'%s' ",tampon);
		printf("a la ligne %d\n",lignes);
#endif
#endif		
#else
                printf("Error: unable to create file : '%s'\n",tampon);
		printf("on line %d\n",lignes);
#endif
		exit(1);
		}
/* incremente le nombre de fichiers documentaires crees */
			doc_satellites++;

/* on commence par ecrire la version de SATDOC et la date de mise jour dans le fichier DOC */
	fwrite(ans, strlen(ans), 1, fiche);
	fprintf(fiche," - SATDOC version %s ",VERSION);
#ifdef FRANCAIS
	fprintf(fiche," - %02d/%02d/%04d ",cejour->tm_mday, cejour->tm_mon +1, cejour->tm_year+1900);
	fprintf(fiche,"- %02u:%02u:%02u\n",cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#else
        fprintf(fiche," - %04d/%02d/%02d ",cejour->tm_year+1900, cejour->tm_mon +1,cejour->tm_mday);
	fprintf(fiche,"- %02u:%02u:%02u\n",cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#endif
	fwrite("\n", 1, 1, fiche);
/* on recopie les deux premieres lignes */
/* nom de l'objet */
	strcpy(tampon, nom_objet);
	fwrite(tampon, strlen(tampon), 1, fiche);
/* ligne catalogue number */
	strcpy(tampon, infos);
	strcat(tampon," ");
	strcat(tampon, buf2);
	fwrite(tampon, strlen(tampon), 1, fiche);
	fwrite("\n", 1, 1, fiche);
/* lire jusqu'a la fin du paragraphe */
 do {
	if (fgets(buffer, LINE, fptr) == NULL) {
		fin=1;
        	break;
	}
 	lignes++;
/* sauvegarde pour memoire si ligne avec nom du satellite */
	k = sscanf(buffer, "%s \n", tampon);
	fwrite(buffer, strlen(buffer), 1, fiche);
 } while ((strncasecmp(tampon,"NNNN",4) != 0) && (strncasecmp(tampon,"=====",5) != 0) && (strncasecmp(tampon,"/EX",3) != 0) && (strncasecmp(tampon,"[ANS",4) != 0));

/* while .... && 'strncasecmp(tampon,"-----",5) != 0 enlevé car utilisé parfois en interne */

/* if (strncasecmp(tampon,"/EX",3) == 0)*/
 if ((strncasecmp(tampon,"/EX",3) == 0) || (strncasecmp(tampon,"NNNN",4) == 0))
	info_ans = 0; 		/* fin de bulletin ANS */
	info_satellite = 0;	/* fin du paragraphe du satellite */
/* fermeture fichier documentaire satellite */
 fclose(fiche);
}

int main (int argc, char *argv[])
{
 time_t temps;          	/* date et time */

 int j, k, succes;
 short inverse;
 FILE *fptr;
 char buffer[LINE];
 char infos[LINE];
 char document[LINE];
 char identification[LINE]="\0";  /* tampon pour ligne nom des satellites */
 int fin;		          /* indicateur fin de fichier */
 char buf1[LINE]="\0";	          /* tampon pour lecture 1er mot cle */
 char buf2[TAILLE]="\0";          /* tampon pour lecture 2eme mot cle */
 char buf3[TAILLE]="\0";          /* tampon pour lecture numero catalogue */

/* recupere la date et l'heure systeme */
	time(&temps);
	cejour = gmtime(&temps);

#ifdef FRANCAIS
#ifdef _LINUX_
#ifdef ACCENTS
        printf("\nProgramme DOCumentaire pour BBS F6FBB\n");
        printf("Ã  partir des fichiers bulletins AMSAT SATELLITE NEWS WSR\n");
        printf("Version %s par f6bvp, Bernard Pidoux\n",VERSION);
        printf("10 octobre 2009 - f6bvp@amsat.org\n");
#else
        printf("\nProgramme DOCumentaires pour BBS F6FBB\n");
        printf("à partir des fichiers bulletins AMSAT SATELLITE NEWS WSR\n");
        printf("Version %s par f6bvp, Bernard Pidoux\n",VERSION);
        printf("10 octobre  2009 - f6bvp@amsat.org\n");
#endif
#endif
#else
	printf("\nSATellite DOCumentation program for F6FBB BBS\n");
        printf("using WSR AMSAT SATELLITE NEWS bulletin files\n");
        printf("Version %s by f6bvp, Bernard Pidoux\n",VERSION);
        printf("October 10, 2009 - f6bvp@amsat.org\n");
#endif

 inverse = lire_option(argc,argv);
 if (inverse == -1) usage();
/*
#ifdef FRANCAIS
	printf("\n%02d/%02d/%04d ",  cejour->tm_mday, cejour->tm_mon +1, cejour->tm_year+1900);
	printf("- %02u:%02u:%02u\n", cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#else
	printf("\n%04d/%02d/%02d ",  cejour->tm_year+1900, cejour->tm_mon +1, cejour->tm_mday);
	printf("- %02u:%02u:%02u\n", cejour->tm_hour, cejour->tm_min, cejour->tm_sec);
#endif
*/
/* initialise les tampons */
	*infos='\0';
	*document='\0';

/* D'abord ouvrir le fichier message, puis le lire */
 if (inverse == 2)
   strcpy(identification, argv[2]);
 if (inverse == 1)
   strcpy(identification, argv[1]);
/* if (inverse == 0) exit(1); */
 fptr = fopen(identification, "rt") ; /* Ouvrir le message recu */

 if (fptr == NULL) {
#ifdef FRANCAIS
#ifdef _LINUX_
#ifdef ACCENTS
	printf("\nFichier '%s' non trouvÃ©\n",identification);
#else
	printf("\nFichier '%s' non trouvé\n",identification);
#endif
#endif
#else
	printf("\nFile '%s' not found\n",identification);
#endif
	 exit(1);
 }
/* Initialisation des drapeaux et compteurs... */
 valable = 0;
 lignes = 0;
 succes = 0;
 bulletin_ANS = 0;
 doc_satellites = 0;

/* Examiner le fichier message ligne par ligne */
do {
	info_satellite = 0;
	fin = 0;
/* Aller au debut d'une ligne d'information */
do {
	if (fgets(buffer, LINE, fptr) == NULL) {
		fin = 1;
 	}
   lignes++;
/* sauvegarde pour memoire si ligne avec nom du satellite */
 if (info_satellite == 0)
	strcpy (identification, buffer);

 buf1[0] = '\0';
 buf2[0] = '\0';
 buf3[0] = '\0';

 k = sscanf (buffer, "%s %s %s\n", buf1, buf2, buf3);

 if (k == 0 || k == -1)
	break;
 else {
	if ((strlen(buf1) + strlen(buf2)) < TAILLE-1) {
		strcat(buf1," ");
		strncat(buf1, buf2, strlen(buf2));
	}
	strcpy(infos, buf1);
	strcpy(document, buf3);
/* recherche si fichier bulletin AMSAT Weekly Satellite Report WSR */
	if (repereANS(infos) != 1) {
/* Non : verification si fin d'un bulletin general ANS */
		if (strncasecmp(infos,"[ANS",4) == 0) {
			info_ans = 0;
			info_satellite = 0;
/* printf("C'est un bulletin ANS general\n"); */
		}
    		else
/* verification si presence champ BID d'elements keps satellites */
    		if (strncasecmp(infos,"BID: ",5) == 0) {
			info_ans = 0;
			info_satellite = 0;
/*	printf("C'est un bulletin d'elements KEPS\n"); */
		}
    		else
		{	
/* se caler au debut d'une documentation satellite */
		j = repereOBJ(buf2, buf3, infos);
	 	if (j == 0) {	
			info_satellite = 0;
/* si la ligne ne comporte pas de numero de catalogue alors sauvegarder 'identification' du satellite */
			strcpy(nom_objet, identification);
		}
		else {
/* c'est une ligne numero de catalogue */
			info_satellite = 1;
/* ecrire les deux lignes dans le fichier documentaire de l'objet 
 * et la suite du fichier jusqu'a la ligne =====  */
			if (info_ans == 1) {
				printf("\nSatellite : %s", nom_objet);
/* ne pas écrire de fichier si option par numéro et numéro de catalogue absent */ 
			if (!(inverse == 1 && j == -1))
					cataloguer(fptr, lignes, infos, buf2, inverse);
			}
		}	
	}
	}	/* repereANS */
  } 		/* else */
  } while (!fin); 	/* do fgets */
 } while (!feof(fptr));
/* fermeture fichier ANS */
 fclose(fptr);

#ifdef FRANCAIS
        printf("\n 6%d bulletins WSR lus\n", bulletin_ANS);
#ifdef _LINUX_
#ifdef ACCENTS
        printf(" %6d fichier(s) documentaire(s) satellites crÃ©Ã©s\n", doc_satellites);
	printf(" %6d lignes traitÃ©es\n", lignes);
#else
        printf(" %6d fichier(s) documentaire(s) satellites créés\n", doc_satellites);
        printf("%7ld lignes traitées\n", lignes);
#endif
#endif
#else
	printf("\n %6d WSR bulletins read\n", bulletin_ANS);
	printf(" %6d satellite characteristics file(s) created\n", doc_satellites);
	printf("%7ld lines processed\n", lignes);
#endif
return succes;
} /* Fin de main() */

