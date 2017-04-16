import platform

if platform.system() == 'SunOS':
   statsite_with_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -Wall -Wstrict-aliasing=0 -Wformat=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
   statsite_without_err_cc_flags = '-g -std=gnu99 -D_GNU_SOURCE -DLOG_PERROR=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
else:
   statsite_with_err_cc_flags = '-g -std=c99 -D_GNU_SOURCE -Wall -Werror -Wstrict-aliasing=0 -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'
   statsite_without_err_cc_flags = '-g -std=c99 -D_GNU_SOURCE -O3 -pthread -Ideps/inih/ -Ideps/ae/ -Isrc/'

envmurmur = Environment(CPATH = ['deps/murmurhash/'], CFLAGS="-std=c99 -O3")
murmur = envmurmur.Library('murmur', Glob("deps/murmurhash/*.c"))

envinih = Environment(CPATH = ['deps/inih/'], CFLAGS="-O3")
inih = envinih.Library('inih', Glob("deps/inih/*.c"))

envae = Environment(CPATH = ['deps/ae/'], CFLAGS="-O3")
ae = envae.Library('ae', Glob("deps/ae/ae.c"))

env_statsite_with_err = Environment(CCFLAGS = statsite_with_err_cc_flags)
env_statsite_without_err = Environment(CCFLAGS = statsite_without_err_cc_flags)

if platform.system() == 'SunOS':
  for env in [envmurmur, envinih, env_statsite_with_err, env_statsite_without_err]:
    env.AppendENVPath('PATH', '/opt/local/bin')

if platform.system() == "Darwin":
  for env in [envmurmur, envinih, env_statsite_with_err, env_statsite_without_err]:
    env.Append(CCFLAGS= ' -I/usr/local/include/')
    env.Append(LINKFLAGS= ' -L/usr/local/lib/')


instdir = ARGUMENTS.get('PREFIX')
if instdir == None:
   instdir = "/usr/local"
bin = '%s/bin' % instdir
etc = '/etc'
share = '%s/share/statsite' % instdir


env_install = Environment()

objs = env_statsite_with_err.Object('src/hashmap', 'src/hashmap.c')           + \
        env_statsite_with_err.Object('src/heap', 'src/heap.c')                + \
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
        env_statsite_without_err.Object('src/networking', 'src/networking.c') + \
        env_statsite_without_err.Object('src/conn_handler', 'src/conn_handler.c')

statsite_libs = ["m", "pthread", murmur, inih, ae]
if platform.system() == 'Linux':
   statsite_libs.append("rt")
elif platform.system() == 'SunOS':
   statsite_libs.append(["socket", "nsl"])

statsite = env_statsite_with_err.Program('statsite', objs + ["src/statsite.c"], LIBS=statsite_libs)
statsite_test = env_statsite_without_err.Program('test_runner', objs + Glob("tests/runner.c"), LIBS=statsite_libs + ["check"])

statsite_conf = env_statsite_with_err.File('src/statsite.conf')
binary_sink = env_install.File('sinks/binary_sink.py')
graphite_sink = env_install.File('sinks/graphite.py')
gmetric_sink = env_install.File('sinks/gmetric.py')
influxdb_sink = env_install.File('sinks/influxdb.py')
librato_sink = env_install.File('sinks/librato.py')
json_sink = env_install.File('sinks/statsite_json_sink.rb')

# By default, only compile statsite
Default(statsite)

env_install.Install(bin, statsite)
env_install.Install(share, [binary_sink,graphite_sink,gmetric_sink,influxdb_sink,librato_sink])
env_install.Install(etc, statsite_conf)
env_install.Alias('install', [bin, etc, share])
