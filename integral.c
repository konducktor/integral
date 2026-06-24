#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>

#ifdef CUSTOM // If custom, script will be built with their functions replacing default ones
#include "functions-custom.h"
#else
#include "functions.h"
#endif

// double f1(double x) { return 1 + 4 / (x*x + 1); }
// double df1(double x) { return -8 * x / pow(x*x + 1, 2); }
//
// double f2(double x) { return x*x*x; }
// double df2(double x) { return 3*x*x; }
//
// double f3(double x) { return 1 / pow(2, x); }
// double df3(double x) { return -1 * log(2) / pow(2, x); }

// amount of iterations done to count roots (works for both methods)
static unsigned root_iteractions;

// two borders for root search for fixed functions
static double left_border = -2.0;
static double right_border = 2.0;

// epsilon that would guarantee that the error will be less than 0.001
static double default_eps = 0.0001;

typedef double afunc(double);

double root(afunc *f, afunc *g, double a, double b, double eps1, afunc *df, afunc *dg);

double integral_with_n(afunc *f, double a, double b, unsigned n);

double integral(afunc *f, double a, double b, double eps2);

int main(int argc, char* argv[])
{
#ifdef CUSTOM_INPUT_FILE // setting borders if working with custom functions from the file
   FILE* input_file = fopen(CUSTOM_INPUT_FILE, "r");

   if (fscanf(input_file, "%lf %lf\n", &left_border, &right_border) != 2)
   {
      printf("Error reading borders from file\n");
      return -1;
   }

   fclose(input_file);
#endif
   const struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"root", no_argument, 0, 'r'},
      {"method", no_argument, 0, 'm'},
      {"iterations", no_argument, 0, 'i'},
      {"test-root", required_argument, 0, 'R'},
      {"test-integral", required_argument, 0, 'I'},
      {0, 0, 0, 0}
   };

   int root_flag = 0; // whether we need to output found roots
   int iterations_flag = 0; // whether we need to output iteration counts
   int test_flag = 0; // whether we need to calculate default functions or not

   int option_idx, fa_flag, fb_flag;
   double a, b, eps, expected, result;
   afunc *fa, *fb, *dfa, *dfb;
   while (1) {
      int option = getopt_long(argc, argv, "hrmiR:I:", options, &option_idx);
      if (option == -1) break;

      switch (option)
      {
      case 'h':
         printf("Optional arguments:\n");
         printf("\t-h | --help\tDisplays all options\n");
         printf("\t-r | --root\tAdditionally outputs intersections of curves\n");
         printf("\t-m | --method\tWill print out the root calculation method in use\n");
         printf("\t-i | --iterations\tOutputs the number of iterations required to calculate roots\n");
         printf("\t-R | --test-root\tTest root function with parameters in the form: F1:F2:A:B:E:R\n");
         printf("\t-I | --test-integral\tTest integral function with parameters in the form: F:A:B:E:R\n");
         break;
      case 'r':
         root_flag = 1;
         break;
      case 'm':
#ifdef METHOD_BISECTION
         printf("Bisection method is being used\n");
#else
         printf("Tangent (Newton) method is being used\n");
#endif
         break;
      case 'i':
         iterations_flag = 1;
         break;
      case 'R': // F1:F2:A:B:E:R
         sscanf(optarg, "%d:%d:%lf:%lf:%lf:%lf", &fa_flag, &fb_flag, &a, &b, &eps, &expected);

         if ((fa_flag == 1 && fb_flag == 2) || (fa_flag == 2 && fb_flag == 1)) {
            fa = f1;
            fb = f2;
            dfa = df1;
            dfb = df2;
         } else if ((fa_flag == 2 && fb_flag == 3) || (fa_flag == 3 && fb_flag == 2)) {
            fa = f2;
            fb = f3;
            dfa = df2;
            dfb = df3;
         } else {
            fa = f1;
            fb = f3;
            dfa = df1;
            dfb = df3;
         }

         result = root(fa, fb, a, b, eps, dfa, dfb);
         printf("%lf; %lf; %lf\n", result, 
            fabs(result - expected), 
            fabs((result - expected) / expected)
         );

         test_flag = 1;
         break;
      case 'I': // F:A:B:E:R
         sscanf(optarg, "%d:%lf:%lf:%lf:%lf", &fa_flag, &a, &b, &eps, &expected);

         if (fa_flag == 1) {
            fa = f1;
         } else if (fa_flag == 2) {
            fa = f2;
         } else {
            fa = f3;
         }

         result = integral(fa, a, b, eps);
         printf("%lf; %lf; %lf\n", result,
            fabs(result - expected),
            fabs((result - expected) / expected)
         );

         test_flag = 1;
         break;
      default:
         printf("Unknown argument used\n");
         return 1;
      } // switch (option)
   } // while (1)
      if (test_flag) return 0;

      root_iteractions = 0;
      double root1 = root(f1, f2, left_border, right_border, default_eps, df1, df2);
      double root2 = root(f2, f3, left_border, right_border, default_eps, df2, df3);
      double root3 = root(f1, f3, left_border, right_border, default_eps, df1, df3);

      if (root_flag)
      {
         printf("Roots:\n");
         printf("\troot1: %lf\n", root1);
         printf("\troot2: %lf\n", root2);
         printf("\troot3: %lf\n", root3);
      }
      if (iterations_flag) printf("Root iterations: %d\n", root_iteractions);

      double area = fabs(
         integral(f1, root1, root3, default_eps) -
         integral(f2, root1, root2, default_eps) -
         integral(f3, root2, root3, default_eps));
      printf("Resulting area: %lf\n", area);

   return 0;
} // int main(int argc, char* argv[])

