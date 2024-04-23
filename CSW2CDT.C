  //@@@@ //@@@@@//@@ //@@//@@@@  //@@@@//@@@@@ //@@@@@@ -----------------------
 //@@//@@/@@ //@@/@@ //@@/@@//@@//@@//@@//@@/@@//@/@@/@  codec of CDT/TZX v1.21
//@@    //@@    //@@ //@@   //@@/@@     //@@//@@ //@@    tape files from CSW v1
//@@     //@@@@@//@@/@/@@//@@@@//@@     //@@//@@ //@@    tape samples, coded by
//@@         //@@/@@@@@@@/@@   //@@     //@@//@@ //@@    Cesar Nicolas-Gonzalez
 //@@//@@/@@ //@@/@@@/@@@/@@//@@//@@//@@//@@/@@  //@@    since 2020/05/01-18:50
  //@@@@ //@@@@@//@@ //@@/@@@@@@ //@@@@//@@@@@  //@@@@ ------------------------

#define MY_VERSION "20240328"
#define MY_LICENSE "Copyright (C) 2020-2023 Cesar Nicolas-Gonzalez"

#define GPL_3_INFO \
	"This program comes with ABSOLUTELY NO WARRANTY; for more details" "\n" \
	"please see the GNU General Public License. This is free software" "\n" \
	"and you are welcome to redistribute it under certain conditions."

/* This notice applies to the source code of CSW2CDT and its binaries.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Contact information: <mailto:cngsoft@gmail.com> */

#include <stdio.h> // fopen,printf...
#include <stdlib.h> // atoll,malloc...
#include <string.h> // memcmp,strcmp...
#include <time.h> // time_t,strftime...

#define length(x) (sizeof(x)/sizeof*(x))
unsigned char buffer[512]; FILE *fi,*fo,*fu;
char *autosuffix(char *t,char *s,char *x) // returns a valid target path, generating it from the source and an extension if required
{
	if (t) return t; else if (!s) return NULL;
	if ((char*)buffer!=s) strcpy(buffer,s);
	if ((t=strrchr(buffer,'.'))&&(!(s=strrchr(buffer,
	#ifdef _WIN32
	'\\'
	#else
	'/'
	#endif
	))||t>s)) return strcpy(t,x),buffer; // override old suffix
	return strcat(buffer,x); // append new suffix
}
int ucase(int i) { return i>='a'&&i<='z'?i-32:i; } // returns ASCII upper case
int endswith(char *s,char *x) // does the string `s` end in the suffix `x`? returns zero if FALSE
{
	int i=strlen(s),j=strlen(x);
	if (i>=j) for (s+=i,x+=j;j&&ucase(*--s)==ucase(*--x);) --j;
	return !j;
}
char topdigit(int i) { return (i+(i<10?'0':'7')); } // returns "fake" decimals beyond 9

// I/O file operations, buffering and Intel lil-endian integer logic -------- //

// non-buffered shortcuts
#define fget1() fgetc(fi)
#define fput1(n) fputc((n),fo)
#define fread1(t,i) fread((t),1,(i),fi)
#define fwrite1(t,i) fwrite((t),1,(i),fo)
int fget4(void) { int i=fget1(); i|=fget1()<<8; i|=fget1()<<16; return (fget1()<<24)|i; }
int fput4(int i) { fput1(i); fput1(i>>8); fput1(i>>16); return fput1(i>>24); }

// buffered I/O operations
unsigned char tsrc[1<<12],ttgt[1<<12]; int isrc=0,ilen=0,itgt=0;
#define flush1() (fwrite1(ttgt,itgt),itgt=0)
int frecv1(void) { while (isrc>=ilen) if (isrc-=ilen,!(ilen=fread1(tsrc,sizeof(tsrc)))) return -1; return tsrc[isrc++]; }
int frecv2(void) { int i=frecv1();return i|(frecv1()<<8); }
int frecv3(void) { int i=frecv1(); i|=frecv1()<<8; return i|(frecv1()<<16); }
int frecv4(void) { int i=frecv1(); i|=frecv1()<<8; i|=frecv1()<<16; return i|(frecv1()<<24); }
int ftell1(void) { return ftell(fi)-ilen+isrc; }
void fseek1(int i) { fseek(fi,i&-sizeof(tsrc),SEEK_SET); ilen=fread1(tsrc,sizeof(tsrc)); isrc=i&(sizeof(tsrc)-1); }
void fskip1(int i) { isrc+=i; } // should really handle negative cases beyond -1 here...
int fsend1(int i) { ttgt[itgt++]=i; if (itgt>=length(ttgt)) { if (!flush1()) return -1; itgt=0; } return i; }
int fsend2(int i) { fsend1(i); return fsend1(i>>8); }
int fsend3(int i) { fsend1(i); fsend1(i>>8); return fsend1(i>>16); }
int fsend4(int i) { fsend1(i); fsend1(i>>8); fsend1(i>>16); return fsend1(i>>24); }

// common variables and functions ------------------------------------------- //

#define ZXKHZ (3500)
#define ZX1HZ (3500000)
int flag_e=5,hz=44100,ordinal=0,seconds=0; char tape_sign=0;
const char csw24b[24]="Compressed Square Wave\032\001";

int tape_head[256][2]; // head pairs: {length, weight}; can't be "signed short", legit tones can go beyond 32768 edges, f.e. 45000 in MSX hi-speed header blocks :-/
int tape_word[256][32]; // word items: {type, weight..., -1}; can't be "unsigned short" because we need "-1" as the end marker: 0 is a legit value!
#define tape_bit0l tape_word[0][0] // BIT0 pair:
#define tape_bit0w tape_word[0][1] // {length, weight}
#define tape_bit1l tape_word[1][0] // BIT1 pair:
#define tape_bit1w tape_word[1][1] // {length, weight}
#define tape_datas tape_twotwo // 99% cases use x2:x2
int tape_heads=0,tape_waves=0,tape_kansas=0; int tape_tails=0; // amount of tape block primitives
int tape_twofor=0,tape_twotwo=0,tape_onetwo=0,tape_oneone=0; // lengths of DATA primitives: 4:2 (FMx2), 2:2 (normal), 2:1 (FM) and 1:1 (halved)
int tape_wave,tape_width; // parameters of the primitives WAVE and DATA: sample weight in T and word bitwidth
int tape_kansasi,tape_kansasin,tape_kansaso,tape_kansason,tape_kansasio,tape_kansasn[2]; // KANSAS CITY parameters
unsigned char tape_temp[1<<18]; // when we're decoding we must store the bytes before sending them to file

#define APPROX2(x) (((x)+1)>>1)
#define APPROX4(x) (((x)+2)>>2)
#define APPROX8(x) (((x)+4)>>3)
#define APPROX16(x) (((x)+8)>>4)
#define APPROX32(x) (((x)+16)>>5)
#define APPROX64(x) (((x)+32)>>6)
int approx(int x,int y) { return (x+(y>>1))/y; } // division and rounding of integer/integer x/y
int approxy(int a,int b,int y) { long long int x=a; x*=b; return (x+(y>>1))/y; } // ditto, a*b/c
int approxyz(int a,int b,int c,int d) { long long int x=a,y=c; x*=b,y*=d; return (x+(y>>1))/y; }
int within100(int x,int y,int z) { z=approxy(y,z,100); return x+z>=y&&x<=y+z; }

char tape_pzx=0,tape_tzx=20,tape_tsx=0; // TZX (PZX if nonzero) 1.20 by default, but it can be 0 (disable extended blocks such as $19)
char *tape_name="",tape_closegroup=0; int tape_groupcount=0; // encoding description (if any), "close block" flag and open block count
void uprintl(void) { fprintf(fu,"-------------------------------------------------------------------------------\n"); }
void uprinth(void) { fprintf(fu,tape_pzx?"#####  type   encoding                                            length  check\n":
	"#####  type   encoding    pilot     syncs   BIT0:BIT1 pct  length  check  pause\n"); }
void uprintm(void) { fprintf(fu,tape_pzx?"#####  type  contents                                                    millis\n":
	"#####  type            pilot     syncs   BIT0:BIT1 pct  length   pause   millis\n"); }
char uprint_t=1; // timeline or ordinals?
void do_print(void)
{
	++ordinal; if (uprint_t)
		fprintf(fu,"%c%c:%02d ",topdigit(seconds/600),((seconds/60)%10)+48,seconds%60);
	else
		fprintf(fu,"%c%04d ",topdigit(ordinal/10000),ordinal%10000);
}
#define uprints(s) (do_print(),fprintf(fu,s))
#define uprintf(s,...) (do_print(),fprintf(fu,s,__VA_ARGS__))
void uprintz(int i)
{
	uprintf("STATS: %d bytes, ",i);
	if (uprint_t)
		fprintf(fu,"%d blocks\n",ordinal-1);
	else
		fprintf(fu,"%d:%02d time\n",seconds/60,seconds%60);
}

// encoding-only vars and funcs --------------------------------------------- //

int oo=0,samples,*sample;
void do_clock(void) { static int ooo=0,tt=0; while (ooo<oo) tt+=sample[ooo++]; seconds+=tt/hz; tt%=hz; } // exactly what it says: tick the clock!

//#define ZX2HZ(x) approxy((x),hz,ZX1HZ)
#define ZX2HZ(x) zx2hz[x] // x must stay within length(zx2hz)!
#define HZ2ZX(x) approxy((x),ZX1HZ,hz)
#define HZ2MS(x) approxy((x),+1000,hz)
#define APPROX_HZ2ZX(x,y) approxyz((x),ZX1HZ,hz,(y))
#define APPROX_ZX2HZ(x,y) approxyz((x),hz,ZX1HZ,(y))
int zx2hz[1<<14]; // i.e. up to 16383 T; 8191 T aren't enough (f.e. Keytone)
void zx2hz_setup(void) { for (int i=0;i<length(zx2hz);++i) zx2hz[i]=approxy(i,hz,ZX1HZ); }

// TZX1 and PZX1 output logic, based on a generalised block: a HEAD made of TONES and EDGES, a body made of WORDS or WAVE, and a silent TAIL

int flag_m=70,flag_t=70,flag_z=5,tape_checksum,tape_howtosum;
int tape_parity8b(unsigned char *s,int l) // calculates XOR8 of a string of bytes
	{ for (;l>0;--l) tape_checksum^=*s++; return tape_checksum; }
int tape_ccitt256(unsigned char *s,int l) // checks CCITT of a string of 256-byte chunks; `tape_checksum` is the amount of mistakes
{
	for (;(l-=258)>=0;)
	{
		int t=-1; for (int n=256;n;++s,--n)
			t=(t<<4)^(4129*(((*s>>4)^(t>>12))&15)),
			t=(t<<4)^(4129*(( *s    ^(t>>12))&15));
		t^=*s++<<8; if (~(t^=*s++)&65535) ++tape_checksum;
	}
	return tape_checksum;
}
char *tape_is_ok(void)
{
	int i=-1; switch (tape_howtosum)
	{
		case 1: // amstrad: amount of errors must be ZERO
			i=tape_checksum;
			break;
		case 2: // spectrum: the low 8 bits must be ZERO
			i=tape_checksum&255;
			break;
		case 3: // negative spectrum: the low 8 bits must be ONE
			i=~tape_checksum&255;
			break;
	}
	return i?"--":"OK";
}
int chrcmp(char *t,char k,int n) { while (n>0) { if (*t!=k) return n; ++t,--n; } return 0; }

int tape_tzx12n13[2]={0,0}; char flag_ll=0;
void tape_export_tzx12n13(void) // shows an abridged log instead of the normal style
{ if (tape_tzx12n13[0]) {
	if (flag_ll) uprintf("PULSES %-10s%5dx%04d --------- --------- --- -------- -- -- ------\n",tape_name,tape_tzx12n13[0],approx(tape_tzx12n13[1],tape_tzx12n13[0]));
} tape_tzx12n13[0]=tape_tzx12n13[1]=0; }

void tape_export_tzx12(int j,int k) // stores a series of `k` edges of `j` T each; not to be used directly
{ if (k) {
	fsend1(0X12),fsend2(j),fsend2(k);
	if (flag_ll) tape_tzx12n13[0]+=k,tape_tzx12n13[1]+=k*j;
	else uprintf("TONE   %-10s%5dx%04d --------- --------- --- -------- -- -- ------\n",tape_name,k,j);
} }
void tape_export_tzx13(int j,int k) // stores a set of `k` edges starting at HEAD[`j`]; not to be used directly
{ if (k) {
	fsend1(0X13),fsend1(k);
	int t=0; for (int i=0;i<k;++i) fsend2(tape_head[j][1]),t+=tape_head[j][1],++j;
	if (flag_ll) tape_tzx12n13[0]+=k,tape_tzx12n13[1]+=t;
	else uprintf("SYNC   %-10s ---------%5dx%04d --------- --- -------- -- -- ------\n",tape_name,k,approx(t,k));
} }

#define TAPE_TWOX1 do{ if (q&1) o|=oq; }while(0)
#define TAPE_TWOX2 do{ if (!(oq>>=2)) fsend1(o),o=0,oq=192; }while(0)
#define TAPE_TWOX9 do{ if (oq<192) fsend1(o); }while(0) // send last byte if available
#define TAPE_ONEX1 do{ if (q&1) o|=oq; }while(0)
#define TAPE_ONEX2 do{ if (!(oq>>=1)) fsend1(o),o=0,oq=128; }while(0)
#define TAPE_ONEX9 do{ if (oq<128) fsend1(o); }while(0) // send last byte if available
#define tape_onetwo2length(l) (l<<1) // all bits are stored as two samples!
#define tape_twofor2length(l) (l<<2) // all bits are stored as four samples!
int tape_oneone2length(int l) // bit 0 is one sample, bit 1 is two samples
	{ int i=0,o=0; unsigned char iq=128; while (l--) { if (tape_temp[i]&iq) ++o; ++o; if (!(iq>>=1)) iq=128,++i; } return o; }
int tape_twofor2sample(char q,int l) // write samples of a 4:2 data block; return the new output status (only the lowest bit matters!)
{
	int i=0; unsigned char iq=128,oq=192,o=0; while (l-->0)
	{
		TAPE_TWOX1; if (!(tape_temp[i]&iq)) ++q; // i.e. always send two samples, but the second may change
		oq>>=2; TAPE_TWOX1; TAPE_TWOX2; ++q; if (!(iq>>=1)) iq=128,++i; // we can reduce a TAPE_TWOX2 to oq>>=2
	}
	TAPE_TWOX9; return q;
}
int tape_onetwo2sample(char q,int l) // write samples of a 2:1 data block; return the new output status (only the lowest bit matters!)
{
	int i=0; unsigned char iq=128,oq=128,o=0; while (l-->0)
	{
		TAPE_ONEX1; if (!(tape_temp[i]&iq)) ++q; // i.e. always send two samples, but the second may change
		oq>>=1; TAPE_ONEX1; TAPE_ONEX2; ++q; if (!(iq>>=1)) iq=128,++i; // we can reduce a TAPE_ONEX2 to oq>>=1
	}
	TAPE_ONEX9; return q;
}
int tape_oneone2sample(char q,int l) // write samples of a 1:1 data block; return the new output status (ditto!)
{
	int i=0; unsigned char iq=128,oq=128,o=0; while (l-->0)
	{
		if (tape_temp[i]&iq) { TAPE_ONEX1; TAPE_ONEX2; } // i.e. send either one or two equal samples
		TAPE_ONEX1; TAPE_ONEX2; ++q; if (!(iq>>=1)) iq=128,++i; // we must always check bits with TAPE_ONEX2
	}
	TAPE_ONEX9; return q;
}
void tape_export_pascal4(char *s) { fsend4(strlen(s)); while (*s) fsend1(*s++); }
void tape_export_pascal1(char *s) { fsend1(strlen(s)); while (*s) fsend1(*s++); }
void tape_export_newblock(char *s) // stores the beginning of a super-block (i.e. a container of sub-blocks)
{
	tape_closegroup=-1;
	sprintf(buffer,"%s %04d",tape_name,++tape_groupcount);
	uprintf("GROUP: %s\n",buffer); if (tape_pzx) // PZX format?
		fsend4(0X53575242),tape_export_pascal4(buffer); // "BRWS"
	else
		fsend1(0X21),tape_export_pascal1(buffer);
}
#define tape_export_endblock() (tape_closegroup=tape_closegroup?1:0) // tags an open super-block as finished
void tape_export_oldblock(void) // stores the end of a super-block
{
	tape_closegroup=0;
	uprints("GROUP.\n"); if (tape_pzx) // PZX format?
		fsend4(0X42525753),fsend4(0); // "SWRB" (end of BRWS)
	else
		fsend1(0X22);
}
void tape_export_infos(char *s) // stores an information text string
{
	uprintf("INFOS: %s\n",s); if (tape_pzx) // PZX format?
		fsend4(0X54584554),tape_export_pascal4(s); // "TEXT"
	else
		fsend1(0X30),tape_export_pascal1(s);
}

