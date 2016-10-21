/* $Id: iocscall.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.3  1999/12/07  12:42:59  yfujii
 * *** empty log message ***
 *
 * Revision 1.3  1999/10/25  03:24:58  yfujii
 * Trace output is now controlled with command option.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <string.h>
#include "run68.h"
#if !defined(WIN32)
#if defined(DOSX)
#include <dos.h>
#else
#include <sys/sysinfo.h>
#endif
#include <time.h>
#elif defined(WIN32)
#include <windows.h>
#endif

static long	Putc( UShort );
static long	Color( short );
static void	Putmes( void );
static long	Dateget( void );
static long	Timeget( void );
static long	Datebin( long );
static long	Timebin( long );
static long	Dateasc( long, long );
static long	Timeasc( long, long );
static void	Dayasc( long, long );
static long	Intvcs( long, long );
static void	Dmamove( long, long, long, long );

/*
 �@�@�\�FIOCSCALL�����s����
 �߂�l�F TRUE = ���s�I��
         FALSE = ���s�p��
*/
int iocs_call()
{
	UChar	*data_ptr;
	ULong	ul;
	UChar	no;
	int	x, y;
	short	save_s;

	no = rd [ 0 ] & 0xff;

    if (func_trace_f)
    {
        printf( "IOCS(%02X): PC=%06lX\n", no, pc );
    }
	switch( no ) {
		case 0x20:	/* B_PUTC */
			rd [ 0 ] = Putc( (rd [ 1 ] & 0xFFFF) );
			break;
		case 0x21:	/* B_PRINT */
			data_ptr = (UChar *)prog_ptr + ra [ 1 ];
			printf( "%s", data_ptr );
			ra [ 1 ] += strlen((char *)data_ptr);
			rd [ 0 ] = get_locate();
			break;
		case 0x22:	/* B_COLOR */
			rd [ 0 ] = Color( (rd [ 1 ] & 0xFFFF) );
			break;
		case 0x23:	/* B_LOCATE */
			if ( rd [ 1 ] != -1 ) {
				x = (rd [ 1 ] & 0xFFFF) + 1;
				y = (rd [ 2 ] & 0xFFFF) + 1;
				printf( "%c[%d;%dH", 0x1B, y, x );
			}
			rd [ 0 ] = get_locate();
			break;
		case 0x24:	/* B_DOWN_S */
			printf( "%c[s\n%c[u%c[1B", 0x1B, 0x1B, 0x1B );
			break;
		case 0x25:	/* B_UP_S *//* (�X�N���[�����T�|�[�g) */
			printf( "%c[1A", 0x1B );
			break;
		case 0x2F:	/* B_PUTMES */
			Putmes();
			break;
		case 0x54:	/* DATEGET */
			rd [ 0 ] = Dateget();
			break;
		case 0x55:	/* DATEBIN */
			rd [ 0 ] = Datebin( rd [ 1 ] );
			break;
		case 0x56:	/* TIMEGET */
			rd [ 0 ] = Timeget();
			break;
		case 0x57:	/* TIMEBIN */
			rd [ 0 ] = Timebin( rd [ 1 ] );
			break;
		case 0x5A:	/* DATEASC */
			rd [ 0 ] = Dateasc( rd [ 1 ], ra [ 1 ] );
			break;
		case 0x5B:	/* TIMEASC */
			rd [ 0 ] = Timeasc( rd [ 1 ], ra [ 1 ] );
			break;
		case 0x5C:	/* DAYASC */
			Dayasc( rd [ 1 ], ra [ 1 ] );
			break;
		case 0x6C:	/* VDISPST */
			save_s = SR_S_REF();
			SR_S_ON();
			if ( ra [ 1 ] == 0 ) {
				mem_set( 0x118, 0, S_LONG );
			} else {
				rd [ 0 ] = mem_get( 0x118, S_LONG );
				if ( rd [ 0 ] == 0 )
					mem_set( 0x118, ra [ 1 ], S_LONG );
			}
			if ( save_s == 0 )
				SR_S_OFF();
			break;
		case 0x6D:	/* CRTCRAS */
			save_s = SR_S_REF();
			SR_S_ON();
			if ( ra [ 1 ] == 0 ) {
				mem_set( 0x138, 0, S_LONG );
			} else {
				rd [ 0 ] = mem_get( 0x138, S_LONG );
				if ( rd [ 0 ] == 0 )
					mem_set( 0x138, ra [ 1 ], S_LONG );
			}
			if ( save_s == 0 )
				SR_S_OFF();
			break;
		case 0x6E:	/* HSYNCST */
			err68( "�����������荞�݂�ݒ肵�悤�Ƃ��܂���" );
			return( TRUE );
		case 0x7F:	/* ONTIME */
#if defined(WIN32)
            ul = GetTickCount() / 1000;
			rd [ 0 ] = (ul % (60 * 60 * 24)) * 100;
			rd [ 1 ] = ((ul / (60 * 60 * 24)) & 0xFFFF);
#elif defined(DOSX)
			ul = time( NULL );
			rd [ 0 ] = (ul % (60 * 60 * 24)) * 100;
			rd [ 1 ] = ((ul / (60 * 60 * 24)) & 0xFFFF);
#else
                        {
				struct sysinfo info;
				sysinfo(&info);
				ul = info.uptime;
				rd [ 0 ] = (ul % (60 * 60 * 24)) * 100; 
				rd [ 1 ] = ((ul / (60 * 60 * 24)) & 0xFFFF);
                        }
#endif
			break;
		case 0x80:	/* B_INTVCS */
			rd [ 0 ] = Intvcs( rd [ 1 ], ra [ 1 ] );
			break;
		case 0x81:	/* B_SUPER */
			if ( ra [ 1 ] == 0 ) {
				/* user -> super */
				if ( SR_S_REF() != 0 ) {
					rd [ 0 ] = -1;	/* �G���[ */
				} else {
					rd [ 0 ] = ra [ 7 ];
					SR_S_ON();
				}
			} else {
				/* super -> user */
				ra [ 7 ] = ra [ 1 ];
				rd [ 0 ] = 0;
				SR_S_OFF();
			}
			break;
		case 0x82:	/* B_BPEEK */
			save_s = SR_S_REF();
			SR_S_ON();
			rd [ 0 ] = ( ( rd [ 0 ] & 0xFFFFFF00 ) |
				     ( mem_get( ra [ 1 ], S_BYTE ) & 0xFF ) );
			if ( save_s == 0 )
				SR_S_OFF();
			ra [ 1 ] += 1;
			break;
		case 0x83:	/* B_WPEEK */
			save_s = SR_S_REF();
			SR_S_ON();
			rd [ 0 ] = ( ( rd [ 0 ] & 0xFFFF0000 ) |
				     ( mem_get( ra [ 1 ], S_WORD ) & 0xFFFF ) );
			if ( save_s == 0 )
				SR_S_OFF();
			ra [ 1 ] += 2;
			break;
		case 0x84:	/* B_LPEEK */
			save_s = SR_S_REF();
			SR_S_ON();
			rd [ 0 ] = mem_get( ra [ 1 ], S_LONG );
			if ( save_s == 0 )
				SR_S_OFF();
			ra [ 1 ] += 4;
			break;
		case 0x8A:	/* DMAMOVE */
			Dmamove( rd [ 1 ], rd [ 2 ], ra [ 1 ], ra [ 2 ] );
			break;
		case 0xAE:	/* OS_CURON */
			printf( "%c[>5l", 0x1B );
			break;
		case 0xAF:	/* OS_CUROF */
			printf( "%c[>5h", 0x1B );
			break;
		default:
    if (func_trace_f)
    {
			printf( "IOCS(%02X): Unknown IOCS call. Ignored.\n", no );
    }
			break;
	}

	return( FALSE );
}

