import platform

envmurmur = Environment(CPPPATH = ['deps/murmurhash/'], CPPFLAGS="-fno-exceptions -O2")
murmur = envmurmur.Library('murmur', Glob("deps/murmurhash/*.cpp"))

envinih = Environment(CPATH = ['deps/inih/'], CFLAGS="-O2")
inih = envinih.Library('inih', Glob("deps/inih/*.c"))

env_statsite_with_err = Environment(CCFLAGS = '-std=c99 -D_GNU_SOURCE -Wall -Werror -O2 -pthread -Ideps/inih/ -Ideps/libev/ -Isrc/')
env_statsite_without_err = Environment(CCFLAGS = '-std=c99 -D_GNU_SOURCE -O2 -pthread -Isrc/bloomd/ -Ideps/inih/ -Ideps/libev/ -Isrc/')

objs = env_statsite_with_err.Object('src/hashmap', 'src/hashmap.c')

statsite_libs = ["m", "pthread", murmur, inih]
if platform.system() == 'Linux':
   statsite_libs.append("rt")

statsite = env_statsite_with_err.Program('statsite', objs + ["src/statsite.c"], LIBS=statsite_libs)
statsite_test = env_statsite_without_err.Program('test_runner', objs + Glob("tests/runner.c"), LIBS=statsite_libs + ["check"])

# By default, only compile statsite
Default(statsite)
