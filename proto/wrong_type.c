/* Must NOT compile cleanly: an int option bound to a bool variable.
 * The build expects a diagnostic from the compiler. */

#include "macros.h"

static bool flag;

static const argh_opt opts[] = {
    ARGH_INT('n', "num", &flag, "Wrong target type"),
    ARGH_END,
};

int main(void)
{
    return opts[0].kind;
}
