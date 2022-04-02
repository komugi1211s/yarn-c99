#include "yarn_spinner.pb-c.h"

#define YARN_C99_IMPLEMENTATION
#include "yarn_c99.h"
#include "utest.h"


typedef struct {
    int a;
    int b;
    int c;
} for_kvmap;

typedef struct {
    const char *key;
    for_kvmap value;
} for_kvmap_entry;

UTEST(kvmap, normal_usage) {
    yarn_kvmap kvmap = yarn_kvcreate(for_kvmap, 3);

    ASSERT_TRUE(kvmap.entries);
    ASSERT_TRUE(kvmap.used == 0);

    for_kvmap_entry e[] = {
        { "hello", { 1, 2, 3 } },
        { "world", { 4, 5, 6 } },
        { "foo", { 10, 20, 30 } },
        { "bar", { 40, 50, 60 } },
        { "baz", { 100, 200, 300 } },
    };

    for (int i = 0; i < YARN_LEN(e); ++i) {
        yarn_kvpush(&kvmap, e[i].key, e[i].value);
    }
    ASSERT_TRUE(kvmap.used == 5);
    ASSERT_TRUE(kvmap.capacity == 3 * 2);

    for (int i = 0; i < YARN_LEN(e); ++i) {
        ASSERT_TRUE(yarn_kvhas(&kvmap, e[i].key));
    }

    for_kvmap m = {0};
    for (int i = 0; i < YARN_LEN(e); ++i) {
        ASSERT_TRUE(yarn_kvget(&kvmap, e[i].key, &m) != -1);
        ASSERT_TRUE(m.a == e[i].value.a);
        ASSERT_TRUE(m.b == e[i].value.b);
        ASSERT_TRUE(m.c == e[i].value.c);
    }

    for (int i = 0; i < YARN_LEN(e); ++i) {
        ASSERT_TRUE(yarn_kvdelete(&kvmap, e[i].key) != -1);
    }

    ASSERT_TRUE(kvmap.used == 0);
    yarn_kvdestroy(&kvmap);
}


struct TableF {
    yarn_string_table *t;
};

UTEST_F_SETUP(TableF) {
    utest_fixture->t = yarn_create_string_table();
}

UTEST_F_TEARDOWN(TableF) {
    yarn_destroy_string_table(utest_fixture->t);
}

UTEST_F(TableF, success_normal) {
    const char safe[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,0\r\nline:b,\"hey hello there\",testfile,Start,11";
    const char safe2[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,0\nline:b,\"hey hello there\",testfile,Start,2147483647\n"; /* barely non-overflow, with linebreak at the end */
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
}
UTEST_F(TableF, success_with_crlf) {
    const char safe[] = "id,text,file,node,lineNumber\r\nline:a,\"hey hello there\",testfile,Start,2147483647";
    const char safe2[] = "id,text,file,node,lineNumber\r\nline:a,\"hey hello there\",testfile,Start,2147483647\r\n";
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
}
UTEST_F(TableF, success_empty_table) {
    const char safe[] = "id,text,file,node,lineNumber";
    const char safe2[] = "id,text,file,node,lineNumber\n";
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
}

UTEST_F(TableF, should_completely_overflow) {
    const char overflower[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,1000000000000000000000000000000";
    ASSERT_FALSE(yarn__load_string_table(utest_fixture->t, (char*)overflower, sizeof(overflower)));
}

UTEST_F(TableF, should_BARELY_overflow) {
    const char overflower[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,2147483648"; /* INT_MAX+1 */
    ASSERT_FALSE(yarn__load_string_table(utest_fixture->t, (char*)overflower, sizeof(overflower)));
}

UTEST_MAIN();
