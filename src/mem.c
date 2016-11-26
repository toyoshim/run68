/* $Id: mem.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:08  masamic
 * First imported source code and docs
 *
 * Revision 1.4  1999/12/07  12:47:22  yfujii
 * *** empty log message ***
 *
 * Revision 1.4  1999/11/29  06:18:06  yfujii
 * Calling CloseHandle instead of fclose when abort().
 *
 * Revision 1.3  1999/11/01  06:23:33  yfujii
 * Some debugging functions are introduced.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <stdlib.h>
#include "run68.h"

static	int	mem_red_chk( long );
static	int	mem_wrt_chk( long );
void	run68_abort( long );

static  UChar graphic_palette[512];

#if defined(EMSCRIPTEN)
extern void jsrt_io_graphic_data(UShort page, ULong index, UShort color);
extern void jsrt_io_graphic_palette(UShort index, UShort color);
extern void jsrt_io_sprite_data(ULong index, UShort data);
#endif

/*
 �@�@�\�FPC�̎w������������C���f�b�N�X���W�X�^�{8�r�b�g�f�B�X�v���[�X�����g
 �@�@�@�@�̒l�𓾂�
 �߂�l�F���̒l
*/
long idx_get()
{
	char	*mem;
	char	idx2;
	char	idx_reg;
	long	idx;

	mem = prog_ptr + pc;
	idx2 = *(mem++);
	idx_reg = ((idx2 >> 4) & 0x07);
	if ( (idx2 & 0x80) == 0 )
		idx = rd [ idx_reg ];
	else
		idx = ra [ idx_reg ];
	if ( (idx2 & 0x08) == 0 ) {	/* WORD */
		if ((idx & 0x8000) != 0)
			idx |= 0xFFFF0000;
		else
			idx &= 0x0000FFFF;
	}
	pc += 2;

	return( idx + *mem );
}

/*
 �@�@�\�FPC�̎w������������w�肳�ꂽ�T�C�Y�̃C�~�f�B�G�C�g�f�[�^���Q�b�g���A
 �@�@�@�@�T�C�Y�ɉ�����PC��i�߂�
 �߂�l�F�f�[�^�̒l
*/
long imi_get( char size )
{
	UChar	*mem;
	long	d;

	mem = (UChar *)prog_ptr + pc;

	switch( size ) {
		case S_BYTE:
			pc += 2;
			return( *(mem + 1) );
		case S_WORD:
			pc += 2;
			d = *(mem++);
			d = ((d << 8) | *mem);
			return( d );
		default:	/* S_LONG */
			pc += 4;
			d = *(mem++);
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *mem);
			return( d );
	}
}

/*
 �@�@�\�F����������w�肳�ꂽ�T�C�Y�̃f�[�^���Q�b�g����
 �߂�l�F�f�[�^�̒l
*/
long mem_get( long adr, char size )
{
	UChar   *mem;
	long	d;

	if ( adr < ENV_TOP || adr >= mem_aloc ) {
		if ( mem_red_chk( adr ) == FALSE )
			return( 0 );
	}
    if ( 0xE82000 <= adr && adr < 0xE82200 )
        mem = &graphic_palette[adr - 0xE82000];
    else
        mem = (UChar *)prog_ptr + adr;

	switch( size ) {
		case S_BYTE:
			return( *mem );
		case S_WORD:
			d = *(mem++);
			d = ((d << 8) | *mem);
			return( d );
		default:	/* S_LONG */
			d = *(mem++);
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *mem);
			return( d );
	}
}

/*
 �@�@�\�F�������Ɏw�肳�ꂽ�T�C�Y�̃f�[�^���Z�b�g����
 �߂�l�F�Ȃ�
*/
void mem_set( long adr, long d, char size )
{
	UChar   *mem;

#if defined(EMSCRIPTEN)
    if ( 0xD00000 <= adr && adr < 0xD80000 ) {   // Graphic Page 2
        if ( size != S_WORD )
            abort();
        jsrt_io_graphic_data( 2, ( adr - 0xD00000 ) / 2, d );
        return;
    }
    if ( 0xEB0000 <= adr && adr < 0xEB0400 ) {   // Sprite Control
        if ( size != S_WORD )
            abort();
        jsrt_io_sprite_data( ( adr - 0xEB0000 ) / 2, d );
    }
#endif
	if ( adr < ENV_TOP || adr >= mem_aloc ) {
		if ( mem_wrt_chk( adr ) == FALSE )
			return;
	}
    if ( 0xE82000 <= adr && adr < 0xE82200 )  // Graphic Palette
        mem = &graphic_palette[adr - 0xE82000];
    else
        mem = (UChar *)prog_ptr + adr;

	switch( size ) {
		case S_BYTE:
			*mem = (d & 0xFF);
            break;
		case S_WORD:
			*(mem++) = ((d >> 8) & 0xFF);
			*mem = (d & 0xFF);
            break;
		default:	/* S_LONG */
			*(mem++) = ((d >> 24) & 0xFF);
			*(mem++) = ((d >> 16) & 0xFF);
			*(mem++) = ((d >> 8) & 0xFF);
			*mem = (d & 0xFF);
            break;
	}
#if defined(EMSCRIPTEN)
    if ( 0xE82000 <= adr && adr < 0xE82200 ) {  // Graphic Palette
        if ( size != S_WORD || ( ( adr & 1 ) != 0 ) )
            abort();
        jsrt_io_graphic_palette( ( adr - 0xE82000 ) / 2, d );
    }
#endif

}

