# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2017 Intel Corporation.
# Copyright(c) 2017 Cavium, Inc
# Copyright(c) 2021 PANTHEON.tech s.r.o.

# common flags to all aarch64 builds, with lowest priority
flags_common = [
        # Accelerate rte_memcpy. Be sure to run unit test (memcpy_perf_autotest)
        # to determine the best threshold in code. Refer to notes in source file
        # (lib/eal/arm/include/rte_memcpy_64.h) for more info.
        ['RTE_ARCH_ARM64_MEMCPY', false],
        #    ['RTE_ARM64_MEMCPY_ALIGNED_THRESHOLD', 2048],
        #    ['RTE_ARM64_MEMCPY_UNALIGNED_THRESHOLD', 512],
        # Leave below RTE_ARM64_MEMCPY_xxx options commented out,
        # unless there are strong reasons.
        #    ['RTE_ARM64_MEMCPY_SKIP_GCC_VER_CHECK', false],
        #    ['RTE_ARM64_MEMCPY_ALIGN_MASK', 0xF],
        #    ['RTE_ARM64_MEMCPY_STRICT_ALIGN', false],

        # Enable use of ARM wait for event instruction.
        # ['RTE_ARM_USE_WFE', false],

        ['RTE_ARCH_ARM64', true],
        ['RTE_CACHE_LINE_SIZE', 128]
]

## Part numbers are specific to Arm implementers
# implementer specific armv8 flags have middle priority
#     (will overwrite common flags)
# part number specific armv8 flags have higher priority
#     (will overwrite both common and implementer specific flags)
implementer_generic = {
    'description': 'Generic armv8',
    'flags': [
        ['RTE_MACHINE', '"armv8a"'],
        ['RTE_USE_C11_MEM_MODEL', true],
        ['RTE_MAX_LCORE', 256],
        ['RTE_MAX_NUMA_NODES', 4]
    ],
    'part_number_config': {
        'generic': {
            'march': 'armv8-a',
            'march_features': ['crc'],
            'compiler_options': ['-moutline-atomics']
        },
        'generic_aarch32': {
            'march': 'armv8-a',
            'force_march': true,
            'march_features': ['simd'],
            'compiler_options': ['-mfpu=auto'],
            'flags': [
                ['RTE_ARCH_ARM_NEON_MEMCPY', false],
                ['RTE_ARCH_STRICT_ALIGN', true],
                ['RTE_ARCH_ARMv8_AARCH32', true],
                ['RTE_ARCH', 'armv8_aarch32'],
                ['RTE_CACHE_LINE_SIZE', 64]
            ]
        }
    }
}

