// .pak write + read + crc + split boundary (source/mge_pak.c).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "../source/mge_pak.h"

#if defined(_WIN32)
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
    #define RMDIR(p) _rmdir(p)
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #define MKDIR(p) mkdir(p, 0755)
    #define RMDIR(p) rmdir(p)
#endif

static void write_file(const char* path, const void* data, size_t n)
{
    FILE* f = fopen(path, "wb");
    fwrite(data, 1, n, f);
    fclose(f);
}

TEST(crc32_known_value)
{
    // CRC-32/ISO-HDLC of "123456789" is 0xCBF43926
    CHECK(Mge_Crc32("123456789", 9) == 0xCBF43926u);
    CHECK(Mge_Crc32("", 0) == 0u);
}

TEST(write_read_roundtrip_with_split)
{
    MKDIR("pak_tmp");
    MKDIR("pak_tmp/root");
    MKDIR("pak_tmp/root/sub");
    MKDIR("pak_tmp/root/build"); // must be skipped

    char big[5000];
    for (size_t i = 0; i < sizeof(big); i++)
        big[i] = (char)(i * 7 + 1);

    write_file("pak_tmp/root/hello.txt", "hello world", 11);
    write_file("pak_tmp/root/sub/data.bin", big, sizeof(big));
    write_file("pak_tmp/root/sub/tiny", "x", 1);
    write_file("pak_tmp/root/build/junk.o", "nope", 4);
    write_file("pak_tmp/root/skip.dll", "nope", 4);

    // tiny split so the 5000-byte file spans several physical files
    CHECK(Mge_PakWrite("pak_tmp/game", "pak_tmp/root", 2048));

    // .001 and at least .002 exist
    FILE* a = fopen("pak_tmp/game.pak.001", "rb");
    FILE* b = fopen("pak_tmp/game.pak.002", "rb");
    CHECK(a != NULL && b != NULL);
    if (a) fclose(a);
    if (b) fclose(b);

    Pak* pak = Mge_PakOpen("pak_tmp/game");
    CHECK(pak != NULL);

    CHECK(Mge_PakHas(pak, "hello.txt"));
    CHECK(Mge_PakHas(pak, "sub/data.bin"));
    CHECK(!Mge_PakHas(pak, "build/junk.o"));   // dir skipped
    CHECK(!Mge_PakHas(pak, "skip.dll"));       // ext skipped
    CHECK(!Mge_PakHas(pak, "nope.txt"));

    size_t n = 0;
    char* h = Mge_PakRead(pak, "hello.txt", &n);
    CHECK(h != NULL && n == 11 && memcmp(h, "hello world", 11) == 0);
    CHECK(h[11] == '\0'); // NUL past the size
    free(h);

    unsigned char* d = Mge_PakRead(pak, "sub/data.bin", &n);
    CHECK(d != NULL && n == sizeof(big) && memcmp(d, big, sizeof(big)) == 0);
    free(d);

    char* t = Mge_PakRead(pak, "sub/tiny", &n);
    CHECK(t != NULL && n == 1 && t[0] == 'x');
    free(t);

    CHECK(Mge_PakRead(pak, "does/not/exist", &n) == NULL);

    Mge_PakClose(pak);
}

TEST(mount_stack)
{
    // (pak_tmp/game.* from the previous test still on disk)
    CHECK(Mge_MountPak("pak_tmp/game"));
    size_t n = 0;
    void* d = Mge_MountedRead("hello.txt", &n);
    CHECK(d != NULL && n == 11);
    free(d);
    CHECK(Mge_MountedRead("missing", &n) == NULL);
    Mge_UnmountPaks();
    CHECK(Mge_MountedRead("hello.txt", &n) == NULL);

    CHECK(!Mge_MountPak("pak_tmp/nonexistent"));
}

int main(void)
{
    RUN(crc32_known_value);
    RUN(write_read_roundtrip_with_split);
    RUN(mount_stack);

    remove("pak_tmp/game.pak.001");
    remove("pak_tmp/game.pak.002");
    remove("pak_tmp/game.pak.003");
    remove("pak_tmp/root/hello.txt");
    remove("pak_tmp/root/sub/data.bin");
    remove("pak_tmp/root/sub/tiny");
    remove("pak_tmp/root/build/junk.o");
    remove("pak_tmp/root/skip.dll");
    RMDIR("pak_tmp/root/sub");
    RMDIR("pak_tmp/root/build");
    RMDIR("pak_tmp/root");
    RMDIR("pak_tmp");
    return test_summary();
}
