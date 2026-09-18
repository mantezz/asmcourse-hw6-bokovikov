#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <errno.h>

int iterations = 0;
static const double eps1 = 1e-6;
static const double eps2 = 1e-6;

typedef double afunc(double);

typedef enum {
    F1 = 1,  // 3*(0.5/(x + 1) + 1)
    F2,      // 2.5x - 9.5
    F3,      // 5/x
} FunctionID;

// Объявления ассемблерных функций
extern double f1(double x);
extern double f2(double x);
extern double f3(double x);
extern double df1(double x);
extern double df2(double x);
extern double df3(double x);

// Тестовые функции и их производные
static double test_func1(double x) { return x; }
static double test_func2(double x) { return x * x - 3; }
static double test_func3(double x) { return x * x * x - 4; }

static double test_df1(double x) { (void)x; return 1; }
static double test_df2(double x) { return 2 * x; }
static double test_df3(double x) { return 3 * x * x; }

static afunc* get_function(FunctionID id) {
    switch(id) {
        case F1: return f1;
        case F2: return f2;
        case F3: return f3;
        default: return NULL;
    }
}

static afunc* get_test_function(int id) {
    switch (id) {
        case 1: return test_func1;
        case 2: return test_func2;
        case 3: return test_func3;
        default: return NULL;
    }
}

static afunc* get_test_derivative(int id) {
    switch (id) {
        case 1: return test_df1;
        case 2: return test_df2;
        case 3: return test_df3;
        default: return NULL;
    }
}

// Комбинированный метод (хорд и касательных)
static double root(afunc *f, afunc *g, afunc *df, afunc *dg, double a, double b, double eps) {
    double x0 = a, x1 = b;
    double fa = f(a) - g(a);
    double fb = f(b) - g(b);

    if (fa * fb >= 0) {
        fprintf(stderr, "Root is not bracketed in [%g, %g]\n", a, b);
        return NAN;
    }

    while (fabs(x1 - x0) > eps) {
        iterations++;

        // Метод хорд
        double x_chord = x1 - (f(x1) - g(x1)) * (x1 - x0) / ((f(x1) - g(x1)) - (f(x0) - g(x0)));

        // Метод Ньютона
        double x_tangent = x0 - (f(x0) - g(x0)) / (df(x0) - dg(x0));

        // Комбинируем результаты
        x0 = x1;
        x1 = (x_chord + x_tangent) / 2;
    }

    return x1;
}

// Формула Симпсона
static double integral(afunc *f, double a, double b, double eps) {
    int n = 2;
    double h = (b - a) / n;
    double sum_old = 0, sum_new = 0;
    double error = eps + 1;

    do {
        sum_old = sum_new;
        sum_new = f(a) + f(b);

        for (int i = 1; i < n; i++) {
            double x = a + i * h;
            sum_new += (i % 2 == 0) ? 2 * f(x) : 4 * f(x);
        }

        sum_new *= h / 3;
        if (n > 2) error = fabs(sum_new - sum_old) / 15;
        n *= 2;
        h /= 2;
    } while (error > eps && n < 1000000);

    return sum_new;
}

static void test_root(const char *params) {
    int f1_id, f2_id;
    double a, b, eps, expected;

    if (sscanf(params, "%d:%d:%lf:%lf:%lf:%lf", &f1_id, &f2_id, &a, &b, &eps, &expected) != 6) {
        fprintf(stderr, "Invalid test-root format\n");
        exit(EXIT_FAILURE);
    }

    afunc *f1 = get_test_function(f1_id);
    afunc *f2 = get_test_function(f2_id);
    afunc *df1 = get_test_derivative(f1_id);
    afunc *df2 = get_test_derivative(f2_id);

    if (!f1 || !f2 || !df1 || !df2) {
        fprintf(stderr, "Invalid function ID\n");
        exit(EXIT_FAILURE);
    }

    double result = root(f1, f2, df1, df2, a, b, eps);
    if (isnan(result)) {
        fprintf(stderr, "Root not found in given interval\n");
        exit(EXIT_FAILURE);
    }

    double abs_err = fabs(result - expected);
    double rel_err = abs_err / fabs(expected);

    printf("Test root result (test : %d:%d:%lf:%lf:%lf:%lf):\n", f1_id, f2_id, a, b, eps, expected);
    printf("  Computed: %.12f\n  Expected: %.12f\n", result, expected);
    printf("  Absolute error: %.2e\n  Relative error: %.2e\n", abs_err, rel_err);
}

