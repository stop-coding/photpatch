
#include "patch.h"

// 补丁名称，必须跟配置文件保持一样
static char HOTPATCH_NAME[] = "hongch";


// 加载补丁前回调
int pre_patch_callback(void)
{
    printf("previouce patch callback.");
    return 0;
}

int post_patch_callback(void)
{
    printf("previouce patch callback.");
    return 0;
}

int pre_unpatch_callback(void)
{
    printf("post unpatch callback.");
    return 0;
}

int post_unpatch_callback(void)
{
    printf("post unpatch callback.");
    return 0;
}

void helloworld(void)
{
    printf("Hello world, I'm new patch.");
}