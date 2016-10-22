/* $Id: doscall.c,v 1.3 2009/08/08 06:49:44 masamic Exp $ */

/*
 * $Log: doscall.c,v $
 * Revision 1.3  2009/08/08 06:49:44  masamic
 * Convert Character Encoding Shifted-JIS to UTF-8.
 *
 * Revision 1.2  2009/08/05 14:44:33  masamic
 * Some Bug fix, and implemented some instruction
 * Following Modification contributed by TRAP.
 *
 * Fixed Bug: In disassemble.c, shift/rotate as{lr},ls{lr},ro{lr} alway show word size.
 * Modify: enable KEYSNS, register behaiviour of sub ea, Dn.
 * Add: Nbcd, Sbcd.
 *
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.14  2000/01/19  03:51:39  yfujii
 * NFILES is debugged.
 *
 * Revision 1.13  2000/01/16  05:38:53  yfujii
 * Function call 'NAMESTS' is debugged.
 *
 * Revision 1.12  2000/01/09  04:24:13  yfujii
 * Func call CURDRV is fixed according to buginfo0002.
 *
 * Revision 1.11  1999/12/23  08:06:27  yfujii
 * Bugs of FILES/NFILES calls are fixed.
 *
 * Revision 1.10  1999/12/07  12:41:55  yfujii
 * *** empty log message ***
 *
 * Revision 1.10  1999/11/29  07:57:04  yfujii
 * Modified time/date retrieving code to be correct.
 *
 * Revision 1.7  1999/10/29  13:44:11  yfujii
 * FGETC function is debugged.
 *
 * Revision 1.6  1999/10/26  12:26:08  yfujii
 * Environment variable function is drasticaly modified.
 *
 * Revision 1.5  1999/10/25  03:23:59  yfujii
 * Some function calls are modified to be correct.
 *
 * Revision 1.4  1999/10/21  04:08:21  yfujii
 * A lot of warnings are removed.
 *
 * Revision 1.3  1999/10/20  06:00:06  yfujii
 * Compile time errors are all removed.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef    MAIN

#include "run68.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if !defined(__GNUC__)
#include <conio.h>
#endif
#if defined(DOSX)
#include <dos.h>
#include <direct.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "ansicolor-w32.h"
#if defined(WIN32)
#include <io.h>
#elif !defined(DOSX)
#include <ctype.h>
#include <unistd.h>
#endif

static long Gets( long );
static long Kflush( short );
static long Ioctrl( short, long );
static long Dup( short );
static long Dup2( short, short );
static long Malloc( long );
static long Mfree( long );
static long Dskfre( short, long );
static long Setblock( long, long );
static long Create( char *, short );
static long Newfile( char *, short );
static long Open( char *, short );
static long Close( short );
static long Fgets( long, short );
static long Read( short, long, long );
static long Write( short, long, long );
static long Delete( char * );
static long Seek( short, long, short );
static long Rename( long, long );
static long Chmod( long, short );
static long Mkdir( long );
static long Rmdir( long );
static long Chdir( long );
static long Curdir( short, char* );
static long Files( long, long, short );
static long Nfiles( long );
static long Filedate( short, long );
static long Getdate( void );
static long Setdate( short );
static long Gettime( int );
static long Settim2( long );
static long Getenv( long, long, long );
static long Namests( long, long );
static long Nameck( long, long );
static long Conctrl( short, long );
static long Keyctrl( short, long );
static void Fnckey( short, long );
static long Intvcg( UShort );
static long Intvcs( UShort, long );
static long Assign( short, long );
static long Getfcb( short );
static long Exec01( long, long, long, int );
static long Exec2( long, long, long );
static long Exec3( long, long, long );
static void Exec4( long );
static void get_jtime( UShort *, UShort *, int );
static long gets2( char *, int );

long Getenv_common(const char *name_p, char *buf_p);

/*
 �@�@�\�FDOSCALL�����s����
 �߂�l�F TRUE = ���s�I��
         FALSE = ���s�p��
 */
int dos_call( UChar code )
{
	char *data_ptr = 0;
	long stack_adr;
	long data;
	long env;
	long buf;
	long len;
	short    srt;
	short    fhdl;
	long c = 0;
	int    i;
#if defined(WIN32)
	DWORD st;
#endif
	if (func_trace_f) {
		printf( "$%06x FUNC(%02X):", pc-2, code);
	}
	stack_adr = ra [ 7 ];
	if ( code >= 0x80 && code <= 0xAF )
	  code -= 0x30;

#ifdef    TRACE
	printf( "trace: DOSCALL  0xFF%02X PC=%06lX\n", code, pc );
#endif

	switch( code ) {
	  case 0x01:    /* GETCHAR */
		if (func_trace_f) {
			printf("%-10s\n", "GETCHAR");
		}
#if defined(WIN32)
		FlushFileBuffers(finfo[ 1 ].fh);
#elif defined(DOSX)
		fflush( stdout );
#endif
		rd [ 0 ] = (_getche() & 0xFF);
		break;
	  case 0x02:    /* PUTCHAR */
		srt = (short)mem_get( stack_adr, S_WORD );
		c = (unsigned char)srt;
		if (func_trace_f) {
			printf("%-10s char='%c'\n", "PUTCHAR", c);
		}
#if defined(WIN32)
		if (GetConsoleMode(finfo[1].fh, &st) != 0) {
			// �񃊃_�C���N�g
			WriteW32( 1, finfo[ 1 ].fh, data_ptr, 1 );
		} else {
			long nwritten;
			/* Win32API */
			WriteFile( finfo[ 1 ].fh, &c, 1,
					   (LPDWORD)&nwritten, NULL);
		}
#elif defined(DOSX)
		_dos_write( fileno(finfo[ 1 ].fh), &c, 1, &drv );
#endif
		rd [ 0 ] = 0;
		break;
	  case 0x06:    /* KBHIT */
		if (func_trace_f) {
			printf("%-10s\n", "KBHIT");
		}
		srt = (short)mem_get( stack_adr, S_WORD );
		srt &= 0xFF;
		if ( srt >= 0xFE ) {
#if defined(WIN32)
			INPUT_RECORD ir;
			DWORD read_len;
			BOOL b;
			rd [ 0 ] = 0;
			b = PeekConsoleInput(finfo [0].fh, &ir, 1, (LPDWORD)&read_len);
			if (read_len == 0) {
				/* Do nothing. */
			} else if(ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown
					  || c == 0x0)
			{
				/* �s�v�ȃC�x���g�͓ǂݎ̂Ă� */
				b = ReadConsoleInput(finfo [0].fh, &ir, 1, (LPDWORD)&read_len);
			} else if ( srt != 0xFE ) {
				b = ReadConsoleInput(finfo [0].fh, &ir, 1, (LPDWORD)&read_len);
				c = ir.Event.KeyEvent.uChar.AsciiChar;
				if ( ini_info.pc98_key == TRUE )
				  c = cnv_key98( c );
				rd [ 0 ] = c;
			}
#elif defined(DOSX)
			rd [ 0 ] = 0;
			if ( kbhit() != 0 ) {
				c = _getch();
				if ( c == 0x00 ) {
					c = _getch();
				} else {
					if ( ini_info.pc98_key == TRUE )
					  c = cnv_key98( c );
				}
				if ( srt == 0xFE )
				  ungetch( c );
				rd [ 0 ] = c;
			}
#else
                        abort();  // FIXME
#endif
		} else {
			putchar( srt );
			rd [ 0 ] = 0;
		}
		break;
	  case 0x07:    /* INKEY */
	  case 0x08:    /* GETC */
		if (func_trace_f) {
			printf("%-10s\n", code == 0x07 ? "INKEY" : "GETC");
		}
#if defined(WIN32)
		FlushFileBuffers(finfo[ 1 ].fh);
#elif defined(DOSX)
		fflush( stdout );
#endif
		c = _getch();
		if ( c == 0x00 ) {
			c = _getch();
			c = 0x1B;
		}
		rd [ 0 ] = c;
		break;
	  case 0x09:    /* PRINT */
		data = mem_get( stack_adr, S_LONG );
		data_ptr = prog_ptr + data;
		len = strlen( data_ptr );
		if (func_trace_f) {
			printf("%-10s str=%s\n", "PRINT", data_ptr);
		}
#if defined(WIN32)
		if (GetConsoleMode(finfo[1].fh, &st) != 0) {
			WriteW32( 1, finfo[ 1 ].fh, data_ptr, len );
		} else {
			long nwritten;
			/* Win32API */
			WriteFile( finfo[ 1 ].fh, data_ptr, len,
					   (LPDWORD)&nwritten, NULL);
		}
#elif defined(DOSX)
		_dos_write( fileno(finfo[ 1 ].fh), data_ptr,
					(unsigned)len, &drv );
#endif
		/* printf( "%s", data_ptr ); */
		rd [ 0 ] = 0;
		break;
	  case 0x0A:     /* GETS */
		buf = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s\n", "GETS");
		}
		rd [ 0 ] = Gets( buf );
		break;
#if defined(WIN32) || defined(DOSX)
		//#elif defined(DOSX)
	  case 0x0B:    /* KEYSNS */
		if (func_trace_f) {
			printf("%-10s\n", "KEYSNS");
		}
		if ( _kbhit() != 0 )
		  rd [ 0 ] = -1;
		else
		  rd [ 0 ] = 0;
		break;
#endif
	  case 0x0C:    /* KFLUSH */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s mode=%d\n", "KFLUSH", srt);
		}
		rd [ 0 ] = Kflush( srt );
		break;
	  case 0x0D:    /* FFLUSH */
		if (func_trace_f) {
			printf("%-10s\n", "FFLUSH");
		}
#if defined(WIN32)
		/* �I�[�v�����̑S�Ẵt�@�C�����t���b�V������B*/
		for( i = 5; i < FILE_MAX; i ++ ) {
			if ( finfo [ i ].fh == NULL)
			  continue;
			FlushFileBuffers(finfo [ i ].fh);
		}
#else
		_flushall();
#endif
		rd [ 0 ] = 0;
		break;
	  case 0x0E:    /* CHGDRV */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s drv=%c:\n", "CHGDRV", srt+'A');
		}
#if defined(WIN32)
		{
			char drv[3];
			BOOL b;
			sprintf(drv, "%c:", srt+'A');
			/* Win32 API */
			b = SetCurrentDirectory(drv);
			if (b) {
				/* When succeeded. */
				rd [ 0 ] = srt;
			}
		}
#elif defined(DOSX)
		srt += 1;
		dos_setdrive( srt, &drv );
		rd [ 0 ] = drv;
#endif
		// Verify
#if defined(WIN32)
		{
			char drv[512];
			BOOL b;
			b = GetCurrentDirectoryA(sizeof(drv), drv);
			if (b && strlen(drv) != 0 && (drv[0] - 'A') == rd[0]) {
				/* OK, nothing to do. */
			} else {
				rd [ 0 ] = -15;    /* �h���C�u�w���� */
			}
		}
#elif defined(DOSX)
		dos_getdrive( &drv );
		srt += 1;
		if ( srt != drv )
		  rd [ 0 ] = -15;
#endif
		break;
	  case 0x0F:    /* DRVCTRL(�������Ȃ�) */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s drv=%c:\n", "DRVCTRL", srt);
		}
		if ( srt > 26 && srt < 256 )
		  rd [ 0 ] = -15;    /* �h���C�u�w���� */
		else
		  rd [ 0 ] = 0x02;    /* READY */
		break;
	  case 0x10:    /* CONSNS */
		if (func_trace_f) {
			printf("%-10s\n", "CONSNS");
		}
		_flushall();
		rd [ 0 ] = -1;
		break;
	  case 0x11:    /* PRNSNS */
	  case 0x12:    /* CINSNS */
	  case 0x13:    /* COUTSNS */
		if (func_trace_f) {
			printf("%-10s\n", code == 0x11 ? "PRNSNS" : code == 0x12 ? "CINSNS" : "COUTSNS");
		}
		_flushall();
		rd [ 0 ] = 0;
		break;
	  case 0x19:    /* CURDRV */
		if (func_trace_f) {
			printf("%-10s\n", "CURDRV");
		}
#if defined(WIN32)
		{
			char path[512];
			BOOL b;
			b = GetCurrentDirectory(sizeof(path), path);
			if (b && strlen(path) != 0) {
				rd [ 0 ] = path[0] - 'A';
			} else {
				rd [ 0 ] = -15;    /* �h���C�u�w���� */
			}
		}
#elif defined(DOSX)
		dos_getdrive( &drv );
		rd [ 0 ] = drv - 1;
#endif
		break;
	  case 0x1B:    /* FGETC */
		if (func_trace_f) {
			printf("%-10s\n", "FGETC");
		}
		fhdl = (short)mem_get( stack_adr, S_WORD );
		if ( finfo [ fhdl ].mode == 1 ) {
			rd [ 0 ] = -1;
		} else {
#if defined(WIN32)
			DWORD read_len;
			BOOL b = FALSE;
			INPUT_RECORD ir;
			if (GetFileType(finfo [ fhdl ].fh) == FILE_TYPE_CHAR) {
				/* �W�����͂̃n���h�����L�����N�^�^�C�v��������AReadConsole�������Ă݂�B*/
				while(TRUE) {
					b = ReadConsoleInput(finfo [ fhdl ].fh, &ir, 1, (LPDWORD)&read_len);
					if (b == FALSE) {
						/* �R���\�[���ł͂Ȃ������B*/
						ReadFile(finfo [ fhdl ].fh, &c, 1, (LPDWORD)&read_len, NULL);
						break;
					}
					if (read_len == 1 && ir.EventType == KEY_EVENT
						&& ir.Event.KeyEvent.bKeyDown) {
						c = ir.Event.KeyEvent.uChar.AsciiChar;
						if (0x01 <= c && c <= 0xff)
						  break;
					}
				}
			} else {
				ReadFile(finfo [ fhdl ].fh, &c, 1, (LPDWORD)&read_len, NULL);
			}
			if (read_len == 0)
				c = EOF;
			rd[0] = c;
#else
			rd [ 0 ] = fgetc( finfo [ fhdl ].fh );
#endif
		}
		break;
	  case 0x1C:    /* FGETS */
		data = mem_get( stack_adr, S_LONG );
		fhdl = (short)mem_get( stack_adr + 4, S_WORD );
		if (func_trace_f) {
			printf("%-10s file_no=%d\n", "FGETS", fhdl);
		}
		rd [ 0 ] = Fgets( data, fhdl );
		break;
	  case 0x1D:    /* FPUTC */
		srt  = (short)mem_get( stack_adr, S_WORD );
		fhdl = (short)mem_get( stack_adr + 2, S_WORD );
		if (func_trace_f) {
			printf("%-10s file_no=%d char=0x%02X\n", "FPUTC", fhdl, srt);
		}