/*
 �@�@�\�F������\������
 �߂�l�F�J�[�\���ʒu
*/
static long Putc( UShort code )
{
	if ( code == 0x1A ) {
		printf( "%c[0J", 0x1B ); /* �ŏI�s���[�܂ŏ��� */
	} else {
		if ( code >= 0x0100 )
			putchar( code >> 8 );
		putchar( code );
	}
	return( get_locate() );
}

/*
 �@�@�\�F�����̃J���[�������w�肷��
 �߂�l�F�ύX�O�̃J���[�܂��͌��݂̃J���[
*/
static long Color( short arg )
{
	if ( arg == -1 )	/* ���݂̃J���[�𒲂ׂ�(���T�|�[�g) */
		return( 3 );

	text_color( arg );

	return( 3 );
}

/*
 �@�@�\�F�������\������
 �߂�l�F�Ȃ�
*/
static void Putmes()
{
	char	temp [ 97 ];
	char	*p;
	int	x, y;
	int	keta;
	int	len;

	x = (rd [ 2 ] & 0xFFFF) + 1;
	y = (rd [ 3 ] & 0xFFFF) + 1;
	keta = (rd [ 4 ] & 0xFFFF) + 1;

	p = prog_ptr + ra [ 1 ];
	len = strlen( p );
	if ( keta > 96 )
		keta = 96;
	memcpy( temp, p, keta );
	temp [ keta ] = '\0';

	printf( "%c[%d;%dH", 0x1B, y, x );
	text_color( (rd [ 1 ] & 0xFF) );
	printf("%s", temp);

	ra [ 1 ] += len;
}

