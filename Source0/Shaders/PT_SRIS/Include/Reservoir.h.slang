#pragma once

struct Reservoir<T>{
    T           y;          //output sample
    float32_t   p_hat_y;
    float32_t   ucw = 0.0f;
    uint32_t    M = 0;         // number of samples
    float32_t   weight_sum = 0.0f;  // sum of weights

    __init() {
        weight_sum = 0.0f; // sum of weights
        M = 0;     // number of samples
        p_hat_y = 0.0f;
        ucw = 0.0f;
    }

    // 基本的に，update weight = mis_weight * p_hat * ucw
    // で，ucw = 1/q, or,  1/p_hat * sum(weight)
    // mis_weight を update weight に含めるかどうかが設計の違い，定数の1/Mなら外で掛けたほうが負荷は小さい
    [mutating]
    bool update(T x_i, float p_hat, float w, uint32_t m, float rand) {
        weight_sum += w;
        M += m;
        if (rand <= (w / weight_sum)) { //ここで=を入れないと，１個もサンプルが入らない可能性が出てくる
            p_hat_y = p_hat;
            y = x_i;
            return true;
        }
        return false;
    }
};