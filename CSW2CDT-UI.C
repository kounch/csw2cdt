  //@@@@ //@@@@@//@@ //@@//@@@@  //@@@@//@@@@@ //@@@@@@ -----------------------
 //@@//@@/@@ //@@/@@ //@@/@@//@@//@@//@@//@@/@@//@/@@/@  a companion of CSW2CDT
//@@    //@@    //@@ //@@   //@@/@@     //@@//@@ //@@    that provides a simple
//@@     //@@@@@//@@/@/@@//@@@@//@@     //@@//@@ //@@    Windows user interface
//@@         //@@/@@@@@@@/@@   //@@     //@@//@@ //@@    hiding the commandline
 //@@//@@/@@ //@@/@@@/@@@/@@//@@//@@//@@//@@/@@  //@@    bits of CSW0 + CSW2CDT
  //@@@@ //@@@@@//@@ //@@/@@@@@@ //@@@@//@@@@@  //@@@@ ------------------------

#define MY_VERSION "20240224"
#define MY_LICENSE "Copyright (C) 2020-2023 Cesar Nicolas-Gonzalez"

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
//#include <direct.h> // getcwd

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <commdlg.h>

char *fgets0(char *t,int n,FILE *s) // perform fgets() and remove all spaces and blanks
{
	if (!fgets(t,n,s)) return NULL;
	char *tt=t; while (*tt) ++tt;
	while (--tt>=t&&*tt<=' ') *tt=0;
	return t;
}

HINSTANCE hinstance; char csw0_exe[1<<9],csw0_opt[1<<9],csw2cdt_exe[1<<9],csw2cdt_opt[1<<9],csw2cdt_txt[1<<9],csw2cdt_ui_ini[1<<9],csw2cdt_ui_lst[1<<9];
OPENFILENAME ofn; char buffer[1<<9]="",params[1<<9]="",source[1<<9]="",target[1<<9]="",readme[1<<9]="";
char *FileBox(HWND hwnd,int reading,char *pattern,char *caption) // 0 = CREATE, MUST NOT EXIST, >0 = OPEN, MUST EXIST, <0 = EITHER
{
	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hwnd;
	ofn.Flags=reading>0?OFN_FILEMUSTEXIST:reading?0:OFN_OVERWRITEPROMPT;
	ofn.lpstrFile=buffer;
	ofn.nMaxFile=sizeof(buffer)-1;
	char *t=params; t+=sprintf(t,"%s",pattern)+1; t+=sprintf(t,"%s",pattern)+1; *t=0;
	ofn.lpstrFilter=params;
	ofn.lpstrDefExt=&pattern[2]; // reduce "*.XXX" to "XXX"
	ofn.lpstrTitle=caption;
	ofn.nFilterIndex=1;
	return (reading?GetOpenFileName(&ofn):GetSaveFileName(&ofn))?buffer:NULL;
}
char *NewExtension(char *x) // replace the extension of the filename stored at `buffer`
{
	while (*x=='*') ++x;
	char *t=strrchr(buffer,'.'),*p=strrchr(buffer,'\\'),*pp=strrchr(buffer,':');
	if (!t||(p&&p>t)||(pp&&pp>t)) strcat(buffer,x); else strcpy(t,x);
	return buffer;
}
int WarningBox(HWND hwnd,char *s) // show the warning `s`
	{ return MessageBox(hwnd,s,"Warning!",MB_ICONWARNING); }
int OkTarget(HWND hwnd) // show the size of file `target`
{
	int i=-1; FILE *f=fopen(target,"rb"); if (f) { fseek(f,0,SEEK_END); i=ftell(f); fclose(f); }
	sprintf(buffer,"%s: %d bytes.",target,i); return MessageBox(hwnd,buffer,"Success",MB_ICONINFORMATION);
}
int RunExe(HWND hwnd,char *s) // run a program and show a warning if required
{
	sprintf(buffer,"%s %s \"%s\" \"%s\"",s,params,source,target);
	return system(buffer)?WarningBox(hwnd,buffer),0:1;
}
int RunStdout(HWND hwnd,char *s) // run a program and send its output to `target`
{
	sprintf(buffer,"%s %s \"%s\" nul > \"%s\"",s,params,source,target);
	return system(buffer)?WarningBox(hwnd,buffer),0:1;
}

