cbuffer global
{
    matrix gWVP;
};

struct GS_IN
{
    float4 Pos : POSITION;
};

struct GS_OUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};
 
GS_IN VS(float4 Pos : POSITION)
{
    GS_IN Out;
    Out.Pos = Pos;
    return Out;
}
 
float4 PS(GS_OUT In) : SV_Target
{
    return In.Color;
}

[maxvertexcount(101)]
void GS(lineadj GS_IN gs_in[4], inout LineStream<GS_OUT> stream)
{
    GS_OUT Out;

    for (float t = 0.0f; t <= 1.0f; t += 0.01f)
    {
        float tInv = 1.0f - t;
        Out.Color = float4(t, tInv, 0, 1);
        Out.Pos = t * t * t * gs_in[0].Pos + t * t * tInv * gs_in[1].Pos + t * tInv * tInv * gs_in[2].Pos + tInv * tInv * tInv * gs_in[3].Pos;
        Out.Pos = mul(Out.Pos, gWVP);
        stream.Append(Out);
    }
    stream.RestartStrip();
}