part_number_config_arm = {
    '0xd03': {'mcpu': 'cortex-a53'},
    '0xd04': {'mcpu': 'cortex-a35'},
    '0xd05': {'mcpu': 'cortex-a55'},
    '0xd07': {'mcpu': 'cortex-a57'},
    '0xd08': {'mcpu': 'cortex-a72'},
    '0xd09': {'mcpu': 'cortex-a73'},
    '0xd0a': {'mcpu': 'cortex-a75'},
    '0xd0b': {'mcpu': 'cortex-a76'},
    '0xd0c': {
        'march': 'armv8.2-a',
        'march_features': ['crypto', 'rcpc'],
        'mcpu': 'neoverse-n1',
        'flags': [
            ['RTE_MACHINE', '"neoverse-n1"'],
            ['RTE_ARM_FEATURE_ATOMICS', true],
            ['RTE_MAX_MEM_MB', 1048576],
            ['RTE_MAX_LCORE', 256],
            ['RTE_MAX_NUMA_NODES', 8]
        ]
    },
    '0xd40': {
        'march': 'armv8.4-a',
        'march_features': ['sve'],
        'mcpu': 'neoverse-v1',
        'flags': [
            ['RTE_MACHINE', '"neoverse-v1"'],
            ['RTE_ARM_FEATURE_ATOMICS', true],
            ['RTE_MAX_NUMA_NODES', 1]
        ]
    },
    '0xd42': {
        'march': 'armv8.4-a',
    },
    '0xd49': {
        'march': 'armv9-a',
        'march_features': ['sve2'],
        'fallback_march': 'armv8.5-a',
        'mcpu': 'neoverse-n2',
        'flags': [
            ['RTE_MACHINE', '"neoverse-n2"'],
            ['RTE_ARM_FEATURE_ATOMICS', true],
            ['RTE_MAX_LCORE', 128],
            ['RTE_MAX_NUMA_NODES', 2]
        ]
    },
    '0xd4f': {
        'march': 'armv9-a',
        'march_features': ['sve2'],
        'mcpu' : 'neoverse-v2',
        'fallback_march': 'armv8.5-a',
        'flags': [
            ['RTE_MACHINE', '"neoverse-v2"'],
            ['RTE_ARM_FEATURE_ATOMICS', true],
            ['RTE_MAX_LCORE', 144],
            ['RTE_MAX_NUMA_NODES', 2]
        ]
    },
    '0xd8e': {
        # Only when -march=armv9-a+wfxt is used will the WFET
        # feature be compiled with armv9 instructions.
        # However, +wfxt is not supported by GCC at the moment.
        # Although armv9-a is the fitting version of Arm ISA for
        # Neoverse N3, it cannot be used when enabling wfxt for
        # the above reasons.
        # The workaround for this is to use armv8.7-a, which
        # doesn't require +wfxt for binutils version 2.36 or greater.
        'march': 'armv8.7-a',
        'march_features': ['sve2'],
        'fallback_march': 'armv8.5-a',
        'flags': [
            ['RTE_MACHINE', '"neoverse-n3"'],
            ['RTE_ARM_FEATURE_ATOMICS', true],
            ['RTE_ARM_FEATURE_WFXT', true],
            ['RTE_MAX_LCORE', 192],
            ['RTE_MAX_NUMA_NODES', 2]
        ]
    }
}
implementer_arm = {
    'description': 'Arm',
    'flags': [
        ['RTE_MACHINE', '"armv8a"'],
        ['RTE_USE_C11_MEM_MODEL', true],
        ['RTE_CACHE_LINE_SIZE', 64],
        ['RTE_MAX_LCORE', 64],
        ['RTE_MAX_NUMA_NODES', 4]
    ],
    'part_number_config': part_number_config_arm
}

flags_part_number_thunderx = [
    ['RTE_MACHINE', '"thunderx"'],
    ['RTE_USE_C11_MEM_MODEL', false]
]
implementer_cavium = {
    'description': 'Cavium',
    'flags': [
        ['RTE_MAX_VFIO_GROUPS', 128],
        ['RTE_MAX_LCORE', 96],
        ['RTE_MAX_NUMA_NODES', 2]
    ],
    'part_number_config': {
        '0xa1': {
            'mcpu': 'thunderxt88',
            'flags': flags_part_number_thunderx
        },
        '0xa2': {
            'mcpu': 'thunderxt81',
            'flags': flags_part_number_thunderx
        },
        '0xa3': {
            'march': 'armv8-a',
            'march_features': ['crc', 'crypto'],
            'mcpu': 'thunderxt83',
            'flags': flags_part_number_thunderx
        },
        '0xaf': {
            'march': 'armv8.1-a',
            'march_features': ['crc', 'crypto'],
            'mcpu': 'thunderx2t99',
            'flags': [
                ['RTE_MACHINE', '"thunderx2"'],
                ['RTE_ARM_FEATURE_ATOMICS', true],
                ['RTE_USE_C11_MEM_MODEL', true],
                ['RTE_CACHE_LINE_SIZE', 64],
                ['RTE_MAX_LCORE', 256]
            ]
        },
        '0xb2': {
            'march': 'armv8.2-a',
            'march_features': ['crc', 'crypto', 'lse'],
            'mcpu': 'octeontx2',
            'flags': [
                ['RTE_MACHINE', '"cn9k"'],
                ['RTE_ARM_FEATURE_ATOMICS', true],
                ['RTE_USE_C11_MEM_MODEL', true],
                ['RTE_MAX_LCORE', 36],
                ['RTE_MAX_NUMA_NODES', 1]
            ]
        }
    }
}

