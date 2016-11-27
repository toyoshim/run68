/* $Id: conditions.c,v 1.3 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2009/08/05 14:44:33  masamic
 * Some Bug fix, and implemented some instruction
 * Following Modification contributed by TRAP.
 *
 * Fixed Bug: In disassemble.c, shift/rotate as{lr},ls{lr},ro{lr} alway show word size.
 * Modify: enable KEYSNS, register behaiviour of sub ea, Dn.
 * Add: Nbcd, Sbcd.
 *
 * Revision 1.1.1.1  2001/05/23 11:22:05  masamic
 * First imported source code and docs
 *
 * Revision 1.2  1999/12/07  12:39:54  yfujii
 * *** empty log message ***
 *
 * Revision 1.2  1999/11/29  06:24:04  yfujii
 * Condition code operations are modified to be correct.
 *
 *
 */

// /* conditions.c */

// void general_conditions(long result, int size);
// void add_conditions(long src , long dest, long result, int size, BOOL zero_flag);
// void cmp_conditions(long src , long dest, long result, int size);
// void sub_conditions(long src , long dest, long result, int size, BOOL zero_flag);
// void neg_conditions(long dest, long result, int size, BOOL zero_flag);
// void check(char *mode, long src, long dest, long result, int size, short before);

#include "run68.h"

static void ccr2bitmap(short ccr, char *bitmap) {
	int  i;
	int  flag;
	int  j = 0;

	ccr &= 0x1f;

	for (i = 6; i >= 0; i--) {
		flag = (ccr >> i) & 1;
		if (flag == 1) {
			bitmap[j++] = '1';
		} else {
			bitmap[j++] = '0';
		}
	}
	bitmap[j] = '\0';
}

void check(char *mode, long src, long dest, long result, int size, short before) {
	char  befstr[9];
	char  aftstr[9];

	ccr2bitmap((short)(before & 0x1f), befstr);
	ccr2bitmap((short)(sr & 0x1f), aftstr);

	printf("%s: 0x%08x 0x%08x 0x%08x %1d %8s %8s\n", mode, src, dest, result, size, befstr, aftstr);
}

long getMSB(long num, int size) {

	long ret;

	switch (size) {
		case S_BYTE:
			ret = ((num >> 7) & 1);
			break;
		case S_WORD:
			ret = ((num >> 15) & 1);
			break;
		case S_LONG:
			ret = ((num >> 31) & 1);
			break;
		default:
			err68a("�s���ȃf�[�^�T�C�Y�ł��B", __FILE__, __LINE__);
	}

	return(ret);
}

long getBitsByDataSize(long num, int size) {
	long ret;
	switch (size) {
		case S_BYTE:
			ret = num & 0xff;
			break;
		case S_WORD:
			ret = num & 0xffff;
			break;
		case S_LONG:
			ret = num;
			break;
		default:
			err68a("�s���ȃf�[�^�T�C�Y�ł��B", __FILE__, __LINE__);
	}
	return(ret);
}



/*
 * �y�����z
 *   ��ʌn�R���f�B�V�����t���O�̐ݒ�
 *
 * �y���W�X�^�̕ω��z
 *   X: �ω��Ȃ�
 *   N: �����̂Ƃ�ON�A��܂��͐����̂Ƃ�OFF
 *   Z: ��̂Ƃ�ON�A��ȊO�̂Ƃ�OFF
 *   V: ���0
 *   C: ���0
 *
 * �y�֐������z
 *   general_conditions(result, size);
 *
 * �y�����z
 *   long result;    <in>  Result�l
 *   int  size;      <in>  �A�N�Z�X�T�C�Y
 *
 * �y�Ԓl�z
 *   �Ȃ�
 *
 */

void general_conditions(long result, int size) {

	int 	Rm;

	Rm = (getMSB(result, size) != (long)0);

	/* Overflow Flag */
	CCR_V_OFF();

	/* Carry Flag & Extend Flag */
	CCR_C_OFF();
//	CCR_X_OFF();

	/* Zero Flag */
	if (getBitsByDataSize(result, size) == 0) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
	}

	/* Negative Flag */
	if (Rm != 0) {
		CCR_N_ON();
	} else {
		CCR_N_OFF();
	}
}

/*
 * �y�����z
 *   add�n�R���f�B�V�����t���O�̐ݒ�
 *
 * �y�֐������z
 *   add_conditions(src, dest, result, size, zero_flag);
 *
 * �y�����z
 *   long src;       <in>  Source�l
 *   long dest;      <in>  Destination�l
 *   long result;    <in>  Result�l
 *   int  size;      <in>  �A�N�Z�X�T�C�Y
 *   BOOL zero_flag; <in>  addx�p���Z�O zero flag �l�B
 *                         ���̑��̏ꍇ�͏�� 1 ���w��̂��ƁB
 *
 * �y�Ԓl�z
 *   �Ȃ�
 *
 */