void tape_export(void) // exports a general tape block and flushes the primitives' data structures
{
	static char q=1; // output status: only bit 0 matters
	if (tape_tails<0) tape_tails=0; // sanity check!
	if (tape_pzx) // PZX format?
	{
		if (tape_heads) // "PULS"?
		{
			fsend4(0X534C5550);
			int j=0; // actual block size
			int n=10; // fields in the log
			char *t=buffer+sprintf(buffer,"%-10s",tape_name);
			if ((q&1)) ++j; // HIGH first signal
			for (int i=0;i<tape_heads;++i)
				if (tape_head[i][0]>1)
					{ if (j+=2,(n-=2)>=0) t+=sprintf(t,(tape_head[i][0]<10000?" %04dx%04d":"%5dx%04d"),tape_head[i][0],tape_head[i][1]); }
				else
					{ if (++j,--n>=0) t+=sprintf(t," %04d",tape_head[i][1]); }
			if (n<0) sprintf(t," +%4d",-n);
			uprintf("PULSES %s\n",buffer);
			fsend4(j<<1);
			if ((q&1)) fsend2(0); // first signal HIGH is encoded with a ZERO here
			for (int i=0;i<tape_heads;++i)
			{
				if ((j=tape_head[i][0])>1) fsend2(j+0X8000); // pilot? set bit 15!
				fsend2(tape_head[i][1]);
				q+=j; // the sign changes as many times as there are edges
			}
		}
		if (tape_twofor|tape_twotwo|tape_onetwo|tape_oneone) // "DATA"?
		{
			fsend4(0X41544144);
			int i,j=approx(hz,1000),k; // not 50! (see ZXKHZ below)
			char *t=buffer+sprintf(buffer,"%-10s",tape_name);
			if (i=tape_twofor) // 4:2?
				k=8+4;
			else if (i=tape_twotwo) // 2:2?
				k=4+4;
			else if (i=tape_onetwo) // 2:1?
				k=2+4;
			else // if (tape_oneone) // 1:1?
				i=tape_oneone,k=2+2;
			if ((q&1)&&tape_tails>=j)
				tape_tails-=j,j=ZXKHZ; // create tail from silence
			else
				j=0; // no tail, do nothing
			t+=sprintf(t," BIT0 =%3dx%04d BIT1 =%3dx%04d%4d%%%5d",tape_bit0l,tape_bit0w,tape_bit1l,tape_bit1w,
				approx(171000+342000,tape_bit0l*tape_bit0w+tape_bit1l*tape_bit1w),j);
			uprintf("DATA   %s%8d:%d %02X %s\n",buffer,(i+7)>>3,((i-1)&7)+1,tape_checksum&255,tape_is_ok());
			i=(i+7)>>3; // turn bits into bytes
			fsend4(i+8+k);
			fsend4((tape_twofor|tape_twotwo|tape_onetwo|tape_oneone)+(q<<31));
			fsend2(j);
			if (j) j=1; // if there's a pause, the signal changes once at the end
			if (tape_twofor) // 4:2?
			{
				fsend1(4); // =tape_bit0l
				fsend1(2); // =tape_bit1l
				fsend2(tape_bit0w);
				fsend2(tape_bit0w);
				fsend2(tape_bit0w);
				fsend2(tape_bit0w);
				fsend2(tape_bit1w);
				fsend2(tape_bit1w);
				// signals always come in pairs, final sign stays the same
			}
			else if (tape_twotwo) // 2:2?
			{
				fsend1(2); // =tape_bit0l
				fsend1(2); // =tape_bit1l
				fsend2(tape_bit0w);
				fsend2(tape_bit0w);
				fsend2(tape_bit1w);
				fsend2(tape_bit1w);
				// signals always come in pairs, final sign stays the same
			}
			else if (tape_onetwo) // 2:1?
			{
				fsend1(2); // =tape_bit0l
				fsend1(1); // =tape_bit1l
				fsend2(tape_bit0w);
				fsend2(tape_bit0w);
				fsend2(tape_bit1w);
				for (int x=i>>3;x-->0;)
					for (unsigned char y=128;y;y>>=1)
						if (!(tape_temp[x]&y)) ++j; // single signal? actual change!
			}
			else //if (tape_oneone) // 1:1?
			{
				fsend1(1); // =tape_bit0l
				fsend1(1); // =tape_bit1l
				fsend2(tape_bit0w);
				fsend2(tape_bit1w);
				j+=tape_oneone; // all signals are single: as many actual changes as signals
			}
			if (j&1) q=0; // set the signal's final status
			flush1(); fwrite1(tape_temp,i);
		}
		if (tape_tails) // "PAUS"?
		{
			long long int j=tape_tails; j=(j*ZX1HZ+(hz>>1))/hz;
			do
			{
				int i=/*(q&1)?ZX1HZ/50:*/0X7FFFFFFF; if (i>j) i=j;
				fsend4(0X53554150); fsend4(4); fsend4(i+(q<<31)); j-=i; q=0;
				uprintf("PAUSE  ---------- %09d%8dms\n",i,approx(i,ZXKHZ));
			}
			while (j); // it loops every 10 minutes and 14 seconds :-/
			++q;
		}
	}
	else // TZX format, 1.00 if possible
	{
		tape_tails=HZ2MS(tape_tails);
		tape_tzx12n13[0]=tape_tzx12n13[1]=0;
		int i,j=0,k=0,l,h; for (i=0;i<tape_heads;++i)
			q+=tape_head[i][0]; // toggle the output as many times as the pulses change!
		if (tape_kansas&&tape_heads<2) // KANSAS CITY bytes; limited to the unofficial TSX (TZX 1.21) format!
		{
			// this block won't be generated if tape_pzx is true or tape_tzx is false!
			fsend1(0X4B);
			fsend4(tape_kansas+12);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i; fsend2(i);
			if (tape_heads)
				fsend2(tape_head[0][1]),fsend2(tape_head[0][0]);
			else // no tone
				fsend2(tape_head[0][1]=tape_bit0w),fsend2(tape_head[0][0]=0);
			fsend2(tape_bit0w);
			fsend2(tape_bit1w);
			fsend1((tape_kansasn[0]<<4)+(tape_kansasn[1]&15));
			fsend1((tape_kansasin<<6)+(tape_kansasi<<5)+(tape_kansason<<3)+(tape_kansaso<<2)+(tape_kansasio&1));
			flush1(); fwrite1(tape_temp,tape_kansas);
			uprintf("KANSAS %-10s%5dx%04d %c-(%c:%c)-%c %04d:%04d%4d%7d:8 -- --%7d\n",
				tape_name,tape_head[0][0],tape_head[0][1],48+tape_kansasin,48+tape_bit0l,48+tape_bit1l,48+tape_kansason,tape_bit0w,tape_bit1w,
				approx(512800,tape_bit0w*tape_bit0l+tape_bit1w*tape_bit1l),tape_kansas,i);
			tape_heads=0;
		}
		for (i=0;i<tape_heads;++i)
		{
			if (((i+3==tape_heads&&tape_head[i+1][0]==1&&tape_head[i+2][0]==1) // ... PILOT + SYNCS + DATAS [+ PAUSE]?
				||(i+2==tape_heads&&tape_head[i+1][0]==2))&&/*tape_head[i][0]>1&&*/tape_twotwo) // 2-edge TONE = two SYNCS
			{
				tape_export_tzx13(j,k); tape_export_tzx12n13(); // PURE SYNC (EARLY)
				j=tape_bit0w,k=tape_bit1w;
				int h1,h2; if (i+2==tape_heads)
					h1=h2=tape_head[i+1][1]; // 2-edge tone
				else
					h1=tape_head[i+1][1],h2=tape_head[i+2][1];
				int z=tape_temp[0]<128?8062:3222;
				if (tape_twotwo<(1<<19)&&!(tape_twotwo&7)
					&&within100(j+k,855+1710,flag_z)
					&&within100(tape_head[i+0][0],z,flag_z)
					&&within100(tape_head[i+0][1],2168,flag_z)
					&&within100(h1+h2,667+735,flag_z))
				{
					fsend1(0X10); tape_head[i+0][0]=z; z=i;
					i=tape_tails; if (i>65535) i=65535; tape_tails-=i; fsend2(i);
					l=(tape_twotwo+7)>>3; fsend2(l);
					flush1(); fwrite1(tape_temp,l);
					uprintf("NORMAL %-10s%5dx2168 0667,0735 0855:1710 100%7d:8 %02X %s%7d\n",
						tape_name,tape_head[z+0][0],l,tape_checksum&255,tape_is_ok(),i);
				}
				else
				{
					fsend1(0X11); z=i;
					fsend2(tape_head[z+0][1]);
					fsend2(h1);
					fsend2(h2);
					fsend2(j); fsend2(k);
					fsend2(tape_head[z+0][0]);
					h=((tape_twotwo+7)&7)+1; fsend1(h);
					i=tape_tails; if (i>65535) i=65535; tape_tails-=i; fsend2(i);
					l=(tape_twotwo+7)>>3; fsend3(l);
					flush1(); fwrite1(tape_temp,l);
					uprintf("CUSTOM %-10s%5dx%04d %04d,%04d %04d:%04d%4d%7d:%d %02X %s%7d\n",
						tape_name,tape_head[z+0][0],tape_head[z+0][1],h1,h2,
						j,k,approx(256500,j+k),l,h,tape_checksum&255,tape_is_ok(),i);
					if (i==1) ++q; else if (i>0) q=1;
				}
				tape_twotwo=j=k=0; break; // force exit!
			}
			else if (tape_head[i][0]>1) // is it a tone?
			{
				tape_export_tzx13(j,k); k=0; // PURE SYNC
				tape_export_tzx12(tape_head[i][1],tape_head[i][0]);
			}
			else // not a tone, add to list of syncs!
				if (!k++) j=i;
		}
		tape_export_tzx13(j,k); // PURE SYNC (FINAL)
		tape_export_tzx12n13();
		if (tape_twotwo) // DATAS [+ PAUSE]?
		{
			fsend1(0X14);
			j=tape_bit0w,k=tape_bit1w;
			fsend2(j); fsend2(k);
			h=((tape_twotwo+7)&7)+1; fsend1(h);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i; fsend2(i);
			l=(tape_twotwo+7)>>3; fsend3(l);
			flush1(); fwrite1(tape_temp,l);
			uprintf("DATA   %-10s --------- --------- %04d:%04d%4d%7d:%d %02X %s%7d\n",
				tape_name,j,k,approx(256500,j+k),l,h,tape_checksum&255,tape_is_ok(),i);
		}
		else if (tape_twofor) // 4:2-edged DATAS
		{
			j=APPROX4(tape_bit0w*2+tape_bit1w);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i;
			if (!tape_tzx) // compatible but heavier
			{
				fsend1(0X15);
				fsend2(j);
				fsend2(i);
				l=tape_twofor2length(tape_twofor); h=((l+7)&7)+1; l=(l+7)>>3;
				fsend1(h); fsend3(l); // convert bits (0/1) into samples (short+short/long)
				q=tape_twofor2sample(q,tape_twofor);
				uprintf("SAMPLE %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,j,j*2,approx(64125,j),l,h,i);
				if (i==1) ++q; else if (i>0) q=1;
			}
			else // compact but requiring 1.20 support
			{
				fsend1(0X19);
				l=(tape_twofor+7)>>3; h=((tape_twofor+7)&7)+1; fsend4(14+0+18+l);
				fsend2(i);
				fsend4(0); fsend1(0); fsend1(0);
				fsend4(tape_twofor); fsend1(4); fsend1(2);
				fsend1(0); fsend2(tape_bit0w); fsend2(tape_bit0w); fsend2(tape_bit0w); fsend2(tape_bit0w);
				fsend1(0); fsend2(tape_bit1w); fsend2(tape_bit1w); fsend2(0); fsend2(0);
				flush1(); fwrite1(tape_temp,l);
				uprintf("DATA42 %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,tape_bit0w,tape_bit1w,approx(64125,j),l,h,i);
			}
		}
		else if (tape_onetwo) // 2:1-edged DATAS
		{
			j=APPROX4(tape_bit0w*2+tape_bit1w);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i;
			if (!tape_tzx) // compatible but heavier
			{
				fsend1(0X15);
				fsend2(j);
				fsend2(i);
				l=tape_onetwo2length(tape_onetwo); h=((l+7)&7)+1; l=(l+7)>>3;
				fsend1(h); fsend3(l); // convert bits (0/1) into samples (short+short/long)
				q=tape_onetwo2sample(q,tape_onetwo);
				uprintf("SAMPLE %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,j,j*2,approx(128250,j),l,h,i);
				if (i==1) ++q; else if (i>0) q=1;
			}
			else // compact but requiring 1.20 support
			{
				fsend1(0X19);
				l=(tape_onetwo+7)>>3; h=((tape_onetwo+7)&7)+1; fsend4(14+0+10+l);
				fsend2(i);
				fsend4(0); fsend1(0); fsend1(0);
				fsend4(tape_onetwo); fsend1(2); fsend1(2);
				fsend1(0); fsend2(tape_bit0w); fsend2(tape_bit0w);
				fsend1(0); fsend2(tape_bit1w); fsend2(0);
				flush1(); fwrite1(tape_temp,l);
				uprintf("DATA21 %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,tape_bit0w,tape_bit1w,approx(128250,j),l,h,i);
			}
		}
		else if (tape_oneone) // 1:1-edged DATAS
		{
			j=APPROX4(tape_bit0w*2+tape_bit1w);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i;
			if (!tape_tzx) // compatible but heavier
			{
				fsend1(0X15);
				fsend2(j);
				fsend2(i);
				l=tape_oneone2length(tape_oneone); h=((l+7)&7)+1; l=(l+7)>>3;
				fsend1(h); fsend3(l); // convert bits (0/1) into samples (short/long)
				q=tape_oneone2sample(q,tape_oneone);
				uprintf("SAMPLE %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,j,j*2,approx(171000,j),l,h,i);
				if (i==1) ++q; else if (i>0) q=1;
			}
			else // compact but requiring 1.20 support
			{
				fsend1(0X19);
				l=(tape_oneone+7)>>3; h=((tape_oneone+7)&7)+1; fsend4(14+0+ 6+l);
				fsend2(i);
				fsend4(0); fsend1(0); fsend1(0);
				fsend4(tape_oneone); fsend1(1); fsend1(2);
				fsend1(0); fsend2(tape_bit0w);
				fsend1(0); fsend2(tape_bit1w);
				flush1(); fwrite1(tape_temp,l);
				uprintf("DATA11 %-10s --------- --------- %04d:%04d%4d%7d:%d -- --%7d\n",
					tape_name,tape_bit0w,tape_bit1w,approx(171000,j),l,h,i);
			}
		}
		while (tape_tails) // PAUSE?
		{
			fsend1(0X20);
			i=tape_tails; if (i>65535) i=65535; tape_tails-=i; fsend2(i);
			uprintf("PAUSE  ---------- --------- --------- --------- --- -------- -- --%7d\n",i);
			if (i==1) ++q; else if (i>0) q=1;
		}
	}
	do_clock(); if (tape_closegroup>0) tape_export_oldblock();
	tape_heads=tape_twotwo=tape_twofor=tape_onetwo=tape_oneone=tape_kansas=tape_tails=0; // reset all counters!
}

// CSW1 input analysis

// the general idea behind all these operations is that there are three main ways to encode bits:
// 1.- "double" or "twotwo": the most widespread and reliable encodings use two short edges for BIT0 and two long edges for BIT1;
// 2.- "hybrid" or "onetwo": frequency modulation (used by the MSX world, rare elsewhere) uses two short edges for BIT0 and a long one for BIT1;
// 3.- "single" or "oneone": the least widespread encodings (they're the most vulnerable to noise) use a short edge for BIT0 and a long one for BIT1;
// 4.- the Kansas City Standard "twofor" is a "onetwo" that sends bits twice (four short edges, two long ones): the MSX1 firmware's default encoding.
// telling whether a stream of bits is a single-signal encoding is trivial, while hybrid-signal and double-signal encodings need more attention.

int detect_singles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int u) // guess the longest self-contained single-signal stream:
// o = offset, w = NULL / *stream weight, w_l = NULL / *minimum detected signal, w_h = NULL / *maximum detected signal,
// l_l = minimum stream length, l_h = maximum stream length, v = threshold minimum/maximum double-percentage;
// return 0 if failed (this won't update the optional variables!) or a value in the [l_l,l_h] range if succesful.
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,u_l=sample[o],u_h=u_l,v=u_l; if ((l_h+=o)>samples) l_h=samples;
	while (++l<l_h)
	{
		if (u_h<(i=sample[l]))
			if (i*u>=u_l*200) break; else u_h=i;
		else if (u_l>i)
			if (u_h*u>=i*200) break; else u_l=i;
		v+=i;
	}
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int detect_singlez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int u) // slightly more strict version: reject maximum length
	{ return ((o=detect_singles(o,w,w_l,w_h,l_l,l_h,u))&&o<l_h)?o:0; }
int define_singles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_h) // measure the longest valid single-signal stream:
// o = offset, w = NULL / *stream weight, w_l = NULL / *minimum detected signal, w_h = NULL / *maximum detected signal,
// l_l = minimum stream length, l_h = maximum stream length, v_l = minimum signal weight, v_h = maximum signal weight;
// return 0 if failed (this won't update the optional variables!) or a value in the [l_l,l_h] range if succesful.
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,v=0,u_l=v_h,u_h=v_l; if ((l_h+=o)>samples) l_h=samples;
	while (l<l_h&&(i=sample[l])>=v_l&&i<=v_h) { v+=i,++l; if (u_l>i) u_l=i; if (u_h<i) u_h=i; }
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int define_singlez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_h) // slightly more strict version: reject maximum length
	{ return ((o=define_singles(o,w,w_l,w_h,l_l,l_h,v_l,v_h))&&o<l_h)?o:0; }

int choose_singles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_lh,int v_hl,int v_h) // measure the longest valid single-signal stream:
// similar to define_singles() with additional parameters v_lh and v_hl to specify the longest short signal and the shortest long signal
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,v=0,u_l=v_h,u_h=v_l; if ((l_h+=o)>samples) l_h=samples;
	while (l<l_h&&(i=sample[l])>=v_l&&i<=v_h) { v+=i,++l; if (u_l>i) u_l=i; if (u_h<i) u_h=i; }
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int choose_singlez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_lh,int v_hl,int v_h) // slightly more strict version: reject maximum length
	{ return ((o=choose_singles(o,w,w_l,w_h,l_l,l_h,v_l,v_lh,v_hl,v_h))&&o<l_h)?o:0; }