/*
 �@�@�\�F���t�𓾂�
 �߂�l�FBCD�̓��t�f�[�^
*/
static long Dateget()
{
	long	ret;
#if defined(WIN32)
    SYSTEMTIME st;
    GetSystemTime(&st);
	ret = (st.wDayOfWeek << 24);
	ret |= (((st.wYear - 1980) / 10) << 20);
	ret |= (((st.wYear - 1980) % 10) << 16);
	ret |= ((st.wMonth / 10) << 12);
	ret |= ((st.wMonth % 10) << 8);
	ret |= ((st.wDay / 10) << 4);
	ret |= (st.wDay % 10);
#elif defined(DOSX)
	struct dos_date_t ddate;
	dos_getdate( &ddate );
	ret = (ddate.dayofweek << 24);
	ret |= (((ddate.year - 1980) / 10) << 20);
	ret |= (((ddate.year - 1980) % 10) << 16);
	ret |= ((ddate.month / 10) << 12);
	ret |= ((ddate.month % 10) << 8);
	ret |= ((ddate.day / 10) << 4);
	ret |= (ddate.day % 10);
#else
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	ret = (t->tm_wday << 24);
	ret |= (((t->tm_year - 80) / 10) << 20);
	ret |= (((t->tm_year - 80) % 10) << 16);
	ret |= ((t->tm_mon / 10) << 12);
	ret |= ((t->tm_mon % 10) << 8);
	ret |= ((t->tm_mday / 10) << 4);
	ret |= (t->tm_mday % 10);
#endif
	return( ret );
}

/*
 �@�@�\�F�����𓾂�
 �߂�l�FBCD�̎����f�[�^
*/
static long Timeget()
{
	long	ret;
#if defined(WIN32)
    SYSTEMTIME st;
    GetSystemTime(&st);
	ret  = ((st.wHour / 10) << 20);
	ret |= ((st.wHour % 10) << 16);
	ret |= ((st.wMinute / 10) << 12);
	ret |= ((st.wMinute % 10) << 8);
	ret |= ((st.wSecond / 10) << 4);
	ret |= (st.wSecond % 10);
#elif defined(DOSX)
	struct dos_time_t dtime;
	dos_gettime( &dtime );
	ret  = ((dtime.hour / 10) << 20);
	ret |= ((dtime.hour % 10) << 16);
	ret |= ((dtime.minute / 10) << 12);
	ret |= ((dtime.minute % 10) << 8);
	ret |= ((dtime.second / 10) << 4);
	ret |= (dtime.second % 10);
#else
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	ret  = ((t->tm_hour / 10) << 20);
	ret |= ((t->tm_hour % 10) << 16);
	ret |= ((t->tm_min / 10) << 12);
	ret |= ((t->tm_min % 10) << 8);
	ret |= ((t->tm_sec / 10) << 4);
	ret |= (t->tm_sec % 10);
#endif
	return( ret );
}

/*
 �@�@�\�FBCD�\���̓��t�f�[�^���o�C�i���\���ɒ���
 �߂�l�F�o�C�i���̓��t�f�[�^
*/
static long Datebin( long bcd )
{
	UShort	youbi;
	UShort	year;
	UShort	month;
	UShort	day;

	youbi = ( bcd >> 24 );
	year  = (( bcd >> 20 ) & 0xF) * 10 + (( bcd >> 16 ) & 0xF) + 1980;
	month = (( bcd >> 12 ) & 0xF) * 10 + (( bcd >> 8 ) & 0xF);
	day   = (( bcd >> 4 ) & 0xF) * 10 + (bcd & 0xF);

	return( (youbi << 28) | (year << 16) | (month << 8) | day );
}

/*
 �@�@�\�FBCD�\���̎����f�[�^���o�C�i���\���ɒ���
 �߂�l�F�o�C�i���̎����f�[�^
*/
static long Timebin( long bcd )
{
	UShort	hh;
	UShort	mm;
	UShort	ss;

	hh = (( bcd >> 20 ) & 0xF) * 10 + (( bcd >> 16 ) & 0xF);
	mm = (( bcd >> 12 ) & 0xF) * 10 + (( bcd >> 8 ) & 0xF);
	ss = (( bcd >> 4 ) & 0xF) * 10 + (bcd & 0xF);

	return( (hh << 16) | (mm << 8) | ss );
}

