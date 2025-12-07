// SPDX-License-Identifier: LGPL-3.0-or-later

#include <core/kernel/userspace.h>
#include <core/kernel/kstd.h>

// External declarations for userspace programs
extern int echo_main(int argc, char** argv);
extern int clear_main(int argc, char** argv);
extern int ls_main(int argc, char** argv);
extern int cat_main(int argc, char** argv);
extern int rm_main(int argc, char** argv);
extern int write_main(int argc, char** argv);
extern int nova_main(int argc, char** argv);
extern int uname_main(int argc, char** argv);

// Register all userspace programs
void userspace_init_programs(void) {
    kprint(":: Registering userspace programs...\n", 7);
    
    userspace_register("echo", echo_main);
    userspace_register("clear", clear_main);
    userspace_register("ls", ls_main);
    userspace_register("cat", cat_main);
    userspace_register("rm", rm_main);
    userspace_register("write", write_main);
    userspace_register("nova", nova_main);
    userspace_register("uname", uname_main);
    
    kprint(":: Userspace programs registered\n", 7);
}
