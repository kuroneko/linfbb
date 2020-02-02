/***********************************************************************
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
    along with this program. If not, see <http://www.gnu.org/licenses/>.

    Parts of code have been taken from many other softwares.
    Thanks for the help.
***********************************************************************/


#ifdef __linux__
#define far
#endif

#ifdef __FBBDOS__
extern int _wscroll;
extern int directvideo;
extern int errno;
extern long g_timer;

#endif


/* Variables globales */

#ifndef PUBLIC
#define PUBLIC extern
#endif

#ifndef MAIN
extern tp_ems t_ems[NB_EMS];

#endif

/*-- Variables globales ----------------------------------------------*/

#ifdef __linux__
PUBLIC int t_tell;				/* decompteur : appel du sysop */
PUBLIC int aff_use;				/* Affichage de la memoire disponible */
PUBLIC int time_bcl;			/* Temps entre deux broadcast */
PUBLIC int tempo;				/* decompteur : environ 18 par seconde */
PUBLIC int all_packets;			/* monitoring of all packets */

#endif
#ifdef __WINDOWS__

PUBLIC int t_tell;				/* decompteur : appel du sysop */
PUBLIC int aff_use;				/* Affichage de la memoire disponible */
PUBLIC int time_bcl;			/* Temps entre deux broadcast */
PUBLIC int tempo;				/* decompteur : environ 18 par seconde */
PUBLIC int win_debug;			/* Affichage debug valide */

#endif

#ifdef __FBBDOS__

PUBLIC void (*XMSPtr) (void);	/* Pointeur sur Extended Memory Manager (XMM) */
PUBLIC unsigned XMSErr;			/* Code d'erreur de la derniŠre op‚ration */
PUBLIC XMSRegs Xr;
PUBLIC fen *fen_dos;			/* Sauvegarde fenetre DOS */

extern int far desqview;		/* presence de desqview */
extern int far ton_bip;			/* bip de connexion */
extern int far t_tell;			/* decompteur : appel du sysop */
extern int far kam_timer;		/* Temporisation du KAM */
extern int far tempo;			/* decompteur : environ 18 par seconde */
extern int far time_bcl;		/* Temps entre deux broadcast */
extern int far attcar;			/* Tempo d'attente d'un caractere */
extern int far t_scroll;		/* tempo du scroll */
extern int far aff_use;			/* Affichage de la memoire disponible */

#endif

PUBLIC int editor_request;		/* Ask for editor */
PUBLIC int moto;				/* endian mode INTEL (R) /MOTOROLA (R) */
PUBLIC unsigned tid;			/* Task identifier */
PUBLIC int pactor_scan[NBPORT];	/* Scanning of pactor */
PUBLIC int watchport;			/* Port for watchdog */
PUBLIC int accept_connection;	/* Accepting connections */
PUBLIC int is_idle;				/* No processing is done */
PUBLIC int daemon_mode;			/* Indique un mode sans IHM (Linux) */
PUBLIC int EditorOff;			/* Etat de l'editeur */
PUBLIC int reply;				/* Reponse a un message */
PUBLIC int fast_fwd;			/* Cache forward */
PUBLIC int MAXTACHE;			/* Time_out de tache */
PUBLIC unsigned prot_fwd;		/* Masque des protocoles de fwd */
PUBLIC unsigned w_mask;			/* Masque des messages warning */
PUBLIC int debug_on;			/* Flag de debuggage */
PUBLIC FILE *debug_fptr;		/* Fichier */
PUBLIC char *debug_ptr;			/* Pointeur de debuggage */

PUBLIC long sys_disk;			/* Free KB of main disk */
PUBLIC long tmp_disk;			/* Free KB of temp (BINMAIL) disk */

PUBLIC char mute_unproto;		/* Suppression des sujets prives */
PUBLIC char priv_unproto;		/* Autorisation unproto (P locaux) */
PUBLIC char ack_unproto;		/* Autorisation unproto (ACK) */
PUBLIC char via_unproto;		/* Autorisation unproto (Messages transit) */

PUBLIC unsigned std_header;		/* Type de header utilise */
PUBLIC unsigned BufSeg;			/* Segment reel de la memoire partagee TSR */
PUBLIC unsigned BufSel;			/* Select protege de la memoire partagee TSR */
PUBLIC char *BufReel;			/* pointeur du segment partage reel/protege */

