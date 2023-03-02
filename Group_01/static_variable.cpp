#include <stdio.h>

int main()
{
	void thi_du();
	int n;
	for(n=1;n<=5;n++)
	{
		thi_du();
		printf("Hello\n");
	}
	return 3;
}

void thi_du()
{
	static int i=0;
	i++;
	printf("Goi ham lan thu: %d \n",i);
}
