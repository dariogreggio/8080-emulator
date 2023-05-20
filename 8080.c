//https://pastraiser.com/cpu/i8080/i8080_opcodes.html

//v. metodo usato su 68000 senza byte alto... provare

// 19/5/23

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include <graph.h>
//#include <dos.h>
//#include <malloc.h>
//#include <memory.h>
//#include <fcntl.h>
//#include <io.h>
#include <xc.h>


#include "8080_PIC.h"
#include "Adafruit_ST77xx.h"      // per LOBYTE ecc...


#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )


extern BYTE fExit;
extern BYTE debug;

extern volatile BYTE keysFeedPtr;
extern volatile BYTE TIMIRQ;

BYTE DoReset=0,DoIRQ=0,DoNMI=0,DoHalt=0,DoWait=0;
#define MAX_WATCHDOG 100      // x30mS v. sotto
WORD WDCnt=MAX_WATCHDOG;
BYTE ColdReset=1;
BYTE Pipe1;
union /*__attribute__((__packed__))*/ {
	SWORD x;
	BYTE bb[4];
	struct /*__attribute__((__packed__))*/ {
		BYTE l;
		BYTE h;
//		BYTE u;		 bah no, sposto la pipe quando ci sono le istruzioni lunghe 4...
		} b;
	} Pipe2;





union /*__attribute__((__packed__))*/ Z_REG {
  SWORD x;
  struct /*__attribute__((__packed__))*/ { 
    BYTE l;
    BYTE h;
    } b;
//    } _B1,_D1,_H1,_PSW1,_PSW2,_B2,_D2,_H2;
  };
union /*__attribute__((__packed__))*/ Z_REGISTERS {
  BYTE  b[8];
  union Z_REG r[4];
  };

#define ID_CARRY 0x1
#define ID_Parity 0x4
#define ID_AUXCARRY 0x10
#define ID_ZERO 0x40
#define ID_SIGN 0x80
union /*__attribute__((__packed__))*/ REGISTRO_F {
  BYTE b;
  struct /*__attribute__((__packed__))*/ {
    unsigned int Carry: 1;
    unsigned int unused: 1;		// always 1
    unsigned int Parity: 1;   // 1=pari 0=dispari
    unsigned int unused2: 1;
    unsigned int AuxCarry: 1;
    unsigned int unused3: 1;
    unsigned int Zero: 1;
    unsigned int Sign: 1;
    };
  };
union /*__attribute__((__packed__))*/ OPERAND {
  BYTE *reg8;
  WORD *reg16;
  WORD mem;
  };
union /*__attribute__((__packed__))*/ RESULT {
  struct /*__attribute__((__packed__))*/ {
    BYTE l;
    BYTE h;
    } b;
  WORD x;
  DWORD d;
  };
    
