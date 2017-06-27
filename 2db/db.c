/* -*- Mode: C; c-default-style: "linux"; c-basic-offset: 4; indent-tabs-mode: nil -*-
 * vim:sw=4:sts=4:et
 */

/*
 * db.c - general database module, forwards to backends
 *
 * Copyright (c) 2017 Sebastian Gniazdowski
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Sebastian Gniazdowski or the Zsh Development Group be
 * liable to any party for direct, indirect, special, incidental, or
 * consequential damages arising out of the use of this software and its
 * documentation, even if Paul Falstad and the Zsh Development Group have
 * been advised of the possibility of such damage.
 *
 * Sebastian Gniazdowski and the Zsh Development Group specifically disclaim
 * any warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software provided
 * hereunder is on an "as is" basis, and Paul Falstad and the Zsh Development
 * Group have no obligation to provide maintenance, support, updates,
 * enhancements, or modifications.
 */

#include "db.mdh"
#include "db.pro"
#include "db.h"

#ifndef PM_UPTODATE
#define PM_UPTODATE     (1<<19) /* Parameter has up-to-date data (e.g. loaded from DB) */
#endif

static void ztie_usage();
static void zuntie_usage();
static void ztaddress_usage();
static void ztclear_usage();

static HashTable createhashtable(char *name);
static void freebackendnode(HashNode hn);
static void backend_scan_fun(HashNode hn, int unused);

static Param createhashparam(char *name, int flags);

/* Type of provided (by backend module) entry-point */
typedef int (*DbBackendEntryPoint)(VA_ALIST1(int cmd));

struct backend_node {
    struct hashnode node;
    DbBackendEntryPoint main_entry;
};

typedef struct backend_node *BackendNode;

/* Maps db/dbtype onto BackendNode */
static HashTable backends_hash = NULL;

/* For searching with scanhashtable */
char *In_ParamName = NULL;
DbBackendEntryPoint Out_FoundBe = NULL;

static struct builtin bintab[] =
{
    /* h - help, d - backend type, r - read-only, a/f - address/file,
     * l - load password from terminal, p - password as argument,
     * P - password from file, z - zero read-cache */
    BUILTIN("ztie", 0, bin_ztie, 0, -1, 0, "hrzf:d:a:p:P:L:", NULL),
    BUILTIN("zuntie", 0, bin_zuntie, 0, -1, 0, "uh", NULL),
    BUILTIN("ztaddress", 0, bin_ztaddress, 0, -1, 0, "h", NULL),
    BUILTIN("ztclear", 0, bin_ztclear, 0, -1, 0, "h", NULL),
};

#define ROARRPARAMDEF(name, var)                                        \
    { name, PM_ARRAY | PM_READONLY, (void *) var, NULL,  NULL, NULL, NULL }

static void update_user_hash(char *id, int is_loading) {
    Param pm = (Param) paramtab->getnode(paramtab, "zdb_backends");
    HashTable ht;
    HashNode hn;
    Param val_pm;

    if(!pm) {
        zwarn("no such parameter: zdb_backends, internal error");
        return;
    }

    /* Must be a special hash */
    if (!(pm->node.flags & PM_HASHED) || !(pm->node.flags & PM_SPECIAL)) {
        zwarn("parameter zdb_backends is defined by user, will not update it");
        return;
    }

    ht = pm->u.hash;
    hn = gethashnode2(ht, id);
    val_pm = (Param) hn;

    if (val_pm) {
        if (val_pm->u.str) {
            zsfree(val_pm->u.str);
            val_pm->u.str = NULL;
        }
        if (is_loading) {
            val_pm->u.str = ztrdup("re-loaded");
        } else {
            val_pm->u.str = ztrdup("unloaded");
        }
    } else {
        val_pm = (Param) zshcalloc(sizeof (*val_pm));
        val_pm->node.flags = PM_SCALAR | PM_HASHELEM;
        val_pm->gsu.s = &stdscalar_gsu;
        if (is_loading) {
            val_pm->u.str = ztrdup("loaded");
        } else {
            val_pm->u.str = ztrdup("unloaded");
        }
        addhashnode(ht, ztrdup(id), val_pm); /* sets pm->node.nam */
    }
}

