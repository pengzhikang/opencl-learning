// Copyright (c) 2009-2014 Intel Corporation
// All rights reserved.
//
// WARRANTY DISCLAIMER
//
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


kernel void process_buffer(
    global const float4*    restrict p_src, // src RGBA buffer
    global float4*          restrict p_dst, // dst RGBA buffer
    const int2              size,           // size of both input pictures
    const int               dst_pitch)      // distance between line in pixels for dst buffer
{
    int x = (int)get_global_id(0);
    int y = (int)get_global_id(1);
    // make gamma correction out=power(inp,1/4);
    p_dst[y*dst_pitch+x] = sqrt(sqrt(p_src[y*size.x+x]));
}

const sampler_t smp = CLK_FILTER_LINEAR|CLK_ADDRESS_CLAMP|CLK_NORMALIZED_COORDS_FALSE;
kernel void process_image(
    __read_only image2d_t   src,            // src RGBA buffer
    global float4*          restrict p_dst, // dst RGBA buffer
    const int2              size,           // size of both image
    const float4            M,              // matrix for affine transform
    const float2            T)              // vector for affine transform
{
    int x = (int)get_global_id(0);
    int y = (int)get_global_id(1);
    float2 p = (float2)(x,y);
    float2 s;
    s.x = M.x*p.x+M.y*p.y + T.x;// s = Mx + T
    s.y = M.z*p.x+M.w*p.y + T.y;
    p_dst[y*size.x+x] = read_imagef(src, smp, s);
}
