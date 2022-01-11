#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <assert.h>
#include <sys/syscall.h>
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
//计时函数
static double GetCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 获取当前线程使用的cpu核心
void ProUsedCpu()
{
    long CpuNum = sysconf(_SC_NPROCESSORS_CONF);
    std::vector<size_t> cpu_set;
    cpu_set_t mask;
    sched_getaffinity(0, sizeof(cpu_set_t), &mask);
    for(size_t i = 0; i < CpuNum; i++)
    {
        if(CPU_ISSET(i, &mask))
        {
            cpu_set.push_back(i);
        }
    }
    if(cpu_set.size() > 0)
    {
        std::cout << "使用到的cpu核心数目:" << cpu_set.size() << std::endl;
        std::cout << "使用到的cpu:";
        for ( size_t j = 0; j < cpu_set.size(); j++)
        {
            std::cout << cpu_set[j] << ",";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "未使用到任何cpu核心" << std::endl;
    }
}

// 根据cpu_set来改变核亲和性
void SetUsedCpu(std::vector<size_t> cpu_set)
{
    size_t CpuNum = (size_t)sysconf(_SC_NPROCESSORS_CONF);
    assert(cpu_set.size() <= CpuNum);
    cpu_set_t mask;
    sched_getaffinity(0, sizeof(cpu_set_t), &mask);
    for(size_t i = 0; i < CpuNum; i++)
    {
        auto it = std::find(cpu_set.begin(), cpu_set.end(), i);
        if(it != cpu_set.end())
        {
            //在列表中，则继续设置
            CPU_SET(i, &mask);
        }
        else
        {
            // 否则去除
            CPU_CLR(i, &mask);
        }
        // int cpu_id = std::max(0, std::min(CpuNum, cpu_set[i]));
    }
    // 设置
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

// 设置一个使用cpu的函数进行计算，按照计时函数来进行结束
void AccTime(double EndTime)
{
    double t1 = GetCurrentTime();
    float a = 0.1;
    float b = 0.2131;
    float c = 0;
    do
    {
        c = a + b;
        c = a - b;
        c = a * b;
        c = a / b;
    } while ((GetCurrentTime() - t1) <= EndTime);
    
}
// 循环使用cpu核心
void CycleUseCpu(double EndTime = 1000, double OnceTime = 1000)
{
    double t1 = GetCurrentTime();
    size_t CpuNum = (size_t)sysconf(_SC_NPROCESSORS_CONF);
    size_t i = 0;
    do
    {
        SetUsedCpu(std::vector<size_t>({i}));
        std::cout << "[" << (GetCurrentTime() - t1) << "ms]" << "开启" << (i + 1) << "号核" << std::endl;
        AccTime(OnceTime);
        ++i;
        i = i % CpuNum;
    } while((GetCurrentTime() - t1) <= EndTime);
}

int main1()
{
    long CpuNum = sysconf(_SC_NPROCESSORS_CONF);
    long UsedCpuNum = sysconf(_SC_NPROCESSORS_ONLN);
    long PageSize = sysconf(_SC_PAGESIZE);
    long PhysPages = sysconf(_SC_PHYS_PAGES);
    long AvPhysPages = sysconf(_SC_AVPHYS_PAGES);
    long LoginNameMax = sysconf(_SC_LOGIN_NAME_MAX);
    long HostNameMax = sysconf(_SC_HOST_NAME_MAX);
    long OpenMax = sysconf(_SC_OPEN_MAX);
    long ClkTck = sysconf(_SC_CLK_TCK);
    std::cout << "处理器个数:" << CpuNum << std::endl;
    std::cout << "正在使用的处理器个数:" << UsedCpuNum << std::endl;
    std::cout << "内存页面大小:" << PageSize << std::endl;
    std::cout << "内存的总页面数目:" << PhysPages << std::endl;
    std::cout << "目前可利用的总页面数目:" << AvPhysPages << std::endl;
    std::cout << "最大登录名长度:" << LoginNameMax << std::endl;
    std::cout << "最大主机名长度:" << HostNameMax << std::endl;
    std::cout << "单个进程运行时能够打开的最大文件数目:" << OpenMax << std::endl;
    std::cout << "每秒跑过的运行速率:" << ClkTck << std::endl;
    std::cout << "默认的亲和性情况:" << std::endl;
    ProUsedCpu();
    AccTime(10 * 1000); // 单位是ms
    std::cout << "改变为只使用0,2,4号核" << std::endl;
    SetUsedCpu(std::vector<size_t>({0}));
    std::cout << "改变后的亲和性情况:" << std::endl;
    ProUsedCpu();
    AccTime(10 * 1000);
    std::cout << "开始循环进行核亲和性循环使用" << std::endl;
    CycleUseCpu(CpuNum * 10 * 1000, 4000);
    return 0;
}