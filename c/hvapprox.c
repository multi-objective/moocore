#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "hvapprox.h"
#include "common.h"
#include "pow_int.h"
#include "rng.h"

#define STATIC_CAST(TYPE,OP) ((TYPE)(OP))
#define EPSILON 1e-20

typedef uint_fast8_t dimension_t;

#ifndef M_PIl
# define M_PIl		3.141592653589793238462643383279502884L /* pi */
#endif
#ifndef M_PI_2l
# define M_PI_2l	1.570796326794896619231321691639751442L /* pi/2 */
#endif
#ifndef M_PI_4l
# define M_PI_4l	0.785398163397448309615660845819875721L /* pi/4 */
#endif

static uint_fast32_t *
construct_polar_a(dimension_t dim, uint_fast32_t nsamples)
{
    ASSUME(dim >= 1);
    ASSUME(dim <= 32);
    // Step 1: find prime p such that dim <= eularfunction(p)/2 == (p-1)/2
    static const dimension_t primes [] = {
        1,  3,  5,  7, 11, 11, 13, 17, 17, 19,
        23, 23, 29, 29, 29, 31, 37, 37, 37, 41,
        41, 43, 47, 47, 53, 53, 53, 59, 59, 59,
        61, 67, 67 };

    const dimension_t p = primes[dim];
    DEBUG2_PRINT("construct_polar_a: prime: %u\n", p);

    uint_fast32_t * a = malloc(dim * sizeof(uint_fast32_t));
    a[0] = 1;
    DEBUG2_PRINT("construct_polar_a: a[%u] = %lu", dim, a[0]);
    for (dimension_t k = 1; k < dim; k++) {
        long double temp = 2 * fabsl(cosl(2 * M_PIl * k / p));
        temp = temp - floorl(temp);
        a[k] = STATIC_CAST(uint_fast32_t, llroundl(nsamples * temp));
        DEBUG2_PRINT(", %lu", a[k]);
    }
    DEBUG2_PRINT("\n");
    return a;
}

static void
compute_polar_sample(long double * sample, dimension_t dim,
                     uint_fast32_t i, uint_fast32_t nsamples,
                     const uint_fast32_t * a)
{
    ASSUME(i + 1 <= nsamples);
    if (i + 1 < nsamples) {
        long double factor = (i+1) / STATIC_CAST(long double, nsamples);
        for (dimension_t k = 0; k < dim; k++) {
            long double val = (factor * a[k]);
            sample[k] = val - floorl(val);
        }
    } else { // Last point is always 0.
        for (dimension_t k = 0; k < dim; k++)
            sample[k] = 0.0;
    }
}

