   /****************************************************************
    Copyright (C) 1986-2000 by

    F6FBB - Jean-Paul ROUBELAT
    6, rue George Sand
    31120 - Roquettes - France
	jpr@f6fbb.org

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Parts of code have been taken from many other softwares.
    Thanks for the help.
    ****************************************************************/

/*
 *    MODULE REDIST.C    Server from G7FCI
 *
 *
 * REDIST.C - Allows SP messages sent to the server to be redistributed
 *            as bulletins to the relevant local bulletin grouping.
 *
 * The intention of this server is to allow users to send local, regional or
 * national bulletins in areas, regions or countries that are remote from
 * them.  For example, someone could send a bulletin to all BBSs in an
 * area local to a BBS in Los Angeles, even though they don't know the
 * relevant bulletin grouping for that area.  Messages are also sent as
 * SP, so they don't leave a trail of useless bulletins at all the BBSs on
 * the way.
 *
 * The server also has the side effect that allows TNC PMS users to send
 * personal messages which will be translated into bulletins.  This can also
 * be used by users who can't remember the relevant groupings for the bulletin
 * groupings.
 *
 * Three bulletin groupings are allowed, ie LOCAL, REGION and
 * NATION.  The translation for each grouping is held in the
 * file SYSTEM\REDIST.SYS. This file contains one line for each
 * translation.  For example, the file at GB7FCI would contain
 * the following three lines :
 *
 *      #NW
 *      #ZONEA
 *      GBR
 *
 * With the above file, the server will translate a message in the following
 * format :
 *
 *  SP LOCAL @ GB7FCI < G3UVQ
 *  Subject line
 *  R:..............
 *  R:..............
 *
 *  Lines of message text
 *  /EX
 *
 * ...into a bulletin with the format :
 *
 *  SB ALL @ #NW < G3UVQ
 *  Subject line
 *  R:.............
 *
 *  Lines of message text
 *  /EX
 *
 * The subject line and full text are copied.
 *
 * The copied message is appended onto the MAIL.IN BBS file.
 *
 * Next 2 lines remove exception handling */


#include <serv.h>


#define BBS_CALL 1				/* Offset of BBS Call in INIT.SRV */
#define BBS_QRA  3				/* Offset of QRA line in INIT.SRV */
#define BBS_QTH  4				/* Offset of QTH line in INIT.SRV */
#define HOST_SYS 12				/* Offset of SYSOP callsign in INIT.SRV */
#define MAIL_IN  14				/* Offset of MAIL.IN path in INIT.SRV */
#define LINE 128				/* line 'buffer' length */
#define AT_FLD 32				/* Length of fields to hold addresses */
#define DESC_FLD 32				/* Length of a description field */
#define TO_FLD 7				/* Length of TO field */

/* Global variables */

char locbbs_addr[LINE] = "";
char locbbs_desc[DESC_FLD] = "";
char local_addr[LINE] = "";
char local_desc[DESC_FLD] = "";
char region_addr[LINE] = "";
char region_desc[DESC_FLD] = "";
char nation_addr[LINE] = "";
char nation_desc[DESC_FLD] = "";
char default_to[TO_FLD] = "";

int mail_sysop = 1;				/* Does the sysop need to be mailed? */
int redist_update = 0;			/* Infos got from config file */

FILE *mail_file;


/*
 * Copy string 2 into string 1 removing spaces
 * Copy until NULL in str2 or length reached.
 */

void tidy_string (char *str1, char *str2, int length)
{
	char *ptr1, *ptr2;

	int cntr = 0;

	ptr1 = str1;
	ptr2 = str2;
	while (*ptr2 != '\0' && cntr < length)
	{
		if (*ptr2 == ' ' || *ptr2 == '=')
		{
			ptr2++;
		}
		else
		{
			*ptr1 = *ptr2;
			ptr1++;
			ptr2++;
			cntr++;
		}
	}
}

/*
 * Read values from config file into global variables.
 */

