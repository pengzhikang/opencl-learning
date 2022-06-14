import numpy as np
import cv2
import math

def mywarpAffine(inputs:np.ndarray, angle, scale):
    reshapesize = inputs.shape[:2] # 宽，长
    # angle = 0.3
    # scale = 0.7
    M = [[math.cos(angle) / scale, math.sin(angle) / scale], [-math.sin(angle) / scale, -math.cos(angle) / scale]]

    T = [0.5 * (reshapesize[1] - (M[0][0] * reshapesize[1] + M[1][0] * reshapesize[0])),
         0.5 * (reshapesize[0] - (M[1][0] * reshapesize[1] + M[1][1] * reshapesize[0]))]
    # T = [-545.116211, 118.877533]
    tfrom = np.zeros((2, 3))
    tfrom[:2, :2] = np.array(M)
    tfrom[:, 2] = np.array(T)
    # tfrom = np.array([tfrom.tolist()[1], tfrom.tolist()[0]])
    dst = np.zeros(inputs.shape).astype(inputs.dtype)
    for x in range(reshapesize[1]):
        for y in range(reshapesize[0]):
            dst_xy = np.array([x, y, 1]).astype(tfrom.dtype)
            src_xy = np.matmul(tfrom, dst_xy).astype(np.int32)
            if not (src_xy[0] >= reshapesize[1] or src_xy[0] < 0 or src_xy[1] >= reshapesize[0] or src_xy[1] < 0):
                dst[y, x] = inputs[src_xy[1], src_xy[0]]
    return dst



rgbafile = "../images/ImageFromBuffer.rgb"
bgrafile = "../images/ImageFromBuffer.png"
resultfile = "../result/ImageFromBufferFromPython.png"
fp = open(rgbafile, mode='rb')
rgba = fp.read()
fp.close()
imagesize = np.frombuffer(rgba, dtype=np.int32, count=2)
reshapesize = [imagesize.tolist()[1],imagesize.tolist()[0]]
reshapesize.extend([4,])
rgba = np.frombuffer(rgba, dtype=np.float32, offset=8).reshape(reshapesize)
# rgba = rgba * 255.0
# rgba = rgba.astype(np.uint8)
# rgb = rgba[:,:,:3]
bgra = cv2.cvtColor(rgba, cv2.COLOR_RGBA2BGRA)
bgr = cv2.cvtColor(bgra, cv2.COLOR_BGRA2BGR)
# cv2.imshow("show rgba image", bgr)
# cv2.waitKey(0)
bgrs = bgr * 255.0
bgrs = bgrs.astype(np.uint8)
cv2.imwrite(bgrafile, bgrs)
# 进行同C++代码一致的操作
# 首先曝光操作
bgr = np.sqrt(np.sqrt(bgr))
# 然后旋转操作
result = mywarpAffine(bgr, 0.3, 0.7)
# angle = 0.3
# scale = 0.7
#
# M = [[math.cos(angle)/scale, math.sin(angle)/scale], [-math.sin(angle)/scale, -math.cos(angle)/scale]]
#
# T = [0.5*(reshapesize[1]-(M[0][0]*reshapesize[1]+M[1][0]*reshapesize[0])),
#      0.5*(reshapesize[0]-(M[1][0]*reshapesize[1]+M[1][1]*reshapesize[0]))]
# # T = [-545.116211, 118.877533]
# tfrom = np.zeros((2,3))
# tfrom[:2,:2] = np.array(M)
# tfrom[:, 2] = np.array(T)
# tfrom = np.array([tfrom.tolist()[1], tfrom.tolist()[0]])
# # 求解其逆向解
# Mt = np.linalg.inv(tfrom[:2,:2])
# Tt = -np.matmul(Mt, tfrom[:,2])
# tfrom[:2,:2] = Mt
# tfrom[:, 2] = Tt
# result = cv2.warpAffine(bgr, tfrom, (reshapesize[1], reshapesize[0]), flags=cv2.WARP_INVERSE_MAP+cv2.INTER_CUBIC, borderMode=cv2.BORDER_CONSTANT, borderValue=[0,0,0])
result = result * 255.0
result = result.astype(np.uint8)
cv2.imwrite(resultfile, result)
pass