#include <sdk/config.h>
#include <stdio.h>
#include <string.h>

extern int aps_template_asmp_class(int argc, char *argv[]);

int aps_template_asmp_main(int argc, char *argv[])
{
    return aps_template_asmp_class(argc, argv);
}