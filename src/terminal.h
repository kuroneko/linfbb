/****************************************************************
 * This software is Copyright (C) 1986-1998 by                  *
 *                                                              *
 * F6FBB - Jean-Paul ROUBELAT,  jpr@f6fbb.org                   *
 * 6, rue George Sand                                           *
 * 31120 - Roquettes - France                                   *
 *                                                              *
 * License to copy and use this software is granted for         *
 * non-commercial use provided that it is identified as         *
 *                                                              *
 * "FBB packet-radio BBS software by Jean-Paul ROUBELAT, F6FBB" *
 *                                                              *
 * in all material mentioning or referencing this software      *
 * or this function.                                            *
 *                                                              *
 * These notices must be retained in any copies of any part of  *
 * this documentation and/or software.                          *
 *                                                              *
 * Parts of code have been taken from many other softwares.     *
 * Thanks for the help.                                         *
 ****************************************************************/


void init_terminal(int termmode, char *call);
void end_terminal(void);
int read_terminal(char *buf, int len);
void write_terminal(char *buf, int bytes);
