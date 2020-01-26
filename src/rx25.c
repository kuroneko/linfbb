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
 * RX25.C : Decodage de trames au format ROSE X25
 *
 * D'apres KD.BAS de F1EBN
 *
 * FC1OAT@F6ABJ - 920820
 */

#include <serv.h>

/******************************************************************************
 * Dump ASCII avec Filtrage des caracteres non affichables
 ******************************************************************************/

static int decode_x25 (uchar *, int, char *);

static void decode_X121 (uchar *, int, char *);
static void decode_digi (uchar *, int, char *);
static void dump_hexa (uchar *, int, char *);

void put_rose (uchar * texte, int attr, int nbcar)
{
	uchar chaine[400];
	uchar *ptr = chaine;

	int decode = 1;				/* A patcher, au cas ou ... */

	if (decode)
	{

		if (nbcar > 256)
			nbcar = 256;

		strcpy (ptr, "X25 : ");
		ptr += 6;

		nbcar = 6 + decode_x25 (texte, nbcar, ptr);
	}
	else
		memcpy (chaine, texte, nbcar);


	put_ui (chaine, attr, nbcar);

}

/******************************************************************************
 * Les Causes
 *****************************************************************************/

static char *ResetCause (int cause)
{
	static char hex_cause[3];

	/* From ISO 8208 page 48 */

	switch (cause)
	{
	case 0:
		return "DTE Originated";
	case 0xC1:
		return "Gateway-detected Procedure Error";
	case 0xC3:
		return "Gateway Congestion";
	case 0xC7:
		return "Gateway Operational";
	default:
		switch (cause & 0x7F)
		{
		case 0x01:
			return "Out Of Order";
		case 0x03:
			return "Remote Procedure Error";
		case 0x05:
			return "Local Procedure Error";
		case 0x07:
			return "Network Congestion";
		case 0x09:
			return "Remote DTE Operational";
		case 0x0F:
			return "Network Operational";
		case 0x11:
			return "Incompatible Destination";
		case 0x1D:
			return "Network Out Of Order";
		default:
			sprintf (hex_cause, "%2.2X", cause);
			return hex_cause;
		}
	}
}

static char *ClearCause (int cause)
{
	static char hex_cause[3];

	/* From ISO 8208 page 43 */

	switch (cause)
	{
	case 0:
		return "DTE Originated";
	case 0xC1:
		return "Gateway-detected Procedure Error";
	case 0xC3:
		return "Gateway Congestion";
	case 0xC7:
		return "Gateway Operational";
	default:
		switch (cause & 0x7F)
		{
		case 0x01:
			return "Number Busy";
		case 0x09:
			return "Out Of Order";
		case 0x11:
			return "Remote Procedure Error";
		case 0x19:
			return "Reverse Charging Acceptance Not Subscribed";
		case 0x21:
			return "Incompatible Destination";
		case 0x29:
			return "Fast Select Acceptance Not Subscribed";
		case 0x39:
			return "Ship Absent";
		case 0x03:
			return "Invalid Facility Requested";
		case 0x0B:
			return "Access Barred";
		case 0x13:
			return "Local Procedure Error";
		case 0x05:
			return "Network Congestion";
		case 0x0D:
			return "Not Obtainable";
		case 0x15:
			return "RPOA Out Of Order";
		default:
			sprintf (hex_cause, "%2.2X", cause);
			return hex_cause;
		}
	}
}


static char *RestartCause (int cause)
{
	static char hex_cause[3];

	/* From ISO 8208 page 50 */

	switch (cause)
	{
	case 0:
		return "DTE Originated";
	case 1:
		return "Local Procedure Error";
	case 3:
		return "Network Congestion";
	case 7:
		return "Network Operational";
	case 0x7F:
		return "Registration/Cancellation Confirmed";
	default:
		sprintf (hex_cause, "%2.2X", cause);
		return hex_cause;
	}
}

/******************************************************************************
 * Decodage de trames X25
 *****************************************************************************/

/* Decodage d'une adresse X.121 */