#if defined(WIN32)
		if (GetConsoleMode(finfo[fhdl].fh, &st) != 0 &&
			(fhdl == 1 || fhdl == 2) ) {
			// �񃊃_�C���N�g�ŕW���o�͂��W���G���[�o��
			WriteW32( fhdl, finfo[fhdl].fh, &srt, 1 );
			rd [ 0 ] = 0;
		} else {
			if (WriteFile(finfo [ fhdl ].fh, &srt,
						  1, (LPDWORD)&len, NULL) == FALSE)
			  rd [ 0 ] = 0;
			else
			  rd [ 0 ] = 1;
		}
#else
		if ( fputc( srt, finfo [ fhdl ].fh ) == EOF )
		  rd [ 0 ] = 0;
		else
		  rd [ 0 ] = 1;
#endif
		break;
	  case 0x1E:    /* FPUTS */
		data = mem_get( stack_adr, S_LONG );
		fhdl = (short)mem_get( stack_adr + 4, S_WORD );
		data_ptr = prog_ptr + data;
		if (func_trace_f) {
			printf("%-10s file_no=%d str=\"%s\"\n", "FPUTS", fhdl, data_ptr);
		}
#if defined(WIN32)
		if (GetConsoleMode(finfo[1].fh, &st) != 0 &&
			(fhdl == 1 || fhdl == 2) ) {
			// �񃊃_�C���N�g�ŕW���o�͂��W���G���[�o��
			len = WriteW32( fhdl, finfo [ fhdl ].fh, data_ptr, strlen(data_ptr) );
		} else {
			WriteFile(finfo [ fhdl ].fh, data_ptr,
					  strlen(data_ptr), (LPDWORD)&len, NULL);
		}
		rd[0] = len;
#else
		if ( fprintf( finfo [ fhdl ].fh, "%s", data_ptr ) == -1 )
		  rd [ 0 ] = 0;
		else
		  rd [ 0 ] = strlen( data_ptr );
#endif
		break;
	  case 0x1F:    /* ALLCLOSE */
		if (func_trace_f) {
			printf("%-10s\n", "ALLCLOSE");
		}
		for ( i = 5; i < FILE_MAX; i ++ ) {
			if ( finfo [ i ].fh != NULL )
#if defined(WIN32)
			  CloseHandle(finfo [ i ].fh);
#else
			  fclose(finfo [ i ].fh);
#endif
		}
		rd [ 0 ] = 0;
		break;
	  case 0x20:    /* SUPER */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s data=%d\n", "SUPER", data);
		}
		if ( data == 0 ) {
			/* user -> super */
			if ( SR_S_REF() != 0 ) {
				rd [ 0 ] = -26;
			} else {
				rd [ 0 ] = ra [ 7 ];
				usp = ra [ 7 ];
				SR_S_ON();
			}
		} else {
			/* super -> user */
			ra [ 7 ] = data;
			rd [ 0 ] = 0;
			usp = 0;
			SR_S_OFF();
		}
		break;
	  case 0x21:    /* FNCKEY */
		srt = (short)mem_get( stack_adr, S_WORD );
		buf = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s mode=%d\n", "FNCKEY", srt);
		}
		Fnckey( srt, buf );
		rd [ 0 ] = 0;
		break;
	  case 0x23:    /* CONCTRL */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s mode=%d\n", "CONCTRL", srt);
		}
		rd [ 0 ] = Conctrl( srt, stack_adr + 2 );
		break;
	  case 0x24:    /* KEYCTRL */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s mode=%d\n", "KEYCTRL", srt);
		}
		rd [ 0 ] = Keyctrl( srt, stack_adr + 2 );
		break;
	  case 0x25:    /* INTVCS */
		srt  = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s intno=%d vec=%X\n", "INTVCS", srt, data);
		}
		rd [ 0 ] = Intvcs( srt, data );
		break;
	  case 0x27:    /* GETTIM2 */
		if (func_trace_f) {
			printf("%-10s\n", "GETTIM2");
		}
		rd [ 0 ] = Gettime( 1 );
		break;
	  case 0x28:    /* SETTIM2 */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s %X\n", "SETTIM2", data);
		}
		rd [ 0 ] = Settim2( data );
		break;
	  case 0x29:    /* NAMESTS */
		data = mem_get( stack_adr, S_LONG );
		buf  = mem_get( stack_adr + 4, S_LONG );
		if (func_trace_f) {
			printf("%-10s fname=%s\n", "NAMESTS", prog_ptr+data);
		}
		rd [ 0 ] = Namests( data, buf );
		break;
	  case 0x2A:    /* GETDATE */
		if (func_trace_f) {
			printf("%-10s", "GETDATE");
		}
		rd [ 0 ] = Getdate();
		if (func_trace_f) {
			printf("date=%X\n", rd[0]);
		}
		break;
	  case 0x2B:    /* SETDATE */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s date=%X\n", "SETDATE", srt);
		}
		rd [ 0 ] = Setdate( srt );
		break;
	  case 0x2C:    /* GETTIME */
		if (func_trace_f) {
			printf("%-10s flag=0\n", "GETTIME");
		}
		rd [ 0 ] = Gettime( 0 );
		break;
	  case 0x30:    /* VERNUM */
		if (func_trace_f) {
			printf("%-10s\n", "VERNUM");
		}
		rd [ 0 ] = 0x36380302;
		break;
	  case 0x32:    /* GETDPB */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s: drive=%d. Not supported, returns -1.\n", "GETDPB", srt);
		}
		rd[0] = -1;
		break;
	  case 0x33:    /* BREAKCK */
		if (func_trace_f) {
			printf("%-10s\n", "BREAKCK");
		}
		rd [ 0 ] = 1;
		break;
	  case 0x34:    /* DRVXCHG */
		if (func_trace_f) {
			printf("%-10s\n", "DRVXCHG");
		}
		rd [ 0 ] = -15;    /* �h���C�u�w���� */
		break;
	  case 0x35:    /* INTVCG */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s intno=%d", "INTVCG", srt);
		}
		rd [ 0 ] = Intvcg( srt );
		if (func_trace_f) {
			printf(" vec=%X\n", rd[0]);
		}
		break;
	  case 0x36:    /* DSKFRE */
		srt = (short)mem_get( stack_adr, S_WORD );
		buf = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s drv=%c:\n", "DISKFRE", srt);
		}
		rd [ 0 ] = Dskfre( srt, buf );
		break;
	  case 0x37:    /* NAMECK */
		data = mem_get( stack_adr, S_LONG );
		buf  = mem_get( stack_adr + 4, S_LONG );
		if (func_trace_f) {
			printf("%-10s fname=%s\n", "NAMECK", prog_ptr+data);
		}
		rd [ 0 ] = Nameck( data, buf );
		break;
	  case 0x39:    /* MKDIR */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s dname=%s\n", "MKDIR", prog_ptr+data);
		}
		rd [ 0 ] = Mkdir( data );
		break;
	  case 0x3A:    /* RMDIR */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s dname=%s\n", "RMDIR", prog_ptr+data);
		}
		rd [ 0 ] = Rmdir( data );
		break;
	  case 0x3B:    /* CHDIR */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s dname=%s\n", "CHDIR", prog_ptr+data);
		}
		rd [ 0 ] = Chdir( data );
		break;
	  case 0x3C:    /* CREATE */
		data = mem_get( stack_adr, S_LONG );
		srt  = (short)mem_get( stack_adr + 4, S_WORD );
		data_ptr = prog_ptr + data;
		if (func_trace_f) {
			printf("%-10s fname=%s attr=%d\n", "CREATE", data_ptr, srt);
		}
		rd [ 0 ] = Create( data_ptr, srt );
		break;
	  case 0x3D:    /* OPEN */
		data = mem_get( stack_adr, S_LONG );
		srt  = (short)mem_get( stack_adr + 4, S_WORD );
		data_ptr = prog_ptr + data;
		if (func_trace_f) {
			printf("%-10s fname=%s mode=%d\n", "OPEN", data_ptr, srt);
		}
		rd [ 0 ] = Open( data_ptr, srt );
		break;
	  case 0x3E:    /* CLOSE */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s file_no=%d\n", "CLOSE", srt);
		}
		rd [ 0 ] = Close(srt);
		break;
	  case 0x3F:    /* READ */
		srt  = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		len  = mem_get( stack_adr + 6, S_LONG );
		rd [ 0 ] = Read( srt, data, len );
		if (func_trace_f) {
			char *str = prog_ptr + data;
			printf("%-10s file_no=%d size=%d ret=%d str=", "READ", srt, len, rd[0]);
			for (i = 0; i < (len <= 30 ? len : 30); i ++) {
				if (str[i] == 0)
					break;
				if (str[i] < ' ')
					printf("\\%03o", (unsigned char)str[i]);
				putchar(str[i]);
			}
			if (len > 30)
				printf(" ...(truncated)");
			printf("\n");
		}
		break;
	  case 0x40:    /* WRITE */
		srt  = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		len  = mem_get( stack_adr + 6, S_LONG );
		rd [ 0 ] = Write( srt, data, len );
		if (func_trace_f) {
			char *str = prog_ptr + data;
			printf("%-10s file_no=%d size=%d ret=%d str=", "WRITE", srt, len, rd[0]);
			for (i = 0; i < (len <= 30 ? len : 30); i ++) {
				if (str[i] == 0)
					break;
				if (str[i] < ' ')
					printf("\\%03o", (unsigned char)str[i]);
				putchar(str[i]);
			}
			if (len > 30)
			  printf(" ...(truncated)");
			printf("\n");
		}
		break;
	  case 0x41:    /* DELETE */
		data = mem_get( stack_adr, S_LONG );
		data_ptr = prog_ptr + data;
		if (func_trace_f) {
			printf("%-10s fname=%s\n", "DELETE", data_ptr);
		}
		rd [ 0 ] = Delete( data_ptr );
		break;
	  case 0x42:    /* SEEK */
		fhdl = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		srt  = (short)mem_get( stack_adr + 6, S_WORD );
		rd [ 0 ] = Seek( fhdl, data, srt );
		if (func_trace_f) {
			printf("%-10s file_no=%d offset=%d mode=%d ret=%d\n", "SEEK", fhdl, data, srt, rd[0]);
		}
		break;
	  case 0x43:    /* CHMOD */
		data = mem_get( stack_adr, S_LONG );
		srt  = (short)mem_get( stack_adr + 4, S_WORD );
		if (func_trace_f) {
			printf("%-10s name=%s attr=%02X\n", "CHMOD", prog_ptr+data, srt);
		}
		rd [ 0 ] = Chmod( data, srt );
		break;
	  case 0x44:    /* IOCTRL */
		srt = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s mode=%d stack=%08X\n", "IOCTRL", srt, stack_adr+2);
		}
		rd [ 0 ] = Ioctrl( srt, stack_adr + 2 );
		break;
	  case 0x45:    /* DUP */
		fhdl = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s org-handle=%d\n", "DUP", fhdl);
		}
		rd [ 0 ] = Dup( fhdl );
		break;
	  case 0x46:    /* DUP2 */
		srt  = (short)mem_get( stack_adr, S_WORD );
		fhdl = (short)mem_get( stack_adr + 2, S_WORD );
		if (func_trace_f) {
			printf("%-10s org-handle=%d new-handle=%d\n", "DUP", srt, fhdl);
		}
		rd [ 0 ] = Dup2( srt, fhdl );
		break;
	  case 0x47:    /* CURDIR */
		srt  = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s drv=%c:\n", "CURDIR", srt+'A');
		}
		rd [ 0 ] = Curdir( srt, prog_ptr + data );
		break;
	  case 0x48:    /* MALLOC */
		len  = mem_get( stack_adr , S_LONG );
		if (func_trace_f) {
			printf("%-10s len=%d\n", "MALLOC", len);
		}
		rd [ 0 ] = Malloc( len );
		break;
	  case 0x49:    /* MFREE */
		data = mem_get( stack_adr , S_LONG );
		if (func_trace_f) {
			printf("%-10s addr=%08X\n", "MFREE", data);
		}
		rd [ 0 ] = Mfree( data );
		break;
	  case 0x4A:    /* SETBLOCK */
		data = mem_get( stack_adr, S_LONG );
		len  = mem_get( stack_adr + 4, S_LONG );
		if (func_trace_f) {
			printf("%-10s size=%d\n", "SETBLOCK", len);
		}
		rd [ 0 ] = Setblock( data, len );
		break;
	  case 0x4B:    /* EXEC */
		srt  = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		if ( srt < 4 ) {
			buf  = mem_get( stack_adr + 6, S_LONG );
			len  = mem_get( stack_adr + 10, S_LONG );
		}
		if (func_trace_f) {
			printf("%-10s md=%d cmd=%s\n", "EXEC", srt, prog_ptr+data);
		}
		switch( srt ) {
		  case 0:
			rd [ 0 ] = Exec01( data, buf, len, 0 );
			break;
		  case 1:
			rd [ 0 ] = Exec01( data, buf, len, 1 );
			break;
		  case 2:
			rd [ 0 ] = Exec2( data, buf, len );
			break;
		  case 3:
			rd [ 0 ] = Exec3( data, buf, len );
			break;
		  case 4:
			Exec4( data );
			break;
		  default:
			err68( "DOSCALL EXEC(5)�����s����܂���" );
			return( TRUE );
		}
		break;
	  case 0x4E:    /* FILES */
		buf  = mem_get( stack_adr, S_LONG );
		data = mem_get( stack_adr + 4, S_LONG );
		srt  = (short)mem_get( stack_adr + 8, S_WORD );
		if (func_trace_f) {
			printf("%-10s fname=\"%s\" attr=%02X\n", "FILES", prog_ptr+data, srt);
		}
		rd [ 0 ] = Files( buf, data, srt );
		break;
	  case 0x4F:    /* NFILES */
		buf  = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s\n", "NFILES");
		}
		rd [ 0 ] = Nfiles( buf );
		break;
	  case 0x51:    /* GETPDB */
		rd [ 0 ] = psp [ nest_cnt ] + MB_SIZE;
		if (func_trace_f) {
			printf("%-10s\n", "GETPDB");
		}
		break;
	  case 0x53:    /* GETENV */
		data = mem_get( stack_adr, S_LONG );
		env  = mem_get( stack_adr + 4, S_LONG );
		buf  = mem_get( stack_adr + 8, S_LONG );
		if (func_trace_f) {
			printf("%-10s env=%s\n", "GETENV", prog_ptr+data);
		}
		rd [ 0 ] = Getenv( data, env, buf );
		break;
	  case 0x54:    /* VERIFYG */
		if (func_trace_f) {
			printf("%-10s\n", "VERIFYG");
		}
		rd [ 0 ] = 1;
		break;
	  case 0x56:    /* RENAME */
		data = mem_get( stack_adr, S_LONG );
		buf  = mem_get( stack_adr + 4, S_LONG );
		if (func_trace_f) {
			printf("%-10s old=\"%s\" new=\"%s\"\n", "RENAME", prog_ptr+data, prog_ptr+buf);
		}
		rd [ 0 ] = Rename( data, buf );
		break;
	  case 0x57:    /* FILEDATE */
		fhdl = (short)mem_get( stack_adr, S_WORD );
		data = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s file_no=%d datetime=%X\n", "FILEDATE", fhdl, data);
		}
		rd [ 0 ] = Filedate( fhdl, data );
		break;
	  case 0x58:    /* MALLOC2 */
		len  = mem_get( stack_adr + 2, S_LONG );
		if (func_trace_f) {
			printf("%-10s len=%d\n", "MALLOC2", len);
		}
		rd [ 0 ] = Malloc( len );
		break;
	  case 0x5B:    /* NEWFILE */
		data = mem_get( stack_adr, S_LONG );
		srt  = (short)mem_get( stack_adr + 4, S_WORD );
		data_ptr = prog_ptr + data;
		if (func_trace_f) {
			printf("%-10s name=\"%s\" attr=%d\n", "NEWFILE", data_ptr, srt);
		}
		rd [ 0 ] = Newfile( data_ptr, srt );
		break;
	  case 0x5F:    /* ASSIGN */
		srt  = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s mode=%d", "ASSIGN", srt);
		}
		rd [ 0 ] = Assign( srt, stack_adr + 2 );
		break;
	  case 0x7C:    /* GETFCB */
		fhdl = (short)mem_get( stack_adr, S_WORD );
		if (func_trace_f) {
			printf("%-10s file_no=%d\n", "GETFCB", fhdl);
		}
		rd [ 0 ] = Getfcb( fhdl );
		break;
	  case 0xF6:    /* SUPER_JSR */
		data = mem_get( stack_adr, S_LONG );
		if (func_trace_f) {
			printf("%-10s adr=$%08X\n", "SUPER_JSR", data);
		}
		ra [ 7 ] -= 4;
		mem_set( ra [ 7 ], pc, S_LONG );
		if ( SR_S_REF() == 0 ) {
			superjsr_ret = pc;
			SR_S_ON();
		}
		pc = data;
		break;
	  case 0x4C:    /* EXIT2 */
		srt = (short)mem_get( stack_adr, S_WORD );
	  case 0x00:    /* EXIT */
		if (func_trace_f) {
			printf("%-10s\n", code == 0x4C ? "EXIT2" : "EXIT");
		}
		Mfree( 0 );
		for( i = 5; i < FILE_MAX; i ++ ) {
			if (finfo[i].nest == nest_cnt) {
				if (finfo [i].fh != NULL) {
#if defined(WIN32)
					CloseHandle(finfo [i].fh);
#else
					fclose(finfo [i].fh);
#endif
					finfo[i].fh = NULL;
				}
			}
		}
		if (nest_cnt <= 0) {
			if (code == 0x00)
				rd[0] = 0;
			else
				rd[0] = (UShort)srt;
			return TRUE;
		}
		sr = (short)mem_get( psp [ nest_cnt ] + 0x44, S_WORD );
		Mfree( psp [ nest_cnt ] + MB_SIZE );
		nest_cnt --;
		pc =       nest_pc [ nest_cnt ];
		ra [ 7 ] = nest_sp [ nest_cnt ];
		if ( code == 0x00 )
		  rd [ 0 ] = 0;
		else
		  rd [ 0 ] = (UShort)srt;
		break;
	  case 0x31:    /* KEEPPR */
		len = mem_get( stack_adr, S_LONG );
		srt = (short)mem_get( stack_adr + 4, S_WORD );
		if (func_trace_f) {
			printf("%-10s\n", "KEEPPR");
		}
		Mfree( 0 );
		for( i = 5; i < FILE_MAX; i ++ ) {
			if ( finfo [ i ].nest == nest_cnt ) {
				if ( finfo [ i ].fh != NULL )
#if defined(WIN32)
				  CloseHandle(finfo [ i ].fh);
#else
				  fclose(finfo [ i ].fh);
#endif
			}
		}
		if ( nest_cnt <= 0 )
		  return( TRUE );
		Setblock( psp [ nest_cnt ] + MB_SIZE,
				  len + PSP_SIZE - MB_SIZE );
		mem_set( psp [ nest_cnt ] + 0x04, 0xFF, S_BYTE );
		sr = (short)mem_get( psp [ nest_cnt ] + 0x44, S_WORD );
		nest_cnt --;
		pc =       nest_pc [ nest_cnt ];
		ra [ 7 ] = nest_sp [ nest_cnt ];
		rd [ 0 ] = (UShort)srt;
	  default:
		if (func_trace_f) {
			printf("%-10s code=0xFF%02X\n", "????????", code );
		}
		break;
	}
	return( FALSE );
}