implementer_ampere = {
    'description': 'Ampere Computing',
    'flags': [
        ['RTE_CACHE_LINE_SIZE', 64],
        ['RTE_MAX_LCORE', 1024],
        ['RTE_MAX_NUMA_NODES', 8]
    ],
    'part_number_config': {
        '0x0': {
            'march': 'armv8-a',
            'march_features': ['crc', 'crypto'],
            'mcpu': 'emag',
            'flags': [
                ['RTE_MACHINE', '"eMAG"'],
                ['RTE_MAX_LCORE', 32],
                ['RTE_MAX_NUMA_NODES', 1]
            ]
        },
        '0xac3': {
            'march': 'armv8.6-a',
            'march_features': ['crc', 'crypto'],
            'mcpu': 'ampere1',
            'flags': [
                ['RTE_MACHINE', '"AmpereOne"'],
                ['RTE_MAX_LCORE', 320],
                ['RTE_MAX_NUMA_NODES', 8]
            ]
        },
        '0xac4': {
            'march': 'armv8.6-a',
            'march_features': ['crc', 'crypto'],
            'mcpu': 'ampere1a',
            'flags': [
                ['RTE_MACHINE', '"AmpereOneAC04"'],
                ['RTE_MAX_LCORE', 384],
                ['RTE_MAX_NUMA_NODES', 8]
            ]
        }
    }
}

implementer_hisilicon = {
    'description': 'HiSilicon',
    'flags': [
        ['RTE_USE_C11_MEM_MODEL', true],
        ['RTE_CACHE_LINE_SIZE', 128]
    ],
    'part_number_config': {
        '0xd01': {
            'march': 'armv8.2-a',
            'march_features': ['crypto'],
            'mcpu': 'tsv110',
            'flags': [
                ['RTE_MACHINE', '"Kunpeng 920"'],
                ['RTE_ARM_FEATURE_ATOMICS', true],
                ['RTE_MAX_LCORE', 256],
                ['RTE_MAX_NUMA_NODES', 8]
            ]
        },
        '0xd02': {
            'march': 'armv8.2-a',
            'march_features': ['crypto', 'sve'],
            'flags': [
                ['RTE_MACHINE', '"Kunpeng 930"'],
                ['RTE_ARM_FEATURE_ATOMICS', true],
                ['RTE_MAX_LCORE', 1280],
                ['RTE_MAX_NUMA_NODES', 16]
            ]
        },
        '0xd03': {
            'march': 'armv8.5-a',
            'march_features': ['crypto', 'sve'],
            'flags': [
                ['RTE_MACHINE', '"hip10"'],
                ['RTE_ARM_FEATURE_ATOMICS', true],
                ['RTE_MAX_LCORE', 1280],
                ['RTE_MAX_NUMA_NODES', 16]
            ]
        }
    }
}

implementer_ionic = {
    'description': 'AMD Pensando',
    'flags': [
        ['RTE_MAX_NUMA_NODES', 1],
        ['RTE_CACHE_LINE_SIZE', 64],
        ['RTE_LIBRTE_VHOST_NUMA', false],
        ['RTE_EAL_NUMA_AWARE_HUGEPAGES', false],
        ['RTE_LIBRTE_IONIC_PMD_EMBEDDED', true],
    ],
    'part_number_config': {
        '0xc1': {
            'mcpu' : 'cortex-a72',
            'flags': [
                ['RTE_MAX_LCORE', 4],
                ['RTE_LIBRTE_IONIC_PMD_BARRIER_ERRATA', true],
            ]
        },
        '0xc2': {
            'mcpu' : 'cortex-a72',
            'flags': [
                ['RTE_MAX_LCORE', 16],
                ['RTE_LIBRTE_IONIC_PMD_BARRIER_ERRATA', true],
            ]
        }
    }
}

