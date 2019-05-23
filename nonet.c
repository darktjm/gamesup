#include "utils.h"
#include "namespace.h"


int main(int argc, char **argv, char **envp)
{
    if(argc < 2)
	exit(1);
    uid_t id =  getuid();
    gid_t g = getgid();
    if(netns_switch("nonet") || setresuid(id, id, id) ||
       setresgid(g, g, g) ||
       execvpe(argv[1], argv + 1, envp)) {
	perror(argv[1]);
	exit(1);
    }
}