/*
 �@�@�\�F
     DOSCALL GETS�����s����
   �p�����[�^�F
     long  buf    <in>    ���̓o�b�t�@�A�h���X
   �߂�l�F
     long  ���͕�����
 */
static long Gets( long buf )
{
	char    str [ 256 ];
	char    *buf_ptr;
	UChar    max;
	long len;

	buf_ptr = prog_ptr + buf;
	max = (UChar)(buf_ptr[ 0 ]);
	len = gets2( str, max );
	buf_ptr[ 1 ] = (char)len;
	strcpy( &(buf_ptr[ 2 ]), str );
	return( len );
}

/*
 �@�@�\�F
     DOSCALL KFLUSH�����s����
   �p�����[�^�F
     long  buf    <in>    ���[�h
   �߂�l�F
     long  �L�[�R�[�h��
 */
static long Kflush( short mode )
{
	UChar    c;

#if defined(WIN32)
#elif defined(DOSX)
	while( kbhit() != 0 )
	  _getch();
#endif
	switch( mode ) {
	  case 0x01:
		return( _getche() & 0xFF );
	  case 0x07:
	  case 0x08:
		c = _getch();
		if ( c == 0x00 ) {
			c = _getch();
			c = 0x1B;
		}
		return( c );
	  case 0x0A:
		return( 0 );
	  default:
		return( 0 );
	}
}

/*
 �@�@�\�F
     DOSCALL IOCTRL�����s����
   �p�����[�^�F
     short    mode      <in>    ���[�h
     long  stack_adr <in>    �X�^�b�N�A�h���X
   �߂�l�F
     long  �o�C�g����
 */
static long Ioctrl( short mode, long stack_adr )
{
	short    fno;

	switch( mode ) {
	  case 0:
		fno = (short)mem_get( stack_adr, S_WORD );
		if ( fno == 0 )
		  return( 0x80C1 );
		if ( fno == 1 || fno == 2 )
		  return( 0x80C2 );
		return( 0 );
	  case 6:
		fno = (short)mem_get( stack_adr, S_WORD );
		if ( fno == 0 )
		  return( 0xFF );    /* ���͉� */
		if ( fno < 5 )
		  return( 0 );
		if ( finfo [ fno ].fh == NULL )
		  return( 0 );
		if ( finfo [ fno ].mode == 0 || finfo [ fno ].mode == 2 )
		  return( 0xFF );
		return( 0 );
	  case 7:
		fno = (short)mem_get( stack_adr, S_WORD );
		if ( fno == 1 || fno == 2 )
		  return( 0xFF );    /* �o�͉� */
		if ( fno < 5 )
		  return( 0 );
		if ( finfo [ fno ].fh == NULL )
		  return( 0 );
		if ( finfo [ fno ].mode == 1 || finfo [ fno ].mode == 2 )
		  return( 0xFF );
		return( 0 );
	  default:
		return( 0 );
	}
}

/*
 �@�@�\�F
     DOSCALL DUP�����s����
   �p�����[�^�F
     short    org       <in>    �I���W�i���t�@�C���n���h��?
   �߂�l�F
     long  ���ʐ�̃n���h���܂��̓G���[�R�[�h
 */
static long Dup( short org )
{
	long ret;
	int    i;

	if ( org < 5 )
	  return( -14 );

	ret = 0;
	for ( i = 5; i < FILE_MAX; i++ ) {
		if ( finfo [ i ].fh == NULL ) {
			ret = i;
			break;
		}
	}
	if ( ret == 0 )
	  return( -4 );    /* �I�[�v�����Ă���t�@�C������������ */
	finfo [ ret ].fh   = finfo [ org ].fh;
	finfo [ ret ].date = finfo [ org ].date;
	finfo [ ret ].time = finfo [ org ].time;
	finfo [ ret ].mode = finfo [ org ].mode;
	finfo [ ret ].nest = finfo [ org ].nest;
	strcpy( finfo [ ret ].name, finfo [ org ].name );

	return( ret );
}

/*
 �@�@�\�FDOSCALL DUP2�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Dup2( short org, short new )
{
	if ( new < 5 || org < 5 )
	  return( -14 );

	if ( new >= FILE_MAX )
	  return( -14 );    /* �����ȃp�����[�^ */

	if ( finfo [ new ].fh != NULL ) {
		Close( new );
		if ( Close( new ) < 0 )
		  return( -14 );
	}
	finfo [ new ].fh   = finfo [ org ].fh;
	finfo [ new ].date = finfo [ org ].date;
	finfo [ new ].time = finfo [ org ].time;
	finfo [ new ].mode = finfo [ org ].mode;
	finfo [ new ].nest = finfo [ org ].nest;
	strcpy( finfo [ new ].name, finfo [ org ].name );

	return( 0 );
}

/*
 �@�@�\�F
     DOSCALL MALLOC�����s����
   �p�����[�^�F
     long  size      <in>    �������T�C�Y(�o�C�g)
   �߂�l�F
     long  �������u���b�N�ւ̃|�C���^(>0)
              �G���[�R�[�h(<0)
 */
static long Malloc( long size )
{
	char    *mem_ptr;
	long mem_adr;    /* �������u���b�N�̃A�h���X */
	long mem_end;    /* �������u���b�N�̏I�[�A�h���X */
	long end_adr;    /* �������u���b�N�̈�Ԓ�ʂ̃A�h���X */
	long next_adr;    /* ���̃������u���b�N�̃A�h���X */
	long data;

	mem_adr  = psp [ nest_cnt ];
	end_adr  = mem_get( mem_adr + 0x08, S_LONG );
	size &= 0xFFFFFF;

	while( (next_adr=mem_get( mem_adr + 0x0C, S_LONG )) != 0 ) {
		/* �������u���b�NID������ */
		data = mem_get( next_adr + 0x04, S_BYTE );
		if ( data != 0x00 && data != 0xFF )
		  return( 0x82000000 );    /* ���S�Ɋm�ۂł��Ȃ� */
		mem_adr = next_adr;
		mem_end = mem_get( mem_adr + 0x08, S_LONG );
		if ( mem_end > end_adr )
		  end_adr = mem_end;
	}

	if ( (end_adr & 0xF) != 0 )
	  end_adr += (16 - (end_adr % 16));

	if ( end_adr + MB_SIZE + size > mem_aloc ) {
		if ( mem_aloc - (end_adr + MB_SIZE) < 0 )
		  return( 0x82000000 );    /* ���S�Ɋm�ۂł��Ȃ� */
		/* �m�ۂł���ő咷 */
		return( 0x81000000 + mem_aloc - (end_adr + MB_SIZE) );
	}

	/* �������Ǘ��u���b�N���쐬 */
	mem_ptr = prog_ptr + end_adr;
	memset( mem_ptr, 0x00, MB_SIZE );
	mem_set( mem_adr + 0x0C, end_adr, S_LONG );
	mem_set( end_adr, mem_adr, S_LONG );
	mem_set( end_adr + 0x04, psp [ nest_cnt ], S_LONG );
	mem_set( end_adr + 0x08, end_adr + MB_SIZE + size, S_LONG );
	return ( end_adr + MB_SIZE );
}

