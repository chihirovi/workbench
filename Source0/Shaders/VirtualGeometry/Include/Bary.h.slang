#pragma once

/*
struct BarycentricDeriv{
    float3 m_lambda;
    float3 m_ddx;
    float3 m_ddy;
};

BarycentricDeriv CalcFullBary(float4 pt0, float4 pt1, float4 pt2, float2 pixelNdc, float2 winSize){

    BarycentricDeriv ret = (BarycentricDeriv)0;

    float3 invW = rcp(float3(pt0.w, pt1.w, pt2.w));

    float2 ndc0 = pt0.xy * invW.x;
    float2 ndc1 = pt1.xy * invW.y;
    float2 ndc2 = pt2.xy * invW.z;

    float invDet = rcp(determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1)));
    ret.m_ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet * invW;
    ret.m_ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet * invW;
    float ddxSum = dot(ret.m_ddx, float3(1, 1, 1));
    float ddySum = dot(ret.m_ddy, float3(1, 1, 1));

    float2 deltaVec = pixelNdc - ndc0;
    float interpInvW = invW.x + deltaVec.x * ddxSum + deltaVec.y * ddySum;
    float interpW = rcp(interpInvW);

    ret.m_lambda.x = interpW * (invW[0] + deltaVec.x * ret.m_ddx.x + deltaVec.y * ret.m_ddy.x);
    ret.m_lambda.y = interpW * (0.0f + deltaVec.x * ret.m_ddx.y + deltaVec.y * ret.m_ddy.y);
    ret.m_lambda.z = interpW * (0.0f + deltaVec.x * ret.m_ddx.z + deltaVec.y * ret.m_ddy.z);

    ret.m_ddx *= (2.0f / winSize.x);
    ret.m_ddy *= (2.0f / winSize.y);
    ddxSum *= (2.0f / winSize.x);
    ddySum *= (2.0f / winSize.y);

    ret.m_ddy *= -1.0f;
    ddySum *= -1.0f;

    float interpW_ddx = 1.0f / (interpInvW + ddxSum);
    float interpW_ddy = 1.0f / (interpInvW + ddySum);

    ret.m_ddx = interpW_ddx * (ret.m_lambda * interpInvW + ret.m_ddx) - ret.m_lambda;
    ret.m_ddy = interpW_ddy * (ret.m_lambda * interpInvW + ret.m_ddy) - ret.m_lambda;

    return ret;
}

float3 InterpolateWithDeriv(BarycentricDeriv deriv, float v0, float v1, float v2){
    float3 mergedV = float3(v0, v1, v2);
    float3 ret;
    ret.x = dot(mergedV, deriv.m_lambda);
    ret.y = dot(mergedV, deriv.m_ddx);
    ret.z = dot(mergedV, deriv.m_ddy);
    return ret;
}

void CalcDerivativeFloat2(BarycentricDeriv deriv, float2 v0, float2 v1, float2 v2, out float2 v, out float2 dx, out float2 dy){
	float3 x = InterpolateWithDeriv(deriv, v0.x, v1.x, v2.x);
	float3 y = InterpolateWithDeriv(deriv, v0.y, v1.y, v2.y);
	v = float2(x.x, y.x);
	dx = float2(x.y, y.y);
	dy = float2(x.z, y.z);
}

void CalcDerivativeFloat3(BarycentricDeriv deriv, float3 v0, float3 v1, float3 v2, out float3 v, out float3 dx, out float3 dy){
	float3 x = InterpolateWithDeriv(deriv, v0.x, v1.x, v2.x);
	float3 y = InterpolateWithDeriv(deriv, v0.y, v1.y, v2.y);
	float3 z = InterpolateWithDeriv(deriv, v0.z, v1.z, v2.z);
	v = float3(x.x, y.x, z.x);
	dx = float3(x.y, y.y, z.y);
	dy = float3(x.z, y.z, z.z);
}
*/

// =========================================


struct PerspectiveWeights
{
    // 指定位置における透視補正済み重心ウェイト。
    // x, y, z はそれぞれ頂点 0, 1, 2 に対応する。
    float3 value;

    // 画面上で右に 1 ピクセル移動したときのウェイトの変化量。
    float3 dx;

    // 画面上で下に 1 ピクセル移動したときのウェイトの変化量。
    float3 dy;
};


// 2D ベクトルの外積。
// 結果の符号は向きを、絶対値は平行四辺形の面積を表す。
float CrossProduct2D(float2 a, float2 b)
{
    return a.x * b.y - a.y * b.x;
}


// スクリーン空間上の三角形に対して、位置 p のアフィン重心座標を求める。
// この段階では透視補正を行わない。
float3 ComputeScreenBarycentrics(
    float2 p,
    float2 p0,
    float2 p1,
    float2 p2){

    // 三角形の符号付き面積の 2 倍。
    float area = CrossProduct2D(p1 - p0, p2 - p0);

    // 各頂点に対応する重心座標を、符号付き面積の比から求める。
    float weight0 = CrossProduct2D(p1 - p, p2 - p) / area;

    float weight1 = CrossProduct2D(p2 - p, p0 - p) / area;

    // 重心座標の合計は 1 なので、残りの成分は差し引きで求められる。
    float weight2 = 1.0f - weight0 - weight1;

    return float3(weight0, weight1, weight2);
}