PUBLIC int miniserv;			/* Acces aux rubriques */
PUBLIC int deflang;				/* Langue par defaut */
PUBLIC int multi_prive;			/* Multi-forwarding des prives */
PUBLIC int mail_ch;				/* Canal de l'import/export */
PUBLIC int def_time_bcl;		/* Temps minimum entre deux listes */
PUBLIC long nb_unproto;			/* Nb de messages en arriere en liste UI */

PUBLIC int dde_wp_serv;			/* demande de traitement du serveur WP */
PUBLIC int nb_ovr;				/* Nombre de pages de la gestion d'overlay */
PUBLIC int stop_min;			/* Minute de la demande d'arret */

PUBLIC int max_yapp;			/* Maximum de transfert autorise en YAPP */
PUBLIC int max_mod;				/* Maximum de transfert autorise en MODEM */

PUBLIC int num_semaine;			/* Numero de la semaine durant la session */
PUBLIC unsigned in_exms;		/* Donnees chargees en EMS-XMS */
PUBLIC char admin[8];			/* Indicatif de l'administrateur */
PUBLIC Msysop *mess_sysop;		/* Indicatifs des messages sysop  */

PUBLIC int FOND_VOIE, INIT, CONS, DEF, INDIC, VOIE;
PUBLIC int SEND, RECV, UI, HEADER, STA, TOUR;

PUBLIC int MWARNING;;			/* Voie des messages warning */
PUBLIC unsigned long mem_alloue;	/* Taille de la memoire allouee */
PUBLIC unsigned long tot_mem;	/* Taille du bloc disponible */

PUBLIC unsigned BLK_TO;			/* Time out blankscreen */
PUBLIC unsigned blank;			/* Compteur blankscreen */

PUBLIC int video_off;			/* Validation ecran */
PUBLIC int test_fichiers;		/* Demande le test des fichiers systeme */
PUBLIC int console;				/* Presence de la console */
PUBLIC int canaff;				/* Canal a d'afficher */
PUBLIC int winlig;				/* Nombre de lignes de la fenetre */
PUBLIC int editor;				/* Console en mode editeur */
PUBLIC int ems_aut;				/* Autorise l'utilisation d'EMS */
PUBLIC int internal_int;		/* Interruption interne du drsi */
PUBLIC int tf_int;				/* Interruption interne du tfpcx */
PUBLIC int bpq_deconnect;		/* Mode de deconnexion du BPQ */
PUBLIC int test_message;		/* Procedure de test message */
PUBLIC int NBVOIES;				/* Nombre de voies allouees */
PUBLIC int balbul;				/* Indicatifs bulletins dans la balise */
PUBLIC unsigned def_mask;		/* Masque par defaut a la 1ere connexion */
PUBLIC int niveau;				/* Niveau de test dans forward */
PUBLIC int comlog;				/* Log complet */
PUBLIC int msg_cons;			/* Nb de messages nouveaux pour la console */
PUBLIC int hold_cons;			/* Nb de messages retenus pour la console */
PUBLIC int ch_fen;				/* Modification de la position des fenetres */
PUBLIC int nb_hold;				/* Nombre de messages "held" */
PUBLIC int direct;				/* Type d'acces a l'ecran */
PUBLIC int backscroll;			/* Ecran en backscroll */
PUBLIC int doubl;				/* Nb de fenetres possibles actives */
PUBLIC int cnf_prec;			/* Voie du dernier qui a parle */
PUBLIC int type_sortie;			/* Fin du programme (0 = Arret 1 = reboot) */
PUBLIC int save_fic;			/* Demande de fin de programme */
PUBLIC int ptype;				/* Type de paquet recu en protocole Yapp */
PUBLIC int nb_trait;			/* Nb de caracteres a traiter */
PUBLIC int aut_ui;				/* Autorise la lecture des ui */
PUBLIC int maxbbid;				/* Nombre de BIDs en fichier */
PUBLIC int d_blanc;				/* Dernier blanc rencontre dans la ligne */
PUBLIC int DEBUG;				/* TRUE = pas d'entrees sorties vers TNC2 */
PUBLIC int fbb_fwd;				/* Flag autorisation forward type FBB */
PUBLIC int bin_fwd;				/* Flag autorisation forward binaire */
PUBLIC int print;				/* TRUE = imprimante valide */
PUBLIC int maxlang;				/* Nombre de langues disponibles */
PUBLIC int NBLANG;				/* Nombre de langues statiques */
PUBLIC int lastaff;				/* Derniere voie affichee */
PUBLIC int vlang;				/* Numero buffer utilisee sur la voie en traitement */
PUBLIC int nlang;				/* Langue utilisee sur la voie en traitement */
PUBLIC int time_n;				/* Time_out pour user normal */
PUBLIC int time_b;				/* Time_out pour forward */

