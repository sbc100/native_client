/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_SYSCALL_DEFINES_H_
#define NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_SYSCALL_DEFINES_H_ 1

#define FUTEX_WAIT_PRIVATE 128
#define FUTEX_WAKE_PRIVATE 129

#if defined(__i386__) || defined(__arm__)
#define LINUX_TCGETS 0x5401
#elif defined(__mips__)
#define LINUX_TCGETS 0x540d
#else
# error Unsupported architecture
#endif

#if defined(__i386__) || defined(__arm__)
#define LINUX_SIG_UNBLOCK 1
#elif defined(__mips__)
#define LINUX_SIG_UNBLOCK 2
#else
# error Unsupported architecture
#endif

#if defined(__i386__) || defined(__arm__)
#define LINUX_SA_SIGINFO 0x00000004
#elif defined(__mips__)
#define LINUX_SA_SIGINFO 0x00000008
#else
# error Unsupported architecture
#endif

#define LINUX_SA_ONSTACK 0x08000000
#define LINUX_SA_NODEFER 0x40000000
#define LINUX_SA_RESTART 0x10000000

/* From linux/arch/{arch}/include/uapi/asm/signal.h */
#if defined(__mips__)

#define LINUX_SIGHUP           1
#define LINUX_SIGINT           2
#define LINUX_SIGQUIT          3
#define LINUX_SIGILL           4
#define LINUX_SIGTRAP          5
#define LINUX_SIGABRT          6
#define LINUX_SIGBUS          10
#define LINUX_SIGFPE           8
#define LINUX_SIGKILL          9
#define LINUX_SIGUSR1         16
#define LINUX_SIGSEGV         11
#define LINUX_SIGUSR2         17
#define LINUX_SIGPIPE         13
#define LINUX_SIGALRM         14
#define LINUX_SIGTERM         15
#define LINUX_SIGCHLD         18
#define LINUX_SIGSYS          12

#elif defined(__i386__) || defined(__arm__)

#define LINUX_SIGHUP           1
#define LINUX_SIGINT           2
#define LINUX_SIGQUIT          3
#define LINUX_SIGILL           4
#define LINUX_SIGTRAP          5
#define LINUX_SIGABRT          6
#define LINUX_SIGBUS           7
#define LINUX_SIGFPE           8
#define LINUX_SIGKILL          9
#define LINUX_SIGUSR1         10
#define LINUX_SIGSEGV         11
#define LINUX_SIGUSR2         12
#define LINUX_SIGPIPE         13
#define LINUX_SIGALRM         14
#define LINUX_SIGTERM         15
#define LINUX_SIGSTKFLT       16
#define LINUX_SIGCHLD         17
#define LINUX_SIGSYS          31

#else
# error Unsupported architecture
#endif

#endif

/* Contains only subset of error codes which newlib defines */

#if defined(__i386__) || defined(__arm__)

