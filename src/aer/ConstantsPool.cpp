#include "ConstantsPool.h"
#include "sl_consts.h"
#include <glm/ext/matrix_transform.hpp>
#include <sl_matrix_helpers.h>

namespace GlobalPool
{
    using namespace sl;
    Constants g_constants[CONSTANTS_HISTORY_SIZE]{};
    static glm::mat4 permutationRHToLH = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f));

    float4x4 getCameraToWorldMatrix(int frame) {
        const glm::mat4 finalViewRH  = get_final_view(frame);
//        auto cur_view = permutationRHToLH * finalViewRH * permutationRHToLH;
        auto cur_view = finalViewRH;
        float3 cameraRight = { cur_view[0].x, cur_view[0].y, cur_view[0].z };
        float3 cameraFwd    = { cur_view[2].x, cur_view[2].y, cur_view[2].z };
        float3 cameraPos  = { cur_view[3].x, cur_view[3].y, cur_view[3].z };
        float3 cameraUp;
        vectorNormalize(cameraRight);
        vectorNormalize(cameraFwd);
        vectorCrossProduct(cameraUp, cameraFwd, cameraRight);
        vectorNormalize(cameraUp);
        float4x4 cameraViewToWorld = {
            float4(cameraRight.x, cameraRight.y, cameraRight.z, 0.f),
            float4(cameraUp.x,    cameraUp.y,    cameraUp.z,    0.f),
            float4(cameraFwd.x,   cameraFwd.y,   cameraFwd.z,   0.f),
            float4(cameraPos.x,   cameraPos.y,   cameraPos.z,   1.f)
        };
        return cameraViewToWorld;
    }

    glm::mat4 get_correction_matrix(int frame, int past_frame) {
        //[Important] this call happening with same frame count as engine, however in real it is one off frame

        auto cameraViewToClip = *(float4x4*)&get_projection(frame-1);
        auto cameraViewToClipPrev = *(float4x4*)&get_projection(past_frame);
        auto cameraViewToWorld = getCameraToWorldMatrix(frame-1);
        auto cameraViewToWorldPrev = getCameraToWorldMatrix(past_frame-1);

        float4x4 clipToCameraView;
        matrixFullInvert(clipToCameraView, cameraViewToClip);

        float4x4 cameraViewToPrevCameraView;
        calcCameraToPrevCamera(cameraViewToPrevCameraView, cameraViewToWorld, cameraViewToWorldPrev);

        float4x4 clipToPrevCameraView;
        matrixMul(clipToPrevCameraView, clipToCameraView, cameraViewToPrevCameraView);
        glm::mat4 clipToPrevClip;
        matrixMul(*(float4x4*)&clipToPrevClip, clipToPrevCameraView, cameraViewToClipPrev);
        return clipToPrevClip;
    }

}