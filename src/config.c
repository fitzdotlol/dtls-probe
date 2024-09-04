#include <stdio.h>
#include <assert.h>

#include "vendor/toml.h"
#include "config.h"

int g_configLoaded = 0;

char *GAME_CONTENT_PATH = { 0 };
char *UPDATE_CONTENT_PATH = { 0 };
char *MOD_WORKSPACE_PATH = { 0 };
char *MOD_CONTENT_PATH = { 0 };
char *EXTRACT_PATH = { 0 };

void config_load()
{
    FILE *fp = fopen("project.toml", "r");
    assert(fp && "cannot open project.toml");

    char errbuf[0x100];
    toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    assert(conf);

    toml_datum_t dGAME_CONTENT_PATH = toml_string_in(conf, "GAME_CONTENT_PATH");
    toml_datum_t dUPDATE_CONTENT_PATH = toml_string_in(conf, "UPDATE_CONTENT_PATH");
    toml_datum_t dMOD_WORKSPACE_PATH = toml_string_in(conf, "MOD_WORKSPACE_PATH");
    toml_datum_t dMOD_CONTENT_PATH = toml_string_in(conf, "MOD_CONTENT_PATH");
    toml_datum_t dEXTRACT_PATH = toml_string_in(conf, "EXTRACT_PATH");

    assert(
        1
        && dGAME_CONTENT_PATH.ok
        && dUPDATE_CONTENT_PATH.ok
        && dMOD_WORKSPACE_PATH.ok
        && dMOD_CONTENT_PATH.ok
        && dEXTRACT_PATH.ok
    );

    GAME_CONTENT_PATH = dGAME_CONTENT_PATH.u.s;
    UPDATE_CONTENT_PATH = dUPDATE_CONTENT_PATH.u.s;
    MOD_WORKSPACE_PATH = dMOD_WORKSPACE_PATH.u.s;
    MOD_CONTENT_PATH = dMOD_CONTENT_PATH.u.s;
    EXTRACT_PATH = dEXTRACT_PATH.u.s;

    g_configLoaded = 1;
}
