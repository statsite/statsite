import platform

envmurmur = Environment(CPPPATH = ['deps/murmurhash/'], CPPFLAGS="-fno-exceptions -O2")
murmur = envmurmur.Library('murmur', Glob("deps/murmurhash/*.cpp"))

envtest = Environment(CCFLAGS = '-std=c99 -D_GNU_SOURCE -Isrc/libbloom/')
envtest.Program('test_libbloom_runner', Glob("tests/libbloom/*.c"), LIBS=["check", "m", bloom, murmur, spooky])

envinih = Environment(CPATH = ['deps/inih/'], CFLAGS="-O2")
inih = envinih.Library('inih', Glob("deps/inih/*.c"))

envbloomd_with_err = Environment(CCFLAGS = '-std=c99 -D_GNU_SOURCE -Wall -Werror -O2 -pthread -Ideps/inih/ -Ideps/libev/ -Isrc/libbloom/')
envbloomd_without_err = Environment(CCFLAGS = '-std=c99 -D_GNU_SOURCE -O2 -pthread -Isrc/bloomd/ -Ideps/inih/ -Ideps/libev/ -Isrc/libbloom/')

objs =  envbloomd_with_err.Object('src/bloomd/config', 'src/bloomd/config.c') + \
        envbloomd_without_err.Object('src/bloomd/networking', 'src/bloomd/networking.c') + \
        envbloomd_with_err.Object('src/bloomd/conn_handler', 'src/bloomd/conn_handler.c') + \
        envbloomd_with_err.Object('src/bloomd/hashmap', 'src/bloomd/hashmap.c') + \
        envbloomd_with_err.Object('src/bloomd/filter', 'src/bloomd/filter.c') + \
        envbloomd_with_err.Object('src/bloomd/filter_manager', 'src/bloomd/filter_manager.c') + \
        envbloomd_with_err.Object('src/bloomd/background', 'src/bloomd/background.c')

bloom_libs = ["m", "pthread", murmur, bloom, inih, spooky]
if platform.system() == 'Linux':
   bloom_libs.append("rt")

bloomd = envbloomd_with_err.Program('bloomd', objs + ["src/bloomd/bloomd.c"], LIBS=bloom_libs)
bloomd_test = envbloomd_without_err.Program('test_bloomd_runner', objs + Glob("tests/bloomd/runner.c"), LIBS=bloom_libs + ["check"])

bench_obj = Object("bench", "bench.c", CCFLAGS="-std=c99 -O2")
Program('bench', bench_obj, LIBS=["pthread"])

# By default, only compile bloomd
Default(bloomd)
