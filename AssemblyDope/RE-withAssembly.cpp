// RE-withAssembly.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

extern "C" int _stdcall func_plus(int a, int b) {
	return a + b;
}

extern "C" int _fastcall func_minus(int a, int b);
extern "C" int _fastcall func_mul(int a, int b);

int main()
{
	int x = func_plus(1, 2);
	printf("x = %d \n", x);
	x = func_minus(10, 5);
	printf("x = %d \n", x);

	x = func_mul(10, 5);
	printf("x = %d \n", x);

    return 0;
}

