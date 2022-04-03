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

struct Populated_KVMap {
    yarn_kvmap kvmap;
};

static for_kvmap_entry premade_for_fixture[] = {
    { "hello", { 1, 2, 3 } },
    { "world", { 4, 5, 6 } },
    { "foo", { 10, 20, 30 } },
    { "bar", { 40, 50, 60 } },
    { "baz", { 100, 200, 300 } },
    { "ello", { 10, 20, 30 } },
    { "belbelbe", { 40, 50, 60 } },
    { "cinco de mayo", { 100, 200, 300 } },
    { "hello sailor", { 110, 210, 310 } },
    { "tokyo", { 120, 220, 320 } },
    { "new york", { 130, 230, 330 } },
    { "ivory coast", { 140, 240, 340 } },
};

UTEST_F_SETUP(Populated_KVMap) {
    utest_fixture->kvmap = yarn_kvcreate(for_kvmap, 4);

    for (int i = 0; i < YARN_LEN(premade_for_fixture); ++i) {
        yarn_kvpush(&utest_fixture->kvmap, (const char *)premade_for_fixture[i].key, premade_for_fixture[i].value);
    }

    EXPECT_EQ(utest_fixture->kvmap.used, YARN_LEN(premade_for_fixture));

     /* this line should affect the capacity check directly below;
      * kvmap began with 4 and inserted 12 elements, capacity doubles each time when it hits threshold of (used > cap * 0.7)
      * so it should be 16 or 32 */
    EXPECT_EQ(YARN_LEN(premade_for_fixture), 12);
    EXPECT_GE(utest_fixture->kvmap.capacity, 16);
}

UTEST_F_TEARDOWN(Populated_KVMap) {
    yarn_kvdestroy(&utest_fixture->kvmap);
}

UTEST_F(Populated_KVMap, basic_operation) {
    size_t used = utest_fixture->kvmap.used;

    for_kvmap value = {0, 1, 2};
    yarn_kvpush(&utest_fixture->kvmap, "hellope!", value);

    EXPECT_EQ(used + 1, utest_fixture->kvmap.used);

    for_kvmap value0;
    int exists_or_negativeone = yarn_kvget(&utest_fixture->kvmap, "hellope!", &value0);

    EXPECT_NE(exists_or_negativeone, -1);
    EXPECT_EQ(value0.a, value.a);
    EXPECT_EQ(value0.b, value.b);
    EXPECT_EQ(value0.c, value.c);

    yarn_kvdelete(&utest_fixture->kvmap, "hellope!");
    EXPECT_EQ(used, utest_fixture->kvmap.used);

    exists_or_negativeone = yarn_kvget(&utest_fixture->kvmap, "hellope!", &value0);
    EXPECT_EQ(exists_or_negativeone, -1);
}

UTEST_F(Populated_KVMap, should_be_able_to_iterate) { char *key;
    for_kvmap value = {0};
    int count = 0;

    EXPECT_EQ(utest_fixture->kvmap.used, YARN_LEN(premade_for_fixture));

    yarn_kvforeach(&utest_fixture->kvmap, &key, &value) {
        EXPECT_LT(count, YARN_LEN(premade_for_fixture));

        for_kvmap_entry *e = 0;
        for (int n = 0; n < YARN_LEN(premade_for_fixture); ++n) {
            if (strlen(key) == strlen(premade_for_fixture[n].key)) {
                if (strncmp(key, premade_for_fixture[n].key, strlen(key)) == 0) {
                    e = &premade_for_fixture[n];
                     break;
                }
            }
        }

        ASSERT_TRUE(e);
        EXPECT_EQ(value.a, e->value.a);
        EXPECT_EQ(value.b, e->value.b);
        EXPECT_EQ(value.c, e->value.c);
        count += 1;
    }

    EXPECT_EQ(count, YARN_LEN(premade_for_fixture));
}

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

struct CSVParsing {
    yarn_string_table *t;
};

UTEST_F_SETUP(CSVParsing) {
    utest_fixture->t = yarn_create_string_table();
}

UTEST_F_TEARDOWN(CSVParsing) {
    yarn_destroy_string_table(utest_fixture->t);
}

UTEST_F(CSVParsing, success_normal) {
    const char safe[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,0\r\nline:b,\"hey hello there\",testfile,Start,11";
    const char safe2[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,0\nline:b,\"hey hello there\",testfile,Start,2147483647\n"; /* barely non-overflow, with linebreak at the end */
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    EXPECT_EQ(utest_fixture->t->table.used, 2);

    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
    EXPECT_EQ(utest_fixture->t->table.used, 2); /* Overlapping ID. should be overwriting */
}

UTEST_F(CSVParsing, success_with_crlf) {
    const char safe[] = "id,text,file,node,lineNumber\r\nline:a,\"hey hello there\",testfile,Start,2147483647";
    const char safe2[] = "id,text,file,node,lineNumber\r\nline:a,\"hey hello there\",testfile,Start,2147483647\r\n";
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
}

UTEST_F(CSVParsing, success_empty_table) {
    const char safe[] = "id,text,file,node,lineNumber";
    const char safe2[] = "id,text,file,node,lineNumber\n";
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe, sizeof(safe)));
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)safe2, sizeof(safe2)));
}

UTEST_F(CSVParsing, should_completely_overflow) {
    const char overflower[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,1000000000000000000000000000000";
    ASSERT_FALSE(yarn__load_string_table(utest_fixture->t, (char*)overflower, sizeof(overflower)));
}

UTEST_F(CSVParsing, should_BARELY_overflow) {
    const char overflower[] = "id,text,file,node,lineNumber\nline:a,\"hey hello there\",testfile,Start,2147483648"; /* INT_MAX+1 */
    ASSERT_FALSE(yarn__load_string_table(utest_fixture->t, (char*)overflower, sizeof(overflower)));
}

UTEST_F(CSVParsing, removing_double_quotation) {
    const char double_quotes[] = "id,text,file,node,lineNumber\nline:a,\"hey \"\"hello\"\" there\",testfile,Start,10";
    ASSERT_TRUE(yarn__load_string_table(utest_fixture->t, (char*)double_quotes, sizeof(double_quotes)));

    yarn_parsed_entry parsed = {0};
    EXPECT_NE(yarn_kvget(&utest_fixture->t->table, "line:a", &parsed), -1);
    EXPECT_STREQ(parsed.text, "hey \"hello\" there");
    EXPECT_EQ(strlen(parsed.text), sizeof("hey \"hello\" there") - 1);
}


UTEST_MAIN();