char logtext[1<<16],*logname,*loghead;
int LogProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowText(hwnd,loghead?loghead:logname);
			SetDlgItemText(hwnd,4001,logtext);
			BringWindowToTop(hwnd); return 1;
		case WM_USER:
			SendDlgItemMessage(hwnd,4001,EM_SETSEL,0,0); return 1;
		case WM_CLOSE:
			EndDialog(hwnd,0); return 1;
	}
	return 0;
}
void LogBox(HWND hwnd,char *s,char *t) // show the contents of the text file `s`; optional string `t` is the title
{
	FILE *f=fopen(logname=s,"rb"); if (f)
	{
		ZeroMemory(logtext,sizeof(logtext));
		int i=fread(logtext,1,sizeof(logtext)-4-1,f); fclose(f);
		if (i==sizeof(logtext)-4-1) strcat(logtext,"..."); // ellipsis!
		if (!memcmp(logtext,"\357\273\277",3)) memcpy(logtext,logtext+3,sizeof(logtext)-3); // strip UTF-8 prefix!
		ShowWindow(hwnd,0); loghead=t;
		DialogBox(hinstance,MAKEINTRESOURCE(1002),NULL,(DLGPROC)LogProc);
		ShowWindow(hwnd,1); BringWindowToTop(hwnd);
	}
	else
		WarningBox(hwnd,s);
}
char encoding[64][64]; int encodings=0;
void SaveConfig(HWND hwnd) // save the current config, if required
{
	GetDlgItemText(hwnd,4004,source,sizeof(source)); // default CSW0 flags
	GetDlgItemText(hwnd,4015,target,sizeof(target)); // default CSW2CDT flags
	if (strcmp(csw0_opt,source)||strcmp(csw2cdt_opt,target)) // any differences?
	{
		FILE *f=fopen(csw2cdt_ui_ini,"w"); if (f)
		{
			fprintf(f,"%s\n",source);
			fprintf(f,"%s\n",target);
			fclose(f);
		}
	}
}

int DlgProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowText(hwnd,"CSW2CDT-UI " MY_VERSION);
			GetCurrentDirectory(sizeof(buffer)-1,buffer);//getcwd(buffer,sizeof(buffer)-1);
			sprintf(csw0_exe,"%s\\%s",buffer,"csw0.exe");
			sprintf(csw2cdt_exe,"%s\\%s",buffer,"csw2cdt.exe");
			sprintf(csw2cdt_txt,"%s\\%s",buffer,"csw2cdt.txt");
			sprintf(csw2cdt_ui_ini,"%s\\%s",buffer,"csw2cdt-ui.ini");
			sprintf(csw2cdt_ui_lst,"%s\\%s",buffer,"csw2cdt-ui.lst");
			*encoding[encodings]=0;
			SendDlgItemMessage(hwnd,4012,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)"--default--"),++encodings;
			FILE *f; if (f=fopen(csw2cdt_ui_lst,"r"))
			{
				while (fgets0(buffer,sizeof(buffer)-1,f))
				{
					if (!fgets0(encoding[encodings],sizeof(encoding[0])-1,f)) break;
					SendDlgItemMessage(hwnd,4012,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buffer),++encodings;
				}
				SendDlgItemMessage(hwnd,4012,LB_SELITEMRANGE,0,0);
				fclose(f);
			}
			if (f=fopen(csw2cdt_ui_ini,"r"))
			{
				if (fgets0(csw0_opt,sizeof(csw0_opt)-1,f))
					SetDlgItemText(hwnd,4004,csw0_opt); // default CSW0 flags
				if (fgets0(csw2cdt_opt,sizeof(csw2cdt_opt)-1,f))
					SetDlgItemText(hwnd,4015,csw2cdt_opt); // default CSW2CDT flags
				fclose(f);
			}
			BringWindowToTop(hwnd); return 1;
		case WM_COMMAND:
			switch (LOWORD(wparam))
			{
					case IDCANCEL: // i.e. ESCAPE key
						SaveConfig(hwnd); EndDialog(hwnd,0); return 1;
					case 4001: // SAMPLE file...
						GetDlgItemText(hwnd,4002,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,1,"*.wav","SAMPLE file..."))
						{
							SetDlgItemText(hwnd,4002,buffer);
							SetDlgItemText(hwnd,4008,NewExtension("*.csw"));
						}
						break;
					case 4005: // Generate source!
						if (GetDlgItemText(hwnd,4002,source,sizeof(source)-1)&&
							GetDlgItemText(hwnd,4008,target,sizeof(target)-1))
						{
							GetDlgItemText(hwnd,4004,params,sizeof(params)-1);
							if (RunExe(hwnd,csw0_exe))
							{
								strcpy(buffer,target);
								SetDlgItemText(hwnd,4010,NewExtension("*.cdt"));
								OkTarget(hwnd);
							}
						}
						else
							WarningBox(hwnd,"Click SAMPLE file button!");
						break;
					case 4006: // Playback...
						GetDlgItemText(hwnd,4008,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,1,"*.csw","Playback..."))
						{
							strcpy(source,buffer);
							NewExtension("*.wav");
							if (FileBox(hwnd,0,"*.wav","Playback to..."))
							{
								strcpy(target,buffer);
								GetDlgItemText(hwnd,4004,params,sizeof(params)-1);
								if (RunExe(hwnd,csw0_exe))
									OkTarget(hwnd);
							}
						}
						break;
					case 4007: // SOURCE file...
						GetDlgItemText(hwnd,4008,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,-1,"*.csw","SOURCE file..."))
						{
							SetDlgItemText(hwnd,4008,buffer);
							SetDlgItemText(hwnd,4010,NewExtension("*.cdt"));
						}
						break;
					case 4009: // TARGET file...
						GetDlgItemText(hwnd,4010,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,0,"*.cdt;*.cdp;*.tzx;*.pzx;*.tsx","TARGET file..."))
						{
							SetDlgItemText(hwnd,4010,buffer);
						}
						break;
					case 4012: // Known methods
						if (HIWORD(wparam)==1) // select item
						{
							int i=SendDlgItemMessage(hwnd,4012,LB_GETCURSEL,0,0);
							if (i>=0&&i<encodings)
								SetDlgItemText(hwnd,4013,encoding[i]);
						}
						break;
					case 4016: // ENCODE!
						if (GetDlgItemText(hwnd,4008,source,sizeof(source)-1)&&
							GetDlgItemText(hwnd,4010,target,sizeof(target)-1))
						{
							GetDlgItemText(hwnd,4013,params,sizeof(params)-1);
							GetDlgItemText(hwnd,4015,readme,sizeof(readme)-1);
							sprintf(params,"%s %s",params,readme);
							if (RunExe(hwnd,csw2cdt_exe))
							{
								GetDlgItemText(hwnd,4010,buffer,sizeof(buffer)-1);
								strcpy(readme,NewExtension("*.log"));
								LogBox(hwnd,readme,NULL);
							}
							else
								*readme=0;
						}
						else
							WarningBox(hwnd,"Click SOURCE file button!");
						break;
					case 4017: // View log...
						if (*readme)
							LogBox(hwnd,readme,NULL);
						else
							WarningBox(hwnd,"Click TARGET file button!");
						break;
					case 4018: // Examine...
						GetDlgItemText(hwnd,4010,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,1,"*.cdt;*.cdp;*.tzx;*.pzx;*.tsx;*.tap","Examine..."))
						{
							strcpy(source,buffer);
							NewExtension("*.lst");
							strcpy(target,buffer);
							GetDlgItemText(hwnd,4015,params,sizeof(params)-1);
							if (RunStdout(hwnd,csw2cdt_exe))
							{
								strcpy(readme,target);
								LogBox(hwnd,readme,NULL);
							}
							else
								*readme=0;
						}
						break;
					case 4019: // Decode...
						GetDlgItemText(hwnd,4010,buffer,sizeof(buffer)-1);
						if (FileBox(hwnd,1,"*.cdt;*.cdp;*.tzx;*.pzx;*.tsx;*.tap","Decode..."))
						{
							strcpy(source,buffer);
							NewExtension("*.csw");
							if (FileBox(hwnd,0,"*.csw","Decode to..."))
							{
								strcpy(target,buffer);
								GetDlgItemText(hwnd,4015,params,sizeof(params)-1);
								if (RunExe(hwnd,csw2cdt_exe))
								{
									strcpy(source,target);
									strcpy(buffer,target);
									NewExtension("*.wav");
									if (FileBox(hwnd,0,"*.wav","Playback to..."))
									{
										strcpy(target,buffer);
										GetDlgItemText(hwnd,4004,params,sizeof(params)-1);
										if (RunExe(hwnd,csw0_exe))
											OkTarget(hwnd); // the WAV file the user requested
									}
									else
										OkTarget(hwnd); // user rejected the WAV, stick to the CSW
								}
							}
						}
						break;
					case 4020: // Help...
						LogBox(hwnd,csw2cdt_txt,MY_LICENSE);
						break;
			}
			return 1;
		case WM_CLOSE:
			SaveConfig(hwnd); EndDialog(hwnd,0); return 1;
	}
	return 0;
}

// in theory, GCC is smart enough to understand when to use WinMain() and when to use main()...
int APIENTRY WinMain(HINSTANCE hinst,HINSTANCE hprev,LPSTR lpszline,int nshow) { return DialogBox(hinstance=hinst,MAKEINTRESOURCE(1001),NULL,(DLGPROC)DlgProc)<0; }
//int main(int argc,char *argv[]) { return DialogBox(hinstance=GetModuleHandle(0),MAKEINTRESOURCE(1001),NULL,(DLGPROC)DlgProc)<0; }