/*
 �@�@�\�F�ǂݍ��݃A�h���X�̃`�F�b�N
 �߂�l�F TRUE = OK
         FALSE = NG�����A0��ǂݍ��񂾂Ƃ݂Ȃ�
*/
static int mem_red_chk( long adr )
{
	char message[256];

	adr &= 0x00FFFFFF;
    if ( ( 0xE82000 <= adr && adr < 0xE82200 ) )
        return( TRUE );
	if ( ( 0xE82400 > adr && adr >= 0xE82200 ) ||
         ( 0xEBC000 > adr && adr >= 0xEB8000 ) ||
         ( 0xE82500 == adr ) ) {
		return( FALSE );
    }
	if ( adr >= 0xC00000 ) {
		if ( ini_info.io_through == TRUE )
			return( FALSE );
		sprintf(message, "I/O�|�[�gorROM($%06X)����ǂݍ������Ƃ��܂����B", adr);
		err68(message);
		run68_abort( adr );
	}
	if ( SR_S_REF() == 0 || adr >= mem_aloc ) {
		sprintf(message, "�s���A�h���X($%06X)����̓ǂݍ��݂ł��B", adr);
		err68(message);
		run68_abort( adr );
	}
	return( TRUE );
}

/*
 �@�@�\�F�������݃A�h���X�̃`�F�b�N
 �߂�l�F TRUE = OK
         FALSE = NG�����A�����������܂���OK�Ƃ݂Ȃ�
*/
static int mem_wrt_chk( long adr )
{
	char message[256];

	adr &= 0x00FFFFFF;
    if ( ( 0xE82000 <= adr && adr < 0xE82200 ) )   // Graphic Palette
        return( TRUE );
    if ( ( 0xD00000 <= adr && adr < 0xD80000 ) ||  // Graphic Page 2
	     ( 0xEB0000 <= adr && adr < 0xEB0400 ) )   // Sprite Control
        return( FALSE );
	if ( ( 0xE82400 > adr && adr >= 0xE82000 ) ||
	     ( 0xEBC000 > adr && adr >= 0xEB0400 ) ||
         ( 0xE82500 == adr ) ) {
		return( FALSE );
    }
	if ( adr >= 0xC00000 ) {
		if ( ini_info.io_through == TRUE )
			return( FALSE );
/*
		if ( adr == 0xE8A01F )	/# RESET CONTROLLER #/
			return( FALSE );
*/
		sprintf(message, "I/O�|�[�gorROM($%06X)�ɏ����������Ƃ��܂����B", adr);
		err68(message);
		run68_abort(adr);
	}
	if ( SR_S_REF() == 0 || adr >= mem_aloc ) {
		sprintf(message, "�s���A�h���X�ւ̏������݂ł�($%06X)", adr);
		err68(message);
		run68_abort( adr );
	}
	return( TRUE );
}

/*
 �@�\�F�ُ�I������
*/
void run68_abort( long adr )
{
	int	i;

	fprintf( stderr, "�A�h���X�F%08X\n", adr );

	for ( i = 5; i < FILE_MAX; i ++ ) {
		if ( finfo [ i ].fh != NULL )
#if defined(WIN32)
			CloseHandle(finfo [ i ].fh);
#else
			fclose(finfo [ i ].fh);
#endif
	}

#ifdef	TRACE
	printf( "d0-7=%08lx" , rd [ 0 ] );
	for ( i = 1; i < 8; i++ ) {
		printf( ",%08lx" , rd [ i ] );
	}
	printf("\n");
	printf( "a0-7=%08lx" , ra [ 0 ] );
	for ( i = 1; i < 8; i++ ) {
		printf( ",%08lx" , ra [ i ] );
	}
	printf("\n");
	printf( "  pc=%08lx    sr=%04x\n" , pc, sr );
#endif
#ifdef EMSCRIPTEN
    abort();
#else
	longjmp(jmp_when_abort, 2);
#endif
}
