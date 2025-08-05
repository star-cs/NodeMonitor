#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

// 完全自定义的getuid
#include <sys/types.h>
#include <sys/syscall.h> 
#include <unistd.h>

static uid_t my_getuid(void)
{
    return syscall(__NR_getuid); // 或直接使用内联汇编
}

// 自定义的getpwuid
struct my_passwd {
    char *pw_name;
    uid_t pw_uid;
};

static struct my_passwd *my_getpwuid(uid_t uid)
{
    static struct my_passwd result;
    static char line[1024];
    FILE *fp = fopen("/etc/passwd", "r");

    if (!fp)
        return NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *name = strtok(line, ":");
        strtok(NULL, ":"); // 跳过密码
        char *uid_str = strtok(NULL, ":");

        if (uid_str && atoi(uid_str) == uid) {
            result.pw_name = strdup(name);
            result.pw_uid = uid;
            fclose(fp);
            return &result;
        }
    }

    fclose(fp);
    return NULL;
}

int main()
{
    std::vector<std::shared_ptr<monitor::MonitorInter>> runners_;

    return 0;
}