// Calculate \int_{0}^{b} \sin^m x dx
/* Generated using this code:

```python
import sympy as sp
from sympy import sin, cos, Rational,Float
x = sp.symbols('x', real=True)
# Upper limit of integration
b = sp.symbols('b', real=True)

manually_simplified = {
    4: Rational(3,8)*b - cos(b)*sin(b)*(Rational(1,4)*sin(b)**2 + Rational(3,8)),
    5: Rational(8,15) - cos(b) * (cos(b)**4/5 - 2*cos(b)**2/3 + 1),
    6: Rational(5,16)*b - sin(b) * cos(b) * (Rational(1,6)*sin(b)**4 + 5*sin(b)**2/24 + Rational(5,16)),
    7: (cos(b)**6/7 - 3*cos(b)**4/5 + cos(b)**2 - 1)*cos(b) + Rational(16,35),
    8: 35*b/128 - (sin(b)**6/8 + 7*sin(b)**4/48 + 35*sin(b)**2/192 + Rational(35,128))*sin(b)*cos(b),
    9: Rational(128,315) - cos(b) * (cos(b)**8/9 - 4*cos(b)**6/7 + 6*cos(b)**4/5 - 4*cos(b)**2/3 + 1)
}

# Function to calculate the definite integral of sin(x)^n from 0 to b
def sin_power_integral(n, b):
    expr = sin(x)**n
    integral = sp.integrate(expr, (x, 0, b))
    if n in manually_simplified:
        assert sp.simplify(integral - manually_simplified[n], rational=True) == 0
        return manually_simplified[n]
    integral = integral.collect(sin(b)*cos(b), exact=False)
    if n == 2:
        return sp.simplify(integral);
    if n == 3:
        return sp.simplify(sp.simplify(integral));
    return integral

# Calculate and print the integrals for n from 0 to 32
results = {n: sin_power_integral(n, b) for n in range(33)}

# Function to check if a number has an exact floating-point representation
def is_exact_float(e):
    return isinstance(e, sp.Rational) and e.q != 1 and e.q & (e.q - 1) == 0  # Check if denominator is power of 2

# Function to convert rational numbers to float only if exact
def convert_rational(expr):
    return expr.replace(lambda e: is_exact_float(e), lambda e: Float(e,precision=128))

from sympy.printing.c import C99CodePrinter
class MyCCodePrinter(C99CodePrinter):
    def _print_Rational(self, expr):
        p, q = int(expr.p), int(expr.q)
        return '%d/%d.' % (p, q)

def custom_ccode(expr):
    expr = convert_rational(expr)  # Convert exact rationals to floats
    expr = expr.subs({cos(b): sp.Symbol('cos_b'), sin(b): sp.Symbol('sin_b')})  # Replace cos(b) -> cos_b, sin(b) -> sin_b
    return MyCCodePrinter().doprint(expr)

for n, integral in results.items():
    c_code = custom_ccode(integral)
    print(f'case {n}:\n    return {c_code};')
``` */
static double
int_of_power_of_sin_from_0_to_b(dimension_t m, double b)
{
#define POW fast_pow_uint_max32
    double sin_b, cos_b;

    switch (m) {
      case 0:
          return b;
      case 1:
          return 1 - cos(b);
      case 2:
          return 0.5*b - 0.25*sin(2*b);
      case 3:
          cos_b = cos(b);
          return POW(cos_b, 3)/3 - cos_b + 2/3.;
      case 4:
          sin_b = sin(b); cos_b = cos(b);
          return 0.375*b - cos_b*sin_b*(0.25*POW(sin_b, 2) + 0.375);
      case 5:
          cos_b = cos(b);
          return 8/15. - cos_b*(POW(cos_b, 4)/5 - 2/3.*POW(cos_b, 2) + 1);
      case 6:
          sin_b = sin(b); cos_b = cos(b);
          return 0.3125*b - cos_b*sin_b*(POW(sin_b, 4)/6 + (5/24.)*POW(sin_b, 2) + 0.3125);
      case 7:
          cos_b = cos(b);
          return cos_b*(POW(cos_b, 6)/7 - 3/5.*POW(cos_b, 4) + POW(cos_b, 2) - 1) + 16/35.;
      case 8:
          sin_b = sin(b); cos_b = cos(b);
          return 0.2734375*b - cos_b*sin_b*(0.125*POW(sin_b, 6) + (7/48.)*POW(sin_b, 4) + (35/192.)*POW(sin_b, 2) + 0.2734375);
      case 9:
          cos_b = cos(b);
          return 128/315. - cos_b*(POW(cos_b, 8)/9. - 4/7.*POW(cos_b, 6) + (6/5.)*POW(cos_b, 4) - 4/3.*POW(cos_b, 2) + 1);
      case 10:
          sin_b = sin(b); cos_b = cos(b);
          return 0.24609375*b - cos_b*sin_b*(POW(sin_b, 8)/10. + 9/80.*POW(sin_b, 6) + 21/160.*POW(sin_b, 4) + 0.1640625*POW(sin_b, 2) + 0.24609375);
      case 11:
          cos_b = cos(b);
          return POW(cos_b, 11)/11. - 5/9.*POW(cos_b, 9) + (10/7.)*POW(cos_b, 7) - 2*POW(cos_b, 5) + (5/3.)*POW(cos_b, 3) - cos_b + 256/693.;
      case 12:
          sin_b = sin(b); cos_b = cos(b);
          return 0.2255859375*b - cos_b*sin_b*(POW(sin_b, 10)/12 + 11/120.*POW(sin_b, 8) + 33/320.*POW(sin_b, 6) + 77/640.*POW(sin_b, 4) + 0.150390625*POW(sin_b, 2) + 0.2255859375);
      case 13:
          cos_b = cos(b);
          return 1024/3003. -POW(cos_b, 13)/13. + (6/11.)*POW(cos_b, 11) - 5/3.*POW(cos_b, 9) + (20/7.)*POW(cos_b, 7) - 3*POW(cos_b, 5) + 2*POW(cos_b, 3) - cos_b;
      case 14:
          sin_b = sin(b); cos_b = cos(b);
          return 0.20947265625*b - cos_b*sin_b*(POW(sin_b, 12)/14. + 13/168.*POW(sin_b, 10) + 143/1680.*POW(sin_b, 8) + 429/4480.*POW(sin_b, 6) + 143/1280.*POW(sin_b, 4) + 0.1396484375*POW(sin_b, 2) + 0.20947265625);
      case 15:
          cos_b = cos(b);
          return POW(cos_b, 15)/15. - 7/13.*POW(cos_b, 13) + (21/11.)*POW(cos_b, 11) - 35/9.*POW(cos_b, 9) + 5*POW(cos_b, 7) - 21/5.*POW(cos_b, 5) + (7/3.)*POW(cos_b, 3) - cos_b + 2048/6435.;
      case 16:
          sin_b = sin(b); cos_b = cos(b);
          return 0.196380615234375*b - cos_b*sin_b*(0.0625*POW(sin_b, 14) + 15/224.*POW(sin_b, 12) + 65/896.*POW(sin_b, 10) + 143/1792.*POW(sin_b, 8) + 1287/14336.*POW(sin_b, 6) + 0.104736328125*POW(sin_b, 4) + 0.13092041015625*POW(sin_b, 2) + 0.196380615234375);
      case 17:
          cos_b = cos(b);
          return 32768/109395. - POW(cos_b, 17)/17. + (8/15.)*POW(cos_b, 15) - 28/13.*POW(cos_b, 13) + (56/11.)*POW(cos_b, 11) - 70/9.*POW(cos_b, 9) + 8*POW(cos_b, 7) - 28/5.*POW(cos_b, 5) + (8/3.)*POW(cos_b, 3) - cos_b;
      case 18:
          sin_b = sin(b); cos_b = cos(b);
          return 0.1854705810546875*b - cos_b*sin_b*(POW(sin_b, 16)/18. + 17/288.*POW(sin_b, 14) + 85/1344.*POW(sin_b, 12) + 1105/16128.*POW(sin_b, 10) + 2431/32256.*POW(sin_b, 8) + 2431/28672.*POW(sin_b, 6) + 2431/24576.*POW(sin_b, 4) + 12155/98304.*POW(sin_b, 2) + 0.1854705810546875);
      case 19:
          cos_b = cos(b);
          return POW(cos_b, 19)/19. - 9/17.*POW(cos_b, 17) + (12/5.)*POW(cos_b, 15) - 84/13.*POW(cos_b, 13) + (126/11.)*POW(cos_b, 11) - 14*POW(cos_b, 9) + 12*POW(cos_b, 7) - 36/5.*POW(cos_b, 5) + 3*POW(cos_b, 3) - cos_b + 65536/230945.;
      case 20:
          sin_b = sin(b); cos_b = cos(b);
          return 0.17619705200195313*b - cos_b*sin_b*(POW(sin_b, 18)/20. + 19/360.*POW(sin_b, 16) + 323/5760.*POW(sin_b, 14) + 323/5376.*POW(sin_b, 12) + 4199/64512.*POW(sin_b, 10) + 46189/645120.*POW(sin_b, 8) + 46189/573440.*POW(sin_b, 6) + 46189/491520.*POW(sin_b, 4) + 46189/393216.*POW(sin_b, 2) + 0.17619705200195313);
      case 21:
          cos_b = cos(b);
          return 262144/969969. -POW(cos_b, 21)/21 + (10/19.)*POW(cos_b, 19) - 45/17.*POW(cos_b, 17) + 8*POW(cos_b, 15) - 210/13.*POW(cos_b, 13) + (252/11.)*POW(cos_b, 11) - 70/3.*POW(cos_b, 9) + (120/7.)*POW(cos_b, 7) - 9*POW(cos_b, 5) + (10/3.)*POW(cos_b, 3) - cos_b;
      case 22:
          sin_b = sin(b); cos_b = cos(b);
          return 0.16818809509277344*b - cos_b*sin_b*(
              POW(sin_b, 20)/22. + 21/440.*POW(sin_b, 18) + 133/2640.*POW(sin_b, 16) + 2261/42240.*POW(sin_b, 14) + 323/5632.*POW(sin_b, 12) + 4199/67584.*POW(sin_b, 10)
              + 4199/61440.*POW(sin_b, 8) + 12597/163840.*POW(sin_b, 6) + 29393/327680.*POW(sin_b, 4) + 0.11212539672851563*POW(sin_b, 2) + 0.16818809509277344);
      case 23:
          cos_b = cos(b);
          return POW(cos_b, 23)/23 - 11/21.*POW(cos_b, 21) + (55/19.)*POW(cos_b, 19) - 165/17.*POW(cos_b, 17) + 22*POW(cos_b, 15) - 462/13.*POW(cos_b, 13) + 42*POW(cos_b, 11) - 110/3.*POW(cos_b, 9) + (165/7.)*POW(cos_b, 7) - 11*POW(cos_b, 5) + (11/3.)*POW(cos_b, 3) - cos_b + 524288/2028117.;
      case 24:
          sin_b = sin(b); cos_b = cos(b);
          return 0.16118025779724121*b - cos_b*sin_b*(POW(sin_b, 22)/24. + 23/528.*POW(sin_b, 20) + 161/3520.*POW(sin_b, 18) + 3059/63360.*POW(sin_b, 16) + 52003/1013760.*POW(sin_b, 14) + 7429/135168.*POW(sin_b, 12) + 96577/1622016.*POW(sin_b, 10) + 96577/1474560.*POW(sin_b, 8) + 96577/1310720.*POW(sin_b, 6) + 676039/7864320.*POW(sin_b, 4) + 676039/6291456.*POW(sin_b, 2) + 0.16118025779724121);
      case 25:
          cos_b = cos(b);
          return 4194304/16900975. - POW(cos_b, 25)/25. + (12/23.)*POW(cos_b, 23) - 22/7.*POW(cos_b, 21) + (220/19.)*POW(cos_b, 19) - 495/17.*POW(cos_b, 17) + (264/5.)*POW(cos_b, 15) - 924/13.*POW(cos_b, 13) + 72*POW(cos_b, 11) - 55*POW(cos_b, 9) + (220/7.)*POW(cos_b, 7) - 66/5.*POW(cos_b, 5) + 4*POW(cos_b, 3) - cos_b;
      case 26:
          sin_b = sin(b); cos_b = cos(b);
          return 0.15498101711273193*b - cos_b*sin_b*(
              POW(sin_b, 24)/26. + 25/624.*POW(sin_b, 22) + 575/13728.*POW(sin_b, 20) + 805/18304.*POW(sin_b, 18) + 15295/329472.*POW(sin_b, 16) + 260015/5271552.*POW(sin_b, 14)
              + 185725/3514368.*POW(sin_b, 12) + 185725/3244032.*POW(sin_b, 10) + 37145/589824.*POW(sin_b, 8) + 0.070848464965820313*POW(sin_b, 6) + 260015/3145728.*POW(sin_b, 4) + 1300075/12582912.*POW(sin_b, 2) + 0.15498101711273193);
      case 27:
          cos_b = cos(b);
          return POW(cos_b, 27)/27. - 13/25.*POW(cos_b, 25) + (78/23.)*POW(cos_b, 23) - 286/21.*POW(cos_b, 21) + (715/19.)*POW(cos_b, 19) - 1287/17.*POW(cos_b, 17) + (572/5.)*POW(cos_b, 15) - 132*POW(cos_b, 13) + 117*POW(cos_b, 11) - 715/9.*POW(cos_b, 9) + (286/7.)*POW(cos_b, 7) - 78/5.*POW(cos_b, 5) + (13/3.)*POW(cos_b, 3) - cos_b + 8388608/35102025.;
      case 28:
          sin_b = sin(b); cos_b = cos(b);
          return 0.14944598078727722*b - cos_b*sin_b*(
              POW(sin_b, 26)/28. + 27/728.*POW(sin_b, 24) + 225/5824.*POW(sin_b, 22) + 5175/128128.*POW(sin_b, 20) + 3105/73216.*POW(sin_b, 18) + 6555/146432.*POW(sin_b, 16) + 111435/2342912.*POW(sin_b, 14) + 1671525/32800768.*POW(sin_b, 12) + 557175/10092544.*POW(sin_b, 10) + 111435/1835008.*POW(sin_b, 8) + 1002915/14680064.*POW(sin_b, 6) + 0.079704523086547852*POW(sin_b, 4) + 0.099630653858184814*POW(sin_b, 2) + 0.14944598078727722);
      case 29:
          cos_b = cos(b);
          return 33554432/145422675. - POW(cos_b, 29)/29 + (14/27.)*POW(cos_b, 27) - 91/25.*POW(cos_b, 25) + (364/23.)*POW(cos_b, 23) - 143/3.*POW(cos_b, 21) + (2002/19.)*POW(cos_b, 19) - 3003/17.*POW(cos_b, 17) + (1144/5.)*POW(cos_b, 15) - 231*POW(cos_b, 13) + 182*POW(cos_b, 11) - 1001/9.*POW(cos_b, 9) + 52*POW(cos_b, 7) - 91/5.*POW(cos_b, 5) + (14/3.)*POW(cos_b, 3) - cos_b;
      case 30:
          sin_b = sin(b); cos_b = cos(b);
          return 0.14446444809436798*b - cos_b*sin_b*(
              POW(sin_b, 28)/30. + 29/840.*POW(sin_b, 26) + 261/7280.*POW(sin_b, 24) + 435/11648.*POW(sin_b, 22) + 10005/256256.*POW(sin_b, 20) + 6003/146432.*POW(sin_b, 18) + 12673/292864.*POW(sin_b, 16) + 215441/4685824.*POW(sin_b, 14) + 3231615/65601536.*POW(sin_b, 12) + 1077205/20185088.*POW(sin_b, 10) + 215441/3670016.*POW(sin_b, 8) + 1938969/29360128.*POW(sin_b, 6) + 0.07704770565032959*POW(sin_b, 4) + 0.096309632062911987*POW(sin_b, 2) + 0.14446444809436798);
      case 31:
          cos_b = cos(b);
          return POW(cos_b, 31)/31. - 15/29.*POW(cos_b, 29) + (35/9.)*POW(cos_b, 27) - 91/5.*POW(cos_b, 25) + (1365/23.)*POW(cos_b, 23) - 143*POW(cos_b, 21) + (5005/19.)*POW(cos_b, 19) - 6435/17.*POW(cos_b, 17) + 429*POW(cos_b, 15) - 385*POW(cos_b, 13) + 273*POW(cos_b, 11) - 455/3.*POW(cos_b, 9) + 65*POW(cos_b, 7) - 21*POW(cos_b, 5) + 5*POW(cos_b, 3) - cos_b + 67108864/300540195.;
      case 32:
          sin_b = sin(b); cos_b = cos(b);
          return 0.13994993409141898*b - cos_b*sin_b*(
              0.03125*POW(sin_b, 30) + 31/960.*POW(sin_b, 28) + 899/26880.*POW(sin_b, 26) + 8091/232960.*POW(sin_b, 24) + 13485/372736.*POW(sin_b, 22) + 310155/8200192.*POW(sin_b, 20) + 186093/4685824.*POW(sin_b, 18) + 392863/9371648.*POW(sin_b, 16) + 6678671/149946368.*POW(sin_b, 14) + 100180065/2099249152.*POW(sin_b, 12) + 33393355/645922816.*POW(sin_b, 10) + 6678671/117440512.*POW(sin_b, 8) + 60108039/939524096.*POW(sin_b, 6) + 0.07463996484875679*POW(sin_b, 4) + 0.093299956060945988*POW(sin_b, 2) + 0.13994993409141898);
      default:
          unreachable();
    }
#undef POW
}