/*
 �@�@�\�F
     DOSCALL MFREE�����s����
   �p�����[�^�F
     long  adr       <in>    �������A�h���X
   �߂�l�F
     long  �G���[�R�[�h(<0)
 */
static long Mfree( long adr )
{
	long prev_adr;
	long next_adr;
	long data;

	if ( adr < 0 )
	  return( -9 );    /* �����ȃ������Ǘ��|�C���^ */

	if ( adr == 0 ) {
		mem_set( psp [ nest_cnt ] + 0x0C, 0, S_LONG );
		return( 0 );
	}

	/* �������u���b�NID������ */
	data = mem_get( adr - MB_SIZE + 0x04, S_BYTE );
	if ( data != 0x00 && data != 0xFF )
	  return( -9 );    /* �����ȃ������Ǘ��|�C���^ */

	/* �O�̃u���b�N�𒲂ׂ� */
	prev_adr = mem_get( adr - MB_SIZE, S_LONG );
	data = mem_get( prev_adr + 0x04, S_BYTE );
	if ( data != 0x00 && data != 0xFF )
	  return( -7 );    /* �������Ǘ��̈悪�󂳂ꂽ */

	/* ���̃u���b�N�𒲂ׂ違�|�C���^�𒣂�ւ��� */
	next_adr = mem_get( adr - MB_SIZE + 0x0C, S_LONG );
	if ( next_adr != 0 ) {
		data = mem_get( next_adr + 0x04, S_BYTE );
		if ( data != 0x00 && data != 0xFF )
		  return( -7 );    /* �������Ǘ��̈悪�󂳂ꂽ */
		mem_set( next_adr, prev_adr, S_LONG );
	}
	mem_set( prev_adr + 0x0C, next_adr, S_LONG );
	return ( 0 );
}

/*
 �@�@�\�F
     DOSCALL DSKFRE�����s����
   �p�����[�^�F
     long  drv       <in>    �h���C�u�ԍ�(0)
     long  buf       <in>    �������A�h���X
   �߂�l�F
     long  �f�B�X�N�̋󂫗e��(�o�C�g>0)
              �G���[�R�[�h(<0)
 */
static long Dskfre( short drv, long buf )
{
	long disksize;
#if defined(WIN32)
	BOOL b;
	unsigned long SectorsPerCluster, BytesPerSector,
	NumberOfFreeClusters, TotalNumberOfClusters;
	b = GetDiskFreeSpaceA(
		NULL,
		(LPDWORD)&SectorsPerCluster,
		(LPDWORD)&BytesPerSector,
		(LPDWORD)&NumberOfFreeClusters,
		(LPDWORD)&TotalNumberOfClusters);
	if (!b)
	  return (-15);
	NumberOfFreeClusters &= 0xFFFF;
	mem_set( buf    , NumberOfFreeClusters, S_WORD );
	TotalNumberOfClusters &= 0xFFFF;
	mem_set( buf + 2, TotalNumberOfClusters, S_WORD );
	SectorsPerCluster &= 0xFFFF;
	mem_set( buf + 4, SectorsPerCluster, S_WORD );
	BytesPerSector &= 0xFFFF;
	mem_set( buf + 6, BytesPerSector, S_WORD );
	disksize = NumberOfFreeClusters *
	  SectorsPerCluster *
		BytesPerSector;
#elif defined(DOSX)
	static buf_save;
	struct    diskfree_t    dspace;
	buf_save = buf;    /* dos_getdiskfree��Dskfre�̈������󂷂��� */
	if ( _dos_getdiskfree( drv, &dspace ) != 0 )
	  return( -15 );        /* �h���C�u�w���� */
	buf = buf_save;
	buf = buf_save;
	dspace.avail_clusters &= 0xFFFF;
	mem_set( buf    , dspace.avail_clusters, S_WORD );
	dspace.total_clusters &= 0xFFFF;
	mem_set( buf + 2, dspace.total_clusters, S_WORD );
	dspace.sectors_per_cluster &= 0xFFFF;
	mem_set( buf + 4, dspace.sectors_per_cluster, S_WORD );
	dspace.bytes_per_sector &= 0xFFFF;
	mem_set( buf + 6, dspace.bytes_per_sector, S_WORD );
	disksize = dspace.avail_clusters *
		dspace.sectors_per_cluster *
		dspace.bytes_per_sector;
#endif
	return disksize;
}

/*
   �@�\�F
     DOSCALL SETBLOCK�����s����
   �p�����[�^�F
     long  adr       <in>    �A�h���X
     long  size      <in>    �T�C�Y
   �߂�l�F
     long  �G���[�R�[�h
 */
static long Setblock( long adr, long size )
{
	long data;
	long tail_adr;
	long near_adr;
	long mem_adr;
	long next_adr;

	if( adr == 0 )
	  adr = psp [ nest_cnt ] + MB_SIZE;

	/* �������u���b�NID������ */
	data = mem_get( adr - 0x0C, S_BYTE );
	if ( data != 0x00 && data != 0xFF )
	  return( -9 );

	/* �T�C�Y������ */
	size &= 0x00FFFFFF;
	tail_adr = mem_get( adr - 0x08, S_LONG );
	data = tail_adr - adr;
	if ( size > data ) {
		near_adr = mem_aloc;
		/* �O�̃u���b�N������ */
		mem_adr = adr - MB_SIZE;
		while( (next_adr=mem_get( mem_adr, S_LONG )) != HUMAN_HEAD ) {
			if ( next_adr >= tail_adr && next_adr < near_adr )
			  near_adr = next_adr;
			mem_adr = next_adr;
		}
		/* ���̃u���b�N������ */
		mem_adr = adr - MB_SIZE;
		while( (next_adr=mem_get( mem_adr + 0x0C, S_LONG )) != 0 ) {
			if ( next_adr >= tail_adr && next_adr < near_adr )
			  near_adr = next_adr;
			mem_adr = next_adr;
		}
		if ( adr + size > near_adr )
		  return( 0x81000000 + near_adr - adr );
	}

	mem_set( adr - 0x08, adr + size, S_LONG );
	return( 0 );
}

/*
   �@�\�F
     DOSCALL CREATE�����s����
   �p�����[�^�F
     long  p         <in>    �t�@�C���p�X��������̃|�C���^
     short    atr       <in>    �t�@�C������
   �߂�l�F
     long  �t�@�C���n���h��(>=0)
              �G���[�R�[�h(<0)
 */
