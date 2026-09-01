#pragma once

#include "../../Include/GlobalDescriptor.h.slang"
#include "../../Include/ColorSpace.h.slang"
#include "../../Include/FetchTexture.h.slang"
#include "../../Include/Rt.h.slang"
#include "Rand.h.slang"
#include "LightSampleDesc.h.slang"
#include "ReservoirSample.h.slang"
#include "BSDF.h.slang"
#include "Reservoir.h.slang"

#define RAY_MAX_DEPTH_TR 8
#define RAY_MIN_DEPTH_TR 3

float3 path_trace_nee_sris_TR(
    RayDesc ray_desc, 
    inout Random random, 
    RaytracingAccelerationStructure as, 
    GPU_FrameRenderData frame_data,
    Texture2D<float4> texs[],
    SamplerState sampler_stats[],
    float Le_scale,
    RWStructuredBuffer<Reservoir<ReservoirSample>, ScalarDataLayout> reservoirs[2],
    uint32_t pixel_id_1d,
    Optional<int32_t2> pre_pixel_id,
    uint32_t frame_count) {

    float3 radiance = float3(0.0);
    float3 alpha = float3(1.0); // fs * cos / pdf
    
    // calc first hit 
    RtPayload payload;

    TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
    
    // miss
    if(payload.is_hit == false) return radiance;
    
    // hit
    VertexType0Attribute hit_surface = fetch_hit_point_info_linear_albedo_no_lod(
        ray_desc,
        0,
        payload,
        frame_data,
        texs,
        sampler_stats 
    );
    
    // back face check
    bool is_back_face_hit = dot(-ray_desc.Direction, hit_surface.normal) <= 0.0;
    if(is_back_face_hit) hit_surface.normal *= -1.0;
    
    // first hit
    radiance += hit_surface.emissive.rgb;

    for(int ray_idx = 0; ray_idx < RAY_MAX_DEPTH_TR; ray_idx++){
        BSDF bsdf = BSDF(
            hit_surface.roughness, 
            hit_surface.metalness, 
            hit_surface.specular, 
            hit_surface.normal, 
            hit_surface.albedo.rgb, 
            -ray_desc.Direction
        );

        // NEE + sris

        // sris light sampling
        Reservoir<LightSample> reservoir = Reservoir<LightSample>();

        uint32_t M = 16;

        for (int i = 0; i < M; i++) {
            LightSample light_sample = light_samples_4096[uint32_t(random.Get() * 4096.f)];

            // Fs * V * G * Le
            float p_hat = calc_p_hat(bsdf, hit_surface.position, light_sample);
            float inv_q = light_sample.inv_pdf;
            
            reservoir.update(light_sample, p_hat, p_hat*inv_q, 1, random.Get());
        }

        if (reservoir.p_hat_y > 0.0f) {
            reservoir.ucw = 1.0f / reservoir.p_hat_y * 1.0f / M * reservoir.weight_sum;
        }

        // shadow ray
        RtPayload shadow_ray_payload = shot_shadow_ray(as, hit_surface.position, reservoir.y.position);

        if (shadow_ray_payload.is_hit == true) {
            reservoir.weight_sum = 0.0f;
            reservoir.ucw = 0.0f;
            reservoir.p_hat_y = 0.0f;
        }
        
        // Temporal reuse
        // なんか O0, O3 で挙動が違う，コンパイラのバグ？

        /*
            RAY_MAX_DEPTH_TR が ２以上の場合，結果がバグる, 以下がその条件

            設定	                結果
            -O0 -fp-mode precise    OK
            -O0 -fp-mode fast	    NG
            -O3 -fp-mode precise    OK
            -O3 -fp-mode fast	    OK
            
            十中八九コンパイラのバグっぽい, 
            試しに手動で２ループ展開してみてたら，O0, fastでも動いた．

            // その他の実験結果
            reservoir 更新の，temp.w_sum = reservoir.w_sum; をコメントアウトすると，O0 fast でも動く，なんで？
            temp.weight_sum             = reservoir.p_hat_y; // これでも動く
            
            // claude との対話で出た結論
            確定した原因:
            w_sumというフィールドの存在自体は問題ではない（単純な式reservoir.p_hat_yを代入しても直った）
            問題はw_sumに代入していた式m_cur * reservoir.p_hat_y * reservoir.ucwという、
            PHI由来の依存グラフが深い（複数のループキャリー変数を掛け合わせた）式が、集約ストアの一部として存在すること

            つまり最終的な原因は：
            fast-math + 実ループ(PHIノードあり) + PHI由来の値を複数掛け合わせた複雑な式が
            構造体の1フィールドとしてUAVへのストアに含まれる

            という組み合わせで、O0のコード生成パスが壊れる、という結論でほぼ確定です。
            式の複雑さ（掛け算の連鎖・依存の深さ）が引き金であって、
            フィールド数やストアの命令数そのものが原因ではありませんでした。
        */
        if(ray_idx == 0){
            if(pre_pixel_id != none){
                uint32_t pre_id_1d = pre_pixel_id.value.y * frame_data.render_target_width + pre_pixel_id.value.x;
                Reservoir<ReservoirSample> temporal_reservoir = reservoirs[(frame_count+1)%2][pre_id_1d];

                // 信頼度の計算
                uint32_t r_M = min(temporal_reservoir.M, M * 32);
                let pre_first_hit_pos = temporal_reservoir.y.first_hit_position;
                let pre_first_hit_norm = temporal_reservoir.y.first_hit_normal.xyz;
                r_M = uint(float(r_M) * rejectionHeuristics(
                    frame_render_data.camera_position.xyz, 
                    pre_first_hit_pos, 
                    pre_first_hit_norm, 
                    hit_surface.position, 
                    hit_surface.normal
                ));
                
                // calc weight
                // Fs * V * G * Le
                let prev_light_sample = temporal_reservoir.y.light_sample;
                float p_hat_cur_y_prev = calc_p_hat(bsdf, hit_surface.position, prev_light_sample);

                /*
                // 1. 
                // まとめ，だと，sampling weight = m()*p_hat*UCW, UCW = 1/p_hat * sum(sampling weight)
                // とか書いてあったが，ここではm()の分母を抜き出して，UCW側に別で乗算してる
                // これは，cur reservoir, prev reservoir で同じ m() = M_cur/(M_cur + M_prev), M_prev/(M_cur + M_prev) を
                // を使いたいけど，M_prev はコードの順番的に，後ろのほうで取得されるから
                // 式変形して，定数の分母だけ抜き出して, UCWのつじつまだけ合わせてる感じ
                // (正確に言うと，Temporal Reuse のUCWの計算で，上書きしてるから平気
                // TRする前のUCWは，m() = 1/M_cur として計算してるから.
                // でも,分子の形は同じなので,sampling weight では問題なく
                // UCWの計算で，1/(M_cur + M_prev)として上書きしてる)
                */
                float3 prev_view_dir = normalize(frame_render_data.pre_camera_position.xyz - temporal_reservoir.y.first_hit_position);
                BSDF prev_bsdf = BSDF(
                    temporal_reservoir.y.first_hit_roughness/255.0f, 
                    temporal_reservoir.y.first_hit_metalness/255.0f, 
                    temporal_reservoir.y.first_hit_specular/255.0f, 
                    normalize(float3(temporal_reservoir.y.first_hit_normal)), 
                    temporal_reservoir.y.first_hit_albedo/255.0f, 
                    prev_view_dir
                );
                float p_hat_prev_y_cur = calc_p_hat(prev_bsdf, temporal_reservoir.y.first_hit_position, reservoir.y);

                // m_cur()
                float m_cur_denom = (r_M * p_hat_prev_y_cur + reservoir.M * reservoir.p_hat_y);
                float m_cur = m_cur_denom == 0.0f ? 0.0f : reservoir.M * reservoir.p_hat_y / m_cur_denom;
                //float m_cur = float(reservoir.M) / (r_M + reservoir.M);

                // m_prev()
                float m_prev_denom = (r_M * temporal_reservoir.p_hat_y + reservoir.M * p_hat_cur_y_prev);
                float m_prev = m_prev_denom == 0.0f ? 0.0f : r_M * temporal_reservoir.p_hat_y / m_prev_denom;
                //float m_prev = float(r_M) / (r_M + reservoir.M);

                // p_prev を考慮した weight に変換する
                reservoir.weight_sum = m_cur * reservoir.p_hat_y * reservoir.ucw; //m() * p_hat * ucw
                

                bool is_update = reservoir.update(
                    prev_light_sample, 
                    p_hat_cur_y_prev,
                    m_prev * p_hat_cur_y_prev * temporal_reservoir.ucw,
                    //r_M * p_hat_cur_y_prev * temporal_reservoir.ucw,
                    r_M,
                    random.Get()
                );

                if (reservoir.p_hat_y > 0.0f) {
                    reservoir.ucw = 1.0f / reservoir.p_hat_y * reservoir.weight_sum;
                    //reservoir.ucw = 1.0f / reservoir.p_hat_y * 1.0f/reservoir.M * reservoir.w_sum;
                }else{
                    reservoir.ucw = 0.0f;
                    reservoir.p_hat_y = 0.0f;
                    reservoir.weight_sum = 0.0f;
                }

                
                // spatial reuse するならここで

                // shadow ray payload を上書きする
                if(is_update){ // update した時だけ再度トレース
                    shadow_ray_payload = shot_shadow_ray(as, hit_surface.position, reservoir.y.position);
                    if (shadow_ray_payload.is_hit == true) {
                        reservoir.weight_sum = 0.0f;
                        reservoir.ucw = 0.0f;
                        reservoir.p_hat_y = 0.0f;
                    } 
                }
            }

            // store reservoir
            Reservoir<ReservoirSample> temp       = Reservoir<ReservoirSample>();
            temp.y.light_sample         = reservoir.y;
            temp.y.first_hit_position   = hit_surface.position;
            temp.y.first_hit_normal     = float16_t3(hit_surface.normal);
            temp.y.first_hit_albedo     = uint8_t3(hit_surface.albedo.xyz * 255.0f);
            temp.y.first_hit_roughness  = uint8_t(hit_surface.roughness * 255.0f);
            temp.y.first_hit_metalness  = uint8_t(hit_surface.metalness * 255.0f);
            temp.y.first_hit_specular   = uint8_t3(hit_surface.specular * 255.0f);
            temp.p_hat_y                = reservoir.p_hat_y;
            temp.M                      = reservoir.M;
            temp.ucw                    = reservoir.ucw;
            temp.weight_sum             = reservoir.weight_sum; 
            //temp.weight_sum           = reservoir.weight_sum; // ここをコメントアウトすると，O0 fast でも動く，なんで？
            //temp.weight_sum             = reservoir.p_hat_y; // これでも動く
            reservoirs[frame_count % 2][pixel_id_1d] = temp;
        }
        
        // nee radiance
        float3 light_dir = normalize(reservoir.y.position - hit_surface.position);
        float V = shadow_ray_payload.is_hit == false ? 1.0 : 0.0; //このshadow_ray_payloadは，ray_idx==0では意味が変わることに注意
        float3 Fs = bsdf.eval_fs(light_dir);
        float G = max(0, dot(hit_surface.normal, light_dir)) * max(0, dot(reservoir.y.normal, -light_dir)) 
            / dot(reservoir.y.position - hit_surface.position, reservoir.y.position - hit_surface.position);
        float3 Le = reservoir.y.emittanse * Le_scale;
        radiance += alpha * Fs * V * G * Le * reservoir.ucw;

        // debug
        if (any(isnan(radiance)) || any(isinf(radiance))) {
            return float3(1.0, 0.0, 1.0);
        }

        
        // Next ray
        
        // trace ray, ω_oは出射方向，ω_iは入射方向, 逆向きにトレースしてることに注意
        float3 omega_i = bsdf.sample_fs( float4(random.Get(), random.Get(), random.Get(), random.Get()));
        if (dot(omega_i, hit_surface.normal) <= 0) break;

        // update alpha (alpha = fs * cos / pdf)
        alpha *= bsdf.eval_fs(omega_i) * dot(hit_surface.normal, omega_i) * 1.0f / bsdf.eval_pdf(omega_i);

        
        // Russian Roulette
        if(RAY_MIN_DEPTH_TR <= ray_idx){
            float p = min(max3(alpha.x, alpha.y, alpha.z), 0.95);
            if(random.Get() > p) break;
            
            alpha /= p;
        }

        // next ray
        ray_desc.Origin         = hit_surface.position;
        ray_desc.Direction      = omega_i;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
        
        // miss
        if(payload.is_hit == false) break;
        
        // hit
        hit_surface = fetch_hit_point_info_linear_albedo_no_lod(
            ray_desc,
            0,
            payload,
            frame_data,
            texs,
            sampler_stats 
        );
        
        //  back face check
        bool is_back_face_hit = dot(-ray_desc.Direction, hit_surface.normal) <= 0.0;
        if (is_back_face_hit) hit_surface.normal *= -1.0;
    };

    return radiance;
}