// \int_{0}^{pi/2} sin^i(x) dx
static const long double int_power_of_sin_from_0_to_half_pi[] = {
    M_PI_2l, 1.L, M_PI_4l, 2 / 3.L, 3 * M_PIl / 16,
    8 / 15.L, 5.L * M_PIl / 32, 16 / 35.L, 35.L * M_PIl / 256, 128 / 315.L,
    63 * M_PIl / 512, 256 / 693.L, 231 * M_PIl / 2048, 1024 / 3003.L, 429 * M_PIl / 4096,
    // 16 -> 19
    2048 / 6435.L,  6435 * M_PIl / 65536.L, 32768 / 109395.L, 12155 * M_PIl / 131072, 65536 / 230945.L,
    // 20 -> 24
    46189 * M_PIl / 524288, 262144 / 969969.L, 88179 * M_PIl / 1048576, 524288 / 2028117.L, 676039 * M_PIl / 8388608,
    // 25 -> 29
    4194304 / 16900975.L, 1300075 * M_PIl / 16777216, 8388608 / 35102025.L, 5014575 * M_PIl / 67108864, 33554432 / 145422675.L,
    // 30 -> 32
    9694845 * M_PIl / 134217728.L, 67108864 / 300540195.L, 300540195 * M_PIl / 4294967296
};