static long Create( char *p, short atr )
{
#if defined(WIN32)
	HANDLE  fp;
#else
	FILE    *fp;
#endif
	long ret;
	long i;
	int    len;

	ret = 0;
	for ( i = 5; i < FILE_MAX; i++ ) {
		if ( finfo [ i ].fh == NULL ) {
			ret = i;
			break;
		}
	}
	if ( ret == 0 )
	  return( -4 );    /* �I�[�v�����Ă���t�@�C������������ */

	/* �t�@�C�������̋󔒂��߂� */
	len = strlen( p );
	for( i = len - 1; i >= 0 && p [ i ] == ' '; i-- )
	  p [ i ] = '\0';

	/* �t�@�C�����̃`�F�b�N */
	if ( (len=strlen( p )) > 88 )
	  return( -13 );    /* �t�@�C�����̎w���� */

	for( i = len - 1; i >= 0 && p [ i ] != '.'; i-- )
	  ;
	if ( i >= 0 ) {
		/* �g���q�����݂��� */
		if ( strlen( &(p [ i ]) ) > 4 )
		  return( -13 );
	}
#if defined(WIN32)
	if ((fp = CreateFile(p, GENERIC_WRITE | GENERIC_READ, 0, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
#else
	if ((fp=fopen( p, "w+b" )) == NULL)
#endif
	  return( -23 );    /* �f�B�X�N�������ς� */
	finfo [ ret ].fh   = fp;
	finfo [ ret ].mode = 2;
	finfo [ ret ].nest = nest_cnt;
	strcpy( finfo [ ret ].name, p );
	return( ret );
}

/*
 �@�@�\�FDOSCALL NEWFILE�����s����
 �߂�l�F�t�@�C���n���h��(���Ȃ�G���[�R�[�h)
 */
static long Newfile( char *p, short atr )
{
#if defined(WIN32)
	HANDLE  fp;
#else
	FILE    *fp;
#endif
	long ret;
	long i;
	long len;

	ret = 0;
	for ( i = 5; i < FILE_MAX; i++ ) {
		if ( finfo [ i ].fh == NULL ) {
			ret = i;
			break;
		}
	}
	if ( ret == 0 )
	  return( -4 );    /* �I�[�v�����Ă���t�@�C������������ */

	/* �t�@�C�������̋󔒂��߂� */
	len = strlen( p );
	for( i = len - 1; i >= 0 && p [ i ] == ' '; i-- )
	  p [ i ] = '\0';

	/* �t�@�C�����̃`�F�b�N */
	if ( (len=strlen( p )) > 88 )
	  return( -13 );    /* �t�@�C�����̎w���� */

	for( i = len - 1; i >= 0 && p [ i ] != '.'; i-- )
	  ;
	if ( i >= 0 ) {
		/* �g���q�����݂��� */
		if ( strlen( &(p [ i ]) ) > 4 )
		  return( -13 );
	}
#if defined(WIN32)
	/*
	 * �uX68000���n���h�u�b�N�v�ɂ��ƁA�t�@�C�������݂���ꍇ�ł�
	 * �V���Ƀt�@�C���𐶐�����Ƃ���̂ŁA�t�@�C���̑��݃`�F�b�N�͕s�v
	 * �ł���B
    if ((fp = CreateFile(p, GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
    {
        CloseHandle(fp);
        return( -80 );
    }
	 */
	if ((fp = CreateFile(p, GENERIC_READ | GENERIC_WRITE, 0, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	  {
		  return( -23 );    /* �f�B�X�N�������ς� */
	  }
#else
	if ((fp=fopen( p, "rb" )) != NULL) {
		fclose( fp );
		return( -80 );    /* ���ɑ��݂��Ă��� */
	}
	if ((fp=fopen( p, "w+b" )) == NULL)
	  return( -23 );    /* �f�B�X�N�������ς� */
#endif

	finfo [ ret ].fh = fp;
	finfo [ ret ].mode = 2;
	finfo [ ret ].nest = nest_cnt;
	strcpy( finfo [ ret ].name, p );
	return( ret );
}

/*
 �@�@�\�FDOSCALL OPEN�����s����
 �߂�l�F�t�@�C���n���h��(���Ȃ�G���[�R�[�h)
 */
static long Open( char *p, short mode )
{
#if defined(WIN32)
	HANDLE fh;
	DWORD md;
#else
	FILE    *fp;
	char    md [ 4 ];
#endif
	int    len;
	long ret;
	long i;

	switch( mode ) {
	  case 0: /* �ǂݍ��݃I�[�v�� */
#if defined(WIN32)
		md = GENERIC_READ;
#else
		strcpy( md, "rb" );
#endif
		break;
	  case 1: /* �������݃I�[�v�� */
#if defined(WIN32)
		if ((fh = CreateFile(p, GENERIC_READ, 0, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		  return -2;
		CloseHandle(fh);
		md = GENERIC_WRITE;
#else
		if ((fp=fopen( p, "rb" )) == NULL)
		  return( -2 );    /* �t�@�C���͌�����Ȃ� */
		fclose( fp );
		strcpy( md, "r+b" );
#endif
		break;
	  case 2: /* �ǂݏ����I�[�v�� */
#if defined(WIN32)
		md = GENERIC_READ | GENERIC_WRITE;
#else
		strcpy( md, "r+b" );
#endif
		break;
	  default:
		return( -12 );        /* �A�N�Z�X���[�h���ُ� */
	}

	/* �t�@�C�������̋󔒂��߂� */
	len = strlen( p );
	for( i = len - 1; i >= 0 && p [ i ] == ' '; i-- )
	  p [ i ] = '\0';

	if ( (len=strlen( p )) > 88 )
	  return( -13 );    /* �t�@�C�����̎w���� */
#if defined(WIN32)
	if ((fh = CreateFile(p, md, 0, NULL,
						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
#else
	if ((fp=fopen( p, md )) == NULL)
#endif
	{
		if ( mode == 1 )
			return( -23 );    /* �f�B�X�N�������ς� */
		else
			return( -2 );    /* �t�@�C���͌�����Ȃ� */
	}

	ret = 0;
	for ( i = 5; i < FILE_MAX; i++ ) {
		if ( finfo [ i ].fh == NULL ) {
			ret = i;
			break;
		}
	}

	if ( ret == 0 ) {
#if defined(WIN32)
		CloseHandle(fh);
#else
		fclose( fp );
#endif
		return( -4 );    /* �I�[�v�����Ă���t�@�C������������ */
	}

#if defined(WIN32)
	finfo [ ret ].fh   = fh;
#else
	finfo [ ret ].fh   = fp;
#endif
	finfo [ ret ].mode = mode;
	finfo [ ret ].nest = nest_cnt;
	strcpy( finfo [ ret ].name, p );
	return( ret );
}

/*
 �@�@�\�FDOSCALL CLOSE�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Close( short hdl )
{
	if ( finfo [ hdl ].fh == NULL )
	  return( -6 );    /* �I�[�v������Ă��Ȃ� */

	if ( hdl <= 4 )
	  return( 0 );

#if defined(WIN32)
	if (CloseHandle(finfo[hdl].fh) == FALSE)
#else
	if ( fclose( finfo [ hdl ].fh ) == EOF )
#endif
	  return( -14 );    /* �����ȃp�����[�^�ŃR�[������ */

	finfo [ hdl ].fh = NULL;
	/* �^�C���X�^���v�ύX */
#if defined(WIN32)
	if ( finfo [ hdl ].date != 0 || finfo [ hdl ].time != 0 ) {
		FILETIME ft0, ft1, ft2;
		HANDLE fh;
		__int64 datetime;

		fh = CreateFileA(finfo [ hdl ].name,
						 GENERIC_WRITE, 0, NULL,
						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		GetFileTime(fh, &ft0, &ft1, &ft2);
		// �b��100nsec�ɕϊ�����B
		datetime = ((__int64)finfo [ hdl ].date*86400L + finfo [ hdl ].time) * 10000000;
		ft2.dwLowDateTime = (unsigned long)(datetime & 0xffffffff);
		ft2.dwHighDateTime = (unsigned long)(datetime >> 32);
		SetFileTime(fh, &ft0, &ft1, &ft2);
		CloseHandle(fh);
		finfo [ hdl ].date = 0;
		finfo [ hdl ].time = 0;
	}
#elif defined(DOSX)
	if ( finfo [ hdl ].date != 0 || finfo [ hdl ].time != 0 ) {
		if ( _dos_open( finfo [ hdl ].name, 1, &dos_fh ) == 0 ) {
			dos_setftime( dos_fh, finfo [ hdl ].date,
						  finfo [ hdl ].time );
			dos_close( dos_fh );
		}
		finfo [ hdl ].date = 0;
		finfo [ hdl ].time = 0;
	}
#endif
	return( 0 );
}

/*
 �@�@�\�FDOSCALL FGETS�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Fgets( long adr, short hdl )
{
	char    buf [ 257 ];
	char    *p;
	size_t  len;
	UChar   max;

	if ( finfo [ hdl ].fh == NULL )
	  return( -6 );    /* �I�[�v������Ă��Ȃ� */

	if ( finfo [ hdl ].mode == 1 )
	  return( -1 );

	max = (unsigned char)mem_get( adr, S_BYTE );
#if defined(WIN32)
	{
		BOOL b;
		DWORD read_len;
		char c;
		int i;
		for (i = 0; i < max; i ++) {
			b = ReadFile(finfo [ hdl ].fh, &c, 1, (LPDWORD)&read_len, NULL);
			if (c == '\r') {
				b = ReadFile(finfo [ hdl ].fh, &c, 1, (LPDWORD)&read_len, NULL);
				if (c == 'n') {
					buf[i] = '\0';
					break;
				} else {
					buf[i] = '\r';
				}
			}
		}
		if (b == FALSE)
		  return -1;
	}
#else
	if ( fgets( buf, max, finfo [ hdl ].fh ) == NULL )
	  return( -1 );
#endif
	len = strlen( buf );
	if ( len < 2 )
	  return( -1 );

	len -= 2;
	buf [ len ] = '\0';
	mem_set( adr + 1, len, S_BYTE );
	p = prog_ptr + adr + 2;
	strcpy( p, buf );

	return( len );
}

/*
 �@�@�\�FDOSCALL READ�����s����
 �߂�l�F�ǂݍ��񂾃o�C�g��(���Ȃ�G���[�R�[�h)
 */
static long Read( short hdl, long buf, long len )
{
	char    *read_buf;
	long read_len;
	BOOL    ret;

	if ( finfo [ hdl ].fh == NULL )
	  return( -6 );    /* �I�[�v������Ă��Ȃ� */

	if ( finfo [ hdl ].mode == 1 )
	  return( -1 );    /* �����ȃt�@���N�V�����R�[�� */

	if ( len == 0 )
	  return( 0 );

	read_buf = prog_ptr + buf;
#if defined(WIN32)
	ret = ReadFile(finfo [ hdl ].fh, read_buf, len, (LPDWORD)&read_len, NULL);
#else
	read_len = fread( read_buf, 1, len, finfo [ hdl ].fh );
#endif

	return( read_len );
}

/*
 �@�@�\�FDOSCALL WRITE�����s����
 �߂�l�F�������񂾃o�C�g��(���Ȃ�G���[�R�[�h)
 */
static long Write( short hdl, long buf, long len )
{
	char    *write_buf;
	long write_len = 0;
	unsigned len2;

	if ( finfo [ hdl ].fh == NULL )
	  return( -6 );    /* �I�[�v������Ă��Ȃ� */

	if ( len == 0 )
	  return( 0 );

	write_buf = prog_ptr + buf;
#if defined(WIN32)
	WriteFile(finfo[ hdl ].fh, (LPCVOID)write_buf, len, &len2, NULL);
	write_len = len2;
	if (finfo [ hdl ].fh == GetStdHandle(STD_OUTPUT_HANDLE))
	  FlushFileBuffers(finfo [ hdl ].fh);
#elif defined(DOSX)
	if ( len < 65536 ) {
		_dos_write( fileno(finfo[ hdl ].fh), write_buf, (unsigned)len,
					&len2 );
		write_len = len2;
	} else {
		write_len = fwrite( write_buf, 1, len, finfo [ hdl ].fh );
	}
	if (finfo [ hdl ].fh == stdout)
	  fflush( stdout );
#endif

	return( write_len );
}

/*
 �@�@�\�FDOSCALL DELETE�����s����
 �߂�l�F�t�@�C���n���h��(���Ȃ�G���[�R�[�h)
 */
static long Delete( char *p )
{
	int    err_save;
	unsigned int    len;
	int    hdl;
	int    i;

	errno = 0;
	if ( remove( p ) != 0 ) {
		/* �I�[�v�����̃t�@�C���𒲂ׂ� */
		err_save = errno;
		len = strlen( p );
		hdl = 0;
		for( i = 5; i < FILE_MAX; i ++ ) {
			if ( finfo [ i ].fh == NULL || nest_cnt != finfo [ i ].nest )
			  continue;
			if ( len == strlen( finfo [ i ].name ) ) {
				if ( memcmp( p, finfo [ i ].name, len ) == 0 ) {
					hdl = i;
					break;
				}
			}
		}
		if ( len > 0 && hdl > 0 ) {
#if defined(WIN32)
			CloseHandle(finfo [hdl].fh);
#else
			fclose(finfo [hdl].fh);
#endif
			errno = 0;
			if ( remove( p ) != 0 ) {
				if ( errno == ENOENT )
				  return( -2 );    /* �t�@�C�����Ȃ� */
				else
				  return( -13 );    /* �t�@�C�����w���� */
			}
		} else {
			if ( err_save == ENOENT )
			  return( -2 );
			else
			  return( -13 );
		}
	}
	return( 0 );
}

/*
 �@�@�\�FDOSCALL SEEK�����s����
 �߂�l�F�擪����̃I�t�Z�b�g(���Ȃ�G���[�R�[�h)
 */
static long Seek( short hdl, long offset, short mode )
{
	int    sk;
	long ret;

#if defined(WIN32)
	if (finfo [ hdl ].fh == INVALID_HANDLE_VALUE)
	  return( -6 );        /* �I�[�v������Ă��Ȃ� */
	switch( mode ) {
	  case 0:
		sk = FILE_BEGIN;
		break;
	  case 1:
		sk = FILE_CURRENT;
		break;
	  case 2:
		sk = FILE_END;
		break;
	  default:
		return( -14 );    /* �����ȃp�����[�^ */
	}
	if ((ret = SetFilePointer(finfo [ hdl ].fh, offset, NULL, sk )) < 0 )
	  return( -25 );        /* �w��̈ʒu�ɃV�[�N�ł��Ȃ� */
#else
	if ( finfo [ hdl ].fh == NULL )
	  return( -6 );        /* �I�[�v������Ă��Ȃ� */
	switch( mode ) {
	  case 0:
		sk = SEEK_SET;
		break;
	  case 1:
		sk = SEEK_CUR;
		break;
	  case 2:
		sk = SEEK_END;
		break;
	  default:
		return( -14 );    /* �����ȃp�����[�^ */
	}
	if ( fseek( finfo [ hdl ].fh, offset, sk ) != 0 )
	  return( -25 );        /* �w��̈ʒu�ɃV�[�N�ł��Ȃ� */
	ret = ftell( finfo [ hdl ].fh );
#endif
	return( ret );
}

/*
 �@�@�\�FDOSCALL RENAME�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Rename( long old, long new1 )
{
	char    *old_ptr;
	char    *new_ptr;

	old_ptr = prog_ptr + old;
	new_ptr = prog_ptr + new1;
	errno = 0;
	if (rename( old_ptr, new_ptr ) != 0 ) {
		if ( errno == EACCES )
			return( -22 );    /* �t�@�C���������ă��l�[���ł��Ȃ� */
		else
			return( -7 );    /* �t�@�C�����w���� */
	}

	return( 0 );
}

/*
 �@�@�\�FDOSCALL CHMOD�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Chmod( long adr, short atr )
{
	char    *name_ptr;
	unsigned long ret;

	name_ptr = prog_ptr + adr;
	if ( atr == -1 ) {
		/* �ǂݏo�� */
#if defined(WIN32)
		if ((ret = GetFileAttributesA(name_ptr)) == 0xFFFFFFFF)
		  return -2;
#elif defined(DOSX)
		if ( _dos_getfileattr( name_ptr, &ret ) != 0 )
		  return( -2 );        /* �t�@�C�����Ȃ� */
#else
		abort();  // FIXME
#endif
		return( ret );
	} else {
		atr &= 0x3F;
		errno = 0;
#if defined(WIN32)
		if (SetFileAttributesA(name_ptr, atr) == FALSE) {
#elif defined(DOSX)
		if ( _dos_setfileattr( name_ptr, atr ) != 0 ) {
#else
		abort();  // FIXME
                {
#endif
			if ( errno == ENOENT )
			  return( -2 );        /* �t�@�C�����Ȃ� */
			else
			  return( -19 );        /* �������݋֎~ */
		}
		return( atr );
	}
}

/*
 �@�@�\�FDOSCALL MKDIR�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Mkdir( long name )
{
	char    *name_ptr;

	name_ptr = prog_ptr + name;
#if defined(WIN32)
	if (CreateDirectoryA(name_ptr, NULL) == FALSE) {
#else
	if (mkdir(name_ptr, 0777) != 0) {
#endif
		if ( errno == EACCES )
			return( -20 );    /* �f�B���N�g���͊��ɑ��݂��� */
		return( -13 );        /* �t�@�C�����w���� */
	}
	return( 0 );
}

/*
 �@�@�\�FDOSCALL RMDIR�����s����
 �߂�l�F�G���[�R�[�h
	 */
static long Rmdir( long name )
{
	char    *name_ptr;

	name_ptr = prog_ptr + name;
	errno = 0;
#if defined(WIN32)
	if (RemoveDirectoryA(name_ptr) == FALSE) {
#else
	if (rmdir(name_ptr) != 0) {
#endif
		if ( errno == EACCES )
			return( -21 );    /* �f�B���N�g�����Ƀt�@�C�������� */
		return( -13 );        /* �t�@�C�����w���� */
	}
	return( 0 );
}

/*
 �@�@�\�FDOSCALL CHDIR�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Chdir( long name )
{
	char    *name_ptr;

	name_ptr = prog_ptr + name;
#if defined(WIN32)
	if (SetCurrentDirectory(name_ptr) == FALSE )
#else
	if (chdir(name_ptr) != 0 )
#endif
		return( -3 );        /* �f�B���N�g����������Ȃ� */
	return( 0 );
}

/*
   �@�\�F
     DOSCALL CURDIR�����s����
   �߂�l�F
     �G���[�R�[�h
 */
static long Curdir( short drv, char *buf_ptr )
{
	char    str [ 67 ];
	char     *ret_ptr = str; /* NULL�ȊO�Ȃ牽�ł��悢�B*/
#if defined(WIN32)
	char cpath[512], tpath[512];
	char cdrv[3], tdrv[3];
	BOOL b;

	if ( drv != 0 ) { /* �J�����g�h���C�u�łȂ������� */
		/* �܂��A�J�����g�f�B���N�g�����擾���ĕۑ����Ă����B*/
		b = GetCurrentDirectory(sizeof(cpath), cpath);
		sprintf(cdrv, "%c:", cpath[0]);
		/* ���ɁA�J�����g�h���C�u��ύX����B*/
		sprintf(tdrv, "%c:", drv+'A'-1);
		b = SetCurrentDirectory(tdrv);
		if (b == FALSE) {
			/* �h���C�u�̕ύX�Ɏ��s�����B*/
			ret_ptr = NULL;
		}
	}
	if (ret_ptr != NULL) {
		/* �ύX�����h���C�u�̃J�����g�h���C�u���擾����B*/
		b = GetCurrentDirectory(sizeof(tpath), tpath);
	}
	if ( drv != 0 ) { /* �J�����g�h���C�u�łȂ������� */
		/* �Ō�ɁA�J�����g�h���C�u�����ɖ߂��B*/
		b = SetCurrentDirectory(cdrv);
	}
	if (ret_ptr == NULL)
		return( -15 );
	strncpy(str, tpath, sizeof(str)-1);
	str[sizeof(str)-1] = '\0';
	strcpy( buf_ptr, str);
#else
	unsigned getdrv;
	if ( drv != 0 ) {
#if defined(DOSX)
		dos_getdrive( &getdrv );
		dos_setdrive( drv, &dmy );
#elif defined(WIN32)
		dos_getdrive( &getdrv );
		dos_setdrive( drv - 1, &dmy );
#endif
	}

	ret_ptr = getcwd( str, 66 );
	if ( drv != 0 ) {
#if defined(DOSX)
		dos_setdrive( getdrv, &dmy );
#elif defined(WIN32)
		dos_setdrive( getdrv - 1, &dmy );
#endif
		if ( ret_ptr == NULL )
			return( -15 );
		if ( toupper( str[ 0 ] ) != drv - 1 + 'A' )
			return( -15 );        /* �h���C�u���w���� */
	} else {
		if ( ret_ptr == NULL )
			return( -15 );
	}
	strcpy( buf_ptr, &(str[ 3 ]) );
#endif
	return( 0 );
}

/*
   �@�\�F
     DOSCALL FILES�����s����
   �p�����[�^�F
     long  buf       <in>    �t�@�C�������o�b�t�@�̃A�h���X
     long  name      <in>    �t�@�C����(���C���h�J�[�h�܂�)�ւ̃|�C���^
     short    atr       <in>    ����
   �߂�l�F
     �G���[�R�[�h
 */
static long Files( long buf, long name, short atr )
{
#if defined(WIN32)
	WIN32_FIND_DATA f_data;
	HANDLE handle;
	char        *name_ptr;
	char        *buf_ptr;
	name_ptr = prog_ptr + name;
	buf_ptr  = prog_ptr + buf;

	/* �ŏ��Ƀ}�b�`����t�@�C����T���B*/
	/* FindFirstFileEx()��WindowsNT�ɂ����Ȃ��̂Ń{�c
    handle = FindFirstFileEx
                   (name_ptr,
                    FindExInfoStandard,
                    (LPVOID)&f_data,
                    FindExSearchNameMatch,
                    NULL,
                    0);
	 */
	/* �ŏ��̃t�@�C������������B*/
	handle = FindFirstFile(name_ptr, &f_data);
	/* �\��̈���Z�b�g */
	buf_ptr[0] = atr;  /* �t�@�C���̑��� */
	buf_ptr[1] = 0;    /* �h���C�u�ԍ�(not used) */
	*((HANDLE*)&buf_ptr[2]) = handle; /* �T�[�`�n���h�� */
	{
		BOOL b = handle != INVALID_HANDLE_VALUE;
		/* �����̈�v����t�@�C����������܂ŌJ�Ԃ���������B*/
		while(b == TRUE) {
			unsigned char fatr;
			fatr  = f_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0x01 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? 0x02 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? 0x04 : 0;
/*			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_VOLUMEID ? 0x08 : 0; */
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 0x10 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? 0x20 : 0;
			if (fatr & buf_ptr[0] || (fatr == 0 && (buf_ptr[0] & 0x20))) {
				/* ATR���Z�b�g */
				buf_ptr[21] = fatr;
				break;  /* �w�肳�ꂽ�����̃t�@�C�������������B*/
			}
			b = FindNextFile(handle, &f_data);
		}
		if (!b)
			return( -2 );
	}
	/* DATE��TIME���Z�b�g */
	{
		SYSTEMTIME st;
		unsigned short s;
		FileTimeToSystemTime(&f_data.ftLastWriteTime, &st);
		s = (st.wHour << 11) +
			(st.wMinute << 5) +
			 st.wSecond / 2;
		buf_ptr[22] = (s & 0xff00) >> 8;
		buf_ptr[23] = s & 0xff;
		s =((st.wYear - 1980) << 9) +
			(st.wMonth << 5) +
			 st.wDay;
		buf_ptr[24] = (s & 0xff00) >> 8;
		buf_ptr[25] = s & 0xff;
	}
	/* FILELEN���Z�b�g */
	buf_ptr[26] = (unsigned char)((f_data.nFileSizeLow & 0xff000000) >> 24);
	buf_ptr[27] = (unsigned char)((f_data.nFileSizeLow & 0x00ff0000) >> 16);
	buf_ptr[28] = (unsigned char)((f_data.nFileSizeLow & 0x0000ff00) >> 8);
	buf_ptr[29] = (unsigned char)(f_data.nFileSizeLow & 0x000000ff);
	/* PACKEDNAME���Z�b�g */
	strncpy(&buf_ptr[30], f_data.cFileName, 22);
	buf_ptr[30+22] = 0;
#else
	abort();  // FIXME
#endif
	return( 0 );
}

/*
 �@�@�\�FDOSCALL NFILES�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Nfiles( long buf )
{
#if defined(WIN32)
	WIN32_FIND_DATA f_data;
	HANDLE handle;
	unsigned int i;
	char        *buf_ptr;
	short atr;

	buf_ptr = prog_ptr + buf;
	atr = buf_ptr[0]; /* �������ׂ��t�@�C���̑��� */
	{
		/* todo:buf_ptr�̎w���̈悩��K�v�ȏ������o���āAf_data�ɃR�s�[����B*/
		/* DOSX�̏������Q�l�ɂ���B*/
		/* 2�b��100ns�ɕϊ�����B*/
		SYSTEMTIME st;
		BOOL b;
		unsigned short s;

		s = *((unsigned short*)&buf_ptr[24]);
		st.wYear  = ((s & 0xfe00) >> 9) + 1980;
		st.wMonth = (s & 0x01e0) >> 5;
		st.wDay   = (s & 0x1f);
		s = *((unsigned short*)&buf_ptr[22]);
		st.wHour   = (s & 0xf800) >> 11;
		st.wMinute = (s & 0x07e0) >> 5;
		st.wSecond = (s & 0x001f);
		st.wMilliseconds = 0;
		b = SystemTimeToFileTime(&st, &f_data.ftLastWriteTime);
		/* �t�@�C���� */
		/* �t�@�C������buf������o����f_data�ɃR�s�[����K�v�͂Ȃ��Ǝv���B
   ���̕����̃R�[�h�������Ō듮�삵�Ă����ƍl������̂ŁA�����[�X��
   1�����҂��č폜����B  Y.Fujii 2000/1/19
        {
            char *p;
            int i;
            p = (char*)&buf_ptr[30];
            for (i = 0; i < 19; i++)
            {
                if (*p == ' ' || *p == 0)
                {
                    f_data.cFileName[i] = 0;
                    break;
                }
                f_data.cFileName[i] = *p++;
            }
            p = (char*)&buf_ptr[30+19];
            if (*p != ' ')
            {
                strcat(f_data.cFileName, ".");
            }
            for (i = strlen(f_data.cFileName); i < 22; i++)
            {
                if (*p == ' ' || *p == 0)
                {
                    f_data.cFileName[i] = 0;
                    break;
                }
                f_data.cFileName[i] = *p++;
            }
            strncpy(f_data.cFileName, (char*)&buf_ptr[30], 22);
            f_data.cFileName[21] = '\0';
        }
		 */
		f_data.nFileSizeHigh = 0;
		f_data.nFileSizeLow = *((unsigned long*)&buf_ptr[29]);
		/* �t�@�C���̃n���h�����o�b�t�@����擾����B*/
		handle = *((HANDLE*)&buf_ptr[2]);
		b = FindNextFile(handle, &f_data);
		/* �����̈�v����t�@�C����������܂ŌJ�Ԃ���������B*/
		while(b == TRUE) {
			unsigned char fatr;
			fatr  = f_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0x01 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? 0x02 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? 0x04 : 0;
/*			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_VOLUMEID ? 0x08 : 0; */
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? 0x10 : 0;
			fatr |= f_data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? 0x20 : 0;
			if (fatr & buf_ptr[0] || (fatr == 0 && (buf_ptr[0] & 0x20))) {
				/* ATR���Z�b�g */
				buf_ptr[21] = fatr;
				break;  /* �w�肳�ꂽ�����̃t�@�C�������������B*/
			}
			b = FindNextFile(handle, &f_data);
		}
		if (!b) {
			return -2;
		}
	}
	/* buf_ptr�̈�ɔ��������t�@�C���̏����Z�b�g�������B*/
	/* �\��̈���Z�b�g(4�o�C�g�����g��Ȃ��B*/
	*((HANDLE*)&buf_ptr[2]) = handle; /* DIRCLS, DIRFAT�̗̈���g�����B*/
	for (i = 6; i < 21; i ++)
		buf_ptr[i] = 0;
	/* DATE��TIME���Z�b�g */
	{
		SYSTEMTIME st;
		unsigned short s;
		FileTimeToSystemTime(&f_data.ftLastWriteTime, &st);
		s = (st.wHour << 11) +
		  (st.wMinute << 5) +
			st.wSecond / 2;
		buf_ptr[22] = (s & 0xff00) >> 8;
		buf_ptr[23] = s & 0xff;
		s =((st.wYear - 1980) << 9) +
		  (st.wMonth << 5) +
			st.wDay;
		buf_ptr[24] = (s & 0xff00) >> 8;
		buf_ptr[25] = s & 0xff;
	}
	/* FILELEN���Z�b�g */
	buf_ptr[26] = (unsigned char)((f_data.nFileSizeLow & 0xff000000) >> 24);
	buf_ptr[27] = (unsigned char)((f_data.nFileSizeLow & 0x00ff0000) >> 16);
	buf_ptr[28] = (unsigned char)((f_data.nFileSizeLow & 0x0000ff00) >> 8);
	buf_ptr[29] = (unsigned char)(f_data.nFileSizeLow & 0x000000ff);
	/* PACKEDNAME���Z�b�g */
	strncpy(&buf_ptr[30], f_data.cFileName, 22);
	buf_ptr[30+22] = 0;
#else
	abort();  // FIXME
#endif
	return( 0 );
}

/*
 �@�@�\�FDOSCALL FILEDATE�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Filedate( short hdl, long dt )
{
#if defined(WIN32)
	FILETIME ctime, atime, wtime;
	__int64 ll_wtime;
	HANDLE hFile;
	BOOL b;
#elif defined(DOSX)
	int     dosfh;
	unsigned fd;
	unsigned ft;
	UShort     fdate;
	UShort     ftime;
#endif
	if ( finfo [ hdl ].fh == NULL )
		return( -6 );        /* �I�[�v������Ă��Ȃ� */
#if defined(DOSX)
	dosfh = fileno( finfo [ hdl ].fh );
#endif
	if ( dt != 0 ) {    /* �ݒ� */
#if defined(WIN32)
		hFile = finfo [ hdl ].fh;
		GetFileTime(hFile, &ctime, &atime, &wtime);
		ll_wtime = (dt >> 16)*86400*10000000 + (dt & 0xFFFF)*10000000;
		wtime.dwLowDateTime = (DWORD)(ll_wtime & 0xFFFFFFFF);
		wtime.dwHighDateTime = (DWORD)(ll_wtime >> 32);
		b = SetFileTime(hFile, &ctime, &atime, &wtime);
		if (b)
			return( -19 );    /* �������ݕs�� */
		finfo [ hdl ].date = (unsigned long)(ll_wtime / 10000000 / 86400);
		finfo [ hdl ].time = (unsigned long)((ll_wtime / 10000000) % 86400);
#elif defined(DOSX)
		fdate = (unsigned short)(dt >> 16);
		ftime = (unsigned short)(dt & 0xFFFF);
		get_jtime( &fdate, &ftime, -1 );
		fd = fdate;
		ft = ftime;
		if ( dos_setftime( dosfh, fd, ft ) != 0 )
			return( -19 );    /* �������ݕs�� */
		finfo [ hdl ].date = fd;
		finfo [ hdl ].time = ft;
#endif
		return( 0 );
	}

#if defined(WIN32)
	hFile = finfo [ hdl ].fh;
	GetFileTime(hFile, &ctime, &atime, &wtime);
	ll_wtime = (((__int64)wtime.dwLowDateTime) << 32) + (__int64)wtime.dwLowDateTime;
	return (long)(((ll_wtime / 86400 / 10000000) << 16)
					+ (ll_wtime / 10000000) % 86400);
#elif defined(DOSX)
	if ( dos_getftime( dosfh, &fd, &ft ) != 0 )
		return( 0 );

	fdate = fd;
	ftime = ft;
	get_jtime( &fdate, &ftime, 1 );
	return( (fdate << 16) | ftime );
#else
	abort();  // FIXME
#endif
}

/*
 �@�@�\�FDOSCALL GETDATE�����s����
 �߂�l�F���݂̓��t
 */
static long Getdate()
{
	long       ret;

#if defined(WIN32)
	SYSTEMTIME stime;
	//GetSystemTime(&stime);
	GetLocalTime(&stime);
	ret = ((long)(stime.wDayOfWeek) << 16) + (((long)(stime.wYear) - 1980) << 9) +
		((long)(stime.wMonth) << 5) + (long)(stime.wDay);
#elif defined(DOSX)
	struct dos_date_t ddate;
	dos_getdate( &ddate );
	ret = (ddate.dayofweek << 16) + ((ddate.year -1980) << 9) +
		(ddate.month << 5) + ddate.day;
#else
	abort();  // FIXME
#endif
	return( ret );
}

/*
 �@�@�\�FDOSCALL SETDATE�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Setdate( short dt )
{
#if defined(WIN32)
	SYSTEMTIME stime;
	BOOL b;
	stime.wYear  = (dt >> 9) & 0x7F + 1980;
	stime.wMonth = (dt >> 5) & 0xF;
	stime.wDay   = dt & 0x1f;
	stime.wSecond = 0;
	stime.wMilliseconds = 0;
	// b = SetSystemTime(&stime);
	b = SetLocalTime(&stime);
	if (!b)
		return -14;     /* �p�����[�^�s�� */
#elif defined(DOSX)
	struct dos_date_t ddate;

	ddate.year  = ((dt >> 9) & 0x7F) + 1980;
	ddate.month = ((dt >> 5) & 0xF);
	ddate.day   = (dt & 0x1F);

	if ( dos_setdate( &ddate ) != 0 )
		return( -14 );        /* �p�����[�^�s�� */
#else
	abort();  // FIXME
#endif
	return( 0 );
}

/*
 �@�@�\�FDOSCALL GETTIME / GETTIME2�����s����
 �߂�l�F���݂̎���
 */
static long Gettime( int flag )
{
	long       ret;
#if defined(WIN32)
	SYSTEMTIME stime;
	// GetSystemTime(&stime);
	GetLocalTime(&stime);
	if ( flag == 0 )
		// ret = stime.wHour << 11 + stime.wMinute << 5 + stime.wSecond >> 1;
		ret = ((long)(stime.wHour) << 11) + ((long)(stime.wMinute) << 5) + ((long)(stime.wSecond) >> 1);
	else
		// ret = stime.wHour << 16 + stime.wMinute << 8 + stime.wSecond;
		ret = ((long)(stime.wHour) << 16) + ((long)(stime.wMinute) << 8) + (long)(stime.wSecond);
#elif defined(DOSX)
	struct dos_time_t dtime;
	dos_gettime( &dtime );

	if ( flag == 0 )
		ret = (dtime.hour << 11) + (dtime.minute << 5) + (dtime.second >> 1);
	else
		ret = (dtime.hour << 16) + (dtime.minute << 8) + dtime.second;
#else
	abort();  // FIXME
#endif
	return( ret );
}

/*
 �@�@�\�FDOSCALL SETTIM2�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Settim2( long tim )
{
#if defined(WIN32)
	SYSTEMTIME stime;
	BOOL b;
	stime.wYear  = (tim >> 16) & 0x1F;
	stime.wMonth = (tim >> 8) & 0x3F;
	stime.wDay   = tim & 0x3f;
	stime.wSecond = 0;
	stime.wMilliseconds = 0;
	b = SetSystemTime(&stime);
	if (!b)
		return -14;     /* �p�����[�^�s�� */
#elif defined(DOSX)
	struct dos_time_t dtime;

	dtime.hour    = ((tim >> 16) & 0x1F);
	dtime.minute  = ((tim >> 8) & 0x3F);
	dtime.second  = (tim & 0x3F);
	dtime.hsecond = 0;
	if ( dos_settime( &dtime ) != 0 )
		return( -14 );        /* �p�����[�^�s�� */
#else
	abort();  // FIXME
#endif
	return( 0 );
}

/*
 �@�@�\�FDOSCALL GETENV�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Getenv( long name, long env, long buf )
{
	long ret;
	if ( env != 0 )
		return( -10 );
	ret = Getenv_common(prog_ptr + name, prog_ptr + buf);
	return ret;
}

long Getenv_common(const char *name_p, char *buf_p)
{
	unsigned char *mem_ptr;
	/*
	WIN32�̊��ϐ��̈悩��run68�̃G�~�����[�V�����̈�ɕ������Ă���
	�l����������d�l�ɂ���B
	 */
	/*
	���G���A�̐擪(ENV_TOP)���珇�Ɋ��ϐ�������������B
	 */
	for (mem_ptr = prog_ptr + ENV_TOP + 4;
		 *mem_ptr != 0;
		 mem_ptr ++) {
		char ename[256];
		int i;
		/* ���ϐ������擾����B*/
		for (i = 0; *mem_ptr != '\0' && *mem_ptr != '='; i ++) {
			ename[i] = *(mem_ptr ++);
		}
		ename[i] = '\0';
		if (_stricmp(name_p, ename) == 0) {
			/* ���ϐ������������B*/
			while (*mem_ptr == '=' || *mem_ptr == ' ') {
				mem_ptr ++;
			}
			/* �󕶎���̏ꍇ������B*/
			/*            *buf_p = (long)((char*)mem_ptr - prog_ptr);*/
			strcpy(buf_p, mem_ptr);
			return 0;
		}
		/* �ϐ�������v���Ȃ�������A�ϐ��̒l���X�L�b�v����B*/
		while (*mem_ptr)
			mem_ptr ++;
		/* '\0'�̌�ɂ������'\0'�������ꍇ�́A���ϐ��̈�̏I��ł���B*/
	}
	/* �ϐ���������Ȃ�������NULL�|�C���^��Ԃ��B*/
	(*buf_p) = 0;
	return -10;
}

/*
   �@�\�F
     DOSCALL NAMESTS�����s����
   �߂�l�F
     �G���[�R�[�h
 */
static long Namests( long name, long buf )
{
	char     nbuf [ 256 ];
	char     cud [ 67 ];
	char     *name_ptr;
	char     *buf_ptr;
#if !defined(WIN32)
	unsigned getdrv;
#endif
	UChar    drv;
	int      wild = 0;
	int      len;
	int      i;

	name_ptr = prog_ptr + name;
	buf_ptr  = prog_ptr + buf;
	memset( buf_ptr, 0x00, 88 );
	if ( (len=strlen( name_ptr )) > 88 )
		return( -13 );        /* �t�@�C�����̎w���� */
	strcpy( nbuf, name_ptr );

	/* �g���q���Z�b�g */
	for( i = len - 1; i >= 0 && nbuf [ i ] != '.'; i-- ) {
		if ( nbuf [ i ] == '*' || nbuf [ i ] == '?' )
			wild = 1;
	}
	if ( strlen( &(nbuf [ i ]) ) > 4 )
		return( -13 );
	memset( buf_ptr + 75, ' ', 3 );
	if ( i < 0 ) {
		/* �g���q�Ȃ� */
		i = len;
	} else {
		memcpy( buf_ptr + 75, &(nbuf [ i + 1 ]), strlen(&(nbuf [ i + 1 ])) );
		nbuf [ i ] = '\0';
	}

	/* �t�@�C�������Z�b�g */
	for( i --; i >= 0; i-- ) {
		if ( nbuf [ i ] == '\\' || nbuf[ i ] == '/' || nbuf [ i ] == ':' )
			break;
		if ( nbuf [ i ] == '*' || nbuf [ i ] == '?' )
			wild = 1;
	}
	i ++;
	if ( strlen( &(nbuf [ i ]) ) > 18 )
		return( -13 );
	if ( strlen( &(nbuf [ i ]) ) > 8 )    /* �{���̓G���[�ł͂Ȃ� */
		return( -13 );
	memset( buf_ptr + 67, ' ', 8 );
	memcpy( buf_ptr + 67, &(nbuf [ i ]), strlen(&(nbuf [ i ])) );
	nbuf [ i ] = '\0';

	if ( wild != 0 )
		mem_set( buf, 0x01, S_BYTE );

	/* �p�X�����Z�b�g */
	if ( i == 0 ) {
		/* �J�����g�f�B���N�g�����Z�b�g */
		if (Curdir(0, cud) != 0)
			return( 13 );
		strcpy( buf_ptr + 2, &(cud [ 2 ]) );
		if ( cud [ strlen( cud ) - 1 ] != '\\' )
			strcat( buf_ptr + 2, "\\" );
		nbuf [ 0 ] = cud [ 0 ];
		i = 1;
	} else {
		for( i --; i >= 0; i-- ) {
			if ( nbuf [ i ] == ':' )
				break;
		}
		i ++;
		if ( strlen( &(nbuf [ i ]) ) > 64 )
			return( -13 );
		strcpy( buf_ptr + 2, &(nbuf [ i ]) );
	}

	/* �h���C�u�����Z�b�g */
	if ( i == 0 ) {
		/* �J�����g�h���C�u���Z�b�g */
#if defined(WIN32)
		char path[MAX_PATH];
		GetCurrentDirectory(strlen(path), path);
		mem_set( buf + 1, path[0] - 'A', S_BYTE );
#elif defined(DOSX)
		dos_getdrive( &getdrv );
		mem_set( buf + 1, getdrv - 1, S_BYTE );
#else
		mem_set( buf + 1, 0, S_BYTE );
#endif
	} else {
		drv = toupper(nbuf[ 0 ]) - 'A';
		if ( drv >= 26 )
			return( -13 );
		mem_set( buf + 1, drv, S_BYTE );
	}

	return( 0 );
}

/*
 �@�@�\�FDOSCALL NAMECK�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Nameck( long name, long buf )
{
	char     nbuf [ 89 ];
	char     *name_ptr;
	char     *buf_ptr;
	unsigned int drv;
	int     ret = 0;
	int     len;
	int     i;

	name_ptr = prog_ptr + name;
	buf_ptr  = prog_ptr + buf;
	memset( buf_ptr, 0x00, 91 );
	if ( (len=strlen( name_ptr )) > 88 )
		return( -13 );        /* �t�@�C�����̎w���� */
	strcpy( nbuf, name_ptr );

	/* �g���q���Z�b�g */
	for( i = len - 1; i >= 0 && nbuf [ i ] != '.'; i-- ) {
		if ( nbuf [ i ] == '*' || nbuf [ i ] == '?' )
		  ret = 1;
	}
	if ( strlen( &(nbuf [ i ]) ) > 4 )
		return( -13 );
	if ( i < 0 ) {    /* �g���q�Ȃ� */
		i = len;
	} else {
		strcpy( buf_ptr + 86, &(nbuf [ i ]) );
		nbuf [ i ] = '\0';
	}

	/* �t�@�C�������Z�b�g */
	for( i --; i >= 0; i-- ) {
		if ( nbuf [ i ] == '\\' || nbuf[ i ] == '/' || nbuf [ i ] == ':' )
			break;
		if ( nbuf [ i ] == '*' || nbuf [ i ] == '?' )
			ret = 1;
	}
	i ++;
	if ( strlen( &(nbuf [ i ]) ) > 18 )
		return( -13 );
	strcpy( buf_ptr + 67, &(nbuf [ i ]) );
	nbuf [ i ] = '\0';

	/* �p�X�����Z�b�g */
	if ( i == 0 ) {
		strcpy( buf_ptr + 2, ".\\" );
	} else {
		for( i --; i >= 0; i-- ) {
			if ( nbuf [ i ] == ':' )
				break;
		}
		i ++;
		if ( strlen( &(nbuf [ i ]) ) > 64 )
			return( -13 );
		strcpy( buf_ptr + 2, &(nbuf [ i ]) );
	}

	/* �h���C�u�����Z�b�g */
	if ( i == 0 ) {
		/* �J�����g�h���C�u���Z�b�g */
#if defined(WIN32)
		char path[MAX_PATH];
		BOOL b;
		b = GetCurrentDirectoryA(sizeof(path), path);
		drv = path[0] - 'A' + 1;
#elif defined(DOSX)
		dos_getdrive( &drv );
#else
		drv = 1;
#endif
		buf_ptr [ 0 ] = drv - 1 + 'A';
		buf_ptr [ 1 ] = ':';
	} else {
		memcpy( buf_ptr, nbuf, 2 );
	}

	return( ret );
}

/*
 �@�@�\�FDOSCALL CONCTRL�����s����
 �߂�l�Fmode�ɂ���ĈقȂ�
 */
static long Conctrl( short mode, long adr )
{
	char   *p;
	long   mes;
	UShort usrt;
	short  srt;
	short  x, y;

	switch( mode ) {
	case  0:
		usrt = (unsigned short)mem_get( adr, S_WORD );
		if ( usrt >= 0x0100 )
			putchar( usrt >> 8 );
		putchar( usrt );
#if defined(WIN32)
		FlushFileBuffers(finfo[ 1 ].fh);
#else
		fflush( stdout );
#endif
		break;
	case  1:
		mes = mem_get( adr, S_LONG );
		p = prog_ptr + mes;
		printf( "%s", p );
		break;
	case  2:    /* ���� */
		srt = (short)mem_get( adr, S_WORD );
		text_color( srt );
		break;
	case  3:    /* locate */
		x = (short)mem_get( adr, S_WORD );
		y = (short)mem_get( adr + 2, S_WORD );
		printf( "%c[%d;%dH", 0x1B, y + 1, x + 1 );
		break;
	case  4:    /* �P�s���ɃJ�[�\���ړ�(�X�N���[���L��) */
		printf( "%c[s\n%c[u%c[1B", 0x1B, 0x1B, 0x1B );
		break;
	case  5:    /* �P�s��ɃJ�[�\���ړ�(�X�N���[�����T�|�[�g) */
		printf( "%c[1A", 0x1B );
		break;
	case  6:    /* srt�s��ɃJ�[�\���ړ� */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dA", 0x1B, srt );
		break;
	case  7:    /* srt�s���ɃJ�[�\���ړ� */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dB", 0x1B, srt );
		break;
	case  8:    /* srt�����E�ɃJ�[�\���ړ� */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dC", 0x1B, srt );
		break;
	case  9:    /* srt�������ɃJ�[�\���ړ� */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dD", 0x1B, srt );
		break;
	case 10:
		srt = (short)mem_get( adr, S_WORD );
		switch ( srt ) {
		case 0:    /* �ŏI�s���[�܂ŏ��� */
			printf( "%c[0J", 0x1B );
			break;
		case 1:    /* �z�[������J�[�\���ʒu�܂ŏ��� */
			printf( "%c[1J", 0x1B );
			break;
		case 2:    /* ��ʂ����� */
			printf( "%c[2J", 0x1B );
			break;
		}
		break;
	case 11:
		srt = (short)mem_get( adr, S_WORD );
		switch ( srt ) {
		case 0:    /* �E�[�܂ŏ��� */
			printf( "%c[K", 0x1B );
			break;
		case 1:    /* ���[����J�[�\���ʒu�܂ŏ��� */
			printf( "%c[1K", 0x1B );
			break;
		case 2:    /* 1�s���� */
			printf( "%c[s", 0x1B );    /* �ʒu�ۑ� */
			printf( "%c[999D", 0x1B ); /* ���[�Ɉړ� */
			printf( "%c[K", 0x1B );    /* �E�[�܂ŏ��� */
			printf( "%c[u", 0x1B );    /* �ʒu�Đݒ� */
			break;
		}
		break;
	case 12:    /* �J�[�\���s��srt�s�}�� */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dL", 0x1B, srt );
		break;
	case 13:    /* �J�[�\���s����srt�s�폜 */
		srt = (short)mem_get( adr, S_WORD );
		printf( "%c[%dM", 0x1B, srt );
		break;
	case 17:    /* �J�[�\���\�� */
		printf( "%c[>5l", 0x1B );
		break;
	case 18:    /* �J�[�\������ */
		printf( "%c[>5h", 0x1B );
		break;
	}

	return( 0 );
}

/*
 �@�@�\�FDOSCALL KEYCTRL�����s����
 �߂�l�F�L�[�R�[�h��(mode�ɂ���ĈقȂ�)
 */
static long Keyctrl( short mode, long stack_adr )
{
	UChar    c;

	switch( mode ) {
	case 0:
		c = _getch();
		if ( c == 0x00 ) {
			c = _getch();
			if ( c == 0x85 )    /* F11 */
				c = 0x03;    /* break */
		} else {
			if ( ini_info.pc98_key == TRUE )
				c = cnv_key98( c );
		}
		return( c );
#if defined(WIN32) || defined(DOSX)
		//#if defined(WIN32)
		//#elif defined(DOSX)
	case 1:        /* �L�[�̐�ǂ� */
		if ( _kbhit() == 0 )
		  return( 0 );
		c = _getch();
		if ( c == 0x00 ) {
			c = _getch();
			if ( c == 0x85 )    /* F11 */
				c = 0x03;    /* break */
		} else {
			if ( ini_info.pc98_key == TRUE )
				c = cnv_key98( c );
		}
		_ungetch( c );
		return( c );
#endif
	default:
		return( 0 );
	}
}

/*
 �@�@�\�FDOSCALL FNCKEY�����s����
 �߂�l�F�Ȃ�
 */
static void Fnckey( short mode, long buf )
{
	char    *buf_ptr;

	buf_ptr = prog_ptr + buf;

	if ( mode < 256 )
		get_fnckey( mode, buf_ptr );
	else
		put_fnckey( mode - 256, buf_ptr );
}

/*
 �@�@�\�FDOSCALL INTVCG�����s����
 �߂�l�F�x�N�^�̒l
 */
static long Intvcg( UShort intno )
{
	long  adr2;
	long  mae;
	short save_s;

	if ( intno >= 0xFF00 ) {    /* DOSCALL */
		intno &= 0xFF;
		adr2 = 0x1800 + intno * 4;
		save_s = SR_S_REF();
		SR_S_ON();
		mae = mem_get( adr2, S_LONG );
		if ( save_s == 0 )
			SR_S_OFF();
		return( mae );
	}

	intno &= 0xFF;
	adr2 = intno * 4;
	save_s = SR_S_REF();
	SR_S_ON();
	mae = mem_get( adr2, S_LONG );
	if ( save_s == 0 )
		SR_S_OFF();
	return( mae );
}

/*
 �@�@�\�FDOSCALL INTVCS�����s����
 �߂�l�F�ݒ�O�̃x�N�^
 */
static long Intvcs( UShort intno, long adr )
{
	long adr2;
	long mae;
	short    save_s;

	if ( intno >= 0xFF00 ) {    /* DOSCALL */
		intno &= 0xFF;
		adr2 = 0x1800 + intno * 4;
		save_s = SR_S_REF();
		SR_S_ON();
		mae = mem_get( adr2, S_LONG );
		mem_set( adr2, adr, S_LONG );
		if ( save_s == 0 )
			SR_S_OFF();
		return( mae );
	}

	return( 0 );
}

/*
 �@�@�\�FDOSCALL ASSIGN�����s����
 �߂�l�F�G���[�R�[�h��
 */
static long Assign( short mode, long stack_adr )
{
	long drv;
	long buf;
	char    *drv_ptr;
	char    *buf_ptr;

	switch( mode ) {
	case 0:
		drv = mem_get( stack_adr, S_LONG );
		buf = mem_get( stack_adr + 4, S_LONG );
		drv_ptr = prog_ptr + drv;
		if ( drv_ptr[ 1 ] != ':' || drv_ptr[ 2 ] != '\0' )
			return( -14 );
		drv = toupper( drv_ptr[ 0 ] ) - 'A' + 1;
		if ( drv < 1 || drv > 26 )
			return( -14 );
		if (Curdir((short)drv, prog_ptr + buf) != 0)
			return( -14 );
		if (func_trace_f) {
			buf_ptr = prog_ptr + buf;
			printf( " drv=%s cudir=%s\n", drv_ptr, buf_ptr );
		}
		return( 0x40 );
	default:
		return( -14 );
	  }
}

/*
 �@�@�\�FDOSCALL GETFCB�����s����
 �߂�l�FFCB�̃A�h���X
 */
static long Getfcb( short fhdl )
{
	static unsigned char fcb [ 4 ] [ 0x60 ] = {
		{0x01,0xC1,0x00,0x02,0xC6,0x04,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x43,0x4F,0x4E,0x20,
		 0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00},
		{0x01,0xC2,0x00,0x02,0xC6,0x04,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x43,0x4F,0x4E,0x20,
		 0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00},
		{0x01,0xC2,0x00,0x02,0xC6,0x04,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x43,0x4F,0x4E,0x20,
		 0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00},
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00}
	};
	char    *fcb_ptr;
	fcb_ptr = prog_ptr + FCB_WORK;
	switch( fhdl ) {
	case 0:
	case 1:
	case 2:
		memcpy( fcb_ptr, fcb [ fhdl ], 0x60 );
		return( FCB_WORK );
	default:
		fcb [ 3 ] [ 14 ] = (unsigned char)fhdl;
		memcpy( fcb_ptr, fcb [ 3 ], 0x60 );
		return( FCB_WORK );
	}
}

