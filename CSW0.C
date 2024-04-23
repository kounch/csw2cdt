  //@@@@ //@@@@@//@@ //@@//@@@@  //@@@@//@@@@@ //@@@@@@ -----------------------
 //@@//@@/@@ //@@/@@ //@@/@@//@@//@@//@@//@@/@@//@/@@/@  a companion of CSW2CDT
//@@    //@@    //@@ //@@   //@@/@@     //@@//@@ //@@    turning WAV audio into
//@@     //@@@@@//@@/@/@@//@@@@//@@     //@@//@@ //@@    CSW v1 files, coded by
//@@         //@@/@@@@@@@/@@   //@@     //@@//@@ //@@    Cesar Nicolas-Gonzalez
 //@@//@@/@@ //@@/@@@/@@@/@@//@@//@@//@@//@@/@@  //@@    since 2020/05/31-09:50
  //@@@@ //@@@@@//@@ //@@/@@@@@@ //@@@@//@@@@@  //@@@@ ------------------------

#define MY_VERSION "20240224"
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

#define length(x) (sizeof(x)/sizeof*(x))
unsigned char buffer[512]; FILE *fi,*fo;
char *autosuffix(char *t,char *s,char *x) // return a valid path, with a new suffix if required
{
	if (t) return t; else if (!s) return NULL; else if ((char*)buffer!=s) strcpy(buffer,s);
	if ((t=strrchr(buffer,'.'))&&(!(s=strrchr(buffer,
	#ifdef _WIN32
	'\\'
	#else
	'/'
	#endif
	))||t>s)) return strcpy(t,x),buffer; // override old suffix
	return strcat(buffer,x); // append new suffix
}

// I/O file operations, buffering and Intel lil-endian integer logic -------- //

#define fread1(t,i) fread(t,1,i,fi)
#define fwrite1(t,i) fwrite(t,1,i,fo)
#define fput1(n) fputc(n,fo)
int fput4(int i) { fput1(i); fput1(i>>8); fput1(i>>16); return fput1(i>>24); } // non-buffered!

unsigned char tsrc[1<<12],ttgt[1<<12]; int isrc=0,ilen=0,itgt=0; // I/O file buffers
#define flush1() fwrite1(ttgt,itgt)
int frecv1(void) { while (isrc>=ilen) if (isrc-=ilen,!(ilen=fread1(tsrc,sizeof(tsrc)))) return -1; return tsrc[isrc++]; }
int frecv4(void) { int i=frecv1(); i|=frecv1()<<8; i|=frecv1()<<16; return i|(frecv1()<<24); }
int fsend1(int i) { ttgt[itgt++]=i; if (itgt>=length(ttgt)) { if (!flush1()) return -1; itgt=0; } return i; }
int fsend4(int i) { fsend1(i); fsend1(i>>8); fsend1(i>>16); return fsend1(i>>24); }

// the main procedure proper ------------------------------------------------ //

const char csw24b[]="Compressed Square Wave\032\001",ok_bytes[]="%d:%d bytes.\n",bad_target[]="error: cannot create target!\n";
int plusatoi(char *s) { return atoi(*s=='+'?s+1:s); } // handle "+1234" as "1234"
int readmmss(char *s) { int h=0,l=0,c; while ((c=*s++)>='0'&&c<='9') l=l*10+c-'0'; if (c==':') { h=l*60,l=0; while ((c=*s++)>='0'&&c<='9') l=l*10+c-'0'; } return c?-1:h+l; }