/**/
void
zsh_db_register_backend(char *id, void *entry_point) {
    BackendNode bn = (BackendNode)zshcalloc(sizeof(struct backend_node));
    if (bn) {
        bn->main_entry = entry_point;
        addhashnode(backends_hash, ztrdup(id), (void *)bn);
    } else {
        zwarn("out of memory when allocating backend entry");
        return;
    }

    update_user_hash(id, 1);
}

/**/
void
zsh_db_unregister_backend(char *id) {
    HashNode bn = backends_hash->removenode(backends_hash, id);
    if (bn) {
        freebackendnode(bn);
    }
    update_user_hash(id, 0);
}

/**/
static int
bin_ztie(char *nam, char **args, Options ops, UNUSED(int func))
{
    char *pmname;
    char *address = NULL, *pass = NULL, *pfile = NULL, *lazy = NULL;
    int rdonly = 0, zcache = 0, pprompt = 0;

    /* Check options */

    if (OPT_ISSET(ops,'h')) {
        ztie_usage();
        return 0;
    }

    if (!OPT_ISSET(ops,'d')) {
        zwarnnam(nam, "you must pass e.g. `-d db/redis', see `-h'");
        return 1;
    }

    if (!OPT_ISSET(ops,'f') && !OPT_ISSET(ops,'a')) {
        zwarnnam(nam, "you must pass `-f' (file, or `-a', address) with e.g. {host}[:port][/[db_idx][/key]], see `-h'");
        return 1;
    }

    if (0 != strcmp(OPT_ARG(ops, 'd'), "db/redis") && 0 != strcmp(OPT_ARG(ops, 'd'), "db/gdbm") ) {
        zwarnnam(nam, "Unsupported backend type `%s', see `-h'", OPT_ARG(ops, 'd'));
        return 1;
    }

    /* Check argument */
    pmname = *args;

    if (!pmname) {
        zwarnnam(nam, "You must pass non-option argument - the target parameter to create, see -h");
        return 1;
    }

    /* Address */
    if (OPT_ISSET(ops,'f')) {
        address = OPT_ARG(ops,'f');
    } else {
        address = OPT_ARG(ops,'a');
    }

    /* Read-only */
    if (OPT_ISSET(ops,'r')) {
        rdonly = 1;
    } else {
        rdonly = 0;
    }

    /* Zero-cache */
    if (OPT_ISSET(ops,'z')) {
        zcache = 1;
    } else {
        zcache = 0;
    }

    /* Password */
    if (OPT_ISSET(ops,'p')) {
        pass = OPT_ARG(ops,'p');
    }

    /* Password file */
    if (OPT_ISSET(ops,'P')) {
        pfile = OPT_ARG(ops,'P');
    }

    /* Password load request */
    if (OPT_ISSET(ops,'l')) {
        pprompt = 1;
    } else {
        pprompt = 0;
    }

    /* Lazy binding */
    if (OPT_ISSET(ops,'L')) {
        lazy = OPT_ARG(ops,'L');
    }

    BackendNode node = NULL;
    DbBackendEntryPoint be = NULL;

    if(!(node = (BackendNode) gethashnode2(backends_hash, OPT_ARG(ops, 'd')))) {
        zwarnnam(nam, "backend module for %s not loaded (or loaded before the main `db' module)", OPT_ARG(ops, 'd'));
        return 1;
    }

    be = node->main_entry;
    if (!be) {
        zwarnnam(nam, "backend for %s is uninitialized", OPT_ARG(ops, 'd'));
        return 1;
    }

    return be(DB_TIE, address, rdonly, zcache, pass, pfile, pprompt, pmname, lazy);
}

/**/
static int
bin_zuntie(char *nam, char **args, Options ops, UNUSED(int func))
{
    char *pmname;
    int ret = 0, rountie = 0;

    if (OPT_ISSET(ops,'h')) {
        zuntie_usage();
        return 0;
    }

    if (!*args) {
        zwarnnam(nam, "at least one variable name is needed, see -h");
        return 1;
    }

    for (pmname = *args; *args++; pmname = *args) {
        In_ParamName = pmname;
        Out_FoundBe = NULL;

        scanhashtable(backends_hash, 0, 0, 0, backend_scan_fun, 0);

        if (!Out_FoundBe) {
            zwarnnam(nam, "didn't recognize `%s' as a tied parameter", pmname);
            continue;
        }

        if (OPT_ISSET(ops,'u')) {
            rountie = 1;
        }

        ret = Out_FoundBe(DB_UNTIE, rountie, pmname) ? 1 : ret;
    }

    return ret;
}