// Solve inverse integral of power of sin.
static long double
solve_inverse_int_of_power_sin(long double theta, dimension_t dim)
{
    long double x = M_PI_2l;
    long double newf = int_power_of_sin_from_0_to_half_pi[dim] - theta;
    // ??? Does this need to be EPSILON? If it does, it becomes very slow.
    // Even 1e-16 is much slower.
    while (fabsl(newf) > 1e-15) {
        long double g = powl_uint(sinl(x), dim);
        x -= newf / g;
        newf = int_of_power_of_sin_from_0_to_b(dim, (double)x) - theta;
    }
    return x;
}

// FIXME: Pre-calculate for all values of dim \in [1, 32].
/* We have 2*pi^(d/2)/gamma(d/2)/(2^d) = (pi^(d/2) / gamma(d/2)) * 2^(1-d)
   We do this in long double to no lose precision.
   ldexp(x, i) = x * 2^i,
*/
static inline long double sphere_volume(int d)
{
    ASSUME(d >= 1);
    ASSUME(d <= 32);
    return ldexpl(powl(M_PIl, d * .5L) / tgammal(d * .5L), 1 - d);
}

// FIXME: Pre-calculate for all values of dim.
/* Same as sphere_volume(d) / d but faster.
   We have 2*pi^(d/2)/gamma(d/2)/(2^d)/d = (pi^(d/2) / gamma(d/2 + 1)) * 2^(-d)
   We do this in long double to no lose precision.
*/
static inline long double sphere_volume_div_by_dim(int d)
{
    ASSUME(d >= 1);
    ASSUME(d <= 32);
    return ldexpl(powl(M_PIl, d * .5L) / tgammal(d * .5L + 1), -d);
}


