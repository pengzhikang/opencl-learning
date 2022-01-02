
__kernel
void vector_add(global const float *a,global const float *b,global float *result)
{
    int gid = get_global_id(0);
    result[gid] = a[gid] + b[gid];
}

__kernel
void vector_mul(global const float*a, global const float* b, global float* result)
{
    int gid = get_global_id(0);
    result[gid] = a[gid]*b[gid];
}