void add_conditions(long src, long dest, long result, int size, BOOL zero_flag) {

	int 	Sm, Dm, Rm;

	Sm = (getMSB(src,    size) != (long)0);
	Dm = (getMSB(dest,   size) != (long)0);
	Rm = (getMSB(result, size) != (long)0);

	/* Overflow Flag */
	if ((Sm && Dm && !Rm) || (!Sm && !Dm && Rm)) {
		CCR_V_ON();
	} else {
		CCR_V_OFF();
	}

	/* Carry Flag & Extend Flag */
	if ((Sm && Dm) || (Dm && !Rm) || (Sm && !Rm)) {
		CCR_C_ON();
		CCR_X_ON();
	} else {
		CCR_C_OFF();
		CCR_X_OFF();
	}

	/* Zero Flag */
	if (zero_flag && getBitsByDataSize(result, size) == 0) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
	}

	/* Negative Flag */
	if (Rm != 0) {
		CCR_N_ON();
	} else {
		CCR_N_OFF();
	}
}


/*
 * �y�����z
 *   cmp�n�R���f�B�V�����t���O�̐ݒ�
 *
 * �y�֐������z
 *   cmp_conditions(src, dest, result, size, zero_flag);
 *
 * �y�����z
 *   long src;       <in>  Source�l
 *   long dest;      <in>  Destination�l
 *   long result;    <in>  Result�l
 *   int  size;      <in>  �A�N�Z�X�T�C�Y
 *   BOOL zero_flag; <in>  subx�p���Z�O zero flag �l�B
 *                         ���̑��̏ꍇ�͏�� 1 ���w��̂��ƁB
 *
 * �y�Ԓl�z
 *   �Ȃ�
 *
 */

void cmp_conditions(long src, long dest, long result, int size) {
	int 	Sm, Dm, Rm;

	Sm = (getMSB(src,    size) != (long)0);
	Dm = (getMSB(dest,   size) != (long)0);
	Rm = (getMSB(result, size) != (long)0);

	/* Overflow Flag */
	if ((!Sm && Dm && !Rm) || (Sm && !Dm && Rm)) {
		CCR_V_ON();
	} else {
		CCR_V_OFF();
	}

	/* Carry Flag & Extend Flag */
	if ((Sm && !Dm) || (!Dm && Rm) || (Sm && Rm)) {
		CCR_C_ON();
	} else {
		CCR_C_OFF();
	}

	/* Zero Flag */
	if (getBitsByDataSize(result, size) == 0) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
	}

	/* Negative Flag */
	if (Rm != 0) {
		CCR_N_ON();
	} else {
		CCR_N_OFF();
	}
}


/*
 * �y�����z
 *   sub�n�R���f�B�V�����t���O�̐ݒ�
 *
 * �y�֐������z
 *   sub_conditions(src, dest, result, size, zero_flag);
 *
 * �y�����z
 *   long src;       <in>  Source�l
 *   long dest;      <in>  Destination�l
 *   long result;    <in>  Result�l
 *   int  size;      <in>  �A�N�Z�X�T�C�Y
 *   BOOL zero_flag; <in>  subx�p���Z�O zero flag �l�B
 *                         ���̑��̏ꍇ�͏�� 1 ���w��̂��ƁB
 *
 * �y�Ԓl�z
 *   �Ȃ�
 *
 */

void sub_conditions(long src, long dest, long result, int size, BOOL zero_flag) {

	cmp_conditions(src, dest, result, size);

	if (CCR_C_REF()) {
		CCR_X_ON();
	} else {
		CCR_X_OFF();
	}

	/* Zero Flag */
	if ((zero_flag == 1) && (CCR_Z_REF() != 0)) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
	}
	
}

/*
 * �y�����z
 *   neg�n�R���f�B�V�����t���O�̐ݒ�
 *
 * �y�֐������z
 *   neg_conditions(dest, result, size, zero_flag);
 *
 * �y�����z
 *   long dest;      <in>  Destination�l
 *   long result;    <in>  Result�l
 *   int  size;      <in>  �A�N�Z�X�T�C�Y
 *   BOOL zero_flag; <in>  negx�p���Z�O zero flag �l�B
 *                         ���̑��̏ꍇ�͏�� 1 ���w��̂��ƁB
 *
 * �y�Ԓl�z 
 *   �Ȃ�
 *
 */

void neg_conditions(long dest, long result, int size, BOOL zero_flag) {

	int 	Dm, Rm;

	Dm = (getMSB(dest,   size) != (long)0);
	Rm = (getMSB(result, size) != (long)0);

	/* Overflow Flag */
	if (Dm && Rm) {
		CCR_V_ON();
	} else {
		CCR_V_OFF();
	}

	/* Carry Flag & Extend Flag */
	if (Dm || Rm) {
		CCR_C_ON();
		CCR_X_ON();
	} else {
		CCR_C_OFF();
		CCR_X_OFF();
	}

	/* Zero Flag */
	if (getBitsByDataSize(result, size) == 0) {
		CCR_Z_ON();
	} else {
		CCR_Z_OFF();
	}

	/* Negative Flag */
	if (Rm != 0) {
		CCR_N_ON();
	} else {
		CCR_N_OFF();
	}
}