/*
 �@�@�\�FDOSCALL EXEC(mode=0,1)�����s����
 �߂�l�F�G���[�R�[�h��
 */
static long Exec01( long nm, long cmd, long env, int md )
{
	FILE *fp;
	char fname [ 89 ];
	char *name_ptr;
	int  loadmode;
	long mem;
	long prev_adr;
	long end_adr;
	long pc1;
	long size;
	long prog_size;
	long prog_size2;

	loadmode = ((nm >> 24) & 0x03);
	nm &= 0xFFFFFF;
	name_ptr = prog_ptr + nm;
	if ( strlen( name_ptr ) > 88 )
		return( -13 );        /* �t�@�C�����w���� */

	strcpy( fname, name_ptr );
	if ( (fp=prog_open( fname, FALSE )) == NULL )
		return( -2 );

	if ( nest_cnt + 1 >= NEST_MAX )
		return( -8 );

	mem = Malloc( mem_aloc );
	if ( (mem=Malloc( mem_aloc )) == (long)0x82000000 ) {
		fclose( fp );
		return( -8 );        /* ���������m�ۂł��Ȃ� */
	}
	mem &= 0xFFFFFF;
	size = mem;
	if ( (mem=Malloc( mem )) > 0xFFFFFF ) {
		fclose( fp );
		return( -8 );
	}
	prev_adr = mem_get( mem - 0x10, S_LONG );
	end_adr  = mem_get( mem - 0x08, S_LONG );
	memset( prog_ptr + mem, 0, size );

	prog_size2 = ((loadmode << 24) | end_adr);
	pc1 = prog_read( fp, fname, mem - MB_SIZE + PSP_SIZE, &prog_size,
					 &prog_size2, FALSE );
	if ( pc1 < 0 ) {
		Mfree( mem );
		return( pc1 );
	}

	nest_pc [ nest_cnt ] = pc;
	nest_sp [ nest_cnt ] = ra [ 7 ];
	ra [ 0 ] = mem - MB_SIZE;
	ra [ 1 ] = mem - MB_SIZE + PSP_SIZE + prog_size;
	ra [ 2 ] = cmd;
	if ( env == 0 )
		ra [ 3 ] = mem_get( psp [ nest_cnt ] + 0x10, S_LONG );
	else
		ra [ 3 ] = env;
	ra [ 4 ] = pc1;
	nest_cnt ++;
	if ( make_psp( fname, prev_adr, end_adr, psp [ nest_cnt - 1 ], prog_size2 )
		 == FALSE ) {
		nest_cnt --;
		Mfree( mem );
		return( -13 );
	}

	if ( md == 0 ) {
		pc = ra [ 4 ];
		return( rd [ 0 ] );
	} else {
		nest_cnt --;
		return( ra [ 4 ] );
	}
}

