#ifndef _REASON_H
#define _REASON_H

#if _WIN32_WINNT >= 0x0501

#define SHTDN_REASON_MAJOR_APPLICATION     0x00040000
#define SHTDN_REASON_MAJOR_HARDWARE        0x00010000
#define SHTDN_REASON_MAJOR_LEGACY_API      0x00070000
#define SHTDN_REASON_MAJOR_OPERATINGSYSTEM 0x00020000
#define SHTDN_REASON_MAJOR_OTHER           0x00000000
#define SHTDN_REASON_MAJOR_POWER           0x00060000
#define SHTDN_REASON_MAJOR_SOFTWARE        0x00030000
#define SHTDN_REASON_MAJOR_SYSTEM          0x00050000

#define SHTDN_REASON_MINOR_BLUESCREEN            0x0000000f
#define SHTDN_REASON_MINOR_CORDUNPLUGGED         0x0000000b
#define SHTDN_REASON_MINOR_DISK                  0x00000007
#define SHTDN_REASON_MINOR_ENVIRONMENT           0x0000000c
#define SHTDN_REASON_MINOR_HARDWARE_DRIVER       0x0000000d
#define SHTDN_REASON_MINOR_HOTFIX                0x00000011
#define SHTDN_REASON_MINOR_HOTFIX_UNINSTALL      0x00000017
#define SHTDN_REASON_MINOR_HUNG                  0x00000005
#define SHTDN_REASON_MINOR_INSTALLATION          0x00000002
#define SHTDN_REASON_MINOR_MAINTENANCE           0x00000001
#define SHTDN_REASON_MINOR_MMC                   0x00000019
#define SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY  0x00000014
#define SHTDN_REASON_MINOR_NETWORKCARD           0x00000009
#define SHTDN_REASON_MINOR_OTHER                 0x00000000
#define SHTDN_REASON_MINOR_OTHERDRIVER           0x0000000e
#define SHTDN_REASON_MINOR_POWER_SUPPLY          0x0000000a
#define SHTDN_REASON_MINOR_PROCESSOR             0x00000008
#define SHTDN_REASON_MINOR_RECONFIG              0x00000004
#define SHTDN_REASON_MINOR_SECURITY              0x00000013
#define SHTDN_REASON_MINOR_SECURITYFIX           0x00000012
#define SHTDN_REASON_MINOR_SECURITYFIX_UNINSTALL 0x00000018
#define SHTDN_REASON_MINOR_SERVICEPACK           0x00000010
#define SHTDN_REASON_MINOR_SERVICEPACK_UNINSTALL 0x00000016
#define SHTDN_REASON_MINOR_TERMSRV               0x00000020
#define SHTDN_REASON_MINOR_UNSTABLE              0x00000006
#define SHTDN_REASON_MINOR_UPGRADE               0x00000003
#define SHTDN_REASON_MINOR_WMI                   0x00000015

#define SHTDN_REASON_FLAG_USER_DEFINED           0x40000000
#define SHTDN_REASON_FLAG_PLANNED                0x80000000

#endif /* _WIN32_WINNT >= 0x0501 */

#endif /* _REASON_H */