static long double * compute_int_all(dimension_t dm1)
{
    ASSUME(dm1 >= 1);
    long double * int_all = malloc(dm1 * sizeof(long double));
    dimension_t i;
    DEBUG2_PRINT("int_all[%u] =", dm1);
    for (i = 0; i < dm1; i++) {
        int_all[i] = int_power_of_sin_from_0_to_half_pi[i];
        DEBUG2_PRINT(" %25.18Lg ", int_all[i]);
    }
    DEBUG2_PRINT("\n");

    long double prod_int_all = int_all[0];
    for (i = 1; i < dm1; i++) {
        prod_int_all *= int_all[i];
    }

    ASSUME(prod_int_all > 0);
    DEBUG2_PRINT(" sphere / prod_int_all = %22.15Lg / %22.15Lg = %22.15Lg\n",
                 sphere_volume(dm1 + 1), prod_int_all,
                 sphere_volume(dm1 + 1) / prod_int_all);
    // This should be always one, so why calculate it?
    prod_int_all = sphere_volume(dm1 + 1) / prod_int_all;
    ASSUME(fabsl(prod_int_all - 1.0) <= 1e-15);
    DEBUG2_PRINT("int_all[%u] =", dm1);
    for (i = 0; i < dm1; i++) {
        int_all[i] *= prod_int_all;
        DEBUG2_PRINT(" %22.15Lg ", int_all[i]);
    }
    DEBUG2_PRINT("\n");
    return int_all;
}

