/*
 *  linux/kernel/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{   //将一个字符串转换成using long 型数据。返回转换后的数据，cp指向分析的字符串末尾的位置，base为要用的基数，base为0表示通过cp来自动判断基数，函数自动可识别的基数：
 ‘0x’表示16进制，‘0’表示8进制，其他都认定为10进制。函数可转换成数字的有效字符为
	unsigned long result = 0,value;

	if (!base) {      //base为0表示默认10进制
		base = 10;
		if (*cp == '0') {      //cp为0表示8进制
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {    //cp为0x表示16进制
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp) ? toupper(*cp) : *cp)-'A'+10) < base) {        
            //isxdigit检查cp指向的数字是否为16进制数字，只要c为（0123456789abcdefABCDEF中的一个则返回非0值，
            //并且如果cp指向的是字符‘0’～‘9’的话将其转为数字0～9，
            //否则islower检查cp指向的字符是否为小写字符，当字符为小写英文字母(a-z)时返回非零值，否则返回零
            //topper将小写字母对应的大写字母返回   ？？？？
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

/* we use this so that we can do without the ctype library */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)           //将“123”转换成123，并跳过字符串
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */       //用零填充
#define SIGN	2		/* unsigned/signed long */   
#define PLUS	4		/* show plus */        //显示加号
#define SPACE	8		/* space if plus */       //加上空格
#define LEFT	16		/* left justified */      //左对齐
#define SPECIAL	32		/* 0x */                   //十六进制
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */   //用小写字母替代大写字母

#define do_div(n,base) ({ \                //do_div得到的结果是余数
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })  

static char * number(char * str, int num, int base, int size, int precision
	,int type)
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base<2 || base>36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;
	if (type&SIGN && num<0) {          //如果是负数标志位记为-，负数取绝对值
		sign='-';                                             
		num = -num;
	} else                                 //否则如果type要求强制正数填+号，否则填空格或补0
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);
	if (sign) size--;
	
    if (type&SPECIAL)                   
		if (base==16) size -= 2;         //如果是16进制数，输出字段的宽度减2
		else if (base==8) size--;       //如果是8进制数，输出字段的宽度减1
	i=0;
	
    if (num==0)
		tmp[i++]='0';
	else while (num!=0)                 
		tmp[i++]=digits[do_div(num,base)];       //tmp记录余数
	                                                 
    if (i>precision) precision=i;                
	size -= precision;
	
    if (!(type&(ZEROPAD+LEFT)))                  
		while(size-->0)
			*str++ = ' ';
	
    if (sign)
		*str++ = sign;
	
    if (type&SPECIAL)
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	
    if (!(type&LEFT))
		while(size-->0)          
			*str++ = c;
	while(i<precision--)
		*str++ = '0';
	while(i-->0)
		*str++ = tmp[i];
	while(size-->0)        
		*str++ = ' ';
	return str;
}

int vsprintf(char *buf, const char *fmt, va_list args)   //使用参数列表发送格式化输出到字符串
{
    //fmt这是字符串，包含了要被写入到字符串buf的文本，它可以被随后的附加参数中指定的值替换，并按需求进行格式化
    //fmt标签属性是%[flags][width][.precision][length]specifler
	int len;   
	int i;
	char * str;
	char *s;
	int *ip;

	int flags;		/* flags to number() */    //标志

	int field_width;	/* width of output field */    //输出字段的宽度
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */    //精度
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */     //限定符

	for (str=buf ; *fmt ; ++fmt) {    //
		if (*fmt != '%') {
			*str++ = *fmt;  //如果是字符直接输入到str字符串中
			continue;
		}
			
		/* process flags */
		flags = 0;
		repeat:       //无限循环语句
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;    //在给定的字段宽度内左对齐，默认是右对齐
				case '+': flags |= PLUS; goto repeat;    //强制在结果之前显示加号或减号，即整数前面会显示+号。默认情况下只有负数前面会显示一个-号
				case ' ': flags |= SPACE; goto repeat;     //如果没有写入任何符号，则在该值前面插入一个空格
				case '#': flags |= SPECIAL; goto repeat;     //如：与o、x、X 说明符一起使用时，非零值前面会分别显示0,0x,0X
				case '0': flags |= ZEROPAD; goto repeat;   
                                                   //在指定填充padding的数字左边放置零（0），而不是空格（参见width子说明符）
			}
		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);    
                    //要输出的字符的最小数目，如果输出的值短于该数，结果会用空格填充。如果输出的值长于该数，结果不会被截断
		else if (*fmt == '*') {          //宽度在fmt字符串中未指定，但是会作为附加整数值参数放置于要被格式化的参数之前
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {       //判断精度
			++fmt;	
			if (is_digit(*fmt))        //如：对于整数说明符（d、i、o、u、x、X）precision指定了要写入的数字的最小位数
				precision = skip_atoi(&fmt);     
			else if (*fmt == '*') {           //精度在fmt字符串中未指定，但是会作为附加整数值参数放置于要被格式化的参数之前
				/* it's the next argument */
				precision = va_arg(args, int);   //va_arg找变参列表下一个参数
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {    //参数被解释为短整型或无符号短整型，长整形或无符号长整形，长双精度型
			qualifier = *fmt;       //qualifier记录长度类型
			++fmt;
		}

		switch (*fmt) {               
		case 'c':                        //如果遇到格式占位符c,取出变参列表对应的那个字符
            if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's':                       //取出变参列表相对应的字符串
			s = va_arg(args, char *);
			if (!s)                       
				s = "<NULL>";
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;

			if (!(flags & LEFT))             //如果不是左对齐先输出空格
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)           //str记录s的每个字符
				*str++ = *s++;
			while (len < field_width--)         //如果是左对齐后输出空格
				*str++ = ' ';
			break;

		case 'o':
			str = number(str, va_arg(args, unsigned long), 8,       //八进制输出
				field_width, precision, flags);
			break;

		case 'p':                   //指针地址输出
			if (field_width == -1) {             
				field_width = 8;
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;

		case 'x':               //无符号十六进制整数
			flags |= SMALL;
		case 'X':                  //无符号十六进制整数（大写字母）
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		case 'd':              //有符号十进制整数
		case 'i':               //有符号十进制整数
			flags |= SIGN;
		case 'u':               //无符号十进制整数
			str = number(str, va_arg(args, unsigned long), 10,
				field_width, precision, flags);
			break;

		case 'n':                  //无输出
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:            //字符
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';
	return str-buf;
}

int sprintf(char * buf, const char *fmt, ...)
{
	va_list args;   //定义参数列表
	int i;

	va_start(args, fmt);    //args指向可变参数列表的第一个元素，fmt是可变参数列表前面紧挨着的一个变量，即"..."之前的那个参数
	i=vsprintf(buf,fmt,args);   
	va_end(args);    //使输入的参数args置为NULL
	return i;
}