static void decode_X121 (uchar * data, int lgaddr, char *result)
{
	char bcd;
	int pfo, pfa;

	while (lgaddr > 0)
	{
		pfo = (*data & 0xF0) >> 4;
		pfa = *data & 0x0F;
		bcd = pfo + '0';
		*result++ = bcd;
		if (--lgaddr == 0)
			break;
		bcd = pfa + '0';
		*result++ = bcd;
		lgaddr--;
		data++;
	}

	*result = '\0';
}

/* Dump Hexa */

static void dump_hexa (uchar * data, int l_data, char *result)
{
	int i;

	i = 0;
	while (l_data-- > 0)
	{
		sprintf (result, "%2.2X ", *data++);
		result += 3;
		if (++i == 16)
		{
			*result++ = '\r';
			i = 0;
		}
	}

	*result = '\0';
}

static char *rs_addr(char *addr)
{
	static char str[20];
	
	strncpy(str, addr, 4);
	str[4] = ',';
	strncpy(str+5, addr+4, 6);
	str[11] = '\0';
	
	return str;
}

/* Decodage d'un champ digi */

static void decode_digi (uchar * data, int l_data, char *result)
{
	while (l_data-- > 1)
	{
		*result++ = *data++ >> 1;
	}

	*result++ = '-';
	*result++ = (*data & 0x7F) >> 1;
	*result = 0;
}

/* Decodage d'une suite de champs facilite */

static void decodage_facilite (uchar * data, char *result)
{
	int lgfac, lg, fct, lgdigi, lgaddcall;
	int lgad, lgaddr, lgadind;
	static char digis[10], digid[10], icd[10];
	static char indorig[10], inddest[10];
	static char addstorig[20], addstdest[20];

	lgfac = *data++;
	lg = lgfac;

	digid[0] = digis[0] = '\0';

	while (lg > 0)
	{
		fct = *data++;
		lg--;
		switch (fct)
		{
		case 0:
			/* Marker=0 National Fac ou Marker=15 CCITT */
			data++;
			lg--;
			break;
		case 0x3F:
			/* Utilise si appel par call digi au lieu du call N3 */
			sprintf (result, "\rFacility 3F%2.2X", *data++);
			lg--;
			break;
		case 0x7F:
			/* Nombre aleatoire pour eviter les rebouclages */
			sprintf (result, " NbAlea: %2.2X%2.2X",
					 *data, *(data + 1));
			data += 2;
			lg -= 2;
			break;
		case 0xE9:
			/* Digi destination */
			lgdigi = *data++;
			decode_digi (data, lgdigi, digid);
			data += lgdigi;
			lg -= 1 + lgdigi;
			break;
		case 0xEB:
			/* Digi origine */
			lgdigi = *data++;
			decode_digi (data, lgdigi, digis);
			data += lgdigi;
			lg -= 1 + lgdigi;
			break;
		case 0xC9:
			/* Adresse et indicatif du nodal destination */
		case 0xCB:
			/* Adresse et indicatif du nodal origine */
			lgaddcall = *data++;
			data++;
			dump_hexa (data, 3, icd);
			data += 3;
			lgad = *data++;
			lg -= 6;
			lgaddr = lgad;
			if (fct == 0xCB)
				decode_X121 (data, lgaddr, addstorig);
			else
				decode_X121 (data, lgaddr, addstdest);
			data += (lgad + 1) / 2;
			lg -= (lgad + 1) / 2;
			lgadind = lgaddcall - (lgad + 1) / 2 - 5;
			if (fct == 0xCB)
				strncpy (indorig, data, lgadind);
			else
				strncpy (inddest, data, lgadind);
			data += lgadind;
			lg -= lgadind;
			break;
		default:
			sprintf (result, "\rUnknown Facility Type %2.2X", fct);
			lg = 0;
			break;
		}

		result += strlen (result);
	}

	sprintf (result, "\r%s@%s", indorig, rs_addr(addstorig));
	result += strlen (result);
	if (*digis)
		sprintf (result, " via %s", digis);
	strcat (result, " -> ");
	result += strlen (result);
	sprintf (result, "%s@%s", inddest, rs_addr(addstdest));
	result += strlen (result);
	if (*digid)
		sprintf (result, " via %s", digid);
}