int divide_singles(int o,int *w,int l,int v) // tell apart between short and long signals in a single-signal stream
// o = offset, w = NULL / *short signals' weight, l = stream length, v = average signal weight;
// return the amount of short signals -- n.b. the amount of long signals is l minus the amount of short signals.
{
	if (!l) return 0; // early exit!
	int i,j=0,u=0; do { if ((i=sample[o])<v) u+=i,++j; ++o; } while (--l);
	if (w) *w=u;
	return j;
}
// no need for strict_singles() because single-signal streams are the finest possible granularity
int encode_singles(int o,unsigned char *t,int l,int v) // encode a single-signal stream into bits
// o = offset, *t = target buffer, l = stream length, v = average signal weight;
// return the amount of encoded bits (in this case, it's equal to l)
{
	if (!l) return 0; // early exit!
	unsigned char m=128,b=0; int n=l;
	do { if (sample[o]>=v) b|=m; ++o; if (!(m>>=1)) m=128,*t++=b,b=0; } while (--l);
	if (m!=128) *t=b;
	return n;
}

// notice that detect_hybrids(), define_hybrids() and divide_hybrids() are effectively identical to their single-signal counteparts;
// the differences only apply to strict_hybrids() and encode_hybrids() because they deal with the actual bits' validity and content.
int strict_hybrids(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v) // check whether a single-signal stream is actually made of hybrid signals
// o = offset, w = NULL / *stream weight, w_l = NULL / *minimum detected signal, w_h = NULL / *maximum detected signal,
// l_l = minimum stream length, l_h = maximum stream length, v = average signal weight;
// return the length of the valid hybrid stream, or zero if the stream wasn't valid at all.
{
	if (o+l_l>samples) return 0; // this should never happen!
	int i,l=o,u=0,u_l=v,u_h=v; if ((l_h+=o)>samples) l_h=samples;
	while (l<l_h) { if ((i=sample[l])>=v) { u+=i,++l; if (u_h<i) u_h=i; } else if (l+1<l_h&&(i+=sample[l+1])<=v*2) { u+=i,l+=2; if (u_l>i) u_l=i; } else break; }
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=u;
	if (w_l) *w_l=APPROX2(u_l);
	if (w_h) *w_h=u_h;
	return l;
}
int encode_hybrids(int o,unsigned char *t,int l,int v) // encode a hybrid-signal stream into bits
// (beware: the stream must have been deemed valid in advance and its length must be known)
// o = offset, *t = target buffer, l = stream length, v = average signal weight;
// returns the amount of encoded bits (halfway between l and l/2)
{
	if (!l) return 0; // early exit!
	unsigned char m=128,b=0; int n=0; l+=o;
	do { ++n; if (sample[o]>=v) b|=m; else ++o; if (!(m>>=1)) m=128,*t++=b,b=0; } while (++o<l);
	if (m!=128) *t=b;
	return n;
}

// because the bulk of tapes use double-signal encodings, we're better off giving them their own analysis functions
// instead of calling detect_singles() or define_singles() first, then running strict_doubles() and the like on them.

int detect_doubles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int u) // guess the longest self-contained double-signal stream:
// o = offset, w = NULL / *stream weight, w_l = NULL / *minimum detected signal, w_h = NULL / *maximum detected signal,
// l_l = minimum stream length, l_h = maximum stream length, u = threshold minimum/maximum double-percentage;
// return 0 if failed (this won't update the optional variables!) or a value in the [l_l,l_h] range if succesful.
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,u_l=sample[o]+sample[o+1],u_h=u_l,v=u_l; if ((l_h+=o)>samples) l_h=samples;
	while ((l+=2)<l_h)
	{
		if (u_h<(i=sample[l]+sample[l+1]))
			if (i*u>=u_l*200) break; else u_h=i;
		else if (u_l>i)
			if (u_h*u>=i*200) break; else u_l=i;
		v+=i;
	}
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int detect_doublez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int u) // slightly more strict version: reject maximum length
	{ return ((o=detect_doubles(o,w,w_l,w_h,l_l,l_h,u))&&o<l_h)?o:0; }
int define_doubles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_h) // measure the longest possible double-signal stream
// o = offset, w = NULL / *stream weight, w_l = NULL / *minimum detected signal, w_h = NULL / *maximum detected signal,
// l_l = minimum stream length, l_h = maximum stream length, v_l = minimum signal weight, v_h = maximum signal weight;
// return 0 if failed (this won't update the optional variables!) or a value in the [l_l,l_h] range if succesful.
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,v=0,u_l=v_h,u_h=v_l; if ((l_h+=o)>=samples) l_h=samples-1;
	while (l<l_h&&(i=sample[l]+sample[l+1])>=v_l&&i<=v_h) { v+=i,l+=2; if (u_l>i) u_l=i; if (u_h<i) u_h=i; }
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int define_doublez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_h) // slightly more strict version: reject maximum length
	{ return ((o=define_doubles(o,w,w_l,w_h,l_l,l_h,v_l,v_h))&&o<l_h)?o:0; }
int choose_doubles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_lh,int v_hl,int v_h) // measure the longest possible double-signal stream
// similar to define_doubles() with additional parameters v_lh and v_hl to specify the longest short signal and the shortest long signal
{
	if (o+l_l>samples) return 0; // early exit!
	int i,l=o,v=0,u_l=v_h,u_h=v_l; if ((l_h+=o)>=samples) l_h=samples-1;
	while (l<l_h&&(i=sample[l]+sample[l+1])>=v_l&&i<=v_h&&(i<=v_lh||i>=v_hl)) { v+=i,l+=2; if (u_l>i) u_l=i; if (u_h<i) u_h=i; }
	if ((l-=o)<l_l) return 0; // failure!
	if (w) *w=v;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int choose_doublez(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v_l,int v_lh,int v_hl,int v_h) // slightly more strict version: reject maximum length
	{ return ((o=choose_doubles(o,w,w_l,w_h,l_l,l_h,v_l,v_lh,v_hl,v_h))&&o<l_h)?o:0; }
int divide_doubles(int o,int *w,int l,int v) // tell apart between short and long signals in a double-signal stream
// o = offset, w = NULL / *short signals' weight, l = stream length, v = average signal weight;
// return the amount of short signals -- n.b. the long signals' amount is l minus the short signals' amount; their weight is the stream weight minus the short signals' weight.
{
	if (!(l>>=1)) return 0; // early exit!
	int i,j=0,u=0; do { if ((i=sample[o]+sample[o+1])<v) u+=i,++j; o+=2; } while (--l);
	if (w) *w=u;
	return j<<1;
}
/*int strict_doubles(int o,int *w,int *w_l,int *w_h,int l_l,int l_h,int v) // check whether a single-signal stream is actually made of double signals
// o = offset, w = NULL / stream weight, l_l = minimum stream length, l_h = maximum stream length, v = average signal weight;
// return the length of the valid double stream, or zero if the stream wasn't valid at all.
{
	if (o+l_l>samples) return 0; // this should never happen!
	int i,l=o,u=0,u_l=v,u_h=v; if ((l_h+=o)>=samples) l_h=samples-1;
	while (l<l_h) { if (((i=sample[l])>=v?sample[l+1]>=v:sample[l+1]<v)) { u+=(i+=sample[l+1]),l+=2; if (u_h<i) u_h=i; if (u_l>i) u_l=i; } else break; }
	if ((l-=o)<l_l) return 0;
	if (w) *w=u;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}*/
int encode_doubles(int o,unsigned char *t,int l,int v) // encode a double-signal stream into bits
// o = offset, *t = target buffer, l = stream length, v = average signal weight;
// return the amount of encoded bits (in this case, it's equal to l/2)
{
	if (!(l>>=1)) return 0; // early exit!
	unsigned char m=128,b=0; int n=l;
	do { if (sample[o]+sample[o+1]>=v) b|=m; o+=2; if (!(m>>=1)) m=128,*t++=b,b=0; } while (--l);
	if (m!=128) *t=b;
	return n;
}

int weight_singles(int o,int l) // just sum all the weights of the samples at offset `o` and length `l`
	{ int i=0; if ((l+=o)>samples) l=samples; while (o<l) i+=sample[o++]; return i; }
int weight_couples(int o,int l) // sum the weights of the interleaved samples at offset `o` and length `l` (i.e. 0, 2, 3.. l-1)
	{ int i=0; if ((l+=o)>samples) l=samples; while (o<l) i+=sample[o],o+=2; return i; }
int recalc_singles(int o,int *w,int *w_l,int *w_h,int l) // recalculate all weights (stream, minimum and maximum) in a known single-signal stream
// o = offset, w = NULL / stream weight, w_l = NULL / minimum weight, w_h = NULL / maximum weight, l = stream length, v = average signal weight;
// return the stream length, or zero if something went wrong.
{
	int i,k=(o+l),u=0,u_l,u_h; if (k>samples) return 0; // this should never happen!
	u_l=u_h=sample[o]; while (++o<k) { u+=i=sample[o]; if (u_h<i) u_h=i; if (u_l>i) u_l=i; }
	if (w) *w=u;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}
int recalc_doubles(int o,int *w,int *w_l,int *w_h,int l) // recalculate all weights (stream, minimum and maximum) in a known double-signal stream
// o = offset, w = NULL / stream weight, w_l = NULL / minimum weight, w_h = NULL / maximum weight, l = stream length, v = average signal weight;
// return the stream length, or zero if something went wrong.
{
	int i,k=(o+l),u=0,u_l,u_h; if (k>samples) return 0; // this should never happen!
	u_l=u_h=sample[o]+sample[o+1]; while ((o+=2)<k) { u+=i=sample[o]+sample[o+1]; if (u_h<i) u_h=i; if (u_l>i) u_l=i; }
	if (w) *w=u;
	if (w_l) *w_l=u_l;
	if (w_h) *w_h=u_h;
	return l;
}

int middle_doubles(int o,int *w,int l,int h) // validate exactly one double-signal: returns 0 if invalid, 2 if valid
{
	if (o+2>samples) return 0; // early exit!
	o=sample[o]+sample[o+1]; if (w) *w=o;
	return o>=l&&o<=h?2:0;
}
// notice that middle_hybrid() would be equal to middle_single() when sample[o] is long, and to middle_doubles() when it's short;
// middle_single() itself would simply set `*w` to sample[o] and tell whether sample[o] fits between `l` and `h`.

// tape data detection and encoding

char flag_b=0,flag_y=0,flag_r=0,flag_0=-1; // encoding flags: BIT1:BIT0 and SYNC2:SYNC1 ratios, tone parity, incomplete byte handling
int header_length,header_weight,middle_length,middle_weight,stream_length,stream_weight,weight_l,weight_h; // measurements:
// header_* refers to the heading tone/pilot, middle_* refers to the synchro edges/syncs, stream_* refers to the bitstream.

void encode_common_heads(char r,char y) // encode common pilot and syncs, see below
{
	tape_heads=0; if (tape_head[0][0]=header_length) // pilot?
	{
		if (r==1) // odd tones?
			{ if (!(tape_head[0][0]&1)) --tape_head[0][0]; } // i.e. turn 2 into 1, but keep 1
		else if (r)//==2 // even tones?
			{ if (tape_head[0][0]&1) ++tape_head[0][0]; } // inverse: turn 1 into 2 but keep 2
		tape_head[0][1]=APPROX_HZ2ZX(header_weight,header_length),tape_heads=1;
	}
	if (middle_length==1) // syncs? notice that some block types use one sync
		tape_head[tape_heads][0]=1,tape_head[tape_heads][1]=HZ2ZX(middle_weight),++tape_heads;
	else if (middle_length)//==2 // all other block types with syncs use exactly two
	{
		tape_head[tape_heads][0]=tape_head[tape_heads+1][0]=1; int i=HZ2ZX(middle_weight);
		if (y)
			tape_head[tape_heads][0]=2,tape_head[tape_heads][1]=APPROX2(i),++tape_heads; // turn syncs into a couple
		else
			tape_head[tape_heads][1]=i-(tape_head[tape_heads+1][1]=HZ2ZX(sample[oo+header_length+1])),tape_heads+=2;
	}
}

#define TAPE_BITWZERO do{ if (!tape_bit0w) tape_bit0w=APPROX2(tape_bit1w); else if (!tape_bit1w) tape_bit1w=tape_bit0w*2; }while(0) // sanity check: fill empty fields
#define TAPE_FLAG_B(w,n) do{ if (flag_b) tape_bit1w=(tape_bit0w=APPROX_HZ2ZX(w,n))*2; }while(0) // flag "-b": set BIT1 twice as long as BIT0

void encode_common_double(char r,char y,char z) // encode a common 2:2 tape block previously detected
{
	encode_common_heads(r,y); oo+=header_length+middle_length;
	tape_bit0l=tape_bit1l=2;
	int w,n; if (stream_length)
	{
		n=divide_doubles(oo,&w,stream_length,APPROX2(weight_l+weight_h));
		tape_bit0w=n?APPROX_HZ2ZX(w,n):0; // BIT0 values
		w=stream_weight-w; n=stream_length-n;
		tape_bit1w=n?APPROX_HZ2ZX(w,n):0; // BIT1 values
		TAPE_BITWZERO;
		tape_twotwo=encode_doubles(oo,tape_temp,stream_length,ZX2HZ(tape_bit0w+tape_bit1w));
		TAPE_FLAG_B(stream_weight,n+stream_length);
	}
	else
		tape_bit0w=tape_bit1w=0;
	oo+=stream_length; tape_tails=0; // this value will develop as rejected samples are appended
	if (n=(tape_twotwo&7)) switch (z)
	{
		case 5: // clip or pad incomplete bytes with 1s
		case 4: // clip or pad incomplete bytes with 0s
			if (n>=4)
			{
				case 1: // pad all incomplete bytes with 1s
				case 0: // pad all incomplete bytes with 0s
					if (z&1) tape_temp[tape_twotwo>>3]|=255>>n;
					tape_twotwo+=8-n;
					tape_tails-=APPROX2((8-n)*(weight_l+weight_h)); // decrease tails
					break;
			}
			else
		case 8: // clip all incomplete bytes
		{
			tape_twotwo-=n;
			tape_tails+=APPROX2(n*(weight_l+weight_h)); // increase tails
			break;
		}
	}
	tape_checksum=0; tape_parity8b(tape_temp,tape_twotwo>>3); // default to XOR8
}
void encode_common_hybrid(char r,char y) // encode a common 2:1 hybrid-edge block
{
	encode_common_heads(r,y); oo+=header_length+middle_length;
	tape_bit0l=2,tape_bit1l=1;
	int w,n; if (stream_length)
	{
		n=divide_singles(oo,&w,stream_length,APPROX2(weight_l+weight_h));
		tape_bit0w=n?APPROX_HZ2ZX(w,n):0; // BIT0 values
		w=stream_weight-w; n=stream_length-n;
		tape_bit1w=n?APPROX_HZ2ZX(w,n):0; // BIT1 values
		TAPE_BITWZERO;
		tape_onetwo=encode_hybrids(oo,tape_temp,stream_length,APPROX_ZX2HZ(tape_bit0w+tape_bit1w,2));
		TAPE_FLAG_B(stream_weight,n+stream_length);
	}
	else
		tape_bit0w=tape_bit1w=0;
	oo+=stream_length; tape_tails=0; // this value will develop as rejected samples are appended
	tape_checksum=0; tape_parity8b(tape_temp,tape_onetwo>>3);
}
void encode_common_single(char r,char y) // encode a common 1:1 single-edge block
{
	encode_common_heads(r,y); oo+=header_length+middle_length;
	tape_bit0l=tape_bit1l=1;
	int w,n; if (stream_length)
	{
		n=divide_singles(oo,&w,stream_length,APPROX2(weight_l+weight_h));
		tape_bit0w=n?APPROX_HZ2ZX(w,n):0; // BIT0 values
		w=stream_weight-w; n=stream_length-n;
		tape_bit1w=n?APPROX_HZ2ZX(w,n):0; // BIT1 values
		TAPE_BITWZERO;
		tape_oneone=encode_singles(oo,tape_temp,stream_length,APPROX_ZX2HZ(tape_bit0w+tape_bit1w,2));
		TAPE_FLAG_B(stream_weight,n+stream_length);
	}
	else
		tape_bit0w=tape_bit1w=0;
	oo+=stream_length; tape_tails=0; // this value will develop as rejected samples are appended
	tape_checksum=0; tape_parity8b(tape_temp,tape_oneone>>3);
}

int splice_stream(unsigned char *t,int l) // reduce a stream made of double bits to a single-bit stream, top bit first, bottom bit last
{
	int i,n=l>>1; l=(l+15)>>4; unsigned char *s=t; do
	{
		i=(*s&128)+(*s&32)*2+(*s&8)*4+(*s&2)*8; ++s;
		*t++=i+((*s&64)>>3)+((*s&16)>>2)+((*s&4)>>1)+(*s&1); ++s;
	}
	while (--l>0); return n;
}

int flag_w=0;
int detect_config_pl1,detect_config_pl2,detect_config_ph1,detect_config_ph2,detect_config_p_l,detect_config_p_h; // PILOT TONE parameters
int detect_config_s_l,detect_config_s_h,detect_config_b_l,detect_config_b_h,detect_config_n_l,detect_config_n_h; // SYNCS + DATAS param's

int detect_call_double(int o) // detect normal blocks defined by parameters
{
	return ((header_length=define_singlez(o,&header_weight,NULL,NULL,detect_config_pl1,detect_config_ph2,detect_config_p_l,detect_config_p_h))
		&&(header_length<detect_config_ph1||header_length>=detect_config_pl2)
		&&(middle_length=middle_doubles(o+header_length,&middle_weight,detect_config_s_l,detect_config_s_h))
		&&(stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,detect_config_n_l,detect_config_n_h,detect_config_b_l,detect_config_b_h)));
}
void encode_call_double(void) // encode normal blocks defined by parameters
	{ tape_howtosum=0; encode_common_double(flag_r,flag_y,flag_0); }

