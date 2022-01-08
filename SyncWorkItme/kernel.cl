__kernel void kernel_test(__global int *pDst,
                         __global int *pSrc1, __global int *pLocks)
{
    local int tmpBuffer[GROUP_NUMBER_OF_WORKITEMS];
    const int index = get_local_id(0);
    const int group_index = get_group_id(0);
    int group_count = get_num_groups(0);
    const uint max_dims = get_work_dim();
    for (int i = 0; i < max_dims; i++)
    {
        index *= get_local_id(i);
        
    }
    int address_offset = group_index * GROUP_NUMBER_OF_WORKITEMS+index;
    //第一次迭代从pSrc1获取数据
    __global int *pData = pSrc1;
    while(group_count >= GROUP_NUMBER_OF_WORKITEMS && group_index < group_count)
    {
        if(index == 0)
        {
            //这里使用按位或来设置锁,由于不管哪个整 数,
            //与1相或之后都不会为0,这里用非0表示上 锁状态;0表示解锁状态
            atomic_or(&pLocks[group_index],1);
        }
        //对当前工作组中的所有工作项先暂存局部存储 器
        tmpBuffer[index] = pData[address_offset];
        barrier(CLK_LOCAL_MEM_FENCE);
        if(index == 0)
        {
            //用第一个工作项做求和计算
            int sum = 0;
            for(int i = 0; i < GROUP_NUMBER_OF_WORKITEMS; i++)
                sum += tmpBuffer[i];
            //输出到对应工作组索引的全局输出存储器
            pDst[group_index] = sum;
            //这里假定GROUP_NUMBER_OF_WORKITEMS是2 的n次幂
            //由于绝大多数计算设备都能满足这个条件
            if((group_index & (GROUP_NUMBER_OF_WORKITEMS - 1)) == GROUP_NUMBER_OF_WORKITEMS - 1)
			{
                //让工作组索引对应于一个工作组
                //最后一个工作项的工作项进行解锁
                atomic_xchg(&pLocks[group_index / GROUP_NUMBER_OF_WORKITEMS], 0);
            }
        }
        //后续要处理的工作组个数需要除以最大工作组 大小
        //上述已经处理了GROUP_NUMBER_OF_WORKITEMS个 工作组的元素个数,
        //即一共GROUP_NUMBER_OF_WORKITEMS  *GROUP_NUMBER_OF_WORKITEMS
        //个元素
        group_count /= GROUP_NUMBER_OF_WORKITEMS;
        if(index == 0 && group_index < group_count)
        {
            //对于剩余的工作项,在每一个工作组中用第 一个工作项等待锁被打开
            int lock;
            do
            {
                //对于OpenCL 2.0以下的版本,
                //我们可以使用原子加法来模拟OpenCL  2.0中的atomic_load
                lock = atomic_add(&pLocks[group_index],0);
            }
            while(lock!=0);
        }
        //从第二次迭代开始,从pDst获取数据
        pData =pDst;
    } 
    //将pLock作为剩余要被主机端处理的元素个数输出
    if(get_global_id(0) ==0)
        pLocks[0] =group_count *  GROUP_NUMBER_OF_WORKITEMS;
}