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
************************************************************************/

/*
 *    MODULE CONFERENCE
 */

#include <serv.h>

static void conferenciers (void);
static void send_conf (char *, int);

int conference (void)
{
	int c, port, voie;

	switch (pvoie->niv2)
	{
	case 0:
		c = toupper (*indd);
		switch (c)
		{
		case 'W':
			conferenciers ();
			retour_mbl ();
			break;
		case '\0':
		case '\r':
			cnf_prec = -1;
			text_conf (T_CNF + 3);
			pvoie->conf = 1;
			texte (T_CNF + 0);
			conferenciers ();
			ch_niv2 (1);
			break;
		default:
			return (1);
			/*
			   sprintf(varx[0], "C%c", c) ;
			   texte(T_ERR + 1) ;
			   retour_mbl() ;
			 */
			/* break ; */
		}
		break;
	case 1:
		if (*indd == '.')
		{
			switch (toupper (*(indd + 1)))
			{
			case 'Q':
				pvoie->conf = 0;
				texte (T_CNF + 4);
				text_conf (T_CNF + 5);
				retour_mbl ();
				break;
			case 'W':
				conferenciers ();
				break;
			case 'D':
				indd += 2;
				if (teste_espace ())
				{
					sup_ln (strupr (indd));
					if (((voie = num_voie (indd)) != -1) &&
						(svoie[voie]->conf))
					{
						deconnexion (voie, 1);
					}
					else
					{
						var_cpy (0, indd);
						texte (T_CNF + 10);
					}
				}
				break;
			case 'C':
				indd += 2;
				while_space ();
				if ((isdigit (*indd)) && (isspace (*(indd + 1))))
				{
					port = *indd - '0';
					++indd;
				}
				else
					port = no_port (voiecur);
				if ((port > 0) &&
					(port < NBPORT) &&
					((p_port[port].moport & 0x10) || (droits (ACCGATE))) &&
					(p_port[port].pvalid))
				{
					if ((p_port[port].typort == TYP_BPQ) ||
						(p_port[port].typort == TYP_MOD) ||
						(p_port[port].typort == TYP_TCP))
					{
						texte (T_ERR + 14);
						break;
					}
					if (teste_espace ())
					{
						if ((voie = ch_voie (port, 0)) > 0)
						{
							/* save_voie = voiecur ; */
							sup_ln (indd);
							indd -= 2;
							*indd = 'C';
							*(indd + 1) = ' ';
							/*                                  cprintf("Envoie <%s>\r\n", indd) ; */
							if (connect_station (voie, 1, strupr (indd)) == 0)
							{
								svoie[voie]->conf = 1;
								svoie[voie]->niv1 = N_CONF;
								svoie[voie]->niv2 = svoie[voie]->niv3 = 0;
								/*                                                                      cprintf("Met %d sur la voie %d\r\n", N_CONF, voie); */
							}
							/* selvoie(save_voie) ; */
						}
						else
							texte (T_GAT + 3);
					}
					else
						texte (T_ERR + 2);
				}
				else
					texte (T_GAT + 7);
				break;
			case 'H':
			case '?':
				pvoie->niv1 = N_MBL;
				out_help ("C");
				pvoie->niv1 = N_CONF;
				break;
			default:
				send_conf (indd, nb_trait);
				break;
			}
		}
		else
			send_conf (indd, nb_trait);
		break;
	}
	return (0);
}


static void conferenciers (void)
{
	int voie, vide = 1;

	texte (T_CNF + 1);
	for (voie = 0; voie < NBVOIES; voie++)
	{
		if ((svoie[voie]->conf) && (svoie[voie]->sta.connect >= 1))
		{
			var_cpy (0, svoie[voie]->sta.indicatif.call);
			itoa ((voie > 0) ? voie - 1 : voie, varx[1], 10);
			texte (T_CNF + 2);
			vide = 0;
		}
	}
	if (vide)
		texte (T_CNF + 7);
}


char *k_var (void)
{
	int voie, vide = 1;
	static char buffer[257];

	*buffer = '\0';
	for (voie = 0; voie < NBVOIES; voie++)
	{
		if ((svoie[voie]->conf) && (svoie[voie]->sta.connect >= 1))
		{
			if (!vide)
				strcat (buffer, ", ");
			strcat (buffer, svoie[voie]->sta.indicatif.call);
			vide = 0;
		}
	}
	if (vide)
		strcpy (buffer, langue[vlang]->plang[T_CNF + 7 - 1]);
	return (buffer);
}


void text_conf (int numero)
{
	int save_voie = voiecur;
	int voie;

	var_cpy (0, pvoie->sta.indicatif.call);
	for (voie = 0; voie < NBVOIES; voie++)
	{
		if ((svoie[voie]->sta.connect) && (voie != save_voie) && (svoie[voie]->conf))
		{
			selvoie (voie);
			texte (numero);
			aff_etat ('E');
			send_buf (voie);
		}
	}
	selvoie (save_voie);
}


static void send_conf (char *txt_conf, int nbcar)
{
	int save_voie = voiecur;
	int voie;

	for (voie = 0; voie < NBVOIES; voie++)
	{
		if ((voie != save_voie) && (svoie[voie]->conf))
		{
			selvoie (voie);
			pvoie->lignes = -1;
			if (cnf_prec != save_voie)
			{
				var_cpy (0, svoie[save_voie]->sta.indicatif.call);
				texte (T_CNF + 6);
			}
			out (txt_conf, nbcar);
			aff_etat ('E');
			send_buf (voie);
		}
	}
	selvoie (save_voie);
	cnf_prec = voiecur;
}