static void
compute_theta(long double *theta, dimension_t dim, const long double *int_all)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    for (dimension_t j = 0; j < dim - 1; j++) {
        // We multiply here because we computed 1 / int_all[j] before.
        theta[j] = solve_inverse_int_of_power_sin(
            theta[j] * int_all[(dim - 2) - j],
            STATIC_CAST(dimension_t, (dim - j) - 2));
    }
}

static void
compute_hua_wang_direction(double * direction, dimension_t dim,
                           const long double * theta)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    dimension_t k, j;
    direction[0] = STATIC_CAST(double, sinl(theta[0]));
    for (k = 1; k < dim - 1; k++)
        direction[0] *= STATIC_CAST(double, sinl(theta[k]));
    for (j = 1; j < dim; j++) {
        direction[j] = STATIC_CAST(double, cosl(theta[dim - j - 1]));
        for (k = 0; k < dim - j - 1; k++) {
            direction[j] *= STATIC_CAST(double, sinl(theta[k]));
        }
    }
    for (k = 0; k < dim; k++) {
        direction[k] = (fabs(direction[k]) <= EPSILON)
            ? 1. / EPSILON
            : 1. / direction[k];
    }
}

// Hypervolume approximation.
double hv_approx_hua_wang(const double * data, int nobjs, int npoints,
                          const double * ref, const bool * maximise,
                          uint_fast32_t nsamples)
{
    ASSUME(nobjs < 32);
    ASSUME(nobjs > 1);
    ASSUME(npoints >= 0);
    const dimension_t dim = (dimension_t) nobjs;
    double * points = malloc(dim * npoints * sizeof(double));

    int i, j;
    dimension_t k;
    // Transform data (ref - data)
    for (i = 0, j = 0; i < npoints; i++) {
        for (k = 0; k < dim; k++) {
            points[j * dim + k] = ref[k] - data[i * dim + k];
            if (maximise[k])
                points[j * dim + k] = -points[j * dim + k];
            // Filter out dominated points (must be >0 in all objectives)
            if (points[j * dim + k] <= 0)
                break;
        }
        if (k == dim)
            j++;
    }
    npoints = j;

    if (npoints == 0)
        return 0;

    const long double * int_all = compute_int_all(dim - 1);
    const uint_fast32_t * polar_a = construct_polar_a(dim - 1, nsamples);
    long double * theta = malloc((dim - 1) * sizeof(long double));
    double * w = malloc(dim * sizeof(double));
    const double c_m = (double) sphere_volume_div_by_dim(nobjs);
    double expected = 0.0;
    for (uint_fast32_t j = 0; j < nsamples; j++) {
        compute_polar_sample(theta, dim - 1, j, nsamples, polar_a);
        compute_theta(theta, dim, int_all);
        compute_hua_wang_direction(w, dim, theta);
        // points >= 0 && w >=0 so max_s_w cannot be < 0.
        double max_s_w = 0;
        for (i = 0; i < npoints; i++) {
            double min_ratio = INFINITY;
            for (k = 0; k < dim; k++) {
                double ratio = points[i * dim + k] * w[k];
                if (ratio < min_ratio) {
                    min_ratio = ratio;
                }
            }
            if (min_ratio > max_s_w) {
                max_s_w = min_ratio;
            }
        }
        expected += pow_uint(max_s_w, dim);
    }
    expected /= (double) nsamples;
    free((void *) int_all);
    free((void *) polar_a);
    free(theta);
    free(w);
    free(points);
    return c_m * expected;
}

