/*
 * Test suite for argh.h
 */

#define ARGH_IMPLEMENTATION
#include "../argh.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)
/* A test passes only if it did not bump tests_failed */
#define RUN_TEST(name)                     \
    do                                     \
    {                                      \
        int failed_before = tests_failed;  \
        tests_run++;                       \
        printf("  Running %s... ", #name); \
        name();                            \
        if (tests_failed == failed_before) \
        {                                  \
            tests_passed++;                \
            printf("PASSED\n");            \
        }                                  \
    } while (0)

#define ASSERT(cond)                                                                        \
    do                                                                                      \
    {                                                                                       \
        if (!(cond))                                                                        \
        {                                                                                   \
            printf("FAILED\n    %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #cond); \
            tests_failed++;                                                                 \
            return;                                                                         \
        }                                                                                   \
    } while (0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_FLOAT_EQ(a, b) ASSERT(fabs((a) - (b)) < 0.0001f)
#define ASSERT_DOUBLE_EQ(a, b) ASSERT(fabs((a) - (b)) < 0.0000001)
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)
#define ASSERT_TRUE(a) ASSERT(a)
#define ASSERT_FALSE(a) ASSERT(!(a))

/* ============================================================================
 * Basic initialization tests
 * ============================================================================ */

TEST(test_init_basic)
{
    char *argv[] = {"program", "-v", "--output", "file.txt"};
    int argc = 4;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_EQ(parser.argc, 4);
    ASSERT_STR_EQ(parser.program, "program");
    ASSERT_EQ(parser.option_count, 0);
    ASSERT_EQ(parser.positional_count, 0);
    ASSERT_FALSE(argh_has_errors(&parser));

    argh_free(&parser);
}

TEST(test_init_extract_program_name)
{
    char *argv[] = {"/usr/bin/myapp", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_STR_EQ(parser.program, "myapp");

    argh_free(&parser);
}

TEST(test_init_windows_path)
{
    char *argv[] = {"C:\\Program Files\\app.exe", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_STR_EQ(parser.program, "app.exe");

    argh_free(&parser);
}

/* ============================================================================
 * Boolean option tests
 * ============================================================================ */

TEST(test_bool_short)
{
    char *argv[] = {"prog", "-v", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose mode");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));
    ASSERT_TRUE(argh_get_bool(&parser, "verbose"));

    argh_free(&parser);
}

TEST(test_bool_long)
{
    char *argv[] = {"prog", "--verbose", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose mode");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));
    ASSERT_TRUE(argh_get_bool(&parser, "verbose"));

    argh_free(&parser);
}

TEST(test_bool_false_value)
{
    char *argv[] = {"prog", "--verbose=false", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose mode");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));
    ASSERT_FALSE(argh_get_bool(&parser, "verbose"));

    argh_free(&parser);
}

TEST(test_bool_combined_short)
{
    char *argv[] = {"prog", "-abc", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "a", NULL, ARGH_BOOL, NULL, "Option A");
    argh_add(&parser, "b", NULL, ARGH_BOOL, NULL, "Option B");
    argh_add(&parser, "c", NULL, ARGH_BOOL, NULL, "Option C");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "a"));
    ASSERT_TRUE(argh_has(&parser, "b"));
    ASSERT_TRUE(argh_has(&parser, "c"));
    ASSERT_TRUE(argh_get_bool(&parser, "a"));
    ASSERT_TRUE(argh_get_bool(&parser, "b"));
    ASSERT_TRUE(argh_get_bool(&parser, "c"));

    argh_free(&parser);
}

/* ============================================================================
 * String option tests
 * ============================================================================ */

TEST(test_string_space_separator)
{
    char *argv[] = {"prog", "--output", "file.txt", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, "default.txt", "Output file");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_STR_EQ(argh_get_string(&parser, "output"), "file.txt");

    argh_free(&parser);
}

TEST(test_string_equals_separator)
{
    char *argv[] = {"prog", "--output=file.txt", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, "default.txt", "Output file");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_STR_EQ(argh_get_string(&parser, "output"), "file.txt");

    argh_free(&parser);
}

TEST(test_string_short)
{
    char *argv[] = {"prog", "-o", "myfile.txt", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, "default.txt", "Output file");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_STR_EQ(argh_get_string(&parser, "output"), "myfile.txt");

    argh_free(&parser);
}

TEST(test_string_default)
{
    char *argv[] = {"prog", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, "default.txt", "Output file");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_STR_EQ(argh_get_string(&parser, "output"), "default.txt");

    argh_free(&parser);
}

/* ============================================================================
 * Integer option tests
 * ============================================================================ */

TEST(test_int_basic)
{
    char *argv[] = {"prog", "--count", "42", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "c", "count", ARGH_INT, "0", "Count value");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_EQ(argh_get_int(&parser, "count"), 42);

    argh_free(&parser);
}

TEST(test_int_negative)
{
    char *argv[] = {"prog", "--offset", "-100", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, NULL, "offset", ARGH_INT, "0", "Offset value");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_EQ(argh_get_int(&parser, "offset"), -100);

    argh_free(&parser);
}

TEST(test_int_invalid)
{
    char *argv[] = {"prog", "--count", "not_a_number", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "c", "count", ARGH_INT, "0", "Count value");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_TRUE(argh_has_errors(&parser));

    argh_free(&parser);
}

/* ============================================================================
 * Float/Double option tests
 * ============================================================================ */

TEST(test_float_basic)
{
    char *argv[] = {"prog", "--ratio", "3.14", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "r", "ratio", ARGH_FLOAT, "1.0", "Ratio value");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_FLOAT_EQ(argh_get_float(&parser, "ratio"), 3.14f);

    argh_free(&parser);
}

TEST(test_float_scientific)
{
    char *argv[] = {"prog", "--value", "1.5e-10", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "value", ARGH_FLOAT, "0.0", "Small value");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_FLOAT_EQ(argh_get_float(&parser, "value"), 1.5e-10f);

    argh_free(&parser);
}

TEST(test_double_precision)
{
    char *argv[] = {"prog", "--pi", "3.14159265358979", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "p", "pi", ARGH_DOUBLE, "0.0", "Pi value");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_DOUBLE_EQ(argh_get_double(&parser, "pi"), 3.14159265358979);

    argh_free(&parser);
}

/* ============================================================================
 * Positional arguments tests
 * ============================================================================ */

TEST(test_positional_basic)
{
    char *argv[] = {"prog", "input.txt", "output.txt", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_EQ(parser.positional_count, 2);
    ASSERT_STR_EQ(parser.positional[0], "input.txt");
    ASSERT_STR_EQ(parser.positional[1], "output.txt");

    argh_free(&parser);
}

TEST(test_positional_mixed)
{
    char *argv[] = {"prog", "-v", "input.txt", "--output", "out.txt", "extra.txt", NULL};
    int argc = 6;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose");
    argh_add(&parser, "o", "output", ARGH_STRING, NULL, "Output");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));
    ASSERT_STR_EQ(argh_get_string(&parser, "output"), "out.txt");
    ASSERT_EQ(parser.positional_count, 2);
    ASSERT_STR_EQ(parser.positional[0], "input.txt");
    ASSERT_STR_EQ(parser.positional[1], "extra.txt");

    argh_free(&parser);
}

TEST(test_positional_end_of_options)
{
    char *argv[] = {"prog", "-v", "--", "-not_an_option", "file.txt", NULL};
    int argc = 5;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));
    ASSERT_EQ(parser.positional_count, 2);
    ASSERT_STR_EQ(parser.positional[0], "-not_an_option");
    ASSERT_STR_EQ(parser.positional[1], "file.txt");

    argh_free(&parser);
}

/* ============================================================================
 * Required options tests
 * ============================================================================ */

TEST(test_required_present)
{
    char *argv[] = {"prog", "--config", "settings.json", NULL};
    int argc = 3;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "c", "config", ARGH_STRING, NULL, "Config file");
    argh_require(&parser, "config");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_FALSE(argh_has_errors(&parser));

    argh_free(&parser);
}

TEST(test_required_missing)
{
    char *argv[] = {"prog", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "c", "config", ARGH_STRING, NULL, "Config file");
    argh_require(&parser, "config");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_TRUE(argh_has_errors(&parser));

    argh_free(&parser);
}

/* ============================================================================
 * Error handling tests
 * ============================================================================ */

TEST(test_unknown_option)
{
    char *argv[] = {"prog", "--unknown", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_TRUE(argh_has_errors(&parser));

    argh_free(&parser);
}

TEST(test_missing_value)
{
    char *argv[] = {"prog", "--output", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, NULL, "Output file");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_TRUE(argh_has_errors(&parser));

    argh_free(&parser);
}

TEST(test_error_printing)
{
    char *argv[] = {"prog", "--unknown", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    argh_parse(&parser);

    /* Should not crash */
    argh_print_error(&parser);
    argh_print_help(&parser);

    argh_free(&parser);
}

/* ============================================================================
 * Edge cases tests
 * ============================================================================ */

TEST(test_empty_argv)
{
    char *argv[] = {"prog", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_FALSE(argh_has_errors(&parser));

    argh_free(&parser);
}

TEST(test_long_only_option)
{
    char *argv[] = {"prog", "--verbose", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, NULL, "verbose", ARGH_BOOL, NULL, "Verbose mode");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "verbose"));

    argh_free(&parser);
}

TEST(test_short_only_option)
{
    char *argv[] = {"prog", "-v", NULL};
    int argc = 2;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", NULL, ARGH_BOOL, NULL, "Verbose mode");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_has(&parser, "v"));

    argh_free(&parser);
}

TEST(test_multiple_values)
{
    char *argv[] = {"prog", "--name", "first", "--name", "second", NULL};
    int argc = 5;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "n", "name", ARGH_STRING, NULL, "Name");

    ASSERT_TRUE(argh_parse(&parser));
    /* Last value wins */
    ASSERT_STR_EQ(argh_get_string(&parser, "name"), "second");

    argh_free(&parser);
}

TEST(test_help_generation)
{
    char *argv[] = {"prog", NULL};
    int argc = 1;

    argh_Parser parser;
    argh_init(&parser, argc, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Enable verbose output");
    argh_add(&parser, "o", "output", ARGH_STRING, "out.txt", "Output file path");
    argh_add(&parser, "c", "count", ARGH_INT, "10", "Number of iterations");
    argh_require(&parser, "output");

    /* Should not crash and should produce output */
    argh_print_help(&parser);

    argh_free(&parser);
}

/* ============================================================================
 * Main
 * ============================================================================ */

/* ============================================================================
 * Regression tests
 * ============================================================================ */

TEST(test_double_dash_is_not_a_value)
{
    char *argv[] = {"prog", "--output", "--", "file.txt"};
    argh_Parser parser;
    argh_init(&parser, 4, argv);
    argh_add(&parser, "o", "output", ARGH_STRING, NULL, "Output");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_EQ(parser.errors[0].code, ARGH_ERR_MISSING_VALUE);
    ASSERT_EQ(parser.positional_count, 1);
    ASSERT_STR_EQ(parser.positional[0], "file.txt");
    argh_free(&parser);
}

TEST(test_invalid_bool_value)
{
    char *argv[] = {"prog", "--verbose=maybe"};
    argh_Parser parser;
    argh_init(&parser, 2, argv);
    argh_add(&parser, "v", "verbose", ARGH_BOOL, NULL, "Verbose");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_EQ(parser.errors[0].code, ARGH_ERR_INVALID_VALUE);
    argh_free(&parser);
}

TEST(test_bool_value_case_insensitive)
{
    char *argv[] = {"prog", "--a=YES", "--b=Off"};
    argh_Parser parser;
    argh_init(&parser, 3, argv);
    argh_add(&parser, NULL, "a", ARGH_BOOL, NULL, "A");
    argh_add(&parser, NULL, "b", ARGH_BOOL, "true", "B");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_TRUE(argh_get_bool(&parser, "a"));
    ASSERT_FALSE(argh_get_bool(&parser, "b"));
    argh_free(&parser);
}

TEST(test_negative_number_as_value)
{
    char *argv[] = {"prog", "-n", "-5"};
    argh_Parser parser;
    argh_init(&parser, 3, argv);
    argh_add(&parser, "n", "count", ARGH_INT, "1", "Count");

    ASSERT_TRUE(argh_parse(&parser));
    ASSERT_EQ(argh_get_int(&parser, "count"), -5);
    argh_free(&parser);
}

TEST(test_int_out_of_range)
{
    char *argv[] = {"prog", "-n", "99999999999999999999"};
    argh_Parser parser;
    argh_init(&parser, 3, argv);
    argh_add(&parser, "n", "count", ARGH_INT, NULL, "Count");

    ASSERT_FALSE(argh_parse(&parser));
    ASSERT_EQ(parser.errors[0].code, ARGH_ERR_INVALID_VALUE);
    argh_free(&parser);
}

int main(void)
{
    printf("=== argh.h Test Suite ===\n\n");

    /* Init tests */
    printf("Initialization tests:\n");
    RUN_TEST(test_init_basic);
    RUN_TEST(test_init_extract_program_name);
    RUN_TEST(test_init_windows_path);

    /* Boolean tests */
    printf("\nBoolean option tests:\n");
    RUN_TEST(test_bool_short);
    RUN_TEST(test_bool_long);
    RUN_TEST(test_bool_false_value);
    RUN_TEST(test_bool_combined_short);

    /* String tests */
    printf("\nString option tests:\n");
    RUN_TEST(test_string_space_separator);
    RUN_TEST(test_string_equals_separator);
    RUN_TEST(test_string_short);
    RUN_TEST(test_string_default);

    /* Integer tests */
    printf("\nInteger option tests:\n");
    RUN_TEST(test_int_basic);
    RUN_TEST(test_int_negative);
    RUN_TEST(test_int_invalid);

    /* Float/Double tests */
    printf("\nFloat/Double option tests:\n");
    RUN_TEST(test_float_basic);
    RUN_TEST(test_float_scientific);
    RUN_TEST(test_double_precision);

    /* Positional tests */
    printf("\nPositional argument tests:\n");
    RUN_TEST(test_positional_basic);
    RUN_TEST(test_positional_mixed);
    RUN_TEST(test_positional_end_of_options);

    /* Required tests */
    printf("\nRequired option tests:\n");
    RUN_TEST(test_required_present);
    RUN_TEST(test_required_missing);

    /* Error handling tests */
    printf("\nError handling tests:\n");
    RUN_TEST(test_unknown_option);
    RUN_TEST(test_missing_value);
    RUN_TEST(test_error_printing);

    /* Edge cases */
    printf("\nEdge case tests:\n");
    RUN_TEST(test_empty_argv);
    RUN_TEST(test_long_only_option);
    RUN_TEST(test_short_only_option);
    RUN_TEST(test_multiple_values);
    RUN_TEST(test_help_generation);

    /* Regression tests */
    printf("\nRegression tests:\n");
    RUN_TEST(test_double_dash_is_not_a_value);
    RUN_TEST(test_invalid_bool_value);
    RUN_TEST(test_bool_value_case_insensitive);
    RUN_TEST(test_negative_number_as_value);
    RUN_TEST(test_int_out_of_range);

    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Run: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
