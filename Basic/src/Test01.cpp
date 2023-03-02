
#include <stdio.h>

int  main()
{
	enum Color { red, green, blue };
	Color r = red;
	switch(r)
	{
		case red  : printf("%d",r);   break;
		case green: printf("green\n"); break;
		case blue : printf("blue\n");  break;
	}
	return 0;
}
