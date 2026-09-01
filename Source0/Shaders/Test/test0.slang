[shader("compute")]
[numthreads(32,32,1)]
func main_comp(uint3 global_thread_id: SV_DispatchThreadID) {
    int32_t a = -1;
    uint32_t b = 1;

    //int + uint が uint + uintになっちゃう，注意
    int32_t c = a + b;
    uint32_t d = a + b;
}