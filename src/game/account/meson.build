account_include = include_directories('.', 'include')

###
### reflection data
###

src = [
    daocc.process('data/account.xml'),
]

deps = [ core, db, dao ]

libaccount_meta = library('account-meta', src,
    include_directories : account_include,
    dependencies : deps,
)

account_meta = declare_dependency(
    link_with : libaccount_meta,
    include_directories : [ account_include, libaccount_meta.private_dir_include() ],
    dependencies : deps,
)

###
### implementation
###

src = [
    'src/salt.cpp',
    'src/packets.cpp',
    'src/server.cpp',
    'src/client.cpp',
    'src/router.cpp',
    'src/stream.cpp',
]

deps = [ logs, net, account_meta ]

libaccount = library('account', src,
    cpp_args : [ '-DSM_ACCOUNT_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : account_include,
    dependencies : deps,
)

account = declare_dependency(
    link_with : libaccount,
    include_directories : account_include,
    dependencies : deps,
)

###
### test utils
###

libaccounttest = library('account-test', 'test/account_test_common.cpp',
    include_directories : '.',
    dependencies : [ dbtest, nettest, account ]
)

accounttest = declare_dependency(
    link_with : libaccounttest,
    include_directories : '.',
    dependencies : [ dbtest, nettest, account ]
)

###
### tests
###

testcases = {
    'Password salt': 'test/salt.cpp',
    'Account creation': 'test/create.cpp',
    'Account login': 'test/login.cpp',
    'Posting messages': 'test/post.cpp',
    'Get session list': 'test/session_list.cpp',
    'Get lobby list': 'test/lobby_list.cpp',
    'Join lobby': 'test/join_lobby.cpp',
    'Multiple join lobby': 'test/join_lobby_multi.cpp',
    #'Sending messages': 'test/streaming_messages.cpp',
}

foreach name, source : testcases
    exe = executable('test-account-' + name.to_lower().replace(' ', '-'), source,
        include_directories : account_include,
        dependencies : [ accounttest ],
    )

    test(name, exe,
        suite : 'account',
        kwargs : testkwargs + { 'timeout': 15 }
    )
endforeach
