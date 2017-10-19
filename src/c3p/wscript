# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('c3p', ['network', 'internet', 'dcn'])
    module.source = [
        'model/c3-division.cc',
        'model/c3-cs-division.cc',
        'model/c3-cs-flow.cc',
        'model/c3-cs-tunnel.cc',
        'model/c3-ds-division.cc',
        'model/c3-ds-flow.cc',
        'model/c3-ds-tunnel.cc',
        'model/c3-ls-division.cc',
        'model/c3-ls-flow.cc',
        'model/c3-ls-tunnel.cc',
        'model/c3-ecn-recorder.cc',
        'model/c3-flow.cc',
        'model/c3-l3_5-protocol.cc',
        'model/c3-tag.cc',
        'model/c3-tunnel.cc',
        ]

    module_test = bld.create_ns3_module_test_library('c3p')
    module_test.source = [
        ]

    headers = bld(features='ns3header')
    headers.module = 'c3p'
    headers.source = [
        'model/c3-division.h',
        'model/c3-cs-division.h',
        'model/c3-cs-flow.h',
        'model/c3-cs-tunnel.h',
        'model/c3-ds-division.h',
        'model/c3-ds-flow.h',
        'model/c3-ds-tunnel.h',
        'model/c3-ls-division.h',
        'model/c3-ls-flow.h',
        'model/c3-ls-tunnel.h',
        'model/c3-ecn-recorder.h',
        'model/c3-flow.h',
        'model/c3-l3_5-protocol.h',
        'model/c3-tag.h',
        'model/c3-tunnel.h',
        'model/c3-type.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        pass

    # bld.ns3_python_bindings()