// アフィン重心座標に透視補正を適用する。
// inverseW は各頂点のクリップ空間 w 成分の逆数。
float3 ApplyPerspectiveCorrection(
    float3 screenWeights,
    float3 inverseW){

    // 各頂点の重心座標に 1/w を乗じ、透視補正用の重みを作る。
    float3 correctedWeights = screenWeights * inverseW;

    // 重みの合計で除算し、合計が 1 になるように正規化する。
    float weightSum = correctedWeights.x + correctedWeights.y + correctedWeights.z;

    return correctedWeights / weightSum;
}


// 透視補正済み重心ウェイトと、画面上の X / Y 方向の差分を構築する。
// clip0～clip2 は各頂点のクリップ空間座標。
// pixelNdc は評価する位置の NDC 座標。
// viewportSize は描画領域の幅と高さ（ピクセル単位）。
PerspectiveWeights BuildPerspectiveWeights(
    float4 clip0,
    float4 clip1,
    float4 clip2,
    float2 pixelNdc,
    float2 viewportSize){

    PerspectiveWeights result;

    // クリップ空間から NDC への射影に使用する逆 w。
    float3 inverseW = 1.0f / float3(clip0.w, clip1.w, clip2.w);

    // 各頂点を NDC に変換し、スクリーン空間での三角形を得る。
    float2 p0 = clip0.xy * inverseW.x;
    float2 p1 = clip1.xy * inverseW.y;
    float2 p2 = clip2.xy * inverseW.z;

    // NDC 座標における 1 ピクセル分の移動量。
    // ここでは NDC の Y 軸が上向き、画面の Y 軸が下向きと仮定する。
    float2 pixelStepX = float2(2.0f / viewportSize.x, 0.0f);
    float2 pixelStepY = float2(0.0f, -2.0f / viewportSize.y);

    // 現在位置と、X / Y 方向に 1 ピクセル進んだ位置の重心座標を求める。
    float3 screenWeights = ComputeScreenBarycentrics(pixelNdc, p0, p1, p2);
    float3 screenWeightsX = ComputeScreenBarycentrics(pixelNdc + pixelStepX, p0, p1, p2);
    float3 screenWeightsY = ComputeScreenBarycentrics(pixelNdc + pixelStepY, p0, p1, p2);

    // 各位置の重心座標に透視補正を適用する。
    result.value = ApplyPerspectiveCorrection(screenWeights, inverseW);
    float3 weightsX = ApplyPerspectiveCorrection(screenWeightsX, inverseW);
    float3 weightsY = ApplyPerspectiveCorrection(screenWeightsY, inverseW);

    // 隣接位置との差分を計算する。
    result.dx = weightsX - result.value;
    result.dy = weightsY - result.value;

    return result;
}


// スカラー属性を補間し、値と X / Y 方向の差分を返す。
// 戻り値の x は補間値、y は X 方向の差分、z は Y 方向の差分。
float3 EvaluateAttributeAndGradient(
    PerspectiveWeights weights,
    float value0,
    float value1,
    float value2){

    float3 vertexValues = float3(value0, value1, value2);

    float interpolatedValue = dot(vertexValues, weights.value);
    float differenceX       = dot(vertexValues, weights.dx);
    float differenceY       = dot(vertexValues, weights.dy);

    return float3(interpolatedValue, differenceX, differenceY);
}


// float2 属性を透視補正付きで補間し、値と X / Y 方向の差分を返す。
void EvaluateFloat2Attribute(
    PerspectiveWeights weights,
    float2 value0,
    float2 value1,
    float2 value2,
    out float2 value,
    out float2 dx,
    out float2 dy){

    // X 成分と Y 成分を、それぞれスカラー属性として評価する。
    float3 resultX = EvaluateAttributeAndGradient(weights, value0.x, value1.x, value2.x);
    float3 resultY = EvaluateAttributeAndGradient(weights, value0.y, value1.y, value2.y);

    // 各スカラー評価結果から、ベクトル形式に組み直す。
    value = float2(resultX.x, resultY.x);
    dx    = float2(resultX.y, resultY.y);
    dy    = float2(resultX.z, resultY.z);
}


// float3 属性を透視補正付きで補間し、値と X / Y 方向の差分を返す。
void EvaluateFloat3Attribute(
    PerspectiveWeights weights,
    float3 value0,
    float3 value1,
    float3 value2,
    out float3 value,
    out float3 dx,
    out float3 dy){

    // X / Y / Z の各成分を個別に評価する。
    float3 resultX = EvaluateAttributeAndGradient(weights, value0.x, value1.x, value2.x);
    float3 resultY = EvaluateAttributeAndGradient(weights, value0.y, value1.y, value2.y);
    float3 resultZ = EvaluateAttributeAndGradient(weights, value0.z, value1.z, value2.z);

    // スカラー成分の結果を float3 にまとめる。
    value = float3(resultX.x, resultY.x, resultZ.x);
    dx    = float3(resultX.y, resultY.y, resultZ.y);
    dy    = float3(resultX.z, resultY.z, resultZ.z);
}