#define LINUX_EDEADLK      35     /* Deadlock condition */
#define LINUX_ENAMETOOLONG 36     /* File name too long */
#define LINUX_ENOLCK       37     /* No record locks available */
#define LINUX_ENOSYS       38     /* Function not implemented */
#define LINUX_ENOTEMPTY    39     /* Directory not empty */
#define LINUX_ELOOP        40     /* Too many symbolic links */
#define LINUX_ENOMSG       42     /* No message of desired type */
#define LINUX_EIDRM        43     /* Identifier removed */
#define LINUX_ECHRNG       44     /* Channel number out of range */
#define LINUX_EL2NSYNC     45     /* Level 2 not synchronized */
#define LINUX_EL3HLT       46     /* Level 3 halted */
#define LINUX_EL3RST       47     /* Level 3 reset */
#define LINUX_ELNRNG       48     /* Link number out of range */
#define LINUX_EUNATCH      49     /* Protocol driver not attached */
#define LINUX_ENOCSI       50     /* No CSI structure available */
#define LINUX_EL2HLT       51     /* Level 2 halted */
#define LINUX_EBADE        52     /* Invalid exchange */
#define LINUX_EBADR        53     /* Invalid request descriptor */
#define LINUX_EXFULL       54     /* Exchange full */
#define LINUX_ENOANO       55     /* No anode */
#define LINUX_EBADRQC      56     /* Invalid request code */
#define LINUX_EBADSLT      57     /* Invalid slot */
#define LINUX_EDEADLOCK    LINUX_EDEADLK  /* File locking deadlock error */
#define LINUX_EBFONT       59     /* Bad font file fmt */
#define LINUX_ENOSTR       60     /* Device not a stream */
#define LINUX_ENODATA      61     /* No data (for no delay io) */
#define LINUX_ETIME        62     /* Timer expired */
#define LINUX_ENOSR        63     /* Out of streams resources */
#define LINUX_ENONET       64     /* Machine is not on the network */
#define LINUX_ENOPKG       65     /* Package not installed */
#define LINUX_EREMOTE      66     /* The object is remote */
#define LINUX_ENOLINK      67     /* The link has been severed */
#define LINUX_EADV         68     /* Advertise error */
#define LINUX_ESRMNT       69     /* Srmount error */
#define LINUX_ECOMM        70     /* Communication error on send */
#define LINUX_EPROTO       71     /* Protocol error */
#define LINUX_EMULTIHOP    72     /* Multihop attempted */
#define LINUX_EDOTDOT      73     /* Cross mount point (not really error) */
#define LINUX_EBADMSG      74     /* Trying to read unreadable message */
#define LINUX_EOVERFLOW    75     /* Value too large for defined data type */
#define LINUX_ENOTUNIQ     76     /* Given log. name not unique */
#define LINUX_EBADFD       77     /* f.d. invalid for this operation */
#define LINUX_EREMCHG      78     /* Remote address changed */
#define LINUX_ELIBACC      79     /* Can't access a needed shared lib */
#define LINUX_ELIBBAD      80     /* Accessing a corrupted shared lib */
#define LINUX_ELIBSCN      81     /* .lib section in a.out corrupted */
#define LINUX_ELIBMAX      82     /* Attempting to link in too many libs */
#define LINUX_ELIBEXEC     83     /* Attempting to exec a shared library */
#define LINUX_EILSEQ       84
#define LINUX_EUSERS       87
#define LINUX_ENOTSOCK     88    /* Socket operation on non-socket */
#define LINUX_EDESTADDRREQ 89    /* Destination address required */
#define LINUX_EMSGSIZE     90    /* Message too long */
#define LINUX_EPROTOTYPE   91    /* Protocol wrong type for socket */
#define LINUX_ENOPROTOOPT  92    /* Protocol not available */
#define LINUX_EPROTONOSUPPORT 93 /* Unknown protocol */
#define LINUX_ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define LINUX_EOPNOTSUPP   95 /* Operation not supported on transport endpoint */
#define LINUX_EPFNOSUPPORT 96 /* Protocol family not supported */
#define LINUX_EAFNOSUPPORT 97 /* Address family not supported by protocol family */
#define LINUX_EADDRINUSE   98    /* Address already in use */
#define LINUX_EADDRNOTAVAIL 99   /* Address not available */
#define LINUX_ENETDOWN     100   /* Network interface is not configured */
#define LINUX_ENETUNREACH  101   /* Network is unreachable */
#define LINUX_ENETRESET    102
#define LINUX_ECONNABORTED 103   /* Connection aborted */
#define LINUX_ECONNRESET   104   /* Connection reset by peer */
#define LINUX_ENOBUFS      105   /* No buffer space available */
#define LINUX_EISCONN      106   /* Socket is already connected */
#define LINUX_ENOTCONN     107   /* Socket is not connected */
#define LINUX_ESHUTDOWN    108   /* Can't send after socket shutdown */
#define LINUX_ETOOMANYREFS 109
#define LINUX_ETIMEDOUT    110   /* Connection timed out */
#define LINUX_ECONNREFUSED 111   /* Connection refused */
#define LINUX_EHOSTDOWN    112   /* Host is down */
#define LINUX_EHOSTUNREACH 113   /* Host is unreachable */
#define LINUX_EALREADY     114   /* Socket already connected */
#define LINUX_EINPROGRESS  115   /* Connection already in progress */
#define LINUX_ESTALE       116
#define LINUX_ENOTSUP      LINUX_EOPNOTSUPP  /* Not supported */
#define LINUX_EDQUOT       122    /* Quota exceeded */
#define LINUX_ENOMEDIUM    123    /* No medium (in tape drive) */
#define LINUX_ECANCELED    125    /* Operation canceled. */
#define LINUX_EOWNERDEAD   130    /* Owner died */
#define LINUX_ENOTRECOVERABLE 131 /* State not recoverable */

