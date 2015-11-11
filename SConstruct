import os
from copy import copy
import platform

ENV = Environment()
ENV["CC"] = os.getenv("CC") or ENV["CC"]
ENV["CXX"] = os.getenv("CXX") or ENV["CXX"]
ENV["PATH"] = os.getenv("PATH") or ENV["PATH"]
ENV["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

if "CURL_LIB" in os.environ:
  curl_lib = os.environ["CURL_LIB"]
else:
  curl_lib = "curl"

CFLAGS = ["-Wall",
          "-Wno-unused-function",
          "-std=c99",
          "-fno-strict-aliasing",
          "-fno-omit-frame-pointer", # Allow for perf
          "-D_GNU_SOURCE",
          "-g3",
          "-O3",
          "-pthread",
          "-Ideps/inih", "-Ideps/libev", "-Isrc"]
CFLAGS_ERROR = copy(CFLAGS)
CFLAGS_ERROR.extend(["-Werror"])
CFLAGS_LIBEV = copy(CFLAGS)
CFLAGS_LIBEV.extend(["-Wno-strict-aliasing",
                     "-Wno-unused-value",
                     "-Wno-unused-variable",
                     "-Wno-comment",
                     "-Wno-parentheses"])

envmurmur = ENV.Clone(CPATH = ['deps/murmurhash/'], CFLAGS=" ".join(CFLAGS))
murmur = envmurmur.Library('murmur', Glob("deps/murmurhash/*.c"))

envinih = ENV.Clone(CPATH = ['deps/inih/'], CFLAGS= " ".join(CFLAGS))
inih = envinih.Library('inih', Glob("deps/inih/*.c"))

env_statsite_with_err = ENV.Clone(CFLAGS = " ".join(CFLAGS_ERROR))
env_statsite_without_err = ENV.Clone(CFLAGS = " ".join(CFLAGS))
env_statsite_libev = ENV.Clone(CFLAGS = " ".join(CFLAGS_LIBEV))

objs = env_statsite_with_err.Object('src/hashmap', 'src/hashmap.c')           + \
        env_statsite_with_err.Object('src/heap', 'src/heap.c')                + \
        env_statsite_with_err.Object('src/strbuf', 'src/strbuf.c')            + \
        env_statsite_with_err.Object('src/radix', 'src/radix.c')              + \
        env_statsite_with_err.Object('src/hll_constants', 'src/hll_constants.c') + \
        env_statsite_with_err.Object('src/hll', 'src/hll.c')                  + \
        env_statsite_with_err.Object('src/set', 'src/set.c')                  + \
        env_statsite_with_err.Object('src/cm_quantile', 'src/cm_quantile.c')  + \
        env_statsite_with_err.Object('src/timer', 'src/timer.c')              + \
        env_statsite_with_err.Object('src/counter', 'src/counter.c')          + \
        env_statsite_with_err.Object('src/metrics', 'src/metrics.c')          + \
        env_statsite_with_err.Object('src/streaming', 'src/streaming.c')      + \
        env_statsite_with_err.Object('src/config', 'src/config.c')            + \
        env_statsite_with_err.Object('src/circqueue', 'src/circqueue.c')      + \
        env_statsite_with_err.Object('src/sink', 'src/sink.c')                + \
        env_statsite_with_err.Object('src/sink_stream', 'src/sink_stream.c')  + \
        env_statsite_with_err.Object('src/lifoq', 'src/lifoq.c')              + \
        env_statsite_with_err.Object('src/sink_http', 'src/sink_http.c')      + \
        env_statsite_with_err.Object('src/utils', 'src/utils.c')                + \
        env_statsite_libev.Object('src/networking', 'src/networking.c')       + \
        env_statsite_libev.Object('src/conn_handler', 'src/conn_handler.c')

statsite_libs = ["m", "pthread", murmur, inih, "jansson", curl_lib]
if platform.system() == 'Linux':
   statsite_libs.append("rt")

statsite = env_statsite_with_err.Program('statsite', objs + ["src/statsite.c"], LIBS=statsite_libs)
statsite_test = env_statsite_without_err.Program('test_runner', objs + Glob("tests/runner.c"), LIBS=statsite_libs + ["check"])

# By default, only compile statsite
Default(statsite)
