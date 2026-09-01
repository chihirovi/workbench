#include "../Include/GlobalDescriptor.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/RayDirection.h.slang"

[shader("compute")]
[numthreads(32, 1, 1)]
void main_comp(
    uint3 global_thread_id: SV_DispatchThreadID,
    uint3 local_thread_id: SV_GroupThreadID,
    uint3 group_id : SV_GroupID) {

    uint32_t probe_index_1d = global_thread_id.x;
    uint32_t num_probes = ddgi_desc.sizes.x * ddgi_desc.sizes.y * ddgi_desc.sizes.z;
    if (probe_index_1d >= num_probes) return;
    
    // Read the current world position offset
    float3 offset = probe_relocation_offset[probe_index_1d].xyz;

    // Initialize variables
    int   closestBackfaceIndex = -1;
    int   closestFrontfaceIndex = -1;
    int   farthestFrontfaceIndex = -1;
    float closestBackfaceDistance = 1e27f;
    float closestFrontfaceDistance = 1e27f;
    float farthestFrontfaceDistance = 0.f;
    float backfaceCount = 0.f;

    for (int ray_idx = 0; ray_idx < DDGI_PROBE_NUM_RAY; ray_idx++) {
        float hit_dist = probe_trace_results[probe_index_1d*DDGI_PROBE_NUM_RAY + ray_idx].a;
        
        if(hit_dist < 0.f){
            // Found a backface
            backfaceCount++;

            hit_dist *= -1;
            if (hit_dist < closestBackfaceDistance) {
                // Store the closest backface distance and ray index
                closestBackfaceDistance = hit_dist;
                closestBackfaceIndex = ray_idx;
            }

        } else {
            // Found a frontface
            if (hit_dist < closestFrontfaceDistance){
                // Store the closest frontface distance and ray index
                closestFrontfaceDistance = hit_dist;
                closestFrontfaceIndex = ray_idx;

            }else if (hit_dist > farthestFrontfaceDistance){
                // Store the farthest frontface distance and ray index
                farthestFrontfaceDistance = hit_dist;
                farthestFrontfaceIndex = ray_idx;
            }

        }
    }
    
    float3 full_offset = float3(1e27f);
    
    if(closestBackfaceIndex != -1 && ((float)backfaceCount/DDGI_PROBE_NUM_RAY) > ddgi_desc.min_front_face_dist_ray_backface_threshold[1]){
        // If at least one backface triangle is hit AND backfaces are hit by enough probe rays,
        // assume the probe is inside geometry and move it outside of the geometry.
        float3 closestBackfaceDirection = get_probe_ray_direction(ddgi_desc, closestBackfaceIndex);
        full_offset = offset + (closestBackfaceDirection * (closestBackfaceDistance + ddgi_desc.min_front_face_dist_ray_backface_threshold[0] * 0.5f));

    } else if (closestFrontfaceDistance < ddgi_desc.min_front_face_dist_ray_backface_threshold[0]) {
        // Don't move the probe if moving towards the farthest frontface will also bring us closer to the nearest frontface
        float3 closestFrontfaceDirection = get_probe_ray_direction(ddgi_desc, closestFrontfaceIndex);
        float3 farthestFrontfaceDirection = get_probe_ray_direction(ddgi_desc, farthestFrontfaceIndex);

        if (dot(closestFrontfaceDirection, farthestFrontfaceDirection) <= 0.f){
            // Ensures the probe never moves through the farthest frontface
            farthestFrontfaceDirection *= min(farthestFrontfaceDistance, 0.5f);
            full_offset = offset + farthestFrontfaceDirection;
        }

    } else if (closestFrontfaceDistance >= ddgi_desc.min_front_face_dist_ray_backface_threshold[0]) {
        // Probe isn't near anything, try to move it back towards zero offset
        float moveBackMargin = min(closestFrontfaceDistance - ddgi_desc.min_front_face_dist_ray_backface_threshold[0], length(offset));
        float3 moveBackDirection = normalize(-offset);
        full_offset = offset + (moveBackMargin * moveBackDirection);
    }
    
    // Absolute maximum distance that probe could be moved should satisfy ellipsoid equation:
    // x^2 / probeGridSpacing.x^2 + y^2 / probeGridSpacing.y^2 + z^2 / probeGridSpacing.y^2 < (0.5)^2
    // Clamp to less than maximum distance to avoid degenerate cases
    float3 normalizedOffset = full_offset / ddgi_desc.scales.xyz;
    if (dot(normalizedOffset, normalizedOffset) < 0.2025f) { // 0.45 * 0.45 == 0.2025
        offset = full_offset;
    }
    
    // write the probe offsets
    probe_relocation_offset[probe_index_1d].xyz = offset;
    
    // clear
    if(frame_render_data.editor_data[0] == 1){
        probe_relocation_offset[probe_index_1d].xyz = float3(0.0);
    }
}