/**/
static int
bin_ztaddress(char *nam, char **args, Options ops, UNUSED(int func))
{
    char *pmname;
    int ret = 0;

    if (OPT_ISSET(ops,'h')) {
        ztaddress_usage();
        return 0;
    }

    if (!*args) {
        zwarnnam(nam, "one parameter name is needed, see -h");
        return 1;
    }

    pmname = *args;
    In_ParamName = pmname;
    Out_FoundBe = NULL;

    scanhashtable(backends_hash, 0, 0, 0, backend_scan_fun, 0);

    if (!Out_FoundBe) {
        zwarnnam(nam, "didn't recognize `%s' as a tied parameter", pmname);
        return 1;
    }

    ret = Out_FoundBe(DB_GET_ADDRESS, pmname) ? 1 : ret;

    return ret;
}

/**/
static int
bin_ztclear(char *nam, char **args, Options ops, UNUSED(int func))
{
    char *pmname, *key;
    int ret = 0;

    if (OPT_ISSET(ops,'h')) {
        ztclear_usage();
        return 0;
    }

    if (!*args) {
        zwarnnam(nam, "one-to-two parameters' names are needed, see -h");
        return 1;
    }

    pmname = *args;
    key = *(args+1);

    In_ParamName = pmname;
    Out_FoundBe = NULL;

    scanhashtable(backends_hash, 0, 0, 0, backend_scan_fun, 0);

    if (!Out_FoundBe) {
        zwarnnam(nam, "didn't recognize `%s' as a tied parameter", pmname);
        return 1;
    }

    ret = Out_FoundBe(DB_CLEAR_CACHE, pmname, key) ? 1 : ret;

    return ret;
}

/*************** MAIN CODE ***************/

static struct features module_features =
{
    bintab, sizeof(bintab)/sizeof(*bintab),
    NULL, 0,
    NULL, 0,
    NULL, 0,
    0
};

/**/
int
setup_(UNUSED(Module m))
{
    Param pm  = NULL;

    /* Create private registration hash */
    if (!(backends_hash = createhashtable("ZSH_BACKENDS"))) {
        zwarn("cannot create backend-register hash");
        return 1;
    }

    /* Unset zdb_backends if it exists. Inherited from db_gdbm,
     * it clears current scope, leaving any upper scope untouched,
     * which can result in zdb_backends being local. Can be seen
     * as a feature */
    if ((pm = (Param)paramtab->getnode(paramtab, "zdb_backends")) && !(pm->node.flags & PM_UNSET)) {
        pm->node.flags &= ~PM_READONLY;
        if (unsetparam_pm(pm, 0, 1)) {
            zwarn("cannot properly manage scoping of variables");
            return 1;
        }
    }

    /* Create zdb_backends public hash that will hold state of backends */
    pm = createhashparam("zdb_backends", PM_READONLY | PM_REMOVABLE);
    if (NULL == pm) {
        zwarn("cannot create user backends-list hash parameter");
        if (backends_hash) {
            deletehashtable(backends_hash);
            backends_hash = NULL;
        }
        return 1;
    }
    pm->gsu.h = &stdhash_gsu;

    return 0;
}

/**/
int
features_(Module m, char ***features)
{
    *features = featuresarray(m, &module_features);
    return 0;
}

/**/
int
enables_(Module m, int **enables)
{
    return handlefeatures(m, &module_features, enables);
}

/**/
int
boot_(UNUSED(Module m))
{
    return 0;
}

/**/
int
cleanup_(Module m)
{
    /* This frees `zredis_tied` */
    return setfeatureenables(m, &module_features, NULL);

    if (backends_hash) {
        deletehashtable(backends_hash);
        backends_hash = NULL;
    }
}