int read_config (void)
{
	char buffer[LINE];
	char *string_id;
	char *ptr;
	char str_val[LINE];
	char tmp_str[2];
	FILE *config;

	if (redist_update)
		return (1);

	config = fopen (c_disque ("REDIST.SYS"), "rt");
	if (config == NULL)
		return (0);

	while (!feof (config) && !ferror (config))
	{
		fgets (buffer, LINE, config);

		string_id = strtok (buffer, " =\n");

		if (*string_id == '#')
			continue;

		ptr = strtok (NULL, "\n");
		if (ptr)
			strcpy (str_val, ptr);

		if (strcmpi (string_id, "LOCBBS") == 0)
		{
			tidy_string (locbbs_addr, str_val, LINE);
			locbbs_addr[LINE - 1] = '\0';
		}
		if (strcmpi (string_id, "LOCBBS_DESC") == 0)
		{
			strncpy (locbbs_desc, str_val, DESC_FLD);
			locbbs_desc[DESC_FLD - 1] = '\0';
		}
		if (strcmpi (string_id, "LOCAL") == 0)
		{
			tidy_string (local_addr, str_val, LINE);
			local_addr[LINE - 1] = '\0';
		}
		if (strcmpi (string_id, "LOCAL_DESC") == 0)
		{
			strncpy (local_desc, str_val, DESC_FLD);
			local_desc[DESC_FLD - 1] = '\0';
		}
		if (strcmpi (string_id, "REGION") == 0)
		{
			tidy_string (region_addr, str_val, LINE);
			region_addr[LINE - 1] = '\0';
		}
		if (strcmpi (string_id, "REGION_DESC") == 0)
		{
			strncpy (region_desc, str_val, DESC_FLD);
			region_desc[DESC_FLD - 1] = '\0';
		}
		if (strcmpi (string_id, "NATION") == 0)
		{
			tidy_string (nation_addr, str_val, LINE);
			nation_addr[LINE - 1] = '\0';
		}
		if (strcmpi (string_id, "NATION_DESC") == 0)
		{
			strncpy (nation_desc, str_val, DESC_FLD);
			nation_desc[DESC_FLD - 1] = '\0';
		}
		if (strcmpi (string_id, "DEFAULT_TO") == 0)
		{
			strncpy (default_to, str_val, TO_FLD);
			default_to[TO_FLD - 1] = '\0';
		}
		/*
		   if (strcmpi(string_id,"MAIL_IN") == 0)
		   {
		   strncpy(mail_in,str_val,LINE);
		   mail_in[LINE-1] = '\0';
		   }
		 */
		if (strcmpi (string_id, "MAIL_SYSOP") == 0)
		{
			tidy_string (tmp_str, str_val, 1);
			tmp_str[1] = '\0';
			strupr (tmp_str);
			if (tmp_str[0] == 'N')
				mail_sysop = 0;
		}

	}
	fclose (config);
	if (strlen (locbbs_addr) < 1)
	{
		strcpy (locbbs_addr, my_call);
	}

	redist_update = 1;			/* Has an update been sent yet? 0 = no */

	return (1);
}