/*
 �@�@�\�FDOSCALL EXEC(mode=2)�����s����
 �߂�l�F�G���[�R�[�h
 */
static long Exec2( long nm, long cmd, long env )
{
	FILE *fp;
	char *name_ptr;
	char *cmd_ptr;
	char *p;
	int  len;

	name_ptr = prog_ptr + nm;
	cmd_ptr  = prog_ptr + cmd;
	p = name_ptr;
	while( *p != '\0' && *p != ' ' )
		p ++;
	if ( *p != '\0' ) {    /* �R�}���h���C������ */
		*p = '\0';
		p ++;
		len = strlen( p );
		*((UChar *)cmd_ptr) = (UChar)len;
		strcpy( cmd_ptr + 1, p );
	}

	/* ���ϐ�PATH�ɏ]���ăt�@�C�����������A�I�[�v������B*/
	fp = prog_open(name_ptr, TRUE);
	if (fp == NULL)
	{
		return 0;
	} else {
		fclose(fp);
	}
	return( 0 );
}

/*
 �@�@�\�FDOSCALL EXEC(mode=3)�����s����
 �߂�l�F�G���[�R�[�h��
 */
static long Exec3( long nm, long adr1, long adr2 )
{
	FILE *fp;
	char fname [ 89 ];
	char *name_ptr;
	int  loadmode;
	long ret;
	long prog_size;
	long prog_size2;

	loadmode = ((nm >> 24) & 0x03);
	nm   &= 0xFFFFFF;
	adr1 &= 0xFFFFFF;
	adr2 &= 0xFFFFFF;
	name_ptr = prog_ptr + nm;
	if ( strlen( name_ptr ) > 88 )
		return( -13 );        /* �t�@�C�����w���� */

	strcpy( fname, name_ptr );
	if ( (fp=prog_open( fname, FALSE )) == NULL )
		return( -2 );

	prog_size2 = ((loadmode << 24) | adr2);
	ret = prog_read( fp, fname, adr1, &prog_size, &prog_size2, FALSE );
	if ( ret < 0 )
		return( ret );

	return( prog_size );
}

