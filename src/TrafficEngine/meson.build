project('my-dpdk-app', 'c')

dpdk = dependency('dpdk', method: 'pkg-config')

executable('dpdk-traffic-gen',
    'main.c',
    dependencies: dpdk,
    install: false)
