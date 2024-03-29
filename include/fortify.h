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
#ifndef __FORTIFY_H__
#define __FORTIFY_H__
/*
 * FILE:
 *   fortify.h
 *
 * DESCRIPTION:
 *     Header file for fortify.c - A fortified shell for malloc, realloc, 
 *   calloc & free
 *
 * WRITTEN:
 *   spb 29/4/94
 *
 * VERSION:
 *   1.0 29/4/94
 */
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (*OutputFuncPtr) (char *);

	void *Fortify_malloc (size_t size, char *file, unsigned long line);
	void *Fortify_realloc (void *ptr, size_t new_size, char *file, unsigned long line);
	void *Fortify_calloc (size_t num, size_t size, char *file, unsigned long line);
	void Fortify_free (void *uptr, char *file, unsigned long line);
	char *Fortify_strdup (char *str, char *file, unsigned long line);

	int Fortify_OutputAllMemory (char *file, unsigned long line);
	int Fortify_CheckAllMemory (char *file, unsigned long line);
	int Fortify_CheckPointer (void *uptr, char *file, unsigned long line);
	int Fortify_Disable (char *file, unsigned long line);
	int Fortify_SetMallocFailRate (int Percent);
	int Fortify_EnterScope (char *file, unsigned long line);
	int Fortify_LeaveScope (char *file, unsigned long line);
	int Fortify_DumpAllMemory (int scope, char *file, unsigned long line);
	void *Fortify_m_alloue (size_t size, char *file, unsigned long line);
	void Fortify_m_libere (void *uptr, unsigned int taille, char *file, unsigned long line);

	typedef void (*Fortify_OutputFuncPtr) (const char *);
	Fortify_OutputFuncPtr Fortify_SetOutputFunc (Fortify_OutputFuncPtr Output);

#ifdef __cplusplus
}
#endif

#ifndef __FORTIFY_C__			/* Only define the macros if we're NOT in fortify.c */

#ifdef FORTIFY					/* Add file and line information to the fortify calls */

#define malloc(size)                  Fortify_malloc(size, __FILE__, __LINE__)
#define realloc(ptr,new_size)         Fortify_realloc(ptr, new_size, __FILE__, __LINE__)
#define calloc(num,size)              Fortify_calloc(num, size, __FILE__, __LINE__)
#define free(ptr)                     Fortify_free(ptr, __FILE__, __LINE__)
#define strdup(str)                   Fortify_strdup(str, __FILE__, __LINE__)
#define m_alloue(num)                 Fortify_m_alloue(num, __FILE__, __LINE__)
#define m_libere(ptr, num)            Fortify_m_libere(ptr, num, __FILE__, __LINE__)

#define Fortify_OutputAllMemory()     Fortify_OutputAllMemory(__FILE__, __LINE__)
#define Fortify_CheckAllMemory()      Fortify_CheckAllMemory(__FILE__, __LINE__)
#define Fortify_CheckPointer(ptr)     Fortify_CheckPointer(ptr, __FILE__, __LINE__)
#define Fortify_Disable()             Fortify_Disable(__FILE__, __LINE__)
#define Fortify_EnterScope()          Fortify_EnterScope(__FILE__, __LINE__)
#define Fortify_LeaveScope()          Fortify_LeaveScope(__FILE__, __LINE__)
#define Fortify_DumpAllMemory(s)      Fortify_DumpAllMemory(s,__FILE__, __LINE__)

#else							/* FORTIFY Define the special fortify functions away to nothing */

#define Fortify_OutputAllMemory()     0
#define Fortify_CheckAllMemory()      0
#define Fortify_CheckPointer(ptr)     1
#define Fortify_Disable()             1
#define Fortify_SetOutputFunc()       0
#define Fortify_SetMallocFailRate(p)  0
#define Fortify_EnterScope()          0
#define Fortify_LeaveScope()          0
#define Fortify_DumpAllMemory(s)      0

#endif							/*   FORTIFY     */
#endif							/* __FORTIFY_C__ */
#endif							/* __FORTIFY_H__ */