PUBLIC int nb_error;			/* Nbre d'erreurs TNC */
PUBLIC int hour_time;			/* Indicateur de l'heure */
PUBLIC int new_om;				/* premiere connexion */
PUBLIC int temp_sec;			/* Cadence de la seconde (19 tempo) */
PUBLIC int v_tell;				/* voie connectee avec le sysop. -  0 : libre */
PUBLIC int son;					/* duree de la tonalite d'appel */
PUBLIC int operationnel;		/* Phase d'initialisations */
PUBLIC int snd_io;				/* TRUE = en entree-sortie */
PUBLIC int port;				/* Numero du port RS232 en traitement */

PUBLIC short bip;				/* Vrai si bip de connexion */
PUBLIC short ok_tell;			/* autorisation de l'appel */
PUBLIC short ok_aff;			/* affichage de l'indicatif */
PUBLIC short separe;			/* position de la separation des fenetres */
PUBLIC short doub_fen;			/* mode de visualisation des fenetres */
PUBLIC short gate;				/* Autorisation d'utilisation du gateway */
PUBLIC short just;				/* Justification de la console */
PUBLIC short sed;				/* Usage du fullscreen editor */
PUBLIC short aff_inexport;		/* Affichage du canal import/export */
PUBLIC short aff_popsmtp;		/* Affichage du port POP/SMTP */
PUBLIC ushort p_forward;		/* pointeur de mise a jour du forward */

PUBLIC int voiecur;				/* voie courante */

PUBLIC int af_voie[NBLIG];		/* position de la ligne d'affichage */
PUBLIC int v_aff;				/* Numero de la voie a afficher */

PUBLIC int let_prec;			/* Etat d'affichage du status */

PUBLIC int arret;				/* etat du serveur */

PUBLIC int stat_fwd;			/* status de forwarding */

PUBLIC int trait_time;			/* temps de traitement */
PUBLIC int t_balise[NBPORT];	/* temporisation balise */
PUBLIC int cmd_fct;				/* touche de fonction demandee */

PUBLIC int inexport;			/* Import-export de fichiers */
PUBLIC int EGA;					/* Type de carte video utilise */
PUBLIC int h_screen;			/* Hauteur de l'ecran */
PUBLIC int h_maint;				/* Heure de maintenance */
PUBLIC int stype;				/* Type d'ecran demande */
PUBLIC int max_indic;			/* Nombre d'indicatifs dans la balise */
PUBLIC unsigned rinfo;			/* nb. enr. INF.DAT */
PUBLIC long nb_jour_val;		/* nb de jours pour bulletin valide */
PUBLIC long nomess;				/* no du dernier message */
PUBLIC long nbmess;				/* nb de messages en instance */

PUBLIC long t_appel;			/* date de mise a jour de OPTIONS.SYS */
PUBLIC long t_bbs;				/* date de mise a jour de BBS.SYS */
PUBLIC long t_rej;				/* date de mise a jour de REJET.SYS */
PUBLIC long t_swap;				/* date de mise a jour de SWAPP.SYS */
PUBLIC long t_thm;				/* date de mise a jour de THEMES.SYS */
PUBLIC long *time_include;		/* date de mise a jour de FORWARD.SYS */
PUBLIC int include_size;		/* Longueur de la table FORWARD.SYS */
PUBLIC unsigned d_droits;		/* droits de tous */
PUBLIC unsigned ds_droits;		/* droits sysop */
PUBLIC unsigned dss_droits;		/* droits sysop + cmde SYS */
PUBLIC long timeprec;			/* Test des temps */
PUBLIC long stemps[NBRUB];		/* Temps d'occupation par rubrique */
PUBLIC char wp_line[258];		/* Ligne d'envoi des messages White Pages */
PUBLIC char msg_header[257];	/* Header du message */

PUBLIC char *indd;				/* index du tableau data */
PUBLIC char mycall[10];			/* indicatif du serveur */
PUBLIC char mypath[40];			/* path du serveur */
PUBLIC int myssid;				/* SSID du serveur */
PUBLIC char txtfwd[52];			/* texte entete forwarding */

PUBLIC char qra_locator[9];		/* Qra locator du serveur */