/*
 �@�@�\�F�o�C�i���\���̓��t�f�[�^�𕶎���ɒ���
 �߂�l�F-1�̂Ƃ��G���[
*/
static long Dateasc( long data, long adr )
{
	char	*data_ptr;
	UShort	year;
	UShort	month;
	UShort	day;
	int	form;

	data_ptr = prog_ptr + adr;

	form = data >> 28;
	year = ((data >> 16) & 0xFFF);
	if ( year < 1980 || year > 2079 )
		return( -1 );
	month = ((data >> 8) & 0xFF);
	if ( month < 1 || month > 12 )
		return( -1 );
	day = (data & 0xFF);
	if ( day < 1 || day > 31 )
		return( -1 );

	switch( form ) {
		case 0:
			sprintf( data_ptr, "%04d/%02d/%02d", year, month, day);
			ra [ 1 ] += 10;
			break;
		case 1:
			sprintf( data_ptr, "%04d-%02d-%02d", year, month, day);
			ra [ 1 ] += 10;
			break;
		case 2:
			sprintf( data_ptr, "%02d/%02d/%02d", year % 100, month, day);
			ra [ 1 ] += 8;
			break;
		case 3:
			sprintf( data_ptr, "%02d-%02d-%02d", year % 100, month, day);
			ra [ 1 ] += 8;
			break;
		default:
			return( -1 );
	}

	return( 0 );
}

/*
 �@�@�\�F�o�C�i���\���̎����f�[�^�𕶎���ɒ���
 �߂�l�F-1�̂Ƃ��G���[
*/
static long Timeasc( long data, long adr )
{
	char	*data_ptr;
	UShort	hh;
	UShort	mm;
	UShort	ss;

	data_ptr = prog_ptr + adr;

	hh = ((data >> 16) & 0xFF);
	if ( hh < 0 || hh > 23 )
		return( -1 );
	mm = ((data >> 8) & 0xFF);
	if ( mm < 0 || mm > 59 )
		return( -1 );
	ss = (data & 0xFF);
	if ( ss < 0 || ss > 59 )
		return( -1 );

	sprintf( data_ptr, "%02d:%02d:%02d", hh, mm, ss);
	ra [ 1 ] += 8;

	return( 0 );
}

/*
 �@�@�\�F�j���ԍ����當����𓾂�
 �߂�l�F�Ȃ�
*/
static void Dayasc( long data, long adr )
{
	char	*data_ptr;

	data_ptr = prog_ptr + adr;

	switch( data ) {
		case 0:
			strcpy( data_ptr, "��" );
			break;
		case 1:
			strcpy( data_ptr, "��" );
			break;
		case 2:
			strcpy( data_ptr, "��" );
			break;
		case 3:
			strcpy( data_ptr, "��" );
			break;
		case 4:
			strcpy( data_ptr, "��" );
			break;
		case 5:
			strcpy( data_ptr, "��" );
			break;
		case 6:
			strcpy( data_ptr, "�y" );
			break;
		default:
			ra [ 1 ] -= 2;
			break;
	}
	ra [ 1 ] += 2;
}

/*
 �@�@�\�F�x�N�^�E�e�[�u��������������
 �߂�l�F�ݒ�O�̏����A�h���X
*/
static long Intvcs( long no, long adr )
{
	long	adr2;
	long	mae = 0;
	short	save_s;

	no &= 0xFFFF;
	adr2 = no * 4;
	save_s = SR_S_REF();
	SR_S_ON();
	mae = mem_get( adr2, S_LONG );
	mem_set( adr2, adr, S_LONG );
	if ( save_s == 0 )
		SR_S_OFF();

	return( mae );
}

/*
 �@�@�\�FDMA�]��������
 �߂�l�F�ݒ�O�̏����A�h���X
*/
static void Dmamove( long md, long size, long adr1, long adr2 )
{
	char	*p1;
	char	*p2;
	long	tmp;

	if ( (md & 0x80) != 0 ) {
		/* adr1 -> adr2�]���ɂ��� */
		tmp = adr1;
		adr1 = adr2;
		adr2 = tmp;
	}

	/* adr1,adr2���ɃC���N�������g���[�h�łȂ��ꍇ�͖��T�|�[�g */
	if ( (md & 0x0F) != 5 )
		return;

	p1 = prog_ptr + adr1;
	p2 = prog_ptr + adr2;
	memcpy( p2, p1, size );
}