int redist (char *filename)
{
	FILE *fptr1;
	FILE *temp_file;

	char buffer[LINE] = "";
	char who_from[TO_FLD] = "";
	char what_area[TO_FLD] = "";
	char to_field[TO_FLD] = "";
	char new_area[LINE] = "";
	char area_desc[DESC_FLD] = "";
	char at_field[LINE] = "";
	char from_bbs[LINE] = "";
	char subject_line[LINE] = "";

	char *buffptr;
	char *buffptr2;
	char *ptr;

	int in_header;
	int cntr;


	fptr1 = fopen (filename, "rt");		/* Open the received message */
	if (fptr1 == NULL)
		return (1);

	if (!read_config ())
		return (1);				/* Get the REDIST config values */

	mail_file = fappend (MAILIN, "b");
	if (mail_file == NULL)
	{
		return (1);
	}

	fgets (buffer, LINE, fptr1);	/* Read the command line */
	sscanf (buffer, "%*s %s %*s %s\n",	/* Extract details of area */
			what_area,			/* and who sent the message */
			who_from);


	if (strcmp (what_area, "LOCBBS") == 0)
	{
		strcpy (new_area, locbbs_addr);
		strcpy (area_desc, locbbs_desc);
	}
	else if (strcmp (what_area, "LOCAL") == 0)
	{
		strcpy (new_area, local_addr);
		strcpy (area_desc, local_desc);
	}
	else if (strcmp (what_area, "REGION") == 0)
	{
		strcpy (new_area, region_addr);
		strcpy (area_desc, region_desc);
	}
	else if (strcmp (what_area, "NATION") == 0)
	{
		strcpy (new_area, nation_addr);
		strcpy (area_desc, nation_desc);
	}
	else
	{
		strcpy (new_area, locbbs_addr);
		strcpy (area_desc, locbbs_desc);
	}

	if (strlen (area_desc) < 1)	/* If descriptions not set up */
	{
		strcpy (area_desc, new_area);
	}

	temp_file = fopen ("REDIST.$$$", "wb");
	if (temp_file == NULL)
		return (1);

	/* Append the copies to mail in file */

	fgets (buffer, LINE, fptr1);	/* Get subject line */
	buffer[strlen (buffer) - 1] = '\0';		/* Strip LF at end of line */
	buffptr = buffer;
	if (*buffer == '#')			/* User requested TO field? */
	{
		buffptr++;				/* Skip past # */
		while ((*buffptr) && (isspace (*buffptr)))
			buffptr++;
		cntr = 0;
		buffptr2 = to_field;
		while (isalnum (*buffptr))
		{
			if (cntr < 6)
			{
				*buffptr2 = toupper (*buffptr);
				buffptr2++;
				cntr++;
			}
			buffptr++;
		}
		*buffptr2 = '\0';
		while ((*buffptr) && (isspace (*buffptr)))
			buffptr++;
	}
	buffptr2 = subject_line;
	cntr = 0;
	while (*buffptr != '\0')
	{
		if (cntr == 78)
			break;
		*buffptr2 = *buffptr;
		buffptr++;
		buffptr2++;
		cntr++;
	}
	*buffptr2 = '\0';

	if (strlen (to_field) < 1)
	{
		if (strlen (default_to) > 0)
		{
			strcpy (to_field, default_to);
		}
		else
		{
			strcpy (to_field, "REDIST");
		}
	}

	fprintf (temp_file, "%s\r\n", subject_line);	/* Output subject line */

	in_header = 1;
	fgets (buffer, LINE, fptr1);
	buffer[strlen (buffer) - 1] = '\0';		/* Strip LF at end of line */
	while (!feof (fptr1) && !ferror (fptr1))
	{
		if (in_header == 1)
		{
			if (strncmp (buffer, "R:", 2) == 0)
			{
				strupr (buffer);	/* Capitalize */
				/* Get FROM_BBS */

				buffptr = buffer;	/* Point at buffer */

				buffptr = strchr (buffptr, '@');	/* Find the '@' */

				if (buffptr != NULL)
				{
					buffptr++;
					if (!isalnum (*buffptr))
						buffptr++;	/* Skip past the ':' */

					buffptr = strtok (buffptr, " ");
					if (buffptr)
					{
						ptr = strtok (buffptr, " ");
						if (ptr)
						{
							strcpy (from_bbs, ptr);
						}
					}
				}
				fgets (buffer, LINE, fptr1);
				buffer[strlen (buffer) - 1] = '\0';		/* Strip LF at EOL */
				continue;
			}
			else
				in_header = 0;

			if (strlen (from_bbs) > 0)
			{
				/* Work out the date and time for the new BBS header */
				long temps;
				struct tm *sdate;

				temps = time (NULL);
				sdate = gmtime (&temps);

				fprintf (temp_file,
						 "R:%02d%02d%02d/%02d%02dZ @:%s {REDIST}\r\n",
						 sdate->tm_year % 100, sdate->tm_mon + 1,
						 sdate->tm_mday, sdate->tm_hour,
						 sdate->tm_min, from_bbs);
			}
		}

		fprintf (temp_file, "%s\r\n", buffer);
		fgets (buffer, LINE, fptr1);
		sup_ln (buffer);
	}
	fclose (temp_file);
	fclose (fptr1);

	/*
	 * Make copies of the temp file for each destination in the
	 * new_area variable.
	 */

	ptr = strtok (new_area, ",\r\n");

	while (ptr)
	{
		n_cpy (AT_FLD - 1, at_field, ptr);
		fprintf (mail_file, "#\nSB %s @ %s < %s\r\n", to_field, at_field, who_from);
		fptr1 = fopen ("REDIST.$$$", "rb");
		if (fptr1 == NULL)
			return (1);

		fgets (buffer, LINE, fptr1);
		sup_ln (buffer);
		while (!feof (fptr1) && !ferror (fptr1))
		{
			if (strcmpi (buffer, "/EX") == 0)
			{
				fprintf (mail_file, "\r\n** This bulletin was distributed in your area using a local REDIST server.\r\n");
			}
			fprintf (mail_file, "%s\r\n", buffer);
			fgets (buffer, LINE, fptr1);
			buffer[strlen (buffer) - 1] = '\0';		/* Strip LF at end of line */
		}
		fclose (fptr1);
		ptr = strtok (NULL, ",\r\n");
	}

	/* Report successful distribution to sender */

	if (strlen (from_bbs) > 0)
	{
		fprintf (mail_file, "SP %s @ %s < %s\r\n", who_from, from_bbs, mycall);
	}
	else
	{
		fprintf (mail_file, "SP %s < %s\r\n", who_from, mycall);
	}
	fprintf (mail_file, "Re: %s\r\n\r\n", subject_line);
	fprintf (mail_file, "The %s server at %s has redistributed your message as\r\n",
			 what_area, mycall);
	fprintf (mail_file, "a bulletin covering %s.\r\n", area_desc);
	fprintf (mail_file, "/EX\r\n");

	/* Mail sysop if required */

	if (mail_sysop == 1)
	{
		fprintf (mail_file, "SP %s < REDIST\r\n", admin);
		fprintf (mail_file, "Report\r\n\r\n");
		fprintf (mail_file, "The %s REDIST server has distributed a bulletin from %s.\r\n",
				 what_area, who_from);
		fprintf (mail_file, "/EX\r\n");
	}

	fclose (mail_file);

	remove ("redist.$$$");
	return (0);					/* Tell BBS all is correct */
}