/*
 �@�@�\�FDOSCALL EXEC(mode=4)�����s����
 �߂�l�F�G���[�R�[�h��
 */
static void Exec4( long adr )
{
	nest_pc [ nest_cnt ] = pc;
	nest_sp [ nest_cnt ] = ra [ 7 ];
	nest_cnt ++;
	pc = adr;
}

/*
 �@�@�\�F�W�����Ԃ���{���Ԃɕϊ�����
 �߂�l�F�Ȃ�
 */
static void get_jtime( UShort *d, UShort *t, int offset )
{
	static month_day [ 13 ] = {
		31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	short    year;
	short    month;
	short    day;
	short    hour;

	hour = (*t >> 11);
	if ( hour + offset * 9 >= 0 && hour + offset * 9 <= 23 ) {
		*t += (0x4800 * offset);
		return;
	}

	if ( offset > 0 )
		hour -= 15;
	else
		hour += 15;
	*t = (*t & 0x7FF) + (0x800 * hour);

	year  = (*d >> 9);
	month = ((*d >> 5) & 0xF);
	day   = (*d & 0x1F);

	/* 2���̓����̔��� */
	if ( (year % 4) == 0 ) {
		if ( (year % 100) == 0 ) {
			if ( (year % 400) == 0 )
				month_day [ 2 ] = 29;
			else
				month_day [ 2 ] = 28;
		} else {
			month_day [ 2 ] = 29;
		}
	} else {
		month_day [ 2 ] = 28;
	}

	if ( day + offset >= 1 && day + offset <= month_day [ month ] ) {
		*d += offset;            /* ���}1 */
		return;
	}
	if ( offset > 0 )
		*d = (*d & 0xFFE0) + 1;            /* 1�� */
	else
		*d = (*d & 0xFFE0) + month_day [ month - 1 ];    /* �O���ŏI�� */

	if ( month + offset >= 1 && month + offset <= 12 ) {
		*d += (0x20 * offset);        /* ���}1 */
		return;
	}
	if ( offset > 0 )
		*d = (*d & 0xFE1F) + 0x200 + 0x20;    /* ���N1�� */
	else
		*d = (*d & 0xFE1F) - 0x200 + 0x180;    /* �O�N12�� */
}

/*
 �@�@�\�Fgets�̑��������
 �߂�l�F�Ȃ�
 */
static long gets2( char *str, int max )
{
	int    c;
	int    cnt;
	unsigned dmy;

	cnt = 0;
	c = getchar();
	if ( c == EOF )
		fseek( stdin, 0, 0 );
	while( c != EOF && c != '\n' ) {
		if ( cnt < max )
			str [ cnt ++ ] = c;
		c = getchar();
	}
	if ( c == EOF )
		str[ cnt ++ ] = EOF;
#if defined(WIN32)
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\x01B[1A", 4, &dmy, NULL);
#elif defined(DOSX)
	_dos_write( fileno(stdout), "\x01B[1A", 4, &dmy );
#else
	abort();  // FIXME
#endif
	/* printf("%c[1A", 0x1B); */    /* �J�[�\�����P�s��� */
	str[ cnt ] = '\0';

	return( strlen( str ) );
}