/* Decodage des donnees accompagnant un CALL REQUEST */

static void decode_call_req (uchar * data, int l_data, char *result)
{
	int lgdest, lgorig, lgaddr;

	/* Decodage du champ adresse */

	lgdest = *(data + 3) & 0x0F;
	lgorig = (*(data + 3) & 0xF0) >> 4;
	lgaddr = lgorig + lgdest;
	/* 
	decode_X121 (data + 4 + (lgdest / 2), lgorig, result);
	strcat (result, " -> ");
	decode_X121 (data + 4, lgdest, result + strlen (result));
	*/
	data += 4 + (lgaddr / 2);
	l_data -= 4 + (lgaddr / 2);
	/* result += strlen (result); */

	/* Decodage des champs facilite */

	decodage_facilite (data, result);
}

/* Point d'entree du decodage des trames X25 */

static int decode_x25 (uchar * data, int l_data, char *result)
{
	int tpaq, id_format, r, s, lci, lg;

	char *ptemp = result;


	lci = ((*data & 0x0f) << 4) | *(data + 1);
	id_format = *data;
	tpaq = *(data + 2);
	r = (tpaq & 0xE0) >> 5;
	s = (tpaq & 0x0E) >> 1;

	sprintf (result, "LCI %03d : ", lci);
	result += strlen (result);

	if (id_format & 0x80)
		strcat (result, "Q ");
	if (id_format & 0x40)
		strcat (result, "D ");

	if (tpaq & 1)
	{
		switch (tpaq)
		{
		case 0x0B:
			sprintf (result, "CALL REQUEST ");
			result += strlen (result);
			decode_call_req (data, l_data, result);
			break;
		case 0x0F:
			sprintf (result, "CALL ACCEPTED");
			break;
		case 0x13:
			sprintf (result,
					 "CLEAR REQUEST - Cause %s - Diag %d",
					 ClearCause (*(data + 3)), *(data + 4));
			break;
		case 0x17:
			sprintf (result, "CLEAR CONFIRMATION");
			break;
		case 0x23:
			sprintf (result, "INTERRUPT");
			break;
		case 0x27:
			sprintf (result, "INTERRUPT CONFIRMATION");
			break;
		case 0x1B:
			sprintf (result,
					 "RESET REQUEST - Cause %s - Diag %d",
					 ResetCause (*(data + 3)), *(data + 4));
			break;
		case 0x1F:
			sprintf (result, "RESET CONFIRMATION");
			break;
		case 0xF3:
			sprintf (result, "REGISTRATION REQUEST");
			break;
		case 0xF7:
			sprintf (result, "REGISTRATION CONFIRMATION");
			break;
		case 0xFB:
			sprintf (result,
					 "RESTART REQUEST - Cause %s - Diag %d",
					 RestartCause (*(data + 3)), *(data + 4));
			break;
		case 0xFF:
			sprintf (result, "RESTART CONFIRMATION");
			break;
		default:
			switch (tpaq & 0x0F)
			{
			case 0x01:
				sprintf (result, "RR R%d", r);
				break;
			case 0x05:
				sprintf (result, "RNR R%d", r);
				break;
			case 0x09:
				sprintf (result, "REJ R%d", r);
				break;
			}
			break;
		}
		l_data = 0;
		lg = strlen (ptemp);
	}
	else
	{
		l_data -= 3;

		if (tpaq & 0x10)
			strcat (result, "M ");
		sprintf (result, "DATA R%d S%d L=%d", r, s, l_data);
		strcat (result, "\r");
		lg = strlen (ptemp);

		result += strlen (result);
		if (l_data < 0)
			l_data = 0;
		memcpy (result, data + 3, l_data);
	}

	return (lg + l_data);
}
