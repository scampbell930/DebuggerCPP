
/*---------------------------------------------------------------------------*\
 Factorial   : Simple factorial program
 Composed By : Sim, Mong Tee
 Dated		 : 23rd March 2022
 Copyright   : (C) 2022, Mong
 ==============================================================================
 History
 =======

 Author Date       Description
 ------ ---------- ------------------------------------------------------------
  Mong  20220323   Initial 
 
\*---------------------------------------------------------------------------*/

#include <stdint.h>
#include "stdlib.h"
#include <stdio.h>
#define printf R_printf 

//-----------------------------------------------------------------------------
// function prototype
//-----------------------------------------------------------------------------
uint32_t 	Iterative_Factorial     (uint32_t  num);
uint32_t 	RFactorial_Value        (uint32_t  num);
uint32_t    Recursive_Power         (uint32_t num);
uint32_t    Iterative_Power         (uint32_t num);

//-----------------------------------------------------------------------------
// Performs a math operation 2^num in a recursive method
//-----------------------------------------------------------------------------
uint32_t Recursive_Power(uint32_t num) {
    if (num > 0) {
        return 2 * (Recursive_Power(num - 1));
    }
    return 1;
}

//-----------------------------------------------------------------------------
// Performs a math operation 2^num in an interative method
//-----------------------------------------------------------------------------
uint32_t Iterative_Power(uint32_t num) {
    uint32_t retVal = 1;

    for (uint32_t x = num; x > 0; x--) {
        retVal *= 2;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
// Performs a math operation num! in an iterative method
//-----------------------------------------------------------------------------
uint32_t Iterative_Factorial(uint32_t num)
{
    uint32_t x;
    uint32_t retVal = 1;
    for (x = num; x > 1; x--)
    {
        retVal *= x;
    }

    return retVal;
}

//-----------------------------------------------------------------------------
// Performs a math operation num! in a recursive method pass by value
//-----------------------------------------------------------------------------
uint32_t RFactorial_Value(uint32_t num)
{
    if (num > 1)
	{
		return num * RFactorial_Value(num - 1);
	}
	
	return 1;
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main()
{
    uint32_t fVal   = 0;
    uint32_t num    = 0;

    // Iterative Method
    //
    fVal    = 0;
    num     = 6;
    fVal = Iterative_Factorial(num);
    printf("Iterative Factorial of %d! = %d\n", num, fVal);

    // Pass by value
    //
    fVal    = 0;
    num     = 6;
    fVal = RFactorial_Value(num);
    printf("Recursive Factorial of %d! = %d\n", num, fVal);

    // Iterative 2^num
    //
    fVal = 0;
    num = 3;
    fVal = Iterative_Power(num);
    printf("Iterative Power of 2^%d = %d\n", num, fVal);

    // Recursive 2^num
    //
    fVal = 0;
    num = 3;
    fVal = Recursive_Power(num);
    printf("Recursive Power of 2^%d = %d\n", num, fVal);

    return 0;
}