PUBLIC char *varx[10];			/* Variables de calcul */
PUBLIC char DATADIR[80];		/* Path des fichiers donnees du serveur */
PUBLIC char CONFDIR[80];		/* Path des fichiers conf du serveur */
PUBLIC char MESSDIR[80];		/* Path des fichiers message du serveur */
PUBLIC char MBINDIR[82];		/* Path des fichiers message binaires du serveur */
PUBLIC char PATH[NB_PATH][80];	/* Path des utilisateurs DOS */
PUBLIC char YAPPDIR[82];		/* Path des utilisateurs YAPP */
PUBLIC char DOCSDIR[82];		/* Path des Fichiers DOC */
PUBLIC char PGDIR[82];			/* Path des Programmes PG */
PUBLIC char FILTDIR[82];		/* Path des Programmes filtre */
PUBLIC char SERVDIR[82];		/* Path des Programmes serveur */
PUBLIC char TOOLDIR[82];		/* Path des Outils FWD, CRON, etc... */
PUBLIC char MAILIN[82];			/* Fichier courrier entrant */
PUBLIC char LOCK_IN[82];		/* Fichier de verrou du courrier entrant */

PUBLIC char BBS_UP[80];			/* Script BBS_UP */
PUBLIC char BBS_DW[80];			/* Script BBS_DW */
PUBLIC char my_call[7];			/* Indicatif du Sysop */
PUBLIC char my_name[80];		/* Prenom du Sysop */
PUBLIC char my_city[20];		/* Ville domicile */
PUBLIC char my_zip[10];			/* Zip du serveur */
PUBLIC char data[DATABUF + 1];	/* buffer de donnees recues */
PUBLIC char io_fich[257];		/* Nom du fichier import-export */
PUBLIC char pop_host[41];		/* Hostname for POP session */
PUBLIC char *nomlang;			/* Nom des fichiers langue */
PUBLIC tlang **langue;

PUBLIC char Oui;				/* Caractere 'O' */
PUBLIC char Non;				/* Caractere 'N' */

PUBLIC indicat cons_call;		/* Indicatif de la console */

PUBLIC unsigned fwd_size;		/* Taille du buffer forward */
PUBLIC char *fwd_file;			/* Pointeur du buffer forward */
PUBLIC char *fwd_scan;			/* Pointeur courant du buffer forward */

PUBLIC unsigned rej_size;		/* Taille du buffer rejet */
PUBLIC char *rej_file;			/* Pointeur du buffer rejet */
PUBLIC char *rej_scan;			/* Pointeur courant du buffer rejet */

PUBLIC unsigned swap_size;		/* Taille du buffer swap */
PUBLIC char *swap_file;			/* Pointeur du buffer swap */
PUBLIC char *swap_scan;			/* Pointeur courant du buffer swap */

PUBLIC bloc_indic *racine;		/* debut de la liste d'indicatifs */
PUBLIC bloc_mess *tete_dir;		/* tete de la liste de directory */
PUBLIC iliste t_iliste;			/* tete de la liste des indicatifs de messages */
PUBLIC iliste *p_iliste;		/* pointeur sur la liste des indic_messages */

PUBLIC pglist *tete_pg;			/* Tete de la liste des pg */
PUBLIC serlist *tete_serv;		/* Tete de la liste des serveurs */
PUBLIC lfwd *tete_fwd;			/* tete de la liste forward */
PUBLIC ind_noeud def_cur;		/* Noeud null */

PUBLIC int com_error;
PUBLIC int old_com_error;

PUBLIC defport *p_port;			/* parametres des ports */
PUBLIC defcom *p_com;			/* parametres des COMs */

PUBLIC int time_att[MAXVOIES + 1];	/* Time out des voies */
PUBLIC int time_yapp[MAXVOIES + 1];		/* Time out YAPP */

PUBLIC Hroute *throute;			/* Tete de la liste des maj */
PUBLIC Broute *tbroute;			/* Tete de la liste des blocs */
PUBLIC int h_ok;				/* Validation HRoutes */
PUBLIC int info_ok;				/* Demande obligatoire d'infos */

PUBLIC FScreen winbuf;
PUBLIC FScreen conbuf;
PUBLIC FScreen monbuf;

PUBLIC Svoie *svoie[TOTVOIES];	/* Structures des voies */
PUBLIC Svoie *pvoie;			/* Pointeur de la voie courante */
PUBLIC bullist *ptmes;			/* Pointeur du message courant */

PUBLIC FILE *log_ptr;			/* File ptr du log */
PUBLIC cbuf buf_kb;				/* buffer clavier */
PUBLIC cbuf buf_md;				/* buffer modem */

PUBLIC char *bid_ptr;			/* pointeur du tableau BIDs */
PUBLIC char *bbs_ptr;			/* pointeur du tableau BBSs */

PUBLIC FILE *file_prn;			/* Fichier d'impression */