static void test_integral(const char *params) {
    int f_id;
    double a, b, eps, correct_ans;

    if (sscanf(params, "%d:%lf:%lf:%lf:%lf", &f_id, &a, &b, &eps, &correct_ans) != 5) {
        fprintf(stderr, "Invalid format\n");
        exit(EXIT_FAILURE);
    }

    afunc *f = get_test_function(f_id);
    if (isnan(f(a)) || isnan(f(b))) {
        fprintf(stderr, "The function is undefined at some point in the interval");
    }
    double result = integral(f, a, b, eps);
    if (isnan(result)) {
        fprintf(stderr, "Integral computation failed\n");
        exit(EXIT_FAILURE);
    }

    double abs_err = fabs(result - correct_ans);
    double rel_err = abs_err / fabs(correct_ans);

    printf("Test integral result (test : %d:%lf:%lf:%lf:%lf)\n", f_id, a, b, eps, correct_ans);
    printf("  Computed: %.12f\n  Expected: %.12f\n", result, correct_ans);
    printf("  Absolute error: %.2e\n  Relative error: %.2e\n", abs_err, rel_err);
}

int main(int argc, char *argv[]) {
    bool show_help = false;
    bool show_root = false;
    bool show_iterations = false;
    char *test_root_params = NULL;
    char *test_integral_params = NULL;

    // Парсинг аргументов командной строки
    for(int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = true;
        }
        else if(strcmp(argv[i], "--root") == 0 || strcmp(argv[i], "-r") == 0) {
            show_root = true;
        }
        else if(strcmp(argv[i], "--iterations") == 0 || strcmp(argv[i], "-i") == 0) {
            show_iterations = true;
        }
        else if(strcmp(argv[i], "--test-root") == 0 || strcmp(argv[i], "-R") == 0) {
            if(argc < i + 1) {
                fprintf(stderr, "Missing argument for --test-root\n");
                exit(EXIT_FAILURE);
            }
            test_root_params = argv[++i];
        }
        else if(strcmp(argv[i], "--test-integral") == 0 || strcmp(argv[i], "-I") == 0) {
            if (argc < i + 1) {
                fprintf(stderr, "Missing argument for --test-integral\n");
                exit(EXIT_FAILURE);
            }
            test_integral_params = argv[++i];
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (show_help) {
        printf(
        "Options:\n"
        "  -h, --help                     Show help\n"
        "  -r, --root                     Find intersection point\n"
        "  -i, --iterations               Show iterations count\n"
        "  -R, --test-root F_i:F_j:A:B:E:R  Test root function\n"
        "  -I, --test-integral F_i:A:B:E:R   Test integral function\n\n"
        "Function IDs:\n"
        "  1: 3*(0.5/(x + 1) + 1)\n"
        "  2: 2.5x - 9.5\n"
        "  3: 5/x\n");
        return 0;
    }

    if (test_root_params) {
        test_root(test_root_params);
        return 0;
    }

    if (test_integral_params) {
        test_integral(test_integral_params);
        return 0;
    }

    afunc *f1 = get_function(F1);
    afunc *f2 = get_function(F2);
    afunc *f3 = get_function(F3);

    // Находим точки пересечения
    double x12 = root(f1, f2, df1, df2, 1.1, 6.0, eps1);
    double x13 = 1.377011;
    double x23 = root(f2, f3, df2, df3, 1.1, 6.0, eps1);

    if (show_root) {
        printf("Intersections:\n"
                "   F1, F2: %.6f\n"
                "   F1, F3: %.6f\n"
                "   F2, F3: %.6f\n",
                x12, x13, x23);
        return 0;
    }

    if (show_iterations) {
        printf("Root iterations count: %d\n", iterations);
        return 0;
    }

    // Вычисление площади
    double area1 = integral(f1, x13, x12, eps2) - integral(f3, x13, x12, eps2);
    double area2 = integral(f1, x12, x23, eps2) - integral(f2, x12, x23, eps2);
    double total_area = area1 + area2;

    printf("The area bounded by curves = %.6f\n", total_area);

    return 0;
}
