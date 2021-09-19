// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <kern/console.h>

uint16_t fg_color=7,bg_color=0,ansi_fmt_ptr=0;
const uint8_t ansi2cga[16]={0,4,2,6,1,5,3,7,8,12,10,14,9,13,11,15};
ansi_state state=normal;
char ansi_fmt[100];
inline bool ischar(int ch){return (ch>=0x41&&ch<=0x5A)||(ch>=0x61&&ch<=0x7A);}
inline bool isdigit(int ch){return (ch>=0x30&&ch<=0x39);}
int parse_ansi(char* start, char* end) {
	int res = 0;
	for (start;start<end;++start) {
		if (isdigit(*start)) {
			res *= 10;
			res += *start - 0x30;
		}
	}
	return res;
}
void apply_color(int ansi_color) {
	if (ansi_color>=30 && ansi_color<40) fg_color = ansi2cga[ansi_color-30];
	else if (ansi_color>=40 && ansi_color<50) bg_color = ansi2cga[ansi_color-40];
}
void ansi_control(char *format_string, char cmd){
	char *back = format_string;
	if(cmd=='m'){
		for (;*back;++back){
			if (*back==';'){
				apply_color(parse_ansi(format_string,back));
				format_string = back+1;
			}
			if (format_string<back){
				apply_color(parse_ansi(format_string,back));
			}
		}
	}
	return;
}

static void
putch(int ch, int *cnt)
{
	// cputchar(ch);
	*cnt++;
	switch(state){
		case normal:
			if(ch=='\033') state=start;
			else cputchar(ch);
			break;
		case start:
			if(ch=='[') state=escaping;
			else{
				cputchar('\033');cputchar(ch);
			}
			// break;
		case escaping:
			if(ischar(ch)){
				ansi_fmt[ansi_fmt_ptr]=0;
				ansi_control(ansi_fmt,ch);
				ansi_fmt_ptr = 0;
				state=normal;
			} else if (ansi_fmt_ptr==99)
				state=overflow;
			else ansi_fmt[ansi_fmt_ptr++]=ch;
			break;
		case overflow:
			if(ischar(ch)) state=normal;
		break;
	}
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