int main(int argc,char *argv[])
{
	int h,i,j,k,l; char *si=NULL,*so=NULL;
	int flag_m=0,flag_e=0,flag_lr=0,flag_t=0,flag_n=0,filter_l=0,filter_h=0,hz,mmss_init=0,mmss_exit=-1;
	for (i=1;i<argc;++i)
		if (argv[i][0]=='-')
			if (argv[i][1])
				#define FETCH_INT(t,l,h) ((argv[i][++j]||(j=0,++i<argc))?t=plusatoi(&argv[i][j]),j=-1,t>=l&&t<h:0)
				for (j=0;++j>0&&i<argc&&argv[i][j];)
					switch (argv[i][j])
					{
						case 'f':
							if (!FETCH_INT(filter_l,0,96000))
								i=argc; // help!
							else if (++i>=argc||!FETCH_INT(filter_h,0,96000))
								i=argc; // help!
							if (filter_l&&filter_h&&filter_l>filter_h)
								k=filter_l,filter_l=filter_h,filter_h=k; // swap if L > H
							break;
						case 'm':
							if (!FETCH_INT(flag_m,-127,+127))
								i=argc; // help!
							break;
						case 'e':
							if (!FETCH_INT(flag_e,0,127))
								i=argc; // help!
							break;
						case 'l':
							flag_lr=-1;
							break;
						case 'r':
							flag_lr=+1;
							break;
						case 'n':
							flag_n=1;
							break;
						case 't':
							if (!FETCH_INT(flag_t,0,100))
								i=argc; // help!
							break;
						case '-':
							if (i+1<argc&&(j=readmmss(&argv[i][j+1]))>=0&&(mmss_init=j,(j=readmmss(argv[++i]))>=0))
								mmss_exit=j,j=-1;
							else
								i=argc; // help!
							break;
						case 'h':
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
		return printf(
			"CSW0 " MY_VERSION " " MY_LICENSE "\n"
			"\n"
			"encoding from WAV to CSW:\n"
			"\tcsw0 [option..] source.wav [target.csw]\n"
			"\t-f L H\tenable pass-band filter, in Hz\n"
			"\t-m N\tset middle point between -127 and 127\n"
			"\t-e N\tset error margin between 0 and 127\n"
			"\t-l\tleft channel only\n"
			"\t-r\tright channel only\n"
			"\t-n\tnegative polarity\n"
			"\t--M1:S1 M2:S2\tclip from M1 min S1 sec to M2 min M2 sec\n"
			"decoding from CSW to WAV:\n"
			"\tcsw0 [option..] source.csw [target.wav]\n"
			"\t-t N\tset signal shape between 0 and 100 (square-triangle)\n"
			"\t-n\tnegative polarity\n"
			"\n" GPL_3_INFO "\n"
		),1;
	if (!(fi=fopen(si,"rb")))
		return fprintf(stderr,"error: cannot open source!\n"),1;
	if (*buffer=0,fread1(buffer,12),!memcmp(buffer,"RIFF",4)&&!memcmp(&buffer[8],"WAVE",4)) // encoding?
	{
		while (j=frecv4(),k=frecv4(),k>=0&&j!=0x20746D66) // "fmt "
			isrc+=(k+1)&~1; // RIFF even-padding
		if (k<16)
			return fclose(fi),fprintf(stderr,"error: source lacks header!\n"),1;
		int stereo=(frecv4()>>17)&1; // channels-1
		hz=frecv4();
		frecv4(); // actual byte rate...
		int datasize,byteskip=((frecv4()>>19)&3)-1,bytesize=byteskip+1; // bitdepth 8, 16 or 24 (!) -> byteskip = 0,1,2 (unused bytes per sample); bytesize = 1,2,3 (sample size in bytes)
		isrc+=((k+1)&~1)-16; // RIFF even-padding
		while (j=frecv4(),datasize=frecv4(),datasize>=0&&j!=0x61746164) // "data"
			isrc+=datasize;
		if (datasize<0)
			return fclose(fi),fprintf(stderr,"error: source lacks body!\n"),1;
		if (!(fo=fopen(autosuffix(so,si,".csw"),"wb")))
			return fclose(fi),fprintf(stderr,bad_target),1;
		fwrite1(csw24b,24);
		fsend4(hz*256+1+256*256*256); flag_m+=128;
		if (stereo) if (flag_lr<0) isrc-=bytesize; // avoid right channel
		if (mmss_init>0) { mmss_init*=(stereo+1)*bytesize*hz; isrc+=mmss_init,datasize-=mmss_init; } // source start...
		if (mmss_exit>0) { mmss_exit*=(stereo+1)*bytesize*hz; if (datasize>mmss_exit) datasize=mmss_exit; } // ...and end
		#define GETSAMPLE() (stereo? \
			byteskip?(datasize-=2*bytesize,isrc+=byteskip,i=frecv1()^128,isrc+=byteskip,i=(((frecv1()^128)+i)>>1)): \
				(datasize-=2,i=((frecv1()+frecv1())>>1)): \
			byteskip?(datasize-=bytesize,isrc+=byteskip,i=frecv1()^128): \
				(--datasize,i=frecv1()))
		#define SENDCHUNK do{ if (j) { if (j>255) fsend1(0),fsend4(j); else fsend1(j); j=0; } }while(0)
		GETSAMPLE(); fsend4(flag_n^(k=i>flag_m)); // store 1st signal!
		j=0; l=flag_m-flag_e; h=flag_m+flag_e;
		const float TWOPI=6.28318530717958647692528676655901;
		if (hz*filter_l>0)
		{
			if (hz*filter_h>0) // band-pass filter: H and L nonzero
			{
				int g=0; float h1=0,h2=0,high_alpha=1.0/(TWOPI*filter_l/hz+1.0),
					loww_alpha=(TWOPI*filter_h/hz)/(TWOPI*filter_h/hz+1.0);
				do
				{
					i-=flag_m; h2=high_alpha*(h2+i-g); g=i; // high-pass 1st
					h1=h1+loww_alpha*(h2-h1); // loww-pass 2nd
					if (k?(h1<=-flag_e):(h1>flag_e))
						{ SENDCHUNK; k=!k; }
				}
				while (++j,GETSAMPLE(),datasize>0&&ilen>0);
			}
			else // high-pass filter: H zero, L nonzero
			{
				int g=0; float hh=0,high_alpha=1.0/(TWOPI*filter_l/hz+1.0);
				do
				{
					i-=flag_m; hh=high_alpha*(hh+i-g); g=i; // high-pass
					if (k?(hh<=-flag_e):(hh>flag_e))
						{ SENDCHUNK; k=!k; }
				}
				while (++j,GETSAMPLE(),datasize>0&&ilen>0);
			}
		}
		else if (hz*filter_h>0) // low-pass filter: H nonzero, L zero
		{
			float hh=0,loww_alpha=(TWOPI*filter_h/hz)/(TWOPI*filter_h/hz+1.0);
			do
			{
				i-=flag_m; hh=hh+loww_alpha*(i-hh); // loww-pass
				if (k?(hh<=-flag_e):(hh>flag_e))
					{ SENDCHUNK; k=!k; }
			}
			while (++j,GETSAMPLE(),datasize>0&&ilen>0);
		}
		else // no filter
		{
			do
			{
				if (k?(i<=l):(i>h))
					{ SENDCHUNK; k=!k; }
			}
			while (++j,GETSAMPLE(),datasize>0&&ilen>0);
		}
		SENDCHUNK; flush1(); // flush!
		printf(ok_bytes,(int)ftell(fi),(int)ftell(fo));
		return fclose(fi),fclose(fo),0;
	}
	if (fread1(&buffer[12],(32-12))==(32-12)&&!memcmp(buffer,csw24b,24)) // decoding?
	{
		hz=buffer[25]+buffer[26]*256+(buffer[27]-1)*65536; k=((flag_n^buffer[28])&1)?32:224;
		if (!(fo=fopen(autosuffix(so,si,".wav"),"wb"))) // this call destroys `buffer`!
			return fclose(fi),fprintf(stderr,bad_target),1;
		fwrite1("RIFFFFFFWAVEfmt \020\000\000\000\001\000\001\000~~~~~~~~\001\000\010\000dataaaaa",44);
		int g=2+(hz-1)/1000; // i.e. almost 2 ms
		while ((i=frecv1())>=0)
		{
			k^=192; if (!i) i=frecv4();
			// long edges become flat at the end!
			l=0; if (i>g) l=i-g,i=g;
			// rising slope
			int g1=i*flag_t/200,g2=(i*flag_t+100)/200; i-=g1+g2;
			j=k-128; h=g1>>1; for (int n=1;n<=g1;++n) fsend1(128+(j*n+h)/g1);
			// ceiling, i.e. 128+96 / 128-96
			j=k; while (i>0) fsend1(j),--i;
			// falling slope
			j=k-128; h=g2>>1; for (int n=g2;n>=1;--n) fsend1(128+(j*n+h)/g2);
			// floor
			while (l) fsend1(128),--l;
		}
		if (itgt&1) fsend1(0); flush1(); // RIFF even-padding + flush!
		printf(ok_bytes,(int)ftell(fi),i=(int)ftell(fo));
		fseek(fo, 4,SEEK_SET); fput4(i-8); // file size-8 = chunk size+36
		fseek(fo,24,SEEK_SET); fput4(hz); fput4(hz); // clock + bandwidth
		fseek(fo,40,SEEK_SET); fput4(i-8-36); // chunk size = filesize-44
		return fclose(fi),fclose(fo),0;
	}
	return fprintf(stderr,"error: unknown source type!\n"),1;
}