implementer_phytium = {
    'description': 'Phytium',
    'flags': [
        ['RTE_MACHINE', '"armv8a"'],
        ['RTE_USE_C11_MEM_MODEL', true],
        ['RTE_CACHE_LINE_SIZE', 64],
    ],
    'part_number_config': {
        '0x662': {
            'march': 'armv8-a',
            'march_features': ['crc'],
            'flags': [
                ['RTE_MAX_LCORE', 64],
                ['RTE_MAX_NUMA_NODES', 8]
             ]
        },
       '0x663': {
            'march': 'armv8-a',
            'march_features': ['crc'],
            'flags': [
                ['RTE_MAX_LCORE', 256],
                ['RTE_MAX_NUMA_NODES', 32]
            ]
        }
    }
}

implementer_qualcomm = {
    'description': 'Qualcomm',
    'flags': [
        ['RTE_MACHINE', '"armv8a"'],
        ['RTE_USE_C11_MEM_MODEL', true],
        ['RTE_CACHE_LINE_SIZE', 64],
        ['RTE_MAX_LCORE', 64],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'part_number_config': {
        '0x800': {
            'march': 'armv8-a',
            'march_features': ['crc']
        },
        '0xc00': {
            'march': 'armv8-a',
            'march_features': ['crc']
        }
    }
}

## Arm implementers (ID from MIDR in Arm Architecture Reference Manual)
implementers = {
    'generic': implementer_generic,
    '0x41': implementer_arm,
    '0x43': implementer_cavium,
    '0x48': implementer_hisilicon,
    '0x50': implementer_ampere,
    '0x51': implementer_qualcomm,
    '0x70': implementer_phytium,
    '0x75': implementer_ionic,
    '0xc0': implementer_ampere,
}

# SoC specific armv8 flags have the highest priority
#     (will overwrite all other flags)
soc_generic = {
    'description': 'Generic un-optimized build for armv8 aarch64 exec mode',
    'implementer': 'generic',
    'part_number': 'generic'
}

soc_generic_aarch32 = {
    'description': 'Generic un-optimized build for armv8 aarch32 exec mode',
    'implementer': 'generic',
    'part_number': 'generic_aarch32'
}

soc_altra = {
    'description': 'Ampere Altra/AltraMax',
    'implementer': '0x41',
    'part_number': '0xd0c',
    'numa': true
}

soc_ampereone = {
    'description': 'Ampere AmpereOne',
    'implementer': '0xc0',
    'part_number': '0xac3',
    'numa': true
}

soc_ampereoneac04 = {
    'description': 'Ampere AmpereOne AC04',
    'implementer': '0xc0',
    'part_number': '0xac4',
    'numa': true
}

soc_armada = {
    'description': 'Marvell ARMADA',
    'implementer': '0x41',
    'part_number': '0xd08',
    'flags': [
        ['RTE_MAX_LCORE', 16],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'numa': false
}

soc_bluefield = {
    'description': 'NVIDIA BlueField',
    'implementer': '0x41',
    'part_number': '0xd08',
    'flags': [
        ['RTE_MAX_LCORE', 16],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'numa': false
}

soc_capri = {
    'description': 'AMD Pensando Capri',
    'implementer': '0x75',
    'part_number': '0xc1'
}

soc_cdx = {
    'description': 'AMD CDX',
    'implementer': '0x41',
    'part_number': '0xd42',
    'flags': [
        ['RTE_MACHINE', '"cdx"'],
        ['RTE_MAX_LCORE', 16],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'numa': false
}

soc_centriq2400 = {
    'description': 'Qualcomm Centriq 2400',
    'implementer': '0x51',
    'part_number': '0xc00',
    'numa': false
}

soc_cn9k = {
    'description': 'Marvell OCTEON 9',
    'implementer': '0x43',
    'part_number': '0xb2',
    'numa': false,
}

soc_cn10k = {
    'description' : 'Marvell OCTEON 10',
    'implementer' : '0x41',
    'flags': [
        ['RTE_MAX_LCORE', 24],
        ['RTE_MAX_NUMA_NODES', 1],
        ['RTE_MEMPOOL_ALIGN', 128],
    ],
    'part_number': '0xd49',
    'extra_march_features': ['crypto'],
    'numa': false,
    'sve_acle': false
}

soc_dpaa = {
    'description': 'NXP DPAA',
    'implementer': '0x41',
    'part_number': '0xd08',
    'flags': [
        ['RTE_MACHINE', '"dpaa"'],
        ['RTE_LIBRTE_DPAA2_USE_PHYS_IOVA', false],
        ['RTE_MAX_LCORE', 16],
        ['RTE_MAX_NUMA_NODES', 1],
        ['RTE_DMA_DPAA_ERRATA_ERR050757', true],
        ['RTE_DMA_DPAA_ERRATA_ERR050265', true],
    ],
    'numa': false
}

soc_elba = {
    'description': 'AMD Pensando Elba',
    'implementer': '0x75',
    'part_number': '0xc2'
}

soc_emag = {
    'description': 'Ampere eMAG',
    'implementer': '0x50',
    'part_number': '0x0'
}

soc_ft2000plus = {
    'description': 'Phytium FT-2000+',
    'implementer': '0x70',
    'part_number': '0x662',
    'numa': true
}

soc_tys2500 = {
    'description': 'Phytium TengYun S2500',
    'implementer': '0x70',
    'part_number': '0x663',
    'numa': true
}

soc_grace = {
    'description': 'NVIDIA Grace',
    'implementer': '0x41',
    'part_number': '0xd4f',
    'extra_march_features': ['crypto'],
    'numa': true
}

soc_graviton2 = {
    'description': 'AWS Graviton2',
    'implementer': '0x41',
    'part_number': '0xd0c',
    'numa': false
}

soc_graviton3 = {
    'description': 'AWS Graviton3',
    'implementer': '0x41',
    'part_number': '0xd40',
    'extra_march_features': ['crypto'],
    'numa': false
}

soc_graviton4 = {
    'description': 'AWS Graviton4',
    'implementer': '0x41',
    'part_number': '0xd4f',
    'extra_march_features': ['crypto'],
    'flags': [
        ['RTE_MAX_LCORE', 96],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'numa': false
}

soc_hip10 = {
    'description': 'HiSilicon HIP10',
    'implementer': '0x48',
    'part_number': '0xd03',
    'numa': true
}

soc_kunpeng920 = {
    'description': 'HiSilicon Kunpeng 920',
    'implementer': '0x48',
    'part_number': '0xd01',
    'numa': true
}

soc_kunpeng930 = {
    'description': 'HiSilicon Kunpeng 930',
    'implementer': '0x48',
    'part_number': '0xd02',
    'numa': true
}

soc_n1sdp = {
    'description': 'Arm Neoverse N1SDP',
    'implementer': '0x41',
    'part_number': '0xd0c',
    'flags': [
        ['RTE_MAX_LCORE', 4]
    ],
    'numa': false
}

soc_n2 = {
    'description': 'Arm Neoverse N2',
    'implementer': '0x41',
    'part_number': '0xd49',
    'numa': false
}

soc_n3 = {
    'description': 'Arm Neoverse N3',
    'implementer': '0x41',
    'part_number': '0xd8e',
    'numa': false
}

soc_odyssey = {
    'description': 'Marvell Odyssey',
    'implementer': '0x41',
    'part_number': '0xd4f',
    'extra_march_features': ['crypto'],
    'numa': false,
    'flags': [
        ['RTE_MAX_LCORE', 80],
        ['RTE_MAX_NUMA_NODES', 1],
        ['RTE_MEMPOOL_ALIGN', 128],
    ],
}

soc_stingray = {
    'description': 'Broadcom Stingray',
    'implementer': '0x41',
    'flags': [
        ['RTE_MAX_LCORE', 16],
        ['RTE_MAX_NUMA_NODES', 1]
    ],
    'part_number': '0xd08',
    'numa': false
}

soc_thunderx2 = {
    'description': 'Marvell ThunderX2 T99',
    'implementer': '0x43',
    'part_number': '0xaf'
}

soc_thunderxt88 = {
    'description': 'Marvell ThunderX T88',
    'implementer': '0x43',
    'part_number': '0xa1'
}

soc_thunderxt83 = {
    'description': 'Marvell ThunderX T83',
    'implementer': '0x43',
    'part_number': '0xa3'
}

soc_bluefield3 = {
  'description': 'NVIDIA BlueField-3',
  'implementer': '0x41',
   'flags': [
          ['RTE_MAX_LCORE', 32],
          ['RTE_MAX_NUMA_NODES', 1]
      ],
   'part_number': '0xd42',
   'numa': false
}

soc_v2 = {
    'description': 'Arm Neoverse V2',
    'implementer': '0x41',
    'part_number': '0xd4f',
    'numa': true
}

'''
Start of SoCs list
generic:         Generic un-optimized build for armv8 aarch64 execution mode.
generic_aarch32: Generic un-optimized build for armv8 aarch32 execution mode.
altra:           Ampere Altra/AltraMax
ampereone:       Ampere AmpereOne
ampereoneac04:   Ampere AmpereOneAC04
armada:          Marvell ARMADA
bluefield:       NVIDIA BlueField
bluefield3:      NVIDIA BlueField-3
capri:           AMD Pensando Capri
cdx:             AMD CDX
centriq2400:     Qualcomm Centriq 2400
cn9k:            Marvell OCTEON 9
cn10k:           Marvell OCTEON 10
dpaa:            NXP DPAA
elba:            AMD Pensando Elba
emag:            Ampere eMAG
ft2000plus:      Phytium FT-2000+
tys2500:         Phytium TengYun S2500
grace:           NVIDIA Grace
graviton2:       AWS Graviton2
graviton3:       AWS Graviton3
graviton4:       AWS Graviton4
hip10:           HiSilicon HIP10
kunpeng920:      HiSilicon Kunpeng 920
kunpeng930:      HiSilicon Kunpeng 930
n1sdp:           Arm Neoverse N1SDP
n2:              Arm Neoverse N2
odyssey:         Marvell Odyssey
stingray:        Broadcom Stingray
thunderx2:       Marvell ThunderX2 T99
thunderxt88:     Marvell ThunderX T88
thunderxt83:     Marvell ThunderX T83
v2:              Arm Neoverse V2
End of SoCs list
'''
# The string above is included in the documentation, keep it in sync with the
# SoCs list below.
socs = {
    'generic': soc_generic,
    'generic_aarch32': soc_generic_aarch32,
    'altra': soc_altra,
    'ampereone': soc_ampereone,
    'ampereoneac04': soc_ampereoneac04,
    'armada': soc_armada,
    'bluefield': soc_bluefield,
    'bluefield3': soc_bluefield3,
    'capri': soc_capri,
    'cdx': soc_cdx,
    'centriq2400': soc_centriq2400,
    'cn9k': soc_cn9k,
    'cn10k' : soc_cn10k,
    'dpaa': soc_dpaa,
    'elba': soc_elba,
    'emag': soc_emag,
    'ft2000plus': soc_ft2000plus,
    'tys2500': soc_tys2500,
    'grace': soc_grace,
    'graviton2': soc_graviton2,
    'graviton3': soc_graviton3,
    'graviton4': soc_graviton4,
    'hip10': soc_hip10,
    'kunpeng920': soc_kunpeng920,
    'kunpeng930': soc_kunpeng930,
    'n1sdp': soc_n1sdp,
    'n2': soc_n2,
    'n3': soc_n3,
    'odyssey' : soc_odyssey,
    'stingray': soc_stingray,
    'thunderx2': soc_thunderx2,
    'thunderxt88': soc_thunderxt88,
    'thunderxt83': soc_thunderxt83,
    'v2': soc_v2,
}

dpdk_conf.set('RTE_ARCH_ARM', 1)
dpdk_conf.set('RTE_FORCE_INTRINSICS', 1)

update_flags = false
soc_flags = []
if dpdk_conf.get('RTE_ARCH_32')
    # 32-bit build
    dpdk_conf.set('RTE_CACHE_LINE_SIZE', 64)
    if meson.is_cross_build()
        update_flags = true
        soc = meson.get_external_property('platform', '')
        if soc == ''
            error('Arm SoC must be specified in the cross file.')
        endif
        soc_config = socs.get(soc, {'not_supported': true})
        flags_common = []
    else
        # armv7 build
        dpdk_conf.set('RTE_ARCH_ARMv7', true)
        dpdk_conf.set('RTE_ARCH', 'armv7')
        dpdk_conf.set('RTE_MAX_LCORE', 128)
        dpdk_conf.set('RTE_MAX_NUMA_NODES', 1)
        # the minimum architecture supported, armv7-a, needs the following,
        machine_args += '-mfpu=neon'
    endif
else
    # armv8 build
    dpdk_conf.set('RTE_ARCH', 'armv8')
    update_flags = true
    soc_config = {}
    if not meson.is_cross_build()
        # for backwards compatibility:
        #   machine=native is the same behavior as soc=native
        #   machine=generic/default is the same as soc=generic
        # cpu_instruction_set holds the proper value - native, generic or cpu
        # the old behavior only distinguished between generic and native build
        if machine != 'auto'
            if cpu_instruction_set == 'generic'
                soc = 'generic'
            else
                soc = 'native'
            endif
        else
            soc = platform
        endif
        if soc == 'native'
            # native build
            # The script returns ['Implementer', 'Variant', 'Architecture',
            # 'Primary Part number', 'Revision']
            detect_vendor = py3 + files('armv8_machine.py')
            cmd = run_command(detect_vendor, check: false)
            if cmd.returncode() == 0
                cmd_output = cmd.stdout().to_lower().strip().split(' ')
                implementer_id = cmd_output[0]
                part_number = cmd_output[3]
            else
                error('Error when getting Arm Implementer ID and part number.')
            endif
        else
            # SoC build
            soc_config = socs.get(soc, {'not_supported': true})
        endif
    else
        # cross build
        soc = meson.get_external_property('platform', '')
        if soc == ''
            error('Arm SoC must be specified in the cross file.')
        endif
        soc_config = socs.get(soc, {'not_supported': true})
    endif
endif

if update_flags
    if soc_config.has_key('not_supported')
        error('SoC @0@ not supported.'.format(soc))
    elif soc_config != {}
        implementer_id = soc_config['implementer']
        implementer_config = implementers[implementer_id]
        part_number = soc_config['part_number']
        soc_flags = soc_config.get('flags', [])
        if not soc_config.get('numa', true)
            has_libnuma = false
        endif

        disable_drivers += ',' + soc_config.get('disable_drivers', '')
        enable_drivers += ',' + soc_config.get('enable_drivers', '')
    endif

    if implementers.has_key(implementer_id)
        implementer_config = implementers[implementer_id]
    else
        error('Unsupported Arm implementer: @0@. '.format(implementer_id) +
              'Please add support for it or use the generic ' +
              '(-Dplatform=generic) build.')
    endif

    message('Arm implementer: ' + implementer_config['description'])
    message('Arm part number: ' + part_number)

    part_number_config = implementer_config['part_number_config']
    if part_number_config.has_key(part_number)
        # use the specified part_number machine args if found
        part_number_config = part_number_config[part_number]
    else
        # unknown part number
        error('Unsupported part number @0@ of implementer @1@. '
              .format(part_number, implementer_id) +
              'Please add support for it or use the generic ' +
              '(-Dplatform=generic) build.')
    endif

    # add/overwrite flags in the proper order
    dpdk_flags = flags_common + implementer_config['flags'] + part_number_config.get('flags', []) + soc_flags

    machine_args = [] # Clear previous machine args

    march_features = []
    if part_number_config.has_key('march_features')
        march_features += part_number_config['march_features']
    endif
    if soc_config.has_key('extra_march_features')
        march_features += soc_config['extra_march_features']
    endif

    candidate_mcpu = ''
    candidate_march = ''

    if (part_number_config.has_key('mcpu') and
        cc.has_argument('-mcpu=' + part_number_config['mcpu']))
        candidate_mcpu = '-mcpu=' + part_number_config['mcpu']
        foreach feature: march_features
            if cc.has_argument('+'.join([candidate_mcpu, feature]))
                candidate_mcpu = '+'.join([candidate_mcpu, feature])
            else
                warning('The compiler does not support feature @0@'
                    .format(feature))
            endif
        endforeach
        machine_args += candidate_mcpu
    elif part_number_config.has_key('march')
        # probe supported archs and their features
        if part_number_config.get('force_march', false)
            candidate_march = part_number_config['march']
        else
            supported_marchs = ['armv9-a', 'armv8.7-a', 'armv8.6-a', 'armv8.5-a', 'armv8.4-a', 'armv8.3-a',
                                'armv8.2-a', 'armv8.1-a', 'armv8-a']
            check_compiler_support = false
            foreach supported_march: supported_marchs
                if supported_march == part_number_config['march']
                    # start checking from this version downwards
                    check_compiler_support = true
                endif
                if (check_compiler_support and
                    cc.has_argument('-march=' + supported_march))
                    candidate_march = supported_march
                    # highest supported march version found
                    break
                endif
            endforeach
            if (part_number_config.has_key('fallback_march') and
                candidate_march != part_number_config['march'] and
                cc.has_argument('-march=' + part_number_config['fallback_march']))
                    candidate_march = part_number_config['fallback_march']
            endif
        endif

        if candidate_march != part_number_config['march']
            warning('Configuration march version is @0@, not supported.'
                    .format(part_number_config['march']))
            if candidate_march != ''
                warning('Using march version @0@.'.format(candidate_march))
            endif
        endif

        if candidate_march != ''
            candidate_march = '-march=' + candidate_march
            foreach feature: march_features
                if cc.has_argument('+'.join([candidate_march, feature]))
                    candidate_march = '+'.join([candidate_march, feature])
                else
                    warning('The compiler does not support feature @0@'
                        .format(feature))
                endif
            endforeach
            machine_args += candidate_march
        endif
    endif

    if candidate_mcpu == '' and candidate_march == ''
        error('No suitable ARM march/mcpu version found.')
    endif

    # apply supported compiler options
    if part_number_config.has_key('compiler_options')
        foreach flag: part_number_config['compiler_options']
            if cc.has_multi_arguments(machine_args + [flag])
                machine_args += flag
            else
                warning('Configuration compiler option ' +
                        '@0@ isn\'t supported.'.format(flag))
            endif
        endforeach
    endif

    # apply flags
    foreach flag: dpdk_flags
        if flag.length() > 0
            dpdk_conf.set(flag[0], flag[1])
        endif
    endforeach
endif
message('Using machine args: @0@'.format(machine_args))

if (cc.get_define('__ARM_NEON', args: machine_args) != '' or
    cc.get_define('__aarch64__', args: machine_args) != '')
    compile_time_cpuflags += ['RTE_CPUFLAG_NEON']
endif

if cc.get_define('__ARM_FEATURE_SVE', args: machine_args) != ''
    compile_time_cpuflags += ['RTE_CPUFLAG_SVE']
    if (cc.check_header('arm_sve.h') and soc_config.get('sve_acle', true))
        dpdk_conf.set('RTE_HAS_SVE_ACLE', 1)
    endif
endif

if cc.get_define('__ARM_FEATURE_CRC32', args: machine_args) != ''
    compile_time_cpuflags += ['RTE_CPUFLAG_CRC32']
endif

if cc.get_define('__ARM_FEATURE_CRYPTO', args: machine_args) != ''
    compile_time_cpuflags += ['RTE_CPUFLAG_AES', 'RTE_CPUFLAG_PMULL',
    'RTE_CPUFLAG_SHA1', 'RTE_CPUFLAG_SHA2']
endif