// http://clrhome.org/table/
// https://wikiti.brandonw.net/index.php?title=Z80_Instruction_Set
int Emulate(int mode) {
/*Registers (http://www.z80.info/z80syntx.htm#LD)
--------------
 A = 111
 B = 000
 C = 001
 D = 010
 E = 011
 H = 100
 L = 101*/
#define _a regs1.r[3].b.l //
	// in TEORIA, REGISTRO_F dovrebbe appartenere qua... ho patchato pop af push ecc, SISTEMARE!! _f.b
  // n.b. anche che A/F sono invertiti rispetto agli altri, v. push/pop
#define _f_PSW regs1.r[3].b.h //
#define _b regs1.r[0].b.h
#define _c regs1.r[0].b.l
#define _d regs1.r[1].b.h
#define _e regs1.r[1].b.l
#define _h regs1.r[2].b.h
#define _l regs1.r[2].b.l
#define _PSW regs1.r[3].x
#define _B regs1.r[0].x
#define _D regs1.r[1].x
#define _H regs1.r[2].x
#define WORKING_REG regs1.b[((Pipe1 & 0x38) ^ 8) >> 3]      // la parte bassa/alta è invertita...
#define WORKING_REG2 regs1.b[(Pipe1 ^ 1) & 7]
#define WORKING_BITPOS (1 << ((Pipe2.b.l & 0x38) >> 3))
#define WORKING_BITPOS2 (1 << ((Pipe2.b.h & 0x38) >> 3))
    
	SWORD _pc=0;
	SWORD _sp=0;
	BYTE _i,_r;
	BYTE IRQ_Mode=0;
	BYTE IRQ_Enable1=0,IRQ_Enable2=0;
  union Z_REGISTERS regs1,regs2;
  union RESULT res1,res2,res3;
//  union OPERAND op1,op2;
	union REGISTRO_F _f;
	union REGISTRO_F _f1;
	/*register*/ SWORD i;
  int c=0;


	_pc=0;
  
  
  
  
	do {

		c++;
		if(!(c & 0x3ffff)) {
      ClrWdt();
// yield()
#ifndef USING_SIMULATOR      
			UpdateScreen(1);
#endif
      LED1^=1;    // 42mS~ con SKYNET 7/6/20; 10~mS con Z80NE 10/7/21; 35mS GALAKSIJA 16/10/22; 30mS ZX80 27/10/22
      // QUADRUPLICO/ecc! 27/10/22
      
      }

		if(ColdReset) {
			ColdReset=0;
      DoReset=1;
			continue;
      }



    
		/*
		if((_pc >= 0xa000) && (_pc <= 0xbfff)) {
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
			*/
		if(debug) {
//			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
		/*if(kbhit()) {
			getch();
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			printf("281-284: %02x %02x %02x %02x\n",*(p1+0x281),*(p1+0x282),*(p1+0x283),*(p1+0x284));
			printf("2b-2c: %02x %02x\n",*(p1+0x2b),*(p1+0x2c));
			printf("33-34: %02x %02x\n",*(p1+0x33),*(p1+0x34));
			printf("37-38: %02x %02x\n",*(p1+0x37),*(p1+0x38));
			}*/
		if(DoReset) {
			_pc=0;
      _i=_r=0;
			IRQ_Enable1=0;IRQ_Enable2=0;
     	IRQ_Mode=0;
			DoReset=0;DoHalt=0;//DoWait=0;
      keysFeedPtr=255; //meglio ;)
      continue;
			}
		if(DoNMI) {
			DoNMI=0; DoHalt=0;
			IRQ_Enable2=IRQ_Enable1; IRQ_Enable1=0;
			PutValue(--_sp,HIBYTE(_pc));
			PutValue(--_sp,LOBYTE(_pc));
			_pc=0x0066;
			}
		if(DoIRQ) {
      
      // LED2^=1;    // 
      DoHalt=0;     // 
      
			if(IRQ_Enable1) {
				IRQ_Enable1=0;    // ma non sono troppo sicuro... boh?
				DoIRQ=0;
				PutValue(--_sp,HIBYTE(_pc));
				PutValue(--_sp,LOBYTE(_pc));
				switch(IRQ_Mode) {
				  case 0:
						i=0 /*bus_dati*/;
						// DEVE ESEGUIRE i come istruzione!!
				  	break;
				  case 1:
				  	_pc=0x0038;
				  	break;
				  case 2:
				  	_pc=(((SWORD)_i) << 8) | (0 /*bus_dati*/ << 1) | 0;
				  	break;
				  }
				}
			}

		// buttare fuori il valore di _r per il refresh... :)
    _r++;
  
		if(DoHalt) {
      //mettere ritardino per analogia con le istruzioni?
//      __Dlay_ns(500); non va più nulla... boh...
			continue;		// esegue cmq IRQ e refresh
      }
		if(DoWait) {
      //mettere ritardino per analogia con le istruzioni?
//      __Dlay_ns(100);
			continue;		// esegue cmq IRQ?? penso di no... sistemare
      }

//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
    
    
      if(!SW1) {        // test tastiera, me ne frego del repeat/rientro :)
       // continue;
        __Dlay_ms(100); ClrWdt();
        DoReset=1;
        }
      if(!SW2) {        // test tastiera
        if(keysFeedPtr==255)      // debounce...
          keysFeedPtr=254;
        }

      LED2^=1;    // ~700nS 7/6/20, ~600 con 32bit 10/7/21 MA NON FUNZIONA/visualizza!! verificare; 5-700nS 27/10/22

    
/*      if(_pc == 0x069d ab5 43c Cd3) {
        ClrWdt();
        }*/
  
		switch(GetPipe(_pc++)) {
			case 0:   // NOP
			case 0x8:
			case 0x10:
			case 0x18:
			case 0x20:
			case 0x28:
			case 0x30:
			case 0x38:
				break;

			case 1:   // LXI B,nn ecc
			case 0x11:
			case 0x21:
#define WORKING_REG16 regs1.r[(Pipe1 & 0x30) >> 4].x
			  WORKING_REG16=Pipe2.x;
			  _pc+=2;
				break;

			case 2:   // LD (BC),a
			  PutValue(_B,_a);
				break;

			case 3:   // INX B ecc
			case 0x13:
			case 0x23:
				WORKING_REG16 ++;
				break;

			case 4:   // INR B ecc
			case 0xc:
			case 0x14:
			case 0x1c:
			case 0x24:
			case 0x2c:
			case 0x3c:
				WORKING_REG ++;
        res3.b.l=WORKING_REG;
        
aggInc:
      	_f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
      	_f.Parity= res3.b.l == 0x80 ? 1 : 0; //P = 1 if x=7FH before, else 0
      	_f.AuxCarry= (res3.b.l & 0xf) == 0 ? 1 : 0; // INC x      1 if carry from bit 3 else 0 
        break;

			case 5:   // DCR B ecc
			case 0xd:
			case 0x15:
			case 0x1d:
			case 0x25:
			case 0x2d:
			case 0x3d:
				WORKING_REG --;
        res3.b.l=WORKING_REG;
        
aggDec:
      	_f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
      	_f.Parity= res3.b.l == 0x7f ? 1 : 0; //P = 1    // DEC x         P = 1 if x=80H before, else 0
      	_f.AuxCarry= (res3.b.l & 0xf) == 0xf ? 1 : 0; // DEC x      1 if borrow from bit 4 else 0 
				break;

			case 6:   // LD B,n ecc
			case 0xe:
			case 0x16:
			case 0x1e:
			case 0x26:
			case 0x2e:
			case 0x3e:
			  WORKING_REG=Pipe2.b.l;
			  _pc++;
				break;

			case 7:   // RLCA
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f.Carry;
        
aggRotate:
        _f.AuxCarry=0;
				break;
                                         
			case 8:   // EX AF,AF'
        _f_PSW=_f.b;
        
			  res3.x=_PSW;
				_PSW=regs2.r[3].x;
				regs2.r[3].x=res3.x;

        _f.b=_f_PSW;
				break;

			case 9:   // ADD HL,BC ecc
			case 0x19:
			case 0x29:
        res1.x=_H;
        res2.x=WORKING_REG16;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
			  _H = res3.x;
        _f.AuxCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
        break;

			case 0xa:   // LD A,(BC)
			  _a=GetValue(_B);
				break;

      case 0xb:   // DEC BC ecc
      case 0x1b:
      case 0x2b:
				WORKING_REG16 --;
				break;

			case 0xf:   // RRCA
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x10:   // DJNZ
				_pc++;
			  _b--;
				if(_b)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x12:    // LD (DE),A
			  PutValue(_D,_a);
				break;

      case 0x17:    // RLA
				_f1=_f;
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f1.Carry;
        goto aggRotate;
				break;

      case 0x18:    // JR
				_pc++;
				_pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x1a:    // LD A,(DE)
			  _a=GetValue(_D);
				break;

			case 0x1f:    // RRA
				_f1=_f;
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f1.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x20:    // JR nz
				_pc++;
				if(!_f.Zero)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x22:    // LD(nn),HL
			  PutIntValue(Pipe2.x,_H);
			  _pc+=2;
				break;

			case 0x27:		// DAA
        res3.x=res1.x=_a;
        i=_f.Carry;
        _f.Carry=0;
        if((_a & 0xf) > 9 || _f.AuxCarry) {
          res3.x+=6;
          _a=res3.b.l;
          _f.Carry= i || HIBYTE(res3.x);
          _f.AuxCarry=1;
          }
        else
          _f.AuxCarry=0;
        if((res1.b.l>0x99) || i) {
          _a+=0x60;  
          _f.Carry=1;
          }
        else
          _f.Carry=0;
        goto calcParity;
				break;

      case 0x28:    // JR z
				_pc++;
        if(_f.Zero)
				  _pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x2a:    // LD HL,(nn)
			  _H=GetIntValue(Pipe2.x);
			  _pc+=2;
				break;

  		case 0x2f:    // CPL
        _a=~_a;
        _f.AuxCarry=1;
				break;
        
			case 0x30:    // JR nc
				_pc++;
				if(!_f.Carry)
					_pc+=(signed char)Pipe2.b.l;
				break;

			case 0x31:    // LD SP,nn (v. anche HL ecc)
			  _sp=Pipe2.x;
			  _pc+=2;
				break;

			case 0x32:    // LD (nn),A
			  PutValue(Pipe2.x,_a);
			  _pc+=2;
				break;

			case 0x33:    // INC SP (v. anche INC BC ecc)
			  _sp++;
				break;

			case 0x34:    // INC (HL)
        res3.b.l=GetValue(_H)+1;
			  PutValue(_H,res3.b.l);
        goto aggInc;
				break;

			case 0x35:    // DEC (HL)
        res3.b.l=GetValue(_H)-1;
			  PutValue(_H,res3.b.l);
        goto aggDec;
				break;

			case 0x36:    // LD (HL),n
			  PutValue(_H,Pipe2.b.l);
			  _pc++;
				break;
              
      case 0x37:    // SCF
        _f.Carry=1;
        _f.AuxCarry=0;
        break;
                                   
      case 0x38:    // JR c
				_pc++;
        if(_f.Carry)
				  _pc+=(signed char)Pipe2.b.l;
				break;
				
			case 0x39:    // ADD HL,SP
        res1.x=_H;
        res2.x=_sp;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
        _H = res3.x;
        _f.AuxCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
				break;

			case 0x3a:    // LD A,(nn)
			  _a=GetValue(Pipe2.x);
			  _pc+=2;
				break;

      case 0x3b:    // DEC SP (v. anche DEC BC ecc)
			  _sp--;
				break;

      case 0x3f:    // CCF
        _f.AuxCarry=_f.Carry;
        _f.Carry=!_f.Carry;
        break;
                                   
			case 0x40:    // LD r,r
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x47:
			case 0x48:                             // 
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4f:
			case 0x50:                             // 
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x57:
			case 0x58:                             // 
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5f:
			case 0x60:                             // 
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x67:
			case 0x68:                             // 
			case 0x69:
			case 0x6a:
			case 0x6b:
			case 0x6c:
			case 0x6d:
			case 0x6f:
			case 0x78:                             // 
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7f:
				WORKING_REG=WORKING_REG2;
				break;

			case 0x46:    // LD r,(HL)
			case 0x4e:
			case 0x56:
			case 0x5e:
			case 0x66:
			case 0x6e:
			case 0x7e:
				WORKING_REG=GetValue(_H);
				break;

			case 0x70:                             // 
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x77:
				PutValue(_H,/* regs1.b[((Pipe1 & 7) +1) & 7]*/ WORKING_REG2);
				break;
        
			case 0x76:    // HALT
			  DoHalt=1;
				break;

			case 0x80:    // ADD A,r
			case 0x81:
			case 0x82:
			case 0x83:
			case 0x84:
			case 0x85:
			case 0x87:
        res2.b.l=WORKING_REG2;
        
aggSomma:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 

aggFlagB:
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x40) != !!(((res1.b.l & 0x80) + (res2.b.l & 0x80)) & 0x80);
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
  //(M^result)&(N^result)&0x80 is nonzero. That is, if the sign of both inputs is different from the sign of the result. (Anding with 0x80 extracts just the sign bit from the result.) 
  //Another C++ formula is !((M^N) & 0x80) && ((M^result) & 0x80)
//        _f.Parity = !!((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80);
//        _f.Parity = !!(!((res1.b.l ^ res2.b.l) & 0x80) && ((res1.b.l ^ res3.b.l) & 0x80));
//**        _f.Parity = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
  // Calculate the overflow by sign comparison.
/*  carryIns = ((a ^ b) ^ 0x80) & 0x80;
  if (carryIns) // if addend signs are the same
  {
    // overflow if the sum sign differs from the sign of either of addends
    carryIns = ((*acc ^ a) & 0x80) != 0;
  }*/
	// per overflow e AuxCarry https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80
/*The overflow checks the most significant bit of the 8 bit result. This is the sign bit. If we add two negative numbers (MSBs=1) then the result should be negative (MSB=1), whereas if we add two positive numbers (MSBs=0) then the result should be positive (MSBs=0), so the MSB of the result must be consistent with the MSBs of the summands if the operation was successful, otherwise the overflow bit is set.*/        
/*        if(!_f.Sign) {
          _f.Parity=(res1.b.l & 0x80 || res2.b.l & 0x80) ? 1 : 0;
          }
        else {
          _f.Parity=(res1.b.l & 0x80 && res2.b.l & 0x80) ? 1 : 0;
          }*/
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.Parity=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.Parity=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
        
aggFlagBC:    // http://www.z80.info/z80sflag.htm
				_f.Carry=!!res3.b.h;
        
        _f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
				break;

			case 0x86:    // ADD A,(HL)
        res2.b.l=GetValue(_H);
        goto aggSomma;
				break;

			case 0x88:    // ADC A,r
			case 0x89:
			case 0x8a:
			case 0x8b:
			case 0x8c:
			case 0x8d:
			case 0x8f:
        res2.b.l=WORKING_REG2;
        
aggSommaC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x+_f.Carry;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di sì
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.Parity=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.Parity=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.Parity=0;*/
        goto aggFlagB;
				break;

			case 0x8e:    // ADC A,(HL)
        res2.b.l=GetValue(_H);
        goto aggSommaC;
				break;

			case 0x90:    // SUB A,r
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x97:
        res2.b.l=WORKING_REG2;
        
aggSottr:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _f.Parity = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
  			goto aggFlagBC;
				break;

			case 0x96:    // SUB A,(HL)
        res2.b.l=GetValue(_H);
				goto aggSottr;
				break;

			case 0x98:    // SBC A,r
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9f:
        res2.b.l=WORKING_REG2;
        
aggSottrC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x-_f.Carry;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0  ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di sì..
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        goto aggFlagB;
				break;

			case 0x9e:    // SBC A,(HL)
        res2.b.l=GetValue(_H);
				goto aggSottrC;
				break;

			case 0xa0:    // AND A,r
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			case 0xa5:
			case 0xa7:
				_a &= WORKING_REG2;
        _f.AuxCarry=1;
        
aggAnd:
        res3.b.l=_a;
aggAnd2:
        _f.Carry=0;
aggAnd3:      // usato da IN 
        _f.Zero=_a ? 0 : 1;
        _f.Sign=_a & 0x80 ? 1 : 0;
        // AuxCarry è 1 fisso se AND e 0 se OR/XOR
        
calcParity:
          {
          BYTE par;
          par= res3.b.l >> 1;			// Microchip AN774
          par ^= res3.b.l;
          res3.b.l= par >> 2;
          par ^= res3.b.l;
          res3.b.l= par >> 4;
          par ^= res3.b.l;
          _f.Parity=par & 1 ? 1 : 0;
          }
				break;

			case 0xa6:    // AND A,(HL)
				_a &= GetValue(_H);
        _f.AuxCarry=1;
        goto aggAnd;
				break;

			case 0xa8:    // XOR A,r
			case 0xa9:
			case 0xaa:
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xaf:
				_a ^= WORKING_REG2;
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xae:    // XOR A,(HL)
				_a ^= GetValue(_H);
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb0:    // OR A,r
			case 0xb1:
			case 0xb2:
			case 0xb3:
			case 0xb4:
			case 0xb5:
			case 0xb7:
				_a |= WORKING_REG2;
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb6:    // OR A,(HL)
				_a |= GetValue(_H);
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb8:    // CP A,r
			case 0xb9:
			case 0xba:
			case 0xbb:
			case 0xbc:
			case 0xbd:
			case 0xbf:
				res2.b.l=WORKING_REG2;
				goto compare;
				break;

			case 0xbe:    // CP A,(HL)
				res2.b.l=GetValue(_H);
  			goto compare;
				break;

			case 0xc0:    // RET NZ
			  if(!_f.Zero)
			    goto Return;
				break;

			case 0xc1:    // POP B ecc
			case 0xd1:    
			case 0xe1:    
#define WORKING_REG16B regs1.r[(Pipe1 & 0x30) >> 4].b
				WORKING_REG16B.l=GetValue(_sp++);
				WORKING_REG16B.h=GetValue(_sp++);
				break;
			case 0xf1:    
				_f.b=_f_PSW=GetValue(_sp++);
				_a=GetValue(_sp++);
				break;

			case 0xc2:    // JP nz
			  if(!_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xc3:    // JP
Jump:
				_pc=Pipe2.x;
				break;

			case 0xc4:    // CALL nz
			  if(!_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xc5:    // PUSH B ecc
			case 0xd5:    // 
			case 0xe5:    // 
				PutValue(--_sp,WORKING_REG16B.h);
				PutValue(--_sp,WORKING_REG16B.l);
				break;
			case 0xf5:    // push af..
        _f_PSW=_f.b;
				PutValue(--_sp,_a);
				PutValue(--_sp,_f_PSW);
				break;

			case 0xc6:    // ADD A,n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSomma;
				break;

			case 0xc7:    // RST
			case 0xcf:
			case 0xd7:
			case 0xdf:
			case 0xe7:
			case 0xef:
			case 0xf7:
			case 0xff:
			  i=Pipe1 & 0x38;
RST:
				PutValue(--_sp,HIBYTE(_pc));
				PutValue(--_sp,LOBYTE(_pc));
				_pc=i;
				break;
				
			case 0xc8:    // RET z
			  if(_f.Zero)
			    goto Return;
				break;

			case 0xc9:    // RET
Return:
        _pc=GetValue(_sp++);
        _pc |= ((SWORD)GetValue(_sp++)) << 8;
				break;

			case 0xca:    // JP z
			  if(_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;


			case 0xcb:
				break;

			case 0xcc:    // CALL z
			  if(_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xcd:		// CALL
			case 0xdd:
			case 0xed:      // 
			case 0xfd:

Call:
				i=Pipe2.x;
		    _pc+=2;
				goto RST;
				break;

			case 0xce:    // ADC A,n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSommaC;
				break;

			case 0xd0:    // RET nc
			  if(!_f.Carry)
			    goto Return;
				break;

			case 0xd2:    // JP nc
			  if(!_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xd3:    // OUT
				OutValue(MAKEWORD(Pipe2.b.l,_a),_a);
				_pc++;
				break;

			case 0xd4:    // CALL nc
			  if(!_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xd6:    // SUB n
  		  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSottr;
				break;

			case 0xd8:    // RET c
			  if(_f.Carry)
			    goto Return;
				break;

			case 0xd9:    // EXX
        {
        BYTE n;
        for(n=0; n<3; n++) {
          res3.x=regs2.r[n].x;
          regs2.r[n].x=regs1.r[n].x;
          regs1.r[n].x=res3.x;
          }
        }
				break;

			case 0xda:    // JP c
			  if(_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xdb:    // IN a,  NON tocca flag
				_pc++;
				_a=InValue(MAKEWORD(Pipe2.b.l,_a));
				break;

			case 0xdc:    // CALL c
			  if(_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xde:    // SBC A,n
				_pc++;
				res2.b.l=Pipe2.b.l;
        goto aggSottrC;
				break;

			case 0xe0:    // RET po
			  if(!_f.Parity)
			    goto Return;
				break;

			case 0xe2:    // JP po
			  if(!_f.Parity)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xe3:    // EX (SP),HL
				res3.x=GetIntValue(_sp);
				PutIntValue(_sp,_H);
				_H=res3.x;
				break;

			case 0xe4:    // CALL po
			  if(!_f.Parity)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xe6:    // AND A,n
				_a &= Pipe2.b.l;
        _f.AuxCarry=1;
				_pc++;
        goto aggAnd;
				break;

			case 0xe8:    // RET pe
			  if(_f.Parity)
			    goto Return;
				break;

			case 0xe9:    // JP (HL)
			  _pc=_H;
				break;

			case 0xea:    // JP pe
			  if(_f.Parity)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xeb:    // EX DE,HL
				res3.x=_D;
				_D=_H;
				_H=res3.x;
				break;

			case 0xec:    // CALL pe
			  if(_f.Parity)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xee:    // XOR A,n
				_a ^= Pipe2.b.l;
        _f.AuxCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf0:    // RET p
			  if(!_f.Sign)
			    goto Return;
				break;

			case 0xf2:    // JP p
			  if(!_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xf3:    // DI
			  IRQ_Enable1=IRQ_Enable2=0;
				break;

			case 0xf4:    // CALL p
			  if(!_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xf6:    // OR A,n
				_a |= Pipe2.b.l;
        _f.AuxCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf8:    // RET m
			  if(_f.Sign)
			    goto Return;
				break;

			case 0xf9:    // LD SP,HL
			  _sp=_H;
				break;

			case 0xfa:    // JP m
			  if(_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xfb:    // EI
			  IRQ_Enable1=IRQ_Enable2=1;
				break;

			case 0xfc:    // CALL m
			  if(_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xfe:    // CP n
				res2.b.l=Pipe2.b.l;
				_pc++;
        
compare:        
				res1.b.l=_a;
				res1.b.h=res2.b.h=0;
				res3.x=res1.x-res2.x;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
  			goto aggFlagBC;
				break;
			
			}
		} while(!fExit);
	}