/**/
int
finish_(UNUSED(Module m))
{
    return 0;
}
/* }}} */

/*************** UTILITIES ***************/

static HashTable
createhashtable(char *name)
{
    HashTable ht;

    ht = newhashtable(8, name, NULL);

    ht->hash        = hasher;
    ht->emptytable  = emptyhashtable;
    ht->filltable   = NULL;
    ht->cmpnodes    = strcmp;
    ht->addnode     = addhashnode;
    ht->getnode     = gethashnode2;
    ht->getnode2    = gethashnode2;
    ht->removenode  = removehashnode;
    ht->disablenode = NULL;
    ht->enablenode  = NULL;
    ht->freenode    = freebackendnode;
    ht->printnode   = NULL;

    return ht;
}

static Param
createhashparam(char *name, int flags)
{
    Param pm;
    HashTable ht;

    pm = createparam(name, flags | PM_SPECIAL | PM_HASHED);
    if (!pm) {
        return NULL;
    }

    if (pm->old)
        pm->level = locallevel;

    /* This creates standard hash. */
    ht = pm->u.hash = newparamtable(7, name);
    if (!pm->u.hash) {
        paramtab->removenode(paramtab, name);
        paramtab->freenode(&pm->node);
        zwarnnam(name, "out of memory when allocating user-visible hash of backends");
        return NULL;
    }

    /* Does free Param (unsetfn is called) */
    ht->freenode = zsh_db_freeparamnode;

    return pm;
}

static void
freebackendnode(HashNode hn)
{
    zsfree(hn->nam);
    zfree(hn, sizeof(struct backend_node));
}

static void
backend_scan_fun(HashNode hn, int unused)
{
    BackendNode bn = (BackendNode)hn;
    DbBackendEntryPoint be = bn->main_entry;
    if (!be) {
        zwarn("backend %s registered but uninitialized", hn->nam);
        return;
    }
    /* 0 - shell true value */
    if(0 == be(DB_IS_TIED, In_ParamName)) {
        Out_FoundBe = be;
    }
}

/*********** SHARED UTILITIES ***********/

/*
 * Unmetafy that:
 * - duplicates bufer to work on it,
 * - does zalloc of exact size for the new string,
 * - restores work buffer to original content, to restore strlen
 *
 * No zsfree()-confusing string will be produced.
 */

/**/
char *
zsh_db_unmetafy_zalloc(const char *to_copy, int *new_len)
{
    char *work, *to_return;
    int my_new_len = 0;

    work = ztrdup(to_copy);
    work = unmetafy(work, &my_new_len);

    if (new_len)
        *new_len = my_new_len;

    /* This string can be correctly zsfree()-d */
    to_return = (char *) zalloc((my_new_len+1)*sizeof(char));
    memcpy(to_return, work, sizeof(char)*my_new_len); /* memcpy handles $'\0' */
    to_return[my_new_len]='\0';

    /* Restore original strlen and correctly free */
    strcpy(work, to_copy);
    zsfree(work);

    return to_return;
}

/**/
void
zsh_db_standarize_hash(Param pm) {
    if (0 == (pm->node.flags & PM_HASHED)) {
        return;
    }

    pm->node.flags &= ~(PM_SPECIAL|PM_READONLY);
    pm->gsu.h = &stdhash_gsu;

    HashTable ht = pm->u.hash;

    ht->hash        = hasher;
    ht->emptytable  = emptyhashtable;
    ht->filltable   = NULL;
    ht->cmpnodes    = strcmp;
    ht->addnode     = addhashnode;
    ht->getnode     = gethashnode;
    ht->getnode2    = gethashnode2;
    ht->removenode  = removehashnode;
    ht->disablenode = NULL;
    ht->enablenode  = NULL;
    ht->freenode    = zsh_db_freeparamnode;
}

/*
 * Adds parameter name (to given `*_tied` array)
 */

/**/
int
zsh_db_arr_append(char ***arr, const char *input_s)
{
    int old_len = arrlen(*arr);
    char **new_arr = zshcalloc( (old_len+2) * sizeof(char *));

    /* Copy */
    char **p = *arr;
    char **dst = new_arr;
    while (*p) {
        *dst++ = *p++;
    }

    /* Append new one */
    *dst = ztrdup(input_s);

    /* Substitute, free old one */
    zfree(*arr, sizeof(char *) * (old_len + 1));
    *arr = new_arr;

    return 0;
}