// Hypervolume approximation.
double hv_approx_normal(const double * data, int nobjs, int npoints,
                        const double * ref, const bool * maximise,
                        uint_fast32_t nsamples, uint32_t random_seed)
{
    ASSUME(nobjs < 32);
    ASSUME(nobjs > 1);
    ASSUME(npoints >= 0);
    const dimension_t dim = (dimension_t) nobjs;
    double * points = malloc(dim * npoints * sizeof(double));

    int i, j;
    dimension_t k;
    // Transform data (ref - data)
    for (i = 0, j = 0; i < npoints; i++) {
        for (k = 0; k < dim; k++) {
            points[j * dim + k] = ref[k] - data[i * dim + k];
            if (maximise[k])
                points[j * dim + k] = -points[j * dim + k];
            // Filter out dominated points (must be >0 in all objectives)
            if (points[j * dim + k] <= 0)
                break;
        }
        if (k == dim)
            j++;
    }
    npoints = j;

    if (npoints == 0)
        return 0;

    rng_state * rng = rng_new(random_seed);
    double * w = malloc(dim * sizeof(double));
    const double c_m = (double) sphere_volume_div_by_dim(nobjs);
    double expected = 0.0;

    // Monte Carlo sampling
    for (uint_fast32_t j = 0; j < nsamples; j++) {
        // Generate random weights in positive orthant
        for (k = 0; k < dim; k++) {
            w[k] = fabs(rng_standard_normal(rng));
            if (w[k] <= 1e-15)
                w[k] = 1e-15;
        }
        double norm = 0.0;
        for (k = 0; k < dim; k++)
            norm += w[k] * w[k];
        norm = sqrt(norm);
        for (k = 0; k < dim; k++) {
            // 1 / (w[k] / norm) so we avoid the division when calculating the
            // ratio below.
            w[k] = norm / w[k];
        }
        // points >= 0 && w >=0 so max_s_w cannot be < 0.
        double max_s_w = 0;
        for (i = 0; i < npoints; i++) {
            double min_ratio = INFINITY;
            for (k = 0; k < dim; k++) {
                double ratio = points[i * dim + k] * w[k];
                if (ratio < min_ratio) {
                    min_ratio = ratio;
                }
            }
            if (min_ratio > max_s_w) {
                max_s_w = min_ratio;
            }
        }
        expected += pow_uint(max_s_w, dim);
#ifdef DETAILED_EXPERIMENTS
        if (j+1 >= 10000 && __builtin_popcountl(j+1) == 1)
            fprintf(stdout, "%lu\t%-22.15g\n", j+1, c_m * (expected/(double)(j+1)));
#endif
    }
    expected /= (double) nsamples;
    free(w);
    free(rng);
    free(points);
    return c_m * expected;
}