double root(afunc *f, afunc *g, double a, double b, double eps1, afunc *df, afunc *dg)
{
#ifdef METHOD_BISECTION // Method of dividing sections by half
   double left_border = a;
   double right_border = b;

   double left_value = f(left_border) - g(left_border);
   // double right_value = f(right_border) - g(right_border); // redundant

   while (right_border - left_border >= eps1)
   {
      double mid = (left_border + right_border) / 2;
      double mid_value = f(mid) - g(mid);

      if (left_value * mid_value < 0)
      {
         right_border = mid;
         // right_value = mid_value
      }
      else
      {
         left_border = mid;
         left_value = mid_value;
      }

      root_iteractions += 1;
   }

   return left_border;
#else // By default, Newton (tangent) method will be used
   // We will find a sign at the middle point of the func
   double mid = (a + b) / 2.0;

   double Fprime = df(mid) - dg(mid); // F'

   // F''(x) = (F(x+h) - 2·F(x) + F(x-h)) / h², where h is a lil step for F''
   double h = (b - a) / 100.0;
   double Fdprime = (  (f(mid + h) - g(mid + h))
                    - 2*(f(mid)     - g(mid))
                    +   (f(mid - h) - g(mid - h))  ) / (h * h);

   // Deciding on a starter point
   int  from_right = (Fprime * Fdprime > 0);
   double d = from_right ? b : a;

   // Iterating, finish if F(c+-eps) and F(c) have different signs
   while (1)
   {
      double Fd  = f(d) - g(d);
      double Fpd = df(d) - dg(d);

      double c = d - Fd / Fpd;

      double Fc = f(c) - g(c);

      if (from_right)
      {
         double Fc_left = f(c - eps1) - g(c - eps1);
         if (Fc_left * Fc <= 0.0)
            return c;
      }
      else
      {
         double Fc_right = f(c + eps1) - g(c + eps1);
         if (Fc * Fc_right <= 0.0)
            return c;
      }

      d = c;   // for next iteration
      root_iteractions++;
   } // while (1)
#endif
} // double root()

// Calculates integral with N rectangles
double integral_with_n(afunc *f, double a, double b, unsigned n)
{
   double h = (b - a) / n;
   double rectangle_sum = 0;

   for (unsigned i = 0; i < n; ++i)
      rectangle_sum += f(a + h*(i+0.5));

   return rectangle_sum * h;
}

double integral(afunc *f, double a, double b, double eps2)
{
   unsigned n_precision = 10;
   double base_intgr = integral_with_n(f, a, b, n_precision);

   // Constant given in the task to calculate with rectangles
   const double runge_rule_coef = 1.0 / 3.0;

   while (1)
   {
      double double_intgr = integral_with_n(f, a, b, 2*n_precision);
      double runge_rule_value = runge_rule_coef * fabs(base_intgr - double_intgr);

      if (runge_rule_value < eps2)
         return double_intgr;

      base_intgr = double_intgr;
      n_precision *= 2;
   }
}