/*
 * Removes parameter name (from given `*_tied` array)
 */

/**/
int
zsh_db_filter_arr(char ***arr, const char *input_s)
{
    int old_len = arrlen(*arr), new_len;
    char **new_arr, **dst;

    /* Two stage, to always have arrlen() == zfree-size - 1.
     * Could do allocation and revert when `not found`, but
     * what would be better about that. */

    /* Find one to remove */
    char **p = *arr;
    while (*p) {
        if (0==strcmp(input_s,*p)) {
            break;
        }
        p++;
    }

    /* Copy x+1 to x */
    while (*p) {
        *p=*(p+1);
        p++;
    }

    /* Second stage. Size changed? Only old_size-1
     * change is possible, but.. paranoia way */
    new_len = arrlen(*arr);
    if (new_len != old_len) {
        new_arr = (char **) zshcalloc((new_len+1) * sizeof(char *));

        /* Copy */
        p = *arr;
        dst = new_arr;
        while (*p) {
            *dst++ = *p++;
        }
        *dst = NULL;

        /* Substitute, free old one */
        zfree(*arr, sizeof(char *) * (old_len + 1));
        *arr = new_arr;
    }

    return 0;
}

/**/
void
zsh_db_freeparamnode(HashNode hn)
{
    Param pm = (Param) hn;

    /* Upstream: The second argument of unsetfn() is used by modules to
     * differentiate "exp"licit unset from implicit unset, as when
     * a parameter is going out of scope.  It's not clear which
     * of these applies here, but passing 1 has always worked.
     */

    /* if (delunset) */
    pm->gsu.s->unsetfn(pm, 1);

    zsfree(pm->node.nam);
    /* If this variable was tied by the user, ename was ztrdup'd */
    if (pm->node.flags & PM_TIED && pm->ename) {
        zsfree(pm->ename);
        pm->ename = NULL;
    }
    zfree(pm, sizeof(struct param));
}

/***************** USAGE *****************/

static void
ztie_usage()
{
    fprintf(stdout, "Usage: ztie -d db/... [-z] [-r] [-p password] [-P password_file] [-L type]"
            "-f/-a {db_address} {parameter_name}\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, " -d:       select database type: \"db/gdbm\", \"db/redis\"\n");
    fprintf(stdout, " -z:       zero-cache for read operations (always access database)\n");
    fprintf(stdout, " -r:       create read-only parameter\n" );
    fprintf(stdout, " -f or -a: database-address in format {host}[:port][/[db_idx][/key]] or a file path\n");
    fprintf(stdout, " -p:       database-password to be used for authentication\n");
    fprintf(stdout, " -P:       path to file with database-password\n");
    fprintf(stdout, " -L:       lazy binding - provide type of key to create if it doesn't exist "
                    "(string, set, zset, hash, list)\n");
    fprintf(stdout, "The {parameter_name} - choose name for the created database-bound parameter\n");
    fflush(stdout);
}

static void
zuntie_usage()
{
    fprintf(stdout, "Usage: zuntie [-u] {tied-variable-name} [tied-variable-name] ...\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, " -u: Allow to untie read-only parameter\n");
    fprintf(stdout, "Description: detaches variable from its database and removes the variable;\n");
    fprintf(stdout, "             database is not cleared (unlike when unset)\n");
    fflush(stdout);
}

static void
ztaddress_usage()
{
    fprintf(stdout, "Usage: ztaddress {tied-parameter-name}\n");
    fprintf(stdout, "Description: stores address used by given parameter to $REPLY\n");
    fflush(stdout);
}

static void
ztclear_usage()
{
    fprintf(stdout, "Usage: ztclear {tied-parameter-name} [key name]\n");
    fprintf(stdout, "Description: clears cache of given hash/key or of given plain\n");
    fprintf(stdout, "             parameter: set (array), list (array), string (scalar);\n");
    fprintf(stdout, "             pass `-z' to ztie to globally disable cache for parameter\n");
    fflush(stdout);
}
