// mathdemo.c — Demonstrate <math.h> functions /

#include <stdio.h>
#include <math.h>

int math(void) {
    double angle = 45.0;
    double radians = angle  (MPI / 180.0);

    printf("Angle: %.2f degrees = %.5f radians\n", angle, radians);
    printf("sin(%.2f°) = %.4f\n", angle, sin(radians));
    printf("cos(%.2f°) = %.4f\n", angle, cos(radians));
    printf("tan(%.2f°) = %.4f\n", angle, tan(radians));

    double value = 9.0;
    printf("\nBasic Math:\n");
    printf("sqrt(%.2f) = %.2f\n", value, sqrt(value));
    printf("pow(%.2f, 3) = %.2f\n", value, pow(value, 3));
    printf("log(%.2f) = %.4f\n", value, log(value));
    printf("exp(2) = %.4f\n", exp(2.0));
    printf("fabs(-123.45) = %.2f\n", fabs(-123.45));

    // Rounding examples /
    double n = 7.65;
    printf("\nRounding:\n");
    printf("floor(%.2f) = %.2f\n", n, floor(n));
    printf("ceil(%.2f) = %.2f\n", n, ceil(n));
    printf("round(%.2f) = %.2f\n", n, round(n));

    return 0;
}
/*
Angle: 45.00 degrees = 0.78540 radians
sin(45.00°) = 0.7071
cos(45.00°) = 0.7071
tan(45.00°) = 1.0000

Basic Math:
sqrt(9.00) = 3.00
pow(9.00, 3) = 729.00
log(9.00) = 2.1972
exp(2) = 7.3891
fabs(-123.45) = 123.45

Rounding:
floor(7.65) = 7.00
ceil(7.65) = 8.00
round(7.65) = 8.00
`

Key Points
• Header: <math.h>  
• Link flag: -lm (links libm, the math library)  
• Usage: mathematical computations (floating‑point and transcendental functions)  
• Constants: you can access approximations of π, e*, and huge values (via HUGEVAL).  

*/