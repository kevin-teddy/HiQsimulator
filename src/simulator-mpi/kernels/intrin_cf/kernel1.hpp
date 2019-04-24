using intrin::mul;
using intrin::load;
using intrin::load2;
using intrin::store;
using intrin::add;

namespace intrin_cf {

template <class V>
inline void kernel_compute(V &psi, std::size_t I, std::size_t d0, const __m256d m[], const __m256d mt[])
{
    __m256d v[2];

    v[0] = load2(&psi[I]);
    v[1] = load2(&psi[I + d0]);

    _mm256_storeu2_m128d((double*)&psi[I + d0], (double*)&psi[I], add(mul(v[0], m[0], mt[0]), mul(v[1], m[1], mt[1])));

}

template <class V, class M>
inline void kernel_core(V &psi, unsigned id0, M const& m, std::size_t ctrlmask)
{
    std::size_t n = psi.size();
    std::size_t d0 = 1UL << id0;

    __m256d mm[] = {load(&m[0][0], &m[1][0]), load(&m[0][1], &m[1][1])};
    __m256d mmt[2];

    __m256d neg = _mm256_setr_pd(1.0, -1.0, 1.0, -1.0);
    for (unsigned i = 0; i < 2; ++i){
        auto badc = _mm256_permute_pd(mm[i], 5);
        mmt[i] = _mm256_mul_pd(badc, neg);
    }

    std::size_t dsorted[] = {d0};

    if (ctrlmask == 0){
        #pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
        for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted[0]){
            for (std::size_t i1 = 0; i1 < dsorted[0]; ++i1){
                kernel_compute(psi, i0 + i1, d0, mm, mmt);
            }
        }
    }
    else{
        #pragma omp for collapse(LOOP_COLLAPSE1) schedule(static)
        for (std::size_t i0 = 0; i0 < n; i0 += 2 * dsorted[0]){
            for (std::size_t i1 = 0; i1 < dsorted[0]; ++i1){
                if (((i0 + i1)&ctrlmask) == ctrlmask)
                    kernel_compute(psi, i0 + i1, d0, mm, mmt);
            }
        }
    }
}

template <class V, class M, void K(V &, unsigned, M const&, std::size_t)>
inline void kernelK(V &v, unsigned id0, M const& m, std::size_t ctrlmask)
{
    K(v, id0, m, ctrlmask);
}

}
