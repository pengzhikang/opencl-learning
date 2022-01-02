__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_LINEAR | CLK_ADDRESS_CLAMP;

__kernel void image_rotate(__read_only image2d_t srcImg, __write_only image2d_t dstImg, float angle)
{
    int width = get_image_width(srcImg);
    int height = get_image_height(srcImg);
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    float sinma = sin(angle);
    float cosma = cos(angle);

    int hwidth = width / 2;
    int hheight = height / 2;
    int xt = x - hwidth;
    int yt = y - hheight;
    float2 redCoord;
    redCoord.x = (sinma * yt + cosma * xt) + hwidth;
    redCoord.y = (cosma * yt - sinma * xt) + hheight;
    float4 value = read_imagef(srcImg, sampler, redCoord);
    write_imagef(dstImg, (int2)(x, y), value);
}