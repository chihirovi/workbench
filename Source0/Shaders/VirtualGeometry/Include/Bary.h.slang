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

// スクリーン空間上の三角形について、アフィン重心座標と
// NDC 座標系における X / Y 方向の解析的微分を求める。
void ComputeScreenBarycentricsAndDerivatives(
    float2 p,
    float2 p0,
    float2 p1,
    float2 p2,
    out float3 weights,
    out float3 dWeightsDx,
    out float3 dWeightsDy){

    // 三角形の符号付き面積の 2 倍。
    float area = CrossProduct2D(p1 - p0, p2 - p0);

    // アフィン重心座標。
    // 各成分は、評価位置 p と三角形の頂点から作る
    // 符号付き面積を、三角形全体の面積で割ったもの。
    weights.x = CrossProduct2D(p1 - p, p2 - p) / area;
    weights.y = CrossProduct2D(p2 - p, p0 - p) / area;

    // 重心座標の合計は常に 1。
    weights.z = 1.0f - weights.x - weights.y;

    // アフィン重心座標はスクリーン座標に対して線形なので、
    // その微分は位置 p に依存せず、三角形の頂点だけで決まる。
    //
    // 以下は NDC の X 座標に関する偏微分。
    dWeightsDx = float3(
        p1.y - p2.y,
        p2.y - p0.y,
        p0.y - p1.y
    ) / area;

    // 以下は NDC の Y 座標に関する偏微分。
    dWeightsDy = float3(
        p2.x - p1.x,
        p0.x - p2.x,
        p1.x - p0.x
    ) / area;
}


// アフィン重心座標とその微分に透視補正を適用する。
// weights はアフィン重心座標、inverseW は各頂点の 1 / clip.w。
void ApplyPerspectiveCorrectionAndDerivatives(
    float3 weights,
    float3 dWeightsDxPerPixel,
    float3 dWeightsDyPerPixel,
    float3 inverseW,
    out float3 value,
    out float3 dx,
    out float3 dy){

    // 透視補正前の重心座標に、各頂点の逆 w を乗じる。
    float3 correctedWeights = weights * inverseW;

    // 透視補正の正規化に使う分母 S。
    float weightSum = correctedWeights.x + correctedWeights.y + correctedWeights.z;

    // 透視補正済み重心座標。
    value = correctedWeights / weightSum;

    // 分母 S の X / Y 方向の微分。
    //
    // inverseW は頂点ごとの定数なので、
    // dS/dx = sum(inverseW_i * dWeights_i/dx)
    // dS/dy = sum(inverseW_i * dWeights_i/dy)
    float dWeightSumDx = dot(dWeightsDxPerPixel, inverseW);
    float dWeightSumDy = dot(dWeightsDyPerPixel, inverseW);

    // 透視補正済み重心座標の解析的微分。
    //
    // lambda_i = correctedWeights_i / S より、
    // d(lambda_i)/dx = (inverseW_i * dWeights_i/dx - lambda_i * dS/dx) / S
    // Y 方向も同様。
    dx = (dWeightsDxPerPixel * inverseW - value * dWeightSumDx) / weightSum;

    dy = (dWeightsDyPerPixel * inverseW - value * dWeightSumDy) / weightSum;
}


// クリップ空間の三角形から、評価位置における透視補正済み重心ウェイトと
// 画面上で 1 ピクセル移動したときのウェイトの変化量を求める。
//
// clip0～clip2 : 各頂点のクリップ空間座標。
// pixelNdc     : 評価位置の NDC 座標。
// viewportSize : ビューポートの幅と高さ（ピクセル単位）。
//
// NDC の Y 軸は上向き、画面の Y 軸は下向きと仮定する。
PerspectiveWeights BuildPerspectiveWeights(
    float4 clip0,
    float4 clip1,
    float4 clip2,
    float2 pixelNdc,
    float2 viewportSize){

    PerspectiveWeights result;

    // クリップ空間から NDC への射影に使う逆 w。
    float3 inverseW = 1.0f / float3(clip0.w, clip1.w, clip2.w);

    // 各頂点を NDC に変換する。
    float2 p0 = clip0.xy * inverseW.x;
    float2 p1 = clip1.xy * inverseW.y;
    float2 p2 = clip2.xy * inverseW.z;

    // NDC 座標におけるアフィン重心座標と、その解析的微分。
    float3 screenWeights;
    float3 dWeightsDx;
    float3 dWeightsDy;

    ComputeScreenBarycentricsAndDerivatives(
        pixelNdc,
        p0, p1, p2,
        screenWeights,
        dWeightsDx,
        dWeightsDy
    );

    // NDC 座標の微分を、画面上の 1 ピクセルあたりの微分に変換する。
    //
    // X 方向:
    //   NDC は 1 ピクセルあたり 2 / width 変化する。
    //
    // Y 方向:
    //   NDC の Y 軸は上向きだが、画面の Y 軸は下向きなので符号を反転する。
    dWeightsDx *= 2.0f / viewportSize.x;
    dWeightsDy *= -2.0f / viewportSize.y;

    // 透視補正後の重心座標と、その解析的微分を求める。
    ApplyPerspectiveCorrectionAndDerivatives(
        screenWeights,
        dWeightsDx,
        dWeightsDy,
        inverseW,
        result.value,
        result.dx,
        result.dy
    );

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