float depthHeuristics(float depth, float previous_depth, float sigma){
    const float diff = (depth - previous_depth) * (depth - previous_depth) / depth;
    return clamp(exp(-sigma * diff), 0.0, 1.0);
}

float normalHeuristics(float3 normal, float3 previous_normal, float sigma){
    float d = dot(normal, previous_normal);
    return clamp(pow(max(d, 0.0), sigma), 0.0, 1.0);
}

float rejectionHeuristics(float3 camera_pos, float3 p0, float3 n0, float3 p1, float3 n1){
    float weight = 1.0;
    float DepthRejectionHeuristicsSigma = 0.5;
    float NormalRejectionHeuristicsSigma = 1.0;
    weight *= depthHeuristics(distance(camera_pos, p0), distance(camera_pos, p1), DepthRejectionHeuristicsSigma);
    weight *= normalHeuristics(n0, n1, NormalRejectionHeuristicsSigma);
    return weight;
}

float calc_p_hat(BSDF bsdf, float3 hit_pos, LightSample light_sample) {
    float3 light_dir = normalize(light_sample.position - hit_pos);
    float3 Fs = bsdf.eval_fs(light_dir);
    float G = max(0, dot(bsdf.normal, light_dir)) * max(0, dot(light_sample.normal, -light_dir)) 
        / dot(light_sample.position - hit_pos, light_sample.position - hit_pos);
    float3 Le = light_sample.emittanse;
    float p_hat = (Fs.r + Fs.g + Fs.b) * G * (Le.r + Le.g + Le.b);
    return p_hat;
}

RtPayload shot_shadow_ray(RaytracingAccelerationStructure as, float3 origin, float3 target){
    RayDesc shadow_ray_desc = calc_shadow_ray_desc(origin, target);
    RtPayload shadow_ray_payload;
    shadow_ray_payload.is_hit = true;
    TraceRay(as, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 0, shadow_ray_desc, shadow_ray_payload);
    return shadow_ray_payload;
}

RayDesc calc_shadow_ray_desc(float3 origin, float3 target) {
    RayDesc shadow_ray_desc;
    shadow_ray_desc.Origin = origin;
    shadow_ray_desc.Direction = normalize(target - shadow_ray_desc.Origin);
    shadow_ray_desc.TMin = 1e-5;
    shadow_ray_desc.TMax = length(target - shadow_ray_desc.Origin) - shadow_ray_desc.TMin;
    return shadow_ray_desc;
}

bool IsNaN(float x){
    return (asuint(x) & 0x7fffffff) > 0x7f800000;
}