void config_call_alkatraz(void)
{
	detect_config_pl1= 144- 20; detect_config_ph1= 144+ 20;
	detect_config_pl2=3122-400; detect_config_ph2=3122+400;
	detect_config_p_l=ZX2HZ(2180*4/5); detect_config_p_h=ZX2HZ(2380*5/4);
	detect_config_s_l=ZX2HZ(1200*4/5); detect_config_s_h=ZX2HZ(2100*5/4);
	detect_config_b_l=ZX2HZ(1080*4/5); detect_config_b_h=ZX2HZ(1200*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void config_call_hexagon(void)
{
	detect_config_pl1=detect_config_pl2=2944-350;
	detect_config_ph1=detect_config_ph2=2944+350;
	detect_config_p_l=ZX2HZ(2280*4/5); detect_config_p_h=ZX2HZ(2280*5/4);
	detect_config_s_l=ZX2HZ(1600*4/5); detect_config_s_h=ZX2HZ(1600*5/4);
	detect_config_b_l=ZX2HZ(1280*4/5); detect_config_b_h=ZX2HZ(1280*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}

void sanitize_middle(int i,int j) // catch biased syncs! slow encodings like Amstrad and Cassys are prone to develop noise that makes SYNC1 much longer than SYNC2
{
	if (header_length&&header_weight*i/header_length<middle_weight*j)
		if ((i=sample[oo+header_length])*3>(j=sample[oo+header_length+1])*4)
			{ i=APPROX4(i+j*7); header_weight+=middle_weight-i; middle_weight=i; }
}
int detect_call_amstrad(int o)
{
	int i=sample[o]+sample[o+1]; if (!(header_length=define_singlez(o,&header_weight,NULL,NULL,4096-400,4096+400,i*2/5,i*5/8))) return 0;
	i=approx(header_weight,header_length); return (middle_length=middle_doubles(o+header_length,&middle_weight,i*1/2,i*3/2))
		&&(stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,32,21E5,i*1/2,i*5/2));
}
void encode_call_amstrad(void) // encode Amstrad-like blocks
{
	sanitize_middle(8,7); encode_common_double(flag_r,flag_y,flag_0);
	tape_howtosum=1; tape_checksum=0; tape_ccitt256(&tape_temp[1],tape_twotwo>>3); // the CCITT spans the bytes between the ID (first) and the trailer
	if (!tape_checksum) // all the CRCs are right, but what about the trailer? (the firmware ignores the trailer, but some protections require the dummy bits)
	{
		int i=tape_twotwo%(258<<3),j=(tape_twotwo+7)>>3;
		if (i<40) tape_checksum=250; // the ideal trailer is made of 64 edges!
		else if (i>40) tape_checksum=255;
		else if (tape_temp[j-4]!=255) tape_checksum=251;
		else if (tape_temp[j-3]!=255) tape_checksum=252;
		else if (tape_temp[j-2]!=255) tape_checksum=253;
		else if (tape_temp[j-1]!=255) tape_checksum=254; // the 64 edges must be BIT1!
	} // final 8-bit error code: $00 success; $01..$F9 amount of bad CRCs in the bitstream; $FA trailer is too short; $FB..$FE first non-$FF in trailer; $FF trailer is too long
}
int detect_call_cassys(int o) // similar to Amstrad but it can carry an incompatible bitstream
{
	int i=sample[o]+sample[o+1]; if (!(header_length=define_singlez(o,&header_weight,NULL,NULL,4620,5620,i*2/5,i*5/8))) return 0;
	i=approx(header_weight,header_length); return (middle_length=middle_doubles(o+header_length,&middle_weight,i*1/2,i*3/2))
		&&(stream_length=define_doublez(o+header_length+2,NULL,&weight_l,&weight_h,32,21E5,i*3/4,i*9/4)) // warning: last byte is always 6 bits!
		&&(stream_length=recalc_doubles(o+header_length+2,&stream_weight,&weight_l,&weight_h,((stream_length+4)&-16)-4));
}
void encode_call_cassys(void)
	{ tape_howtosum=0; sanitize_middle(8,7); encode_common_double(flag_r,flag_y,-1); }

void config_call_spectrum(void)
{
	detect_config_pl1=3222*7/8; detect_config_ph1=3222*9/8;
	detect_config_pl2=8062*7/8; detect_config_ph2=8062*9/8;
	detect_config_p_l=ZX2HZ(2168*8/9); detect_config_p_h=ZX2HZ(2168*9/8);
	detect_config_s_l=ZX2HZ(1400*8/9); detect_config_s_h=ZX2HZ(1400*9/8);
	detect_config_b_l=ZX2HZ(1710*8/9); detect_config_b_h=ZX2HZ(1710*9/4);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void config_call_ricochet(void) // very irregular, between 90% and 110% as quick as Spectrum
{
	detect_config_pl1=detect_config_pl2=8048*7/8; // all known pilots are 8448 edges long;
	detect_config_ph1=detect_config_ph2=8848*9/8; // neither 3222 or 8062 seem to be used.
	detect_config_p_l=ZX2HZ(2168*4/5); detect_config_p_h=ZX2HZ(2336*5/4);
	detect_config_s_l=ZX2HZ(1400*4/5); detect_config_s_h=ZX2HZ(2168*5/4);
	detect_config_b_l=ZX2HZ(1567*4/5); detect_config_b_h=ZX2HZ(1710*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void config_call_specvar0(void) // also irregular, somewhere near 87%, f.e. "SUPERTRUX"
{
	detect_config_pl1=3222*7/8; detect_config_ph1=3222*9/8;
	detect_config_pl2=8062*7/8; detect_config_ph2=8062*9/8;
	detect_config_p_l=ZX2HZ(2100*4/5); detect_config_p_h=ZX2HZ(2500*5/4);
	detect_config_s_l=ZX2HZ(1500*4/5); detect_config_s_h=ZX2HZ(2100*5/4);
	detect_config_b_l=ZX2HZ(1200*4/5); detect_config_b_h=ZX2HZ(1800*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void config_call_specvar1(void) // quite irregular too, but normally around 120%
{
	detect_config_pl1=3222*7/8; detect_config_ph1=3222*9/8;
	detect_config_pl2=8062*7/8; detect_config_ph2=8062*9/8;
	detect_config_p_l=ZX2HZ(1818*4/5); detect_config_p_h=ZX2HZ(2518*5/4);
	detect_config_s_l=ZX2HZ(1400*4/5); detect_config_s_h=ZX2HZ(2168*5/4);
	detect_config_b_l=ZX2HZ(1422*4/5); detect_config_b_h=ZX2HZ(1422*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void config_call_specvar2(void) // less irregular, usually almost 133%
{
	detect_config_pl1=3222*7/8; detect_config_ph1=3222*9/8;
	detect_config_pl2=8062*7/8; detect_config_ph2=8062*9/8;
	detect_config_p_l=ZX2HZ(1818*4/5); detect_config_p_h=ZX2HZ(2518*5/4);
	detect_config_s_l=ZX2HZ(1400*4/5); detect_config_s_h=ZX2HZ(2168*5/4);
	detect_config_b_l=ZX2HZ(1120*4/5); detect_config_b_h=ZX2HZ(1440*5/2);
	detect_config_n_l=  48    ; detect_config_n_h=21E5;
}
void encode_call_spectrum(void) // encode Spectrum-like blocks
	{ tape_howtosum=2; /*sanitize_middle(5,8);*/ encode_common_double(flag_r,flag_y,flag_0); }
void encode_call_ricochet(void) // encode Ricochet-like blocks
	{ tape_howtosum=3; /*sanitize_middle(5,8);*/ encode_common_double(flag_r,flag_y,flag_0); } // Ricochet XOR8 is a negative Spectrum

int detect_call_poliload(int o) // cover both Spectrum and CPC at the same time: similar pilots, different SYNC and BITS
{
	if (!(header_length=detect_singlez(o,&header_weight,NULL,NULL,11,3600,flag_m))) return 0; // 70% is too high
	if (header_length>20&&header_length<3000) return 0; // pilot length is either 3228 edges (1st) or 16 (next)
	int i=approx(header_weight,header_length); if (!(middle_length=middle_doubles(o+header_length,&middle_weight,APPROX4(i),i))) return 0;
	//j=APPROX32((i=weight_singles(o+header_length+2,16))*5); i=APPROX32(i); // guess BIT0 and BIT1 from the first byte, always ZERO!
	//return stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,99,2222,i,0,0,j);
	return stream_length=detect_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,99,2222,flag_t/*APPROX4(flag_t*3)*/);
}

int detect_call_zydroload(int o) // cover all Zydroload versions; tricky, because even if the logic is the same, timings are different!
{
	if (!(header_length=define_singlez(o,&header_weight,NULL,NULL,80*7/8,1536*9/8,ZX2HZ(1200*8/9),ZX2HZ(1800*9/8)))) return 0;
	if (header_length>=80*9/8&&header_length<1536*7/8) return 0;
	int i=approx(header_weight,header_length); if (!(middle_length=middle_doubles(o+header_length,&middle_weight,i/2,i*2))) return 0;
	return stream_length=detect_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,48,21E5,flag_t/*APPROX4(flag_t*3)*/);
}
void encode_call_zydroload(void) // Zydroload sub-blocks may be completely glued; fortunately we have a way to tell them apart in the first bytes.
{
	tape_howtosum=2; int o=oo; encode_common_double(flag_r,flag_y,flag_0); // 18 = current sub-block, 11 bytes + next pilot taken for data, 5 bytes + next block, at least 2 bytes
	while (tape_twotwo>18*8&&tape_temp[1]==0XDD&&tape_temp[2]==0X21&&tape_temp[5]==0X11&&(tape_temp[8]==0X3E||tape_temp[8]==0X18)) // MSX version glues blocks together!
	{
		unsigned char x=0; for (int i=0;i<11;++i) x^=tape_temp[i]; if (x) break; // bad checksum!
		int l=tape_temp[7]*256+tape_temp[6]+2;
		oo=o; // rewind and retry
		stream_length=recalc_doubles(oo+header_length+middle_length,&stream_weight,&weight_l,&weight_h,11*16);
		encode_common_double(flag_r,flag_y,0);
		if (!detect_call_zydroload(oo)) break;
		if (stream_length<l*16) break;
		tape_export();
		o=oo; encode_common_double(flag_r,flag_y,flag_0);
		x=0; for (int i=0;i<l;++i) x^=tape_temp[i]; if (x) break; // bad checksum!
		oo=o; // rewind and retry
		stream_length=recalc_doubles(oo+header_length+middle_length,&stream_weight,&weight_l,&weight_h,l*16);
		encode_common_double(flag_r,flag_y,0);
		if (!detect_call_zydroload(oo)) break;
		tape_export();
		o=oo; encode_common_double(flag_r,flag_y,flag_0);
	}
}

char detect_config_prm1,detect_config_prm2;
void config_call_speedlock0(void) { detect_config_prm1=7; detect_config_prm2=0; } // the original "Daley Thompson's Decathlon" for Spectrum 48K used a 7-bit ID
void config_call_speedlock1(void) { detect_config_prm1=6; detect_config_prm2=0; } // early ZX Spectrum games used a 6-bit ID, f.e. "Bounty Bob Strikes Back"
void config_call_speedlock2(void) { detect_config_prm1=5; detect_config_prm2=0; } // early Amstrad CPC games used a 5-bit ID, f.e. "Arkanoid", "Batman", "Highway Encounter"...
void config_call_speedlock3(void) { detect_config_prm1=6; detect_config_prm2=1; } // later ZX Spectrum games are made of multipartite super-blocks
void config_call_speedlock4(void) { detect_config_prm1=5; detect_config_prm2=1; } // later Amstrad CPC games are made of multipartite super-blocks
void config_call_speedlock5(void) { detect_config_prm1=6; detect_config_prm2=3; } // other ZX Spectrum games used a single long tone instead of many short ones
void config_call_speedlock6(void) { detect_config_prm1=5; detect_config_prm2=5; } // the last Amstrad CPC games used 1600 baud instead of 2000
void config_call_speedlock7(void) { detect_config_prm1=6; detect_config_prm2=7; } // the last ZX Spectrum games used a single long tone and 1600 baud
int detect_call_speedlocks(int o) // !(detect_config_prm2&2)
{
	return (header_length=define_singlez(o,NULL,NULL,NULL,192,256,ZX2HZ(1920),ZX2HZ(2880)))
		&&middle_doubles(o+header_length,NULL,ZX2HZ(720),ZX2HZ(3600))
		&&(middle_length=define_singles(o+header_length+2,NULL,NULL,NULL,192,256,ZX2HZ(1920),ZX2HZ(2880)))
		&&middle_doubles(o+header_length+2+middle_length,NULL,ZX2HZ(720),ZX2HZ(3600));
}
int detect_call_speedlockz(int o) // (detect_config_prm2&2)
{
	return (header_length=define_singlez(o,&header_weight,NULL,NULL,1000,2000,ZX2HZ(1800),ZX2HZ(2520)))
		&&(middle_length=define_singles(o+header_length,NULL,NULL,NULL,16,16,ZX2HZ( 360),ZX2HZ(2520)));
}
void encode_speedlock_1st(int n) // the first byte in a Speedlock stream is important (Kukulcan has some words about it)
{
	int l=tape_twotwo-n,t=*tape_temp;
	*tape_temp&=255<<(8-n); tape_twotwo=n; tape_howtosum=0; tape_checksum=*tape_temp;
	tape_export();
	tape_checksum=0; *tape_temp=t; tape_twotwo=l; l>>=3;
	for (int i=0;i<l;++i) tape_temp[i]=(tape_temp[i]<<n)+(tape_temp[i+1]>>(8-n));
}
void encode_call_speedlocks(void)
{
	tape_export_newblock(tape_name);
	int i,j,k,l,h; unsigned char q=0; if (detect_config_prm2&2) // a single long tone followed by approx. 4*720,2*1440,6*2160,2*1440,2*720 T
	{
		tape_head[0][0]=header_length; tape_head[0][1]=APPROX_HZ2ZX(header_weight,header_length);
		for (i=0;i<12;++i)
			tape_head[1+i][0]=1,tape_head[1+i][1]=HZ2ZX(sample[oo+header_length+i]);
		tape_head[1+12+0][0]=tape_head[1+12+1][0]=2;
		tape_head[1+12+0][1]=APPROX_HZ2ZX(sample[oo+header_length+12]+sample[oo+header_length+13],2);
		tape_head[1+12+1][1]=APPROX_HZ2ZX(sample[oo+header_length+14]+sample[oo+header_length+15],2);
		tape_heads=1+12+2; oo+=header_length+middle_length;
	}
	else do // several short 2400-T tones (between 200 and 240 edges each) intertwined with pairs of 720-T SYNCs; last pair is made of 3200-T SYNCs
	{
		i=q?define_doublez(oo,&k,NULL,NULL,192,256,ZX2HZ(2880),ZX2HZ(5760)):(q=1,define_singlez(oo,&k,NULL,NULL,192,256,ZX2HZ(1440),ZX2HZ(2880)));
		if (!(j=middle_doubles(oo+i,&l,ZX2HZ(720),ZX2HZ(1080*3)))) j=middle_doubles(oo+i,&l,ZX2HZ(1800),ZX2HZ(2700*3)); // variation is very high!
		if (!(i&&j)) break; // something went wrong! the pilot is truncated!
		tape_head[tape_heads+0][0]=i; tape_head[tape_heads+0][1]=APPROX_HZ2ZX(k,i);
		tape_head[tape_heads+1][0]=2; tape_head[tape_heads+1][1]=APPROX_HZ2ZX(l,2);
		tape_heads+=2; oo+=i+2;
	}
	while (tape_head[tape_heads-2][1]>tape_head[tape_heads-1][1]); // the pilot lasts while the TONE EDGE is heavier than any SYNC EDGE
	tape_bit0l=tape_bit1l=2; q=0; tape_howtosum=0;
	if (!(detect_config_prm2&1)) // unipartite block; checksum is XOR8
	{
		h=APPROX_ZX2HZ(tape_head[tape_heads-2][1]*5,21); // calculate BIT0 from pilot (~2400 T) instead of assuming ~570 T
		if (i=define_doublez(oo,&k,NULL,NULL,16,21E5,h,h*5)) // this always requests more bits than detect_config_prm1
		{
			tape_twotwo=encode_doubles(oo,tape_temp,i,h*3); //ZX2HZ(1740)
			j=divide_doubles(oo,&l,i,h*3);
			tape_bit0w=  j?APPROX_HZ2ZX(  l,  j):0;
			tape_bit1w=i-j?APPROX_HZ2ZX(k-l,i-j):0;
			TAPE_BITWZERO;
			TAPE_FLAG_B(k,i*2-j);
			tape_tails=0; oo+=i; encode_speedlock_1st(detect_config_prm1); // the first XOR8 must discard the early bits
			tape_parity8b(tape_temp,tape_twotwo>>3);
		}
	}
	else if (!(detect_config_prm2&4)) // multipartite but quick? the separators are distinct to both BIT0 and BIT1!
	{
		if (detect_config_prm2&2) // beware of Speedlock 5 and 7
			h=APPROX_ZX2HZ(tape_head[tape_heads-15][1]*5,19); // not 21?
		else // better than always assuming 570 T, again
			h=APPROX_ZX2HZ(tape_head[tape_heads- 2][1]*5,21);
		while (i=choose_doublez(oo,&k,NULL,NULL,16,21E5,h,APPROX2(h*5),APPROX2(h*7),h*5))
		{
			if (q) tape_export(); // export the 21-bit block
			tape_twotwo=encode_doubles(oo,tape_temp,i,h*3);
			j=divide_doubles(oo,&l,i,h*3);
			tape_bit0w=  j?APPROX_HZ2ZX(  l,  j):0;
			tape_bit1w=i-j?APPROX_HZ2ZX(k-l,i-j):0;
			TAPE_BITWZERO;
			TAPE_FLAG_B(k,i*2-j);
			tape_tails=0; oo+=i; if (!q) encode_speedlock_1st(detect_config_prm1); // discard early bits
			tape_parity8b(tape_temp,tape_twotwo>>3);
			// multipartite blocks are connected with 42-edge blocks of 10 short edges, 22 long ones, and 10 short ones again;
			// while the BIT0 and BIT1 edges are 560 and 1120 T long, the connecting edges are 880 and 1760 T instead
			if (!(i=define_doubles(oo,&k,NULL,NULL,10+22+10,10+22+10,APPROX2(h*3),APPROX2(h*15)))) break;
			tape_export(); // export the previous data block
			l=weight_singles(oo,10+22+10);
			tape_bit0w=APPROX_HZ2ZX(l,64);
			tape_bit1w=APPROX_HZ2ZX(l,32);
			tape_twotwo=q=encode_doubles(oo,tape_temp,i,approxy(l,3,64));
			tape_parity8b(tape_temp,3); tape_checksum^=248; // detect bad separator!
			oo+=i; //if (tape_temp[0]!=7||tape_temp[1]!=255||tape_temp[2]!=0) break; // the separator is wrong!
		}
	}
	else // if (detect_config_prm2&4) // multipartite AND slow! the separators are barely distinguishable from BIT0 edges and the signal bias can be VERY strong :-(
	{
		if (detect_config_prm2&2) // beware of Speedlock 5 and 7, again
			i=tape_head[tape_heads-15][1];
		else
			i=tape_head[tape_heads- 2][1];
		int m=APPROX_ZX2HZ(i*(79+130),227); h=m*2; // bitstream datas and 10+22+10 separators need their own thresholds:
		int ll=APPROX_ZX2HZ(i*( 9*91),227),hl=APPROX_ZX2HZ(i*(21*182),227); // "Batman the Caped Crusader" (1988) is clean;
		int lh=APPROX_ZX2HZ(i*(11*91),227),hh=APPROX_ZX2HZ(i*(23*182),227); // "Midnight Resistance" (1990) is very biased (f.e. h=m*1.5 would fail!)
		for (;;)
		{
			if (q) tape_export(); // export the 21-bit block
			if (q) q=128; else q=128>>detect_config_prm1; // first sub-block starts with N bits; next sub-blocks are made of whole bytes
			i=oo; k=128; tape_temp[0]=j=0; int w=0,v=0,n=0,z;
			while (i<samples-1&&(l=sample[i+0]+sample[i+1])<=h)
			{
				w+=l; if (l>m) tape_temp[j]|=k,v+=l,n+=2; if (!(k>>=1)) tape_temp[++j]=0,k=128; i+=2; // manually encode bits and bytes
				if (k==q) // byte boundary? check for separator! notice we use "couples", not "singles"; hence the APPROX_ZX2HZ(n,2) above
					if ((z=weight_singles(i,10))>=ll&&z<=lh
						&&(z=weight_singles(i+10,22))>=hl&&z<=hh
						&&(z=weight_singles(i+10+22,10))>=ll&&z<=lh)
						{ k=0; break; } // this is a valid separator!
			}
			i-=oo; tape_twotwo=i>>1; // x4 also ensures that `lh` (BIT0*3) is the threshold between Nx2 (BIT0 pair) and Nx4 (BIT1 pair)
			if (!tape_twotwo) break;
			tape_bit0w=i-n?APPROX_HZ2ZX(w-v,i-n):0;
			tape_bit1w=  n?APPROX_HZ2ZX(  v,  n):0;
			TAPE_BITWZERO;
			TAPE_FLAG_B(w,i*2-n);
			tape_tails=0; oo+=i; if (q!=128) encode_speedlock_1st(detect_config_prm1); // discard early bits
			tape_parity8b(tape_temp,tape_twotwo>>3);
			// in later tapes the edge lengths become increasingly similar and effectively undistinguishable, hence the problems above :_(
			if (k) break; // it's also worth stating that Speedlocks of this kind add a dummy trailer of 4x ZERO bits, i.e. 8x BIT0 edges
			tape_export(); // export the previous data block
			l=weight_singles(oo,10+22+10); // store separator
			tape_bit0w=APPROX_HZ2ZX(l,64);
			tape_bit1w=APPROX_HZ2ZX(l,32);
			tape_twotwo=encode_doubles(oo,tape_temp,q=10+22+10,approxy(l,3,64));
			tape_parity8b(tape_temp,3); tape_checksum^=248; // detect bad separator!
			oo+=q; //if (tape_temp[0]!=7||tape_temp[1]!=255||tape_temp[2]!=0) break; // the separator is wrong!
		}
	}
	if (!--flag_w) /*if (detect_config_prm2==1)*/ detect_config_prm2=0; // i.e. turn SPEEDLOCK 3+ into SPEEDLOCK 0, 1 or 2 after N iterations set by "-w N"
	tape_howtosum=2;
	tape_export_endblock();
}

void encode_call_single(void)
	{ tape_howtosum=0; encode_common_single(flag_r,flag_y); }

int detect_call_frankbruno(int o)
{
	int i; return (header_length=detect_singlez(o,&header_weight,NULL,NULL,1850,2250,flag_m))
		&&(i=approx(header_weight,header_length))
		&&(define_singles(o+header_length,NULL,NULL,NULL,16,16,i*1/4,i*3/4)) // the pseudo-sync is made of 16 BIT0 edges...
		&&(define_singles(o+header_length+16,NULL,NULL,NULL,16,16,i*3/4,i*5/4)) // ...and 16 BIT1; i.e. bytes $00,$00,$FF,$FF
		&&(middle_length=0,stream_length=define_singles(o+header_length,&stream_weight,&weight_l,&weight_h,24,1E7,i*1/4,i*5/4));
}

int detect_call_bleepload1(int o)
{
	// Bleepload1 blocks are made of strings of single-edged bytes, where the pilot is made of the same byte (neither 0 or 255) repeated dozens of times
	return (stream_length=detect_singles(o,&stream_weight,&weight_l,&weight_h,1E3,1E7,flag_t))
		&&encode_singles(o,buffer,16*8,APPROX2(weight_l+weight_h))==16*8 // the first bytes must follow a pattern (normal blocks 8 bits, musical blocks 12 bits)
		&&*buffer>0&&*buffer<255&&!memcmp(buffer,&buffer[3],12); // otherwise we'll turn the loader files into Bleepload1!
}
void encode_call_bleepload1(void)
	{ tape_howtosum=header_length=middle_length=0; encode_common_single(0,0); }

int detect_call_bleepload2(int o)
{
	// Bleepload2 tapes carry three types of blocks:
	// 1.- simple double-signal tone+sync+data chunks
	// 2.- data-less tones (useless, but we must keep them)
	// 3.- at least three complex musical loaders: "Bombscare", "Booty" and "Druid" (!)
	return (header_length=detect_singlez(o,&header_weight,NULL,NULL,750,7500,flag_m))&&!(header_length>=1080&&header_length<4800)
		&&(stream_length=0,!(middle_length=middle_doubles(oo+header_length,&middle_weight,ZX2HZ(1200*1/3),ZX2HZ(1440*3/2)))||
		(stream_length=define_doublez(oo+header_length+2,&stream_weight,&weight_l,&weight_h,header_length<960?3600:22,header_length<960?5200:520,ZX2HZ(1200*1/2),ZX2HZ(1440*5/2))));
}
void encode_call_bleepload2(void)
{
	tape_howtosum=0;
	if (stream_length<=0||stream_length>24) // short block or single tone?
		encode_common_double(flag_r,flag_y,-1);
	else // giant musical block! it's made of 11 bits, 2 edges, 10 bits, 2 edges...
	{
		tape_export_newblock(tape_name);
		stream_length=define_doubles(oo+header_length+2,&stream_weight,&weight_l,&weight_h,22,22,ZX2HZ(1280*3/4),ZX2HZ(1680*8/3));
		encode_common_double(0,1,-1); header_length=0;
		while ((middle_length=middle_doubles(oo,&middle_weight,ZX2HZ(2700),ZX2HZ(7700))) // 3600..6800
			&&(stream_length=define_doubles(oo+middle_length,&stream_weight,&weight_l,&weight_h,20,20,ZX2HZ(1280*3/4),ZX2HZ(1680*8/3))))
			{ tape_export(); encode_common_double(0,0,-1); }
		tape_export_endblock();
	}
}

int detect_call_ehservices(int o)
{
	int i; // EH Services blocks are based on frequency modulation (the base of the Kansas City standard): two short pulses make BIT0, one long pulse makes BIT1
	return (header_length=detect_singlez(o,&header_weight,&weight_l,&weight_h,4500,5500,flag_m))
		&&(i=approx(header_weight,header_length))
		&&(middle_length=middle_doubles(o+header_length,&middle_weight,i*1/2,i*3/2))
		&&(stream_length=define_singles(o+header_length+2,&stream_weight,&weight_l,&weight_h,16,21E5,i*1/4,i*5/4))
		//&&!strict_doubles(o+header_length+2,NULL,NULL,NULL,stream_length-8,stream_length-0,APPROX2(weight_l+weight_h))
		&&(stream_length=strict_hybrids(o+header_length+2,&stream_weight,&weight_l,&weight_h,stream_length-8,stream_length-0,APPROX2(weight_l+weight_h)));
}
void encode_call_hybrid(void)
	{ tape_howtosum=0; encode_common_hybrid(flag_r,flag_y); }

int detect_call_kansas(int o)
{
	int i; // the Kansas City blocks used in MSX1 tapes have VERY long syncs made of SHORT pulses!
	return (header_length=detect_singlez(o,&header_weight,&weight_l,&weight_h, 6700,31700,flag_m))
		&&(header_length<8700||header_length>29700)
		&&(i=approx(header_weight,header_length))
		&&(stream_length=define_singles(o+header_length,&stream_weight,&weight_l,&weight_h,16,21E5,i*1/2,i*5/2))
		&&(middle_length=0,stream_length=strict_hybrids(o+header_length,&stream_weight,&weight_l,&weight_h,stream_length-8,stream_length-0,APPROX2(weight_l+weight_h)));
}

unsigned char tape_kansas_temp[1<<16]; int tape_kansas_data;
int encode_kansas_bits(int n,int o,int l) // try encoding a series of samples from offset `o` and length `l` into a Kansas City stream of `n` base bits;
// returns amount of valid samples on success, ZERO otherwise. Notice that it relies on the `tape_kansas_*` variables, and doesn't overwrite the output buffer
{
	tape_kansasn[1]=(tape_kansasn[0]=n)*2; tape_kansas_data=0;
	int h,i,j=o,k=APPROX_ZX2HZ(tape_bit0w+tape_bit1w,2); unsigned char a,b,c;
	l+=o; while (o<l)//&&j<length(tape_kansas_temp))
	{
		i=0; //if (tape_kansasi) // already 0, see below
			//{ for (c=h=tape_kansasin*n*2;c&&o<l;--c) i+=sample[o++]; if (i>=k*h) break; }
		//else
			{ for (c=h=tape_kansasin*n;c&&o<l;--c) i+=sample[o++]; if (i<k*h) break; }
		if (c) return 0; // bad IN codes!
		/*if (tape_kansasio) // already 0, see below
		{
			a=0,b=128; do // build bytes from left (128) to right (1)
			{
				if ((i=sample[o++])<k) // BIT0 is the long signal in KANSAS CITY!!
					{ for (a|=b,c=h=n*2;--c&&o<l;) i+=sample[o++]; if (i>=k*h) break; }
				else
					{ for (c=h=n;--c&&o<l;) i+=sample[o++]; if (i<k*h) break; }
			}
			while (!c&&(b>>=1));
		}
		else*/
		{
			a=0,b=1; do // build bytes from right (1) to left (128)
			{
				if ((i=sample[o++])<k) // BIT0 is the long signal in KANSAS CITY!!
					{ for (a|=b,c=h=n*2;--c&&o<l;) i+=sample[o++]; if (i>=k*h) break; }
				else
					{ for (c=h=n;--c&&o<l;) i+=sample[o++]; if (i<k*h) break; }
			}
			while (!c&&(b<<=1));
		}
		if (c) return 0; // bad BYTE codes!
		tape_kansas_temp[tape_kansas_data++]=a;
		i=0; //if (tape_kansaso) // already 1, see below
			{ for (c=h=tape_kansason*n*2;c&&o<l;--c) i+=sample[o++]; if (i>=k*h) break; }
		//else
			//{ for (c=h=tape_kansason*n;c&&o<l;--c) i+=sample[o++]; if (i<k*h) break; }
		if (c) break; // bad OUT codes! (tolerate a bad last signal)
	}
	return o<=l?o-j:0;
}
void encode_call_kansas(void)
{
	int i,o=oo+header_length+middle_length; // `middle_length` should be zero in MSX1 blocks; not sure about others, if any
	encode_call_hybrid();
	{
		for (i=tape_onetwo>>3;--i>=0;) if (((tape_temp[i]>>1)^tape_temp[i])&0X55) break;
		if (i<0)
		{
			if (tape_onetwo&1) // special case: dangling last bit? get rid of it if possible!
			{
				int j=tape_onetwo&6; switch (j)
				{
					case 0: i=0X00,j=128; break;
					case 2: i=0X40,j= 32; break;
					case 4: i=0X50,j=  8; break;
					case 6: i=0X54,j=  2; break;
				}
				if (((tape_temp[tape_onetwo>>3]>>1)^tape_temp[tape_onetwo>>3])&i) return;
				if (!(tape_temp[tape_onetwo>>3]&j)) tape_tails+=sample[--stream_length];
				tape_tails+=sample[--stream_length],--tape_onetwo;
			}
			tape_twofor=splice_stream(tape_temp,tape_onetwo),tape_onetwo=0,tape_bit0l=4,tape_bit1l=2;
		}
	}
	if (tape_pzx|tape_onetwo) return; // PZX can store both 2:1 and 4:2; pure 2:1 cannot go further!
	if (!tape_tzx) // reencode the 4:2 stream into 2:2; it's more economic than 1:1 samples
		{ oo=o-header_length-middle_length; weight_l<<=1,weight_h<<=1; encode_call_double(); return; }
	if (!tape_tsx) return; // TZX 1.20 can store 4:2 and 2:1, but not KANSAS CITY, a TSX exclusive!
	// try reducing the stream into a pure Kansas City stream, whose parameters are as follows:
	tape_kansasi=0; tape_kansasin=1; // notice that tape_kansasi is always !tape_kansaso,
	tape_kansaso=1; tape_kansason=2; // and short pilot edges imply tape_kansasi=0.
	tape_kansasio=0; tape_kansas=0;
	if ((i=encode_kansas_bits(2,o,stream_length))&&stream_length-i<tape_kansason*2) ; // made of pulses...
	//else if ((i=encode_kansas_bits(1,o,stream_length))&&stream_length-i<tape_kansason) ; // ...or edges?
	else return; // failure, don't change anything!
	oo-=stream_length; oo+=stream_length=i;
	o=tape_bit0w; tape_bit0w=tape_bit1w; tape_bit1w=o; tape_bit0l=tape_kansasn[0]; tape_bit1l=tape_kansasn[1]; // BIT0 is the long signal in KANSAS CITY!!
	tape_twofor=0; memcpy(tape_temp,tape_kansas_temp,tape_kansas=tape_kansas_data); // the resulting KANSAS CITY data is new new data output
}

int detect_call_headerless(int o,int l,int k)
{
	return (stream_length=detect_doublez(o,&stream_weight,&weight_l,&weight_h,l,21E5,flag_t)) // ZX2HZ(660),0,0,ZX2HZ(840*5)
		&&encode_doubles(o,buffer,16*16,APPROX2(weight_l+weight_h))==16*8
		&&(header_length=middle_length=0,!chrcmp(buffer,(char)k,16));
}
void encode_call_headerless(void)
	{ tape_howtosum=tape_checksum=header_length=middle_length=0; encode_common_double(0,0,-1); }

int detect_call_operasoft(int o)
	{ return detect_call_headerless(o,1E5,0XAA); }
void encode_call_operasoft(void)
{
	encode_call_headerless(); if (tape_twotwo>16) // 16-bit CRC?
	{
		tape_howtosum=1; tape_checksum=0; int l=(tape_twotwo>>3)-2; for (int i=0;i<l;++i) tape_checksum+=tape_temp[i];
		tape_checksum=(tape_checksum^(tape_temp[l]+tape_temp[l+1]*256))&65535; // i.e. zero = OK!
	}
}

int detect_call_puffyssaga(int o)
	{ return detect_call_headerless(o,1E4,0X16); } // levels need a smaller value than 1E5

int detect_call_maubeuge(int o)
{
	if (header_length=detect_singlez(o,&header_weight,NULL,NULL,900,1200,flag_m)) return flag_w=1;
	flag_w=0; return detect_call_headerless(o,1E5,0X55);
}
void encode_call_maubeuge(void)
{
	if (flag_w) // key tone?
		tape_heads=1,tape_head[0][0]=header_length,tape_head[0][1]=APPROX_HZ2ZX(header_weight,header_length),oo+=header_length;
	else // headerless body!
		encode_call_headerless();
}

int detect_call_unilode(int o)
	{ return detect_call_headerless(o,1E5,0X20); }
void encode_call_unilode(void)
{
	encode_call_headerless(); if (tape_twotwo>8)
	{
		int i=0,l=tape_twotwo>>3; while (i<l&&tape_temp[i]==0X20) ++i; ++i;
		tape_howtosum=2; tape_checksum=0; tape_parity8b(&tape_temp[i],l-i);
	}
}

// there are two types of Keytone:
// one Keytone decryption block is always a ~40-bit chunk where BIT1 is four times as long as BIT0 (f.e. "Dragontorc")
// the other one is a 4000-edge 1280-T tone followed by a 50-ms pause and a 83-edge 1550-T tone ("They stole a million")
int detect_call_keytone(int o)
{
	return ((stream_length=choose_doublez(o,&stream_weight,&weight_l,&weight_h,64,96,ZX2HZ(1500),ZX2HZ(2500),ZX2HZ(7500),ZX2HZ(8500))) // 1st method
		&&!define_doubles(o+stream_length,NULL,NULL,NULL,64,21E5,ZX2HZ(750),ZX2HZ(3750)))
		||((header_length=define_singlez(o,&header_weight,NULL,NULL,69,4050,ZX2HZ(1150),ZX2HZ(1700))) // 2nd method
		&&(header_length<99||header_length>3950));
}
void encode_call_keytone(void)
{
	if (stream_length) // 1st method?
		header_length=0;
	else // 2nd method: more complex because the first pilot is fragile, its edge must be between 1300 and 1400 T
	{
		if (APPROX_HZ2ZX(header_weight,header_length)<1300) header_weight=approxy(header_weight,105,100); // 100<fix<110
		stream_length=0;
	}
	tape_howtosum=tape_checksum=middle_length=0; encode_common_double(0,0,-1);
}

// several Gremlin tapes rely on a polarity-sensible method (lifted from the C64?) that divides the data stream into sub-blocks
void config_call_gremlin0(void) { detect_config_prm1=0; }
void config_call_gremlin1(void) { detect_config_prm1=1; }
void config_call_gremlin2(void) { detect_config_prm1=2+(flag_w&1); } // i.e. "-w 1" will make GREMLIN2 pilots ODD instead of EVEN
int detect_call_gremlins(int o)
{
	return (header_length=define_singlez(o,&header_weight,NULL,NULL,824,1224,ZX2HZ(2500),ZX2HZ(3100)))
		&&(middle_length=middle_doubles(o+header_length,&middle_weight,ZX2HZ(550),ZX2HZ(1650)))
		&&(stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,32,21E5,ZX2HZ(600),ZX2HZ(3000)));
}
void encode_call_gremlins(void)
{
	tape_howtosum=0; encode_common_double(0,1,-1);
	if (detect_config_prm1&1) // CPC tapes need EVEN pilots
		{ if (tape_head[0][0]&1) ++tape_head[0][0]; }
	else // Spectrum tapes need ODD pilots
		{ if (!(tape_head[0][0]&1)) --tape_head[0][0]; }
	if (detect_config_prm1&2)
	{
		tape_export_newblock(tape_name);
		char q=0; int k=approx(tape_head[0][1]*3,14); // average pilot 2851 T, average BIT0 609 T
		for (;;)
		{
			int i=0,j,l,h;
			if (!q) // the first gap is short, 2 edges; approx. 2200,3000 T, or more rarely 1100,2100 T
				q=1,i=middle_doubles(oo,&h,ZX2HZ(k*4),ZX2HZ(k*10)); // ~2600,~5800
			else // the following gaps are 12 long edges: 800,1900,2300,1000,700,500,600,600,500,900,2600,2900 T (=15300 T); rarely 14, turning 2300 into 1700+200+200
			{
				l=ZX2HZ(k*7); h=ZX2HZ(k*11); // in practice, we only want to reach the final two huge edges
				while (i<16&&middle_doubles(oo+i,NULL,1,l)) i+=2;
				if (middle_doubles(oo+i,NULL,l,h)) i+=2; else i=0;
			}
			if (!i) break; // give up if the super-block is over
			tape_export();
			tape_heads=i; for (i=0;i<tape_heads;++i)
				tape_head[i][0]=1,tape_head[i][1]=HZ2ZX(sample[oo++]);
			if (!(i=define_doublez(oo,&h,NULL,NULL,32,21E5,ZX2HZ(k),ZX2HZ(k*5)))) break;
			j=divide_doubles(oo,&l,i,ZX2HZ(k*3));
			tape_bit0l=tape_bit1l=2;
			tape_bit0w=  j?APPROX_HZ2ZX(  l,  j):0;
			tape_bit1w=i-j?APPROX_HZ2ZX(h-l,i-j):0;
			TAPE_BITWZERO;
			tape_twotwo=encode_doubles(oo,tape_temp,i,ZX2HZ(tape_bit0w+tape_bit1w));
			TAPE_FLAG_B(h,i*2-j);
			tape_checksum=0; tape_parity8b(tape_temp,tape_twotwo>>3);
			oo+=i;
		}
		tape_export_endblock();
	}
}

// Microkey: a clone of the Amstrad CPC firmware where the tone, the sync and the first byte match the 1000 BAUD constants, but the following bytes switch to 2000 BAUD mode!
int detect_call_microkey(int o)
{
	int i=APPROX2(sample[o]+sample[o+1]);
	header_length=define_singlez(o,&header_weight,NULL,NULL,4096-400,4096+400,i*4/5,i*5/4);
	if (!header_length) return 0;
	i=approx(header_weight,header_length);
	return (middle_length=middle_doubles(o+header_length,&middle_weight,i*1/2,i*3/2))
		&&(stream_length=define_doubles(o+header_length+2,&stream_weight,&weight_l,&weight_h,16,16,i*1/2,i*5/2))
		&&define_doublez(o+header_length+2+16,NULL,NULL,NULL,32,21E5,i*1/4,i*5/4);
}
void encode_call_microkey(void)
{
	int i=approx(header_weight,header_length),j; // we'll need this below
	tape_howtosum=0; encode_common_double(flag_r,flag_y,-1); tape_export(); // export heads + first byte
	stream_length=define_doublez(oo,&stream_weight,&weight_l,&weight_h,1E5,21E5,i*1/4,i*5/4);
	tape_howtosum=1; header_length=middle_length=0; encode_common_double(0,0,flag_0);
	i=tape_twotwo%(258<<3),j=(tape_twotwo-1)>>3;
	tape_checksum=0; tape_ccitt256(&tape_temp[0],tape_twotwo>>3); // the CCITT spans the bytes between the ID (first) and the trailer
	if (i<27||i>37||tape_temp[j-3]+tape_temp[j-2]+tape_temp[j-1]!=765) ++tape_checksum; // the ideal trailer is made of four 255 bytes!
}

// "Split Personalities" (are there other games using this same loader?) divides the tape into variable-length sub-blocks
// whose length is tied to the bit contents: this effectively makes each sub-block last the same time, not the same bits
int detect_call_splitpers(int o)
{
	return (header_length=define_singlez(o,&header_weight,NULL,NULL,3920,4320,ZX2HZ(1120),ZX2HZ(1520)))
		&&(middle_length=middle_doubles(o+header_length,&middle_weight,ZX2HZ(550),ZX2HZ(1650)))
		&&(stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,32,21E5,ZX2HZ(600),ZX2HZ(3600)));
}
void encode_call_splitpers(void)
{
	encode_common_heads(flag_r?flag_r:2,1); oo+=header_length+middle_length; header_length=tape_howtosum=0;
	int k0=ZX2HZ(600),k1=ZX2HZ(1800),k2=ZX2HZ(2400),k3=ZX2HZ(3600),j=-1,k=0; // the first sub-block is one bit longer
	for (;;)
	{
		int h=0,i,l=oo; while (l+2<samples&&((k&7)||j<128)&&(i=sample[l]+sample[l+1])>k0&&i<=k3)
			{ h+=i; ++k; l+=2; j+=i>=k1?2:1; }
		if (!(stream_length=i=l-oo)) break;
		j=divide_doubles(oo,&l,i,k1);
		tape_bit0l=tape_bit1l=2;
		tape_bit0w=  j?APPROX_HZ2ZX(  l,  j):0;
		tape_bit1w=i-j?APPROX_HZ2ZX(h-l,i-j):0;
		TAPE_BITWZERO;
		tape_twotwo=encode_doubles(oo,tape_temp,i,ZX2HZ(tape_bit0w+tape_bit1w));
		TAPE_FLAG_B(h,i*2-j);
		tape_checksum=0; tape_parity8b(tape_temp,tape_twotwo>>3);
		oo+=i;
		if (!(middle_length=middle_doubles(oo,&middle_weight,k0,k3))) break; // each sub-block starts with two syncs (in fact, a modified bit pair)
		tape_export();
		encode_common_heads(0,0); oo+=middle_length;
		j=middle_weight>k2?1:0; k=1; // following sub-blocks use the mid-syncs as the 1st bit
	}
}

// "Carson City" is a French CPC game that relies on pilots made of short edges and syncs made of long edges;
// the reversal of the usual state. It's also polarity-fixed!

int detect_call_carsoncity(int o)
{
	return (header_length=define_singlez(o,&header_weight,NULL,NULL,384,4352,ZX2HZ(212),ZX2HZ(612)))
		&&(middle_length=middle_doubles(o+header_length,&middle_weight,ZX2HZ(1448),ZX2HZ(1848)))
		&&(stream_length=define_doublez(o+header_length+2,&stream_weight,&weight_l,&weight_h,32,21E5,ZX2HZ(412),ZX2HZ(2472)));
}
void encode_call_carsoncity(void)
	{ tape_howtosum=0; encode_common_double(flag_r,flag_y,flag_0); }

typedef int int_f_int(int); typedef void nil_f_nil(void);
typedef struct { char t_name[12]; nil_f_nil *config; int_f_int *detect; nil_f_nil *encode; } t_method;
void config_call_null(void) { ; } // nil!
int detect_call_null(int o) { return 0; }
#define encode_call_null config_call_null
t_method method[]={
	{ "default"	,config_call_null	,detect_call_null	,encode_call_null	},
	{ "alkatraz"	,config_call_alkatraz	,detect_call_double	,encode_call_double	},
	{ "amstrad"	,config_call_null	,detect_call_amstrad	,encode_call_amstrad	},
	{ "bleepload1"	,config_call_null	,detect_call_bleepload1	,encode_call_bleepload1	},
	{ "bleepload2"	,config_call_null	,detect_call_bleepload2	,encode_call_bleepload2	},
	{ "carsoncity"	,config_call_null	,detect_call_carsoncity	,encode_call_carsoncity	},
	{ "cassys"	,config_call_null	,detect_call_cassys	,encode_call_cassys	},
	{ "ehservices"	,config_call_null	,detect_call_ehservices	,encode_call_hybrid	},
	{ "frankbruno"	,config_call_null	,detect_call_frankbruno	,encode_call_single	},
	{ "gremlin0"	,config_call_gremlin0	,detect_call_gremlins	,encode_call_gremlins	},
	{ "gremlin1"	,config_call_gremlin1	,detect_call_gremlins	,encode_call_gremlins	},
	{ "gremlin2"	,config_call_gremlin2	,detect_call_gremlins	,encode_call_gremlins	},
	{ "hexagon"	,config_call_hexagon	,detect_call_double	,encode_call_double	},
	{ "kansas"	,config_call_null	,detect_call_kansas	,encode_call_kansas	},
	{ "keytone"	,config_call_null	,detect_call_keytone	,encode_call_keytone	},
	{ "maubeuge"	,config_call_null	,detect_call_maubeuge	,encode_call_maubeuge	},
	{ "microkey"	,config_call_null	,detect_call_microkey	,encode_call_microkey	},
	{ "operasoft"	,config_call_null	,detect_call_operasoft	,encode_call_operasoft	},
	{ "poliload"	,config_call_null	,detect_call_poliload	,encode_call_spectrum	},
	{ "puffyssaga"	,config_call_null	,detect_call_puffyssaga	,encode_call_headerless	},
	{ "ricochet"	,config_call_ricochet	,detect_call_double	,encode_call_ricochet	},
	{ "spectrum"	,config_call_spectrum	,detect_call_double	,encode_call_spectrum	},
	{ "specvar0"	,config_call_specvar0	,detect_call_double	,encode_call_spectrum	},
	{ "specvar1"	,config_call_specvar1	,detect_call_double	,encode_call_spectrum	},
	{ "specvar2"	,config_call_specvar2	,detect_call_double	,encode_call_spectrum	},
	{ "speedlock0"	,config_call_speedlock0	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock1"	,config_call_speedlock1	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock2"	,config_call_speedlock2	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock3"	,config_call_speedlock3	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock4"	,config_call_speedlock4	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock5"	,config_call_speedlock5	,detect_call_speedlockz	,encode_call_speedlocks	},
	{ "speedlock6"	,config_call_speedlock6	,detect_call_speedlocks	,encode_call_speedlocks	},
	{ "speedlock7"	,config_call_speedlock7	,detect_call_speedlockz	,encode_call_speedlocks	},
	{ "splitpers"	,config_call_null	,detect_call_splitpers	,encode_call_splitpers	},
	{ "unilode"	,config_call_null	,detect_call_unilode	,encode_call_unilode	},
	{ "zydroload"	,config_call_null	,detect_call_zydroload	,encode_call_zydroload	},
};

// decoding-only vars and funcs --------------------------------------------- //

// CSW1 output compression

char csw_1st=1,csw_sign=0; int csw_queue=0,csw_remain=0,thz=0,hzt=0;
#define csw_calc() (hzt+=ZX1HZ,thz=hzt/hz,hzt%=hz)
void csw_send(int t,int s) // send 0 or 1 (`s&1`) for `t` T
{
	while (csw_remain>=thz) { ++csw_queue,csw_remain-=thz,csw_calc(); }
	csw_remain+=t; if (csw_queue)
	{
		if (csw_1st) csw_1st=0,fsend4(s&1); // store 1st sign!
		if (((csw_sign^s)&1))
		{
			if (csw_queue>0&&csw_queue<256) fsend1(csw_queue); else fsend1(0),fsend4(csw_queue);
			csw_queue=0; ++csw_sign;
		}
	}
}
void csw_init(void) { fwrite1(csw24b,24); fsend4((1<<24)+(hz<<8)+1); csw_calc(); } // write CSW1 header and setup variables
void csw_exit(void) { csw_send(thz>>1,~csw_sign); } // force a dummy update with the last sample before flushing the buffer

// TZX1 and PZX1 input logic

void un_clock(int t) { static int tt=0; tt+=t; seconds+=tt/ZX1HZ; tt%=ZX1HZ; } // tick the clock during decoding; different to do_clock()

char *tape_getpascal(int n) // load a string of size `n` in the temporary buffer
	{ int i=0; while (n--) buffer[i++]=frecv1(); buffer[i]=0; return buffer; }
int tape_tzxskip(int i) // returns data length of the current TZX block type `i`; returns length in bytes
{
	switch (i)
	{
		case 0X10: return frecv2(),frecv2();
		case 0X11: return fskip1(15),frecv3();
		case 0X12: return 4;
		case 0X13: return frecv1()*2;
		case 0X14: return fskip1(7),frecv3();
		case 0X15: return fskip1(5),frecv3();
		case 0X20: case 0X23: case 0X24: return 2;
		case 0X21: case 0X30: return frecv1();
		case 0X22: case 0X25: case 0X27: return 0;
		case 0X26: return frecv2()*2;
		case 0X28: case 0X32: return frecv2();
		case 0X31: return frecv1(),frecv1();
		case 0X33: return frecv1()*3;
		case 0X35: return fskip1(10),frecv4();
		case 0X5A: return 9;
		default: return frecv4();
	}
}
int tape_tzxtell(void) // tells the position of the current TZX block (0 is first)
{
	int z=ftell1(); fseek1(10); // skip "ZXTape!"
	int n=-1,i; while ((i=frecv1())>=0&&ftell1()<z) ++n,fskip1(tape_tzxskip(i));
	fseek1(z); return n;
}
void tape_tzxseek(int n) // moves to the TZX block `p`, where 0 means the first one
{
	fseek1(10); // skip "ZXTape!"
	int i; while (n>0&&(i=frecv1())>=0) fskip1(tape_tzxskip(i)),--n;
}

int tape_play_1,tape_play_n,tape_play_t;

void tape_init19(int np,int as) // setup both TZX BLOCK $19 halves
{
	int i=0; for (;i<as;++i)
	{
		tape_word[i][0]=frecv1(),tape_word[i][np+1]=-1;
		for (int j=0;j<np;++j)
		{
			int k=frecv2(); if (!k) k=-1; // ZERO is the end marker!
			tape_word[i][j+1]=k;
		}
	}
	for (;i<256;++i)
		tape_word[i][0]=0,tape_word[i][1]=-1; // disable unused words
}
int tape_playword(int c) // decode a "word" {type,duration...,-1}
{
	int t=0,k=tape_word[c][0]&3; tape_sign=k<2?tape_sign+k:k;
	for (int j=0;(k=tape_word[c][++j])>=0;++tape_sign,t+=k)
		tape_play_1+=c&1,++tape_play_n,tape_play_t+=k,csw_send(k,tape_sign);
	return t;
}
int tape_play19(void) // play the TZX BLOCK $19 1st half (tone)
{
	int t=0; for (;tape_heads;--tape_heads)
		for (int i=frecv1(),j=frecv2();j;--j)
			t+=tape_playword(i);
	un_clock(t); return approx(t,ZXKHZ);
}
int tape_play(void) // plays a generalised block; returns the consumed milliseconds
{
	int t=0,i,j,k; unsigned char a=(1<<tape_width)-1,b=0;
	for (i=0;tape_heads;++i,--tape_heads) // play head (tone+sync)
		for (j=0;j<tape_head[i][0];t+=k,++tape_sign,++j)
			k=tape_head[i][1],csw_send(k,tape_sign);
	for (i=0;tape_waves;t+=tape_wave,++tape_sign,--tape_waves) // play samples
	{
		if (--i<0) b=frecv1(),i=7;
		csw_send(tape_wave,tape_sign=b>>i);
	}
	while (tape_kansas)
	{
		k=tape_word[tape_kansasi][1];
		for (i=tape_kansasn[tape_kansasi]*tape_kansasin;i>0;--i)
			csw_send(k,tape_sign),t+=k,++tape_sign;
		b=frecv1(); --tape_kansas;
		if (tape_kansasio)
			for (j=8;j--;)
				{ k=tape_word[a=(b>>j)&1][1]; for (i=tape_kansasn[a];i;--i) csw_send(k,tape_sign),t+=k,++tape_sign; }
		else
			for (j=0;j<8;++j)
				{ k=tape_word[a=(b>>j)&1][1]; for (i=tape_kansasn[a];i;--i) csw_send(k,tape_sign),t+=k,++tape_sign; }
		k=tape_word[tape_kansaso][1];
		for (i=tape_kansasn[tape_kansaso]*tape_kansason;i>0;--i)
			csw_send(k,tape_sign),t+=k,++tape_sign;
	}
	for (i=0;tape_datas;t+=tape_playword((b>>i)&a),--tape_datas) // play bytes
		if ((i-=tape_width)<0) b=frecv1(),i=8-tape_width;
	if (tape_tails) // play tail (silence)
	{
		if (tape_tails>ZXKHZ)
			csw_send(ZXKHZ,tape_sign),csw_send(tape_tails-ZXKHZ,0),tape_sign=1;
		else
			csw_send(tape_tails,tape_sign),++tape_sign;
		t+=tape_tails; tape_tails=0;
	}
	un_clock(t); return approx(t,ZXKHZ);
}

// the main procedure proper ------------------------------------------------ //

const char tzx10b[10]="ZXTape!\032\001",pzx10b[10]="PZXT\002\000\000\000\001\000",
	ok_bytes[]="%d:%d bytes.\n",bad_target[]="error: cannot create target!\n";
int plusatoi(char *s) { return atoi(*s=='+'?s+1:s); } // handle "+1234" as "1234"
int main(int argc,char *argv[])
{
	int h,i,j,k,l; char *si=NULL,*so=NULL,*flag_l=NULL; int flag_c=0,flag_ee=5,flag_p=800;
	char methods=0,method_index[9],*method_t_name=NULL; int method_timer[9];
	char flag_yy=0; // detect short-tone-long-sync blocks, rare but extant
	nil_f_nil *method_config=config_call_null;
	int_f_int *method_detect=detect_call_null;
	nil_f_nil *method_encode=encode_call_null;
	for (i=1;i<argc;++i)
		if (argv[i][0]=='-')
			if (argv[i][1]=='-')
			{
				j=2; h=0; if (argv[i][2]>='0'&&argv[i][2]<='9') // timestamp for the extra methods? (f.e. "--5:30 specvar2")
				{
					do
					{
						l=0; do l=l*10+argv[i][j++]-'0'; while (argv[i][j]>='0'&&argv[i][j]<='9');
						h+=l; if (argv[i][j]!=':') break; // last digit?
						h*=60; ++j;
					}
					while (argv[i][j]>='0'&&argv[i][j]<='9');
					if (argv[i][j]||!h)
						i=argc; // help!
					else
						++i,j=0; // take the method from the next parameter's first character
				}
				k=length(method); if (i<argc)
					for (k=0;k<length(method);++k)
						if (!strcmp(method[k].t_name,&argv[i][j])) break;
				if (k>=length(method))
					i=argc; // help!
				else if (j) // main method
				{
					method_config=method[k].config,
					method_detect=method[k].detect,
					method_encode=method[k].encode;
					method_t_name=method[k].t_name;
				}
				else if (methods<length(method_timer)) // timer-based method
				{
					method_index[methods]=k,
					method_timer[methods]=h,
					++methods;
				}
			}
			else if (argv[i][1])
				#define FETCH_STR(t) ((argv[i][++j]||(j=0,++i<argc))?t=&argv[i][j],j=-1:0)
				#define FETCH_INT(t,l,h) ((argv[i][++j]||(j=0,++i<argc))?t=plusatoi(&argv[i][j]),j=-1,t>=l&&t<=h:0)
				for (j=0;++j>0&&i<argc&&argv[i][j];)
					switch (k=argv[i][j])
					{
						case 'c':
							if (!FETCH_INT(flag_c,1,1E9))
								i=argc; // help!
							break;
						case 'm':
							if (!FETCH_INT(flag_m,1,100))
								i=argc; // help!
							break;
						case 'p':
							if (!FETCH_INT(flag_p,1,1E9))
								i=argc; // help!
							break;
						case 't':
							if (!FETCH_INT(flag_t,1,100))
								i=argc; // help!
							break;
						case 'z':
							if (!FETCH_INT(flag_z,1,100))
								i=argc; // help!
							break;
						case 'w':
							if (!FETCH_INT(flag_w,1,1E9))
								i=argc; // help!
							break;
						case 'f':
							if (!FETCH_INT(hz,8000,64000))
								i=argc; // help!
							break;
						case 'e':
							if (!FETCH_INT(flag_e,1,9))
								i=argc; // help!
							break;
						case 'E':
							if (!FETCH_INT(flag_ee,1,9))
								i=argc; // help!
							break;
						case 'r':
							if (!FETCH_INT(flag_r,1,2))
								i=argc; // help!
							break;
						case 'l':
							if (!FETCH_STR(flag_l))
								i=argc; // help!
							break;
						case 'L':
							flag_ll=1;
							break;
						case '8':
						case '4':
						case '5':
						case '0':
						case '1':
							flag_0=k-'0'; break;
						case 'b':
							flag_b=1; break;
						case 'y':
							flag_y=1; break;
						case 'Y':
							flag_yy=1; break;
						case 'x':
							tape_tzx=0; break;
						case 'X':
							uprint_t=0; break;
						//case 'h':
						default:
							i=argc; // help!
							break;
					}
			else
				i=argc; // help!
		else if (!si)
			si=argv[i];
		else if (!so)
			so=argv[i];
		else i=argc; // help!
	if (i>argc||!si)
	{
		si=buffer; for (i=0;i<length(method);++i)
		{
			if (i&&!(i&3)) *si++='\n'; // every 4 names
			si+=sprintf(si,"\t--%s",method[i].t_name);
		}
		*si++='\n'; *si=0;
		return printf("CSW2CDT " MY_VERSION " " MY_LICENSE "\n"
			"\n"
			"encoding from CSW to CDT/TZX:\n"
			"\tcsw2cdt [option..] source.csw [target.<cdt|tzx|pzx|tsx..>]\n"
			"\t-0\tpad all incomplete bytes with 0s\n"
			"\t-1\tpad all incomplete bytes with 1s\n"
			"\t-4\tclip or pad incomplete bytes with 0s\n"
			"\t-5\tclip or pad incomplete bytes with 1s\n"
			"\t-8\tclip all incomplete bytes\n"
			"\t-b\tforce BIT1:BIT0 = 2\n"
			"\t-y\tforce SYNC1 = SYNC2\n"
			"\t-Y\tallow TONE < SYNC\n"
			"\t-x\tTZX 1.00 compatible mode\n"
			"\t-X\tshow block count in log file\n"
			"\t-l F\tset log file path ('-' for stdout)\n"
			"\t-L\tabridge TONE+SYNC blocks in log file\n"
			"\t-e n\tfrequency bias (from 1 to 9) [5]\n"
			"\t-E N\tpositive-negative bias (1-9) [5]\n"
			"\t-c N\tminimum sample size of edges [%d]\n"
			"\t-m N\tminimum SYNC:TONE percentage [%d]\n"
			"\t-p N\tminimum amount of tone edges [%d]\n"
			"\t-t N\tminimum BIT0:BIT1 percentage [%d]\n"
			"\t-z N\tminimum BLOCK $10 percentage [%d]\n"
			"\t-r N\tforce odd/even (1/2) pilot tone\n"
			"\t-w N\tcurrent encoding fine-tune\n"
			"%s"
			"\t--MM:SS TYPE\tenable encoding TYPE after MM min SS sec\n"
			"decoding from CDT/TZX to CSW:\n"
			"\tcsw2cdt [option..] source.<cdt|tap|tzx|tsx..> [target.csw]\n"
			"\t-f N\toutput sample rate in Hz [%d]\n"
			"\n" GPL_3_INFO "\n"
			,flag_c,flag_m,flag_p,flag_t,flag_z,buffer,(int)hz
		),1;
	}
	if (!(fi=fopen(si,"rb")))
		return fprintf(stderr,"error: cannot open source!\n"),1;
	if (*buffer=0,fread1(buffer,24)&&!memcmp(buffer,csw24b,24)) // encoding?
	{
		if (!(fo=fopen(so=autosuffix(so,si,".cdt"),"wb")))
			return fclose(fi),fprintf(stderr,bad_target),1;
		if (tape_pzx=endswith(so,".pzx")||endswith(so,".cdp"))
			fwrite1(pzx10b,10); // PZX signature
		else
		{
			if (endswith(so,".tsx")) // without "-x" TSX files will be 1.21
				if (tape_tsx=1,tape_tzx) tape_tzx=21; // 1.00 otherwise
			fwrite1(tzx10b,9); fsend1(tape_tzx); // TZX signature + revision
		}
		flag_l=autosuffix(flag_l,so,".log");
		fu=strcmp(flag_l,"-")?fopen(flag_l,"w"):stdout;
		hz=approxy(((fget4()>>8)&0X3FFFF)-65536,100,95+flag_e); tape_sign=fget4()&1; zx2hz_setup();
		flag_m<<=1; // set double-percentage
		fseek(fi,0,SEEK_END); samples=ftell(fi); fseek(fi,32,SEEK_SET);
		sample=malloc(sizeof(int[samples])); flag_ee-=5;
		for (sample[0]=0,j=tape_sign;(i=frecv1())>=0;)
			sample[j++]=(i?i:frecv4())+flag_ee,flag_ee=-flag_ee;
		while (samples!=j)
		{
			for (samples=j,i=j=0;i+2<samples;)
				if (sample[i+1]>flag_c)
					sample[j++]=sample[i++];
				else
					sample[j++]=sample[i]+sample[i+1]+sample[i+2],i+=3;
			while (i<samples) sample[j++]=sample[i++];
		}
		uprintl(); uprinth(); uprintl();
		{
			time_t now=time(NULL); char *t=buffer; t+=strftime(t,256,"%Y-%m-%d %H:%M:%S CSW2CDT " MY_VERSION,localtime(&now));
			for (i=1;i<argc;++i)
				if (argv[i]!=si&&argv[i]!=so)
				{
					if (argv[i][0]!='-'||argv[i][1]!='l')
						t+=sprintf(t," %s",argv[i]); // don't print "-l*"
					else if (!argv[i][2]) ++i; // catch special case "-l LOG"
				}
			*t=0; tape_export_infos(buffer);
		}
		method_config(); j=ZX2HZ(4444); // empirical single upper limit: a signal can't last longer than this!
		int m_timer=methods?method_timer[0]*hz:-1;
		while (oo+8<samples)
		{
			if (!(oo&255)) fprintf(stderr,"%d\015",oo); // progress...
			if (sample[l=oo]>=j)
				tape_tails+=sample[oo++]; // very long, skip
			else if (method_detect(oo))
			{
				tape_export(); // export past block
				tape_name=method_t_name; method_encode();
			}
			else if ((header_length=detect_singlez(oo,&header_weight,NULL,NULL,flag_p,4E4,flag_m))
				&&((i=approx(header_weight,header_length))<j))
			{
				if (middle_length=middle_doubles(oo+header_length,&middle_weight,i*1/2,i*3/2)) // long TONE and short SYNC?
				{
					// make several guesses on the type of block, with the help of ranges of possible lengths and weights
					if (header_length>4096-400&&header_length<4096+400
						&&i* 3<middle_weight*4&&i* 5>middle_weight*4
						&&(stream_length=define_doublez(oo+header_length+2,&stream_weight,&weight_l,&weight_h,80,21E5,i*1/2,i*5/2))
						&&(h=stream_length%(258*16))>70&&h<90
						&&i* 3<weight_l    * 5&&i* 5>weight_l    * 3
						&&i* 3<weight_h    * 2&&i* 5>weight_h    * 2)
						{ tape_export(); tape_name="amstrad*",weight_h=(weight_l=i)*2; encode_call_amstrad(); } // PILOT=BIT1=2x(SYNC1=SYNC2=BIT0)
					else if (((header_length>3222-300&&header_length<3222+300)||(header_length>8062-400&&header_length<8062+400))
						&&(h=HZ2ZX(i))>=2168*7/8&&h<=2168*9/8
						&&i* 3<middle_weight*6&&i* 5>middle_weight*6
						&&(stream_length=define_doublez(oo+header_length+2,&stream_weight,&weight_l,&weight_h,48,21E5,i*3/5,i*9/5))
						&&i* 6<weight_l    *10&&i*10>weight_l    *10
						&&i* 6<weight_h    * 5&&i*10>weight_h    * 5)
						{ tape_export(); tape_name="spectrum*",weight_h=(weight_l=approxy(i,19,24))*2; encode_call_spectrum(); } // i.e. 1710:2168
					else if (stream_length=detect_doublez(oo+header_length+2,&stream_weight,&weight_l,&weight_h,48,21E5,flag_t))
						{ tape_export(); tape_name=""; encode_call_double(); } // unknown method, just do it
					else
						tape_tails+=sample[oo++]; // no DATA, skip
				}
				else if (middle_length=middle_doubles(oo+header_length,&middle_weight,i*5/2,i*9/2)) // short TONE and long SYNC?
				{
					if (((header_length>=6700&&header_length<8700)||(header_length>=29700&&header_length<31700))
						&&(stream_length=define_singlez(oo+header_length,&stream_weight,&weight_l,&weight_h,16,21E5,i*1/2,i*5/2))
						&&(stream_length=strict_hybrids(oo+header_length,&stream_weight,&weight_l,&weight_h,stream_length-8,stream_length-0,APPROX2(weight_l+weight_h))))
						{ tape_export(); tape_name="kansas*",middle_length=0; encode_call_kansas(); }
					else if (flag_yy&&(stream_length=detect_doublez(oo+header_length+2,&stream_weight,&weight_l,&weight_h,48,21E5,flag_t)))
						{ tape_export(); tape_name=""; encode_call_double(); } // unknown method, just do it
					else
						tape_tails+=sample[oo++]; // no DATA, skip
				}
				else
					tape_tails+=sample[oo++]; // no SYNC, skip
			}
			else
				tape_tails+=sample[oo++]; // no TONE, skip
			if (m_timer>0) // timestamp set by second method?
			{
				do m_timer-=sample[l++]; while (l<oo); // update timer after encoding
				static char methodz=0; while (m_timer<=0&&methodz<methods)
				{
					method_config=method[method_index[methodz]].config,
					method_detect=method[method_index[methodz]].detect,
					method_encode=method[method_index[methodz]].encode;
					method_t_name=method[method_index[methodz]].t_name;
					method_config();
					if (++methodz<methods) m_timer+=(method_timer[methodz]-method_timer[methodz-1])*hz;
				}
			}
		}
		while (oo<samples) tape_tails+=sample[oo++]; // build final silence
		tape_export_endblock(); tape_export(); flush1(); // farewell + flush!
		do_clock(); uprintz(i=ftell(fo)); uprintl(); fclose(fu);
		printf(ok_bytes,(int)ftell(fi),i);
		return fclose(fi),fclose(fo),0;
	}
	if (!memcmp(buffer,tzx10b,8)&&buffer[8]==1) // TZX decoding?
	{
		if (!(fo=fopen(autosuffix(so,si,".csw"),"wb")))
			return fclose(fi),fprintf(stderr,bad_target),1;
		fseek(fi,10,SEEK_SET); // rewind!
		tape_sign=1; csw_init(); // signature + setup
		hz=approxy(hz,100,95+flag_e);
		fu=stdout; uprintl(); uprintm(); uprintl();
		int loopleft=0,looppast,loopseek; // LOOP: blocks $24 + $25
		int callleft=0,callpast,callseek; // CALL: blocks $26 + $27
		while ((i=frecv1())>0)
			switch (/*++ordinal,printf("%c%03d [%02X] ",topdigit(ordinal/1000),ordinal%1000,i),*/i)
			{
				case 0X10:
					tape_tails=frecv2()*ZXKHZ;
					tape_datas=(i=frecv2())<<3;
					tape_head[0][0]=frecv1()<128?8062:3222; fskip1(-1); // caution: REAL pilot lengths are EVEN!!
					tape_head[0][1]=2168;
					tape_head[1][0]=tape_head[2][0]=1;
					tape_head[1][1]=667;
					tape_head[2][1]=735;
					tape_heads=3;
					tape_word[0][0]=tape_word[1][0]=0;
					tape_word[0][1]=tape_word[0][2]=855;
					tape_word[1][1]=tape_word[1][2]=1710;
					tape_word[0][3]=tape_word[1][3]=-1;
					tape_width=1;
					uprintf("NORMAL%13dx2168 0667,0735 0855:1710 100%7d:8   1000%9d\n",tape_head[0][0],i,tape_play());
					break;
				case 0X11:
					tape_head[0][1]=frecv2();
					tape_head[1][0]=tape_head[2][0]=1;
					tape_head[1][1]=frecv2();
					tape_head[2][1]=frecv2();
					tape_heads=3;
					tape_word[0][0]=tape_word[1][0]=0;
					tape_word[0][1]=tape_word[0][2]=frecv2();
					tape_word[1][1]=tape_word[1][2]=frecv2();
					tape_word[0][3]=tape_word[1][3]=-1;
					tape_width=1;
					tape_head[0][0]=frecv2();
					if (!(tape_datas=frecv1())) tape_datas=8; // avoid accidents!
					h=tape_tails=frecv2()*ZXKHZ;
					i=tape_datas+=(frecv3()-1)<<3;
					uprintf("CUSTOM%13dx%04d %04d,%04d %04d:%04d%4d%7d:%c%7d%9d\n",
						tape_head[0][0],tape_head[0][1],
						tape_head[1][1],tape_head[2][1],
						tape_word[0][1],tape_word[1][1],
						approx(256500,tape_word[0][1]+tape_word[1][1]),
						(i+7)>>3,((i+7)&7)+'1',h/ZXKHZ,tape_play());
					break;
				case 0X12:
					tape_head[0][1]=frecv2();
					tape_head[0][0]=frecv2();
					tape_heads=1;
					//uprintf("PURE TONE, %d ms\n",tape_play());
					uprintf("TONE  %13dx%04d --------- --------- --- -------- ------%9d\n",
						tape_word[0][1],tape_word[1][1],tape_play());
					break;
				case 0X13:
					h=tape_heads=frecv1();
					for (i=0,l=0;i<tape_heads;++i)
						tape_head[i][0]=1,l+=tape_head[i][1]=frecv2();
					uprintf("SYNC           ---------%5dx%04d --------- --- -------- ------%9d\n",
						h,h?approx(l,h):0,tape_play());
					break;
				case 0X14:
					tape_word[0][0]=tape_word[1][0]=0;
					tape_word[0][1]=tape_word[0][2]=frecv2();
					tape_word[1][1]=tape_word[1][2]=frecv2();
					tape_word[0][3]=tape_word[1][3]=-1;
					tape_width=1;
					if (!(tape_datas=frecv1())) tape_datas=8; // avoid accidents!
					h=tape_tails=frecv2()*ZXKHZ;
					i=tape_datas+=(frecv3()-1)<<3;
					uprintf("DATA           --------- --------- %04d:%04d%4d%7d:%c%7d%9d\n",
						tape_word[0][1],tape_word[1][1],
						approx(256500,tape_word[0][1]+tape_word[1][1]),
						(i+7)>>3,((i+7)&7)+'1',h/ZXKHZ,tape_play());
					break;
				case 0X15:
					tape_wave=frecv2();
					tape_tails=(h=frecv2())*ZXKHZ;
					if (!(tape_waves=frecv1())) tape_waves=8; // avoid accidents!
					i=tape_waves+=(frecv3()-1)<<3;
					uprintf("SAMPLE         --------- --------- %04d:---- ---%7d:%c%7d%9d\n",tape_wave,(i+7)>>3,((i+7)&7)+'1',h,tape_play());
					break;
				case 0x19:
					k=frecv4(); k+=ftell1(); // save offset...
					tape_tails=(h=frecv2())*ZXKHZ;
					tape_heads=frecv4();
					int npp=frecv1(),asp; if (!(asp=frecv1())) asp=256;
					tape_datas=frecv4();
					int npd=frecv1(),asd; if (!(asd=frecv1())) asd=256;
					tape_width=asd>2?asd>4?asd>16?8:4:2:1; i=0;
					tape_play_n=tape_play_t=0;
					if (tape_heads)
					{
						tape_init19(npp,asp),i+=tape_play19();
						uprintf("GENERAL        %5dx%04d --------- ",tape_play_n?approx(tape_play_t,tape_play_n):0,tape_play_n);
					}
					else
						uprints("GENERAL        --------- --------- ");
					tape_play_1=tape_play_n=tape_play_t=0;
					if (l=tape_datas*tape_width)
					{
						tape_init19(npd,asd),i+=tape_play();
						j=tape_play_n+tape_play_1?approx(tape_play_t,tape_play_n+tape_play_1):0;
						printf("%04d:%04d%4d%7d:%c%7d%9d\n",j,j*2,j?approx(513000,j*3):0,(l+7)>>3,((l+7)&7)+'1',h,i);
					}
					else
						printf("--------- --- -------- %7d%9d\n",h,i);
					fseek1(k); // ...and load stored offset
					break;
				case 0X20:
					if (tape_tails=(h=frecv2())*ZXKHZ)
					{
						if (csw_1st) tape_sign=0; // gotcha!
						uprintf("PAUSE          --------- --------- --------- --- --------%7d%9d\n",h,tape_play());
					}
					else
						uprints("STOP THE TAPE\n");
					break;
				case 0X21:
					uprintf("GROUP: %s\n",tape_getpascal(frecv1()));
					break;
				case 0X22:
					uprints("GROUP.\n");
					break;
				// the following blocks are horrible and no sane tool should generate them!
				case 0X23:
					i=(signed short)frecv2();
					tape_tzxseek(ordinal=tape_tzxtell()+i);
					uprintf("JUMP TO BLOCK %d\n",ordinal+1);
					break;
				case 0X24:
					i=frecv2();
					loopleft=i,looppast=ordinal,loopseek=ftell1();
					uprintf("LOOP START: %d\n",loopleft);
					break;
				case 0X25:
					if (loopleft)
						if (--loopleft)
							fseek1(loopseek),ordinal=looppast;
					uprintf("LOOP END: %d\n",loopleft);
					break;
				case 0X26:
					i=frecv2();
					if (callleft=i)
					{
						callpast=ordinal,callseek=ftell1()+2;
						i=tape_tzxtell();
						tape_tzxseek(ordinal=(i+(signed short)frecv2()));
					}
					uprints("CALL TO SEQUENCE\n");
					break;
				case 0X27:
					if (callleft)
						if (fseek1(callseek),ordinal=callpast,--callleft)
						{
							callseek+=2;
							i=tape_tzxtell();
							tape_tzxseek(ordinal=(i+(signed short)frecv2()));
						}
					uprints("RETURN FROM SEQUENCE\n");
					break;
				case 0X28:
					fskip1(frecv2());
					uprints("SELECT BLOCKS\n");
					break;
				case 0X2A:
					fskip1(frecv4());
					uprints("STOP THE TAPE ON 48K MODE\n");
					break;
				case 0X2B:
					k=frecv4()-1; tape_sign=frecv1()&1; fskip1(k);
					uprintf("SET SIGNAL LEVEL %s\n",tape_sign?"HIGH":"LOW");
					break;
				// the remaining blocks are purely informational, fortunately for us...
				case 0X31:
					frecv1(); // this byte is the time in seconds the text spends on screen (!)
				case 0X30:
					uprintf("INFOS: %s\n",tape_getpascal(frecv1()));
					break;
				case 0X32:
					fskip1(frecv2());
					uprints("ARCHIVE INFO\n");
					break;
				case 0X33:
					fskip1(frecv1()*3);
					uprints("HARDWARE TYPE\n");
					break;
				case 0X35:
					fskip1(16);
					fskip1(frecv4());
					uprints("CUSTOM INFO\n");
					break;
				case 0x4B:
					tape_kansas=frecv4()-12;
					tape_tails=(h=frecv2())*ZXKHZ;
					tape_head[0][1]=frecv2();
					tape_head[0][0]=frecv2();
					tape_heads=1;
					tape_word[0][0]=tape_word[1][0]=0;
					tape_word[0][1]=frecv2();
					tape_word[1][1]=frecv2();
					tape_word[0][2]=tape_word[1][2]=-1;
					i=frecv1();
					if (!(tape_kansasn[0]=i>>4)) tape_kansasn[0]=16;
					if (!(tape_kansasn[1]=i&15)) tape_kansasn[1]=16;
					i=frecv1();
					tape_kansasin=(i>>6)&3; tape_kansasi=(i>>5)&1;
					tape_kansason=(i>>3)&3; tape_kansaso=(i>>2)&1;
					tape_kansasio=i&1;
					i=tape_kansas;
					uprintf("KANSAS CITY   %5dx%04d %c-[%c-%c]-%c %04d:%04d%4d%7d:8%7d%9d\n",
						tape_head[0][0],tape_head[0][1],
						48+tape_kansasin,48+tape_kansasn[0],48+tape_kansasn[1],48+tape_kansason,
						tape_word[0][1],tape_word[1][1],
						approx(512800,tape_word[0][1]*tape_kansasn[0]+tape_word[1][1]*tape_kansasn[1]),
						i,h,tape_play());
					break;
				case 0X5A:
					fskip1(9);
					uprints("GLUE\n");
					break;
				default:
					fskip1(frecv4());
					uprints("UNKNOWN ID!?\n");
					break;
			}
		csw_exit(); flush1(); // farewell + flush!
		uprintz(ftell(fi)); uprintl();
		//printf(ok_bytes,(int)ftell(fi),(int)ftell(fo));
		return fclose(fi),fclose(fo),0;
	}
	if (!memcmp(buffer,pzx10b,4)&&buffer[8]==1) // PZX decoding?
	{
		if (!(fo=fopen(autosuffix(so,si,".csw"),"wb")))
			return fclose(fi),fprintf(stderr,bad_target),1;
		fseek(fi,4,SEEK_SET); fskip1(frecv4()); // rewind!
		csw_init(); // signature + setup
		hz=approxy(hz,100,95+flag_e);
		tape_pzx=1; fu=stdout; uprintl(); uprintm(); uprintl();
		while ((i=frecv4())>0&&(l=frecv4())>=0)
		{
			//++ordinal,printf("%c%03d ",topdigit(ordinal/1000),ordinal%1000);
			if (i==0X54585A50) // "PZXT"
				uprints("GLUE\n"),fskip1(l);
			else if (i==0X534C5550) // "PULS"
			{
				uprints("PULSES");
				h=12; j=0; tape_sign=0; do
				{
					k=1; if (l-=2,(i=frecv2())>0X8000) k=i-0X8000,i=frecv2(),l-=2;
					if (i&0X8000) i=((i-0X8000)<<16)+frecv2(),l-=2;
					if (k>1)
						{ h-=2; if (h>0||!(h|l)) printf("%5dx%04d",k,i); else if (!h) printf("%5dx....",k); else if (h==-1) printf(" ...."); }
					else //if (i)
						{ --h; if (h>0||!(h|l)) printf(" %04d",i); else if (!h) printf(" ...."); }
					if (!i) tape_sign+=k; else do { csw_send(i,tape_sign); j+=i; ++tape_sign; } while (--k);
				}
				while (l>0);
				while (h>0) printf("     "),--h;
				printf("%7d\n",approx(j,ZXKHZ)); un_clock(j);
			}
			else if (i==0X41544144) // "DATA"
			{
				i=frecv4(); tape_sign=i>>31; i&=0X7FFFFFFF;
				uprints("DATA  ");
				int zzz=frecv2(),g=0; l=frecv1(); h=frecv1();
				j=0; for (k=0;k<l;++k) j+=tape_head[k][0]=frecv2(); printf(" BIT0 =%3dx%04d",l,l?approx(j,l):0); g+=j;
				j=0; for (k=0;k<h;++k) j+=tape_head[k][1]=frecv2(); printf(" BIT1 =%3dx%04d",h,h?approx(j,h):0); g+=j;
				j=0; printf("%4d%%%5d%8d:%c",approx(513000,g),zzz,(i+7)/8,((i+7)&7)+'1');
				unsigned char m8=0,b8=0; do
				{
					if (!(m8>>=1))
						m8=128,b8=frecv1();
					if (m8&b8)
						for (k=0;k<h;++k) csw_send(tape_head[k][1],tape_sign),j+=tape_head[k][1],++tape_sign;
					else
						for (k=0;k<l;++k) csw_send(tape_head[k][0],tape_sign),j+=tape_head[k][0],++tape_sign;
				}
				while (--i>0);
				if (zzz) /*printf(" +%d",zzz),*/csw_send(zzz,tape_sign),j+=zzz,++tape_sign;
				printf("%17d\n",approx(j,ZXKHZ)); un_clock(j);
				//printf(", %d ms\n",approx(j,ZXKHZ));
			}
			else if (i==0X53554150&&l==4) // "PAUS"
			{
				i=frecv4(); /*tape_sign=(i>>31)&1;*/ i&=0X7FFFFFFF;
				if (i>ZXKHZ)
					csw_send(ZXKHZ,tape_sign),csw_send(i-ZXKHZ,0),tape_sign=1;
				else
					csw_send(i,tape_sign),++tape_sign;
				un_clock(i);
				uprintf("PAUSE  %09d%57d\n",i,approx(i,ZXKHZ));
			}
			else if (i==0X54584554&&l<sizeof(buffer)) // "TEXT"
				uprintf("INFOS: %s\n",tape_getpascal(l));
			else if (i==0X53575242&&l<sizeof(buffer)) // "BRWS"
				uprintf("BLOCK: %s\n",tape_getpascal(l));
			else if (i==0X42525753/*&&!l*/) // "SWRB" (end of BRWS)
				uprintf("BLOCK. %s\n",tape_getpascal(l)); // `l` should be always zero, but just in case...
			else if (i==0X534C5550&&l==4) // "STOP"
				uprintf("STOP   (%07d)\n",i=frecv4());
			else // unknown block, skip bytes
				printf("????   [%07d] %02X%02X%02X%02X!\n",i&255,(i>>8)&255,(i>>16)&255,(i>>24)&255,l),fskip1(l);
		}
		csw_exit(); flush1(); // farewell + flush!
		uprintz(ftell(fi)); uprintl();
		//printf(ok_bytes,(int)ftell(fi),(int)ftell(fo));
		return fclose(fi),fclose(fo),0;
	}
	if (buffer[0]==0X13&&!(buffer[1]|buffer[2]|buffer[3])) // TAP decoding?
	{
		if (!(fo=fopen(autosuffix(so,si,".csw"),"wb")))
			return fclose(fi),fprintf(stderr,bad_target),1;
		fseek(fi,0,SEEK_SET); // rewind!
		tape_sign=1; csw_init(); // signature + setup
		//hz=approxy(hz,100,95+flag_e); // not here, TAP files lack timings!
		tape_head[0][1]=2168;
		tape_head[1][0]=tape_head[2][0]=1;
		tape_head[1][1]=667;
		tape_head[2][1]=735;
		tape_word[0][0]=tape_word[1][0]=0;
		tape_word[0][1]=tape_word[0][2]=855;
		tape_word[1][1]=tape_word[1][2]=1710;
		tape_word[0][3]=tape_word[1][3]=-1;
		tape_width=1;
		fu=stdout; uprintl(); uprintm(); uprintl();
		while ((i=frecv2())>0)
		{
			//++ordinal,printf("%c%03d %04X ",topdigit(ordinal/1000),ordinal%1000,i);
			tape_head[0][0]=frecv1()<128?8062:3222; fskip1(-1); // caution: REAL pilot lengths are EVEN!!
			tape_heads=3;
			tape_datas=i<<3;
			tape_tails=ZX1HZ;
			uprintf("NORMAL%13dx2168 0667,0735 0855:1710 100%7d:8   1000%9d\n",tape_head[0][0],i,tape_play());
		}
		csw_exit(); flush1(); // farewell + flush!
		uprintz(ftell(fi)); uprintl();
		//printf(ok_bytes,(int)ftell(fi),(int)ftell(fo));
		return fclose(fi),fclose(fo),0;
	}
	return fprintf(stderr,"error: unknown source type!\n"),1;
}
