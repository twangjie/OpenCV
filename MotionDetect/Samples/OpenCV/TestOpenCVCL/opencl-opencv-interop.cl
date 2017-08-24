const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;  

__kernel void bitwise_inv_buf_8uC1(
    __global unsigned char* pSrcDst,
             int            srcDstStep,
             int            rows,
             int            cols)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int idx = mad24(y, srcDstStep, x);
    pSrcDst[idx] = ~pSrcDst[idx];
}

__kernel void bitwise_inv_img_8uC1(
    read_only  image2d_t srcImg,
    write_only image2d_t dstImg)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int2 coord = (int2)(x, y);
    uint4 val = read_imageui(srcImg, sampler, coord);
    val.x = (~val.x) & 0x000000FF;
    write_imageui(dstImg, coord, val);
}