#elif defined(__mips__)
#define LINUX_EDEADLK          45
#define LINUX_ENAMETOOLONG     78
#define LINUX_ENOLCK           46
#define LINUX_ENOSYS           89
#define LINUX_ENOTEMPTY        93
#define LINUX_ELOOP            90
#define LINUX_ENOMSG           35
#define LINUX_EIDRM            36
#define LINUX_ECHRNG           37
#define LINUX_EL2NSYNC         38
#define LINUX_EL3HLT           39
#define LINUX_EL3RST           40
#define LINUX_ELNRNG           41
#define LINUX_EUNATCH          42
#define LINUX_ENOCSI           43
#define LINUX_EL2HLT           44
#define LINUX_EBADE            50
#define LINUX_EBADR            51
#define LINUX_EXFULL           52
#define LINUX_ENOANO           53
#define LINUX_EBADRQC          54
#define LINUX_EBADSLT          55
#define LINUX_EDEADLOCK        56
#define LINUX_EBFONT           59
#define LINUX_ENOSTR           60
#define LINUX_ENODATA          61
#define LINUX_ETIME            62
#define LINUX_ENOSR            63
#define LINUX_ENONET           64
#define LINUX_ENOPKG           65
#define LINUX_EREMOTE          66
#define LINUX_ENOLINK          67
#define LINUX_EADV             68
#define LINUX_ESRMNT           69
#define LINUX_ECOMM            70
#define LINUX_EPROTO           71
#define LINUX_EMULTIHOP        74
#define LINUX_EDOTDOT          73
#define LINUX_EBADMSG          77
#define LINUX_EOVERFLOW        79
#define LINUX_ENOTUNIQ         80
#define LINUX_EBADFD           81
#define LINUX_EREMCHG          82
#define LINUX_ELIBACC          83
#define LINUX_ELIBBAD          84
#define LINUX_ELIBSCN          85
#define LINUX_ELIBMAX          86
#define LINUX_ELIBEXEC         87
#define LINUX_EILSEQ           88
#define LINUX_EUSERS           94
#define LINUX_ENOTSOCK         95
#define LINUX_EDESTADDRREQ     96
#define LINUX_EMSGSIZE         97
#define LINUX_EPROTOTYPE       98
#define LINUX_ENOPROTOOPT      99
#define LINUX_EPROTONOSUPPORT 120
#define LINUX_ESOCKTNOSUPPORT 121
#define LINUX_EOPNOTSUPP      122
#define LINUX_EPFNOSUPPORT    123
#define LINUX_EAFNOSUPPORT    124
#define LINUX_EADDRINUSE      125
#define LINUX_EADDRNOTAVAIL   126
#define LINUX_ENETDOWN        127
#define LINUX_ENETUNREACH     128
#define LINUX_ENETRESET       129
#define LINUX_ECONNABORTED    130
#define LINUX_ECONNRESET      131
#define LINUX_ENOBUFS         132
#define LINUX_EISCONN         133
#define LINUX_ENOTCONN        134
#define LINUX_ESHUTDOWN       143
#define LINUX_ETOOMANYREFS    144
#define LINUX_ETIMEDOUT       145
#define LINUX_ECONNREFUSED    146
#define LINUX_EHOSTDOWN       147
#define LINUX_EHOSTUNREACH    148
#define LINUX_EALREADY        149
#define LINUX_EINPROGRESS     150
#define LINUX_ESTALE          151
#define LINUX_ENOTSUP         LINUX_EOPNOTSUPP
#define LINUX_EDQUOT         1133
#define LINUX_ENOMEDIUM       159
#define LINUX_ECANCELED       158
#define LINUX_EOWNERDEAD      165
#define LINUX_ENOTRECOVERABLE 166
#else
# error Unsupported architecture
#endif
